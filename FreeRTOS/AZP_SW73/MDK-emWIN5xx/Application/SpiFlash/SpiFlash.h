#ifndef SPI_FLASH_H
#define SPI_FLASH_H

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#ifdef __BUILD_WITH_EXAMPLE__
	#include "lpc177x_8x_libcfg.h"
#else
	#include "lpc177x_8x_libcfg_default.h"
#endif
#include "lpc177x_8x_ssp.h"
#include "AppCommon.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum ERTYPE{ Sec1, Sec8, Sec16, Chip } ErType;  
typedef enum IDTYPE{ Manufacturer_ID, Device_ID, JEDEC_ID } idtype;

/* Private define ------------------------------------------------------------*/
#if(SPI_FLASH_SST25VF016B == ACTIVE_SPI_FLASH)
	#define MAX_ADDR		0x1FFFFF	/* Maximum address :16MBit SPI Flash */
	#define	SEC_MAX     	511         /* Maximum sectors */
	#define PAGE_SIZE		0x100		/* 256B */
	#define SEC_SIZE		0x1000		/* Sector size : 4KB */
#elif(SPI_FLASH_W25Q128FV == ACTIVE_SPI_FLASH)
	#define MAX_ADDR		0xFFFFFF	/* Maximum address :128MBit SPI Flash */
	#define	SEC_MAX     	4095        /* Maximum sectors */
	#define PAGE_SIZE		0x100		/* 256B */
	#define SEC_SIZE		0x1000		/* Sector size : 4KB */
#elif(SPI_FLASH_S25FL512S == ACTIVE_SPI_FLASH)
	#define MAX_ADDR		0x3FFFFFF	/* Maximum address :512MBit SPI Flash */
	#define	SEC_MAX     	255       /* Maximum sectors */
	#define PAGE_SIZE		0x200		/* 512B */
	#define SEC_SIZE		0x40000		/* Sector size : 256KB */
#endif
#define SPI_FLASH_SIZE	(SEC_SIZE*(SEC_MAX+1))

#if(POWERMCU_DEV_BOARD == USED_HW)
	/* PORT number that /CS pin assigned on */
	#define CS_PORT_NUM		5
	/* PIN number that  /CS pin assigned on */
	#define CS_PIN_NUM		4

	#define SPI_FLASH_CS_LOW()    LPC_GPIO5->CLR = 1<<CS_PIN_NUM; 
	#define SPI_FLASH_CS_HIGH()   LPC_GPIO5->SET = 1<<CS_PIN_NUM; 
	#define SPI_FLASH_SSP					LPC_SSP1
#elif(UMD_DETECTOR_BOARD == USED_HW)
	/* PORT number that /CS pin assigned on */
	#define CS_PORT_NUM		0
	/* PIN number that  /CS pin assigned on */
	#define CS_PIN_NUM		6

	#define SPI_FLASH_CS_LOW()    {\
		LPC_GPIO0->CLR = (1<<CS_PIN_NUM);\
		for(volatile uint32_t indx=0xFFFF ; indx ; indx--) __NOP; \
	}
	#define SPI_FLASH_CS_LOW_SHORT()    {\
		LPC_GPIO0->CLR = (1<<CS_PIN_NUM);\
	}
	#define SPI_FLASH_CS_HIGH()   {\
		LPC_GPIO0->SET = (1<<CS_PIN_NUM);\
		for(volatile uint32_t indx=0xFFFF ; indx ; indx--) __NOP;  \
	}
	#define SPI_FLASH_CS_HIGH_SHORT()   {\
		LPC_GPIO0->SET = (1<<CS_PIN_NUM);\
	}
	#define SPI_FLASH_SSP					LPC_SSP1
#endif


#pragma pack(1)
typedef struct 
{
	uint8_t cmd;
	uint8_t adr[3];	// 24Bit address : Layout= MSB->LSB// 
	uint8_t dummy;
	uint8_t sector_data[SEC_SIZE];
} sSPIReadType;

#pragma pack()

/* Private function prototypes -----------------------------------------------*/
void  SSP_HW_Init     (void);
#if(SPI_FLASH_W25Q128FV == ACTIVE_SPI_FLASH)
	extern int   SSTF016B_Erase     (uint32_t sec1, uint32_t sec2);
	extern int   SSTF016B_ReadID    (idtype IDType,uint32_t* RcvbufPt);
	extern int   SSTF016B_WriteData  (uint32_t Dst, uint8_t* SndbufPt,uint32_t NByte, uint8_t verify, uint8_t check_erased);
	extern int   SSTF016B_ReadData  (uint32_t Dst, uint8_t* RcvBufPt ,uint32_t NByte);
	extern int SSTF016B_Reset( void );

	#define SPI_Flash_Erase 	SSTF016B_Erase
	#define SPI_Flash_ReadID	SSTF016B_ReadID
	#define SPI_Flash_WriteData	SSTF016B_WriteData
	#define SPI_Flash_ReadData	SSTF016B_ReadData
	#define SPI_Flash_Reset 	SSTF016B_Reset
	#define __SPI_Flash_SR_Read __read_SR_SSTF016B
	#define __SPI_Flash_SR_Write __write_SR_SSTF016B
	
