#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <event_groups.h>
#include <task.h>
#include "GLCD.h"
#include "GUIDEMO.h"
#include "DIALOG.h"
#include "BSP.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "AppSettings.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "UartInt.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "DepthCalc.h"
#include "APlay.h"
#include "Analog.h"

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

extern void JD_init(void);
