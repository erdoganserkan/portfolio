/*
 * adv7181c.c
 *
 *  Created on: Jan 17, 2014
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
#include "adv7181c.h"

#include "../inc/libadv.h"
#include "../inc/libadv_common.h"

extern mj_log_type libADV_log_instance;
static sADV7181C_VidInfo ADV7181C_vidinfo;


#define ADV7181C_RESET(fail_label) { \
	TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, ADV7181C_ADR, 0x0F, 0x80, IGNORE_ERR);	/* Set RESET for all I2C registers */\
IGNORE_ERR:\
	usleep(500000);	/* Wait for RESET */\
	TRY_CMASK(libADV_log_instance, I2C_BUS_LIBADV, ADV7181C_ADR, 0x0F, 0x80, IGNORE_ERR2);	/* Release RESET */\
IGNORE_ERR2:\
	do {} while(0);\
}

int8_t analog_video_init(uint8_t mode)
{
	int16_t readval = 0;
	// Be sure that Chip is NOT @ RESET state //
	ADV7181C_RESET(ADV7181C_init_FAIL);

	DEBUGL(libADV_log_instance, "ADC7181C mode(%u)\n", mode);
	// Do parameter specific configuration //
	if(ADV7181C_AUTO_RESOLUTION == mode)
		mode = ADV7181C_CVBS_PAL;
	switch(mode) {
		case ADV7181C_CVBS_PAL: // :AUTODETECT CVBS IN NTSC/PAL/SECAM, 8-Bit 422 encoder:
		case ADV7181C_CVBS_NTSC:
			INFOL(libADV_log_instance, "ADV7181C_CVBS selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, ADV7181C_ADR, 0x00, 0x10, ADV7181C_init_FAIL); // CVBS IN, PAL BGHID (PAL autodetect için 0x00)
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x69, ADV7181C_init_FAIL);	// SDM_SEL[1:0] = 00
			readval &= (~0x3);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, ADV7181C_ADR, 0x69, readval, ADV7181C_init_FAIL);
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x05, ADV7181C_init_FAIL);	// PRIM_MODE[3:0] = 0000
			readval &= (~((0x1<<4)-1));
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, readval, ADV7181C_init_FAIL);
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x06, ADV7181C_init_FAIL);	// PRIM_MODE[3:0] = 0000
			readval &= 0Xf0;
			readval |= 0X02;
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, readval, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0x80, ADV7181C_init_FAIL); // Enable maual input muxing
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0X01, ADV7181C_init_FAIL);	// ADC0 <-> AIN1
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0xED, ADV7181C_init_FAIL);	// 0XED[2] -> 0
			readval &= (~(0X1<<2));
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xED, readval, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x17, ADV7181C_init_FAIL); // Set Latch Clock & power down ADC 1 & ADC2 & ADC3
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x7B, ADV7181C_init_FAIL);	// AV codes insertion TO BOTH LİNES //
			readval |= ((0X1<<1) | (0X1<<4));
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7B, readval, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x03, 0x0, ADV7181C_init_FAIL); // 0x00:10 bit Mode //
			//TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x03, 0x0C, ADV7181C_init_FAIL); // 0x0C:8 bit Mode //
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x04, 0x77, ADV7181C_init_FAIL); // Enable SFL
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x17, 0x41, ADV7181C_init_FAIL); // select SH1
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x31, 0x02, ADV7181C_init_FAIL); // Clears NEWAV_MODE, SAV/EAV  to suit ADV video encoders
			//TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x17, ADV7181C_init_FAIL); // Set Latch Clock & power down ADC 1 & ADC2 & ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3D, 0xA2, ADV7181C_init_FAIL); // MWE Enable Manual Window, Colour Kill Threshold to 2
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3E, 0x6A, ADV7181C_init_FAIL); // BLM optimisation
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3F, 0xA0, ADV7181C_init_FAIL); // BGB
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x0B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF3, 0x01, ADV7181C_init_FAIL); // Enable Anti Alias Filter on ADC0
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF9, 0x03, ADV7181C_init_FAIL); // Set max v lock range
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7F, 0xFF, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x81, 0x30, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x90, 0xC9, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x91, 0x40, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x92, 0x3C, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x93, 0xCA, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x94, 0xD5, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB1, 0xFF, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB6, 0x08, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x9A, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xCF, 0x50, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD0, 0x4E, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD1, 0xB9, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD6, 0xDD, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD7, 0xE2, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xE5, 0x51, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
		break;
		case ADV7181C_SDP_YC:	// :AUTODETECT Y/C IN NTSC/PAL/SECAM, 8 Bit 422 Encoder:
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x00, 0x80, ADV7181C_init_FAIL); // CVBS IN, PAL BGHID (PAL autodetect için 0x00)
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x69, ADV7181C_init_FAIL);	// SDM_SEL[1:0] = 00
			readval &= (~0x3);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x69, readval, ADV7181C_init_FAIL);
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x05, ADV7181C_init_FAIL);	// PRIM_MODE[3:0] = 0000
			readval &= (~((0x1<<4)-1));
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, readval, ADV7181C_init_FAIL);
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x06, ADV7181C_init_FAIL);	// PRIM_MODE[3:0] = 0000
			readval &= 0Xf0;
			readval |= 0X02;
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, readval, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0x80, ADV7181C_init_FAIL); // Enable maual input muxing
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0X01, ADV7181C_init_FAIL);	// ADC0 <-> AIN1
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0xED, ADV7181C_init_FAIL);	// 0XED[2] -> 0
			readval &= (~(0X1<<2));
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xED, readval, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x17, ADV7181C_init_FAIL); // Set Latch Clock & power down ADC 1 & ADC2 & ADC3
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x7B, ADV7181C_init_FAIL);	// AV codes insertion TO BOTH LİNES //
			readval |= ((0X1<<1) | (0X1<<4));
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7B, readval, ADV7181C_init_FAIL);
			INFOL(libADV_log_instance, "ADV7181C_SDP_YC selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x03, 0x0C, ADV7181C_init_FAIL); // 8 Bit Mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x04, 0x57, ADV7181C_init_FAIL); // Enable SFL
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x31, 0x02, ADV7181C_init_FAIL); // Clears NEWAV_MODE, SAV/EAV  to suit ADV video encoders
			//TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x13, ADV7181C_init_FAIL); // Set Latch Clock & turn off ADC2 & ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3D, 0xA2, ADV7181C_init_FAIL); // MWE Enable Manual Window, Colour Kill Threshold to 2
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3E, 0x6A, ADV7181C_init_FAIL); // BLM optimisation
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3F, 0xA0, ADV7181C_init_FAIL); // BGB
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x0B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x69, 0x03, ADV7181C_init_FAIL); // Sets SDM_SEL to 03 for YC/CVBS Auto
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF3, 0x03, ADV7181C_init_FAIL); // Enable Anti Alias Filters on ADC0 & ADC1
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF9, 0x03, ADV7181C_init_FAIL); // Set max v lock range
			//TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0x80, ADV7181C_init_FAIL); // Enable maual input muxing
			//TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0xC2, ADV7181C_init_FAIL); // Enable manual input muxing
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7F, 0xFF, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x81, 0x30, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x90, 0xC9, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x91, 0x40, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x92, 0x3C, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x93, 0xCA, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x94, 0xD5, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB1, 0xFF, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB6, 0x08, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x9A, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xCF, 0x50, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD0, 0x4E, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD1, 0xB9, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD6, 0xDD, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD7, 0xE2, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xE5, 0x51, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			break;
		case ADV7181C_SDP_YPrPb:	// :AUTODETECT YPbPr In, 8 Bit 422 Encoder:
			INFOL(libADV_log_instance, "ADV7181C_SDP_YPrPb selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x8D, 0x83, ADV7181C_init_FAIL); // Fix for Connect/Disconnect Problem
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x00, 0x09, ADV7181C_init_FAIL); // YPrPb
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x03, 0x0C, ADV7181C_init_FAIL); // 8 Bit Mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x27, 0x18, ADV7181C_init_FAIL); // YC Delay Correction
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x31, 0x02, ADV7181C_init_FAIL); // Clears NEWAV_MODE, SAV/EAV  to suit ADV video encoders
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x11, ADV7181C_init_FAIL); // set latch clock settings to 001b, Power Down ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3D, 0xA2, ADV7181C_init_FAIL); // MWE Enable Manual Window
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3E, 0x6A, ADV7181C_init_FAIL); // BLM optimisation
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3F, 0xA0, ADV7181C_init_FAIL); // ADI Recommended
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x0B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB4, 0xF9, ADV7181C_init_FAIL); // Fix for Connect/Disconnect Problem
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB5, 0x00, ADV7181C_init_FAIL); // Fix for Connect/Disconnect Problem
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0x46, ADV7181C_init_FAIL); // ADC1 to Ain4, ADC0 to Ain6,
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0xB5, ADV7181C_init_FAIL); // ADC2 to Ain5 and enables manual override of mux
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF3, 0x07, ADV7181C_init_FAIL); // Enable Anti Alias Filters on ADC 0,1,2
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF9, 0x03, ADV7181C_init_FAIL); // Set max v lock range
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7F, 0xFF, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x81, 0x30, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x90, 0xC9, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x91, 0x40, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x92, 0x3C, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x93, 0xCA, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x94, 0xD5, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7E, 0x73, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB1, 0xFF, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB6, 0x08, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x9A, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xCF, 0x50, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD0, 0x4E, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD1, 0xB9, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD6, 0xDD, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xE5, 0x51, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			break;
		case ADV7181C_CP_YPrPb_525i:	// :525I YPrPb In 10Bit 422 Encoder:
		case ADV7181C_CP_YPrPb_625i : // :625I YPrPb In 10Bit 422 Encoder:
			if(ADV7181C_CP_YPrPb_625i == mode) {
				INFOL(libADV_log_instance, "ADV7181C_CP_YPrPb_625i selected\n");
			} else {
				INFOL(libADV_log_instance, "ADV7181C_CP_YPrPb_525i selected\n");
			}
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x00, ADV7181C_init_FAIL); // 05 00 ; Prim_Mode =000b for SD-M
			if(ADV7181C_CP_YPrPb_625i == mode) {
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x0B, ADV7181C_init_FAIL); // 06 0B ; VID_STD=1010b for SD 4x1 525i
			} else {
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x0A, ADV7181C_init_FAIL); // 06 0A ; VID_STD=1010b for SD 4x1 525i
			}
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // 1D 47 ; Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x11, ADV7181C_init_FAIL); // 3A 11 ; Set Latch Clock 01b, Power Down ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // 3B 81 ; Enable internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3C, 0x52, ADV7181C_init_FAIL); // 3C 52 ; PLL_QPUMP to 010b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x6B, 0xC1, ADV7181C_init_FAIL); // 6B C1 ; Select 422 10 bit YPrPb out from CP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7B, 0x06, ADV7181C_init_FAIL); // 7B 06 ; clears the bits CP_DUP_AV and AV_Blank_EN
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x85, 0x19, ADV7181C_init_FAIL); // 85 19 ; Turn off SSPD and force SOY. For Eval Board.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x1B, ADV7181C_init_FAIL); // 86 1B ; Enable stdi_line_count_mode
			if(ADV7181C_CP_YPrPb_625i == mode)
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x8A, 0xB0, ADV7181C_init_FAIL); // 8A B0 ; Manual VCO_RANGE=01
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x8F, 0x77, ADV7181C_init_FAIL); // 8F 77 ; FR_LL to 1820 & Enable 28.63MHz LLC
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x90, 0x1C, ADV7181C_init_FAIL); // 90 1C ; FR_LL to 1820
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xBF, 0x06, ADV7181C_init_FAIL); // BF 06 ; Blue Screen Free Run Colour
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x40, ADV7181C_init_FAIL); // C0 40 ; default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC1, 0xF0, ADV7181C_init_FAIL); // C1 F0 ; default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC2, 0x80, ADV7181C_init_FAIL); // C2 80 ; Default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC5, 0x01, ADV7181C_init_FAIL); // C5 01 ; CP_CLAMP_AVG_FACTOR[1-0] = 00b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC9, 0x0C, ADV7181C_init_FAIL); // C9 0C ; Enable DDR Mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF3, 0x07, ADV7181C_init_FAIL); // F3 07 ; Enable Anti Alias Filters on ADC 0,1,2
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // 0E 80 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // 52 46 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // 54 00 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // F6 3B ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // 0E 00 ; ADI Recommended Setting
			break;
		case ADV7181C_FAST_BLANK:	// :PAL 10 Bit 422 Encoder:
			INFOL(libADV_log_instance, "ADV7181C_FAST_BLANK selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x03, 0x00, ADV7181C_init_FAIL); // 03 00 ; 10 Bit Mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x17, 0x41, ADV7181C_init_FAIL); // 17 41 ; select SH1
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x00, ADV7181C_init_FAIL); // 06 00 ; 27MHz sample rate
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // 1D 47 ; Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x31, 0x02, ADV7181C_init_FAIL); // 31 02 ; Clears NEWAV_MODE, SAV/EAV  to suit ADV video encoders
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x10, ADV7181C_init_FAIL); // 3A 10 ; Set Latch Clock & Power UP ADC 1 & ADC2 & ADC3& ADC4
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x61, ADV7181C_init_FAIL); // 3B 61 ; Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3D, 0xA2, ADV7181C_init_FAIL); // 3D A2 ; MWE Enable Manual Window, Colour Kill Threshold to 2
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3E, 0x6A, ADV7181C_init_FAIL); // 3E 6A ; BLM optimisation
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3F, 0xA0, ADV7181C_init_FAIL); // 3F A0 ; BGB
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x67, 0x01, ADV7181C_init_FAIL); // 67 01 ; Format 422
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x73, 0xD0, ADV7181C_init_FAIL); // 73 D0 ; Manual Gain Channels A,B,C
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x74, 0x04, ADV7181C_init_FAIL); // 74 04 ; Manual Gain Channels A,B,C
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x75, 0x01, ADV7181C_init_FAIL); // 75 01 ; Manual Gain Channels A,B,C
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x76, 0x00, ADV7181C_init_FAIL); // 76 00 ; Manual Gain Channels A,B,C
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x77, 0x04, ADV7181C_init_FAIL); // 77 04 ; Manual Offsets A to 64d & B,C to 512
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x78, 0x08, ADV7181C_init_FAIL); // 78 08 ; Manual Offsets A to 64d & B,C to 512
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x79, 0x02, ADV7181C_init_FAIL); // 79 02 ; Manual Offsets A to 64d & B,C to 512
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7A, 0x00, ADV7181C_init_FAIL); // 7A 00 ; Manual Offsets A to 64d & B,C to 512
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x0B, ADV7181C_init_FAIL); // 86 0B ; Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC5, 0x00, ADV7181C_init_FAIL); // C5 00 ; Clamp Mode 0 for FB hc based
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xED, 0x12, ADV7181C_init_FAIL); // ED 12 ; FB_CAP_RES,Enable Dynamic Fast Blank Mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF3, 0x0F, ADV7181C_init_FAIL); // F3 0F ; Enable Anti Alias Filter on all ADCs
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // 0E 80 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x49, 0x01, ADV7181C_init_FAIL); // 49 01 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // 52 46 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // 54 00 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7F, 0xFF, ADV7181C_init_FAIL); // 7F FF ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x81, 0x30, ADV7181C_init_FAIL); // 81 30 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x90, 0xC9, ADV7181C_init_FAIL); // 90 C9 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x91, 0x40, ADV7181C_init_FAIL); // 91 40 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x92, 0x3C, ADV7181C_init_FAIL); // 92 3C ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x93, 0xCA, ADV7181C_init_FAIL); // 93 CA ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x94, 0xD5, ADV7181C_init_FAIL); // 94 D5 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB6, 0x08, ADV7181C_init_FAIL); // B6 08 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x9A, ADV7181C_init_FAIL); // C0 9A ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xCF, 0x50, ADV7181C_init_FAIL); // CF 50 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD0, 0x4E, ADV7181C_init_FAIL); // D0 4E ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD1, 0xB9, ADV7181C_init_FAIL); // D1 B9 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD6, 0xDD, ADV7181C_init_FAIL); // D6 DD ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xD7, 0xE2, ADV7181C_init_FAIL); // D7 E2 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xE5, 0x51, ADV7181C_init_FAIL); // E5 51 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // F6 3B ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // 0E 00 ; ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0xF5, ADV7181C_init_FAIL); // C4 F5 ; Manual Muxing
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0x62, ADV7181C_init_FAIL); // C3 62 ; Manual Muxing
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF3, 0x4F, ADV7181C_init_FAIL); // F3 4F ; Manual Muxing
			break;
		case ADV7181C_525p_60Hz_20bit_422_SAVEAV:	// :525p/60Hz YPrPb In 20Bit 422 EAV/SAV Encoder:
		case ADV7181C_625p_50Hz_20bit_422_SAVEAV:	// :625p/50Hz YPrPb In 20Bit 422 EAV/SAV Encoder:
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x01, ADV7181C_init_FAIL); // PRIM_MODE = 001b COMP
			if(ADV7181C_525p_60Hz_20bit_422_SAVEAV == mode) {
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x06, ADV7181C_init_FAIL); // VID_STD for 525P 2x1, NTSC derived
				INFOL(libADV_log_instance, "ADV7181C_525p_60Hz_20bit_422_SAVEAV selected\n");
			} else {
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x07, ADV7181C_init_FAIL); // VID_STD for 625P 2x1, PAL derived
				INFOL(libADV_log_instance, "ADV7181C_625p_50Hz_20bit_422_SAVEAV selected\n");
			}
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0x46, ADV7181C_init_FAIL); // ADC1 to Ain4, ADC0 to Ain6,
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0xB5, ADV7181C_init_FAIL); // ADC2 , 0xo Ain5 and enables manual override of mux
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28.63636MHz crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x11, ADV7181C_init_FAIL); // Set Latch Clock 01b. Power down ADC3.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3C, 0x53, ADV7181C_init_FAIL); // PLL QPUMP to 011b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x6B, 0x81, ADV7181C_init_FAIL); // 422 20bit out
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC9, 0x00, ADV7181C_init_FAIL); // SDR mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x73, 0xCF, ADV7181C_init_FAIL); // Enable Manual Gain and set CH_A gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x74, 0xA3, ADV7181C_init_FAIL); // Set CH_A and CH_B Gain - 0FAh
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x75, 0xE8, ADV7181C_init_FAIL); // Set CH_B and CH_C Gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x76, 0xFA, ADV7181C_init_FAIL); // Set CH_C Gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7B, 0x1E, ADV7181C_init_FAIL); // Enable EAV and SAV Codes.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x85, 0x19, ADV7181C_init_FAIL); // Turn off SSPD and force SOY
			/* Yine asagıdaki ayar sebebiyle MAVI ekran  alındı */
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x1B, ADV7181C_init_FAIL); // Enable STDI Line Count Mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x8A, 0xB0, ADV7181C_init_FAIL); // Manual VCO Range=01
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xBF, 0x06, ADV7181C_init_FAIL); // Blue Screen Free Run Colour
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x40, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC1, 0xF0, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC2, 0x80, ADV7181C_init_FAIL); // Default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC5, 0x01, ADV7181C_init_FAIL); //
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // ADI recommended sequence
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // ADI recommended sequence
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // ADI recommended sequence
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x57, 0x01, ADV7181C_init_FAIL); // ADI recommended sequence
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // ADI recommended sequence
			break;
		case ADV7181C_720p_50Hz_20bit_422_SAVEAV:	// :720p/50Hz YPrPb In 20Bit 422 EAV/SAV Encoder:
			INFOL(libADV_log_instance, "ADV7181C_720p_50Hz_20bit_422_SAVEAV selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x01, ADV7181C_init_FAIL); // Prim_Mode =001b COMP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x0A, ADV7181C_init_FAIL); // VID_STD=1010b for 720P 1x1
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0x46, ADV7181C_init_FAIL); // ADC1 to Ain4, ADC0 to Ain6,
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0xB5, ADV7181C_init_FAIL); // ADC2 to Ain5 and enables manual override of mux
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable , 0x8MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x21, ADV7181C_init_FAIL); // set latch clock settings to 010b, Power Down ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3C, 0x5D, ADV7181C_init_FAIL); // PLL_QPUMP to 101b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x6B, 0x81, ADV7181C_init_FAIL); // 422 20bit out
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC9, 0x00, ADV7181C_init_FAIL); // SDR
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x73, 0xCF, ADV7181C_init_FAIL); // Enable Manual Gain and set CH_A gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x74, 0xA3, ADV7181C_init_FAIL); // Set CH_A and CH_B Gain - 0FAh
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x75, 0xE8, ADV7181C_init_FAIL); // Set CH_B and CH_C Gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x76, 0xFA, ADV7181C_init_FAIL); // Set CH_C gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7B, 0x1E, ADV7181C_init_FAIL); // TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7C, 0x00, ADV7181C_init_FAIL); // set HS position & polarity
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7D, 0x00, ADV7181C_init_FAIL); // set HS position
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7E, 0x00, ADV7181C_init_FAIL); // set HS position
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7F, 0x00, ADV7181C_init_FAIL); //
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x85, 0x19, ADV7181C_init_FAIL); // Turn off SSPD and force SOY. For Eval Board.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x1B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x87, 0xE7, ADV7181C_init_FAIL); // Reprogram pll_div_ratio for 1080i/50Hz = 1980
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x88, 0xBC, ADV7181C_init_FAIL); //
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x8F, 0x02, ADV7181C_init_FAIL); // Reprogram freerun_line_length also to 764 for 28.636MHz xtal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x90, 0xFC, ADV7181C_init_FAIL); //
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xBF, 0x06, ADV7181C_init_FAIL); // Blue Screen Free Run Colour
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x40, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC1, 0xF0, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC2, 0x80, ADV7181C_init_FAIL); // Default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC5, 0x01, ADV7181C_init_FAIL); // CP_CLAMP_AVG_FACTOR[1-0] = 00b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF4, 0x3F, ADV7181C_init_FAIL); // Max Drive Strength
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x57, 0x01, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			break;
		case ADV7181C_720p_60Hz_20bit_422_SAVEAV:	// :720p/60Hz YPrPb In 16Bit 422 EAV/SAV Out Encoder:
			INFOL(libADV_log_instance, "ADV7181C_720p_60Hz_20bit_422_SAVEAV selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x01, ADV7181C_init_FAIL); // Prim_Mode =001b COMP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x0A, ADV7181C_init_FAIL); // VID_STD=1010b for 720P 1x1
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x21, ADV7181C_init_FAIL); // set latch clock settings to 010b, Power Down ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3C, 0x5D, ADV7181C_init_FAIL); // PLL_QPUMP to 101b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x6B, 0xC3, ADV7181C_init_FAIL); // Select 422 16 bit YPrPb out from CP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x85, 0x19, ADV7181C_init_FAIL); // Turn off SSPD and force SOY. For Eval Board.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x1B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xBF, 0x06, ADV7181C_init_FAIL); // Blue Screen Free Run Colour
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x40, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC1, 0xF0, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC2, 0x80, ADV7181C_init_FAIL); // Default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // Enable Hidden Space.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // Enable SOG/SOY Clamp Filter
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // CML Level Change
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // Disable Hidden Space.
			break;
