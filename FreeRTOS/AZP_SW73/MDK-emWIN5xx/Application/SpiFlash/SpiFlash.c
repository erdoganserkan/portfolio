#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "lpc_types.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_ssp.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "SDRAM_K4S561632C_32M_16BIT.h"
#include "debug_frmwrk.h"
#include "Serial.h"
#include "monitor.h"
#include "GuiConsole.h"
#include "SpiFlash.h"

#if(UMD_DETECTOR_BOARD == USED_HW)

static void __write_SR_SSTF016B(uint8_t sr_num, uint8_t new_val);
static uint8_t __read_SR_SSTF016B(uint8_t sr_num);

static inline void __WAIT_UNTIL_IDLE_SSTF016B(uint16_t step_wait_ms) {
	register uint8_t Stat1RgVal;
	do {
		Stat1RgVal = __read_SR_SSTF016B(1);
		if(0 != step_wait_ms)
			App_waitMS(step_wait_ms);
	} while ((Stat1RgVal & 0x03) == 0x03);							  /* Waits until the chip is idle */
}

static inline void __WAIT_UNTIL_IDLE_S25FL(uint16_t step_wait_ms) {
	register uint8_t Stat1RgVal;
	do {
		Stat1RgVal = __read_SR_SSTF016B(1);
		if(0 != step_wait_ms)
			App_waitMS(step_wait_ms);
	} while ((Stat1RgVal & 0x01) == 0x01);							  /* Status-Reg 0th bit */
}


/*******************************************************************************
* Function Name  : SSP_HW_Init
* Description    : SPI FLASH Configuration
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void SSP_HW_Init(void)
{
    SSP_CFG_Type SSP_ConfigStruct;
#if(POWERMCU_DEV_BOARD == USED_HW)
	/*
	 * Initialize SPI pin connect
	 * P5.4 -  SSEL - used as GPIO
	 * P1.31 - SCK
	 * P1.18 - MISO
	 * P0.13 - MOSI
	 */

	/* Default P5.4 - GPIO */
	PINSEL_ConfigPin(1, 31, 2);	 /* SSP1_SCK */
	PINSEL_ConfigPin(1, 18, 5);	 /* SSP1_MISO */
	PINSEL_ConfigPin(0, 13, 2);  /* SSP1_MOSI */

	/* GPIO5 can no use library */

    /* P5.4 CS is output */
	// LPC_GPIO5->DIR |= 1 << CS_PIN_NUM;
	GPIO_SetDir(5, 1<<CS_PIN_NUM, 1);
#elif(UMD_DETECTOR_BOARD == USED_HW)
	/*
	 * Initialize SPI pin connect
	 * P0.6 -  SSEL - used as GPIO
	 * P1.19 - SCK
	 * P1.18 - MISO
	 * P0.13 - MOSI
	 */
	PINSEL_ConfigPin(1, 19, 5);	 /* SSP1_SCK */
	PINSEL_ConfigPin(1, 18, 5);	 /* SSP1_MISO */
	PINSEL_ConfigPin(0, 13, 2);  /* SSP1_MOSI */

  /* P0.6 CS is output */
	// LPC_GPIO0->DIR |= 1 << CS_PIN_NUM;
	GPIO_SetDir(0, 1<<CS_PIN_NUM, 1);
	SPI_FLASH_CS_HIGH();
#endif
	/* initialize SSP configuration structure to default */
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_ConfigStruct.ClockRate = 75000000UL;

	/* Initialize SSP peripheral with parameter given in structure above */
	SSP_Init(SPI_FLASH_SSP, &SSP_ConfigStruct);
	/* Enable SSP peripheral */
	SSP_Cmd(SPI_FLASH_SSP, ENABLE);
}


