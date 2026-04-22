/*
 * frontpanel.c
 *
 * I2C2 master driver for ADG715 analog switch ICs.
 * Bus: PB10 (SCL2_ADG715), PB11 (SDA2_ADG715).
 *
 * Written by Klaus Kupferschmid.
 */

#include "frontpanel.h"

/* I2C2 peripheral and pins. */
#define fp_i2c   i2c2
#define FP_SCL   10  /* PB10 */
#define FP_SDA   11  /* PB11 */

/* APB1 clock = 36 MHz (SYSCLK/2). */
#define APB1_MHZ 36

/* Timeout for I2C polling loops (in SysTick ticks). */
#define I2C_TIMEOUT_MS 50

/*
 * Flash storage address for FrontPanel patterns.
 * Last page of 128KB flash (page size 1KB on STM32F103Cx).
 * Layout (compatible with FrontPanel STM32CubeIDE project):
 *   Byte 3 (bits 24-31): fp_sw_pattern
 *   Byte 2 (bits 16-23): bs_sw_pattern
 *   Byte 1-0: reserved (0)
 */
#define FP_FLASH_ADDR  0x0801F800

/* Live Watch diagnostics. */
volatile uint8_t fp_i2c2_status;
volatile uint8_t fp_sw_pattern;
volatile uint8_t bs_sw_pattern;

/* Has init been called and bus is ready? */
static bool_t fp_initialised;

/* One-shot flag: send initial pattern once. */
static bool_t fp_initial_send_done;

/* Debounce timestamp for HDMI select button. */
static uint32_t hdmi_btn_time;
static bool_t hdmi_btn_debounce;

/* Debounce timestamp for KB select button. */
static uint32_t kb_btn_time;
static bool_t kb_btn_debounce;

/* Boot Select button state machine. */
#define BOOT_BTN_IDLE      0
#define BOOT_BTN_PRESSED   1
#define BOOT_BTN_DEBOUNCE  2
static uint8_t boot_btn_state;
static uint32_t boot_btn_time;

/* R2H display mode (not persisted across power cycles).
 * Exported so main.c can suppress GoTek button reads. */
bool_t r2h_display_enabled;

/* R2H pulse output state machine. */
static bool_t r2h_pulse_active;
static uint8_t r2h_pulse_pin;
static uint32_t r2h_pulse_time;

/* R2H external SELECT button debounce. */
static uint32_t r2h_sel_btn_time;
static bool_t r2h_sel_btn_debounce;

/*
 * Wait for a status bit in SR1, with timeout.
 * Returns TRUE on success, FALSE on timeout/error.
 */
static bool_t fp_i2c2_wait_flag(uint32_t flag, unsigned int timeout_ms)
{
    stk_time_t start = stk_now();

    while (!(fp_i2c->sr1 & flag)) {
        if (fp_i2c->sr1 & I2C_SR1_ERRORS) {
            fp_i2c->sr1 &= ~I2C_SR1_ERRORS;
            return FALSE;
        }
        if (stk_timesince(start) >= stk_ms(timeout_ms))
            return FALSE;
    }
    return TRUE;
}

/*
 * Generate I2C STOP condition.
 */
static void fp_i2c2_stop(void)
{
    fp_i2c->cr1 |= I2C_CR1_STOP;
}

/*
 * Check if an I2C device acknowledges its address.
 * Returns FP_STATUS_OK or an error code.
 */