#if(0)
		case ADV7181C_1080i_50Hz_20bit_422_HSVS:
			INFOL(libADV_log_instance, "ADV7181C_1080i_50Hz_20bit_422_HSVS selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x01, ADV7181C_init_FAIL); // Prim_Mode =001b COMP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x0C, ADV7181C_init_FAIL); // VID_STD for 1080i
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0x46, ADV7181C_init_FAIL); // ADC1 to Ain4, ADC0 to Ain6,
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0xB5, ADV7181C_init_FAIL); // ADC2 to Ain5 and enables manual override of mux
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x21, ADV7181C_init_FAIL); // set latch clock settings to 010b, Power Down ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3C, 0x5D, ADV7181C_init_FAIL); // PLL_QPUMP to 101b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x6B, 0x81, ADV7181C_init_FAIL); // 422 20bit OUT
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC9, 0x00, ADV7181C_init_FAIL); // SDR
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x73, 0xCF, ADV7181C_init_FAIL); // Enable Manual Gain and set CH_A gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x74, 0xA3, ADV7181C_init_FAIL); // Set CH_A and CH_B Gain - 0FAh
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x75, 0xE8, ADV7181C_init_FAIL); // Set CH_B and CH_C Gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x76, 0xFA, ADV7181C_init_FAIL); // Set CH_C gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7B, 0x1C, ADV7181C_init_FAIL); // TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7C, 0x93, ADV7181C_init_FAIL); // set HS position/ HS Polarity - positive
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7D, 0xD4, ADV7181C_init_FAIL); // set HS position
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7E, 0x2D, ADV7181C_init_FAIL); // set HS position
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x85, 0x19, ADV7181C_init_FAIL); // Turn off SSPD and force SOY. For Eval Board.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x0B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x87, 0xEA, ADV7181C_init_FAIL); // Reprogram pll_div_ratio for 1080i/50Hz = 2640
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x88, 0x50, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x8F, 0x03, ADV7181C_init_FAIL); // Reprogram freerun_line_length also to 1018 for 28.636MHz xtal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x90, 0xFA, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB7, 0x1B, ADV7181C_init_FAIL); // use internal VS width for fixing VS vibration
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xBF, 0x06, ADV7181C_init_FAIL); // Blue Screen Free Run Colour
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x40, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC1, 0xF0, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC2, 0x80, ADV7181C_init_FAIL); // Default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC5, 0x01, ADV7181C_init_FAIL); // CP_CLAMP_AVG_FACTOR[1-0] = 00b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF4, 0x3F, ADV7181C_init_FAIL); // Max Drive Strength
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x57, 0x01, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			break;
#endif
		case ADV7181C_1080i_50Hz_20bit_422_SAVEAV:	// :1080i/50Hz YPrPb In 20Bit 422 EAV/SAV Encoder:
			INFOL(libADV_log_instance, "ADV7181C_1080i_50Hz_20bit_422_SAVEAV selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x01, ADV7181C_init_FAIL); // Prim_Mode =001b COMP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x0C, ADV7181C_init_FAIL); // VID_STD for 1080i
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0x46, ADV7181C_init_FAIL); // ADC1 to Ain4, ADC0 to Ain6,
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0xB5, ADV7181C_init_FAIL); // ADC2 to Ain5 and enables manual override of mux
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x21, ADV7181C_init_FAIL); // set latch clock settings to 010b, Power Down ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3C, 0x5D, ADV7181C_init_FAIL); // PLL_QPUMP to 101b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x6B, 0xC1, ADV7181C_init_FAIL); // 422 20bit OUT
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC9, 0x00, ADV7181C_init_FAIL); // SDR
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x73, 0xCF, ADV7181C_init_FAIL); // Enable Manual Gain and set CH_A gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x74, 0xA3, ADV7181C_init_FAIL); // Set CH_A and CH_B Gain - 0FAh
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x75, 0xE8, ADV7181C_init_FAIL); // Set CH_B and CH_C Gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x76, 0xFA, ADV7181C_init_FAIL); // Set CH_C gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7B, 0x1E, ADV7181C_init_FAIL); // Enable EAV & SAV CODES
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7C, 0x93, ADV7181C_init_FAIL); // set HS position/ HS Polarity - positive
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7D, 0xD4, ADV7181C_init_FAIL); // set HS position
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7E, 0x2D, ADV7181C_init_FAIL); // set HS position
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x85, 0x19, ADV7181C_init_FAIL); // Turn off SSPD and force SOY. For Eval Board.
			// Alttaki ayarın ORG değeri 0x0B idi ve renklerin MAVI ağırlıklı olmasına sebep oluyordu.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x1B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x87, 0xEA, ADV7181C_init_FAIL); // Reprogram pll_div_ratio for 1080i/50Hz = 2640
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x88, 0x50, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x8F, 0x03, ADV7181C_init_FAIL); // Reprogram freerun_line_length also to 1018 for 28.636MHz xtal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x90, 0xFA, ADV7181C_init_FAIL);
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB7, 0x1B, ADV7181C_init_FAIL); // use internal VS width for fixing VS vibration
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xBF, 0x0E, ADV7181C_init_FAIL); // Blue Screen Free Run Colour
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x40, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC1, 0xF0, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC2, 0x80, ADV7181C_init_FAIL); // Default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC5, 0x01, ADV7181C_init_FAIL); // CP_CLAMP_AVG_FACTOR[1-0] = 00b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF4, 0x3F, ADV7181C_init_FAIL); // Max Drive Strength
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x57, 0x01, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			break;
		case ADV7181C_1080i_60Hz_20bit_422_SAVEAV:	// :1080i/60Hz YPrPb In 20Bit 422 EAV/SAV Encoder:
			INFOL(libADV_log_instance, "ADV7181C_1080i_60Hz_20bit_422_SAVEAV selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x01, ADV7181C_init_FAIL); // Prim_Mode =001b COMP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x0C, ADV7181C_init_FAIL); // VID_STD=1100b for 1125 1x1
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x21, ADV7181C_init_FAIL); // set latch clock settings to 010b, Power Down ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3C, 0x5D, ADV7181C_init_FAIL); // PLL_QPUMP to 101b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x6B, 0xC1, ADV7181C_init_FAIL); // Select 422 20 bit YPrPb out from CP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x85, 0x19, ADV7181C_init_FAIL); // Turn off SSPD and force SOY. For Eval Board.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x1B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB7, 0x1B, ADV7181C_init_FAIL); // use internal VS width for fixing VS vibration
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xBF, 0x0E, ADV7181C_init_FAIL); // Default Color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x40, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC1, 0xF0, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC2, 0x80, ADV7181C_init_FAIL); // Default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // Enable Hidden Space.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // Enable SOG/SOY Clamp Filter
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // CML Level Change
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // Disable Hidden Space.
			break;
