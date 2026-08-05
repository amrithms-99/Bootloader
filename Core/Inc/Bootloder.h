/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : Bootloder.h
  * @brief          : Bootloader helper definitions and public API.
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __BOOTLODER_H
#define __BOOTLODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "App_Header.h"
#include <stdbool.h>

#define HEADER_SIZE      16U
#define APP_HEADER_ADDR  0x8010000U
#define APP_ADDRESS      0x8018000U
#define CHUNK_SIZE       256U
//STATE MACHINE VARIABLES
#define OK 1
#define NOT_OK 0

extern CRC_HandleTypeDef hcrc;
extern UART_HandleTypeDef huart3;
extern OTA_State_e OTA_MainState;
extern volatile bool chunkReceived;
extern uint8_t Header_Buffer[HEADER_SIZE];
extern uint8_t DataReceived;
extern bool ValidFirmwareImage;

void Bootloader_Init(void);
bool OTA_Flag_Check(void);
void OTA_Prcocess_Header(void);
uint8_t OTA_Process_Firmware(void);
void JumpToApplication(uint32_t addr);
uint8_t Bootloader_CrcCheck(void);
uint8_t OTA_Process_FirmwareImage(void);
extern uint8_t Flash_Erase_Data(void);



typedef enum {
	RX_INPROGRESS = 0,     	     
	RX_COMPLETE       
} Data_Transfer_Status_e;

#ifdef __cplusplus
}
#endif

#endif /* __BOOTLODER_H */
