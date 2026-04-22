/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <read_write_flash.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BT_FP_SELECT_Pin GPIO_PIN_2
#define BT_FP_SELECT_GPIO_Port GPIOA
#define BT_KB_SELECT_Pin GPIO_PIN_3
#define BT_KB_SELECT_GPIO_Port GPIOA
#define BT_Boot_Select_Pin GPIO_PIN_4
#define BT_Boot_Select_GPIO_Port GPIOA
#define BT_FP_DOWN_Pin GPIO_PIN_6
#define BT_FP_DOWN_GPIO_Port GPIOA
#define BT_FP_UP_Pin GPIO_PIN_0
#define BT_FP_UP_GPIO_Port GPIOB
#define Reserve_PB1_Pin GPIO_PIN_1
#define Reserve_PB1_GPIO_Port GPIOB
#define PP_LED_Pin GPIO_PIN_2
#define PP_LED_GPIO_Port GPIOB
#define SCL2_ADG715_Pin GPIO_PIN_10
#define SCL2_ADG715_GPIO_Port GPIOB
#define SDA2_ADG715_Pin GPIO_PIN_11
#define SDA2_ADG715_GPIO_Port GPIOB
#define BT_HDMI_SELECT_Pin GPIO_PIN_12
#define BT_HDMI_SELECT_GPIO_Port GPIOB
#define Disp_Out_Pin GPIO_PIN_15
#define Disp_Out_GPIO_Port GPIOB
#define C_Sync_Pin GPIO_PIN_8
#define C_Sync_GPIO_Port GPIOA
#define DISPLAY_ENABLE_Pin GPIO_PIN_15
#define DISPLAY_ENABLE_GPIO_Port GPIOA
#define KB_Data_CIA_Pin GPIO_PIN_3
#define KB_Data_CIA_GPIO_Port GPIOB
#define KB_CLOCK_CIA_Pin GPIO_PIN_4
#define KB_CLOCK_CIA_GPIO_Port GPIOB
#define BT_R2H_SELECT_U2_Pin GPIO_PIN_5
#define BT_R2H_SELECT_U2_GPIO_Port GPIOB
#define SCL1_GoTek_Pin GPIO_PIN_6
#define SCL1_GoTek_GPIO_Port GPIOB
#define SDA1_GoTek_Pin GPIO_PIN_7
#define SDA1_GoTek_GPIO_Port GPIOB
#define BT_R2H_UP_U0_Pin GPIO_PIN_8
#define BT_R2H_UP_U0_GPIO_Port GPIOB
#define BT_R2H_DOWN_U1_Pin GPIO_PIN_9
#define BT_R2H_DOWN_U1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
