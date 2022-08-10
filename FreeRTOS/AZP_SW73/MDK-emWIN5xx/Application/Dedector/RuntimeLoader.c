#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "debug_frmwrk.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "SDRAM_K4S561632C_32M_16BIT.h"
#include "Serial.h"
#include "monitor.h"
#include "GuiConsole.h"
#include "AppSettings.h"
#include "FlashDriver.h"
#include "StatusBar.h"
#include "ATBrightVol.h"
#include "ATDistance.h"
#include "ATManuelFreq.h"
#include "ATLang.h"
#include "ATMenu.h"
#include "DevSelect.h"
#include "SysLoading.h"
#include "RadialMenu.h"
#include "OTOSearch.h"
#include "RuntimeLoader.h"
#include "AZPMenu.h"		
#include "AZPLoading.h"

// Shared Resources // 
GUI_FONT GUI_FontArray[RFONT_ITEM_COUNT];
const char *ResGroupDirs[APP_PICs_GROUP_COUNT] = {	/* Resource including directory names */
	"SB",			// 0
	"RM",			// 1
	"OTO",		// 2
	"DPT",		// 3
	"GB",			// 4
	"HB",			// 5
	"MAMINSTD",			// 6
	"SYS",		// 7
	"FBAR",		// 8
	"NBAR",		// 9
	"FONTs",	// 10
	"ATRADAR",		// 11
	"AUDIO",			// 12
	"INTRO2",			// 13
	"ATMENU",			// 14
	"ATAUTOFREQ",	// 15
	"ATLOADING",	// 16
	"ATBRIGHTVOL",	// 17
	"ATDISTANCE",		// 18
	"ATMANFREQ",		// 19
	"ATLANG",				// 20
	"DEVSELECT",	// 21
	"SYSLOADING",	// 22
	"ATRADAR",		// 23
	"AZPLOADING",	// 24
	"AZPSB",		// 25
	"AZPGB"			// 26
};

const GroupInfoType GroupInfos[APP_PICs_GROUP_COUNT] = {
	{SB_ICONs_COUNT},	// Status Bar //	
	{RM_ICONs_COUNT},	// Radial Menu // 
	{OTO_ICONs_COUNT},	// oto
	{DPT_ICONs_COUNT},	// dpt
	{GB_ICONs_COUNT},	// GB
	{HB_ICONs_COUNT},	// HB
	{MA_ICONs_COUNT},	// MA 
	{SYS_ICONs_COUNT},	// SYS 
	{FBAR_ICONs_COUNT},	// FBAR 
	{NBAR_ICONs_COUNT},	// NBAR
	{RFONT_ITEM_COUNT},	// RUNTIME FONTs
	{AT_RADAR_ICONs_COUNT},	// FS RADAR RED // 
	{AUDIO_TRACKs_COUNT},	// APPLICATION SOUNDs /
	{INTRO_PICS_INDX_MAX+1}, // INTRO PICs //
	{AT_MENU_PICs_COUNT},	// AT Menu items // 
	{AT_AUTO_FREQ_PICs_COUNT},	// AT AUTO FREQUENCY // 
	{AT_LOADING_PICs_COUNT},	// AT LOADING // 
	{AT_BRIGHT_VOL_PICs},		// AT BRIGHTNESS, AT VOLUME //
	{AT_DISTANCE_PICs_COUNT},    // AT DISTANCE //
	{AT_MAN_FREQ_PICs_COUNT},   // AT MANUEL FREQUENCY //	
	{AT_LANG_PICs_COUNT},   // AT LANGUAGE //
  	{DEV_SELECT_PICs_COUNT},    // DEVICE TYPE SELECT //
	{SYS_LOADING_PICs_COUNT},		// SYS LOADING // 
	{AT_RADAR_ICONs_COUNT}, 	// FS RADAR BLUE // 
	{AZP_LOADING_PICs_COUNT},	// AZP LOADYNG // 
	{SB_AZP_ICONs_COUNT},		// AZP Status Bar // 
	{AZP_GB_PICs_COUNT}		// AZP GB // 
};

ResInfoType SBRes[SB_ICONs_COUNT] = {	// Status Bar Pictures // 
	{	32, 32, 0, 0, NULL, "Bat_0.png" },
	{	32, 32, 0, 0, NULL,	"Bat_10.png" },
	{	32, 32, 0, 0, NULL,	"Bat_30.png" },
	{	32, 32, 0, 0, NULL,	"Bat_45.png" },
	{	32, 32, 0, 0, NULL,	"Bat_55.png" },
	{	32, 32, 0, 0, NULL, "Bat_60.png" },
	{	32, 32, 0, 0, NULL,	"Bat_75.png" },
	{	32, 32, 0, 0, NULL,	"Bat_90.png" },
	{	32, 32, 0, 0, NULL,	"Bat_100.png" }, 
	{	115, GLCD_Y_SIZE, 0, 0, NULL,	"SBBackground.png" },
	{	GLCD_X_SIZE, 100, 0, 0, NULL,	"SBBackTop.png" },
	{	143, 188, 0, 0, NULL,	"BatWARN.png" },
	{	143, 188, 0, 0, NULL,	"BatPOFF.png" }
};

ResInfoType RMRes[RM_ICONs_COUNT] = {	// Radial Menu Pictures : Sequence MUST BE same with "eRMPageType" // 
	{	RM_ICON_SIZE_X, RM_ICON_SIZE_Y, 0, 0, NULL,	"RMGB.png", "RMMask.bmp"},
	{	RM_ICON_SIZE_X, RM_ICON_SIZE_Y, 0, 0, NULL,	"RMSTD.png", "RMMask.bmp"},
	{ RM_ICON_SIZE_X, RM_ICON_SIZE_Y, 0, 0, NULL,	"RMMIN.png", "RMMask.bmp"},
	{ RM_ICON_SIZE_X, RM_ICON_SIZE_Y, 0, 0, NULL, "RMOTO.png", "RMMask.bmp"},
	{	RM_ICON_SIZE_X, RM_ICON_SIZE_Y, 0, 0, NULL,	"RMDPT.png", "RMMask.bmp"},
	{ RM_ICON_SIZE_X, RM_ICON_SIZE_Y, 0, 0, NULL, "RMSYS.png", "RMMask.bmp"},
	{ GLCD_X_SIZE, 60, 0, 0, NULL, "RMUpBack.png"},
	{ GLCD_X_SIZE, 182, 0, 0, NULL, "RMMidBack.png"},
	{ GLCD_X_SIZE, 30, 0, 0, NULL, "RMDownBack.png"},
};

