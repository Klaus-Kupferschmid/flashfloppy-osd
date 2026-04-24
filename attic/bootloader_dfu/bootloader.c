/*
 * bootloader.c
 *
 * USB-DFU Bootloader for FlashFloppy-OSD.
 * Entry via PA4 (Boot-Select) button held at power-on,
 * or via magic RAM value from application.
 *
 * LED feedback on PB2 (BluePill on-board LED, active low):
 *   DFU Ready:    Heartbeat (50-50-50-850ms)
 *   USB Connected: Slow breathing (500ms on/off)
 *   Flashing:     Fast blink (50ms on/off)
 *   Success:      Solid 3 seconds, then reset
 *   Error:        3x fast blink, 1s pause, repeat
 *
 * Written for FlashFloppy-OSD project.
 * This is free and unencumbered software released into the public domain.
 */

#include <stdint.h>

/*
 * Memory map:
 *   0x08000000 - 0x08001FFF: Bootloader (8KB)
 *   0x08002000 - 0x0801F7FF: Application (120KB - 2KB)
 *   0x0801F800 - 0x0801FFFF: Flash storage (2KB)
 */
#define APP_ADDRESS     0x08002000
#define BOOTLOADER_SIZE 0x2000

/* Magic value in RAM to trigger DFU from application */
#define DFU_MAGIC_VALUE 0xDEADBEEF
#define DFU_MAGIC_ADDR  0x20004FF0  /* End of 20KB RAM minus 16 bytes */

/* Register base addresses */
#define RCC_BASE        0x40021000
#define GPIOA_BASE      0x40010800
#define GPIOB_BASE      0x40010C00
#define FLASH_BASE      0x40022000
#define USB_BASE        0x40005C00
#define USB_PMA_BASE    0x40006000
#define SCB_BASE        0xE000ED00
#define STK_BASE        0xE000E010

/* RCC registers */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18))

/* RCC bits */
#define RCC_CR_HSEON    (1u << 16)
#define RCC_CR_HSERDY   (1u << 17)
#define RCC_CR_PLLON    (1u << 24)
#define RCC_CR_PLLRDY   (1u << 25)
#define RCC_CFGR_SW_PLL (2u << 0)
#define RCC_CFGR_SWS_PLL (2u << 2)
#define RCC_CFGR_PLLSRC (1u << 16)
#define RCC_CFGR_PLLMUL6 (4u << 18)  /* 8MHz * 6 = 48MHz */
#define RCC_APB1ENR_USBEN (1u << 23)
#define RCC_APB2ENR_IOPAEN (1u << 2)
#define RCC_APB2ENR_IOPBEN (1u << 3)

/* GPIO registers */
#define GPIOA_CRL       (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_IDR       (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOB_CRL       (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_ODR       (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))
#define GPIOB_BSRR      (*(volatile uint32_t *)(GPIOB_BASE + 0x10))

/* Flash registers */
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_BASE + 0x0C))
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_AR        (*(volatile uint32_t *)(FLASH_BASE + 0x14))

/* Flash bits */
#define FLASH_SR_BSY    (1u << 0)
#define FLASH_SR_EOP    (1u << 5)
#define FLASH_CR_PG     (1u << 0)
#define FLASH_CR_PER    (1u << 1)
#define FLASH_CR_STRT   (1u << 6)
#define FLASH_CR_LOCK   (1u << 7)
#define FLASH_KEY1      0x45670123
#define FLASH_KEY2      0xCDEF89AB

/* SysTick registers */
#define STK_CTRL        (*(volatile uint32_t *)(STK_BASE + 0x00))
#define STK_LOAD        (*(volatile uint32_t *)(STK_BASE + 0x04))
#define STK_VAL         (*(volatile uint32_t *)(STK_BASE + 0x08))

#define STK_CTRL_ENABLE (1u << 0)
#define STK_CTRL_CLKSRC (1u << 2)
#define STK_CTRL_COUNTFLAG (1u << 16)

