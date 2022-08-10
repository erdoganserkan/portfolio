#ifndef INTRO2_H
#define INTRO2_H

#include <stdio.h>
#include <stdint.h>
#include "AppCommon.h"

#define CMAP_COLOR_COUNT	(256)
#define INTRO_LENGTH_S	(18)
#define INTRO_FPS				(20)
#define INTRO_FPS_MS		(1000/INTRO_FPS)

#if(INTRO_FPS == 5)
	#define INTRO_PICS_GAP_START_INDX	(69)	// First One Missing // 
	#define INTRO_PICS_GAP_END_INDX	(90)		// Last One Missing // 
	#define INTRO_PICS_INDX_MAX	(92)
	#define INTRO_PICS_INDX_COUNT	(INTRO_PICS_INDX_MAX - (INTRO_PICS_GAP_END_INDX - INTRO_PICS_GAP_START_INDX + 1))
#elif(INTRO_FPS == 10)
	#define INTRO_PICS_GAP_START_INDX	(133)	// First One Missing // 
	#define INTRO_PICS_GAP_END_INDX	(177)		// Last One Missing // 
	#define INTRO_PICS_INDX_MAX	(183)
	#define INTRO_PICS_INDX_COUNT	(INTRO_PICS_INDX_MAX - (INTRO_PICS_GAP_END_INDX - INTRO_PICS_GAP_START_INDX + 1))
#elif(INTRO_FPS == 15)
	#define INTRO_PICS_GAP_START_INDX	(197)	// First One Missing // 
	#define INTRO_PICS_GAP_END_INDX	(265)		// Last One Missing // 
	#define INTRO_PICS_INDX_MAX	(273)
	#define INTRO_PICS_INDX_COUNT	(INTRO_PICS_INDX_MAX - (INTRO_PICS_GAP_END_INDX - INTRO_PICS_GAP_START_INDX + 1))
#elif(INTRO_FPS == 20)
	#define INTRO_PICS_GAP_START_INDX	(267)	// First One Missing // 
	#define INTRO_PICS_GAP_END_INDX	(357)		// Last One Missing // 
	#define INTRO_PICS_INDX_MAX	(366)
	#define INTRO_PICS_INDX_COUNT	(INTRO_PICS_INDX_MAX - (INTRO_PICS_GAP_END_INDX - INTRO_PICS_GAP_START_INDX + 1))
#else
	#error "UNEXPECTED INTRO FPS"
#endif

#if((SPI_FLASH_SIZE < (32U*1024U*1024U)) && (RES_LOAD_SPI_FLASH == RES_LOAD_SOURCE))	// SpiFlash size is not enough to store all pictures // 
	#undef INTRO_PICS_GAP_START_INDX
	#define INTRO_PICS_GAP_START_INDX	(80)	// First One Missing // 
	#undef INTRO_PICS_GAP_END_INDX
	#define INTRO_PICS_GAP_END_INDX	(90)		// Last One Missing // 
	#undef INTRO_PICS_INDX_MAX
	#define INTRO_PICS_INDX_MAX	(100)
	#undef INTRO_PICS_INDX_COUNT
	#define INTRO_PICS_INDX_COUNT	(INTRO_PICS_INDX_MAX - (INTRO_PICS_GAP_END_INDX - INTRO_PICS_GAP_START_INDX + 1))
#endif

typedef enum {
    // Loading Animations Types //
    LOADING_BEFORE_INTRO = 0,
    LOADING_INTRO_PHASE,
    LOADING_AFTER_INTRO,

    LOADING_INTRO_STAGE_COUNT
} eLOADING_STAGEs;



#pragma pack(1)
typedef struct {
	uint32_t size;	// Size of RLE8 encoded raw picture data // 
	uint8_t *fdata;	// Address of Raw Picture on SDRAM // 
} sRLE8Pic_Header;
#pragma pack()

extern void raw_intro(void);
extern void Intro2(void);
extern int Intro2_LoadPics2RAM(uint8_t force_sdmmc);

#endif
