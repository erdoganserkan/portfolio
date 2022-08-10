#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "lpc177x_8x_gpio.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include <GUIDEMO.h>
#include <DIALOG.h>
#include "GLCD.h"
#include "BSP.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "Strings.h"
#include "APlay.h"
#include "Dac.h"
#include "Intro2.h"

// Memory Usage // 
// 1- Pics Info Header Array (RLE8 data size and pointer)
#define INTRO2_INFO_ARRAY_ADR		INTRO_RAW_PICs_AREA_START
#define INTRO2_INFO_ARRAY_SIZE	(sizeof(sRLE8Pic_Header)*(INTRO_PICS_INDX_MAX+1))
// 2- RAW8 data array (UNCOMPRESSED 8Bit/Pixel Raw Image Data)
#define INTRO2_RAW8_AREA_ADR		(INTRO_RAW_PICs_AREA_START + INTRO2_INFO_ARRAY_SIZE)
#define INTRO2_RAW8_AREA_SIZE		((uint32_t)XSIZE_PHYS * (uint32_t)YSIZE_PHYS)	// Full screen 8bit per pixel raw image data // 
// 3- RLE8 raw data of all pictures sequentially (Each pictures RLE8 compressed data)
#define INTRO2_CMAP_SIZE				(sizeof(uint16_t) * CMAP_COLOR_COUNT)
#define INTRO2_RLE8_DATA_ADR		(INTRO2_RAW8_AREA_ADR + INTRO2_RAW8_AREA_SIZE)

// common color-map for all pictures // 
sRLE8Pic_Header * const infop = (sRLE8Pic_Header *)INTRO2_INFO_ARRAY_ADR;	// Information structure array //
static uint8_t * const raw8p = (uint8_t *)INTRO2_RAW8_AREA_ADR;		// Common for all images, uncompressed 8bit/pixel data //

extern uint8_t sdmmc_ResloadInit(void);
extern uint8_t sdmmc_LoadRes(uint8_t ResGroup, ResInfoType *ResInfoPtr);

void raw_intro(void) 
{
	//volatile uint16_t *pLCDbuf = (uint16_t *)LCD_VRAM_BASE_ADDR;  /* LCD buffer start address */
	//memset((void *)LCD_VRAM_BASE_ADDR, FRAME_BUFFER_LENGTH, 0x40);
	GLCD_Clear(0,Red);
	LCD_X_SHOWBUFFER_INFO x = {.Index = 0};
	LCD_X_DisplayDriver(0, LCD_X_SHOWBUFFER, &x);

	GLCD_Clear(1,Blue);
	x.Index = 1;
	LCD_X_DisplayDriver(0, LCD_X_SHOWBUFFER, &x);

	GLCD_Clear(2, Yellow);
	x.Index = 2;
	LCD_X_DisplayDriver(0, LCD_X_SHOWBUFFER, &x);

	GLCD_Clear(3, Purple);
	x.Index = 3;
	LCD_X_DisplayDriver(0, LCD_X_SHOWBUFFER, &x);
}

