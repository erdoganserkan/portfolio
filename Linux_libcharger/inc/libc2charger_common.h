#ifndef LIBCHARGER_COMMON_H
#define LIBCHARGER_COMMON_H

#include <stdint.h>

#ifndef FALSE
	#define FALSE		(0)
#endif
#ifndef TRUE
	#define TRUE		(1)
#endif

#define I2C_BUS_LIBCHARGER			(2)

// ADC DEFINITIONs //
#define ADC081C027	0x0     // ADR pin exist & 8 bit //
#define ADC081C021	0x1     // ADR pin NOT exist & 8 bit //
#define ADC121C027	0x2     // ADR pin exit & 12 bit //
#define BAT_CHARGER_ADC ADC081C027		// Depends on HARDWARE, be carefull //

#define VBAT_POINT_DEF_MV		(2.942)
#define BATTERY_SAMPLE_DEF_MV	(14.68)
#define POWER_SWITCH_DEF_MV		(14.65)

#define VBAT_POINT_DEF_2S_MV		(1.088)
#define BATTERY_SAMPLE_DEF_2S_MV	(5.683)
#define POWER_SWITCH_DEF_2S_MV		(5.450)

#define PCA9536D_MAP_ADR				(0x82>>1)
#if(BAT_CHARGER_ADC	== ADC081C027)
	// Real Design Value //
	#define ADC081C_MAP_ADR				(0xA4>>1)
	#define ADC_RES_BITs				8
	#define ADC_CORRECTION_STEPs		1
#elif(BAT_CHARGER_ADC == ADC121C027)
	// Ctech Labrotary, Own production versions //
	#define ADC081C_MAP_ADR			(0xA4>>1)
	#define ADC_RES_BITs			12
	#define ADC_CORRECTION_STEPs		5
#elif(BAT_CHARGER_ADC == ADC081C021)
	// Ctech Labrotary, Own production versions //
	#define ADC081C_MAP_ADR			(0xA8>>1)
	#define ADC_RES_BITs			8
	#define ADC_CORRECTION_STEPs	5
#else
	#error "UNDEFINED BATTERY CHARGER CARD ADC"
#endif
/* Actual filter width is 2^ADC_FILTER_POW */
#define ADC_FILTER_POW				2
#define BAT_STATE_READ_COUNT		5

/*parameters for voltage divider before ADC
Vsys____R1___VADC___R2___GND
Vsys = Vadc*(1+R1/R2)
Enter R1 and R2 Values in ohms */
//#define R1_VALUE     91000
//#define R2_VALUE     22000
//#define SCALE_CONSTANT     (256 + ((256*R1_VALUE))/R2_VALUE)

#define VA_VALUE_IN_MILIVOLTS     	(4096)	/* Actually it should be 5000mV but bevause regulator setting  */
#define BATTERY_TABLE_COUNT			(301)

typedef struct {
	double power_switchMV;	// Power ON/OFF switch voltage //
	double battery_sampleMV;	// Battery connector voltage //
	double vbat_pointMV;		// J5 battery connector "BAT" pin voltage //
	uint8_t pack_2s;			// YES/yes if 2S4P(8.4V) pack is used, NO/no if 4S2P(16.8V) pack is used //
} libc2charger_config_type;

typedef struct {
	double capacity;
	uint32_t min_elapsed;
	uint32_t bat_voltage;
} battery_table_type;

extern battery_table_type battery_table[];

#endif	/* End of LIBCHARGER_COMMON_H */