/*******************************************************************************
* Function Name  : SSTF016B_ReadData
* Description    : SST25VF016B read function, can be choose	read ID or read data 
* Input          : - Dst: Destination address 0x0 - MAX_ADDR
*                  - RcvBufPt: Receive buffer pointer
*                  - NByte: number of bytes to be read	
* Output         : successful return OK, failed return ERROR
* Return         : return result
*******************************************************************************/
int SSTF016B_ReadData(uint32_t Dst, uint8_t* RcvBufPt ,uint32_t NByte)
{
	volatile uint32_t i = 0;

	if ( ( Dst+NByte > MAX_ADDR ) || ( NByte == 0 ) ) {
		ERRM("Dest address overflow\n");
		return -1;	 /*	ERROR */
	}
	
  SPI_FLASH_CS_LOW_SHORT();
	__LPC17xx_SPI_SendRecvByte(0x0B); 						/* 80MHz Read CMD: Send read command */
	__LPC17xx_SPI_SendRecvByte(((Dst & 0xFFFFFF) >> 16));	/* Send 3 byte address command */
	__LPC17xx_SPI_SendRecvByte(((Dst & 0xFFFF) >> 8));
	__LPC17xx_SPI_SendRecvByte(Dst & 0xFF);
	__LPC17xx_SPI_SendRecvByte(0xFF);						/* Sending a dummy byte in order to read data	*/
	for ( i = 0; i < NByte; i++ ) {
		RcvBufPt[i] = __LPC17xx_SPI_SendRecvByte(0xFF);		/* Read data */
	}
  SPI_FLASH_CS_HIGH_SHORT();

	return 0;
}

/*******************************************************************************
* Function Name  : SSTF016B_ReadData
* Description    : SST25VF016B read function, can be choose	read ID or read data 
* Input          : - Dst: Destination address 0x0 - MAX_ADDR
*                  - RcvBufPt: Receive buffer pointer
*                  - NByte: number of bytes to be read	
* Output         : successful return OK, failed return ERROR
* Return         : return result
*******************************************************************************/
int S25FL_ReadData(uint32_t Dst, uint8_t* RcvBufPt ,uint32_t NByte)
{
	  volatile uint32_t i = 0;
	
	  if ( ( Dst+NByte > MAX_ADDR ) || ( NByte == 0 ) ) {
		  ERRM("Dest address overflow\n");
		  return -1;   /* ERROR */
	  }
	  
	SPI_FLASH_CS_LOW_SHORT();
	  __LPC17xx_SPI_SendRecvByte(0x0B); 					  /* 80MHz Read CMD: Send read command */
	  __LPC17xx_SPI_SendRecvByte(((Dst & 0xFFFFFF) >> 16));   /* Send 3 byte address command */
	  __LPC17xx_SPI_SendRecvByte(((Dst & 0xFFFF) >> 8));
	  __LPC17xx_SPI_SendRecvByte(Dst & 0xFF);
	  __LPC17xx_SPI_SendRecvByte(0xFF); 					  /* Sending a dummy byte in order to read data   */
	  for ( i = 0; i < NByte; i++ ) {
		  RcvBufPt[i] = __LPC17xx_SPI_SendRecvByte(0xFF);	  /* Read data */
	  }
	SPI_FLASH_CS_HIGH_SHORT();
	
	  return 0;
}

int SSTF016B_ReadSector(uint32_t Dst, sSPIReadType* spip)
{
	SSP_DATA_SETUP_Type spid = {
		.tx_data = NULL,
		.tx_cnt = 0,
		.rx_data = spip,
		.rx_cnt = sizeof(sSPIReadType),
		.length = sizeof(sSPIReadType)
	};

	if ( ( Dst+SEC_SIZE > MAX_ADDR ) || ( spip == 0 ) )	{
		ERRM("Dest address overflow\n");
		return -1;	 /*	ERROR */
	}
	
	SPI_FLASH_CS_LOW();
	if(-1 ==  SSP_ReadWrite (SPI_FLASH_SSP, &spid, SSP_TRANSFER_POLLING)) {
		ERRM("SSP_ReadWrite(Dst:0x%08X) FAILED\n", Dst);
		return -1;	 // FAILED // 
	}
	SPI_FLASH_CS_HIGH();

	return 0;
}

