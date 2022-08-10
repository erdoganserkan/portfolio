/*
 * libCharger.c
 *
 *  Created on: Apr 29, 2014
 *      Author: serkan
 */

#include <string.h>
#include <log.h>
#include <libconfig.h>
#include <libc2charger_cfg.h>
#include "libc2charger_common.h"
#include "libc2charger.h"
#include "libi2c.h"

#define BATTERY_PERCENT_LUT_MEMBERs		15
#define LAST_CAPACITY_ARRAY_CNT				10

mj_log_type logi = LOG_INSTANCE_FAILED;
static uint8_t LastCapacity[LAST_CAPACITY_ARRAY_CNT] = {0};
volatile uint8_t LastCapIndx = 0;

static config_t libconfig_cfg;
static config_setting_t *libconfig_setting;
static uint8_t __config_init(const char *filepath);
static libc2charger_config_type libc2charger_cfg = {
	.power_switchMV = POWER_SWITCH_DEF_MV,
	.battery_sampleMV = BATTERY_SAMPLE_DEF_MV,
	.vbat_pointMV = VBAT_POINT_DEF_MV
};

static double voltage_divider;		// REAL voltage divider proportion of battery voltage //
static double cable_loss;		// REAL Cable loss of voltage on battery charger board //
static uint8_t dev_type = FALSE;

int8_t libc2charger_set_chrg_current(clibc2CHG_Charge_Current_t new_current) {

	uint8_t regval = 0;
	if(BAT_CHRG_LOW_CURRENT == new_current)
		regval |= ((0x1)<<3);
    TRY_WRITE(logi, I2C_BUS_LIBCHARGER, PCA9536D_MAP_ADR, 0x01, regval, LIBC2CHG_CURRENT_SEL_FAILED);

	INFOL(logi, "libc2charger_set_chrg_current(%s, regval:%u) SUCCESS\n", \
		(BAT_CHRG_HIGH_CURRENT == new_current) ? "HIGH CURRENT" : "LOW_CURRENT", regval);
    return 0;

LIBC2CHG_CURRENT_SEL_FAILED:
	ERRL(logi, "libc2charger_set_chrg_current(%s, regval:%u) FAILED\n", \
		(BAT_CHRG_HIGH_CURRENT == new_current) ? "HIGH CURRENT" : "LOW_CURRENT", regval);
	return -1;
}

int8_t libc2charger_get_charger_status(void)
{
	const char * ary[] =  {"CHARGING", "CHARGED", "DISCHARGE", "ERROR", "UNKNOWN"};
   uint8_t res_counts[CHG_STATE_COUNT];
   charger_state_t chg_stat = CHG_STATE_UNKNOWN;
   int16_t ret;
   volatile uint8_t indx;
   uint8_t max=0, max_indx=0xFF;
/* default value of PCA9536 is input. Only read reg 0. 
   PCB connections:
   I0:PG', I1:STAT1, I2:STAT2, I3: CH_CURRENT_SEL
*/
   memset(res_counts, 0, sizeof(res_counts));
   for(indx=0 ; indx<BAT_STATE_READ_COUNT ; indx++) {
		TRY_READ(logi, I2C_BUS_LIBCHARGER, ret, PCA9536D_MAP_ADR, 0x00, get_charger_status_FAIL);
		TRACEL(logi, "io expander ret = 0x%04X\n", ret);
		if(ret & ((uint8_t)0x01)) {
			chg_stat = CHG_STATE_DISCHARGE;        /* pg=1, st1=x, st2=x */
			res_counts[CHG_STATE_DISCHARGE]++;
		}
		else {
            if((ret & 0x07) == 0x02) {
                chg_stat = CHG_STATE_CHARGED;          /* st2:0, st1:1, pg:0 */
                res_counts[CHG_STATE_CHARGED]++;
            }
            else if((ret & 0x07) == 0x04) {
                chg_stat = CHG_STATE_CHARGING;         /* st2:1, st1:1, pg=0 */
                res_counts[CHG_STATE_CHARGING]++;
            }
            else if((ret & 0x07) == 0x06) {
                chg_stat = CHG_STATE_ERROR;            /* st2:1, st1:1, pg=0 */
                res_counts[CHG_STATE_ERROR]++;
            }
		}
		if(0) {
			chg_stat = CHG_STATE_UNKNOWN;          /* pg=x, st1=x, st2=x */
			res_counts[CHG_STATE_UNKNOWN]++;
		}
   }

   for(indx=0, max=0 ; indx<CHG_STATE_COUNT ; indx++) {
       TRACEL(logi, "res_counts[%s] = %u\n", ary[indx], res_counts[indx]);
	   if(res_counts[indx] > max) {
		   max_indx = indx;
		   max = res_counts[indx];
	   }
   }

   return max_indx;

get_charger_status_FAIL:
	ERRL(logi, "get_charger_status() FAILED\n");
	return -1;
}

