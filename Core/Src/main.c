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
#define HEADER_SIZE 16

UART_HandleTypeDef huart3;
#define APP_HEADER_ADDR 0x8010000
#define APP_ADDRESS 0x8018000
#define CHUNK_SIZE 256
#define MAGIC_NUM 0xFF
#define E_OK 0
#define E_NOT_OK 1

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_USART3_UART_Init(void);
/* Bootloader_HeaderCheck removed: OTA_Process_UART handles header parsing */
static uint8_t Flash_Erase_Data(void);
static uint8_t Flash_Write_Data(void);
static uint8_t Bootloader_CrcCheck(void);
static void OTA_Process_UART(void);
void OTA_Flag_Check(void);

/* Globals */
uint8_t Header_Buffer[HEADER_SIZE];
bool Header_Received = false;
uint8_t RX_Buffer[CHUNK_SIZE];
static bool ValidFirmwareImage = false;
app_header_t App_Header;
uint32_t RemainingBytes = 0;
uint32_t CurrentChunkSize = 0;
static uint32_t CurrentFlashAddress = APP_ADDRESS;
static uint32_t OTA_Flag = 0;
typedef void (*pFunction)(void);

/* Parsed header fields (from UART header appended to image) */
static uint32_t ReceivedImageSize = 0;
static uint32_t ReceivedImageCrc = 0;
static uint32_t ReceivedImageMagic = 0;

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

  /* Infinite loop */
  while (1)
  {
    // led blinking to indicate bootloader is running
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    HAL_Delay(5000);

    OTA_Flag_Check();

    if (OTA_Flag == 1)
    {
      OTA_Process_UART();
    }
    else
    {
      // Jump to Application
      JumpToApplication(APP_ADDRESS);
    }
  }
}

/* Read OTA flag from header area in flash. */
void OTA_Flag_Check(void)
{
  /* Read the OTA flag from the application header*/
  uint8_t *otaFlag = (uint8_t *)APP_HEADER_ADDR;
  //copy the OTA flag value to the global variable
  OTA_Flag = (*(uint32_t *)otaFlag);
}

/* Handle UART reception: header first, then chunks, write to flash and verify CRC.
Roll back not yet implemeted  */
static void OTA_Process_UART(void)
{
  /* If header not yet received, read the header over UART */
  if (Header_Received == false)
  {
    if (HAL_UART_Receive(&huart3, Header_Buffer, HEADER_SIZE, 5000) == HAL_OK)
    {
      uint32_t *ph = (uint32_t *)Header_Buffer;
      ReceivedImageMagic = ph[0];
      ReceivedImageSize = ph[1];
      ReceivedImageCrc = ph[2];

      /* Basic integrity check */
      if ((ReceivedImageSize > 0) && (ReceivedImageSize < 0x01000000))
      {
        Header_Received = true;
        ValidFirmwareImage = true;
        RemainingBytes = ReceivedImageSize;
        CurrentFlashAddress = APP_ADDRESS;
        Flash_Erase_Data();
      }
      else
      {
        Header_Received = false;
        ValidFirmwareImage = false;
        return;
      }
    }
    else
    {
      /* failed to receive header */
      return;
    }
  }

  /* If we have a valid image, receive chunks and write to flash */
  if (ValidFirmwareImage)
  {
    while (RemainingBytes != 0)
    {
      CurrentChunkSize = (RemainingBytes > CHUNK_SIZE) ? CHUNK_SIZE : RemainingBytes;

      if (HAL_UART_Receive(&huart3, RX_Buffer, CurrentChunkSize, 2000) == HAL_OK)
      {
        RemainingBytes -= CurrentChunkSize;
        Flash_Write_Data();
      }
      else
      {
        /* receive error - abort */
        Header_Received = false;
        ValidFirmwareImage = false;
        return;
      }
    }

    /* verify CRC */
    if (Bootloader_CrcCheck() == 1)
    {
      HAL_UART_Transmit(&huart3, (uint8_t *)"Application firmware valid", 24, 100);
      /* Do not clear OTA flag here; application will confirm and clear it. */
      JumpToApplication(APP_ADDRESS);
    }
    else
    {
      /* CRC failed */
      Header_Received = false;
      ValidFirmwareImage = false;
    }
  }
}

static uint8_t Flash_Erase_Data(void)
{
  FLASH_EraseInitTypeDef erase_init_parm;
  static uint32_t sector_error;

  erase_init_parm.Sector = FLASH_SECTOR_3;
  erase_init_parm.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init_parm.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  erase_init_parm.NbSectors = 2; // how many sectors to erase
  HAL_FLASH_Unlock();
  if (HAL_FLASHEx_Erase(&erase_init_parm, &sector_error) != HAL_OK)
  {
    return HAL_FLASH_GetError();
  }
  return 1;
}

static uint8_t Flash_Write_Data(void)
{
  for (uint32_t i = 0; i < CurrentChunkSize; i++)
  {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, CurrentFlashAddress++, RX_Buffer[i]);
  }
  return 1;
}

void JumpToApplication(uint32_t addr)
{
  uint32_t JumpAddress = *(uint32_t *)(addr + 0x04);
  pFunction Jump = (pFunction)JumpAddress;

  HAL_RCC_DeInit();
  HAL_DeInit();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  SCB->VTOR = addr;
  __set_MSP(*(uint32_t *)addr);
  Jump();
}

/* Bootloader_HeaderCheck removed - header parsing moved to OTA_Process_UART() */

uint8_t Bootloader_CrcCheck(void)
{
  if (ReceivedImageSize == 0)
    return 0;

  uint32_t addr = APP_ADDRESS;
  uint32_t wordCount = (ReceivedImageSize + 3) / 4;
  uint32_t crc32_val = HAL_CRC_Calculate(&hcrc, (uint32_t *)addr, wordCount);
  return (crc32_val == ReceivedImageCrc) ? 1 : 0;
}

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