int S25FL_ReadSector(uint32_t Dst, sSPIReadType* spip)
{

}

/*******************************************************************************
* Function Name  : SSTF016B_ReadID
* Description    : SST25VF016B read ID function 
* Input          : - IDType: choose in JEDEC_ID, Device_ID, Manufacturer_ID
* Output         : - RcvbufPt: Receive buffer pointer
* Return         : successful return OK, failed return ERROR
*******************************************************************************/
int SSTF016B_ReadID(idtype IDType,uint32_t* RcvbufPt)
{
	uint32_t temp = 0;
              
	if (IDType == JEDEC_ID) {
		SPI_FLASH_CS_LOW();	
		__LPC17xx_SPI_SendRecvByte(0x9F);		 		         /* Send read JEDEC ID command (9Fh) */    	        
		temp = (temp | __LPC17xx_SPI_SendRecvByte(0x00)) << 8;  /* Receive Data */
		temp = (temp | __LPC17xx_SPI_SendRecvByte(0x00)) << 8;	
		temp = (temp | __LPC17xx_SPI_SendRecvByte(0x00)); 	     /* temp value is 0xBF2541 */
		SPI_FLASH_CS_HIGH();

		*RcvbufPt = temp;
		return 0;
	}
	
	if ((IDType == Manufacturer_ID) || (IDType == Device_ID) ) {
		SPI_FLASH_CS_LOW();	

		__LPC17xx_SPI_SendRecvByte(0x90);				/* Send read ID command (90h or ABh) */
		__LPC17xx_SPI_SendRecvByte(0x00);				/* Send address	*/
		__LPC17xx_SPI_SendRecvByte(0x00);				/* Send address	*/
		__LPC17xx_SPI_SendRecvByte(IDType);		    /* Send address - 00H or 01H */
		temp = __LPC17xx_SPI_SendRecvByte(0x00);	    /* Receive Data */

		SPI_FLASH_CS_HIGH();

		*RcvbufPt = temp;
		return 0;
	}
	else {
		return -1;	
	}
}

/*******************************************************************************
* Function Name  : SSTF016B_ReadID
* Description    : SST25VF016B read ID function 
* Input          : - IDType: choose in JEDEC_ID, Device_ID, Manufacturer_ID
* Output         : - RcvbufPt: Receive buffer pointer
* Return         : successful return OK, failed return ERROR
*******************************************************************************/
int S25FL_ReadID(idtype IDType,uint32_t* RcvbufPt)
{
	uint32_t temp = 0;
              
	if (IDType == JEDEC_ID) {
		SPI_FLASH_CS_LOW();	
		__LPC17xx_SPI_SendRecvByte(0x9F);		 		         /* Send read JEDEC ID command (9Fh) */    	        
		temp = (temp | __LPC17xx_SPI_SendRecvByte(0x00)) << 8;  /* Receive Data */
		temp = (temp | __LPC17xx_SPI_SendRecvByte(0x00)) << 8;	
		temp = (temp | __LPC17xx_SPI_SendRecvByte(0x00)); 	     /* temp value is 0xBF2541 */
		SPI_FLASH_CS_HIGH();

		*RcvbufPt = temp;
		return 0;
	}
	
	if ((IDType == Manufacturer_ID) || (IDType == Device_ID) ) {
		SPI_FLASH_CS_LOW();	

		__LPC17xx_SPI_SendRecvByte(0x90);				/* Send read ID command (90h or ABh) */
		__LPC17xx_SPI_SendRecvByte(0x00);				/* Send address	*/
		__LPC17xx_SPI_SendRecvByte(0x00);				/* Send address	*/
		__LPC17xx_SPI_SendRecvByte(IDType);		    /* Send address - 00H or 01H */
		temp = __LPC17xx_SPI_SendRecvByte(0x00);	    /* Receive Data */

		SPI_FLASH_CS_HIGH();

		*RcvbufPt = temp;
		return 0;
	}
	else {
		return -1;	
	}
}