int32_t libc2charger_get_battery_pack_voltage(void)
{
	int32_t  ret;
   uint16_t readADCVal = 0;
   uint32_t sumofSamples = 0;
   uint8_t  indx = 0;
   uint32_t  meanValue = 0;
   uint32_t readMilivolt = 0;
   uint32_t voltageBeforeDivider = 0;
   int32_t  BatteryPackVoltageMV = 0;
   int32_t max=-65535, min=65535;

	// first make a dummy read
	TRY_READ16(logi, I2C_BUS_LIBCHARGER,ret, ADC081C_MAP_ADR, 0x00, get_adc_value_FAIL);
	for(indx=0; indx < (0x01<<ADC_FILTER_POW) + 2; indx++) {
		TRY_READ16(logi, I2C_BUS_LIBCHARGER,ret, ADC081C_MAP_ADR, 0x00, get_adc_value_FAIL);
		// Retrieve real adc conversion result value, ADC' s memory organization is BIG-ENDIAN //
		#if(BAT_CHARGER_ADC != ADC121C027)
			readADCVal = (uint16_t)((ret & 0xF)<<4)+(ret>>12);	// 8bit data read //
		#else
			readADCVal = (uint16_t)((ret & 0xF)<<4)+(ret>>8);	// 12Bit data read //
		#endif
		DEBUGL(logi, "(%d) :: read_adc_value %u\n",indx, readADCVal);
        //printf("CORRECTED RAW READ 0x%02X\n", readADCVal);
		if(readADCVal > max)
			max = readADCVal;
		if(readADCVal < min)
			min = readADCVal;
		sumofSamples += readADCVal;
	}
	sumofSamples -= (min + max);
	DEBUGL(logi, "max(%d) ; min(%d)\n", max, min);
	meanValue = (uint8_t)(sumofSamples>>ADC_FILTER_POW);
	DEBUGL(logi, "mean_value %u\n",meanValue);

   /* convert read value to milivolts u16.0*0.8 = u16.8 truncate to u16.0 */
   readMilivolt = ((uint32_t)VA_VALUE_IN_MILIVOLTS * (uint32_t)(meanValue + ADC_CORRECTION_STEPs))>>ADC_RES_BITs;
   DEBUGL(logi, "Pin Voltage (%d) mV\n", readMilivolt);

   BatteryPackVoltageMV = ((double)readMilivolt / voltage_divider) + cable_loss;
	DEBUGL(logi, "divider(%lf), cable_loss(%lf), BatteryVoltage = %d\n", \
		voltage_divider, cable_loss, BatteryPackVoltageMV);
	return BatteryPackVoltageMV;

get_adc_value_FAIL:
	ERRL(logi, "get_charger_ADC_value() FAILED\n");
	return -1;
}

