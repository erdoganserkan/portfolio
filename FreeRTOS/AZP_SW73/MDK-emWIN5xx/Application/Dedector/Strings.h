#ifndef STRINGS_H
#define STRINGS_H

#include "AppCommon.h"

typedef enum
{
	STR_OWN_LANG_INDX = 0,	// Current languge name // 
	// Welcome SCREEN Strings // 
	STR_BALANS_INDX,
	STR_STD_SEARCH_INDX,
	STR_MINERALIZED_INDX,
	STR_OTO_SEARCH_INDX,
	STR_DEPTH_CALC_INDX,
	STR_SYS_SETTINGs_INDX,
	STR_METAL_ANALYSIS_INDX,
	STR_BAT_WARN_INDX,	// String about that battery is critical 
	STR_BAT_FAIL_INDX,	// String about that battery is null 
	// BLANS SCREEN Strings // 
	STR_CONFIRM_INDX,	// Press confirm to start 
	STR_PROCESSING_INDX,	// Processing started please wait 
	STR_BALANS_OK_INDX,	// Balans is OK 
	STR_GROUND_ID_INDX,	// Ground ID String 
	STR_BALANS_FAILED_INDX,	// Balans failed
	STR_TRY_AGAIN_INDX,	// Try again balance string 
	// STD SEARCH Strings //
	STR_CAVITY_INDX,	// Cavity target 
	STR_MINERAL_INDX,	// Mineral target 
	STR_METAL_INDX,	// metal target 
	// OTO SEARCH Strings // 
	STR_FERROS_INDX,	// ferros target 
	STR_NONFERROS_INDX,	// Nonferros target 
	STR_GOLD_INDX,	// Gold target 
	// DEPTH CALC Strings //
	STR_SIZE_SELECT_INDX,	// Target size selection 
	STR_WIDTH_INDX,	// Width 
	STR_HEIGHT_INDX,	// Height string 
	STR_DEPTH_RESULT_INDX,	// Depth result indx
	// SYS SETTINGs Strings //
	STR_VOLUME_INDX,	// Volume setting 
	STR_SYS_SETTINGS_START = STR_VOLUME_INDX,
	STR_LANG_INDX,	// Languge Selection 
	STR_BRIGHT_INDX,	// Brightness Setting 
	STR_FERROS_SETTING_INDX,	// Ferros Elimination Setting 
	STR_SENS_INDX,	// Sensitivity Setting 
	STR_FACTORY_INDX,	// Factory Settings 
	STR_ACTIVE_INDX,	// Active String 
	STR_PASSIVE_INDX,	// Passive 
	STR_FACTORY_CONFIRM_INDX,	// Return to factory confirmation 
	STR_YES_INDX,	// Yes String 
	STR_NO_INDX,	// No string 
	// LOADING SCREEN //
	STR_SELECT_DEVICE,	// please select device type // 
	STR_DETECTOR,				// metal detector // 
	STR_FIELD_SCANNER,	// field scanner // 
	STR_LOADING_PLEASE_WAIT,	// loading pelase wait // 
	// AT_MENU SCREEN //
	STR_ATM_OTO_FREQ,
	STR_ATM_MAN_FREQ,
	STR_ATM_DISTANCE,
	STR_ATM_LANG,
	STR_ATM_LIGHT,
	STR_ATM_VOL,
	STR_AT_DISTANCE_METER,
	// AT AUTO FREQ // 
	STR_AT_AUTO_TYPE1_STR,		// Long Gold // 
	STR_AT_AUTO_TYPE2_STR,		// Short Gold // 
	STR_AT_AUTO_TYPE3_STR,		// Water // 
	STR_AT_AUTO_TYPE4_STR,		// Cavity // 
	STR_AT_AUTO_TYPE5_STR,		// All Metals // 
	// AT LOADING // 
	STR_AT_LOADING_STR,
	STR_AT_TYPE1_LONG_GOLD_STR,
	STR_AT_TYPE2_SHORT_GOLD_STR,
	STR_AT_TYPE3_WATER_STR,
	STR_AT_TYPE4_CAVITY_STR,
	STR_AT_TYPE5_ALL_METALS_STR,
	// AT MAN FREQ //
	STR_AT_MAN_FREQ_UNIT,
	// AT RADAR // 
	STR_FS_ACTIVE,
	// AT DISTANCE // 
	STR_SHORT_RANGE,
	STR_MID_RANGE,
	STR_LONG_RANGE,
	STR_MAXIMUM_RANGE,
	// AZP STRINGs // 
	STR_AZP_ALL_METALS,
	STR_AZP_DISC,
	STR_AZP_AUTO_GB,
	STR_AZP_MAN_GB,
	STR_AZP_GB_MAN_NUM,
	STR_AZP_ENABLED,
	STR_AZP_DISABLED,
	
	// A5P STRINGs // 
	STR_A5P_TARGET_FOUND,
	
	APP_STR_COUNT
} eAPP_STRINGs;

extern char const **AllLangStrs[LANG_COUNT];

#endif