// read data from status registers // 
static uint8_t __read_SR_SSTF016B(uint8_t sr_num) {
	uint8_t retval=0;
	if((1 > sr_num) || (3 < sr_num ))	// Return if sr_num 1,2 or 3 // 
		return 0;	
	switch(sr_num) {
		case 1:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x05);							    /* Send read status register 1 command */
			retval = __LPC17xx_SPI_SendRecvByte(0xFF);								    
			SPI_FLASH_CS_HIGH();			
			break;
		case 2:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x35);							    /* Send read status register 2 command */
			retval = __LPC17xx_SPI_SendRecvByte(0xFF);								    
			SPI_FLASH_CS_HIGH();			
			break;
		case 3:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x15);							    /* Send read status register 3 command */
			retval = __LPC17xx_SPI_SendRecvByte(0xFF);								    
			SPI_FLASH_CS_HIGH();			
			break;
		default:
			while(1);
			break;
	}
	
	return retval;
}

static uint8_t __read_SR_S25FL(uint8_t sr_num) {
	uint8_t retval=0;
	if((1 > sr_num) || (2 < sr_num ))	// Return if sr_num is NOT 1 or 2 // 
		return 0;	
	switch(sr_num) {
		case 1:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x05);							    /* Send read status register 1 command */
			retval = __LPC17xx_SPI_SendRecvByte(0xFF);								    
			SPI_FLASH_CS_HIGH();			
			break;
		case 2:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x07);							    /* Send read status register 2 command */
			retval = __LPC17xx_SPI_SendRecvByte(0xFF);								    
			SPI_FLASH_CS_HIGH();			
			break;
		default:
			while(1);
			break;
	}
	
	return retval;
}

 // write to status registers // 
 static void __write_SR_SSTF016B(uint8_t sr_num, uint8_t new_val) {
	if((1 > sr_num) || (3 < sr_num ))	// Return if sr_num 1,2 or 3 // 
		return;	

	SPI_FLASH_CS_LOW();			
	__LPC17xx_SPI_SendRecvByte(0x06);								/* Send write enable command */
	SPI_FLASH_CS_HIGH();			

	SPI_FLASH_CS_LOW();				
	__LPC17xx_SPI_SendRecvByte(0x50);		// Non-Volatile StatReg write enable //
	SPI_FLASH_CS_HIGH();	

	switch(sr_num) {
		case 1:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x01);							    /* Send write status register 1 command */
			__LPC17xx_SPI_SendRecvByte(new_val);								    
			SPI_FLASH_CS_HIGH();			
			break;
		case 2:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x31);							    /* Send write status register 2 command */
			__LPC17xx_SPI_SendRecvByte(new_val);								    
			SPI_FLASH_CS_HIGH();			
			break;
		case 3:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x11);							    /* Send write status register 3 command */
			__LPC17xx_SPI_SendRecvByte(new_val);								    
			SPI_FLASH_CS_HIGH();			
			break;
		default:
			while(1);
			break;
	}
}

static void __write_SR_S25FL(uint8_t sr_num, uint8_t new_val) {
	if(1 != sr_num)	// Return if sr_num is NOT 1// 
		return;	

	SPI_FLASH_CS_LOW();			
	__LPC17xx_SPI_SendRecvByte(0x06);								/* Send write enable command */
	SPI_FLASH_CS_HIGH();			

	switch(sr_num) {
		case 1:
			SPI_FLASH_CS_LOW();				
			__LPC17xx_SPI_SendRecvByte(0x01);							    /* Send write status register 1 command */
			__LPC17xx_SPI_SendRecvByte(new_val);								    
			SPI_FLASH_CS_HIGH();			
			break;
		default:
			while(1);
			break;
	}
}

