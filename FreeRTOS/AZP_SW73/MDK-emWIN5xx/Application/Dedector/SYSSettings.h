#ifndef SYS_SETTINGs_H
#define SYS_SETTINGs_H

#include <stdio.h>
#include <stdint.h>

#define SYS_SETTINGs_ICON_COUNT	6
// UPPER ICONs //
#define LINEUP_ICONs_POSY	55
#define VOLUME_ICON_POSX	107
#define LANG_ICON_POSX		206
#define BRIGHT_ICON_POSX	311

// LOWER ICONs // 
#define LINEDOWN_ICONs_POSY	185
#define FERROS_ICON_POSX	VOLUME_ICON_POSX
#define SENS_ICON_POSX		LANG_ICON_POSX
#define FACTORY_POSX		BRIGHT_ICON_POSX
// SELECT CIRCLE //
#define SELECT_CIRCLE_X_DIFF	14
#define SELECT_CIRCLE_Y_DIFF	14
#define SELECT_ANIM_INTERVAL_MS		250

#define SYS_SET_ICON_SIZEX		69
#define SYS_SET_ICON_SIZEY		79

// Screen Name String Display Location // 
#define SYS_SETTINGs_STR_UPX		73
#define SYS_SETTINGs_STR_UPY		10
#define SYS_SETTINGs_STR_DOWNX	405
#define SYS_SETTINGs_STR_DOWNY	32

// Active Icon Related String Display Location // 
#define ACTIVE_STR_UPX		70
#define ACTIVE_STR_UPY		144
#define ACTIVE_STR_DOWNX	397
#define ACTIVE_STR_DOWNY	168

extern uint8_t SYSSettings(void);
extern void SYS_on_comm_fail(void *msgp);

#endif
