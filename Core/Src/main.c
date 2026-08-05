/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "math.h"
#include "stdbool.h"
#include "string.h"
#include "App_Header.h"
#include "Bootloder.h"

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

CRC_HandleTypeDef hcrc;
UART_HandleTypeDef huart3;
OTA_State_e OTA_MainState;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_USART3_UART_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_USART3_UART_Init();

  Bootloader_Init();

  /* Infinite loop */
    OTA_MainState = OTA_STATE_IDLE;
    app_header_t *header = (app_header_t *)APP_HEADER_ADDR;
  while (1)
  {
    // led blinking to indicate bootloader is running
   // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_Delay(5000);
    HAL_UART_Transmit_IT(&huart3, (uint8_t *)"Bootloader is running\r\n", 23);
    
    switch (OTA_MainState)
    {
    case OTA_STATE_IDLE:
        /* code */
        if (OTA_Flag_Check() == 1)
        {
            OTA_MainState = OTA_STATE_PROCESS_HEADER;
            HAL_UART_Receive_IT(&huart3, Header_Buffer, HEADER_SIZE);
            HAL_UART_Transmit_IT(&huart3, (uint8_t *)"OTA update is requested\r\n", 24);
        }
        else
        {
            // jump to application
            HAL_UART_Transmit_IT(&huart3, (uint8_t *)"No OTA update requested. Jumping to application\r\n", 50);
           // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 0);
            JumpToApplication(APP_ADDRESS);
        }
        break;
    case OTA_STATE_PROCESS_HEADER:
        /* code */
        if(ValidFirmwareImage == true)

        {
            HAL_UART_Transmit_IT(&huart3, (uint8_t *)"Valid firmware image header received. Starting firmware update\r\n", 70);
            //erase flash sectors before writing the firmware image
            if(Flash_Erase_Data() == HAL_OK)
            {
                HAL_UART_Transmit_IT(&huart3, (uint8_t *)"Flash sectors erased successfully\r\n", 36);
                OTA_MainState = OTA_STATE_PROCESS_FIRMWARE;
            }
            else
            {
                HAL_UART_Transmit_IT(&huart3, (uint8_t *)"Error erasing flash sector. Aborting OTA update\r\n", 55);
                OTA_MainState = OTA_STATE_ERROR;
            }
            

        }
        else
        {
            HAL_UART_Transmit_IT(&huart3, (uint8_t *)"Invalid firmware image header received. Aborting OTA update\r\n", 66);
            OTA_MainState = OTA_STATE_ERROR;
        }
        break;

    case OTA_STATE_PROCESS_FIRMWARE:
        /* code */

        if(OTA_Process_Firmware()==1)
        {
            HAL_UART_Transmit_IT(&huart3, (uint8_t *)"Firmware image received completely. Verifying CRC\r\n", 55);
            OTA_MainState = OTA_STATE_VERIFY;
        }
        break;
    case OTA_STATE_VERIFY:
        /* code */
        //if crc check is successful, jump to application
        if(Bootloader_CrcCheck() == 1U)
        {
            HAL_UART_Transmit_IT(&huart3, (uint8_t *)"Firmware image verified successfully. Jumping to application\r\n", 65);
            OTA_MainState = OTA_STATE_JUMP_TO_APP;
        }
        else
        {
            HAL_UART_Transmit_IT(&huart3, (uint8_t *)"Firmware image verification failed. Aborting OTA update\r\n", 60);
            OTA_MainState = OTA_STATE_ERROR;
        }
        

        break;
        case OTA_STATE_JUMP_TO_APP:
        /* code */
        HAL_UART_Transmit_IT(&huart3, (uint8_t *)"OTA update completed. Jumping to application\r\n", 45);
        JumpToApplication(APP_ADDRESS);

        break;
    case OTA_STATE_ERROR:
        /* code */
        break;
    
    default:
        break;
    	}
    

  	  }
    
   
}


/* USER CODE BEGIN 2 */
/* Add any custom user functions in Bootloder.c */
/* USER CODE END 2 */

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* MPU Configuration */
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