/*******************************************************************************
* Function Name  : SSTF016B_WriteReg
* Description    : SST25VF016B write functions, can write a more data to the specified address
* Input          : - Dst: Destination address 0x0 - MAX_ADDR
*                  - RcvBufPt: Write buffer pointer
*                  - NByte: number of bytes to be write	
* Output         : None
* Return         : successful return OK, failed return ERROR
*******************************************************************************/
int SSTF016B_WriteData(uint32_t Dst, uint8_t* SndbufPt,uint32_t NByte, uint8_t verify, uint8_t check_erased)
{
	uint8_t SR1, SR3, temp_SR3;
	volatile uint32_t i = 0;
	uint8_t *buf = (uint8_t *)((SDRAM_BASE_ADDR + SDRAM_SIZE) - (PAGE_SIZE*2));

	if ( ( ( Dst + NByte - 1 > MAX_ADDR ) || ( NByte == 0 ) ) ) {
		return -1;	 /*	ERROR */
	}

	/* Make chip writable */
	SR1 = __read_SR_SSTF016B(1);
	__write_SR_SSTF016B(1, 0);	
	/* Set drive strength to maximum and wps=0 */
	SR3 = temp_SR3 = __read_SR_SSTF016B(3);
	// edit status register3 value to archieve maximum drive strenght // 
	temp_SR3 &= (~((uint8_t)0x3<<5));	// DRV0,DRV1 = 0x0;
	temp_SR3 &= (~((uint8_t)0x1<<2));	// WPS = 0 ; Block protection is controlled by SR1 bits //
	__write_SR_SSTF016B(3, temp_SR3);
 
#if(0)	
 for(i = 0; i < NByte; i++) {
		__SEND_CMD(0x06);	// Write Enable // 
		__SEND_CMD_ADR_BYTE(0x02, (Dst+i), (SndbufPt[i]));	// Page Program // 
		__WAIT_UNTIL_IDLE_SSTF016B();
	}
#else 
	uint32_t Send, Next;
	for( ; NByte ; NByte -= Send, Dst += Send, SndbufPt += Send) {
		Next = ((Dst/PAGE_SIZE)+1)*PAGE_SIZE;
		Send = ((Next-Dst)<NByte)?(Next-Dst):(NByte);

		if(TRUE == check_erased) {
			memset(buf, 0, Send);
			SSTF016B_ReadData(Dst, buf , Send);
			for(volatile uint32_t indx=0 ; indx<Send ; indx++)
				if(buf[indx] != 0xFF) {
					ERRM("SpiFlash erase Check FAILED @ Dst(%u)\n", Dst+indx);
					return -1;
				}
		} 
		
		__SEND_CMD(0x06);	// Write Enable // 
		__SEND_CMD_ADR_MULTI_BYTE_RW(0x02, Dst, SndbufPt, NULL, Send, 0);	// Page Program // 
		
		// If desired, Now read-back just burned page and verify // 
		if(TRUE == verify) {
			memset(buf, 0, Send);
			SSTF016B_ReadData(Dst, buf , Send);
			if(0 != memcmp(buf, SndbufPt, Send)) {
				ERRM("SpiFlash verify FAILED @ Dst(%u), Size (%u)\n", Dst, Send);
				return -1;
			}
		}
	}
#endif	
	__write_SR_SSTF016B(1, SR1);
	__write_SR_SSTF016B(3, SR3);

	return 0;		
}

