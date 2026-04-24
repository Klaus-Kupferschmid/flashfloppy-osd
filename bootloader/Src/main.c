/*
* STM32 HID Bootloader - USB HID bootloader for STM32F10X
* Copyright (c) 2018 Bruno Freitas - bruno@brunofreitas.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Modified 20 April 2018
*	by Vassilis Serasidis <info@serasidis.gr>
*	This HID bootloader works with STM32F103 + STM32duino + Arduino IDE <http://www.stm32duino.com/>
*
* Modified January 2019
*	by Michel Stempin <michel.stempin@wanadoo.fr>
*	Cleanup and optimizations
*
* Modified April 2026
*	by Klaus Kupferschmid
*	Added I2C2 Frontpanel LED support and sophisticated state machine for FlashFloppy OSD
*/

#include <stm32f10x.h>
#include <stdbool.h>
#include "usb.h"
#include "config.h"
#include "hid.h"
#include "led.h"

/* Bootloader size - increased to 4KB for I2C support */
#define BOOTLOADER_SIZE			(4 * 1024)

/* SRAM size */
#define SRAM_SIZE			(20 * 1024)

/* SRAM end (bottom of stack) */
#define SRAM_END			(SRAM_BASE + SRAM_SIZE)

/* HID Bootloader takes 4 kb flash. */
#define USER_PROGRAM			(FLASH_BASE + BOOTLOADER_SIZE)

/* Initial stack pointer index in vector table*/
#define INITIAL_MSP			0

/* Reset handler index in vector table*/
#define RESET_HANDLER			1

/* USB Low-Priority and CAN1 RX0 IRQ handler idnex in vector table */
#define USB_LP_CAN1_RX0_IRQ_HANDLER	36

/* I2C2 ADG715 addresses */
#define ADG715_FP_ADDR			0x90  /* 0x48 << 1 */
#define LED_BLUE			0x10  /* Blue LED (DF1) */
#define LED_RED				0x02  /* Red LED (DF2) */
#define LED_PINK			(LED_RED | LED_BLUE)  /* Both = Pink */

/* Timing constants (calibrated for 72MHz, ~100ms per delay unit) */
#define DELAY_100MS			1285000L
#define DELAY_50MS			642500L
#define DELAY_25MS			321250L

/* Button hold time for abort (40 iterations * 100ms = 4 seconds) */
#define ABORT_HOLD_COUNT		40

/* Success/Error display time (30 iterations * 100ms = 3 seconds) */
#define RESULT_DISPLAY_COUNT		30

/* Bootloader states */
#define STATE_WAIT_RELEASE		0  /* Blue blink, waiting for button release */
#define STATE_NO_USB			1  /* Blue/Red alternating, no USB */
#define STATE_USB_READY			2  /* Red/Pink alternating, USB ready */
#define STATE_FLASHING			3  /* Fast pink blink, flashing */
#define STATE_ABORT_COOLDOWN		6  /* Wait for 4s no-press before app */
#define STATE_SUCCESS			4  /* Solid pink, success */
#define STATE_ERROR			5  /* Fast red blink, error */

/* Simple function pointer type to call user program */
typedef void (*funct_ptr)(void);

/* The bootloader entry point function prototype */
void Reset_Handler(void);

/* Minimal initial Flash-based vector table */
uint32_t *VectorTable[] __attribute__((section(".isr_vector"))) = {
	/* Initial stack pointer (MSP) */
	(uint32_t *) SRAM_END,
	/* Initial program counter (PC): Reset handler */
	(uint32_t *) Reset_Handler
};

static void delay(uint32_t timeout)
{
	for (uint32_t i = 0; i < timeout; i++) {
		__NOP();
	}
}

#if defined TARGET_FLASHFLOPPY_OSD
/* Initialize I2C2 for Frontpanel communication */
static void i2c2_init(void)
{
	/* Enable GPIOB and I2C2 clocks */
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPBEN);
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);

	/* PB10 (SCL2) and PB11 (SDA2) as AF open-drain, 2MHz */
	MODIFY_REG(GPIOB->CRH,
		GPIO_CRH_CNF10 | GPIO_CRH_MODE10,
		GPIO_CRH_CNF10_0 | GPIO_CRH_CNF10_1 | GPIO_CRH_MODE10_1);
	MODIFY_REG(GPIOB->CRH,
		GPIO_CRH_CNF11 | GPIO_CRH_MODE11,
		GPIO_CRH_CNF11_0 | GPIO_CRH_CNF11_1 | GPIO_CRH_MODE11_1);

	/* Reset I2C2 */
	SET_BIT(RCC->APB1RSTR, RCC_APB1RSTR_I2C2RST);
	CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_I2C2RST);

	/* Configure I2C2: 100kHz standard mode */
	WRITE_REG(I2C2->CR2, 36);
	WRITE_REG(I2C2->CCR, 180);
	WRITE_REG(I2C2->TRISE, 37);
	SET_BIT(I2C2->CR1, I2C_CR1_PE);
}