ResInfoType OTORes[OTO_ICONs_COUNT] = {		// OTOMATIC Search Pictures // 
	{	TARGET_PICs_SIZEX, TARGET_PICs_SIZEX, 0, 0, NULL, "cavity.png" },
	{	TARGET_PICs_SIZEX, TARGET_PICs_SIZEX, 0, 0, NULL, "ferros.png" },
	{	TARGET_PICs_SIZEX, TARGET_PICs_SIZEX, 0, 0, NULL, "nferros.png" },
	{	TARGET_PICs_SIZEX, TARGET_PICs_SIZEX, 0, 0, NULL, "gold.png" },
	{	TARGET_PICs_SIZEX, TARGET_PICs_SIZEX, 0, 0, NULL, "mineral.png" },
	{	375, 53, 0, 0, NULL, "GStrBack.png" },	// Gauge string 
	{	69, 159, 0, 0, NULL, "InfoBack.png" },	// Info string 
	{	51, 64, 0, 0, NULL, "NOFerros.png" },	
	{	51, 64, 0, 0, NULL, "YESFerros.png" },
	{	300, 159, 0, 0, NULL, "OTOGBack.png" },	// Gauge Background 
	{	373, 45, 0, 0, NULL, "ScrStrBack.png" },	// Screen Name string background 
	{	TARGET_PICs_SIZEX, TARGET_PICs_SIZEX, 0, 0, NULL, "metal.png" },
};

ResInfoType DPTRes[DPT_ICONs_COUNT]	= { // DEPTH PICs //
	{	GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "DptBack.png" },
	{	88, 186, 0, 0, NULL, "Sel.png" },
	{	88, 186, 0, 0, NULL, "Unsel.png" },
	{	35, 151, 0, 0, NULL, "DptRuler.png" },
	{	137, 196, 0, 0, NULL, "DptCoil.png" },
	{	116, 57, 0, 0, NULL, "DptTarget.png" },
	{	161, 256, 0, 0, NULL, "DptReport.png" }
};

ResInfoType GBRes[GB_ICONs_COUNT] = {	// GB PICs, same sequence with eGB_RES_INDXes //
	{	GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "GBBack.png" },
	{	92, 119, 0, 0, NULL, "Hand.png" },
	{	92, 119, 0, 0, NULL, "Hand50.png" },
	{	92, 119, 0, 0, NULL, "HandBack.png" },
	{	80, 99, 0, 0, NULL, "HandSmall.png" },
	{	80, 99, 0, 0, NULL, "GBArmorGR.png" },
	{	80, 99, 0, 0, NULL, "GBArmorRD.png" },
	{	408, 231, 0, 0, NULL, "GBPopupOK.png" },
	{	408, 231, 0, 0, NULL, "GBPopupFail.png" },
	{	128, 85, 0, 0, NULL, "Coil.png" },
	{	132, 150, 0, 0, NULL, "Coil50.png" },
	{	132, 150, 0, 0, NULL, "CoilBack.png" },
	{	224, 52, 0, 0, NULL, "Holder.png" },
	{	224, 52, 0, 0, NULL, "Holder50.png" },
	{ 45, 38, 0, 0, NULL, "Pointer.png" }
};

ResInfoType HBRes[HB_ICONs_COUNT] = {		// HB PICs, same sequence with eHB_RES_INDXes //
	{	47, 36, 0, 0, NULL, "HB_Chrg.png" },
	{	47, 36, 0, 0, NULL, "HB_Warn.png" },
	{	47, 36, 0, 0, NULL, "HBat0.png" },
	{	47, 36, 0, 0, NULL, "HBat20.png" },
	{	47, 36, 0, 0, NULL, "HBat40.png" },
	{	47, 36, 0, 0, NULL, "HBat60.png" },
	{	47, 36, 0, 0, NULL, "HBat80.png" },
	{	47, 36, 0, 0, NULL, "HBat100.png" }
};

ResInfoType MARes[MA_ICONs_COUNT] = {		// MA PICs, same sequence with eMA_RES_INDXes //
	{	GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "MABack.png" },
	{	65, 65, 0, 0, NULL, "MAFerroOK.png"},
	{	65, 65, 0, 0, NULL, "MANOFerro.png"}, 
	{	52, 52, 0, 0, NULL, "MALRBack.png"},
	{	374, 86, 0, 0, NULL, "MAGBack.png"}
};

ResInfoType SYSRes[SYS_ICONs_COUNT] = {		// SYS PICs, same sequence with eSYS_RES_INDXes //
	{ GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "SYSBack.png"},
	{ 98, 98, 0, 0, NULL, "SYSSelect.png"},
	{ 69, 79, 0, 0, NULL, "SYSVolume.png", "SYSIcon2Mask.bmp"},
	{ 69, 79, 0, 0, NULL, "SYSLang.png", "SYSIcon2Mask.bmp"},
	{ 69, 79, 0, 0, NULL, "SYSBright.png", "SYSIcon2Mask.bmp"},
	{ 69, 79, 0, 0, NULL, "SYSFerros.png", "SYSIcon2Mask.bmp"},
	{ 69, 79, 0, 0, NULL, "SYSSens.png", "SYSIcon2Mask.bmp"},
	{ 69, 79, 0, 0, NULL, "SYSFactory.png", "SYSIcon2Mask.bmp"},
	{ 74, 55, 0, 0, NULL, "SYSTurkish.png"},
	{ 74, 55, 0, 0, NULL, "SYSEnglish.png"},
	{ 74, 55, 0, 0, NULL, "SYSArabic.png"},
	{ 74, 55, 0, 0, NULL, "SYSgerman.png"},
	{ 74, 55, 0, 0, NULL, "SYSSpain.png"},
	{ 74, 55, 0, 0, NULL, "SYSPersian.png"},
	{ 74, 55, 0, 0, NULL, "SYSrussian.png"},
	{ 74, 55, 0, 0, NULL, "SYSFrench.png"},
	{ 112, 44, 0, 0, NULL, "SYSButton.png"},
	{ 100, 100, 0, 0, NULL, "NOFerro.png", "SYSIconMask.bmp"},
	{ 100, 100, 0, 0, NULL, "FerroOK.png", "SYSIconMask.bmp"}
};