/* SCB registers */
#define SCB_AIRCR       (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define SCB_AIRCR_VECTKEY (0x05FA << 16)
#define SCB_AIRCR_SYSRESETREQ (1u << 2)

/* Pin definitions */
#define PIN_BOOT_SELECT 4   /* PA4 */
#define PIN_LED         2   /* PB2 */

/* LED states */
typedef enum {
    LED_DFU_READY,      /* Heartbeat: 50-50-50-850ms */
    LED_USB_CONNECTED,  /* Slow: 500ms on/off */
    LED_FLASHING,       /* Fast: 50ms on/off */
    LED_SUCCESS,        /* Solid 3s then reset */
    LED_ERROR           /* 3x fast, 1s pause */
} led_state_t;

/* Global state */
static volatile uint32_t tick_ms;
static uint8_t usb_configured __attribute__((unused));

/* Forward declarations */
static void clock_init(void);
static void gpio_init(void);
static void systick_init(void);
static void delay_ms(uint32_t ms) __attribute__((unused));
static void early_delay(uint32_t ms);
static void debug_blink(int n);
static int check_dfu_trigger(void);
static void jump_to_app(void);
static void led_update(void);
static void led_on(void);
static void led_off(void);
static void system_reset(void);

/* External USB DFU functions */
extern void usb_dfu_init(void);
extern void usb_dfu_poll(void);

/* Exported for usb_dfu.c */
led_state_t led_state = LED_DFU_READY;

/* Reset handler - declared before vector table */
void Reset_Handler(void) __attribute__((noreturn));

/* Vector table for bootloader */
__attribute__((section(".vector_table")))
const uint32_t vector_table[] = {
    0x20005000,                 /* Initial stack pointer (end of 20KB RAM) */
    (uint32_t)&Reset_Handler,   /* Reset handler */
};

void Reset_Handler(void)
{
    /* Initialize .data and .bss (minimal bootloader doesn't need much) */
    extern uint32_t _sdat, _edat, _ldat, _sbss, _ebss;
    uint32_t *src = &_ldat;
    uint32_t *dst = &_sdat;
    while (dst < &_edat) *dst++ = *src++;
    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0;

    /* Check if we should enter DFU mode */
    if (!check_dfu_trigger()) {
        /* No DFU trigger - jump to application */
        jump_to_app();
    }

    /* Clear magic value */
    *(volatile uint32_t *)DFU_MAGIC_ADDR = 0;

    /* Enter DFU mode */
    clock_init();
    gpio_init();
    systick_init();
    usb_dfu_init();

    led_state = LED_DFU_READY;

    /* Main bootloader loop */
    while (1) {
        usb_dfu_poll();
        led_update();
    }
}

/*
 * Simple blocking delay (before SysTick is initialized).
 * At reset, running on 8MHz HSI, ~8 cycles per iteration.
 */
static void early_delay(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < ms * 1000; i++);
}

/*
 * Debug LED blink (before full init).
 * Blinks PB2 n times quickly to show bootloader is running.
 */
static void debug_blink(int n)
{
    /* Enable GPIOB clock */
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN;
    early_delay(1);
    
    /* Configure PB2 as push-pull output (CNF=00, MODE=01 = 10MHz) */
    GPIOB_CRL = (GPIOB_CRL & ~(0xF << (2 * 4))) | (0x1 << (2 * 4));
    
    for (int i = 0; i < n; i++) {
        GPIOB_BSRR = (1 << (PIN_LED + 16));  /* LED on (active low) */
        early_delay(100);
        GPIOB_BSRR = (1 << PIN_LED);         /* LED off */
        early_delay(100);
    }
    early_delay(200);
}

/*
 * Check if DFU mode should be entered:
 * 1. PA4 (Boot-Select) button is held low at power-on
 * 2. Magic value in RAM (set by application for soft-DFU)
 */