#if(0)
		case ADV7181C_1080i_60Hz_20bit_422_HSVS:
			INFOL(libADV_log_instance, "ADV7181C_1080i_60Hz_20bit_422_HSVS selected\n");
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x01, ADV7181C_init_FAIL); // Prim_Mode =001b COMP
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x06, 0x0C, ADV7181C_init_FAIL); // VID_STD for 1080i
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC3, 0x46, ADV7181C_init_FAIL); // ADC1 to Ain4, ADC0 to Ain6,
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC4, 0xB5, ADV7181C_init_FAIL); // ADC2 to Ain5 and enables manual override of mux
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x1D, 0x47, ADV7181C_init_FAIL); // Enable 28MHz Crystal
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3A, 0x21, ADV7181C_init_FAIL); // set latch clock settings to 010b, Power Down ADC3
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3B, 0x81, ADV7181C_init_FAIL); // Enable Internal Bias
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x3C, 0x5D, ADV7181C_init_FAIL); // PLL_QPUMP to 101b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x6B, 0x81, ADV7181C_init_FAIL); // TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x05, 0x01, ADV7181C_init_FAIL);2 20bit OUT
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC9, 0x00, ADV7181C_init_FAIL); // SDR
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x73, 0xCF, ADV7181C_init_FAIL); // Enable Manual Gain and set CH_A gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x74, 0xA3, ADV7181C_init_FAIL); // Set CH_A and CH_B Gain - 0FAh
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x75, 0xE8, ADV7181C_init_FAIL); // Set CH_B and CH_C Gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x76, 0xFA, ADV7181C_init_FAIL); // Set CH_C gain
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7B, 0x1D, ADV7181C_init_FAIL); // TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7C, 0x93, ADV7181C_init_FAIL); // set HS position/ HS Polarity - positive
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7D, 0xD4, ADV7181C_init_FAIL); // set HS position
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x7E, 0x2D, ADV7181C_init_FAIL); // set HS position
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x85, 0x19, ADV7181C_init_FAIL); // Turn off SSPD and force SOY. For Eval Board.
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x86, 0x1B, ADV7181C_init_FAIL); // Enable stdi_line_count_mode
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xB7, 0x1B, ADV7181C_init_FAIL); // use internal VS width for fixing VS vibration
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xBF, 0x06, ADV7181C_init_FAIL); // Blue Screen Free Run Colour
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC0, 0x40, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC1, 0xF0, ADV7181C_init_FAIL); // default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC2, 0x80, ADV7181C_init_FAIL); // Default color
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xC5, 0x01, ADV7181C_init_FAIL); // CP_CLAMP_AVG_FACTOR[1-0] = 00b
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF4, 0x3F, ADV7181C_init_FAIL); // Max Drive Strength
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x80, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x52, 0x46, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x54, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x57, 0x01, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0xF6, 0x3B, ADV7181C_init_FAIL); // ADI Recommended Setting
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0E, 0x00, ADV7181C_init_FAIL); // ADI Recommended Setting
			break;
