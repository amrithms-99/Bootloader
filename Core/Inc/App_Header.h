/*
 * App_Header.h
 *
 *  Created on: 16-May-2026
 *      Author: amrit
 */

#ifndef APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_
#define APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_


typedef struct
{
	uint32_t magic_num;
	uint32_t crc;
	uint32_t size;
}app_header_t ;

#endif /* APPLICATION_USER_CORE_APP_HEADER_APP_HEADER_H_ */