ResInfoType FBARRes[FBAR_ICONs_COUNT] = {		// FBAR PICs, same sequence with eFBAR_RES_INDXes //
	{ 15, 150, 0, 0, NULL, "BarF10.png"},
	{ 15, 150, 0, 0, NULL, "BarF15.png"},
	{ 15, 150, 0, 0, NULL, "BarF20.png"},
	{ 15, 150, 0, 0, NULL, "BarF25.png"},
	{ 15, 150, 0, 0, NULL, "BarF30.png"},
	{ 15, 150, 0, 0, NULL, "BarF35.png"},
	{ 15, 150, 0, 0, NULL, "BarF40.png"},
	{ 15, 150, 0, 0, NULL, "BarF45.png"},
	{ 15, 150, 0, 0, NULL, "BarF50.png"},
	{ 15, 150, 0, 0, NULL, "BarF55.png"},
	{ 15, 150, 0, 0, NULL, "BarF60.png"},
	{ 15, 150, 0, 0, NULL, "BarF65.png"},
	{ 15, 150, 0, 0, NULL, "BarF70.png"},
	{ 15, 150, 0, 0, NULL, "BarF75.png"},
	{ 15, 150, 0, 0, NULL, "BarF80.png"},
	{ 15, 150, 0, 0, NULL, "BarF85.png"},
	{ 15, 150, 0, 0, NULL, "BarF90.png"},
	{ 15, 150, 0, 0, NULL, "BarF95.png"},
	{ 15, 150, 0, 0, NULL, "BarF100.png"}
};

ResInfoType NBARRes[NBAR_ICONs_COUNT] = {		// NBAR PICs, same sequence with eFBAR_RES_INDXes //
	{ 15, 150, 0, 0, NULL, "BarN10.png"},
	{ 15, 150, 0, 0, NULL, "BarN15.png"},
	{ 15, 150, 0, 0, NULL, "BarN20.png"},
	{ 15, 150, 0, 0, NULL, "BarN25.png"},
	{ 15, 150, 0, 0, NULL, "BarN30.png"},
	{ 15, 150, 0, 0, NULL, "BarN35.png"},
	{ 15, 150, 0, 0, NULL, "BarN40.png"},
	{ 15, 150, 0, 0, NULL, "BarN45.png"},
	{ 15, 150, 0, 0, NULL, "BarN50.png"},
	{ 15, 150, 0, 0, NULL, "BarN55.png"},
	{ 15, 150, 0, 0, NULL, "BarN60.png"},
	{ 15, 150, 0, 0, NULL, "BarN65.png"},
	{ 15, 150, 0, 0, NULL, "BarN70.png"},
	{ 15, 150, 0, 0, NULL, "BarN75.png"},
	{ 15, 150, 0, 0, NULL, "BarN80.png"},
	{ 15, 150, 0, 0, NULL, "BarN85.png"},
	{ 15, 150, 0, 0, NULL, "BarN90.png"},
	{ 15, 150, 0, 0, NULL, "BarN95.png"},
	{ 15, 150, 0, 0, NULL, "BarN100.png"}
};

ResInfoType RFONTs[RFONT_ITEM_COUNT] = {		// Application Runtime Initialized Fonts //
	{ 0, 0, 0, 0, NULL, "Arial32Bstd.sif"},
	{ 0, 0, 0, 0, NULL, "Arial24Bstd.sif"},
	{ 0, 0, 0, 0, NULL, "Arial19Bstd.sif"},
	{ 0, 0, 0, 0, NULL, "Arial16Bstd.sif"}
};

ResInfoType FSResRED[AT_RADAR_ICONs_COUNT] = {		// Filed Scanning page pictures //
	{ GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "FSBckgndr.png"},
	{ 186, 176, 0, 0, NULL, "radar_1r.png"},
	{ 186, 176, 0, 0, NULL, "radar_2r.png"},
	{ 186, 176, 0, 0, NULL, "radar_3r.png"},
	{ 186, 176, 0, 0, NULL, "radar_4r.png"},
	{ 186, 176, 0, 0, NULL, "radar_5r.png"},
	{ 186, 176, 0, 0, NULL, "radar_6r.png"},
	{ 186, 176, 0, 0, NULL, "radar_7r.png"},
	{ 186, 176, 0, 0, NULL, "radar_8r.png"},
	{ 186, 176, 0, 0, NULL, "radar_9r.png"},
	{ 186, 176, 0, 0, NULL, "radar_10r.png"},
	{ 186, 176, 0, 0, NULL, "radar_11r.png"},
	{ 186, 176, 0, 0, NULL, "radar_12r.png"},
	{ 186, 176, 0, 0, NULL, "radar_13r.png"},
	{ 186, 176, 0, 0, NULL, "radar_14r.png"},
	{ 186, 176, 0, 0, NULL, "radar_15r.png"},
	{ 186, 176, 0, 0, NULL, "radar_16r.png"},
	{ 186, 176, 0, 0, NULL, "radar_17r.png"}
};