static uint8_t __config_init(const char *filepath)
{
    uint8_t state = TRUE;
    volatile uint8_t indx;
	const char *str = NULL;
	if(NULL == filepath) {
		ERRL(logi, "filepath = NULL\n");
		return FALSE;
	}
	// Init libconfig //
	config_init(&libconfig_cfg);
	if(! config_read_file(&libconfig_cfg, filepath)) {
		ERRL(logi, "config file(%s) reading FAILED, %s:%d - %s\n", filepath, config_error_file(&libconfig_cfg),
				config_error_line(&libconfig_cfg), config_error_text(&libconfig_cfg));
		config_destroy(&libconfig_cfg);
		state = FALSE;
	}
	else {
		INFOL(logi, "config file(%s) reading SUCCESS\n", filepath);
		// LOG HW revisions //
		libconfig_setting = config_lookup(&libconfig_cfg, "hw");
		if(libconfig_setting) {
			double tempDB = 0;
			// Read power switch point voltage //
			if(config_setting_lookup_float(libconfig_setting, libc2charger_config_strs[POWER_SWITCH_MV], &tempDB)) {
			    INFOL(logi, "%s : %lf\n", libc2charger_config_strs[POWER_SWITCH_MV], tempDB);
				libc2charger_cfg.power_switchMV = tempDB;
			} else {
				libc2charger_cfg.power_switchMV = POWER_SWITCH_DEF_MV;	// Default Value //
				ERRL(logi, "%s read FAILED, assumed as %lfmV\n", \
					libc2charger_config_strs[POWER_SWITCH_MV], libc2charger_cfg.power_switchMV);
			}
			// Read battery sample voltage //
			if(config_setting_lookup_float(libconfig_setting, libc2charger_config_strs[BATTERY_SAMPLE_MV], &tempDB)) {
			    INFOL(logi, "%s : %lf\n", libc2charger_config_strs[BATTERY_SAMPLE_MV], tempDB);
				libc2charger_cfg.battery_sampleMV = tempDB;
			} else {
				libc2charger_cfg.battery_sampleMV = BATTERY_SAMPLE_DEF_MV;	// Default Value //
				ERRL(logi, "%s read FAILED, assumed as %lfmV\n", \
					libc2charger_config_strs[BATTERY_SAMPLE_MV], libc2charger_cfg.battery_sampleMV);
			}
			// Read battery sample voltage //
			if(config_setting_lookup_float(libconfig_setting, libc2charger_config_strs[VBAT_POINT_MV], &tempDB)) {
			    INFOL(logi, "%s : %lf\n", libc2charger_config_strs[VBAT_POINT_MV], tempDB);
				libc2charger_cfg.vbat_pointMV = tempDB;
			} else {
				libc2charger_cfg.vbat_pointMV = VBAT_POINT_DEF_MV;	// Default Value //
				ERRL(logi, "%s read FAILED, assumed as %lfmV\n", \
					libc2charger_config_strs[VBAT_POINT_MV], libc2charger_cfg.vbat_pointMV);
			}
			// Read battery pack architecture variable "pack_2s" //
            const char *read_str;
            if(config_setting_lookup_string(libconfig_setting, libc2charger_config_strs[PACK_2S], &read_str)) {
                if((!strcmp(read_str, "yes")) || (!strcmp(read_str, "YES"))) {
                	libc2charger_cfg.pack_2s = TRUE;
                    INFOL(logi, "%s read as TRUE\n", libc2charger_config_strs[PACK_2S]);
                }
                else {
                	libc2charger_cfg.pack_2s = FALSE;
                    INFOL(logi, "%s read as FALSE\n", libc2charger_config_strs[PACK_2S]);
                }
            } else {
                ERRL(logi, "%s read FAILED, assumed as FALSE\n", libc2charger_config_strs[PACK_2S]);
            	libc2charger_cfg.pack_2s = FALSE;
            }
		}
		else {
			ERRL(logi, "%s hw group config read FAILED\n", "hw");
			state = FALSE;
		}
	}

	if(FALSE == state) {
	    ERRL(logi, "config file init/read FAILED, using default values\n");
	    if(FALSE == libc2charger_cfg.pack_2s) {
			libc2charger_cfg.power_switchMV = POWER_SWITCH_DEF_MV;  // Default Value //
			libc2charger_cfg.battery_sampleMV = BATTERY_SAMPLE_DEF_MV;  // Default Value //
			libc2charger_cfg.vbat_pointMV = VBAT_POINT_DEF_MV;  // Default Value //
	    } else {
			libc2charger_cfg.power_switchMV = POWER_SWITCH_DEF_2S_MV;  // Default Value //
			libc2charger_cfg.battery_sampleMV = BATTERY_SAMPLE_DEF_2S_MV;  // Default Value //
			libc2charger_cfg.vbat_pointMV = VBAT_POINT_DEF_2S_MV;  // Default Value //
	    }
	}
	// Calculate Required Constants //
	cable_loss = libc2charger_cfg.battery_sampleMV - libc2charger_cfg.power_switchMV;
	voltage_divider = (double)libc2charger_cfg.vbat_pointMV / (double)libc2charger_cfg.power_switchMV;
	INFOL(logi, "CALCULATED : cable_loss(%lf), voltage_divider(%lf)\n", cable_loss, voltage_divider);

	return TRUE;
}

void libc2charger_deinit(void) {
	if(LOG_INSTANCE_FAILED != logi)
		mj_log_free_instance(logi);
	logi = LOG_INSTANCE_FAILED;
}

