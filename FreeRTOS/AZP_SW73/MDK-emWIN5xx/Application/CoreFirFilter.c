#include <string.h>
#include "LPC177x_8x.h"         // Device specific header file, contains CMSIS
#include "system_LPC177x_8x.h"  // Device specific header file, contains CMSIS
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_adc.h"
#include "lpc177x_8x_timer.h"
#include "lpc177x_8x_pwm.h"
#include "SDRAM_K4S561632C_32M_16BIT.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include "emc_nor.h"
#include "WM.h"
#include "BSP.h"
#include "AppCommon.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "CoreFirFilter.h"

#define FILTER_TAP_COUNT	JACK_DETECT_FILTER_TAP_COUNT

uint8_t FirCounter = 0;
int16_t FilterCFs_OPT_INT[MAX_COEF_COUNT];		// Fracioned Int16 Coefficients // 
int16_t InputBuffer[MAXIMUM_FILTER_TAP_COUNT];	// Int161 ADC Samples Buffer // 
float InputBufferF[MAXIMUM_FILTER_TAP_COUNT];	//  FLOAT ADC Samples Buffer // 
float FilterCFs_OPT_FLOAT[MAX_COEF_COUNT];	 	// Original FLOAT Coefficients // 

// Set original float filter coefficients // 
void InitFilterCFs(float const *coefs_float_ptr)
{
	volatile uint8_t indx;
	if(NULL == coefs_float_ptr)
		while(STALLE_ON_ERR);
	for(indx=(FILTER_TAP_COUNT>>1) ;; indx--) {
	 	FilterCFs_OPT_FLOAT[indx] = coefs_float_ptr[indx];
		if(0 == indx) 
			break;
	}
}

#if(APP_FILTER_COEF_TYPE == INT_COEFS_USED)
////////////////////////////////////////////////////////////////////////////////
// 	Routine:	InitFilterCFs_I32											////
//	Inputs:		ObjPtr : Filter object pointer which needs I32 filters to 	////
//				be initialized 											 	////
//	Outputs:	NONE														////
//	Purpose:	Initialization of Int32 coefficients of filter from float 	////
//				ones with DEF_FRACTION_CONSTANT							 	////
// 	Ext:		This function needs the float coefficients of ObjPtr has 	////
//				been set before this function has been called 				////
////////////////////////////////////////////////////////////////////////////////
void InitFilterCFs_I32(void)
{
	uint8_t indx;
	for(indx=((FILTER_TAP_COUNT/2)+1)-1 ;; indx--) {
	 	FilterCFs_OPT_INT[indx] = (int32_t) (FilterCFs_OPT_FLOAT[indx] * DEF_FRACTION_CONSTANT);
		if(0 == indx) 
			break;
	}
}


////////////////////////////////////////////////////////////////////////////////
// 	Routine:	InitFilterCFs_I32											////
//	Inputs:		ObjPtr : Filter object pointer which needs I32 filters to 	////
//				be initialized 											 	////
//				FirstInp : The value that will be filled int input buffer	////
//	Outputs:	NONE														////
//	Purpose:	Fills all of the inputbuffer samples with new value 		////
////////////////////////////////////////////////////////////////////////////////
void FillFilterInp_I32(int16_t FirstInp)
{
	uint8_t i;
	for(i=(FILTER_TAP_COUNT)-1 ;; i--) {
		InputBuffer[i] = FirstInp;
		if(0 == i) break;
	}
}
#endif


#if(APP_FILTER_COEF_TYPE == FLOAT_COEFS_USED)
////////////////////////////////////////////////////////////////////////////////
// 	Routine:	FillFilterInp_FLOAT											////
//	Inputs:		ObjPtr : Filter object pointer which needs I32 filters to 	////
//				be initialized 											 	////
//				FirstInpF : The value that will be filled int input buffer	////
//	Outputs:	NONE														////
//	Purpose:	Fills all of the inputbuffer samples with new value 		////
////////////////////////////////////////////////////////////////////////////////
void FillFilterInp_FLOAT(float FirstInpF)
{
	uint8_t i;
	for(i=(FILTER_TAP_COUNT)-1 ;; i--) {
		InputBufferF[i] = FirstInpF;
		if(0 == i) break;
	}
}
#endif


