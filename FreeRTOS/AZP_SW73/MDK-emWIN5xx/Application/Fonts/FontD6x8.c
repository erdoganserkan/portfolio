#include "GUI.h"
#include "GUIDEMO.h"
#include "AppFont.h"

#if(FONTD6x8 != 0)
/*********************************************************************
*                                                                    *
*       GUI_FontD6x8                                                 *
*                                                                    *
**********************************************************************
*/
static GUI_CONST_STORAGE unsigned char acFontD6x8[16][8] = {
  {
    _XXX____,
    X___X___,
    X___X___,
    X___X___,
    X___X___,
    X___X___,
    X___X___,
    _XXX____,
  },{
    __X_____,
    _XX_____,
    __X_____,
    __X_____,
    __X_____,
    __X_____,
    __X_____,
    _XXX____,
  },{
    _XXX____,
    X___X___,
    ____X___,
    ___X____,
    __X_____,
    _X______,
    X_______,
    XXXXX___,
  },{
    _XXX____,
    X___X___,
    ____X___,
    ___X____,
    ___X____,
    ____X___,
    X___X___,
    _XXX____,
  },{
    ___X____,
    __XX____,
    _X_X____,
    X__X____,
    XXXXX___,
    ___X____,
    ___X____,
    ___X____,
  },{
    XXXXX___,
    X_______,
    X_______,
    XXXX____,
    ____X___,
    ____X___,
    X___X___,
    _XXX____,
  },{
    __XX____,
    _X______,
    X_______,
    XXXX____,
    X___X___,
    X___X___,
    X___X___,
    _XXX____,
  },{
    XXXXX___,
    ____X___,
    ____X___,
    ___X____,
    __X_____,
    _X______,
    _X______,
    _X______,
  },{
    _XXX____,
    X___X___,
    X___X___,
    _XXX____,
    X___X___,
    X___X___,
    X___X___,
    _XXX____,
  },{
    _XXX____,
    X___X___,
    X___X___,
    _XXXX___,
    ____X___,
    ____X___,
    ___X____,
    _XX_____,
  },{
    ________,
    ________,
    __X_____,
    __X_____,
    XXXXX___,
    __X_____,
    __X_____,
    ________,
  },{
    ________,
    ________,
    ________,
    ________,
    XXXXX___,
    ________,
    ________,
    ________,
  },{
    ________,
    ________,
    ________,
    ________,
    ________,
    ________,
    ________,
    ________,
  },{
    ________,
    ________,
    ________,
    ________,
    ________,
    ________,
    _XX_____,
    _XX_____,
  },{
    ________,
    ________,
    _XX_____,
    _XX_____,
    ________,
    _XX_____,
    _XX_____,
    ________
  },{
    ________,
    _XX___X_,
    _XX__X__,
    ____X___,
    ___X____,
    __X__XX_,
    _X___XX_,
    ________}

};

static GUI_CONST_STORAGE GUI_CHARINFO GUI_FontD6x8_CharInfo[16] = {
   {  6,  6,  1, acFontD6x8[12] } /* code 0020 ' ' */
  ,{  6,  6,  1, acFontD6x8[15] } /* code 0025 '%' */
  ,{  6,  6,  1, acFontD6x8[10] } /* code 002B '+' */
  ,{  6,  6,  1, acFontD6x8[11] } /* code 002D '-' */
  ,{  6,  6,  1, acFontD6x8[13] } /* code 002E '.' */
  ,{  6,  6,  1, acFontD6x8[0]  } /* code 0030 '0' */
  ,{  6,  6,  1, acFontD6x8[1]  } /* code 0031 '1' */
  ,{  6,  6,  1, acFontD6x8[2]  } /* code 0032 '2' */
  ,{  6,  6,  1, acFontD6x8[3]  } /* code 0033 '3' */
  ,{  6,  6,  1, acFontD6x8[4]  } /* code 0034 '4' */
  ,{  6,  6,  1, acFontD6x8[5]  } /* code 0035 '5' */
  ,{  6,  6,  1, acFontD6x8[6]  } /* code 0036 '6' */
  ,{  6,  6,  1, acFontD6x8[7]  } /* code 0037 '7' */
  ,{  6,  6,  1, acFontD6x8[8]  } /* code 0038 '8' */
  ,{  6,  6,  1, acFontD6x8[9]  } /* code 0039 '9' */
  ,{  6,  6,  1, acFontD6x8[14] } /* code 003A ':' */
};

static GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontD6x8_Prop5 = {
   0x0030 /* first character */
  ,0x003A /* last character  */
  ,&GUI_FontD6x8_CharInfo[  5] /* address of first character */
  ,(GUI_CONST_STORAGE GUI_FONT_PROP*)0 /* pointer to next GUI_FONT_PROP */
};

static GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontD6x8_Prop4 = {
   0x002D /* first character */
  ,0x002E /* last character  */
  ,&GUI_FontD6x8_CharInfo[  3] /* address of first character */
  ,&GUI_FontD6x8_Prop5 /* pointer to next GUI_FONT_PROP */
};

static GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontD6x8_Prop3 = {
   0x002B /* first character */
  ,0x002B /* last character  */
  ,&GUI_FontD6x8_CharInfo[  2] /* address of first character */
  ,&GUI_FontD6x8_Prop4 /* pointer to next GUI_FONT_PROP */
};

static GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontD6x8_Prop2 = {
   0x0025 /* first character */
  ,0x0025 /* last character  */
  ,&GUI_FontD6x8_CharInfo[  1] /* address of first character */
  ,&GUI_FontD6x8_Prop3 /* pointer to next GUI_FONT_PROP */
};

static GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontD6x8_Prop1 = {
   0x0020 /* first character */
  ,0x0020 /* last character  */
  ,&GUI_FontD6x8_CharInfo[  0] /* address of first character */
  ,&GUI_FontD6x8_Prop2 /* pointer to next GUI_FONT_PROP */
};

GUI_CONST_STORAGE GUI_FONT GUI_FontD6x8 = {
   GUI_FONTTYPE_PROP /* type of font    */
  ,8 /* height of font  */
  ,8 /* space of font y */
  ,1 /* magnification x */
  ,1 /* magnification y */
  ,{&GUI_FontD6x8_Prop1}
  ,8 /* Baseline */
  ,0 /* LHeight */
  ,8 /* CHeight */
};

#endif		/* End of FONTD6x8 */