/* Send a byte to ADG715 via I2C2 - with interrupt protection */
static void i2c2_send(uint8_t addr, uint8_t data)
{
	uint32_t timeout;

	SET_BIT(I2C2->CR1, I2C_CR1_START);
	timeout = 10000;
	while (!READ_BIT(I2C2->SR1, I2C_SR1_SB) && --timeout);
	if (!timeout) return;

	WRITE_REG(I2C2->DR, addr);
	timeout = 10000;
	while (!READ_BIT(I2C2->SR1, I2C_SR1_ADDR) && --timeout);
	if (!timeout) { SET_BIT(I2C2->CR1, I2C_CR1_STOP); return; }
	(void)I2C2->SR2;

	WRITE_REG(I2C2->DR, data);
	timeout = 10000;
	while (!READ_BIT(I2C2->SR1, I2C_SR1_BTF) && --timeout);

	SET_BIT(I2C2->CR1, I2C_CR1_STOP);
}

/* Set Frontpanel LEDs - DISABLE INTERRUPTS during I2C to prevent USB collision */
static void fp_set_leds(uint8_t pattern)
{
	__disable_irq();  /* Block USB interrupts during I2C */
	i2c2_send(ADG715_FP_ADDR, pattern);
	__enable_irq();   /* Re-enable - USB hardware buffers packets */
}

/* Check if PA4 button is pressed */
static bool button_pressed(void)
{
	return BOOT_BUTTON_PRESSED();
}
#endif /* TARGET_FLASHFLOPPY_OSD */

static bool check_user_code(uint32_t user_address)
{
	uint32_t sp = *(volatile uint32_t *) user_address;

	/* Check if the stack pointer in the vector table points
	   somewhere in SRAM */
	return ((sp & 0x2FFE0000) == SRAM_BASE) ? true : false;
}

static uint16_t get_and_clear_magic_word(void)
{

	/* Enable the power and backup interface clocks by setting the
	 * PWREN and BKPEN bits in the RCC_APB1ENR register
	 */
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
	uint16_t value = READ_REG(BKP->DR4);

	/* ALWAYS clear the register - even if already 0.
	 * This handles random values after first flash. */
	SET_BIT(PWR->CR, PWR_CR_DBP);
	WRITE_REG(BKP->DR4, 0x0000);
	CLEAR_BIT(PWR->CR, PWR_CR_DBP);

	CLEAR_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
	return value;
}

static void set_sysclock_to_72_mhz(void)
{

	/* Enable HSE */
	SET_BIT(RCC->CR, RCC_CR_HSEON);

	/* Wait until HSE is ready */
	while (READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0) {
		;
	}

	/* Enable Prefetch Buffer & set Flash access to 2 wait states */
	SET_BIT(FLASH->ACR, FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2);

	/* SYSCLK = PCLK2 = HCLK */
	/* PCLK1 = HCLK / 2 */
	/* PLLCLK = HSE * 9 = 72 MHz */
	SET_BIT(RCC->CFGR,
		RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_PPRE1_DIV2 |
		RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);

	/* Enable PLL */
	SET_BIT(RCC->CR, RCC_CR_PLLON);

	/* Wait until PLL is ready */
	while (READ_BIT(RCC->CR, RCC_CR_PLLRDY) == 0) {
		;
	}

	/* Select PLL as system clock source */
	SET_BIT(RCC->CFGR, RCC_CFGR_SW_PLL);

	/* Wait until PLL is used as system clock source */
	while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS_1) == 0) {
		;
	}
}

