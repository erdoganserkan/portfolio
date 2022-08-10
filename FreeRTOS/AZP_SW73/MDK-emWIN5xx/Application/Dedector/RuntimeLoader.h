#ifndef RUNTIME_LOADER_H
#define RUNTIME_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "Intro2.h"
#include "GUI.h"

#define RES_NAME_MAX_LENGTH		(25)
#define SDRAM_STORE_ALINGMENT	(8)	// When storing resourcess into SDRAM jump to next address divisible by this 
																		// number for safety // 
	
#define MASK_NAME_POSTFIX		"Mask"	// Example : ResName "RMGB.png", MaskName = "RMGBMask.png" // 

typedef enum
{
	SB_PICs		= 0,	// Search Screen Related Pictures //
	RM_PICs		= 1,	// Radial Menu Related Pictures //
	OTO_PICs	= 2,	// OTOMATIC Search Screen Related Pictures //
	DPT_PICs	= 3,	// Depth Calc Pics //
	GB_PICs		= 4,	// Groud Balance Related Pictures //
	HB_PICs		= 5,	// Horizontal Battery Pics // 
	MA_PICs		= 6,	// Metal Analysed search //
	SYS_PICs	= 7,	// System Settings Pics //
	FBAR_PICs	= 8,	// Full Bar pics @ settings page //
	NBAR_PICs	= 9,	// Null Bar pics @ settings page // 
	RUNTIME_FONTs	= 10,	// Application fonts for language support // 
	AT_RADAR_RED_PICs	= 11,	// Field Scanner Search V1 RED Anim Pics // 
	AUDIO_TRACs	= 12,	// Audio files // 
	INTRO2_PICs	= 13,	// Intro2 Raw Pictures // 
	AT_MENU_PICs	= 14,	// field scanner menu pics 
	AT_AUTO_FREQ_PICs	= 15,	// Field scanner auto freq screen pics // 
	AT_LOADING_PICs	= 16,	// Field Scanner looaing screen pictures // 
	AT_BRIGHT_VOL_PICs	= 17,	// Volume and LCD Brightness setting // 
	AT_DISTANCE_PICs = 18,   // AT Distance selection screen resourcess //
	AT_MAN_FREQ_PICs   = 19,   // AT MAnuel Frequency Setting Resourcess //
	AT_LANG_PICs    = 20,   // AT LANGUAGE Resourcess //
	DEV_SELECT_PICs = 21,   // Device Select Pictures //
	SYS_LOADING_PICs	= 22,	// SYS Loading Pictures // 
	AT_RADAR_BLUE_PICs	= 23,	// Field Scanner Search V1 BLUE Anim Pics // 
	AZP_LOADING_PICs	= 24, 	// AZ Plus Dedector Loading Screen Pics // 
	AZP_SB_PICs			= 25,
	AZP_GB_PICs			= 26,
	
	APP_PICs_GROUP_COUNT
}AppPicsGproups;

typedef struct
{	
	uint16_t xSize;
	uint16_t ySize;
	uint32_t size;	// size of resources on SDRAM // 
	GUI_MEMDEV_Handle hMemHandle;	// emWin memory device handle for the resource // 
	void *SDRAM_adr;	// SDRAM start address for resource // 
	char name[RES_NAME_MAX_LENGTH];	// name of resource in non-volatile memory filesystem //
	void const *ResMaskPtr;	// If resource is located in the memory-map of mcu, this is address // 
} ResInfoType;

typedef struct
{
	uint16_t IconCount;
} GroupInfoType;

// The sequence must be same with SBResources[][] // 
typedef enum
{
	SB_BATTERY0	= 0,	// Battery Icons Start // 
	SB_BATTERY10,
	SB_BATTERY30,
	SB_BATTERY45,
	SB_BATTERY55,
	SB_BATTERY60,
	SB_BATTERY75,
	SB_BATTERY90,
	SB_BATTERY100,
	SB_BACKGROUD,
	SB_BACKGROUD_TOP,
	SB_BATPOPUP_WARNNG,
	SB_BATPOPUP_EMPTY,
	
	SB_ICONs_COUNT	
} eSB_RES_INDXes;	// Status Bar resource indexes // 