/*******************************************************************************
* Function Name  : SSTF016B_WriteReg
* Description    : SST25VF016B write functions, can write a more data to the specified address
* Input          : - Dst: Destination address 0x0 - MAX_ADDR
*                  - RcvBufPt: Write buffer pointer
*                  - NByte: number of bytes to be write	
* Output         : None
* Return         : successful return OK, failed return ERROR
*******************************************************************************/
int S25FL_WriteData(uint32_t Dst, uint8_t* SndbufPt,uint32_t NByte, uint8_t verify, uint8_t check_erased)
{
	uint8_t SR1, SR3, temp_SR3;
	volatile uint32_t i = 0;
	uint8_t *buf = (uint8_t *)((SDRAM_BASE_ADDR + SDRAM_SIZE) - (PAGE_SIZE*2));

	if ( ( ( Dst + NByte - 1 > MAX_ADDR ) || ( NByte == 0 ) ) ) {
		return -1;	 /* ERROR */
	}
 
	uint32_t Send, Next;
	for( ; NByte ; NByte -= Send, Dst += Send, SndbufPt += Send) {
		Next = ((Dst/PAGE_SIZE)+1)*PAGE_SIZE;	// Get the next page's start address // 
		Send = ((Next-Dst)<NByte)?(Next-Dst):(NByte);

		if(TRUE == check_erased) {
			memset(buf, 0, Send);
			S25FL_ReadData(Dst, buf , Send);
			for(volatile uint32_t indx=0 ; indx<Send ; indx++)
				if(buf[indx] != 0xFF) {
					ERRM("SpiFlash erase Check FAILED @ Dst(%u)\n", Dst+indx);	
					return -1;
				}
		} 
		
		__SEND_CMD(0x06);	// Write Enable // 
		__SEND_CMD_ADR_MULTI_BYTE_RW(0x02, Dst, SndbufPt, NULL, Send, 0);	// Page Program // 
		
		// If desired, Now read-back just burned page and verify // 
		if(TRUE == verify) {
			memset(buf, 0, Send);
			S25FL_ReadData(Dst, buf , Send);
			if(0 != memcmp(buf, SndbufPt, Send)) {
				ERRM("SpiFlash verify FAILED @ Dst(%u), Size (%u)\n", Dst, Send);
				return -1;
			}
		}
	}

	return 0;		
}

/*******************************************************************************
* Function Name  : SSTF016B_Erase
* Description    : According to the sector number choose an efficient algorithms to erase
* Input          : - sec1: Start sector number ( 0 ~ MAX )
*                  - sec2: final sector number ( 0 ~ MAX )
* Output         : None
* Return         : successful return OK, failed return ERROR
*******************************************************************************/
int S25FL_Erase( uint32_t sec1, uint32_t sec2 )
{
	uint8_t  SR1, SR3, temp_SR3;
	uint32_t SecnHdAddr = 0;				
	uint32_t no_SecsToEr = 0;								/* number of sectors to be erased */
	uint32_t CurSecToEr = 0;								/* sector to be erase */
					
	TRACEM("%s()-> CALLED with (%u,%u)\n", __FUNCTION__, sec1, sec2);
	if ( ( sec1 > SEC_MAX ) || ( sec2 > SEC_MAX ) ) {
		return -1;	 /* ERROR */
	}
  
/* starting sector number is greater than the final sector number, internal adjustments */
	if ( sec1 > sec2 ) {
		uint8_t temp2 = sec1;
		sec1  = sec2;
		sec2  = temp2;
	} 
	/* If starting and ending sector number equal, then erase a single sector */
	if ( sec1 == sec2 ) {
		__SEND_CMD(0x06);		// Write Enable  // 

		SecnHdAddr = SEC_SIZE * sec1;						  /* Calculating the starting address of the sectors */
		__SEND_CMD_ADR(0xD8, SecnHdAddr);		// Sector Erase // 

		__WAIT_UNTIL_IDLE_S25FL(5);
		return 0;			
	}
	
	/* According to starting sector number and final sector number, Select a fastest erase algorithm */ 
	if ( sec2 - sec1 == SEC_MAX )	{

		__SEND_CMD(0x06);		// Write Enable // 
		__SEND_CMD(0xC7);	// Chip Erase with 0xC7 : type1 //
		__WAIT_UNTIL_IDLE_S25FL(100);

		//__SEND_CMD(0x06);		// Write Enable // 
		//__SEND_CMD(0x60);	// Chip erase with 0x60 : type2 // 
		//__WAIT_UNTIL_IDLE_S25FL();

		return (ENABLE);
	}
	
	no_SecsToEr = sec2 - sec1 +1;							  /* Get the number of sectors to be erased */
	CurSecToEr	= sec1; 									  /* starting sector to erase */
	
	/* using sector erase algorithm erase remaining sectors */
	while (no_SecsToEr >= 1) {
		__SEND_CMD(0x06);	// Write Enable // 

		SecnHdAddr = SEC_SIZE * CurSecToEr; 				  /* Calculating the starting address of the sectors */
		__SEND_CMD_ADR(0xD8, SecnHdAddr);	// Sector Erase // 
		__WAIT_UNTIL_IDLE_S25FL(10);
		CurSecToEr	+= 1;
		no_SecsToEr -=	1;
	}
  
	return 0;
}


