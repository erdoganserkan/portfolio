#ifndef BAT_VALUES_H
#define BAT_VALUES_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define BATTERY_TABLE_COUNT			(301)

typedef struct {
	double capacity;
	uint32_t min_elapsed;
	uint32_t bat_voltage;
} battery_table_type;

extern uint8_t BSP_get_percentage(uint32_t adc_raw_val);
extern uint32_t BAT_RAWADC_2_PACK_MV(uint32_t adc_raw_val);

#endif