// The sequence must be same with SBResources[][] // 
typedef enum
{
	SB_AZP_SETTINGs = 0,
	SB_AZP_GB,
	SB_AZP_SENS,
	SB_AZP_VOL,
	SB_AZP_BRIGHT,
	SB_AZP_BAT,
	
	SB_AZP_ICONs_COUNT	
} eAZPSB_RES_INDXes;	// Status Bar resource indexes // 

typedef enum
{
	RM_GB = 0,
	RM_STD,
	RM_MIN,
	RM_OTO,
	RM_DPT,
	RM_SYS,
	RM_UPBACK,
	RM_MIDBACK,
	RM_DOWNBACK,
	
	RM_ICONs_COUNT
} eRM_RES_INDXes;	// Radial Manu Resourcess Indexes // 

typedef enum
{
	OTO_CAVITY = 0,	// Reduced Target ID, Full Target ID //  
	OTO_FERROs,
	OTO_NFERROs,
	OTO_GOLD,
	OTO_MINERAL,	// Reduced Target ID, Full Target ID // 
	OTO_GSTRBACK,
	OTO_INFO_BACK,
	OTO_NOFERROs,
	OTO_YESFERROs,
	OTO_GBACK,
	OTO_SCRSTRBACK,
	OTO_METAL,		// Reduced TID // 
	
	OTO_ICONs_COUNT
} eOTO_RES_INDXes;

typedef enum
{
	DPT_BACK = 0,
	DPT_SEL,
	DPT_UNSEL,
	DPT_RULER,
	DPT_COIL,
	DPT_TARGET,
	DPT_REPORT_BACK,
	
	DPT_ICONs_COUNT
} eDPT_RES_INDXes;

typedef enum
{
	GB_BACK = 0,
	GB_HAND,
	GB_HAND50,
	GB_HAND_BACK,
	GB_HAND_SMALL,
	GB_ARMOR_GREEN,
	GB_ARMOR_RED,
	GB_POPUP_OK_BACK,
	GB_POPUP_FAIL_BACK,
	GB_COIL,
	GB_COIL50,
	GB_COIL_BACK,
	GB_HOLDER,
	GB_HOLDER50,
	GB_POINTER,	
	
	GB_ICONs_COUNT,
} eGB_RES_INDXes;

typedef enum
{
	HB_CHRG = 0,
	HB_WARN,
	HB_0,
	HB_20,
	HB_40,
	HB_60,
	HB_80,
	HB_100,

	HB_ICONs_COUNT,
} eHB_RES_INDXes;

typedef enum
{
	MA_BACK = 0,
	MA_FERROOK,
	MA_NOFERRO,
	MA_LRBACK,
	MA_GBACK,

	MA_ICONs_COUNT,
} eMA_RES_INDXes;

typedef enum
{
	SYS_BACK = 0,
	SYS_SELECT,
	SYS_VOLUME,
	SYS_LANG,
	SYS_BRIGHT,
	SYS_FERROs,
	SYS_SENS,
	SYS_FACTORY,
	SYS_LANG_TURKISH,
	SYS_LANG_ENGLISH,
	SYS_LANG_ARABIC,
	SYS_LANG_GERMAN,
	SYS_LANG_SPAIN,
	SYS_LANG_PERSIAN,
	SYS_LANG_RUSSIAN,
	SYS_LANG_FRENCH,
	SYS_BUTTON,
	SYS_NOFERRO,
	SYS_FERROOK,
	
	SYS_ICONs_COUNT
} eSYS_RES_INDXes;

typedef enum
{
	FBAR10,
	FBAR15,
	FBAR20,
	FBAR25,
	FBAR30,
	FBAR35,
	FBAR40,
	FBAR45,
	FBAR50,
	FBAR55,
	FBAR60,
	FBAR65,
	FBAR70,
	FBAR75,
	FBAR80,
	FBAR85,
	FBAR90,
	FBAR95,
	FBAR100,
	
	FBAR_ICONs_COUNT
} eFBAR_INDXes;