static uint8_t fp_i2c2_is_device_ready(uint8_t addr7)
{
    stk_time_t start;

    /* Wait until bus is not busy. */
    start = stk_now();
    while (fp_i2c->sr2 & I2C_SR2_BUSY) {
        if (stk_timesince(start) >= stk_ms(I2C_TIMEOUT_MS))
            return FP_STATUS_BUSY;
    }

    /* Generate START. */
    fp_i2c->cr1 |= I2C_CR1_START;
    if (!fp_i2c2_wait_flag(I2C_SR1_SB, I2C_TIMEOUT_MS)) {
        fp_i2c2_stop();
        return FP_STATUS_TIMEOUT;
    }

    /* Send address (write mode). */
    fp_i2c->dr = addr7 << 1;

    /* Wait for ADDR flag (ACK received) or AF (NACK). */
    start = stk_now();
    while (!(fp_i2c->sr1 & (I2C_SR1_ADDR | I2C_SR1_AF))) {
        if (stk_timesince(start) >= stk_ms(I2C_TIMEOUT_MS)) {
            fp_i2c2_stop();
            return FP_STATUS_TIMEOUT;
        }
    }

    if (fp_i2c->sr1 & I2C_SR1_AF) {
        /* NACK received - device not present. */
        fp_i2c->sr1 &= ~I2C_SR1_AF;
        fp_i2c2_stop();
        return FP_STATUS_NACK;
    }

    /* Clear ADDR by reading SR2. */
    (void)fp_i2c->sr2;

    /* Generate STOP. */
    fp_i2c2_stop();
    return FP_STATUS_OK;
}

/*
 * Transmit one byte of data to an I2C device.
 * Equivalent to HAL_I2C_Master_Transmit(&hi2c2, addr<<1, &data, 1, 100).
 * Returns FP_STATUS_OK or an error code.
 */
static uint8_t fp_i2c2_transmit(uint8_t addr7, uint8_t data)
{
    stk_time_t start;

    /* Wait until bus is not busy. */
    start = stk_now();
    while (fp_i2c->sr2 & I2C_SR2_BUSY) {
        if (stk_timesince(start) >= stk_ms(I2C_TIMEOUT_MS))
            return FP_STATUS_BUSY;
    }

    /* Generate START. */
    fp_i2c->cr1 |= I2C_CR1_START;
    if (!fp_i2c2_wait_flag(I2C_SR1_SB, I2C_TIMEOUT_MS)) {
        fp_i2c2_stop();
        return FP_STATUS_TIMEOUT;
    }

    /* Send address (write mode). */
    fp_i2c->dr = addr7 << 1;

    /* Wait for ADDR (ACK). */
    start = stk_now();
    while (!(fp_i2c->sr1 & (I2C_SR1_ADDR | I2C_SR1_AF))) {
        if (stk_timesince(start) >= stk_ms(I2C_TIMEOUT_MS)) {
            fp_i2c2_stop();
            return FP_STATUS_TIMEOUT;
        }
    }
    if (fp_i2c->sr1 & I2C_SR1_AF) {
        fp_i2c->sr1 &= ~I2C_SR1_AF;
        fp_i2c2_stop();
        return FP_STATUS_NACK;
    }

    /* Clear ADDR by reading SR2. */
    (void)fp_i2c->sr2;

    /* Send data byte. */
    fp_i2c->dr = data;
    if (!fp_i2c2_wait_flag(I2C_SR1_BTF, I2C_TIMEOUT_MS)) {
        fp_i2c2_stop();
        return FP_STATUS_TIMEOUT;
    }

    /* Generate STOP. */
    fp_i2c2_stop();
    return FP_STATUS_OK;
}

/*
 * Read patterns from flash.
 * If flash is erased (0xFFFFFFFF), patterns default to 0.
 */
static void fp_flash_read(void)
{
    uint32_t val = *(volatile uint32_t *)FP_FLASH_ADDR;
    if (val == 0xFFFFFFFF) {
        fp_sw_pattern = 0;
        bs_sw_pattern = 0;
    } else {
        fp_sw_pattern = (val >> 24) & 0xFF;
        bs_sw_pattern = (val >> 16) & 0xFF;
    }
}

/*
 * Erase the flash page at FP_FLASH_ADDR, then program both patterns.
 * Uses the existing fpec_* routines which correctly enable HSI and
 * handle flash controller state between operations.
 */
