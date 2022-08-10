#ifndef __FLASHDRIVER_H 
#define __FLASHDRIVER_H 

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "RuntimeLoader.h"
#include "SpiFlash.h"

#define SPI_FLASH_APP_SETTING_INIT_DONE			(0x5A)
#define SPI_FLASH_RES_INIT_DONE							(0xA5)
#define SPI_FLASH_INIT_UNKNOWN				(0x1)		// State when program starts // 
#define SPI_FLASH_NULL								(0xFF)		// dont Change this definition, it is default byte after flash erase // 

#pragma pack(1)
// The first object of spi_flash is this  Header // 
__packed typedef struct {
	uint8_t init_state;	// SpiFlash resource init state (NO_INIT, INIT_DONE)
	uint8_t padding[3];	
	uint32_t data_start_adr;	// Real resource data start address //
} sSpiFlash_InfoHeader;

// The second object group is group-info structures, There are APP_PICs_GROUP_COUNT structures // 
__packed typedef struct {
	uint8_t res_group;	// res group index // 
	uint16_t group_res_num;		// number of resources in group including mased ones // 
	uint32_t group_start_adr;	// group resources start address // 	
} sResGroupInfo;

// The third object group is resource-info structures // 
__packed typedef struct {
	uint32_t res_adr;		// Resource address in flash memory // 
	uint32_t res_len;		// Resource length as bytes // 
	char res_name[RES_NAME_MAX_LENGTH];		// Resource name // 
} sSpiFlash_ResInfo;
#pragma pack()
// The last object group is RAW Real-Resource Data // 

// The application settings object is located in LAST SECTOR of spi-flash // 


/* Private function prototypes -----------------------------------------------*/
void df_read_open(uint32_t addr);
void df_write_open(uint32_t addr);
void df_read(uint8_t *buf,uint32_t size);
void df_write(uint8_t *buf,uint32_t size);
void df_read_seek(uint32_t addr);
void df_write_seek(uint32_t addr);
void df_read_close(void);
void df_write_close(void);
uint32_t df_write_get_pos(void);
uint32_t df_read_get_pos(void);

extern uint8_t spi_flash_ResloadInit(void);
extern uint8_t spi_flash_LoadRes(uint8_t ResGroup, ResInfoType *ResInfoPtr);

#endif 
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
