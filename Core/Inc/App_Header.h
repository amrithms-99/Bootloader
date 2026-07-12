/*
 * App_Header.h
 *
 *  Minimal application header: keep original struct members, add enum.
 */

#ifndef APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_
#define APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_

#include <stdint.h>

typedef enum {
    OTA_STATE_IDLE = 0,      /* no update pending */
    OTA_STATE_PENDING,       /* header received, waiting validation/apply */
    OTA_STATE_VALIDATED,     /* header+CRC checked and image written */
    OTA_STATE_SUCCESS        /* application confirmed successful boot */
} ota_state_e;

typedef struct
{
    uint8_t OTA_Flag;
    uint8_t OTA_Status;
} app_header_t;

#endif /* APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_ */