ResInfoType AUDIOs[AUDIO_TRACKs_COUNT] = {		// Application Runtime Initialized Fonts //
	{ 0, 0, 0, 0, NULL, "accessden.bin"},
	{ 0, 0, 0, 0, NULL, "buttonok.bin"},
	{ 0, 0, 0, 0, NULL, "button5.bin"},
	{ 0, 0, 0, 0, NULL, "deb.bin"},
	{ 0, 0, 0, 0, NULL, "fins.bin"},
	{ 0, 0, 0, 0, NULL, "kickhatp1.bin"},
	{ 0, 0, 0, 0, NULL, "kickhatp2.bin"},
	{ 0, 0, 0, 0, NULL, "menuclick.bin"},
	{ 0, 0, 0, 0, NULL, "menusel.bin"},
	{ 0, 0, 0, 0, NULL, "ms2000b.bin"},
	{ 0, 0, 0, 0, NULL, "multimax.bin"},
	{ 0, 0, 0, 0, NULL, "oldsample.bin"},
	{ 0, 0, 0, 0, NULL, "plunger.bin"},
	{ 0, 0, 0, 0, NULL, "satrebor.bin"},
	{ 0, 0, 0, 0, NULL, "saw.bin"},
	{ 0, 0, 0, 0, NULL, "waterdrop.bin"},
	{ 0, 0, 0, 0, NULL, "intro.bin"}
};

ResInfoType ATMenuRes[AT_MENU_PICs_COUNT] = {		// Filed Scanning page pictures //
	{ GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "ATMenuBack.png"},
	{ GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "ATNullBack.png"},
	{ ICON1_SIZE_X, ICON1_SIZE_Y, 0, 0, NULL, "ManualFreqFull.png"},
	{ ICON1_SIZE_X, ICON1_SIZE_Y, 0, 0, NULL, "ManualFreqNull.png"},
	{ ICON2_SIZE_X, ICON2_SIZE_Y, 0, 0, NULL, "AutoFreqFull.png"},
	{ ICON2_SIZE_X, ICON2_SIZE_Y, 0, 0, NULL, "AutoFreqNull.png"},
	{ ICON3_SIZE_X, ICON3_SIZE_Y, 0, 0, NULL, "DistanceFull.png"},
	{ ICON3_SIZE_X, ICON3_SIZE_Y, 0, 0, NULL, "DistanceNull.png"},
	{ ICON4_SIZE_X, ICON4_SIZE_Y, 0, 0, NULL, "VolFull.png"},
	{ ICON4_SIZE_X, ICON4_SIZE_Y, 0, 0, NULL, "VolNull.png"},
	{ ICON5_SIZE_X, ICON5_SIZE_Y, 0, 0, NULL, "BrightFull.png"},
	{ ICON5_SIZE_X, ICON5_SIZE_Y, 0, 0, NULL, "BrightNull.png"},
	{ ICON6_SIZE_X, ICON6_SIZE_Y, 0, 0, NULL, "LangFull.png"},
	{ ICON6_SIZE_X, ICON6_SIZE_Y, 0, 0, NULL, "LangNull.png"},
	{ ACTIVE_ICON_STR_SIZE_X, ACTIVE_ICON_STR_SIZE_Y, 0, 0, NULL, "ATMStrBack2.png"}
};

ResInfoType ATAutoFreqRes[AT_AUTO_FREQ_PICs_COUNT] = {		// Filed Scanner Auto Frequency pictures //
	{ 154, 166, 0, 0, NULL, "ATAFREQICON.png"},
	{ 192, 44, 0, 0, NULL, "ATAFBlackPart.png"},
	{ 192, 44, 0, 0, NULL, "ATAFRedPart.png"},
	{ 50, 26, 0, 0, NULL, "ATAFREQ_T1.png"},
	{ 50, 26, 0, 0, NULL, "ATAFREQ_T2.png"},
	{ 50, 26, 0, 0, NULL, "ATAFREQ_T3.png"},
	{ 50, 26, 0, 0, NULL, "ATAFREQ_T4.png"},
	{ 50, 26, 0, 0, NULL, "ATAFREQ_T5.png"},
};

ResInfoType ATLoadingRes[AT_LOADING_PICs_COUNT] = {		// Filed Scanner Loading pictures //
	{ 128, 128, 0, 0, NULL, "ATLoad_T1.png"},
	{ 128, 128, 0, 0, NULL, "ATLoad_T2.png"},
	{ 128, 128, 0, 0, NULL, "ATLoad_T3.png"},
	{ 128, 128, 0, 0, NULL, "ATLoad_T4.png"},
	{ 128, 128, 0, 0, NULL, "ATLoad_T5.png"},
	{ 128, 128, 0, 0, NULL, "ATLoad_ManFreq.png"},
	{ 129, 21, 0, 0, NULL, "ATLoad_Bar1.png"},
	{ 129, 21, 0, 0, NULL, "ATLoad_Bar2.png"},
	{ 129, 21, 0, 0, NULL, "ATLoad_Bar3.png"},
	{ 129, 21, 0, 0, NULL, "ATLoad_Bar4.png"},
	{ 129, 21, 0, 0, NULL, "ATLoad_Bar5.png"}
};

ResInfoType ATBrightVolRes[AT_BRIGHT_VOL_PICs] = {		// Filed Scanner Brigntess and volume setting pictures //
	{ 131, 157, 0, 0, NULL, "ATBrightIcon.png"},
	{ 119, 163, 0, 0, NULL, "ATVolIcon.png"},
	{ AT_BRIGHT_VOL_BAR_SIZE_X, AT_BRIGHT_VOL_BAR_SIZE_Y, 0, 0, NULL, "ATSetFull.png"},
	{ AT_BRIGHT_VOL_BAR_SIZE_X, AT_BRIGHT_VOL_BAR_SIZE_Y, 0, 0, NULL, "ATSetNull.png"},
};

ResInfoType ATDistanceRes[AT_DISTANCE_PICs_COUNT] = {       // Filed Scanner Distance Selection pictures //
    { AT_DISTANCE_ICON_SIZE_X, AT_DISTANCE_ICON_SIZE_Y, 0, 0, NULL, "ATDistanceIcon.png"},
	{ 158, 50, 0, 0, NULL, "ATDistBlackPart.png"},
	{ 158, 50, 0, 0, NULL, "ATDistRedPart.png"},
};

