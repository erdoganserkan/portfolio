#ifndef AZP_LOADING_H
#define AZP_LOADING_H

#include <stdio.h>
#include <stdint.h>

// Loading Animation Window on AZP LADING SCREEN // 
#define AZP_LOADING_WIN_SIZE_X			250
#define AZP_LOADING_WIN_SIZE_Y			(((GLCD_Y_SIZE*4)/11)+10)
#define AZP_LOADING_WIN_LEFT_X			((GLCD_X_SIZE - AZP_LOADING_WIN_SIZE_X)/2)
#define AZP_LOADING_WIN_LEFT_Y			(115)

#define AZP_COMPANY_LOGO_LEFT_X			30
#define AZP_COMPANY_LOGO_LEFT_Y			10
#define AZP_DEVICE_LOGO_LEFT_X			250
#define AZP_DEVICE_LOGO_LEFT_Y			10

#define AZP_LOAD_MAX_LENGTH_SECONDs		10
#define AZP_LOADING_BACKGROUND_COLOR	GUI_LIGHTGRAY

extern uint8_t AZP_Loading(void);


#endif
