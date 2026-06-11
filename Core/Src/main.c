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
#define HEADER_SIZE

UART_HandleTypeDef huart3;
#define APP_HEADER_ADDR 0x8010000
#define APP_ADDRESS 0x8018000
#define CHUNK_SIZE 256
#define MAGIC_NUM 0xFF
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_USART3_UART_Init(void);
uint8_t Header_Buffer[16];
bool Header_Received = false;
uint8_t RX_Buffer[CHUNK_SIZE];
static bool validFirmwareImage = false;
static uint32_t Num_of_Bytes;
app_header_t App_Header;


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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	  /* USER CODE END WHILE */
	  	  //one led should

	  	 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
	  	 HAL_Delay(5000);
	  	 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
	  	 uint8_t ret_val = ApplicationImage_Check();
	  	 if(ret_val == 1)
	  	 {
	  		 // magic num check
	  		 HAL_UART_Transmit(huart, (uint8_t*)"Magic Num check failed", 30, 100);

	  	 }
	  	 if(ret_val ==2 )
	  	 {
	  		 // crc check failed
	  		HAL_UART_Transmit(huart, (uint8_t*)"CRC check failed", 30, 100);
	  	 }
	  	 else
	  	 {
	  		HAL_UART_Transmit(huart, (uint8_t*)"Application firmware valid", 40, 100);
	  		JumpToApplication(APP_ADDR);
	  	 }

	  	 if(OTA_Flag == 1)
	  	 {
	  		HAL_UART_Receive_IT(&huart3, Header_Buffer, HEADER_SIZE);


	  	 }
    /* USER CODE END WHILE */



    /* USER CODE BEGIN 3 */


  /* USER CODE END 3 */
}

void OTA_Flag_Check()
{
	// read OTA flag
	uint32_t OTA_Flag = *(uint32_t *)(APP_HEADER_ADDR + 12);



}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{


	if(huart->Instance == USART3)
	    {
	        if(Header_Received == false)
	        {

	            memcpy(&app_header,
	                   HeaderBuffer,
	                   sizeof(app_header_t));

	            Header_Received = true;

	            // Validate header
	            if(app_header.magic_num == MAGIC_NUM)
	            {
	            	// start receiving firmware chunks
	            	validFirmwareImage = true;
	            	Header_Received = true ;
	            	HAL_UART_Receive_IT(&huart3,RX_Buffer,CHUNK_SIZE);
	            	//HAL_UART_Receive_IT(&huart3,RX_Buffer,CHUNK_SIZE);
	            }
	            else
	            {
	            	//do not flash as firmware is corrupt
	            	Header_Received = false;
	            	validFirmwareImage = false;
	            }

	        }

	        else
	        {
	        	// header is recieved . Start recieving the new firmware chunks
	        	if(validFirmwareImage == true)
	        	{
	        		Flash_Write_Data();
	        		HAL_UART_Receive_IT(&huart3,RX_Buffer,CHUNK_SIZE);

	        	}
	        }
	    }
	}

}

uint8_t Flash_Write_Data()
{
	// unlock the flash memory

	FLASH_EraseInitTypeDef erase_init_parm;
	static uint32_t *sector_error;
	app_header_t app_header;
	erase_init_parm.Sector = FLASH_SECTOR_3;
	erase_init_parm.TypeErase = FLASH_TYPEERASE_SECTORS;
	erase_init_parm.VoltageRange= FLASH_VOLTAGE_RANGE_3;
	erase_init_parm.NbSectors = 2; // how many sector to erase
	HAL_FLASH_Unlock();
	// erase Sector 3 and 4
	if(HAL_FLASHEx_Erase(&erase_init_parm, &sector_error) != HAL_OK)
	{
		return HAL_FLASH_GetError();

	}


}


void JumpToApplication(uint32_t addr)
  {

  	uint32_t JumpAddress = *(uint32_t*)(addr + 0x04);

  	pFunction Jump = (pFunction) JumpAddress;
  	HAL_RCC_DeInit();
  	HAL_DeInit();
  	SysTick->CTRL = 0;
  	SysTick->LOAD = 0;
  	SysTick->VAL = 0;

  	 SCB->VTOR = addr;
  	__set_MSP(*(uint32_t *) addr);
  	Jump();

  }

uint8_t ApplicationImage_Check(void)
  {
	  const app_header_t *app_hdr = (const app_header_t *)APP_HEADER_ADDR;
	  uint32_t flash_size = app_header_t->size;
	  uint32_t reset_handler = *(uint32_t*)(APP_ADDR + 4);
	  static const crc32_val = HAL_CRC_Calculate(hcrc,(uint32_t*)addr, flash_size/4);

	  if((app_hdr->magic_num != MAGIC_NUM) &&(reset_handler & 0xFF00000 != 0x8000000) )
	  {
		  return 1;

	  }
	  if(crc32_val != flash_size)
	  {
		  return 2;
	  }


	   else
	  {
		  return 0;

	  }


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
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

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
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
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
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
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
#ifdef USE_FULL_ASSERT
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
