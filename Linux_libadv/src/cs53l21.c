/*
 * CS53L21.c
 *
 *  Created on: Mar 21, 2014
 *      Author: serkan
 */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <log.h>
#include "i2cbusses.h"
#include "util.h"
#include <libi2c.h>
#include "cs53l21.h"

#include "../inc/libadv.h"
#include "../inc/libadv_common.h"

extern mj_log_type libADV_log_instance;

uint8_t analog_sound_init(uint16_t waitMS, uint16_t MaxTryCnt, uint8_t audio_mode)
{
	volatile uint16_t indx;
	int16_t ret;

	for(indx=0 ; indx<MaxTryCnt ; indx++) {
		// Dummy Read(Chip version reading) for access testing //
		TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,ret, CS53L21_I2C_ADR, 0x01, CS53L21_access_FAILED);		// Read value of register (IOMAP, adr:0x01) //
		DEBUGL(libADV_log_instance,"AudioADC Version = 0x%02X\n", ret);
		// Audio ADC (I2C Write)
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x03, 0x4E, CS53L21_access_FAILED);	// Sample Rate (48KHz)
		#if(ACTIVE_SOUND_INPUT == SINGLE_SOUND_INPUT)
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x04, 0x46, CS53L21_access_FAILED);	// I2S Output, Master Role, DIGMIX enabled
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x07, 0x10, CS53L21_access_FAILED);	// MUTE OFF
			DEBUGL(libADV_log_instance,"SINGLE CHANNEL CONNECTION AS INPUT is ACTIVE\n");
		#elif(ACTIVE_SOUND_INPUT == DIFFERENTIAL_SOUND_INPUT)
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x04, 0x45, CS53L21_access_FAILED);	// I2S Output, Master Role, DIGMIX enabled
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x07, 0x10, CS53L21_access_FAILED);	// MUTE OFF
			DEBUGL(libADV_log_instance,"DIFFERENTIAL CHANNEL CONNECTION AS INPUT is ACTIVE\n");
		#else
			#error "UNEXPECTED AUDIO CONNECTION TYPE DEFINITION"
		#endif
		// Set SPE_ENABLE bit to "1" //
		TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,ret, CS53L21_I2C_ADR, 0x09, CS53L21_access_FAILED);
		DEBUGL(libADV_log_instance, "CS53L21(0x09) READ => 0x%02X\n", ret);
		ret |= (0x1<<6);
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x09, ret, CS53L21_access_FAILED);

		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0A, 0x1A, CS53L21_access_FAILED);	// Lch PGA Volume -3db
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0B, 0x1A, CS53L21_access_FAILED);	// Rch PGA Volume -3db
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0C, 0x00, CS53L21_access_FAILED);	// Lch Attenuator 0db(default)
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0D, 0x00, CS53L21_access_FAILED);	// Rch Attenuator 0db(default)

		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0E, 0x00, CS53L21_access_FAILED);	// Mixer Output CH1: Mute Off + 0dB gain
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0F, 0x00, CS53L21_access_FAILED);	// Mixer Output CH2: Mute Off + 0dB gain

		#ifndef AS_OUTPUT_FIXED_OUTPUT
		switch(audio_mode) {
			case ANALOG_AUDIO_DUAL_MONO:
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x18, 0x00, CS53L21_access_FAILED);
				DEBUGL(libADV_log_instance, "ANALOG_AUDIO_DUAL_MONO Selcted\n");
				break;
			case ANALOG_AUDIO_MONO_LEFT:
				DEBUGL(libADV_log_instance, "ANALOG_AUDIO_MONO_LEFT Selcted\n");
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x18, 0x03, CS53L21_access_FAILED);
				break;
			case ANALOG_AUDIO_MONO_RIGHT:
				DEBUGL(libADV_log_instance, "ANALOG_AUDIO_MONO_RIGHT Selcted\n");
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x18, 0x0C, CS53L21_access_FAILED);
				break;
			case ANALOG_AUDIO_STEREO:
				DEBUGL(libADV_log_instance, "ANALOG_AUDIO_STEREO Selcted\n");
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x18, 0x0A, CS53L21_access_FAILED);
				break;
			default:
				ERRL(libADV_log_instance, "UNEXPECTED audio_mode(%u)\n", audio_mode);
				break;
		}
		#else
			#if(AS_OUTPUT_FIXED_OUTPUT == ANALOG_AUDIO_DUAL_MONO)
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x18, 0x00, CS53L21_access_FAILED);
				DEBUGL(libADV_log_instance, "ANALOG_AUDIO_DUAL_MONO Selected\n");
			#else
				#error "WHAT TO DO???"
			#endif
		#endif

		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x1C, 0x00, CS53L21_access_FAILED);	// ALC Enable  (Disable)
		//TRY_CMASK(CS53L21_I2C_ADR, 0x02, ((0x1)<<0), CS53L21_init_FAILED);	// Power ON Chip //
		break;	// Everything is OK, break the loop //

		CS53L21_access_FAILED:
			usleep(waitMS*1000UL);	// Wait and give a try more //
	}

	if(MaxTryCnt == indx) {
		ERRL(libADV_log_instance,"CS53L21_init() FAILED\n\n");
		return FALSE;
	}
	else {
		DEBUGL(libADV_log_instance,"CS53L21_init() SUCCESS\n\n");
		return TRUE;
	}
}

uint8_t cs53l21_inc_vol(uint8_t inc_dB)
{
	// Set PGA gain registers //
	if(INC_VOL_DB_MAX < inc_dB)
		inc_dB = INC_VOL_DB_MAX;	// Truncate to maximum value //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0A, inc_dB*2, cs53l21_inc_vol_FAILED);	// Lch PGA Volume +0db
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0B, inc_dB*2, cs53l21_inc_vol_FAILED);	// Rch PGA Volume +0db
	return TRUE;

cs53l21_inc_vol_FAILED:
	ERRL(libADV_log_instance,"%s(%u dB)-> FAILED\n", inc_dB, __FUNCTION__);
	return FALSE;
}

uint8_t cs53l21_dec_vol(int8_t dec_dB)
{
	if(DEC_VOL_DB_MIN > dec_dB)
		dec_dB = DEC_VOL_DB_MIN;	// Truncate to minimum value //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0C, dec_dB, cs53l21_dec_vol_FAILED);
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CS53L21_I2C_ADR, 0x0D, dec_dB, cs53l21_dec_vol_FAILED);
	return TRUE;

cs53l21_dec_vol_FAILED:
	ERRL(libADV_log_instance,"%s(%d dB)-> FAILED\n", dec_dB, __FUNCTION__);
	return FALSE;
}
