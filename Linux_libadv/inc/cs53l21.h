/*
 * cs53l21.h
 *
 *  Created on: Mar 21, 2014
 *      Author: serkan
 */

#ifndef CS53L21_H_
#define CS53L21_H_

#include "libadv.h"

//#define CS53L21_I2C_ADR			((0x94 | 0x02)>>1)
#define CS53L21_I2C_ADR			(0x94>>1)

#define INC_VOL_DB_MAX		12
#define DEC_VOL_DB_MIN		(-96)

#define SINGLE_SOUND_INPUT			0x0	/* v05 version configuration, development board configuration */
#define DIFFERENTIAL_SOUND_INPUT	0x1	/* real product option */
#define ACTIVE_SOUND_INPUT			SINGLE_SOUND_INPUT

// MONO/STEREO selection implementation is DONE in encoder(fujitsu) configuration //
	// Analog audio output (cs53l21) is fixed to DUAL_MONO,
	// stereo/mono selection action will be done by ENCODER configuration
#define AS_OUTPUT_FIXED_OUTPUT		ANALOG_AUDIO_DUAL_MONO

extern uint8_t cs53l21_inc_vol(uint8_t inc_dB);
extern uint8_t cs53l21_dec_vol(int8_t dec_dB);

#endif /* CS53L21_H_ */