ResInfoType ATManFreqRes[AT_MAN_FREQ_PICs_COUNT] = {       // Filed Scanner Manual Frequency pictures //
    { 154, 166, 0, 0, NULL, "ATManFreqIcon.png"}, 
	{ 162, 210, 0, 0, NULL, "ATMFreqWinB.png"}, 
};

ResInfoType ATLangRes[AT_LANG_PICs_COUNT] = {       // Filed Scanner Brigntess and volume setting pictures //
    { AT_LANG_ICON_SIZE_X, AT_LANG_ICON_SIZE_Y, 0, 0, NULL, "ATLangIcon.png"}, 
    { AT_LANG_ICON_WIN_SIZE_X, AT_LANG_ICON_WIN_SIZE_Y, 0, 0, NULL, "ATLangWinBack.png"},
    { 91, 67, 0, 0, NULL, "ATLangPtr.png"},   
};

ResInfoType DevSelectRes[DEV_SELECT_PICs_COUNT] = {       // Device Type Select //
    { GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "DevSelectBack.png"}, 
    { DEV_SELECT_DETECTOR_SIZE_X, DEV_SELECT_DETECTOR_SIZE_Y, 0, 0, NULL, "DSWithDetect.png"},
    { DEV_SELECT_DETECTOR_SIZE_X, DEV_SELECT_DETECTOR_SIZE_Y, 0, 0, NULL, "DSWithDetectArrow.png"},
    { DEV_SELECT_DETECTOR_SIZE_X, DEV_SELECT_DETECTOR_SIZE_Y, 0, 0, NULL, "DSNODetect.png"},
    { DEV_SELECT_FS_SIZE_X, DEV_SELECT_FS_SIZE_Y, 0, 0, NULL, "DSWithFS.png"},
    { DEV_SELECT_FS_SIZE_X, DEV_SELECT_FS_SIZE_Y, 0, 0, NULL, "DSWithFSArrow.png"},
    { DEV_SELECT_FS_SIZE_X, DEV_SELECT_FS_SIZE_Y, 0, 0, NULL, "DSNOFS.png"},
};
ResInfoType SysLoadingRes[SYS_LOADING_PICs_COUNT] = {	// System Loading //
	{ GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "SLBack2.png"},
	{ SYS_LOAD_MID_WIN_SIZE_X, SYS_LOAD_MID_WIN_SIZE_Y, 0, 0, NULL, "SLMidB2BB.png"},
	{ SYS_LOAD_MID_WIN_SIZE_X, SYS_LOAD_MID_WIN_SIZE_Y, 0, 0, NULL, "SLMidB2BR.png"},
	{ 50, 50, 0, 0, NULL, "SLPower.png"},
	{ 50, 50, 0, 0, NULL, "SLMainB.png"},
	{ 50, 50, 0, 0, NULL, "SLGC.png"},
	{ 50, 50, 0, 0, NULL, "SLCpu.png"},
	{ 50, 50, 0, 0, NULL, "SLRam.png"},
	{ 50, 50, 0, 0, NULL, "SLHdd.png"},
	{ 50, 50, 0, 0, NULL, "SLCoil.png"},
	{ 50, 50, 0, 0, NULL, "SLMagnet.png"},
};

ResInfoType FSResBLUE[AT_RADAR_ICONs_COUNT] = {		// Filed Scanning page pictures //
	{ GLCD_X_SIZE, GLCD_Y_SIZE, 0, 0, NULL, "FSBckgndb.png"},
	{ 186, 176, 0, 0, NULL, "radar_1b.png"},
	{ 186, 176, 0, 0, NULL, "radar_2b.png"},
	{ 186, 176, 0, 0, NULL, "radar_3b.png"},
	{ 186, 176, 0, 0, NULL, "radar_4b.png"},
	{ 186, 176, 0, 0, NULL, "radar_5b.png"},
	{ 186, 176, 0, 0, NULL, "radar_6b.png"},
	{ 186, 176, 0, 0, NULL, "radar_7b.png"},
	{ 186, 176, 0, 0, NULL, "radar_8b.png"},
	{ 186, 176, 0, 0, NULL, "radar_9b.png"},
	{ 186, 176, 0, 0, NULL, "radar_10b.png"},
	{ 186, 176, 0, 0, NULL, "radar_11b.png"},
	{ 186, 176, 0, 0, NULL, "radar_12b.png"},
	{ 186, 176, 0, 0, NULL, "radar_13b.png"},
	{ 186, 176, 0, 0, NULL, "radar_14b.png"},
	{ 186, 176, 0, 0, NULL, "radar_15b.png"},
	{ 186, 176, 0, 0, NULL, "radar_16b.png"},
	{ 186, 176, 0, 0, NULL, "radar_17b.png"}
};

ResInfoType AZPLoadingRes[AZP_LOADING_PICs_COUNT] = {	// AZ plus System Loading //
	{ 200, 100, 0, 0, NULL, "CompanyLogo.png"},
	{ 200, 100, 0, 0, NULL, "DeviceLogo.png"},
};

ResInfoType AZPSBRes[SB_AZP_ICONs_COUNT] = {	// Status Bar Pictures // 
	{ SB_TOP_ICON_SIZEX, SB_TOP_ICON_SIZEY, 0, 0, NULL,	"AZPSettings.png" },
	{ SB_TOP_ICON_SIZEX, SB_TOP_ICON_SIZEY, 0, 0, NULL, "AZPGB.png" },
	{ SB_TOP_ICON_SIZEX, SB_TOP_ICON_SIZEY, 0, 0, NULL, "AZPSense.png" },
	{ SB_TOP_ICON_SIZEX, SB_TOP_ICON_SIZEY, 0, 0, NULL, "AZPVol.png" },
	{ SB_TOP_ICON_SIZEX, SB_TOP_ICON_SIZEY, 0, 0, NULL, "AZPBirght.png" },
	{ SB_TOP_ICON_SIZEX, SB_TOP_ICON_SIZEY, 0, 0, NULL, "AZPBat.png" }
};