#elif(SPI_FLASH_S25FL512S == ACTIVE_SPI_FLASH)
	extern int   S25FL_Erase     (uint32_t sec1, uint32_t sec2);
	extern int   S25FL_ReadID    (idtype IDType,uint32_t* RcvbufPt);
	extern int   S25FL_WriteData  (uint32_t Dst, uint8_t* SndbufPt,uint32_t NByte, uint8_t verify, uint8_t check_erased);
	extern int   S25FL_ReadData  (uint32_t Dst, uint8_t* RcvBufPt ,uint32_t NByte);
	extern int   S25FL_Reset( void );

	#define SPI_Flash_Erase 	S25FL_Erase
	#define SPI_Flash_ReadID	S25FL_ReadID
	#define SPI_Flash_WriteData	S25FL_WriteData
	#define SPI_Flash_ReadData	S25FL_ReadData
	#define SPI_Flash_Reset		S25FL_Reset
	#define __SPI_Flash_SR_Read	__read_SR_S25FL
	#define __SPI_Flash_SR_Write __write_SR_S25FL
#endif
extern int SPI_Flash_Test(void);
/*******************************************************************************
* Function Name  : __LPC17xx_SPI_SendRecvByte
* Description	 : Send one byte then recv one byte of response
* Input 		 : - byte_s: byte_s
* Output		 : None
* Return		 : None
* Attention 	 : None
*******************************************************************************/
static __inline uint8_t __LPC17xx_SPI_SendRecvByte (uint8_t byte_s) {
	/* wait for current SSP activity complete */
	while (SSP_GetStatus(SPI_FLASH_SSP, SSP_STAT_BUSY) ==  SET);

	SSP_SendData(SPI_FLASH_SSP, (uint16_t) byte_s);

	while (SSP_GetStatus(SPI_FLASH_SSP, SSP_STAT_RXFIFO_NOTEMPTY) == RESET);
	return (SSP_ReceiveData(SPI_FLASH_SSP));
}

static inline void __SEND_CMD_ADR_BYTE(uint8_t cmd, uint32_t adr, uint8_t byte) { 
		SPI_FLASH_CS_LOW();					
		__LPC17xx_SPI_SendRecvByte(cmd); 
		__LPC17xx_SPI_SendRecvByte((((adr) & 0xFFFFFF) >> 16));  /* Send 3 byte address command */	
		__LPC17xx_SPI_SendRecvByte((((adr) & 0xFFFF) >> 8));	
		__LPC17xx_SPI_SendRecvByte((adr) & 0xFF);	
		__LPC17xx_SPI_SendRecvByte((byte));	
		SPI_FLASH_CS_HIGH();			
}

static inline void __SEND_CMD_ADR_MULTI_BYTE_RW(uint8_t cmd, uint32_t adr, uint8_t *write_buf, \
	uint8_t *read_buf, uint32_t cnt, uint32_t dummy_cnt) { 
		SPI_FLASH_CS_LOW();					
		__LPC17xx_SPI_SendRecvByte(cmd); 
		__LPC17xx_SPI_SendRecvByte((((adr) & 0xFFFFFF) >> 16));  /* Send 3 byte address command */	
		__LPC17xx_SPI_SendRecvByte((((adr) & 0xFFFF) >> 8));	
		__LPC17xx_SPI_SendRecvByte((adr) & 0xFF);	
		while(dummy_cnt--) 
			__LPC17xx_SPI_SendRecvByte(0xFF);	
		if(NULL != write_buf) {
			if(NULL != read_buf) {
				while(cnt--)
					*read_buf++ = __LPC17xx_SPI_SendRecvByte((*write_buf++));	// send desired bytes, store read bytes 
			} else {
				while(cnt--)
					__LPC17xx_SPI_SendRecvByte((*write_buf++));	// send desired bytes, ignore read 
			}
		} else {
			if(NULL != read_buf) {
				while(cnt--)
					*read_buf++ = __LPC17xx_SPI_SendRecvByte(0x0);	// send dummy, store read bytes 
			} else {
				while(cnt--)
					__LPC17xx_SPI_SendRecvByte(0x0);	// send dummy bytes, ignore read 
			}
		}
		SPI_FLASH_CS_HIGH();			
}

static inline void __SEND_CMD_ADR(uint8_t cmd, uint32_t adr) { \
		SPI_FLASH_CS_LOW();					
		__LPC17xx_SPI_SendRecvByte(cmd); 
		__LPC17xx_SPI_SendRecvByte((((adr) & 0xFFFFFF) >> 16));  /* Send 3 byte address command */	
		__LPC17xx_SPI_SendRecvByte((((adr) & 0xFFFF) >> 8));	
		__LPC17xx_SPI_SendRecvByte((adr) & 0xFF);	
		SPI_FLASH_CS_HIGH();			
}

static inline void __SEND_CMD(uint8_t cmd) { 
		SPI_FLASH_CS_LOW();					
		__LPC17xx_SPI_SendRecvByte(cmd); 
		SPI_FLASH_CS_HIGH();			
}

#endif
