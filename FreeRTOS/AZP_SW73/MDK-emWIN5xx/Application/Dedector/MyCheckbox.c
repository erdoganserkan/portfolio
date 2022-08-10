#include <stdlib.h>
#include <string.h>
#include "AppCommon.h"
#include "AppFont.h"
#include "RuntimeLoader.h"
#include "MyCheckbox.h"


CBHandle CB_init(sCBData_Type *CBDataPtr)
{
  CHECKBOX_SKINFLEX_PROPS Props;
  WM_HWIN                 hCheck;
  unsigned                i;
	CBHandle 								retVal;

	if(NULL == CBDataPtr) {
		while(STALLE_ON_ERR);
		return NULL;
	}
	
	retVal = (sCBData_Type *)calloc(1, sizeof(sCBData_Type));
	if(NULL != retVal) {
		memcpy(retVal, CBDataPtr, sizeof(sCBData_Type));
	}
	else
		return NULL;
  //
  // Get properties of current skin
  //
  CHECKBOX_GetSkinFlexProps(&Props, CHECKBOX_SKINFLEX_PI_ENABLED);
  //
  // Change button size and colors of skin
  //
  Props.ButtonSize = CHECBOX_BUTTON_SIZE;
  for (i = 0; i < GUI_COUNTOF(Props.aColorFrame); i++) {
    Props.aColorFrame[i] |= GUI_BLUE;
  }
  for (i = 0; i < GUI_COUNTOF(Props.aColorInner); i++) {
    Props.aColorInner[i] |= GUI_BLUE;
  }
  Props.ColorCheck |= GUI_BLUE;
  //
  // Set properties of current skin
  //
  CHECKBOX_SetSkinFlexProps(&Props, CHECKBOX_SKINFLEX_PI_ENABLED);
  //
  // Enable skinning
  //
  CHECKBOX_SetDefaultSkin(CHECKBOX_SKIN_FLEX);
  //
  // Create checkbox and do some more configurations
  //
  hCheck = \
		CHECKBOX_CreateUser(CBDataPtr->x0, CBDataPtr->y0, CBDataPtr->xSize, CBDataPtr->ySize, CBDataPtr->hParent, \
			CBDataPtr->WinFlags, CBDataPtr->ExFlags, CBDataPtr->Id, CBDataPtr->NumExtraBytes);
  CHECKBOX_SetUserData(hCheck, &(CBDataPtr->UserDataPtr), sizeof(CBDataPtr->UserDataPtr));
  CHECKBOX_SetTextColor(hCheck, GUI_WHITE);
  CHECKBOX_SetFont(hCheck, APP_24B_FONT);
  CHECKBOX_SetText(hCheck, CBDataPtr->CBStr);
  CHECKBOX_SetTextAlign(hCheck, GUI_TA_VCENTER);
  CHECKBOX_SetState(hCheck, (CB_CHECKED == CBDataPtr->InitialState)?(1):(0));

	return retVal;
}

void CB_delete(CBHandle CB2Del)
{
	if(NULL == CB2Del) {
		while(STALLE_ON_ERR);
		return;
	}
	free(CB2Del);
}