ResInfoType AZGBRes[AZP_GB_PICs_COUNT] = {	// AZ plus GB //
	{ AZP_GB_COIL_SIZEX, AZP_GB_COIL_SIZEY, 0, 0, NULL, "AZPGBCoil.png"}
};

extern uint8_t sdmmc_ResloadInit(void);
extern uint8_t sdmmc_LoadRes(uint8_t ResGroup, ResInfoType *ResInfoPtr);

/* ResInfoType INTRORes[INTRO_PICS_INDX_COUNT] = {0};	// Names will be set later @runtime */

ResInfoType *SBResources[APP_PICs_GROUP_COUNT] = {
	&SBRes[0], &RMRes[0], &OTORes[0], \
		&DPTRes[0], &GBRes[0], &HBRes[0], &MARes[0], &SYSRes[0], &FBARRes[0], &NBARRes[0], \
			&RFONTs[0], &FSResRED[0], &AUDIOs[0], NULL, &ATMenuRes[0], &ATAutoFreqRes[0], &ATLoadingRes[0], \
				&ATBrightVolRes[0], &ATDistanceRes[0], &ATManFreqRes[0], &(ATLangRes[0]), &DevSelectRes[0], \
					&SysLoadingRes[0], &FSResBLUE[0], &AZPLoadingRes[0], &(AZPSBRes[0]), &(AZGBRes[0])
};

uint8_t ResLoadInit(void) 
{
	//for(volatile uint8_t indx=0; indx<APP_PICs_GROUP_COUNT ; indx++)
		//DEBUGM("ResGroup Indx%u, Name%s\n", indx, ResGroupDirs[indx]);
#if(RES_LOAD_SOURCE	== RES_LOAD_SDMMC)	
	return sdmmc_ResloadInit();
#elif(RES_LOAD_SOURCE == RES_LOAD_SPI_FLASH)
	return spi_flash_ResloadInit();
#else
	#error "NOT IMPLEMENTED"
#endif
}

// Loads the resource whose relative path given as parameter //
uint8_t LoadRes(uint8_t ResGroup, ResInfoType *ResInfoPtr)
{
	uint8_t res = 0;
	
	if((NULL == ResInfoPtr) || (ResGroup >= APP_PICs_GROUP_COUNT))
		while(1);	// FATAL dummy Stupid Programmer Error :)) //
		
#if(RES_LOAD_SOURCE	== RES_LOAD_SDMMC)	
	return sdmmc_LoadRes(ResGroup, ResInfoPtr);
#elif(RES_LOAD_SOURCE == RES_LOAD_SPI_FLASH)
	return spi_flash_LoadRes(ResGroup, ResInfoPtr);
#else
	#error "NOT IMPLEMENTED YET:(( But if you do i will love it:))"
#endif	

	return res;
}

void DeInitGroupRes(uint8_t GroupIndx) 
{
	volatile uint8_t indx;
	for(indx=0	;	indx<GroupInfos[GroupIndx].IconCount ;	indx++) {
		GUI_MEMDEV_Delete(SBResources[GroupIndx][indx].hMemHandle);
		SBResources[GroupIndx][indx].SDRAM_adr = NULL;
		SBResources[GroupIndx][indx].hMemHandle = 0;
	}
}

