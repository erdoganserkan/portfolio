/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "lpc_types.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_ssp.h"
#include "AppCommon.h"
#include "SDRAM_K4S561632C_32M_16BIT.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "ErrorLog.h"
#include "SpiFlash.h"
#include "FlashDriver.h"
#include "Intro2.h"
#include "AppSettings.h"

extern uint8_t sdmmc_ResloadInit(void);
extern uint8_t sdmmc_LoadRes(uint8_t ResGroup, ResInfoType *ResInfoPtr);

/* Private variables ---------------------------------------------------------*/
static uint32_t CurReadAddr;	/* current read address */
static uint32_t CurWriteAddr;	/* current write address */

static 	sSpiFlash_InfoHeader ihdr = {
	.init_state = SPI_FLASH_INIT_UNKNOWN,
	.padding = 0,
	.data_start_adr = 0,
};

/*******************************************************************************
* Function Name  : df_read_open
* Description    : Read initialize functions
* Input          : - addr: read address
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void df_read_open(uint32_t addr)
{
	CurReadAddr = addr;	/* current read address */
}


/*******************************************************************************
* Function Name  : df_write_open
* Description    : Write initialize functions
* Input          : - addr: write address
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void df_write_open(uint32_t addr)
{
	CurWriteAddr=addr;	/* current write address */
}

