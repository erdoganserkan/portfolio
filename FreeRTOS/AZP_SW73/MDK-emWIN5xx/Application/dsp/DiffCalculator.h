#ifndef DIFF_CALCULATOR_H
#define DIFF_CALCUALTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DEF_DIFF_DEPTH			(8)

int32_t DiffCalc_getdiff(int32_t NewSample);
void DiffCalc_init(void);


	#endif