uint8_t InitGroupRes(uint8_t GroupIndx, uint8_t findx)
{
	uint32_t prev, duration;
	uint8_t res = 0;
	volatile uint8_t indx;
	GUI_COLOR RecoveryColor;
	
	if((RUNTIME_FONTs != GroupIndx) && (AUDIO_TRACs != GroupIndx)) {
		RecoveryColor = GUI_GetColor();
		GUI_SetColor(GUI_WHITE);
	}

	if(0xFF != findx)	
		indx = findx;	// start from the desired resource's index // 
	else
		indx = 0;		// start from zero to initialize all of the indexes // 
	for(;	((indx<GroupInfos[GroupIndx].IconCount) && (0xFF == findx)) || ((0xFF != findx) && (indx == findx));	indx++) {
		// If resource is NOT AUDIO file and initialized before, skip it //	
			// Audio files can be reinitialized when volume level changed by user // 
		if((0 != SBResources[GroupIndx][indx].hMemHandle) && ((AUDIO_TRACs != GroupIndx)))
			continue;	
		if((RUNTIME_FONTs != GroupIndx) && (AUDIO_TRACs != GroupIndx))
			// All menu pictures will be read into same SDRAM address and then emWin Memory Device will be created from 
				// that picture data located in SDRAM, at the end SDRAM picture data will be thrown out // 			
			SBResources[GroupIndx][indx].SDRAM_adr = (uint8_t *)RUNTIME_PICs_SDRAM_START;
		else {
			// Other types (audio files & fonts) will be located sequentially after previous one and //
				// will stand during all application life-cycle // 
			// If the resource (audio/font) is initialized before use the same address for new cycle // 
			if((0xFF == findx) && (0 == SBResources[GroupIndx][indx].SDRAM_adr)){
				if(0 == indx) { 
					uint32_t sdadr = (RUNTIME_FONTs == GroupIndx) ? \
						(RUNTIME_FONT_LOADING_AREA_START) : (RUNTIME_AUDIO_TRACs_SDRAM_START);
					SBResources[GroupIndx][indx].SDRAM_adr = (void *)sdadr;
				} else {
					uint32_t size_aligned = GET_ALIGNED(SBResources[GroupIndx][indx-1].size, SDRAM_STORE_ALINGMENT);
					SBResources[GroupIndx][indx].SDRAM_adr = \
						(uint8_t *)(SBResources[GroupIndx][indx-1].SDRAM_adr) + size_aligned + 64;
				}
			}
		}
		prev = GUI_X_GetTime();
		// Try to load resource from NVMem to SDRAM // 
		if(0 != (res = LoadRes(GroupIndx, &(SBResources[GroupIndx][indx])))) {	// Resource init FAILED // 
			ERRM("GIndx(%u) :: LoadRes(%s) FAILED\n", GroupIndx, SBResources[GroupIndx][indx].name);
			if((RUNTIME_FONTs != GroupIndx) && (AUDIO_TRACs != GroupIndx)) {	// Menu pictures init FAILED // 
				SBResources[GroupIndx][indx].SDRAM_adr = NULL;	// This points resource loading FAILED // 
				{	// Create null mem device to prevent application crash // 
					SBResources[GroupIndx][indx].hMemHandle = GUI_MEMDEV_CreateFixed(0, 0, \
						SBResources[GroupIndx][indx].xSize, SBResources[GroupIndx][indx].ySize, \
							GUI_MEMDEV_NOTRANS, GUI_MEMDEV_APILIST_32, GUICC_8888);	
					GUI_MEMDEV_Select(SBResources[GroupIndx][indx].hMemHandle);
					GUI_SetBkColor(GUI_BLACK);
					GUI_Clear();	// Set Window is UNCHANGED @ this stage // 
				}
			}
			else {	// Failed resource is FONT or AUDIO file //
				if(RUNTIME_FONTs == GroupIndx) {	// runtime FONT Loading Failed use DEFAULT FONTs // 
					const GUI_FONT *temp_fonts[] = {GUI_FONT_32B_ASCII, GUI_FONT_24B_ASCII, GUI_FONT_20B_ASCII, GUI_FONT_16B_ASCII};
					GUI_FontArray[indx] = *(temp_fonts[indx]);
				}
				else if(AUDIO_TRACs == GroupIndx) { /* :TODO: */}
			}
			while(TODO_ON_ERR);
		}
		else {	// Go on if resource read successfully from non-volatile memory // 
			duration = GUI_X_GetTime() - prev;
			DEBUGM("(%s) loaded in (%u) MS\n", SBResources[GroupIndx][indx].name, duration);
			if((RUNTIME_FONTs == GroupIndx) || (AUDIO_TRACs == GroupIndx)) {	// Storing resource into RAM is enough for FONTs //
				SBResources[GroupIndx][indx].hMemHandle = 0xFF;	// Point that initialization DONE, next time we will skip it // 
				// Do resource family (FONT family) specific operation // 
				if(RUNTIME_FONTs == GroupIndx) {
					GUI_SIF_CreateFont(SBResources[GroupIndx][indx].SDRAM_adr, &GUI_FontArray[indx], GUI_SIF_TYPE_PROP);
				}
				else if(AUDIO_TRACs == GroupIndx) {	// Scale audio file data with AppVolume level // 
					uint16_t *afile_ptr = (uint16_t *)SBResources[GroupIndx][indx].SDRAM_adr;
					uint32_t afile_samples = SBResources[GroupIndx][indx].size / 2;	// Each audio sample is unsigned 16 bit // 
					uint16_t AppVol = APP_GetValue((APP_IS_DETECTOR == APP_GetValue(ST_DEV_TYPE)) ? ST_VOL : ST_AT_VOL);
					volatile uint32_t indx;

					for(indx=afile_samples-1 ;; indx--, afile_ptr++) {
						register int32_t diff = (int32_t)(*afile_ptr) - 0x8000;
						diff = (diff * AppVol) / VOLUME_MAX;
						//diff = (diff * AppVol) / VOLUME_MAX;	// decrease rapidly when level is low // 
						(*afile_ptr) = diff + 0x8000;
						
						if(unlikely(!indx)) break;
					}
				}
				continue;	// Skip the rest of PICTURE family specific operations //  
			}	
			// 1- Load Mask data if there is & Create hMem for it //
			uint8_t mask_state = TRUE;
			char *MaskStart = NULL;
			uint32_t size_aligned = GET_ALIGNED(SBResources[GroupIndx][indx].size, SDRAM_STORE_ALINGMENT);
			ResInfoType TempRes = {SBResources[GroupIndx][indx].xSize, SBResources[GroupIndx][indx].ySize, \
				0, 0, (uint8_t *)SDRAM_BASE_ADDR + size_aligned, NULL};
			if(NULL == SBResources[GroupIndx][indx].ResMaskPtr) {
				// Create Masked resource name that derived from base resource' s name // 
				strncpy(TempRes.name, SBResources[GroupIndx][indx].name, RES_NAME_MAX_LENGTH);
				MaskStart = strrchr(TempRes.name,'.');
				if(NULL != MaskStart) {
					strcpy(MaskStart,"Mask.bmp");
					TRACEM("Masked Res Name(%s)\n", TempRes.name);
				}
				else {
					mask_state = FALSE;
					ERRM("Res(%s) does NOT incldes \".\" character\n", SBResources[GroupIndx][indx].name);
				}
			}
			else {
				strncpy(TempRes.name, SBResources[GroupIndx][indx].ResMaskPtr, RES_NAME_MAX_LENGTH);
				TRACEM("Using PRESET Mask(%s)\n", SBResources[GroupIndx][indx].ResMaskPtr);
			}
			if(TRUE == mask_state) {
				// Try to load masked resource, if it is exist LoadRes() will return with "0" // 
				if(0 == LoadRes(GroupIndx, &TempRes)) {	// Masked resource is exist // 
					// Create hMem for Masked Resource & Initialize with masked picture data // 
					// 1- Create Mask Device //
					TempRes.hMemHandle = GUI_MEMDEV_CreateFixed(0, 0, TempRes.xSize, \
						TempRes.ySize, GUI_MEMDEV_NOTRANS, GUI_MEMDEV_APILIST_8, GUICC_8);
					GUI_MEMDEV_Select(TempRes.hMemHandle);
					GUI_SetBkColor(GUI_BLACK);
					GUI_Clear();
					if((NULL != TempRes.SDRAM_adr) && (0 != TempRes.size)) {	// 8bpp *.bmp image for transparency mask // 
						GUI_BMP_Draw((const void *)TempRes.SDRAM_adr, 0, 0);
					}
				}
				else {
					ERRM("MaskRes(%s) load FAILED\n", TempRes.name);
				}
			}
			else {
				ERRM("Mask ACCESS FAILED\n");
			}
			// 2- Create hMemData for Base Resource //
			SBResources[GroupIndx][indx].hMemHandle = GUI_MEMDEV_CreateFixed(0, 0, \
				SBResources[GroupIndx][indx].xSize, SBResources[GroupIndx][indx].ySize, \
					GUI_MEMDEV_NOTRANS, GUI_MEMDEV_APILIST_32, GUICC_8888);
			GUI_MEMDEV_Select(SBResources[GroupIndx][indx].hMemHandle);
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();	// Set Window is UNCHANGED @ this stage // 
			GUI_PNG_Draw((const void *)SBResources[GroupIndx][indx].SDRAM_adr, \
				SBResources[GroupIndx][indx].size, 0, 0);

			// 3- Punchout hMemMask into hMemData // 
			if(0 != TempRes.hMemHandle) {
				GUI_MEMDEV_PunchOutDevice(SBResources[GroupIndx][indx].hMemHandle, TempRes.hMemHandle);
				// 4- Delete TempRes if created // 
				GUI_MEMDEV_Delete(TempRes.hMemHandle);
			}
		}
	}
	// 5- Recover back GLOAL GUI settings //
	if((RUNTIME_FONTs != GroupIndx) && (AUDIO_TRACs != GroupIndx)) {
		GUI_MEMDEV_Select(0);	// Set LCD for drawing operations target again // 
		GUI_SetColor(RecoveryColor);	// Global drawing color is set back the value before this function // 
	}

	return res;
}