////////////////////////////////////////////////////////////////////////////////
// 	Routine:	NewFirStep_OPT_I32											////
//	Inputs:		ObjPtr : Filter object pointer which needs I32 filters to 	////
//				be initialized 											 	////
//				NewSample : New input sample to be filtered					////
//	Outputs:	NONE														////
//	Purpose:	Calculation of input fir filtering(INT32 implementtion)		////
//	Ext:		Programmer must be carefull for saturation and overflow 	////
////////////////////////////////////////////////////////////////////////////////
#if(APP_FILTER_COEF_TYPE == INT_COEFS_USED)
// The count of multiplications are half of _NOOPT version of this function //
int32_t NewFirStep_OPT_I32(int16_t NewSample)
{
	// Create Local variables and set them with GLobal-Shared Ones // 
	{
		int32_t OutPutI32 = 0;
		uint8_t DecIndx = FirCounter;
		uint8_t IncIndx = FirCounter+1;
		uint8_t CFIndx;

		InputBuffer[FirCounter] = NewSample;
		if(IncIndx == FILTER_TAP_COUNT)
			IncIndx = 0;

		// Process the coefficients until to middle index of COEFFICIENTS //
		for(CFIndx=0 ; CFIndx != (FILTER_TAP_COUNT>>1) ; CFIndx++)
		{
			OutPutI32 += ((int32_t)FilterCFs_OPT_INT[CFIndx])*\
				(((int32_t)InputBuffer[DecIndx])+((int32_t)InputBuffer[IncIndx]));
			if(++IncIndx == FILTER_TAP_COUNT)
				IncIndx = 0;
			if(DecIndx == 0)
				DecIndx = FILTER_TAP_COUNT-1;
			else
				DecIndx--;
		}

		// Process the middle index, too //
		OutPutI32 += (((int32_t)FilterCFs_OPT_INT[CFIndx])*((int32_t)InputBuffer[DecIndx]));
		if((++FirCounter) == FILTER_TAP_COUNT)
			FirCounter=0;
		return (int32_t)((OutPutI32>>DEF_FRACTION_SHIFT));
	}
}
#endif


#if(APP_FILTER_COEF_TYPE == FLOAT_COEFS_USED)
////////////////////////////////////////////////////////////////////////////////
// 	Routine:	NewFirStep_OPT_FLOAT										////
//	Inputs:		ObjPtr : Filter object pointer which needs I32 filters to 	////
//				be initialized 											 	////
//				NewSampleF : New input sample to be filtered				////
//	Outputs:	NONE														////
//	Purpose:	Calculation of input fir filtering(float implementtion)		////
//	Ext:		There is no possiblity of SATURATION and OVERFLOW 			////
////////////////////////////////////////////////////////////////////////////////
// The count of multiplications are half of _NOOPT version of this function //
float NewFirStep_OPT_FLOAT(float NewSampleF)
{
	// Create Local variables and set them with GLobal-Shared Ones // 
	{
		float OutPut = 0;
		uint8_t DecIndx = FirCounter;
		uint8_t IncIndx = FirCounter+1;
		uint8_t CFIndx;

		InputBufferF[FirCounter] = NewSampleF;
		if(IncIndx == FILTER_TAP_COUNT)
			IncIndx = 0;

		// Process the coefficients until to middle index of COEFFICIENTS //
		for(CFIndx=0 ; CFIndx != (FILTER_TAP_COUNT>>1) ; CFIndx++)
		{
			OutPut += (FilterCFs_OPT[CFIndx])*(InputBufferF[DecIndx]+InputBufferF[IncIndx]);
			if(++IncIndx == FILTER_TAP_COUNT)
				IncIndx = 0;
			if(DecIndx == 0)
				DecIndx = FILTER_TAP_COUNT-1;
			else
				DecIndx--;
		}

		// Process the middle index, too //
		OutPut += ((FilterCFs_OPT[CFIndx])*(InputBufferF[DecIndx]));
		if((++FirCounter) == FILTER_TAP_COUNT)
			FirCounter=0;
		return OutPut;
	}
}
#endif