static int check_dfu_trigger(void)
{
    int pressed_count = 0;
    
    /* Check magic RAM value FIRST (before any GPIO setup) */
    if (*(volatile uint32_t *)DFU_MAGIC_ADDR == DFU_MAGIC_VALUE) {
        debug_blink(3);  /* 3 blinks = magic trigger */
        return 1;
    }
    
    /* Enable GPIOA clock for button check */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;
    
    /* Longer delay for GPIO clock to stabilize */
    early_delay(5);
    
    /* Configure PA4 as input with pull-up (CNF=10, MODE=00) */
    GPIOA_CRL = (GPIOA_CRL & ~(0xF << (4 * 4))) | (0x8 << (4 * 4));
    /* Set pull-up via ODR */
    *(volatile uint32_t *)(GPIOA_BASE + 0x0C) |= (1 << PIN_BOOT_SELECT);
    
    /* Give pull-up time to charge any capacitance */
    early_delay(10);
    
    /* Debug: show bootloader is running (1 blink) */
    debug_blink(1);
    
    /* Check button multiple times to debounce (active low) */
    for (int i = 0; i < 5; i++) {
        if (!(GPIOA_IDR & (1 << PIN_BOOT_SELECT))) {
            pressed_count++;
        }
        early_delay(10);
    }
    
    /* Need at least 3 out of 5 reads to confirm button pressed */
    if (pressed_count >= 3) {
        debug_blink(2);  /* 2 blinks = button trigger */
        return 1;
    }
    
    return 0;  /* No trigger, boot normally */
}

/*
 * Jump to application at APP_ADDRESS.
 * Application vector table has SP at offset 0, reset handler at offset 4.
 */
static void jump_to_app(void)
{
    uint32_t app_sp = *(volatile uint32_t *)(APP_ADDRESS);
    uint32_t app_reset = *(volatile uint32_t *)(APP_ADDRESS + 4);
    
    /* Sanity check: SP should be in RAM (0x20000000 - 0x20005000) */
    if ((app_sp < 0x20000000) || (app_sp > 0x20005000)) {
        /* Invalid application - stay in bootloader */
        return;
    }
    
    /* Disable all interrupts */
    __asm volatile ("cpsid i");
    
    /* Disable SysTick */
    STK_CTRL = 0;
    
    /* Reset GPIO clocks */
    RCC_APB2ENR = 0;
    
    /* Set vector table offset to application */
    *(volatile uint32_t *)(SCB_BASE + 0x08) = APP_ADDRESS;
    
    /* Set stack pointer and jump */
    __asm volatile (
        "msr msp, %0\n"
        "bx %1\n"
        : : "r" (app_sp), "r" (app_reset)
    );
    
    /* Never reached */
    while (1);
}

/*
 * Configure clocks: HSE 8MHz -> PLL x6 -> 48MHz SYSCLK
 * Required for USB (needs 48MHz)
 */
static void clock_init(void)
{
    /* Enable HSE */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY));
    
    /* Configure flash latency for 48MHz (1 wait state) */
    FLASH_ACR = (FLASH_ACR & ~0x7) | 0x1;
    
    /* Configure PLL: HSE * 6 = 48MHz */
    RCC_CFGR = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL6;
    
    /* Enable PLL */
    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY));
    
    /* Switch to PLL */
    RCC_CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & (3 << 2)) != RCC_CFGR_SWS_PLL);
    
    /* Enable USB clock (must be done after PLL is running at 48MHz) */
    RCC_APB1ENR |= RCC_APB1ENR_USBEN;
}

/*
 * Configure GPIO:
 * - PB2 as push-pull output (LED)
 * - PA11/PA12 for USB (configured by USB peripheral)
 */
static void gpio_init(void)
{
    /* Enable GPIOA and GPIOB clocks */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
    
    /* PB2 as push-pull output, 2MHz */
    GPIOB_CRL = (GPIOB_CRL & ~(0xF << (2 * 4))) | (0x2 << (2 * 4));
    
    /* PA11 (USB_DM) and PA12 (USB_DP): Let USB peripheral control them
     * Default after reset is floating input, which is correct for USB */
    
    /* LED off initially (active low, so set high) */
    led_off();
}

