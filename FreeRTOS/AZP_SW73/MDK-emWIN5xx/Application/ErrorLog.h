#ifndef ERROR_LOG_H
#define ERROR_LOG_H

#include <stdio.h>
#include <stdint.h>
#include "UMDShared.h"

#define ERR_RECORD_STACK_DEPTH	(10)

typedef enum {
	ERR_CMD_TIMEOUT			= 0,	// CMD send to detector but detector response not received // 
	ERR_UNEXPECTED_PKT	= 1,	// Unexpected packet received from detector // 
	ERR_NVSTORAGE_IO		= 2,	// Non-Volatile storage IO error occurred // 
	ERR_SDMMC_SPIFLASH_NULL		= 3,	// Spi flash is NULL and SD-MMC init FAILED //
	ERR_SPIFLASH_CRC_FAIL		= 4,	// SPI-Flash CRC error occrred // 
	ERR_UART_PERIPHERAL		= 5,	// UART peripheral error occurred // 
	
	APP_ERR_COUNT
} eAPP_ERR_TYPE;

#pragma pack(1)
typedef struct {
	eAPP_ERR_TYPE err_type;
	union {		// ERR_NVSTORAGE_IO	data 
		uint32_t uart_err;
		struct {
			uint8_t media_type;
			uint32_t addr;
		} NVErr;	
		UmdPkt_Type TimeoutPkt;	// ERR_CMD_TIMEOUT data 
		UmdPkt_Type UnexpectedPkt;	// ERR_UNEXPECTED_PKT data 
	} ErrCore;
} sErrStore;
#pragma pack()

extern void push_err(sErrStore *err_obj_ptr);
extern sErrStore *pop_err(void);
extern void LogErr(char *str_buf, const sErrStore *err_datap);

#endif