#if(0)
uint8_t InitGroupResINTMEM(uint8_t GroupIndx)
{
	uint8_t res = 0;
	volatile uint8_t indx;
	GUI_COLOR RecoveryColor = GUI_GetColor();
	GUI_SetColor(GUI_WHITE);
		
	for(indx=0	;	indx<GroupInfos[GroupIndx].IconCount ;	indx++) {
		// If resource initialized before skip it //
		if(0 == SBResources[GroupIndx][indx]->hMemHandle) {
			SBResources[GroupIndx][indx]->SDRAM_adr = (uint8_t *)RUNTIME_PICs_SDRAM_START;
			if(NULL == SBResources[GroupIndx][indx]->ResPtr) {
				SBResources[GroupIndx][indx]->SDRAM_adr = NULL;
				//xprintf("LoadResINITMEM(%s) FAILED", GroupIndx, SBResources[GroupIndx][indx]->name);
				while(TODO_ON_ERR);
			}
			// Goon if resource read successfully from non-volatile memory // 
			else {
				// 1- Load Mask data if there is & Create hMem for it //
				GUI_MEMDEV_Handle hMemHandleTEMP = 0;
				memcpy(SBResources[GroupIndx][indx]->SDRAM_adr, SBResources[GroupIndx][indx]->ResPtr, \
					SBResources[GroupIndx][indx]->size);
				// Try to load masked resource, if it is exist LoadRes() will return with "0" // 
				if(NULL != SBResources[GroupIndx][indx]->ResMaskPtr) {
					uint32_t size_aligned = \
						((SBResources[GroupIndx][indx]->size/SDRAM_STORE_ALINGMENT) + 1)*SDRAM_STORE_ALINGMENT;
					memcpy((uint8_t *)SDRAM_BASE_ADDR + size_aligned, \
						SBResources[GroupIndx][indx]->ResMaskPtr, SBResources[GroupIndx][indx]->mask_size);
					// Create hMem for Masked Resource & Initialize with masked picture data // 
					// 1- Create Mask Device //
					hMemHandleTEMP = GUI_MEMDEV_CreateFixed(0, 0, SBResources[GroupIndx][indx]->xSize, \
						SBResources[GroupIndx][indx]->ySize, GUI_MEMDEV_NOTRANS, GUI_MEMDEV_APILIST_8, GUICC_8);
					GUI_MEMDEV_Select(hMemHandleTEMP);
					GUI_SetBkColor(GUI_BLACK);
					GUI_Clear();
					GUI_DrawBitmap((const void *)((uint8_t *)SDRAM_BASE_ADDR + size_aligned), 0, 0);
					// Masked hMem is Ready for transparency setting // 
				}
				else {
					ERRM("Res(%s) MASK does NOT EXIST\n", SBResources[GroupIndx][indx]->name);
				}
				// 2- Create hMemData for Base Resource //
				SBResources[GroupIndx][indx]->hMemHandle = GUI_MEMDEV_CreateFixed(0, 0, \
					SBResources[GroupIndx][indx]->xSize, SBResources[GroupIndx][indx]->ySize, \
						GUI_MEMDEV_NOTRANS, GUI_MEMDEV_APILIST_32, GUICC_8888);
				GUI_MEMDEV_Select(SBResources[GroupIndx][indx]->hMemHandle);
				GUI_SetBkColor(GUI_BLACK);
				GUI_Clear();	// Set Window is UNCHANGED @ this stage // 
				GUI_DrawBitmap((const void *)SBResources[GroupIndx][indx]->SDRAM_adr, 0, 0);

				// 3- Punchout hMemMask into hMemData // 
				if(0 != hMemHandleTEMP) {
					GUI_MEMDEV_PunchOutDevice(SBResources[GroupIndx][indx]->hMemHandle, hMemHandleTEMP);
					// 4- Delete TempRes if created // 
					GUI_MEMDEV_Delete(hMemHandleTEMP);
				}
			}
		}
	}
	// 5- Recover back GLOBAL GUI settings //
	GUI_MEMDEV_Select(0);	// Set LCD for drawing operations target again // 
	GUI_SetColor(RecoveryColor);	// Global drawing color is set back the value before this function // 

	return res;
}
#endif