// If everthing is ok returns 0, -1 o.w. //
int8_t libc2charger_init(const int libc2charger_log_level, uint8_t dev_t)
{
	dev_type = dev_t;
	// do dummy reading and return //
	volatile uint32_t indx;

	logi = mj_log_init_instance(libc2charger_log_level, "libc2charger");	// ENABLE LOGs
	if(LOG_INSTANCE_FAILED == logi) {
		fprintf(stderr, "Cannot init logfile_instance (libc2charger)\r\n");
		return -1;
	}
	DEBUGL(logi, "libc2chargerinit() entered\n");

	// configuration data read if there is any //
	__config_init(LIBC2CHARGER_CFG_FILE_PATH);
	if(libc2charger_cfg.pack_2s) {
		INFOL(logi, "2S PACK detected, using HALF of BATTERY TABLE VOLTAGEs\n");
		for(indx=0 ; indx<BATTERY_TABLE_COUNT ; indx++)
			battery_table[indx].bat_voltage /= 2;
	}
	// Dummy adc read for adc hw get initializaed //
	for(indx=100;indx;indx--)
		if(-1 == libc2charger_get_battery_pack_voltage()) {
			ERRL(logi, "pack voltage get FAILED\n");
			return -1;
		}
	// Clear & Fill Last Capcity Holding Array with new valid values //
	memset(LastCapacity,100,sizeof(LastCapacity));
	for(indx=0 ; indx<LAST_CAPACITY_ARRAY_CNT ; indx++)
		libc2charger_get_battery_percentage();

    // Init IO expander //
    {
        int16_t ret = 0;
        // set STAT1, STAT2, PG pins as input, IO3 as output : charge current selection //
        TRY_WRITE(logi, I2C_BUS_LIBCHARGER, PCA9536D_MAP_ADR, 0x03, 0xF7, LIBC2CHG_INIT_FAILED);
    }

	  if(0 > libc2charger_set_chrg_current(BAT_CHRG_HIGH_CURRENT)) {
		  ERRL(logi, "libc2charger_set_chrg_current(HIGH) FAILED\n");
	  }

	  return 0;


LIBC2CHG_INIT_FAILED:
    ERRL(logi, "libc2charger_init FAILED\n");
    return -1;
}

int8_t libc2charger_get_battery_percentage(void)
{
	int16_t  readPackVoltage = 0;
   int8_t   batteryPercentage = 0;
   uint16_t  indx = 0;
   readPackVoltage = libc2charger_get_battery_pack_voltage();
   DEBUGL(logi, "PackVoltage(%d) mV\n", readPackVoltage);

   if(readPackVoltage == -1) {
	   ERRL(logi, "get_battery_percentage() FAILED\n");
	   return -1;
   }
   else {
	   double intervalVoltageDistance = 0, intervalPercentDistance = 0, voltageDistance2Base    = 0, percentDistance2Base    = 0;
	   // Use battery_table for percentage determination /
	   if(battery_table[0].bat_voltage <= readPackVoltage) {
	         batteryPercentage = battery_table[0].capacity;
	   }
	   else if(battery_table[BATTERY_TABLE_COUNT-1].bat_voltage >= readPackVoltage) {
	         batteryPercentage = (int8_t)battery_table[BATTERY_TABLE_COUNT-1].capacity;
	   }
	   else {
		      for(indx = BATTERY_TABLE_COUNT-1; indx != 0; indx--)
		         if((uint16_t)readPackVoltage < battery_table[indx].bat_voltage)
		            break;
		      batteryPercentage = battery_table[indx+1].capacity;
		      DEBUGL(logi, "indx(%u), percentage(%lf)\n", indx, battery_table[indx+1].capacity);
	   }
   }

   {
	   volatile uint8_t Found;
	   volatile uint8_t indy = 0, indy_max = 0;
	   typedef struct
	   {
		   uint8_t cap;
		   uint8_t cnt;
	   } CapTableType;
	   CapTableType Temp, Caps[LAST_CAPACITY_ARRAY_CNT];
	   memset(Caps, 0xFF, sizeof(Caps));
	   // Battery Percentage Normalization & Stabilization //
	   LastCapacity[LastCapIndx++] = batteryPercentage;
	   if(LAST_CAPACITY_ARRAY_CNT == LastCapIndx)
		   LastCapIndx = 0;

	   for(indx=0 ; indx<LAST_CAPACITY_ARRAY_CNT ;indx++) {
		   DEBUGL(logi, "LastCaps[%u] = Cap(%u)\n", indx, LastCapacity[indx]);
	   }
	   for(indx=0 ; indx<LAST_CAPACITY_ARRAY_CNT ;indx++) {
		   Found = FALSE;
		   for (indy = 0; indy<indy_max ; indy++) {
			   // Search for existing record for this capacity in table //
			   if(Caps[indy].cap == LastCapacity[indx]) {
				  Found = TRUE;
				  Caps[indy].cnt++;
				  break;
			   }
		   }
		   if(FALSE == Found) {
			  Caps[indy_max].cap = LastCapacity[indx];
			  Caps[indy_max].cnt = 1;
			  indy_max++;
		   }
	   }
	   // Search Maximum Repeated value in the table //
	   for(indy=0 ; indy<indy_max ; indy++) {
		   DEBUGL(logi, "Caps[%u] = Cap(%u), Repeat(%u)\n", indy, Caps[indy].cap, Caps[indy].cnt);
	   }

	   Temp = Caps[0];
	   for(indy=1 ; indy<indy_max ; indy++) {
		   if(Caps[indy].cnt > Temp.cnt)
			   Temp = Caps[indy];
	   }
	   INFOL(logi, "Last Cap(%u), Repeat(%u)\n", Temp.cap, Temp.cnt);

	   return Temp.cap;
   }
}

