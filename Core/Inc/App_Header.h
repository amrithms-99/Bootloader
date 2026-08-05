/*
 * App_Header.h
 *
 *  Minimal application header: keep original struct members, add enum.
 */

#ifndef APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_
#define APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_

#include <stdint.h>
#define APP_HEADER_MAGIC_NUM 0xFFFF

typedef enum
{
    OTA_STATE_IDLE = 0,
    OTA_STATE_PROCESS_HEADER,
    OTA_STATE_PROCESS_FIRMWARE,
    OTA_STATE_VERIFY,
	OTA_STATE_JUMP_TO_APP,
    OTA_STATE_ERROR
} OTA_State_e;


typedef struct
{
    
	uint32_t MagicNumber;
	uint32_t ImageSize;
	uint32_t CRCVal;
	uint8_t OTA_Flag;
} app_header_t;

#endif /* APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_ */
