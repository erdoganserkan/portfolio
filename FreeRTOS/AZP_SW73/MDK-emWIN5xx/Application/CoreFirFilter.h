#ifndef COREFIRFILTER_H
#define COREFIRFILTER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define	MAXIMUM_FILTER_TAP_COUNT	(51)
#define MAX_COEF_COUNT				((MAXIMUM_FILTER_TAP_COUNT/2) + 1)
#define DEF_FRACTION_SHIFT			(14)
#define DEF_FRACTION_CONSTANT 		(0x00000001<<DEF_FRACTION_SHIFT)

// APP FILTER TYPES //
#define INT_COEFS_USED				0x2
#define FLOAT_COEFS_USED			0x3
#define APP_FILTER_COEF_TYPE		INT_COEFS_USED

typedef struct
{
	uint8_t FirCounter;
	uint8_t FilterTapCount;
	void *FilterCFs_OPT;
	void *InputBuffer;
} CoreFirFilterType;

extern void InitFilterCFs(float const *coefs_float_ptr);
#if(APP_FILTER_COEF_TYPE == INT_COEFS_USED)
	extern void FillFilterInp_I32(int16_t FirstInp);
	extern int32_t NewFirStep_OPT_I32(int16_t NewSample);
	extern void InitFilterCFs_I32();
#elif(APP_FILTER_COEF_TYPE == FLOAT_COEFS_USED)
	extern void FillFilterInp_FLOAT(float FirstInpF);
	extern float NewFirStep_OPT_FLOAT(float NewSample);
#endif

#endif /* COREFIRFILTER_H_ */
