#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdint.h>
#include "AppCommon.h"

// Type definitions //
typedef enum
{
	RELOAD_FROM_RAM	= 0,	// Reload last active settings //
	RELOAD_FROM_NVMEM,		// Reload from no-volatile memory (flash, sd-card etc.)
	RELOAD_FROM_SAFE_VALUEs,		// Reload from safe predefined values object 
	
	RELAOD_SRC_COUNT
} eAppReload_te;

// Variable Declarations // 
extern const sSettingType aSettings[ST_COUNT];

// Function Declarations // 
extern void APP_StoreSettings(uAppStore *pSettings, uint8_t storeNVM);
extern void App_ReloadSettings(uAppStore *pSettings, eAppReload_te reaload_src);
extern uAppStore *APP_GetSettingsAdr(void);
extern uint16_t APP_GetValue(uint8_t indx);
extern void APP_SetVal(uint8_t indx, uint16_t val, uint8_t storeNVM);
extern char const *GetString(uint8_t str_indx);
extern char const *GetString2Lang(uint8_t str_indx, uint8_t lang);
extern void AppSettings_StoreDefaults(void);

#endif