typedef enum
{
	NBAR10,
	NBAR15,
	NBAR20,
	NBAR25,
	NBAR30,
	NBAR35,
	NBAR40,
	NBAR45,
	NBAR50,
	NBAR55,
	NBAR60,
	NBAR65,
	NBAR70,
	NBAR75,
	NBAR80,
	NBAR85,
	NBAR90,
	NBAR95,
	NBAR100,
	
	NBAR_ICONs_COUNT
} eNBAR_INDXes;

typedef enum
{
	ARIAL_32B_INDX	= 0,
	ARIAL_24B_INDX,
	ARIAL_19B_INDX,
	ARIAL_16B_INDX,
	
	RFONT_ITEM_COUNT
} eRUNTIME_FONT_INDXes;

typedef enum
{
	AT_RADAR_BCKGD_INDX	 = 0,
	AT_RADAR_RADAR1,
	AT_RADAR_PICs_MIN = AT_RADAR_RADAR1,
	AT_RADAR_RADAR2,
	AT_RADAR_RADAR3,
	AT_RADAR_RADAR4,
	AT_RADAR_RADAR5,
	AT_RADAR_RADAR6,
	AT_RADAR_RADAR7,
	AT_RADAR_RADAR8,
	AT_RADAR_RADAR9,
	AT_RADAR_RADAR10,
	AT_RADAR_RADAR11,
	AT_RADAR_RADAR12,
	AT_RADAR_RADAR13,
	AT_RADAR_RADAR14,
	AT_RADAR_RADAR15,
	AT_RADAR_RADAR16,
	AT_RADAR_RADAR17,
	AT_RADAR_PICs_MAX = AT_RADAR_RADAR17,
	
	AT_RADAR_ICONs_COUNT
} eAT_RADAR_INDXes;

typedef enum {
	ACCESSDEN_SOUND 	= 0,	// error // 
	BUTTON_ERR_SOUND	= ACCESSDEN_SOUND,
	OLD_BUTTON_OK		= 1,
	BUTTON5_SOUND		= 2,
	DEB_SOUND			= 3,
	FINS_SOUND			= 4,
	KICKHAT_P1_SOUND	= 5,
	KICKHAT_P2_SOUND	= 6,
	MENU_CLICK_SOUND	= 7, // button ok // 
	BUTTON_OK_SOUND		= MENU_CLICK_SOUND,
	MENU_SEL_SOUND		= 8,
	MS200B_SOUND		= 9,
	MULTIMAX_SOUND		= 10,
	OLD_SAMPLE_SOUND	= 11,
	PLUNGER_SOUND		= 12,	
	SATREBOR_SOUND		= 13,
	SAW_SOUND			= 14,
	WATERDROP_SOUND		= 15,	// sample sound // 	
	SAMPLE_SOUND		= WATERDROP_SOUND,
	INTRO_SOUND,
	
	AUDIO_TRACKs_COUNT
} eAUDIO_TRACs;

typedef enum {
	DUMMY = 0,
	INTRO2_PICs_COUNT	= INTRO_PICS_INDX_MAX
} eINTRO2_PICs;

typedef enum {
	AT_MENU_BACK	= 0,
	AT_NULL_BACK,
	AT_MENU_POS1_SELECT,
	AT_MENU_POS1_UNSELECT,
	AT_MENU_POS2_SELECT,
	AT_MENU_POS2_UNSELECT,
	AT_MENU_POS3_SELECT,
	AT_MENU_POS3_UNSELECT,
	AT_MENU_POS4_SELECT,
	AT_MENU_POS4_UNSELECT,
	AT_MENU_POS5_SELECT,
	AT_MENU_POS5_UNSELECT,
	AT_MENU_POS6_SELECT,
	AT_MENU_POS6_UNSELECT,
	AT_MENU_STR_BACK,

	AT_MENU_PICs_COUNT
} eAT_MENU_PICs;

typedef enum {
	AT_AUTO_FREQ_ICON = 0,
	AT_AUTO_FREQ_BLACK_PART,
	AT_AUTO_FREQ_RED_PART,
	AT_AUTO_FREQ_TYPE1,
	AT_AUTO_FREQ_TYPE2,
	AT_AUTO_FREQ_TYPE3,
	AT_AUTO_FREQ_TYPE4,
	AT_AUTO_FREQ_TYPE5,

	AT_AUTO_FREQ_PICs_COUNT
} eAT_AUTO_FREQ_PICs;