static void fp_flash_write(void)
{
    /* Word layout (little-endian):
     *   addr+0: hw0 = 0x0000                  (reserved)
     *   addr+2: hw1 = [fp_sw_pattern | bs_sw_pattern]
     * 32-bit read: bits 24-31 = fp, bits 16-23 = bs. */
    uint16_t buf[2];
    buf[0] = 0x0000;
    buf[1] = ((uint16_t)fp_sw_pattern << 8) | bs_sw_pattern;

    fpec_init();
    fpec_page_erase(FP_FLASH_ADDR);
    fpec_write(buf, sizeof(buf), FP_FLASH_ADDR);
}

/*
 * Initialise I2C2 as master on PB10/PB11, 100kHz standard mode.
 * Configure FrontPanel GPIOs.
 */
void frontpanel_init(void)
{
    /* Enable I2C2 peripheral clock. */
    rcc->apb1enr |= RCC_APB1ENR_I2C2EN;

    /* Configure PB10 (SCL2_ADG715) and PB11 (SDA2_ADG715)
     * as alternate-function open-drain. */
    gpio_configure_pin(gpiob, FP_SCL, AFO_opendrain(_2MHz));
    gpio_configure_pin(gpiob, FP_SDA, AFO_opendrain(_2MHz));

    /* Software reset to clear any stuck state. */
    fp_i2c->cr1 = I2C_CR1_SWRST;
    delay_us(10);
    fp_i2c->cr1 = 0;

    /* Set APB1 frequency for timing calculation. */
    fp_i2c->cr2 = I2C_CR2_FREQ(APB1_MHZ);

    /* 100kHz standard mode: CCR = APB1_MHz * 1000000 / (2 * 100000) = 180. */
    fp_i2c->ccr = I2C_CCR_CCR(180);

    /* TRISE = APB1_MHz + 1 = 37 (1000ns max rise time in SM). */
    fp_i2c->trise = APB1_MHZ + 1;

    /* Enable I2C peripheral. */
    fp_i2c->cr1 = I2C_CR1_PE;

    /*
     * FrontPanel GPIO initialisation.
     * Button inputs (active low, no pull -- external pull-ups on board):
     */
    gpio_configure_pin(gpioa, FP_PIN_BT_KB_SELECT, GPI_pull_up);
    gpio_configure_pin(gpioa, FP_PIN_BT_BOOT_SELECT, GPI_pull_up);

    /* Outputs (directly driving external hardware): */
    gpio_configure_pin(gpiob, FP_PIN_RESERVE_PB1, GPO_pushpull(_2MHz, LOW));
    gpio_configure_pin(gpiob, FP_PIN_PP_LED, GPO_pushpull(_2MHz, LOW));
    gpio_configure_pin(gpiob, FP_PIN_BT_R2H_SELECT_U2, GPO_pushpull(_2MHz, HIGH));
    gpio_configure_pin(gpiob, FP_PIN_BT_R2H_UP_U0, GPO_pushpull(_2MHz, HIGH));
    gpio_configure_pin(gpiob, FP_PIN_BT_R2H_DOWN_U1, GPO_pushpull(_2MHz, HIGH));
    gpio_configure_pin(gpiob, FP_PIN_BT_HDMI_SELECT, GPO_opendrain(_2MHz, HIGH));

    /* Load saved patterns from flash. */
    fp_flash_read();

    /* R2H mode is never persisted; clear LEDs if they were set. */
    if (fp_sw_pattern & LED_R2H_DISPLAY_ENABLE) {
        fp_sw_pattern &= ~LED_R2H_DISPLAY_ENABLE;
        fp_flash_write();
    }

    fp_i2c2_status = FP_STATUS_UNINIT;
    fp_initial_send_done = FALSE;
    hdmi_btn_debounce = FALSE;
    kb_btn_debounce = FALSE;
    boot_btn_state = BOOT_BTN_IDLE;
    r2h_display_enabled = FALSE;
    r2h_pulse_active = FALSE;
    r2h_sel_btn_debounce = FALSE;
    fp_initialised = TRUE;
}

/*
 * Called from main loop. Handle buttons, update LEDs via I2C2.
 */