#if(0)
static char cmap24[128][3] = {
	{214, 18, 92},
	{214, 18, 93},
	{214, 18, 94},
	{216, 18, 93},
	{216, 18, 94},
	{ 45, 97,234},
	{ 44, 98,233},
	{ 45, 98,234},
	{ 45, 98,235},
	{140, 97,234},
	{141, 97,233},
	{141, 97,235},
	{140, 98,233},
	{141, 98,234},
	{141, 98,235},
	{140,225,234},
	{140,225,235},
	{141,225,233},
	{140,226,233},
	{141,226,234},
	{141,226,235},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255},
	{255,255,255}
};
void decode_raw_pic(void) 
{
	uint16_t indx;
	uint16_t *fb = (uint16_t *)((uint16_t *)LCD_VRAM_BASE_ADDR + ((uint32_t)XSIZE_PHYS * (uint32_t)YSIZE_PHYS * (uint32_t)0));
	uint16_t red, green, blue;
	volatile int32_t read_bytes, read_temp, apix;
	char file_name[64];
	ResInfoType tres = {
		0,0,0,0,NULL, "", NULL, 0
	};
	
	memset(file_name, 0, sizeof(file_name));
	sprintf(file_name, "DptBack.raw");
	strncpy(tres.name, file_name, RES_NAME_MAX_LENGTH);
	tres.SDRAM_adr = (uint8_t *)INTRO2_RAW8_AREA_ADR;
	if(0 != LoadRes(INTRO2_PICs, &tres)) {
		ERRM("%s raw8 file load FAILED\n", file_name);
		while(TODO_ON_ERR);
	}

	for(indx=0 ; indx<CMAP_COLOR_COUNT ; indx++) {
		red = cmap24[indx][0];
		red >>= 3;
		green = cmap24[indx][1];
		green >>= 2;
		blue = cmap24[indx][2];
		blue >>= 3;
		// Store pixel color values in M565 format //
		cmap16[indx] = (blue<<11) | (green<<5) | (red<<0);
	}

	// Get real 16bit/pixel data by usage of color map //
	for(apix=0; apix<((uint32_t)XSIZE_PHYS * (uint32_t)YSIZE_PHYS) ;apix++) {
		// uint8_t *src = (uint8_t *)&cmap16[raw8[apix]];
		//uint8_t *dest = (uint8_t *)&fb[apix];
		//dest[0] = src[1];
		//dest[1] = src[0];
		fb[apix] = cmap16[0];
	}			

	LCD_X_SHOWBUFFER_INFO x = {.Index = 0};
	LCD_X_DisplayDriver(0, LCD_X_SHOWBUFFER, &x);
}
#endif

int Intro2_StorePics2Nand(void) 
{
	return 0;
}


// Read "Cmap + RLE8" data from picture files and store them into SDRAM sequentially // 
int Intro2_LoadPics2RAM(uint8_t force_sdmmc) 
{
	volatile uint16_t file_indx;
	uint8_t *fdatap = (uint8_t *)INTRO2_RLE8_DATA_ADR;   // Each picture's "cmap + rle8" data is located sequentially into SDRAM //
	ResInfoType tres = {
		0,0,0,0,NULL, "cobra000_rle8.raw", NULL
	};

	for(file_indx=0 ; file_indx<(INTRO_PICS_INDX_MAX+1) ; file_indx++) {
		// open & read data from rle8 encoded file // 
		if((INTRO_PICS_GAP_START_INDX > file_indx) || (INTRO_PICS_GAP_END_INDX < file_indx)) {
#if(0)
			memset(tres.name, 0, sizeof(tres.name));
			snprintf(tres.name, RES_NAME_MAX_LENGTH, "cobra%03u_rle8.raw", file_indx+1);
#else
			uint_fast8_t temp1, temp2;
			tres.name[5] = (temp1 = (file_indx+1)/100) + '0';
			tres.name[6] = (temp2 = ((file_indx+1) - (temp1*100))/10) + '0';
			tres.name[7] = ((file_indx+1) - (temp1*100) - (temp2*10)) + '0';
#endif
			tres.SDRAM_adr = fdatap;
			if(FALSE == force_sdmmc) {	// Load from default application storage //
				if(0 != LoadRes(INTRO2_PICs, &tres)) {
					ERRM("%s raw REL8 file load FAILED\n", tres.name);
					infop[file_indx].size = 0;
					while(TODO_ON_ERR);
				}
			} else {	// Load from sd-mmc, used when copying data from sd-mmc to spi-flash // 
				if(0 != sdmmc_LoadRes(INTRO2_PICs, &tres)) {
					ERRM("%s raw REL8 file load FAILED\n", tres.name);
					infop[file_indx].size = 0;
					while(TODO_ON_ERR);
				}
			}
			infop[file_indx].size = tres.size;	// "cmap + rle8" raw data size //
		}
		else {
			infop[file_indx].size = 0;	// "cmap + rle8" raw data size //
		}

		infop[file_indx].fdata = fdatap;
		//DEBUGM("indx[%u] : file_name(%s) : file_size(%u)\n", file_indx, file_name, tres.size);
		fdatap += tres.size;	// increment global address counter // 		
	}
	
	return 0;
}