#endif
		default:
			ERRL(libADV_log_instance, "UNEXPECTED ADC7181C mode(0x%02X) as parameter\n", mode);
			goto ADV7181C_init_FAIL;
			break;
	}

	// Read input parameters //
	sADV7181C_VidInfo *inpvid_p = ADV7181C_get_input_info(50, 20);
	if(NULL == inpvid_p) {
		ON_FAILURE(libADV_log_instance, "ADV7181C_get_input_info() FAILED\n", ADV7181C_init_FAIL);
	}

	switch(mode) {
		case ADV7181C_CVBS_PAL: // :AUTODETECT CVBS IN NTSC/PAL/SECAM, 8-Bit 422 encoder:
		case ADV7181C_CVBS_NTSC: {
		if(TRUE == inpvid_p->inpok) {
			switch(inpvid_p->cvbs_type) {
				case NTSM_MJ:
				case NTSC_443:
					mode = ADV7181C_CVBS_NTSC;
					DEBUGL(libADV_log_instance, "NTSC MODE DETECTED\n");
					break;
				case PAL_M:
				case PAL_60:
				case PAL_BGHID:
				case PAL_CN:
					mode = ADV7181C_CVBS_PAL;
					DEBUGL(libADV_log_instance, "PAL MODE DETECTED\n");
					break;
				case SECAM:
				case SECAM_525:
				default:
					ERRL(libADV_log_instance, "UNKNOWN CVBS MODE(%u) DETECTED @ INPUT\n", inpvid_p->cvbs_type);
					goto ADV7181C_init_FAIL;
					break;
			}
		}
		else {
			ERRL(libADV_log_instance, "ANALOG VIDEO INPUT NOT VALID\n");
			mode = ADV7181C_CVBS_PAL;	// We assume that it is PAL for now //
			goto ADV7181C_input_not_valid;
		}
	}
	break;
	default:
		ERRL(libADV_log_instance, "UNIMPLENMED MODE(%u) REQUESTED\n");
		goto ADV7181C_init_FAIL;
		break;
	}