/*******************************************************************************
* Function Name  : SSTF016B_Erase
* Description    : According to the sector number choose an efficient algorithms to erase
* Input          : - sec1: Start sector number ( 0 ~ MAX )
*                  - sec2: final sector number ( 0 ~ MAX )
* Output         : None
* Return         : successful return OK, failed return ERROR
*******************************************************************************/
int SSTF016B_Erase( uint32_t sec1, uint32_t sec2 )
{
	uint8_t  SR1, SR3, temp_SR3;
  	uint32_t SecnHdAddr = 0;	  			
	uint32_t no_SecsToEr = 0;				   			    /* number of sectors to be erased */
	uint32_t CurSecToEr = 0;	  						    /* sector to be erase */
					
	TRACEM("%s()-> CALLED with (%u,%u)\n", __FUNCTION__, sec1, sec2);
	if ( ( sec1 > SEC_MAX ) || ( sec2 > SEC_MAX ) ) {
		return -1;	 /*	ERROR */
	}
  
	/* Make chip writable */
	SR1 = __read_SR_SSTF016B(1);
	__write_SR_SSTF016B(1,0);
	/* Set drive strength to maximum and wps=0 */
	SR3 = temp_SR3 = __read_SR_SSTF016B(3);
	// Edit read register value to archieve maximum drive strenght // 
	temp_SR3 &= (~((uint8_t)0x3<<5));	// DRV0,DRV1 = 0x0;
	temp_SR3 &= (~((uint8_t)0x1<<2));	// WPS = 0 ; Block protection is controlled by SR1 bits //
	__write_SR_SSTF016B(3, temp_SR3);

/* starting sector number is greater than the final sector number, internal adjustments */
	if ( sec1 > sec2 ) {
		uint8_t temp2 = sec1;
		sec1  = sec2;
		sec2  = temp2;
	} 
	/* If starting and ending sector number equal, then erase a single sector */
	if ( sec1 == sec2 )	{
		__SEND_CMD(0x06);		// Write Enable  // 

		SecnHdAddr = SEC_SIZE * sec1;				          /* Calculating the starting address of the sectors */
		__SEND_CMD_ADR(0x20, SecnHdAddr);		// Sector Erase // 

		__WAIT_UNTIL_IDLE_SSTF016B(5);
		return 0;			
	}
	
    /* According to starting sector number and final sector number, Select a fastest erase algorithm */	
	if ( sec2 - sec1 == SEC_MAX )	{

		__SEND_CMD(0x06);		// Write Enable // 
		__SEND_CMD(0xC7);	// Chip Erase with 0xC7 : type1 //
		__WAIT_UNTIL_IDLE_SSTF016B(100);

		//__SEND_CMD(0x06);		// Write Enable // 
		//__SEND_CMD(0x60);	// Chip erase with 0x60 : type2 // 
		//__WAIT_UNTIL_IDLE_SSTF016B(10);

		return (ENABLE);
	}
	
	no_SecsToEr = sec2 - sec1 +1;					          /* Get the number of sectors to be erased */
	CurSecToEr  = sec1;								          /* starting sector to erase */
	
	/* take 8 sector erase algorithm */
	while (no_SecsToEr >= 8) {
		__SEND_CMD(0x06);		// Write Enable // 

		SecnHdAddr = SEC_SIZE * CurSecToEr;			          /* Calculating the starting address of the sectors */
		__SEND_CMD_ADR(0x52, SecnHdAddr);	// Block Erase // 
		__WAIT_UNTIL_IDLE_SSTF016B(10);
		CurSecToEr  += 8;
		no_SecsToEr -=  8;
	}
	/* using sector erase algorithm erase remaining sectors */
	while (no_SecsToEr >= 1) {
		__SEND_CMD(0x06);	// Write Enable // 

		SecnHdAddr = SEC_SIZE * CurSecToEr;			          /* Calculating the starting address of the sectors */
		__SEND_CMD_ADR(0x20, SecnHdAddr);	// Sector Erase // 
		__WAIT_UNTIL_IDLE_SSTF016B(10);
		CurSecToEr  += 1;
		no_SecsToEr -=  1;
	}
  
	/* erase end, restore the status register */
	__write_SR_SSTF016B(1, SR1);
	__write_SR_SSTF016B(3, SR3);

	return 0;
}

