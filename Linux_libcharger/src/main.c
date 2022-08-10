/*
 * main.c
 *
 *  Created on: Jan 21, 2014
 *      Author: gurkan
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <client_common.h>
#include <ctech_log.h>
#include "libc2charger.h"

extern mj_log_type logi;

int main(void)
{
	uint8_t deb = 0;
	char chars[3] = {'a', 's', 'd'};
	static uint8_t enteredVal = 0;
	static int8_t  readChargerState = 0;
	static int16_t readADCValue = 0;
	static int8_t  readPercentage = 0;
	volatile uint32_t indx = 0;
	time_t first, last;

	libc2charger_init(LOG_DBG, "logfile");

    printf("\n\n\n\n ####### LOOP ENTERED ####### \n\n\n\n");
	while(1) 
   {
#if(1)
	    printf("enter :\n "
	    		"a for charger state\n"
	    		"s for adc value\n"
	    		"d for battery percentage\n"
	    		"e for LOW CURRENT charging\n"
	    		"f for HIGH CURRENT charging\n");
		enteredVal = getchar();
		printf("read_value %x\n",enteredVal);
#else
		deb = rand()%3;
		enteredVal = chars[deb];
#endif

      if(enteredVal == 'a')
      {
         readChargerState = libc2charger_get_charger_status();
         if(readChargerState == -1)
            printf("charger state can not be read!!!\n");
         else if(readChargerState==CHG_STATE_CHARGING)
            printf("charger state is CHG_STATE_CHARGING\n");
         else if(readChargerState==CHG_STATE_CHARGED)
            printf("charger state is CHG_STATE_CHARGED\n");
         else if(readChargerState==CHG_STATE_DISCHARGE)
            printf("charger state is CHG_STATE_DISCHARGE\n");
         else if(readChargerState==CHG_STATE_ERROR)
            printf("charger state is CHG_STATE_ERROR\n");
         else
            printf("charger state is CHG_STATE_UNKNOWN\n");
      }
      else if(enteredVal == 's')
      {
    	  int32_t battery_voltage = libc2charger_get_battery_pack_voltage();
    	  printf("pack_voltage = %d\n", battery_voltage);
      }
      else if(enteredVal == 'd')
      {
         readPercentage = libc2charger_get_battery_percentage();
         if(readPercentage == -1)
            printf("Battery percentage can not be read!!!!\n");
         else
            printf("Battery percentage = %d\n",readPercentage);
      }
      else if(enteredVal == 'e') {
    	  libc2charger_set_chrg_current(BAT_CHRG_LOW_CURRENT);
      }
      else if(enteredVal == 'f') {
    	  libc2charger_set_chrg_current(BAT_CHRG_HIGH_CURRENT);
      }
      else
			break;
      usleep(500000U);
      //getchar();
#if(0)
      sleep(1);	// Sample battery at every minutes //
      int8_t percentage = libc2charger_get_battery_percentage();
      fprintf(stdout, "Time:	%d	min,	percentage:	%d\n", (time(NULL)-first)/60, percentage);
#endif
	}
	//getchar();
	usleep(250000);
	return 0;
}
