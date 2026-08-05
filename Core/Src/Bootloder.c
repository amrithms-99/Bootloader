/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : Bootloder.c
  * @brief          : Bootloader helper functions.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "Bootloder.h"
#include "App_Header.h"
#include "main.h"
#include <string.h>

/* Global variables used by the bootloader logic */

volatile bool Header_Received = false;
uint8_t RX_Buffer[CHUNK_SIZE];

app_header_t App_Header;
uint32_t RemainingBytes = 0;
uint32_t CurrentChunkSize = 0;
static uint32_t CurrentFlashAddress = APP_ADDRESS;
static uint32_t OTA_Flag = 0;
volatile bool chunkReceived = false;
typedef void (*pFunction)(void);

/* Parsed header fields (from UART header appended to image) */
static uint32_t ReceivedImageSize = 0;
static uint32_t ReceivedImageCrc = 0;
static uint32_t ReceivedImageMagic = 0;
uint8_t DataReceived;
uint8_t Header_Buffer[HEADER_SIZE];
uint8_t DataReceived;
bool ValidFirmwareImage;

uint8_t Flash_Erase_Data(void);
static uint8_t Flash_Write_Data(void);

void Bootloader_Init(void)
{
    Header_Received = false;
    ValidFirmwareImage = false;
    RemainingBytes = 0U;
    CurrentChunkSize = 0U;
    CurrentFlashAddress = APP_ADDRESS;
    OTA_Flag = 0U;
    ReceivedImageSize = 0U;
    ReceivedImageCrc = 0U;
    ReceivedImageMagic = 0U;
    DataReceived = RX_INPROGRESS;
    chunkReceived = false;
    memset(Header_Buffer, 0, sizeof(Header_Buffer));
    memset(RX_Buffer, 0, sizeof(RX_Buffer));
    memset(&App_Header, 0, sizeof(App_Header));
}

bool OTA_Flag_Check(void)
{
    memcpy(&App_Header, (void *)APP_HEADER_ADDR, sizeof(app_header_t));
    OTA_Flag = App_Header.OTA_Flag;
    return (OTA_Flag == 1U);
}

void OTA_Prcocess_Header(void)
{

	if(Header_Received == true)
	{
		uint32_t *tempHeaderBuffer = (uint32_t *)Header_Buffer;
		App_Header.MagicNumber = tempHeaderBuffer[0];
		App_Header.ImageSize = tempHeaderBuffer[1];
		App_Header.CRCVal = tempHeaderBuffer[2];
		if(((App_Header.ImageSize > 0U) && (App_Header.ImageSize < 0x01000000U)) && (App_Header.MagicNumber == APP_HEADER_MAGIC_NUM))
		{
			Header_Received = true;
			RemainingBytes = App_Header.ImageSize;
			ValidFirmwareImage = true;
		}
		else
		{
			ValidFirmwareImage=false;

		}
	}

}

uint8_t OTA_Process_Firmware(void)
{
    if (RemainingBytes == 0U)
    {
        return OK;
    }

    if (!chunkReceived)
    {
        return NOT_OK;
    }

    chunkReceived = false;
    Flash_Write_Data();

    RemainingBytes -= CurrentChunkSize;

    if (RemainingBytes > 0)
    {
        CurrentChunkSize = (RemainingBytes > CHUNK_SIZE) ?
                           CHUNK_SIZE : RemainingBytes;

        if (HAL_UART_Receive_IT(&huart3,
                                RX_Buffer,
                                CurrentChunkSize) != HAL_OK)
        {
            return NOT_OK;
        }
    }

    return (RemainingBytes == 0) ? OK : NOT_OK;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    //recieve operation is complete. Process the received data
  if (huart->Instance == USART3)
  {
	  if(OTA_MainState == OTA_STATE_PROCESS_HEADER)
	  {
		  Header_Received = true;

	  }
    if((OTA_MainState == OTA_STATE_PROCESS_FIRMWARE)&&(chunkReceived == false))
    {
        chunkReceived = true;
       

    }
    
  }
}
 uint8_t Flash_Erase_Data(void)
{
  FLASH_EraseInitTypeDef erase_init_parm;
  static uint32_t sector_error;

  erase_init_parm.Sector = FLASH_SECTOR_3;
  erase_init_parm.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init_parm.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  erase_init_parm.NbSectors = 2;

  HAL_FLASH_Unlock();
  if (HAL_FLASHEx_Erase(&erase_init_parm, &sector_error) != HAL_OK)
  {
    return HAL_FLASH_GetError();
  }
  else
  {
    return HAL_OK;
  }

  
}

static uint8_t Flash_Write_Data(void)
{
  for (uint32_t i = 0U; i < CurrentChunkSize; i++)
  {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, CurrentFlashAddress++, RX_Buffer[i]);
  }
  return 1U;
}

void JumpToApplication(uint32_t addr)
{
  uint32_t JumpAddress = *(uint32_t *)(addr + 0x04);
  pFunction Jump = (pFunction)JumpAddress;

  HAL_RCC_DeInit();
  HAL_DeInit();
  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL = 0U;

  SCB->VTOR = addr;
  __set_MSP(*(uint32_t *)addr);
  Jump();
}

uint8_t Bootloader_CrcCheck(void)
{
  uint32_t addr = APP_ADDRESS;
  uint32_t wordCount = (App_Header.ImageSize + 3U) / 4U;
  uint32_t crc32_val = HAL_CRC_Calculate(&hcrc, (uint32_t *)addr, wordCount);
  if(crc32_val == App_Header.CRCVal)
  {
	  return 1;
  }
  else
  {
	  return 0;
  }
}