int S25FL_Reset( void ) 
{
	__SEND_CMD(0xF0);	// Reset Chip // 
}

int SSTF016B_Reset( void )
{
	__SEND_CMD(0x66);	// Reset Enable // 
	__SEND_CMD(0x99);	// Reset Chip // 

	return 0;
}

int SPI_Flash_Test(void) 
{
	uint8_t * const sdram_ptr = (uint8_t *)INTRO_RAW_PICs_AREA_START;
	uint32_t  ChipID = 0, ManuID = 0, DevID = 0;
	volatile uint32_t indx;
	uint8_t ax;
	char temp[128];
	uint8_t ky = 0;

	srand(0x56);

	// Init SSP HW // 
	SSP_HW_Init();
	
	// Read Special Registers // 
	ax = __SPI_Flash_SR_Read(1);
	DEBUGM("SPI_Flash(Reg1 : %u)\n", ax);

	// Read ID Datas // 
	SPI_Flash_Reset();
	SPI_Flash_ReadID( JEDEC_ID, &ChipID );
	SPI_Flash_ReadID(Manufacturer_ID, &ManuID);
	SPI_Flash_ReadID(Device_ID, &DevID);
	ChipID &= (~(0xFF<<24));				/* retain low 24 bits data */
	DEBUGM("%u: SPI Flash ChipID(0x%X)\n", ky++, ChipID);
	DEBUGM("%u: SPI Flash Manufacturer ID(0x%X)\n", ky++, ManuID);
	DEBUGM("%u: SPI Flash Device ID(0x%X)\n", ky++, DevID);

	// Erase a sector // 
	SPI_Flash_Erase(0,1);
	DEBUGM("%u: SPI Flash Sector-0 Erase DONE\n", ky++);

	// Fill sector with known data // 
	for(indx=SEC_SIZE-1 ;; indx--) {
		sdram_ptr[indx] = rand() & 0xFF;
		if(0 == indx) break;
	}
	SPI_Flash_WriteData(0, (uint8_t *)INTRO_RAW_PICs_AREA_START, SEC_SIZE, TRUE, TRUE);

	while(1);
}

#else
	void SSP_HW_Init(void) {}

	int SSTF016B_Erase( uint32_t sec1, uint32_t sec2 ) {}
	int SSTF016B_WriteData(uint32_t Dst, uint8_t* SndbufPt,uint32_t NByte, uint8_t verify, uint8_t check_erased) {}
	int SSTF016B_ReadData(uint32_t Dst, uint8_t* RcvBufPt ,uint32_t NByte) {}
	int SSTF016B_ReadID(idtype IDType,uint32_t* RcvbufPt) {}
	int SSTF016B_Reset( void ) {}
	
	int S25FL_Erase     (uint32_t sec1, uint32_t sec2) {}
	int S25FL_ReadID    (idtype IDType,uint32_t* RcvbufPt) {}
	int S25FL_WriteData  (uint32_t Dst, uint8_t* SndbufPt,uint32_t NByte, uint8_t verify, uint8_t check_erased) {}
	int S25FL_ReadData  (uint32_t Dst, uint8_t* RcvBufPt ,uint32_t NByte) {}
	int S25FL_Reset( void ) {}

	int SPI_Flash_Test(void) {}

#endif

