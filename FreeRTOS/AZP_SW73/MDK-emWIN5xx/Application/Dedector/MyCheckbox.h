#ifndef MY_CHECKBOX_H
#define MY_CHECKBOX_H

#include <stdint.h>
#include "DIALOG.h"

// Definitions //
#define CHECBOX_BUTTON_SIZE 20


// Type Definitions // 
typedef void (* OnCHF)(void);

typedef enum {
	CB_NOT_CHECKED	= 0,
	CB_CHECKED			= 1,
	
	CB_CHECK_STATEs
} CBState_Type;

typedef struct {
	uint8_t InitialState;
	int x0;
	int y0;
	int xSize;
	int ySize;
	int WinFlags;
	int ExFlags;
	int Id;
	int NumExtraBytes;
	WM_HWIN hParent;
	WM_HWIN hCB;	
	void *UserDataPtr;
	char *CBStr;
	OnCHF OnCHFunc;
} sCBData_Type;
typedef sCBData_Type * CBHandle;

extern CBHandle CB_init(sCBData_Type *CBDataPtr);
extern void CB_delete(CBHandle CB2Del);

#endif