void Reset_Handler(void)
{
	volatile uint32_t *const ram_vectors =
		(volatile uint32_t *const) SRAM_BASE;

	/* Setup the system clock */
	set_sysclock_to_72_mhz();

	/* Setup a temporary vector table into SRAM for USB IRQs */
	ram_vectors[INITIAL_MSP] = SRAM_END;
	ram_vectors[RESET_HANDLER] = (uint32_t) Reset_Handler;
	ram_vectors[USB_LP_CAN1_RX0_IRQ_HANDLER] =
		(uint32_t) USB_LP_CAN1_RX0_IRQHandler;
	WRITE_REG(SCB->VTOR, (volatile uint32_t) ram_vectors);

	/* Check for magic word in BACKUP memory */
	uint16_t magic_word = get_and_clear_magic_word();

	/* Initialize GPIOs */
	pins_init();

#if defined TARGET_FLASHFLOPPY_OSD
	/* Initialize I2C2 for Frontpanel LEDs */
	i2c2_init();
	fp_set_leds(0);
#endif

	delay(72);
	LED2_OFF;

	UploadStarted = false;
	UploadFinished = false;
	UsbEnumerated = false;  /* Initialize USB enumeration flag */
	funct_ptr UserProgram =
		(funct_ptr) *(volatile uint32_t *) (USER_PROGRAM + 0x04);

	/* Enter bootloader if magic word, button pressed, or no user code */
#if defined TARGET_FLASHFLOPPY_OSD
	/* LED Sequence BEFORE USB - I2C safe here! */
	/* Phase 1: Blue blink while button pressed (max 4s) */
	/* Phase 2: Red/Blue alternating for 2s after button release */
	/* Phase 3: Set final color, then USB_Init (NO I2C after this!) */
	if ((magic_word == 0x424C) ||
		button_pressed() ||
		(check_user_code(USER_PROGRAM) == false)) {
		
		if (magic_word == 0x424C) {
			USB_Shutdown();
			delay(4000000L);
		}
		
		/* PHASE 1: Blue blink while button pressed (max 4 seconds) */
		for (int i = 0; i < 40 && button_pressed(); i++) {
			fp_set_leds(LED_BLUE);
			LED1_ON;
			delay(DELAY_50MS);
			fp_set_leds(0);
			LED1_OFF;
			delay(DELAY_50MS);
		}
		
		/* PHASE 2: Red/Blue alternating for 2 seconds (button released) */
		for (int i = 0; i < 10; i++) {
			fp_set_leds(LED_RED);
			LED1_ON;
			delay(DELAY_100MS);
			fp_set_leds(LED_BLUE);
			LED1_OFF;
			delay(DELAY_100MS);
		}
		
		/* PHASE 3: Set Pink LED for USB wait state */
		fp_set_leds(LED_PINK);
		LED1_ON;
		delay(DELAY_100MS);  /* Let I2C finish */
		
		/* Start USB */
		USB_Init();
		
		/* Wait for flash to complete - PB2 blinks (fast = appears constant) */
		while (1) {
			GPIOB->BRR = GPIO_BRR_BR2;   /* ON */
			delay(100000L);
			GPIOB->BSRR = GPIO_BSRR_BS2; /* OFF */
			delay(100000L);
			if (UploadFinished) break;
		}
		
		/* Flash complete - USB done, safe to use I2C again */
		USB_Shutdown();
		delay(DELAY_100MS);
		
		/* SUCCESS: Blue LED blinks 3x */
		for (int i = 0; i < 3; i++) {
			fp_set_leds(LED_BLUE);
			LED1_ON;
			delay(DELAY_100MS * 5);
			fp_set_leds(0);
			LED1_OFF;
			delay(DELAY_100MS * 5);
		}
		
		/* Reset to app */
		fp_set_leds(0);
		NVIC_SystemReset();
		for (;;);
	}
#else
	if ((magic_word == 0x424C) ||
		READ_BIT(GPIOB->IDR, GPIO_IDR_IDR2) ||
		(check_user_code(USER_PROGRAM) == false)) {
		
		if (magic_word == 0x424C) {
			LED2_ON;
			USB_Shutdown();
			delay(4000000L);
		}
		USB_Init();
		
		while (1) {
			if (UploadFinished) break;
			LED1_ON;
			delay(200000L);
			LED1_OFF;
			delay(200000L);
		}
		
		USB_Shutdown();
		NVIC_SystemReset();
		for (;;);
	}
#endif

	LED2_ON;

	/* Turn GPIO clocks off */
	CLEAR_BIT(RCC->APB2ENR,
		LED1_CLOCK | LED2_CLOCK | DISC_CLOCK);

	/* Setup vector table to user program */
	WRITE_REG(SCB->VTOR, USER_PROGRAM);

	/* Setup stack pointer and jump to user firmware */
	__set_MSP((*(volatile uint32_t *) USER_PROGRAM));
	UserProgram();

	for (;;);
}
