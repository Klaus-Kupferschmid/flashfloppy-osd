/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define FLASH_ADDRESS_PATTERN  0x0801F800 //letzte Speicheradresse, da von vorne der Speicher mit dem Code selbst gefüllt wird
uint32_t buttonPressTime = 0;
uint32_t buttonReleaseTime = 0;
uint8_t buttonPressed = 0;

#define FLASH_STORAGE_ADDR 0x0801F800
#define FP 1U //FP = FrontPanel
#define BS 0U //BS = BootSelector
//#define HS 2U //HS = Hdmi Select
#define storage_type FP
uint64_t var_flash_Data =0;
uint64_t *flash_data = &var_flash_Data;
uint8_t var_fp_sw_patern =0;
uint8_t *fp_sw_patern = &var_fp_sw_patern;
uint8_t var_bs_sw_patern;
uint8_t *bs_sw_patern = &var_bs_sw_patern;
//uint8_t var_hs_sw_patern;
//uint8_t *hs_sw_patern = &var_hs_sw_patern;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
    /* Set I2C-Bus-Adress and Pattern for FrontPanel LEDs  */
	static const uint8_t ADG715_FP_ADDR = 0x48 << 1 ; //Use7-bit address
	*fp_sw_patern =                  0b00000000;
	//uint8_t LED_FP_UP =              0b10000000;
	//uint8_t LED_FP_DOWN =            0b01000000;
	//uint8_t LED_FP_SELECT =          0b00100000;
	uint8_t LED_BOOT_SELECT_DF1_3 =  0b00010000;
	uint8_t LED_HDMI_SELECT =        0b00001000;
	uint8_t LED_KB_SELECT =          0b00000100;
	uint8_t LED_BOOT_SELECT_DF2_3 =  0b00000010;
	uint8_t KB_SELECT =              0b00000001;
	uint8_t LED_R2H_DISPLAY_ENABLE = 0b11100000;

	/* Set I2C-Bus-Adress and Pattern for Boot-Selector  */
	static const uint8_t ADG715_Boot_SELECTOR_ADDR = 0x49 << 1 ; //Use7-bit address
	*bs_sw_patern = 0b00000000;
	uint8_t DF0 =   0b00000000;
	uint8_t DF1 =   0b01000011;
	uint8_t DF2 =   0b10001100;
	uint8_t DF3 =   0b11110000;
	//	uint8_t DF_BOOT_SELECT_LED =   0b01000000;

	_Bool R2H_DISPLAY_ENABLED = 0; // 1 = up,down and Select button are used for Rgb2hdmi; 0 = Buttons are used for GoTek

	/* Set Constant of HDMI-Select
	uint8_t hdmi_A =   0b00000000; // this Variable stores the Pattern for selecting the A-output of the HDMI-Switch
	uint8_t hdmi_B =   0b00000001; // this Variable stores the Pattern for selecting the B-output of the HDMI-Switch
	*/






  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_PCD_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  // Listen on GPIO for Interrupts
  //HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);

  // Read Flash_Storage if it is erased than write Default Pattern
  	  	// erase_flash();
  	  	read_flash(FP);
    	read_flash(BS);

    	if (*fp_sw_patern == 255 && *bs_sw_patern == 255)
    	{
    		*fp_sw_patern = 0b00000000;
    		write_flash(FP)	;
    		*bs_sw_patern = 0b00000000;
    		write_flash(BS)	;
    	}
    	//check I2C-Bus and indicate status on PP-LED
		switch (HAL_I2C_IsDeviceReady(&hi2c2, ADG715_FP_ADDR, 1, 300))
		{
			case HAL_BUSY:
				HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 1);
				HAL_Delay(1000);
				HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 0);
				HAL_Delay(1000);
			break;
			case HAL_TIMEOUT:
				HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 1);
				HAL_Delay(500);
				HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 0);
				HAL_Delay(50);
			break;
			case HAL_ERROR:
				HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 1);
				HAL_Delay(50);
				HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 0);
				HAL_Delay(100);
			break;
			case HAL_OK:
				// write pattern to ADG715 Switch IC
				HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100);
				HAL_I2C_Master_Transmit(&hi2c2, ADG715_Boot_SELECTOR_ADDR, bs_sw_patern, 1, 100);
				HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 1);
				//HAL_Delay(100);
			break;
		}
		// Check if R2H_DISPLAY_ENABLED was enabled before PowerOff --> switch off LEDs and write to flash
		if ((*fp_sw_patern & LED_R2H_DISPLAY_ENABLE) == LED_R2H_DISPLAY_ENABLE){
			*fp_sw_patern = *fp_sw_patern - LED_R2H_DISPLAY_ENABLE;
			HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100);
			write_flash(FP);
		}
		// Set HDMI-Switch according to the stored Status (before Powered off)
		if((*fp_sw_patern & LED_HDMI_SELECT) == LED_HDMI_SELECT ){
			HAL_GPIO_WritePin(BT_HDMI_SELECT_GPIO_Port, BT_HDMI_SELECT_Pin,0);
			HAL_Delay(50);
			HAL_GPIO_WritePin(BT_HDMI_SELECT_GPIO_Port, BT_HDMI_SELECT_Pin,1);
			HAL_Delay(50);
		}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 0);

	  // If Button KB_SELECT is pushed switch than toggle LED_KB_SELECT
		if (!HAL_GPIO_ReadPin (BT_KB_SELECT_GPIO_Port,BT_KB_SELECT_Pin))
		{
			// In Case the Button was already pushed switch KB_Select and LED to LOW
			if((*fp_sw_patern & (KB_SELECT + LED_KB_SELECT)) == (KB_SELECT + LED_KB_SELECT)){
				*fp_sw_patern = *fp_sw_patern - (KB_SELECT + LED_KB_SELECT);
				write_flash(FP);
			}else{
				*fp_sw_patern = *fp_sw_patern + (KB_SELECT + LED_KB_SELECT);
				write_flash(FP);
			}
			HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100);
			HAL_Delay(300);
		}

		// If Button HDMI_SELECT is pushed than toggle LED_HDMI_SELECT
		if (!HAL_GPIO_ReadPin (BT_HDMI_SELECT_GPIO_Port, BT_HDMI_SELECT_Pin))
		{
			if((*fp_sw_patern & LED_HDMI_SELECT) == LED_HDMI_SELECT ){
				*fp_sw_patern = *fp_sw_patern - LED_HDMI_SELECT;
				write_flash(FP);
			}else{
				*fp_sw_patern = *fp_sw_patern + LED_HDMI_SELECT;
				write_flash(FP);
			}
			HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100);
			HAL_Delay(300);
		}

		// If Button UP is pushed in R2H_Display_Enebled-Mode than put R2H-PIN for 50 mSec to Low
		if (!HAL_GPIO_ReadPin (BT_FP_UP_GPIO_Port,BT_FP_UP_Pin) && R2H_DISPLAY_ENABLED)
		{
			if(R2H_DISPLAY_ENABLED){
				HAL_GPIO_WritePin(BT_R2H_UP_U0_GPIO_Port, BT_R2H_UP_U0_Pin,0);
				HAL_Delay(50);
				HAL_GPIO_WritePin(BT_R2H_UP_U0_GPIO_Port, BT_R2H_UP_U0_Pin,1);
				HAL_Delay(50);
			}
		}

		// If Button DOWN is pushed in R2H_Display_Enebled-Mode than put R2H-PIN for 50 mSec to Low
		if (!HAL_GPIO_ReadPin (BT_FP_DOWN_GPIO_Port,BT_FP_DOWN_Pin) && R2H_DISPLAY_ENABLED)
		{
			if(R2H_DISPLAY_ENABLED){
				HAL_GPIO_WritePin(BT_R2H_DOWN_U1_GPIO_Port, BT_R2H_DOWN_U1_Pin,0);
				HAL_Delay(50);
				HAL_GPIO_WritePin(BT_R2H_DOWN_U1_GPIO_Port, BT_R2H_DOWN_U1_Pin,1);
				HAL_Delay(50);
			}
		}

		// If Button SELECT is pushed in R2H_Display_Enebled-Mode than put R2H-PIN for 50 mSec to Low
		if (!HAL_GPIO_ReadPin (BT_FP_SELECT_GPIO_Port, BT_FP_SELECT_Pin) && R2H_DISPLAY_ENABLED)
		{
			if(R2H_DISPLAY_ENABLED){
				HAL_GPIO_WritePin(BT_R2H_SELECT_U2_GPIO_Port, BT_R2H_SELECT_U2_Pin,0);
				HAL_Delay(50);
				HAL_GPIO_WritePin(BT_R2H_SELECT_U2_GPIO_Port, BT_R2H_SELECT_U2_Pin,1);
				HAL_Delay(50);
			}
		}

		// If SELECT Button on R2H-Board is pushed than activate R2H_DISPLAY_ENABLED and light on 3-Buttons
		if (!HAL_GPIO_ReadPin (BT_R2H_SELECT_U2_GPIO_Port, BT_R2H_SELECT_U2_Pin) )
		{
			if(R2H_DISPLAY_ENABLED ==0){
				R2H_DISPLAY_ENABLED = 1;
				*fp_sw_patern = *fp_sw_patern + LED_R2H_DISPLAY_ENABLE;
				write_flash(FP);
			}
			HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100);
			HAL_Delay(50);
		}


		if (!HAL_GPIO_ReadPin(BT_Boot_Select_GPIO_Port, BT_Boot_Select_Pin)) {

			buttonPressTime = HAL_GetTick();
			while(!HAL_GPIO_ReadPin(BT_Boot_Select_GPIO_Port, BT_Boot_Select_Pin)){}
			buttonReleaseTime = HAL_GetTick();

			if ((buttonReleaseTime - buttonPressTime) > 800) {
				if(R2H_DISPLAY_ENABLED == 0){
					R2H_DISPLAY_ENABLED = 1;
					*fp_sw_patern = *fp_sw_patern + LED_R2H_DISPLAY_ENABLE;
					write_flash(FP);
				}else{
					R2H_DISPLAY_ENABLED = 0;
					*fp_sw_patern = *fp_sw_patern - LED_R2H_DISPLAY_ENABLE;
					write_flash(FP);
				}
			}else{
				// button was pressed very short
				// df1
				while(1){
						if((LED_BOOT_SELECT_DF1_3 != (*fp_sw_patern & LED_BOOT_SELECT_DF1_3)) && (LED_BOOT_SELECT_DF2_3 !=(*fp_sw_patern & LED_BOOT_SELECT_DF2_3)) )
						{
							*fp_sw_patern = *fp_sw_patern + LED_BOOT_SELECT_DF1_3;
							HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100); //LED on FrontPanel will lightening in Blue
							write_flash(FP);
							*bs_sw_patern = DF1;
							HAL_I2C_Master_Transmit(&hi2c2, ADG715_Boot_SELECTOR_ADDR, bs_sw_patern, 1, 100); //BOOT_SEL Connections between CIA and MB will be changed to DF1
							write_flash(BS);
							HAL_Delay(50);
							break;
						}
						// df2
						if((LED_BOOT_SELECT_DF1_3 == (*fp_sw_patern & LED_BOOT_SELECT_DF1_3)) && (LED_BOOT_SELECT_DF2_3 !=(*fp_sw_patern & LED_BOOT_SELECT_DF2_3)) )
						{
							*fp_sw_patern = (*fp_sw_patern - LED_BOOT_SELECT_DF1_3) + LED_BOOT_SELECT_DF2_3;
							HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100); //LED on FrontPanel will lightening in Red
							write_flash(FP);
							*bs_sw_patern = DF2;
							HAL_I2C_Master_Transmit(&hi2c2, ADG715_Boot_SELECTOR_ADDR, bs_sw_patern, 1, 100); //BOOT_SEL Connections between CIA and MB will be changed to DF2
							write_flash(BS);
							HAL_Delay(50);
							break;
						}
						// df3
						if((LED_BOOT_SELECT_DF1_3 != (*fp_sw_patern & LED_BOOT_SELECT_DF1_3)) && (LED_BOOT_SELECT_DF2_3 ==(*fp_sw_patern & LED_BOOT_SELECT_DF2_3)) )
						{
							*fp_sw_patern = *fp_sw_patern + LED_BOOT_SELECT_DF1_3;
							HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100); //LED on FrontPanel will lightening in Pink
							write_flash(FP);
							*bs_sw_patern = DF3;
							HAL_I2C_Master_Transmit(&hi2c2, ADG715_Boot_SELECTOR_ADDR, bs_sw_patern, 1, 100); //BOOT_SEL Connections between CIA and MB will be changed to DF3
							write_flash(BS);
							HAL_Delay(50);
							break;
						}
						// df0
						if((LED_BOOT_SELECT_DF1_3 == (*fp_sw_patern & LED_BOOT_SELECT_DF1_3)) && (LED_BOOT_SELECT_DF2_3 ==(*fp_sw_patern & LED_BOOT_SELECT_DF2_3)) )
						{
							*fp_sw_patern = *fp_sw_patern - (LED_BOOT_SELECT_DF1_3 + LED_BOOT_SELECT_DF2_3);
							HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100); //LED on FrontPanel will be switched off
							write_flash(FP);
							*bs_sw_patern = DF0;
							HAL_I2C_Master_Transmit(&hi2c2, ADG715_Boot_SELECTOR_ADDR, bs_sw_patern, 1, 100); //BOOT_SEL Connections between CIA and MB will not be changed
							write_flash(BS);
							HAL_Delay(50);
							break;
						}
						break;
				}
			}
			HAL_I2C_Master_Transmit(&hi2c2, ADG715_FP_ADDR, fp_sw_patern, 1, 100);
			HAL_Delay(500);
			buttonPressed = 0; // Zurücksetzen


		}

		switch (HAL_I2C_IsDeviceReady(&hi2c2, ADG715_Boot_SELECTOR_ADDR, 1, 300))
		{
		case HAL_BUSY:
			HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 1);
			HAL_Delay(1000);
			HAL_GPIO_WritePin(GPIOB, PP_LED_Pin, 0);
			HAL_Delay(1000);
		break;
		case HAL_TIMEOUT:
			HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 1);
			HAL_Delay(500);
			HAL_GPIO_WritePin(GPIOB, PP_LED_Pin, 0);
			HAL_Delay(50);
		break;
		case HAL_ERROR:
			/*
			HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 1);
			HAL_Delay(50);
			HAL_GPIO_WritePin(GPIOB, PP_LED_Pin, 0);
			HAL_Delay(500);
			*/
		break;
		case HAL_OK:
			// PluePill-OnBoardLED On , because I2C-Slave-Device is ready
			HAL_GPIO_WritePin(PP_LED_GPIO_Port, PP_LED_Pin, 1);
			HAL_Delay(50);
		}
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_1LINE;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_1LINE;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Reserve_PB1_Pin|PP_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BT_HDMI_SELECT_Pin|BT_R2H_SELECT_U2_Pin|BT_R2H_UP_U0_Pin|BT_R2H_DOWN_U1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DISPLAY_ENABLE_GPIO_Port, DISPLAY_ENABLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BT_FP_SELECT_Pin BT_KB_SELECT_Pin BT_Boot_Select_Pin BT_FP_DOWN_Pin */
  GPIO_InitStruct.Pin = BT_FP_SELECT_Pin|BT_KB_SELECT_Pin|BT_Boot_Select_Pin|BT_FP_DOWN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BT_FP_UP_Pin KB_Data_CIA_Pin KB_CLOCK_CIA_Pin */
  GPIO_InitStruct.Pin = BT_FP_UP_Pin|KB_Data_CIA_Pin|KB_CLOCK_CIA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Reserve_PB1_Pin PP_LED_Pin BT_HDMI_SELECT_Pin BT_R2H_SELECT_U2_Pin
                           BT_R2H_UP_U0_Pin BT_R2H_DOWN_U1_Pin */
  GPIO_InitStruct.Pin = Reserve_PB1_Pin|PP_LED_Pin|BT_HDMI_SELECT_Pin|BT_R2H_SELECT_U2_Pin
                          |BT_R2H_UP_U0_Pin|BT_R2H_DOWN_U1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : C_Sync_Pin */
  GPIO_InitStruct.Pin = C_Sync_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(C_Sync_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DISPLAY_ENABLE_Pin */
  GPIO_InitStruct.Pin = DISPLAY_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DISPLAY_ENABLE_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/*
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == BT_Boot_Select_Pin) {
	  /*
	  if (HAL_GPIO_ReadPin(BT_Boot_Select_GPIO_Port, BT_Boot_Select_Pin) == GPIO_PIN_RESET) {
		  buttonPressTime = HAL_GetTick();
		  buttonPressed = 1;
	  } else {
		  buttonPressed = 0;
	  }

	  if (!buttonPressed){
		  buttonPressTime = HAL_GetTick();
		  buttonPressed = 1;
	  }
  }
}
*/
void read_flash(uint8_t type)
{
	*flash_data = *(uint64_t*)(FLASH_STORAGE_ADDR);
	if (type == BS)
	*bs_sw_patern = *flash_data>>16;
	if (type == FP)
	*fp_sw_patern =	*flash_data>>24;

}
HAL_StatusTypeDef write_flash(uint8_t type)
{
	// write both switch Pattern in one DWORD
	*flash_data = *(uint64_t*)(FLASH_STORAGE_ADDR);
	if (type == BS) {
		*flash_data &= ~(0xFFULL << 16);           		// Maske zum Löschen des 3. Bytes
		*flash_data |= ((uint64_t)*bs_sw_patern << 16); // Neues Byte einsetzen
	}
	if (type == FP) {
		*flash_data &= ~(0xFFULL << 24);           		// Maske zum Löschen des 3. Bytes
		*flash_data |= ((uint64_t)*fp_sw_patern << 24); // Neues Byte einsetzen
	}
	/* Unlock the Flash to enable the flash control register access *************/
		HAL_FLASH_Unlock();

		/* Allow Access to option bytes sector */
		HAL_FLASH_OB_Unlock();
		HAL_StatusTypeDef status;
		if ((erase_flash()) == HAL_OK)
			status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,FLASH_STORAGE_ADDR, *flash_data);
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
		return status;
}
HAL_StatusTypeDef erase_flash(){
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
	/* Fill EraseInit structure*/
		FLASH_EraseInitTypeDef EraseInitStruct;
		EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.PageAddress = FLASH_STORAGE_ADDR;
		EraseInitStruct.NbPages = 1;
		uint32_t  PageError = 0;
		volatile HAL_StatusTypeDef status;
		status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
		return status;
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