/*
 * Configure SysTick for 1ms ticks.
 * At 48MHz: 48000 ticks = 1ms
 */
static void systick_init(void)
{
    STK_LOAD = 48000 - 1;
    STK_VAL = 0;
    STK_CTRL = STK_CTRL_CLKSRC | STK_CTRL_ENABLE;
    tick_ms = 0;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = tick_ms;
    while ((tick_ms - start) < ms) {
        if (STK_CTRL & STK_CTRL_COUNTFLAG) {
            tick_ms++;
        }
    }
}

static void led_on(void)
{
    /* Active low: clear bit to turn on */
    GPIOB_BSRR = (1 << (PIN_LED + 16));
}

static void led_off(void)
{
    /* Active low: set bit to turn off */
    GPIOB_BSRR = (1 << PIN_LED);
}

/*
 * LED state machine - non-blocking.
 * Called from main loop.
 */
static void led_update(void)
{
    static uint32_t last_tick;
    static uint8_t phase;
    static uint8_t error_count;
    static uint32_t success_start;
    
    /* Update tick counter */
    if (STK_CTRL & STK_CTRL_COUNTFLAG) {
        tick_ms++;
    }
    
    uint32_t elapsed = tick_ms - last_tick;
    
    switch (led_state) {
    case LED_DFU_READY:
        /* Heartbeat: 50ms on, 50ms off, 50ms on, 850ms off */
        switch (phase) {
        case 0: /* First pulse on */
            led_on();
            if (elapsed >= 50) { last_tick = tick_ms; phase = 1; }
            break;
        case 1: /* First pulse off */
            led_off();
            if (elapsed >= 50) { last_tick = tick_ms; phase = 2; }
            break;
        case 2: /* Second pulse on */
            led_on();
            if (elapsed >= 50) { last_tick = tick_ms; phase = 3; }
            break;
        case 3: /* Long pause */
            led_off();
            if (elapsed >= 850) { last_tick = tick_ms; phase = 0; }
            break;
        }
        break;
        
    case LED_USB_CONNECTED:
        /* Slow breathing: 500ms on, 500ms off */
        if (phase == 0) {
            led_on();
            if (elapsed >= 500) { last_tick = tick_ms; phase = 1; }
        } else {
            led_off();
            if (elapsed >= 500) { last_tick = tick_ms; phase = 0; }
        }
        break;
        
    case LED_FLASHING:
        /* Fast: 50ms on, 50ms off */
        if (phase == 0) {
            led_on();
            if (elapsed >= 50) { last_tick = tick_ms; phase = 1; }
        } else {
            led_off();
            if (elapsed >= 50) { last_tick = tick_ms; phase = 0; }
        }
        break;
        
    case LED_SUCCESS:
        /* Solid for 3 seconds, then reset */
        led_on();
        if (success_start == 0) {
            success_start = tick_ms;
        } else if ((tick_ms - success_start) >= 3000) {
            system_reset();
        }
        break;
        
    case LED_ERROR:
        /* 3x fast blink, 1s pause */
        if (error_count < 6) {
            /* Blinking phase */
            if (phase == 0) {
                led_on();
                if (elapsed >= 100) { last_tick = tick_ms; phase = 1; error_count++; }
            } else {
                led_off();
                if (elapsed >= 100) { last_tick = tick_ms; phase = 0; }
            }
        } else {
            /* Pause phase */
            led_off();
            if (elapsed >= 1000) { last_tick = tick_ms; error_count = 0; }
        }
        break;
    }
}

static void system_reset(void)
{
    SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
    while (1);
}

/* Minimal exception handlers */
void Default_Handler(void) __attribute__((weak));
void Default_Handler(void)
{
    while (1);
}
