#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <stdint.h>
#include "AppSettings.h"

/* Status Bar üzerinde olacak komponentler //
1- Pil Seviyesi
2- Arama Modu
3- EN SON Toprak Mineralizasyon Degeri 
4- Ses Siddeti
5- Hassasiyet Degeri 
6- Bobin Tipi
7- Aktif Dil Secenegi
8- Kazanc Degeri
*/	

#define SB_ANIM					TRUE		/* statusBar animation state */
#define SB_ANIM_RES_MS			(1000)
#define SB_BAT_WARN_ANIM_RES_MS	(1500)
#define SB_ANIM_MS					(6000)

// FULL MODE Definitions // 
#define STATUS_BAR_X_SIZE					(99)
#define STATUS_BAR_STR_Y_SZIE				(40)
#define SB_STR_COMMON_POSX					50
#define SB_STR_ENDX							87
#define SB_VOLUME_STR_POSY					11
#define SB_SENSITIVITY_STR_POSY				63
#define SB_BRIGHTNESS_STR_POSY				115
#define SB_GID_STR_POSY						168
#define SB_BATTERY_STR_POSY						220
#define SB_BAT_ICON_POSX						17
#define SB_BAT_ICON_POSY						224
#define SB_BAT_STR_POSY							227

// RECUDED MODE Definitions // 
#define REDUCED_SB_SIZEX	47
#define REDUCED_SB_SIZEY	36
#define REDUCED_SB_UR_POSX		(GLCD_X_SIZE-REDUCED_SB_SIZEX)	/* If using RIGHT-MOST */
#define REDUCED_SB_UL_POSX		0	/* If using LEFT-MOST */
#define REDUCED_SB_DR_POSX		(415)	
#define REDUCED_SB_DL_POSX		(GLCD_X_SIZE - REDUCED_SB_DR_POSX)	
#define REDUCED_SB_POSY		0
#define REDUCED_SB_DLR_POSY	(GLCD_Y_SIZE - REDUCED_SB_SIZEY - 25)
#define BATTERY_LEVELs		5

// TOP FULL MODE DEFINITIONs // 
#define SB_FULL_TOP_SIZE_Y		((GLCD_Y_SIZE/6) + 5)
#define SB_FULL_TOP_SIZE_X		(GLCD_X_SIZE)
#define SB_FULL_TOP_PARt_COUNT	6
#define SB_TOP_ICON_SIZEX		40
#define SB_TOP_ICON_SIZEY		35


#define JACK_DETECT_AVG_DEPTH	1

typedef enum {
	SB_SENSITIVITY	= 0,
	SB_VOLUME,
	SB_BRIGHTNESS,
	SB_GROUND_ID,
	SB_BATTERY,
	SB_JACK_STATE,
	SB_AZP_ACTIVE_POS,

	
	SB_MEMBER_COUNT
} eSB_MEMBERs;

typedef enum {
	HP_JACK_NOT_INSERTED	= 0,
	HP_JACK_INSERTED,

	HP_JACKSTATE_COUNT
} eJACK_STATEs;

typedef enum {
	UNMUTE_SPEAKERS_PIN_STATE = 0,
	MUTE_SPEAKERS_PIN_STATE	= 1,	// MAX9710, umute is ACTIVE HIGH // 

	SPEAKERS_PIN_STATE_COUNT
} eSPAKERS_PIN_SATEs;

typedef struct {
	uint16_t BatWarn;	// Battery Warning mode started(TRUE), not started(FALSE) // 
	uint16_t BatVisible;	// TRUE if display battery icon, FALSE if not display it // 
	uint16_t mode;			// Reduced or FULL mode // 
	uint16_t Sensitivity;	// Detector Algorithm Sensitivity 
	uint16_t Volume;	// Volume 
	uint16_t Brightness;	// LCD Backlight 
	uint16_t Mineralization;	// ground Soil degree 
	uint16_t Battery;	// Battery Percentage 
	uint16_t Jack_State;	// Audio Headphone Jack state (inserted or not)
	uint16_t active_pos;
} sSBData_Type;

typedef enum
{
	SB_FULL_MODE	= 0,	// SB display all indications (Batetry, Birghtness, Volume, GroundID, Sensitivity)
	SB_REDUCED_MODE_USE_RIGHT,	// SB shows only Battery @ right-most location // 
	SB_REDUCED_MODE_USE_LEFT,	// SB shows only Battery @ left-most location // 
	SB_REDUCED_MODE_USE_BOTTOM_RIGHT,
	SB_REDUCED_MODE_USE_BOTTOM_LEFT,
	SB_FULL_TOP,
	
	SB_MODEs_COUNT
} eSBTypes;

#define AZP_SYS_SET_MENU_MAX	9
typedef enum {
	AZP_SB_POS_SYS_SET 	= 0,
	AZP_SB_POS_GB				= 1,
	AZP_SB_POS_SENS			= 2,
	AZP_SB_POS_VOL			= 3,
	AZP_SB_POS_BRIGHT		= 4,
	AZP_SB_POS_BAT			= 5,
	AZP_SB_POS_MAX = AZP_SB_POS_BAT,

	AZP_SB_POS_COUNT
} eAZPSB_POS_t;

extern void SB_init(uint8_t mode);
extern void SB_delete(void);
extern uint16_t SB_setget_component(uint8_t indx, uint8_t set, uint16_t *valptr);
extern void OpenBatteryPopup(uint8_t mode);
#endif