typedef enum {
	AT_LOADING_AUTOF_TYPE1 = 0,
	AT_LOADING_AUTOF_TYPE2,
	AT_LOADING_AUTOF_TYPE3,
	AT_LOADING_AUTOF_TYPE4,
	AT_LOADING_AUTOF_TYPE5,
	AT_LOADING_MANFREQ,
	AT_LOADING_BAR1,
	AT_LOADING_BAR2,
	AT_LOADING_BAR3,
	AT_LOADING_BAR4,
	AT_LOADING_BAR5,	

	AT_LOADING_PICs_COUNT
} eAT_LOADING_PICs;

typedef enum  {
	AT_BRIGHT_ICON = 0,
	AT_VOL_ICON,
	AT_SET_FULL,
	AT_SET_NULL,

	AT_BRIGHT_VOL_PICs_COUNT
} eAT_BRIGHT_VOL_PICs;

typedef enum  {
    AT_DISTANCE_ICON = 0,
	AT_DISTANCE_BLACK_PART,
	AT_DISTANCE_RED_PART,

    AT_DISTANCE_PICs_COUNT
} eAT_DISTANCE_PICs;

typedef enum  {
    AT_MAN_FREQ_ICON = 0,
	AT_MAN_FREQ_DIGIT_WIN_BACK,

    AT_MAN_FREQ_PICs_COUNT
} eAT_MAN_FREQ_PICs;

typedef enum  {
    AT_LANG_ICON = 0,
    AT_LANGs_WIN_BACK,
    AT_LANG_PTR,

    AT_LANG_PICs_COUNT
} eAT_LANG_PICs;

typedef enum  {
    DEV_SELECT_BACK = 0,
    DEV_SELECT_WITH_DETECTOR,
    DEV_SELECT_WITH_DETECTOR_and_ARROW,
    DEV_SELECT_WITHOUT_DETECTOR,
    DEV_SELECT_WITH_FS,
    DEV_SELECT_WITH_FS_and_ARROW,
    DEV_SELECT_WITHOUT_FS,

    DEV_SELECT_PICs_COUNT
} eDEV_SELECT_PICs;

typedef enum  {
    SYS_LOADING_BACK = 0,
	SYS_LOAD_MIDDLE_WIN_BACK_BLUE,
	SYS_LOAD_MIDDLE_WIN_BACK_RED,
	SYS_LOADING_ICON1,
	SYS_LOADING_ICON2,
	SYS_LOADING_ICON3,
	SYS_LOADING_ICON4,
	SYS_LOADING_ICON5,
	SYS_LOADING_ICON6,
	SYS_LOADING_ICON7,
	SYS_LOADING_ICON8,

    SYS_LOADING_PICs_COUNT
} eSYS_LOADING_PICs;

typedef enum  {
    AZP_COMPANY_LOGO = 0,
    AZP_DEVICE_LOGO,

    AZP_LOADING_PICs_COUNT
} eAZP_LOADING_PICs;

typedef enum  {
    AZP_GB_COIL = 0,

    AZP_GB_PICs_COUNT
} eAZP_GB_PICs;


// Shared Objects //
extern const char *ResGroupDirs[APP_PICs_GROUP_COUNT];
extern const GroupInfoType GroupInfos[APP_PICs_GROUP_COUNT];
extern ResInfoType *SBResources[APP_PICs_GROUP_COUNT];
extern GUI_FONT GUI_FontArray[RFONT_ITEM_COUNT];
extern uint8_t LoadRes(uint8_t ResGroup, ResInfoType *ResInfoPtr);

// Shared Functions // 
extern uint8_t InitGroupRes(uint8_t GroupIndx, uint8_t findx);
extern void DeInitGroupRes(uint8_t GroupIndx);
extern uint8_t ResLoadInit(void);	
extern char *getMaskedRes(char *res_name);

#endif
