#ifndef ADC_READ_H
#define ADC_READ_H

#include <stdint.h>
#include "Common.h"

#define ADC101S021_DEF			(0x0)
#define STM32F103_ADC_DEF		(0x1)
#define STM32F050_ADC_DEF		(0x2)
#define ACTIVE_ADC					STM32F050_ADC_DEF

#if(ACTIVE_ADC == ADC101S021_DEF)
	#define ADC_RES		(10)
#elif(ACTIVE_ADC == STM32F103_ADC_DEF)
	#define ADC_RES		(12)
	#define ADC_MAX_VOLTAGE_MV	(3290)
#elif(ACTIVE_ADC == STM32F050_ADC_DEF)
	#define ADC_RES		(12)
	#define ADC1_DR_Address     0x40012440
	#define ADC_MAX_VOLTAGE_MV	(3300)
#else
	#error "UNEXPECTED ADC DEFINITION"
#endif

// Hardware Dependent Resistor Division Values //
#define ADC_HIGH_RES_OHM		(27000U)
#define ADC_LOW_RES_OHM			(4700U)
#define SERIAL_CELL_COUNT		(4)
#define CELL_LEVEL_MAX_MV		(4200U)
#define BATTERY_MAX_VAL_MV	(SERIAL_CELL_COUNT * CELL_LEVEL_MAX_MV)

#define USE_MOVING_AVERAGE	TRUE
#define MOVING_AVG_DEPTH		64

#define CABLE_LOSS_DECREASE_MV					(70)
#if(TRUE == BERK_VOLTAGE_DROP_COMPENSATION)
	#warning "BERK ELEKTRONIK BATTERY LEVEL PATCH ACTIVE"
#else
	#warning "BERK ELEKTRONIK BATTERY LEVEL PATCH DISABLED"
#endif

extern void Init_ADC(void);							/* Called from init() functions */
extern void update_adc(void);
extern __IO uint8_t BatteryCapacity;
extern __IO int8_t BatTemp;

#endif