/*******************************************************************************
* Function Name  : df_read
* Description    : Read functions
* Input          : - buf: received data of pointer
*                  - size: read data of size
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void df_read(uint8_t *buf, uint32_t size)
{
	TRACEM("CurReadAddr(%u), size(%u)\n", CurReadAddr, size);
	if( CurReadAddr + size <= MAX_ADDR ) {
		SPI_Flash_ReadData( CurReadAddr, buf, size );
		CurReadAddr += size;
	}
}


/*******************************************************************************
* Function Name  : df_write
* Description    : Write functions
* Input          : - buf: write data of pointer
*                  - size: write data of size
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void df_write(uint8_t *buf, uint32_t size)
{
	if( CurWriteAddr + size <= MAX_ADDR ) {
		SPI_Flash_WriteData( CurWriteAddr, buf, size, TRUE, TRUE);	
		CurWriteAddr += size;
	}
}

/*******************************************************************************
* Function Name  : df_read_seek
* Description    : adjustment read address 	
* Input          : - addr: new read address
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void df_read_seek(uint32_t addr)
{
	df_read_close();
	df_read_open(addr);
}

/*******************************************************************************
* Function Name  : df_write_seek
* Description    : adjustment write address
* Input          : - addr: new write address 
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void df_write_seek(uint32_t addr)
{
	df_write_close();
	df_write_open(addr); 
}

uint32_t df_write_get_pos(void) 
{
	return CurWriteAddr;
}

uint32_t df_read_get_pos(void) 
{
	return CurReadAddr;
}

/*******************************************************************************
* Function Name  : df_read_close
* Description    : None
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void df_read_close(void)
{
}

/*******************************************************************************
* Function Name  : df_write_close
* Description    : None
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void df_write_close(void)
{
}

uint8_t spi_flash_LoadRes(uint8_t ResGroup, ResInfoType *ResInfoPtr)
{
	uint8_t Found = FALSE;
	uint8_t res = 0;
	uint8_t *SDRAMAdr;
	volatile uint32_t indx;
	sSpiFlash_ResInfo temp_res;
	sResGroupInfo grp_info;
	uint32_t crc32, read32;
	
	if(SPI_FLASH_NULL == ihdr.init_state) {
		res = 0x2;
		goto RES_READ_FAILED;
	}
	
	SDRAMAdr = ResInfoPtr->SDRAM_adr;
	
	// Read group info // 
	df_read_seek(sizeof(sSpiFlash_InfoHeader) + sizeof(crc32) + ((sizeof(sResGroupInfo)+sizeof(crc32)) * ResGroup));
	df_read((uint8_t *)&grp_info, sizeof(sResGroupInfo));
	#if(TRUE == SPI_FLASH_CHECK_CRC32)
		df_read((uint8_t *)&read32, sizeof(read32));	// Read CRC32 of res-Group-Data //
		crc32 = get_block_crc32((uint8_t *)&grp_info, sizeof(grp_info));
		if(read32 != crc32) {
			ERRM("RES-GRP-INFO-CRC check FAILED; expected(0x%X), read(0x%X)\n", crc32, read32);
			res = 0x1;
			goto RES_READ_FAILED;
		} else {
			TRACEM("RES-GRP-INFO-CRC check OK crc32(0x%X)\n", crc32);
		}
	#endif
	TRACEM("RES-GRP(%u:(%s)), res_count(%u) start_adr(%u)\n", \
		grp_info.res_group, ResGroupDirs[grp_info.res_group], grp_info.group_res_num, grp_info.group_start_adr);
	// Find ResName in matching record data in SpiFlashInfo Sector //
	df_read_seek(grp_info.group_start_adr);
	for(indx=0 ; indx<grp_info.group_res_num ; indx++) {
		// Read resource-info // 
		df_read((uint8_t *)&temp_res, sizeof(sSpiFlash_ResInfo));	// read resource info entry, read pointer incremented automatically  // 
		TRACEM("READ-RES -> name(%s), adr(%u), len(%u)\n", temp_res.res_name, temp_res.res_adr, temp_res.res_len);
		#if(TRUE == SPI_FLASH_CHECK_CRC32)
			// Read resource-info crc & check it's validty // 
			df_read((uint8_t *)&read32, sizeof(read32));	// Read CRC32 of res-info //
			crc32 = get_block_crc32((uint8_t *)&temp_res, sizeof(temp_res));
			if(read32 != crc32) {
				ERRM("RES-INFO-CRC check FAILED; expected(0x%X), read(0x%X)\n", crc32, read32);
				res = 0x1;
				goto RES_READ_FAILED;
			} else {
				TRACEM("RES-INFO-CRC check OK crc32(0x%X)\n", crc32);
			}
		#endif		
		if(0 == strncmp(ResInfoPtr->name, temp_res.res_name, RES_NAME_MAX_LENGTH)) {	// Record FOUND // 
			DEBUGM("RES(%s) FOUND @ADR(%u):LEN(%u) in SPI-Flash\n", ResInfoPtr->name, temp_res.res_adr, temp_res.res_len);
			Found = TRUE;
			break;
		} 
		#if(FALSE == SPI_FLASH_CHECK_CRC32)
			df_read_seek(df_read_get_pos() + sizeof(crc32) + temp_res.res_len + sizeof(crc32));	// skip this resource's "header's crc32", data and "data's crc32" // 
		#else
			df_read_seek(df_read_get_pos() + temp_res.res_len + sizeof(crc32));	// skip this resource's data and "data's crc32" // 
		#endif
	}
	if(FALSE == Found) {
		res = 0x1;
		goto RES_READ_FAILED;
	}

	// Read resource's data // 
	df_read_seek(temp_res.res_adr);
	df_read(SDRAMAdr, temp_res.res_len);
	#if(TRUE == SPI_FLASH_CHECK_CRC32)	
		// Read resource's data crc32 //
		df_read((uint8_t *)&read32, sizeof(read32));	// Read CRC32 of res-info //
		crc32 = get_block_crc32(SDRAMAdr, temp_res.res_len);
		if(read32 != crc32) {
	f		ERRM("RES-DATA-CRC check FAILED; expected(0x%X), read(0x%X)\n", crc32, read32);
			res = 0x1;
			goto RES_READ_FAILED;
		} else {
			TRACEM("RES-DATA-CRC check OK crc32(0x%X)\n", crc32);
		}
	#endif
	ResInfoPtr->size = temp_res.res_len;

	return res;

RES_READ_FAILED:
	ResInfoPtr->size = 0;
	return res;
}

uint8_t spi_flash_ResloadInit(void) 
{
	uint8_t retval = 0;
	uint32_t  ChipID = 0, ManuID = 0, DevID = 0;
	uint32_t crc32, tempu32;
	uint8_t FlashFormat = FALSE;

	if(SPI_FLASH_INIT_UNKNOWN != ihdr.init_state) {
		return retval;		// Spi-Flash initialization related action done before // 
	}
	
	init_TPs(5);	// Used for detectipn of forcing application to formating spi-flash //
	SSP_HW_Init();
	SPI_Flash_Reset();
	SPI_Flash_ReadID( JEDEC_ID, &ChipID );
	SPI_Flash_ReadID(Manufacturer_ID, &ManuID);
	SPI_Flash_ReadID(Device_ID, &DevID);

	ChipID &= (~(0xFF<<24));				/* retain low 24 bits data */
	DEBUGM("SPI Flash ID(0x%X)\n", ChipID);

	INFOM("sizeof(GlobalHdr(%u), ResInfo(%u), GroupInfo(%u))\n", \
		sizeof(sSpiFlash_InfoHeader), sizeof(sSpiFlash_ResInfo), sizeof(sResGroupInfo));
	#if(TRUE == TP5_FORCE_FLASH_FORMAT)
		if(!READ_TP(TP5_PORT_NUM, TP5_PIN_NUM)) {
			INFOM("TP5 button press detected, Forcing SPI-Flash-Format\n");
			FlashFormat = TRUE;
		}
	#endif
	if(FALSE == FlashFormat) {
		// check spi_flash first sector, if spi_flash is initialized read resource data table // 
		df_read_open( 0 );
		df_read((uint8_t *)&ihdr, sizeof(ihdr));	// Read InfoSector Header // 
		crc32 = get_block_crc32((uint8_t *)&ihdr, sizeof(ihdr));
		df_read((uint8_t *)&tempu32, sizeof(tempu32));	// Read CRC32 of InfoSector Header // 
		if(tempu32 != crc32) {
			ERRM("SpiFlash InfoHeader CRC-FAIL, read(0x%X), expected(0x%X)\n", tempu32, crc32);
			FlashFormat = TRUE;
			// push new error //
			sErrStore temp =  {.err_type = ERR_SDMMC_SPIFLASH_NULL };
			push_err(&temp);
			retval = 0x1;
		} else {
				INFOM("Spiflash InfoHeader CRC-OK (0x%X)\n", crc32);
		}
		if((SPI_FLASH_RES_INIT_DONE == ihdr.init_state)) {	// SpiFlash initialization is Done 
			volatile uint32_t adr;
			uint32_t new_val;
			INFOM("Spiflash init DONE BEFORE\n");
		} else {
			ERRM("Spi-Flash is NULL, Formatting\n");
			FlashFormat = TRUE;
		}
	}
	if(TRUE == FlashFormat) {	// Spi Flash is NULL, we must do do first initialization ; Do SD-MMC initialization // 
		uint8_t RMPics_SmallMask_init = FALSE;
		INFOM("Spiflash FORMAT Required, Starting to Erase\n");
		INFOM("Erasing From Sector:%u -> Sector%u\n", 0, SEC_MAX);
		SPI_Flash_Erase( 0, SEC_MAX );	// Erase Sectors //
		INFOM("SPI Flash Erase DONE (%u->%u)\n", 0, SEC_MAX);
		
		// Store default application settings into spi-flash // 
		AppSettings_StoreDefaults();
		// Try to initialize resources from SD-MMC to spi-Flash // 
		if(0 == sdmmc_ResloadInit()) {
			extern sRLE8Pic_Header *infop;
			static uint8_t pics_loaded = FALSE;
			uint32_t global_data_adr = sizeof(sSpiFlash_InfoHeader) + sizeof(crc32)+ \
				APP_PICs_GROUP_COUNT*(sizeof(sResGroupInfo) + sizeof(crc32));
			volatile uint8_t grp_indx; 
			volatile uint16_t indx;
			uint8_t common_mask_cnt, mask_ok;
			uint16_t real_grp_res_cnt;

			sSpiFlash_ResInfo *temp_res_info_ptr = (sSpiFlash_ResInfo *)calloc(sizeof(sSpiFlash_ResInfo),1);
			if(NULL == temp_res_info_ptr)
				while(STALLE_ON_ERR);
			ResInfoType *TempRes_ptr = (ResInfoType *)calloc(sizeof(ResInfoType), 1);
			if(NULL == TempRes_ptr)
				while(STALLE_ON_ERR);
			sResGroupInfo *temp_grp_info_ptr = (sResGroupInfo *)calloc(sizeof(sResGroupInfo), 1);
			if(NULL == temp_grp_info_ptr)
				while(STALLE_ON_ERR);
			char *common_masks = (char *)calloc(RES_NAME_MAX_LENGTH, 5);
			if(NULL == common_masks)
				while(STALLE_ON_ERR);
			
			INFOM("SpiFlash: SDMMC INIT DONE, INITIAL global_data_adr(%u)\n", global_data_adr);
			df_write_seek(global_data_adr);
			// Read all resources from SD-MMC and store them into SpiFlash // 
			for(grp_indx=0 ; grp_indx<APP_PICs_GROUP_COUNT ; grp_indx++) {
				INFOM("\n\n\n-------Starting %u.th Res Group (%s) initialization --------\n", grp_indx, ResGroupDirs[grp_indx]);
				memset(common_masks, 0, sizeof(RES_NAME_MAX_LENGTH*5));
				common_mask_cnt = 0;
				real_grp_res_cnt = 0;
				temp_grp_info_ptr->group_res_num = 0;
				temp_grp_info_ptr->group_start_adr = global_data_adr;
				temp_grp_info_ptr->res_group = grp_indx;
				DEBUGM("RES-GROUP(%u:%s) : START-UP global_data_adr(%u)\n", grp_indx, ResGroupDirs[grp_indx], global_data_adr);
				for(indx=0	;	indx<GroupInfos[grp_indx].IconCount ;	indx++) {						
					if(INTRO2_PICs == grp_indx) {	// Initial JOB for INTRO2_PICs //
						if(FALSE == pics_loaded) {
							Intro2_LoadPics2RAM(TRUE);	// Force to load pictures data from sd-mmc // 
							pics_loaded = TRUE;
						}
					} else {	// Initial job for others // 
						SBResources[grp_indx][indx].SDRAM_adr = (uint8_t *)RUNTIME_PICs_SDRAM_START;
						mask_ok = FALSE;
					}
					// Check resource existance in SD-MMC, except INTRO2_PICs, they are loaded by Intro2_LoadPics2RAM() function // 
					if(((INTRO2_PICs == grp_indx) && (0 != infop[indx].size)) 
						|| ((INTRO2_PICs != grp_indx) && (0 == sdmmc_LoadRes(grp_indx, &(SBResources[grp_indx][indx]))))) {	
						if(INTRO2_PICs == grp_indx) {
							DEBUGM("INTRO2-RES(%u) exist in SD-RAM, size(%u)\n", indx, infop[indx].size);
						} else {
							DEBUGM("RES(%s) SD-MMC Load DONE, size(%u)\n", SBResources[grp_indx][indx].name, SBResources[grp_indx][indx].size);
						}
						// Resource init OK, Create Spiflash entry for it // 
						temp_res_info_ptr->res_adr = global_data_adr + sizeof(sSpiFlash_ResInfo) + sizeof(crc32);
						memset(temp_res_info_ptr->res_name, 0, sizeof(temp_res_info_ptr->res_name));
						if(INTRO2_PICs == grp_indx) {
							temp_res_info_ptr->res_len = infop[indx].size;
							sprintf(temp_res_info_ptr->res_name, "cobra%03u_rle8.raw", indx+1);
						} else {
							temp_res_info_ptr->res_len = SBResources[grp_indx][indx].size;
							strncpy(temp_res_info_ptr->res_name, SBResources[grp_indx][indx].name, RES_NAME_MAX_LENGTH);
						}
						// Write resource info structure (resource header) // 
						df_write_seek(global_data_adr);
						df_write((uint8_t *)temp_res_info_ptr, sizeof(sSpiFlash_ResInfo)); 
						DEBUGM("RES-INFO(name:%s, @ADR(%u), len(%u)) Recorded @ADR(%u)\n", \
							temp_res_info_ptr->res_name, temp_res_info_ptr->res_adr, temp_res_info_ptr->res_len, global_data_adr);
						// Write res-info crc32 // 
						crc32 = get_block_crc32((uint8_t *)temp_res_info_ptr, sizeof(sSpiFlash_ResInfo));
						df_write_seek(global_data_adr + sizeof(sSpiFlash_ResInfo));
						df_write((uint8_t *)&crc32, sizeof(crc32)); 
						DEBUGM("RES-INFO-CRC32(0x%X) Recorded @ADR(%u)\n", crc32, df_write_get_pos());
						// Write real resource data // 
						df_write_seek(temp_res_info_ptr->res_adr);
						if(INTRO2_PICs == grp_indx)
							df_write(infop[indx].fdata, temp_res_info_ptr->res_len);
						else
							df_write(SBResources[grp_indx][indx].SDRAM_adr, temp_res_info_ptr->res_len);
						DEBUGM("RES-DATA(%s) Recorded @ADR(%u)\n", temp_res_info_ptr->res_name, temp_res_info_ptr->res_adr);
						// Write real-data crc32 //
						if(INTRO2_PICs == grp_indx)
							crc32 = get_block_crc32(infop[indx].fdata, temp_res_info_ptr->res_len);
						else
							crc32 = get_block_crc32(SBResources[grp_indx][indx].SDRAM_adr, temp_res_info_ptr->res_len);
						df_write_seek(temp_res_info_ptr->res_adr + temp_res_info_ptr->res_len);
						df_write((uint8_t *)&crc32, sizeof(crc32)); 
						DEBUGM("RES-DATA-CRC32(0x%X) Recorded @ADR(%u)\n", crc32, df_write_get_pos());
						// Increment global address pointer as "resource info header" and "real_data" // 
						global_data_adr = (temp_res_info_ptr->res_adr + temp_res_info_ptr->res_len + sizeof(crc32));
						real_grp_res_cnt++;
						DEBUGM("New real_grp_res_cnt(%u), global_data_adr(%u)\n", real_grp_res_cnt, global_data_adr);
					}
					if((INTRO2_PICs == grp_indx) || (RUNTIME_FONTs == grp_indx) || (AUDIO_TRACs == grp_indx))
						continue;
					// Check resource mask existance in SD-MMC // 
					if(NULL != SBResources[grp_indx][indx].ResMaskPtr) {	// Resource is using COMMON MASK object // 
						volatile uint8_t indy;
						uint8_t exist = FALSE;
						for(indy=0 ; indy<5 ; indy++) {	// Cehck if we have already stored this object before or NOT // 
							if(0 == strcmp(SBResources[grp_indx][indx].ResMaskPtr, common_masks + (indy*RES_NAME_MAX_LENGTH))) {	
								exist = TRUE;
								break;
							}
						}
						if(FALSE == exist) {	// common-mask not recorded before, this is first time // 
							*TempRes_ptr = SBResources[grp_indx][indx];
							TempRes_ptr->SDRAM_adr = (uint8_t *)RUNTIME_PICs_SDRAM_START;
							memset(TempRes_ptr->name, 0, sizeof(TempRes_ptr->name));
							strncpy(TempRes_ptr->name, TempRes_ptr->ResMaskPtr, RES_NAME_MAX_LENGTH);	// Mask name must be Resource Name now // 
							DEBUGM("CommonMask(%s) init NOT-DONE before\n", TempRes_ptr->name);	// Dont change this line order // 
							if(0 == sdmmc_LoadRes(grp_indx, TempRes_ptr)) {	// New common-mask load DONE // 
								DEBUGM("CommonMask(%s) Load from SD-MMC DONE, size(%u)\n", TempRes_ptr->name, TempRes_ptr->size);
								DEBUGM("CommonMask(%s) Located @%u.th indx\n", TempRes_ptr->name, common_mask_cnt);
								strncpy(common_masks + (RES_NAME_MAX_LENGTH * (common_mask_cnt++)), TempRes_ptr->name, RES_NAME_MAX_LENGTH);	// record it into common-mask list // 
								mask_ok = TRUE;
							}
							else {
								ERRM("CommonMask(%s) Load from SD-MMC FAILED\n", TempRes_ptr->name);
							}
						} else {
							DEBUGM("CommonMask(%s) ALREADY located @ %u.th indx\n", common_masks + (RES_NAME_MAX_LENGTH * indy), indy);
						}
							
					} else {	// Resource has PRIVATE MASK Object 
						char *MaskStart = NULL;
						*TempRes_ptr = SBResources[grp_indx][indx];
						TempRes_ptr->SDRAM_adr = (uint8_t *)RUNTIME_PICs_SDRAM_START;
						// set Mask-Resource name // 
						MaskStart = strrchr(TempRes_ptr->name,'.');
						if(NULL != MaskStart) {
							strcpy(MaskStart,"Mask.bmp");
							if(0 == sdmmc_LoadRes(grp_indx, TempRes_ptr)) {	// New private-mask load DONE // 
								DEBUGM("PrivateMask(%s) Load from SD-MMC DONE, size(%u)\n", TempRes_ptr->name, TempRes_ptr->size);
								mask_ok = TRUE;
							} else {
								ERRM("PrivateMask(%s) Load from SD-MMC FAILED\n", TempRes_ptr->name);
							}
						}
					}
					if(TRUE == mask_ok) {
						// Resource-Mask init OK, Create Spiflash entry for it // 
						temp_res_info_ptr->res_adr = global_data_adr + sizeof(sSpiFlash_ResInfo) + sizeof(crc32);
						temp_res_info_ptr->res_len = TempRes_ptr->size;
						memset(temp_res_info_ptr->res_name, 0, sizeof(temp_res_info_ptr->res_name));
						strncpy(temp_res_info_ptr->res_name, TempRes_ptr->name, RES_NAME_MAX_LENGTH);
						// Write resource info structure (header) // 
						df_write_seek(global_data_adr);
						df_write((uint8_t *)temp_res_info_ptr, sizeof(sSpiFlash_ResInfo)); 
						DEBUGM("RES-MASK-INFO(%s) Recorded @ADR(%u)\n", temp_res_info_ptr->res_name, global_data_adr);
						// Write res-info crc32 // 
						crc32 = get_block_crc32((uint8_t *)temp_res_info_ptr, sizeof(sSpiFlash_ResInfo));
						df_write_seek(global_data_adr + sizeof(sSpiFlash_ResInfo));
						DEBUGM("RES-INFO-CRC32(0x%X) Recorded @ADR(%u)\n", crc32, df_write_get_pos());
						df_write((uint8_t *)&crc32, sizeof(crc32)); 
						// Write real resource data // 
						df_write_seek(temp_res_info_ptr->res_adr);
						df_write(TempRes_ptr->SDRAM_adr, temp_res_info_ptr->res_len); 
						DEBUGM("RES-MASK-DATA(%s) Recorded @ADR(%u)\n", temp_res_info_ptr->res_name, temp_res_info_ptr->res_adr);
						// Write real-data crc32 //
						crc32 = get_block_crc32(TempRes_ptr->SDRAM_adr, temp_res_info_ptr->res_len);
						df_write_seek(temp_res_info_ptr->res_adr + temp_res_info_ptr->res_len);
						DEBUGM("RES-DATA-CRC32(0x%X) Recorded @ADR(%u)\n", crc32, df_write_get_pos());
						df_write((uint8_t *)&crc32, sizeof(crc32)); 
						// Increment global address pointer as "resource info header" and "real_data" // 
						global_data_adr = (temp_res_info_ptr->res_adr + temp_res_info_ptr->res_len + sizeof(crc32));
						real_grp_res_cnt++;
						DEBUGM("NEW real_grp_res(%u), NEW global_data_adr(%u)\n", real_grp_res_cnt, global_data_adr);
					}
				}
				// Resource-Group initialization completed: Update Resource Group Data //
				temp_grp_info_ptr->group_res_num = real_grp_res_cnt;
				df_write_seek(sizeof(sSpiFlash_InfoHeader) + sizeof(crc32)+ grp_indx * (sizeof(sResGroupInfo)+sizeof(crc32)));
				df_write((uint8_t *)temp_grp_info_ptr, sizeof(sResGroupInfo)); 
				DEBUGM("RES-GROUP-INFO(%u:%s) recorded @ADR(%u)\n", grp_indx, ResGroupDirs[grp_indx], df_write_get_pos());
				// Write Res-Group data CRC // 
				crc32 = get_block_crc32((uint8_t *)temp_grp_info_ptr, sizeof(sResGroupInfo));
				df_write_seek(sizeof(sSpiFlash_InfoHeader) + sizeof(crc32) + \
					grp_indx * (sizeof(sResGroupInfo)+sizeof(crc32)) + sizeof(sResGroupInfo));
				df_write((uint8_t *)&crc32, sizeof(crc32)); 
				DEBUGM("RES-GROUP-INFO-CRC(0x%X) recorded @ADR(%u)\n", crc32, df_write_get_pos());
			}

			free(temp_res_info_ptr);
			free(TempRes_ptr);
			free(temp_grp_info_ptr);
			free(common_masks);
				
			// Write Spi-Flash General Header from RAM to spi-Flash // 
			ihdr.init_state = SPI_FLASH_RES_INIT_DONE;
			ihdr.padding[0] = ihdr.padding[1] = ihdr.padding[2] = 0;
			ihdr.data_start_adr = sizeof(sSpiFlash_InfoHeader) + sizeof(crc32) + \
				APP_PICs_GROUP_COUNT*(sizeof(sResGroupInfo) + sizeof(crc32)); // TODO // 
			df_write_seek( 0 );
			df_write((uint8_t *)&ihdr, sizeof(ihdr)); 
			DEBUGM("SPI-FLASH-HEADER (state(%u), data_start_adr(%u)) recorded\n", ihdr.init_state, ihdr.data_start_adr);
			// Write Spi-flash Header CRC //
			crc32 = get_block_crc32((uint8_t *)&ihdr, sizeof(ihdr));
			df_write_seek(sizeof(ihdr));
			df_write((uint8_t *)&crc32, sizeof(crc32)); 
			DEBUGM("SPI-FLASH-HEADER-CRC(0x%X) recorded @ADR(%u)\n", crc32, df_write_get_pos());
			
			#if(TRUE == TP5_FORCE_FLASH_FORMAT)
				if(0 == READ_TP(TP5_PORT_NUM, TP5_PIN_NUM)) {
					INFOM("Please RELEASE TP5, After system will REBOOT\n");
					while(0 == READ_TP(TP5_PORT_NUM, TP5_PIN_NUM)) {
						App_waitMS(500);
					}
				}
			#endif
			// Reboot MCU, To do resources initialization from spi-flash // 
			NVIC_SystemReset();
		} else {
			// Update Spi-Flash general Header State // 
			ihdr.init_state |= SPI_FLASH_APP_SETTING_INIT_DONE;	// Only App-Settings initialization is DONE //
			// Push an error object into stack // 
			sErrStore temp =  {.err_type = ERR_SDMMC_SPIFLASH_NULL };
			push_err(&temp);
			INFOM("SDMMC INIT FAILED\n");
			retval = 0x2;
		}
	}
	
	return retval;
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