void frontpanel_process(void)
{
    uint32_t now;

    if (!fp_initialised)
        return;

    /* One-time: probe both ADG715 chips and send initial patterns. */
    if (!fp_initial_send_done) {
        fp_i2c2_status = fp_i2c2_is_device_ready(ADG715_FP_ADDR);
        if (fp_i2c2_status == FP_STATUS_OK) {
            fp_i2c2_transmit(ADG715_FP_ADDR, fp_sw_pattern);
            fp_i2c2_transmit(ADG715_BS_ADDR, bs_sw_pattern);
            /* Restore HDMI switch state after power cycle:
             * pulse PB12 LOW-HIGH to toggle the external HDMI switch. */
            if (fp_sw_pattern & LED_HDMI_SELECT) {
                gpio_write_pin(gpiob, FP_PIN_BT_HDMI_SELECT, LOW);
                delay_ms(50);
                gpio_write_pin(gpiob, FP_PIN_BT_HDMI_SELECT, HIGH);
                delay_ms(50);
            }
            fp_initial_send_done = TRUE;
        }
        return;
    }

    now = time_now();

    /* --- R2H pulse completion --- */
    if (r2h_pulse_active) {
        if (time_diff(r2h_pulse_time, now) > time_ms(50)) {
            gpio_write_pin(gpiob, r2h_pulse_pin, HIGH);
            r2h_pulse_active = FALSE;
        }
    }

    /* --- BT_HDMI_SELECT (PB12): toggle LED_HDMI_SELECT on press --- */
    if (!gpio_read_pin(gpiob, FP_PIN_BT_HDMI_SELECT)) {
        if (!hdmi_btn_debounce
            || time_diff(hdmi_btn_time, now) > time_ms(300)) {
            fp_sw_pattern ^= LED_HDMI_SELECT;
            fp_i2c2_status = fp_i2c2_transmit(ADG715_FP_ADDR, fp_sw_pattern);
            fp_flash_write();
            hdmi_btn_time = now;
            hdmi_btn_debounce = TRUE;
        }
    }

    /* --- BT_KB_SELECT (PA3): toggle KB_SELECT + LED_KB_SELECT --- */
    if (!gpio_read_pin(gpioa, FP_PIN_BT_KB_SELECT)) {
        if (!kb_btn_debounce
            || time_diff(kb_btn_time, now) > time_ms(300)) {
            fp_sw_pattern ^= (KB_SELECT | LED_KB_SELECT);
            fp_i2c2_transmit(ADG715_FP_ADDR, fp_sw_pattern);
            fp_flash_write();
            kb_btn_time = now;
            kb_btn_debounce = TRUE;
        }
    }

    /* --- BT_Boot_Select (PA4): short press = cycle DF, long = toggle R2H ---
     * Non-blocking state machine (original FrontPanel blocks until release). */
    switch (boot_btn_state) {
    case BOOT_BTN_IDLE:
        if (!gpio_read_pin(gpioa, FP_PIN_BT_BOOT_SELECT)) {
            boot_btn_time = now;
            boot_btn_state = BOOT_BTN_PRESSED;
        }
        break;

    case BOOT_BTN_PRESSED:
        /* Wait for button release. */
        if (gpio_read_pin(gpioa, FP_PIN_BT_BOOT_SELECT)) {
            if (time_diff(boot_btn_time, now) > time_ms(800)) {
                /* Long press: toggle R2H display mode. */
                if (!r2h_display_enabled) {
                    r2h_display_enabled = TRUE;
                    fp_sw_pattern |= LED_R2H_DISPLAY_ENABLE;
                } else {
                    r2h_display_enabled = FALSE;
                    fp_sw_pattern &= ~LED_R2H_DISPLAY_ENABLE;
                }
                fp_i2c2_transmit(ADG715_FP_ADDR, fp_sw_pattern);
                fp_flash_write();
            } else {
                /* Short press: cycle DF0 → DF1 → DF2 → DF3 → DF0.
                 * LED state encodes current DF selection. */
                bool_t df1 = (fp_sw_pattern & LED_BOOT_SELECT_DF1_3) != 0;
                bool_t df2 = (fp_sw_pattern & LED_BOOT_SELECT_DF2_3) != 0;
                if (!df1 && !df2) {
                    /* DF0 → DF1 */
                    fp_sw_pattern |= LED_BOOT_SELECT_DF1_3;
                    bs_sw_pattern = BOOT_DF1;
                } else if (df1 && !df2) {
                    /* DF1 → DF2 */
                    fp_sw_pattern &= ~LED_BOOT_SELECT_DF1_3;
                    fp_sw_pattern |= LED_BOOT_SELECT_DF2_3;
                    bs_sw_pattern = BOOT_DF2;
                } else if (!df1 && df2) {
                    /* DF2 → DF3 */
                    fp_sw_pattern |= LED_BOOT_SELECT_DF1_3;
                    bs_sw_pattern = BOOT_DF3;
                } else {
                    /* DF3 → DF0 */
                    fp_sw_pattern &= ~(LED_BOOT_SELECT_DF1_3
                                       | LED_BOOT_SELECT_DF2_3);
                    bs_sw_pattern = BOOT_DF0;
                }
                fp_i2c2_transmit(ADG715_FP_ADDR, fp_sw_pattern);
                fp_i2c2_transmit(ADG715_BS_ADDR, bs_sw_pattern);
                fp_flash_write();
            }
            boot_btn_time = now;
            boot_btn_state = BOOT_BTN_DEBOUNCE;
        }
        break;

    case BOOT_BTN_DEBOUNCE:
        if (time_diff(boot_btn_time, now) > time_ms(500))
            boot_btn_state = BOOT_BTN_IDLE;
        break;
    }

    /* --- R2H mode: UP/DOWN/SELECT pulse corresponding R2H output pins ---
     * Only one pulse at a time; natural debounce via pulse duration. */
    if (r2h_display_enabled && !r2h_pulse_active) {
        if (!gpio_read_pin(gpiob, FP_PIN_BT_FP_UP)) {
            /* UP → pulse PB8 (R2H_UP_U0) LOW for 50ms. */
            gpio_write_pin(gpiob, FP_PIN_BT_R2H_UP_U0, LOW);
            r2h_pulse_pin = FP_PIN_BT_R2H_UP_U0;
            r2h_pulse_time = now;
            r2h_pulse_active = TRUE;
        } else if (!gpio_read_pin(gpioa, FP_PIN_BT_FP_DOWN)) {
            /* DOWN → pulse PB9 (R2H_DOWN_U1) LOW for 50ms. */
            gpio_write_pin(gpiob, FP_PIN_BT_R2H_DOWN_U1, LOW);
            r2h_pulse_pin = FP_PIN_BT_R2H_DOWN_U1;
            r2h_pulse_time = now;
            r2h_pulse_active = TRUE;
        } else if (!gpio_read_pin(gpioa, FP_PIN_BT_FP_SELECT)) {
            /* SELECT → pulse PB5 (R2H_SELECT_U2) LOW for 50ms. */
            gpio_write_pin(gpiob, FP_PIN_BT_R2H_SELECT_U2, LOW);
            r2h_pulse_pin = FP_PIN_BT_R2H_SELECT_U2;
            r2h_pulse_time = now;
            r2h_pulse_active = TRUE;
        }
    }

    /* --- R2H external SELECT button (PB5 active low): enable R2H mode ---
     * The RGBtoHDMI board pulls PB5 low when its SELECT button is pressed. */
    if (!r2h_display_enabled && !r2h_pulse_active
        && !gpio_read_pin(gpiob, FP_PIN_BT_R2H_SELECT_U2)) {
        if (!r2h_sel_btn_debounce
            || time_diff(r2h_sel_btn_time, now) > time_ms(300)) {
            r2h_display_enabled = TRUE;
            fp_sw_pattern |= LED_R2H_DISPLAY_ENABLE;
            fp_i2c2_transmit(ADG715_FP_ADDR, fp_sw_pattern);
            fp_flash_write();
            r2h_sel_btn_time = now;
            r2h_sel_btn_debounce = TRUE;
        }
    }
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