// Decode the image as given by indx parameter on to the frame-buffer given by fb_indx parameter // 
void Intro2_decode2FB(uint16_t indx, uint8_t fb_indx, uint16_t *cmap16)
{
	uint32_t size = (0 != infop[indx].size) ? (infop[indx].size - INTRO2_CMAP_SIZE):(0);
    uint16_t *fb = (uint16_t *)((uint16_t *)LCD_VRAM_BASE_ADDR +
        ((uint32_t)XSIZE_PHYS * (uint32_t)YSIZE_PHYS * (uint32_t)fb_indx));
    uint8_t *rle8 = ((uint8_t *)(infop[indx].fdata)) + INTRO2_CMAP_SIZE;
    uint32_t spix, apix;
    uint8_t samecnt, new_data;

    // Fill cmap16 with new palette data //
    memcpy(cmap16, infop[indx].fdata, INTRO2_CMAP_SIZE);

    // Decode data uncompressed 8bit/pixel raw image data //
    for(spix=0, apix=0; spix<size ;spix++) {
        new_data = rle8[spix];
        if((CMAP_COLOR_COUNT-1) == new_data) {	// special byte received, it is repeated pixel data //
            samecnt = rle8[++spix];
            new_data = rle8[++spix];
            memset(raw8p + apix, new_data, samecnt);
            apix += samecnt;
        } else {
            raw8p[apix++] = new_data;	// NOT REPEATED Bytes will be directly coded //
        }
    }
#if(0)
    // Get real 16bit/pixel data by usage of color map //
    for(apix=0; apix<((uint32_t)XSIZE_PHYS * (uint32_t)YSIZE_PHYS) ;apix++) {
        //uint8_t *src = (uint8_t *)&cmap16[raw8p[apix]];
        //uint8_t *dest = (uint8_t *)&fb[apix];
        //dest[0] = src[0];
         //dest[1] = src[1];
        fb[apix] = cmap16[raw8p[apix]];
    }
#else
    for(apix = ((uint32_t)XSIZE_PHYS * (uint32_t)YSIZE_PHYS)-1 ;; apix--) {
        fb[apix] = cmap16[raw8p[apix]];
        if(0 == apix) break;
    }
#endif

    LCD_X_SHOWBUFFER_INFO x = {.Index = fb_indx};
    LCD_X_DisplayDriver(0, LCD_X_SHOWBUFFER, &x);

}

void Intro2(void) 
{
	volatile uint_fast16_t findx;
	//int32_t length;
	int32_t length_next;
    uint16_t *cmap_u16;
	
	if(0 != InitGroupRes(AUDIO_TRACs, 0xFF))	
		while(TODO_ON_ERR);
	if(NULL == (cmap_u16 = (uint16_t *)calloc(sizeof(uint16_t), CMAP_COLOR_COUNT))) {
        while(TODO_ON_ERR);
        return;
	}

	Intro2_LoadPics2RAM(FALSE);	// Dont FORCE to load from sd-mmc, default app storage will be used //

#if(TRUE == REPEAT_INTRO_ON_KEY_OK)
intro_start:
#endif
	
	for(length_next = GUI_X_GetTime() + INTRO_FPS_MS, findx=0 ; \
		findx<(INTRO_PICS_INDX_MAX+1) ; findx++, length_next += INTRO_FPS_MS) {
		if(0 == infop[findx].size)
		    continue;
	    Intro2_decode2FB(findx, 0, cmap_u16);
		if(0 == findx)
			start_dac_audio(INTRO_SOUND, FALSE);	// play audio but DONT WAIT to the end // 
		while(GUI_X_GetTime() <= length_next);
		#if(TRUE == KEY_PRESS_INTRO_SKIP)
			if(NO_KEY_PRESSED != get_kp()) {	// check if any key pressed // 
				stop_dac_audio();
				GLCD_Clear(0, Black);GLCD_Clear(1, Black);GLCD_Clear(2, Black);GLCD_Clear(3, Black);
				break;
			}
		#endif
	}
	while(AUDIO_TASK_IDLE != audio_get_state());	// wait until audio play completes // 
	#if(TRUE == REPEAT_INTRO_ON_KEY_OK)
		while(((0x1<<10)|(0x1F<<17)) == (GPIO_ReadValue(0) & ((0x1<<10)|(0x1F<<17))));
		if( !(GPIO_ReadValue(0)&(1<<21)))    // KeyOK pressed repeat intro2 //
			goto intro_start;
		else {}    // Other KEY(s) pressed, exit from intro2 //
	#endif

	free(cmap_u16);
}