ADV7181C_input_not_valid:
	DEBUGL(libADV_log_instance, "ADC7181C_init() SUCCESS\n\n");
	return mode;

ADV7181C_init_FAIL:
	ERRL(libADV_log_instance, "ADC7181C_init() FAILED\n\n");
	return -1;
}

int8_t is_analog_video_active(void)
{
	// TODO //
	return TRUE;
}

int8_t ADV7181C_setpower(uint8_t mode)
{
	if(TRUE == mode) {
		TRY_CMASK(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0F, 0x3A, ADV7181C_setpower_FAIL);	// Clear all-bits, Power UP All bits //
		ADV7181C_RESET(ADV7181C_setpower_FAIL);
		return analog_video_init(ADV7181C_CVBS_PAL);							// ReInit Chip from scratch //
	}
	else {
		TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV,ADV7181C_ADR, 0x0F, 0x3A, ADV7181C_setpower_FAIL);	// Power DOWN, Set All bits //
	}

	return 0;

ADV7181C_setpower_FAIL:
	return -1;
}

sADV7181C_VidInfo *ADV7181C_get_input_info(uint16_t tryCnt, uint16_t waitMS)
{
	uint16_t VidValidCnt = 0;
	uint16_t ValidMax = tryCnt/2;
	int16_t readval = 0;
	volatile uint16_t indx;
	uint16_t palcnt=0, ntsccnt=0;

	ADV7181C_vidinfo.inpok = FALSE;

	for(indx=0 ; indx<tryCnt ; indx++) {
		TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x10, ADV7181C_get_input_info_FAIL);
		TRACEL(libADV_log_instance, "ADC7181C reg(0x%02X) = 0x%02X\n\n", 0x10, readval);
		if(((0x1<<0) & readval) && ((0x1<<2) & readval)) { // Input valid or NOT //
			// Process about CVBS type //
			uint8_t cvbs_type = (readval & (0x07<<4))>>4;
			switch(cvbs_type) {
				case NTSM_MJ:
				case NTSC_443:
					ntsccnt++; if(0 != palcnt) palcnt--;
					break;
				case PAL_M:
				case PAL_60:
				case PAL_BGHID:
				case PAL_CN:
					TRACEL(libADV_log_instance, "NTSC MODE DETECTED\n");
					palcnt++; if(0 != ntsccnt) ntsccnt--;
					break;
				case SECAM:
				case SECAM_525:
				default:
					ERRL(libADV_log_instance, "UNKNOWN CVBS MODE(%u) DETECTED @ INPUT\n", cvbs_type);
					goto ADV7181C_get_input_info_FAIL;
					break;
			}
			// Input valid counter reached maximum or NOT //
			if(++VidValidCnt == ValidMax) {
				DEBUGL(libADV_log_instance," adv7181C counter is %u (STABLE NUM REACHED)\n", VidValidCnt);
				ADV7181C_vidinfo.inpok = TRUE;
				DEBUGL(libADV_log_instance, "PAL counter(%u) vs NTSC counter(%u)\n", palcnt, ntsccnt);
				if(palcnt >= ntsccnt) {
					ADV7181C_vidinfo.cvbs_type = PAL_M;
					TRACEL(libADV_log_instance, "PAL MODE DETECTED\n");
				}
				else {
					ADV7181C_vidinfo.cvbs_type = NTSM_MJ;
					TRACEL(libADV_log_instance, "NTSC MODE DETECTED\n");
				}
				break;
			}
			else {
				TRACEL(libADV_log_instance," adv7181C counter is %u (STABLE NUM NOT REACHED)\n", VidValidCnt);
			}
		}
		else {
			ADV7181C_vidinfo.inpok = FALSE;
			VidValidCnt = 0;
		}

		#if(0)
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x12, ADV7181C_get_input_info_FAIL);
			TRACEL(libADV_log_instance, "ADC7181C reg(0x%02X) = 0x%02X\n\n", 0x12, readval);

			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x13, ADV7181C_get_input_info_FAIL);
			TRACEL(libADV_log_instance, "ADC7181C reg(0x%02X) = 0x%02X\n\n", 0x13, readval);
			if(((0x1)<<2) & readval)	// Check for videoHZ //
				ADV7181C_vidinfo.vidHZ = 50;
			else
				ADV7181C_vidinfo.vidHZ = 60;
			if(((0x1)<<3) & readval)	// Check for CVBS input //
				ADV7181C_vidinfo.cvbs_input = TRUE;
			else
				ADV7181C_vidinfo.cvbs_input = FALSE;
			if(((0x1)<<4) & readval)	// Check for FREE_RUN //
				ADV7181C_vidinfo.free_run = TRUE;
			else
				ADV7181C_vidinfo.free_run = FALSE;
			if(((0x1)<<6) & readval)	// Check for interlaced //
				ADV7181C_vidinfo.interlaced = TRUE;
			else
				ADV7181C_vidinfo.interlaced = FALSE;
		#else
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x12, ADV7181C_get_input_info_FAIL);
			TRACEL(libADV_log_instance, "ADC7181C reg(0x%02X) = 0x%02X\n\n", 0x12, readval);

			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV,readval, ADV7181C_ADR, 0x13, ADV7181C_get_input_info_FAIL);
			TRACEL(libADV_log_instance, "ADC7181C reg(0x%02X) = 0x%02X\n\n", 0x13, readval);
		#endif
		usleep((uint32_t)waitMS*1000UL);
	}

	return &ADV7181C_vidinfo;

ADV7181C_get_input_info_FAIL:
	return NULL;
}
