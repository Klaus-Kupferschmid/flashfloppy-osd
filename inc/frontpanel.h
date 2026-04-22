/*
 * frontpanel.h
 *
 * FrontPanel I2C2 master interface for ADG715 analog switch ICs.
 *
 * I2C2 bus on PB10 (SCL2_ADG715) and PB11 (SDA2_ADG715).
 * Two ADG715 chips:
 *   - ADG715_FP_ADDR (0x48): FrontPanel LEDs
 *   - ADG715_BS_ADDR (0x49): Boot Selector
 */

#ifndef __FRONTPANEL_H__
#define __FRONTPANEL_H__

/* ADG715 7-bit I2C addresses. */
#define ADG715_FP_ADDR  0x48
#define ADG715_BS_ADDR  0x49

/* FrontPanel LED bitmask (ADG715_FP_ADDR). */
#define LED_FP_UP               0x80
#define LED_FP_DOWN             0x40
#define LED_FP_SELECT           0x20
#define LED_BOOT_SELECT_DF1_3   0x10
#define LED_HDMI_SELECT         0x08
#define LED_KB_SELECT           0x04
#define LED_BOOT_SELECT_DF2_3   0x02
#define KB_SELECT               0x01
#define LED_R2H_DISPLAY_ENABLE  0xe0

/* Boot Selector patterns (ADG715_BS_ADDR). */
#define BOOT_DF0  0x00
#define BOOT_DF1  0x43
#define BOOT_DF2  0x8c
#define BOOT_DF3  0xf0

/*
 * FrontPanel GPIO pin assignments.
 * Names match the STM32CubeIDE FrontPanel project (main.h).
 *
 * Buttons (active low, directly readable as inputs):
 *   PA2 = BT_FP_SELECT    (shared with rotary SEL, already init)
 *   PA3 = BT_KB_SELECT    (new)
 *   PA4 = BT_Boot_Select  (new)
 *   PA6 = BT_FP_DOWN      (already init as button)
 *   PB0 = BT_FP_UP        (already init as button)
 *
 * Outputs (directly driving external hardware):
 *   PB1  = Reserve
 *   PB2  = PP_LED          (FrontPanel status LED)
 *   PB5  = BT_R2H_SELECT_U2
 *   PB8  = BT_R2H_UP_U0   (shared with User Output U0, already init)
 *   PB9  = BT_R2H_DOWN_U1 (shared with User Output U1, already init)
 *   PB12 = BT_HDMI_SELECT  (HDMI switch control + button read)
 */
#define FP_PIN_BT_FP_SELECT      2   /* PA2 */
#define FP_PIN_BT_KB_SELECT       3   /* PA3 */
#define FP_PIN_BT_BOOT_SELECT     4   /* PA4 */
#define FP_PIN_BT_FP_DOWN         6   /* PA6 */
#define FP_PIN_BT_FP_UP           0   /* PB0 */
#define FP_PIN_RESERVE_PB1        1   /* PB1 */
#define FP_PIN_PP_LED             2   /* PB2 */
#define FP_PIN_BT_R2H_SELECT_U2  5   /* PB5 */
#define FP_PIN_BT_R2H_UP_U0      8   /* PB8 */
#define FP_PIN_BT_R2H_DOWN_U1    9   /* PB9 */
#define FP_PIN_BT_HDMI_SELECT    12   /* PB12 */

/* I2C2 status codes (observable via Live Watch). */
#define FP_STATUS_UNINIT   0
#define FP_STATUS_OK       1
#define FP_STATUS_NACK     2
#define FP_STATUS_TIMEOUT  3
#define FP_STATUS_BUSY     4

/* Live Watch diagnostics. */
extern volatile uint8_t fp_i2c2_status;
extern volatile uint8_t fp_sw_pattern;
extern volatile uint8_t bs_sw_pattern;
extern bool_t r2h_display_enabled;

/* Initialise I2C2 as master on PB10/PB11 and FrontPanel GPIOs. */
void frontpanel_init(void);

/* Call from main loop: handle buttons, update LEDs via I2C2. */
void frontpanel_process(void);

#endif /* __FRONTPANEL_H__ */
