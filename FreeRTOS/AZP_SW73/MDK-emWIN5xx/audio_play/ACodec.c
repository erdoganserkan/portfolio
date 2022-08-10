/**********************************************************************
* $Id$		uda1380.c			2011-10-26
*//**
* @file		uda1380.c
* @brief	This module contains functions to control UDA1380.
* @version	1.0
* @date		26. October. 2011
* @author	NXP MCU SW Application Team
*
* Copyright(C) 2011, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
* Permission to use, copy, modify, and distribute this software and its
* documentation is hereby granted, under NXP Semiconductors'
* relevant copyright in the software, without fee, provided that it
* is used in conjunction with NXP Semiconductors microcontrollers.  This
* copyright, permission, and disclaimer notice must appear in all copies of
* this code.
**********************************************************************/
#ifdef __BUILD_WITH_EXAMPLE__
	#include "lpc177x_8x_libcfg.h"
#else
	#include "lpc177x_8x_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */
#ifdef _I2C
#include "lpc177x_8x_i2c.h"
#include "lpc177x_8x_pinsel.h"
#include "AppCommon.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "RuntimeLoader.h"
#include "APlay.h"
#include "ACodec.h"

#if(AUDIO_IC_LPCDAC != USED_AUDIO_DEVICE)

//Uda1380 link to I2C0 only
#define AUDIO_CODEC_I2C		I2C_0

#define ON_FUNC_FAILURE(func)	{ \
	if(ACODEC_FUNC_OK != func) \
		return ACODEC_FUNC_ERR; \
}

static int32_t __LM49450_Mute(Bool MuteOn); 
static inline int32_t __Lm49450_set_lp(uint8_t is_LP_on);


static inline int32_t __init_I2Cx(uint32_t i2cClockFreq) {
	// Config Pin for I2C_SDA and I2C_SCL of I2C0
	// It's because the Uda1380/LM49450 IC is linked to LPC177x_8x by I2C0 clearly
	PINSEL_ConfigPin (5, 2, 5);
	PINSEL_ConfigPin (5, 3, 5);
	I2C_Init(AUDIO_CODEC_I2C, i2cClockFreq);
	/* Enable I2Cx operation */
	I2C_Cmd(AUDIO_CODEC_I2C, I2C_MASTER_MODE, ENABLE);
}

// Send new_val into device registers for both headphones(0x07) and loud speakers(0x08) //
static inline int32_t __LM49450_set_vol(uint8_t new_val) {
	int32_t ret = ACODEC_FUNC_OK;
	ON_FUNC_FAILURE(ACodec_WriteData(AUDIO_IC_LM49450, LM49450_HP_VOL_CONTROL_REG, new_val));
	ON_FUNC_FAILURE(ACodec_WriteData(AUDIO_IC_LM49450, LM49450_SPK_VOL_CONTROL_REG, new_val));

	return ret;
}

// Setting for DAC mode // 
static inline int32_t __Lm49450_set_DAC_mode(uint8_t new_DAC_mode)
{
	int32_t ret = ACODEC_FUNC_OK;
	uint16_t tmp = 0;

	ON_FUNC_FAILURE(ACodec_ReadData(AUDIO_IC_LM49450, LM49450_MODE_CONTROL_REG, &tmp));
	tmp &= (~(LM49450_DAC_OSR_BITs_MASK << LM49450_DAC_OSR_BITs_POS));
	tmp |= (new_DAC_mode << LM49450_DAC_OSR_BITs_POS);
	ON_FUNC_FAILURE(ACodec_WriteData(AUDIO_IC_LM49450, LM49450_MODE_CONTROL_REG, tmp));

	return ret;
}

// Setting for Low Power mode // 
static inline int32_t __Lm49450_set_lp(uint8_t is_LP_on) 
{
	int32_t ret = ACODEC_FUNC_OK;
	uint16_t tmp = 0;

	ON_FUNC_FAILURE(ACodec_ReadData(AUDIO_IC_LM49450, LM49450_MODE_CONTROL_REG, &tmp));
	if(TRUE == is_LP_on)
		tmp &= (~0x1);	// Clear B0 bit // 
	else 
		tmp |= 0x1;	// SET B0 bit // 
	ON_FUNC_FAILURE(ACodec_WriteData(AUDIO_IC_LM49450, LM49450_MODE_CONTROL_REG, tmp));

	return ret;
}

// Setting / claring of spread spectrum mode // 
static inline int32_t __Lm49450_set_SS(uint8_t is_SS_on) 
{
	int32_t ret = ACODEC_FUNC_OK;
	uint16_t tmp = 0;

	ON_FUNC_FAILURE(ACodec_ReadData(AUDIO_IC_LM49450, LM49450_MODE_CONTROL_REG, &tmp));
	if(FALSE == is_SS_on)
		tmp &= (~(0x1<<3));	// Clear B0 bit // 
	else 
		tmp |= (0x1<<3);	// SET B0 bit // 
	ON_FUNC_FAILURE(ACodec_WriteData(AUDIO_IC_LM49450, LM49450_MODE_CONTROL_REG, tmp));

	return ret;
}

static int32_t __LM49450_Mute(Bool MuteOn) 
{
	int32_t ret = ACODEC_FUNC_OK;  
	uint16_t tmp = 0;

	ON_FUNC_FAILURE(ACodec_ReadData(AUDIO_IC_LM49450, LM49450_MODE_CONTROL_REG, &tmp));
	if(FALSE == MuteOn)
		tmp &= (~(0x1<<2));	// Mute Disabled // 
	else 	
		tmp |= (0x1<<2);	// Mute Enabled // 
	ON_FUNC_FAILURE(ACodec_WriteData(AUDIO_IC_LM49450, LM49450_MODE_CONTROL_REG, tmp));

	return ret;
}

static int32_t __LM49450_Set_RDIV(uint8_t new_rdiv) 
{
	int32_t ret = ACODEC_FUNC_OK;  
	uint16_t tmp = 0;

	ON_FUNC_FAILURE(ACodec_ReadData(AUDIO_IC_LM49450, LM49450_CLOCK_REG, &tmp));
	tmp &= (~(RDIV_SETTIMG_MASK << RDIV_SETTING_BIT_POST));
	tmp |= (0x03 << 0);	// Setting is fixed to 2 // 
	ON_FUNC_FAILURE(ACodec_WriteData(AUDIO_IC_LM49450, LM49450_CLOCK_REG, tmp));

	return ret;
}

int8_t LM49450_volume_set(uint8_t new_val, uint8_t play_sample)
{
	uint16_t tmp = 0;
	int32_t ret = ACODEC_FUNC_OK;
	
	if(new_val > VOLUME_MAX)
		new_val = VOLUME_MAX;
	if(new_val == VOLUME_MAX)
		new_val = LM49450_VOLUME_LEVELs;
	else 
		new_val = ((uint16_t)new_val * (uint16_t)LM49450_VOLUME_LEVELs)/VOLUME_MAX;

	if(0 == new_val) {
		// Set volume level to minimum 
		ON_FUNC_FAILURE(__LM49450_set_vol(new_val));
		// enable MUTE : 0x00:B2 = 1 
		ON_FUNC_FAILURE(__LM49450_Mute(TRUE));
		// Enable Low Power Shutdown: Mode Control Register (0x00), B0=0 // 
		ON_FUNC_FAILURE(__Lm49450_set_lp(TRUE));
	} 
	else {
		// Disable Low power Shutdown Mode 0x00:B0 = 1 // 
		ON_FUNC_FAILURE(__Lm49450_set_lp(FALSE));
		// Wait For a While //
		App_waitMS(100);
		// Disable MUTE // 
		ON_FUNC_FAILURE(__LM49450_Mute(FALSE));
		// Set new volume level // 
		ON_FUNC_FAILURE(__LM49450_set_vol(new_val));
		if(TRUE == play_sample) {
			// Play sample sound, Wait until sound played // 
			start_i2s_audio(OLD_SAMPLE_SOUND, TRUE);
		}
	}
	
	return ret;
}


int32_t LM49450_Init(uint32_t i2cClockFreq, uint32_t i2sClockFreq)
{
	int32_t ret = ACODEC_FUNC_OK;
	//:TODO:
	// 0- Init I2C peripheral of MCU 
	__init_I2Cx(i2cClockFreq);
	// 0- Disable LP mode 
	ON_FUNC_FAILURE(__Lm49450_set_lp(FALSE));
	// 1- set mode to fixed frequency mode (Not spread spectrum mode)
	ON_FUNC_FAILURE(__Lm49450_set_SS(TRUE));
	ON_FUNC_FAILURE(__Lm49450_set_DAC_mode(LM49450_DAC_OSR_128));
	__LM49450_Set_RDIV(2);
	// 2- set DAC clock divider; Clock Control Register (0x01):RDIV Bits 
	// 3- Set I2S to slave mode 
	// 4- Set input as standart I2S output (Not left or right justified)
	// 5- Do clock settings for audio samples clock frequency (32000 Hz)
	// 6- Set sved volume level 
	ON_FUNC_FAILURE(LM49450_volume_set(VOLUME_MAX, FALSE));
	
	uint16_t txt = 0;
	if(ACodec_ReadData(AUDIO_IC_LM49450, 0x02, &txt) != ACODEC_FUNC_OK) {
		ERRM("ACODEC: RegWRITE(Val:0x%04X to Reg:0x%02X) FAILED\n", txt, 0x02);
		return ACODEC_FUNC_ERR;
	}

			return ret; 
}

/*********************************************************************//**
 * @brief 		Initialize Uda1380
 * @param[in]	i2cClockFreq	I2C clock frequency that Uda1380 operate
 * @param[in] i2sClockFreq  I2S bit clock frequency
 * @return 		ACODEC_FUNC_OK/ACODEC_FUNC_ERR
 **********************************************************************/
int32_t Uda1380_Init(uint32_t i2cClockFreq, uint32_t i2sClockFreq)
{
	int32_t ret = ACODEC_FUNC_OK;
	uint8_t clk;
	
	__init_I2Cx(i2cClockFreq);
	
	/* Reset */
	ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_L3, 0 );
	if(ret != ACODEC_FUNC_OK)
		return ret;

    /* Write clock settings */
	ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_I2S,0 );
	if(ret != ACODEC_FUNC_OK)
		return ret;

#if UDA1380_SYSCLK_USED //Use SYSCLK
	ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_EVALCLK, 
	       EVALCLK_DEC_EN | EVALCLK_DAC_EN | EVALCLK_INT_EN | EVALCLK_DAC_SEL_SYSCLK );
	if(ret != ACODEC_FUNC_OK)
		return ret;
f	
	ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_PWRCTRL,
		              PWR_PON_HP_EN | PWR_PON_DAC_EN | PWR_PON_BIAS_EN);
	if(ret != ACODEC_FUNC_OK)
		return ret;
	
#else //Use WSPLL	
	if(i2sClockFreq >= 6250 && i2sClockFreq < 12500)
		clk = EVALCLK_WSPLL_SEL6_12K;
	else if(i2sClockFreq >= 12501 && i2sClockFreq < 25000)  
		clk = EVALCLK_WSPLL_SEL12_25K;
	else if(i2sClockFreq >= 25001 && i2sClockFreq < 50000)
		clk = EVALCLK_WSPLL_SEL25_50K;
	else if(i2sClockFreq >= 50001 && i2sClockFreq < 100000)
		clk = EVALCLK_WSPLL_SEL50_100K;
	else
		clk= 0;
		
	ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_EVALCLK, 
	                 EVALCLK_DEC_EN | EVALCLK_DAC_EN | EVALCLK_INT_EN | EVALCLK_DAC_SEL_WSPLL | clk);
	if(ret != ACODEC_FUNC_OK)
		return ret;
	
	ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_PWRCTRL,
		              PWR_PON_PLL_EN | PWR_PON_HP_EN | PWR_PON_DAC_EN | PWR_PON_BIAS_EN);
	if(ret != ACODEC_FUNC_OK)
		return ret;
#endif

	ret = Uda1380_Mute(FALSE);
    if(ret != ACODEC_FUNC_OK)
		return ret;

	return ACODEC_FUNC_OK;
}

/*********************************************************************//**
 * @brief 		Write data to a register of Uda1380
 * @param[in]	reg	Register address
 * @param[out] data  data value.
 * @return 		ACODEC_FUNC_OK/ACODEC_FUNC_ERR
 **********************************************************************/
int32_t ACodec_WriteData(uint8_t acodec_type, uint8_t reg_adr, uint16_t data)
{
	I2C_M_SETUP_Type i2cData;
	uint8_t i2cBuf[UDA1380_CMD_BUFF_SIZE];

	i2cBuf[0] = reg_adr;
	if(AUDIO_IC_UDA1380 == acodec_type) {	// Registers are 16bit length // 
		i2cBuf[1] = (data >> 8) & 0xFF;
		i2cBuf[2] = data & 0xFF;
		i2cData.sl_addr7bit = UDA1380_SLAVE_ADDR;
		i2cData.tx_length = UDA1380_CMD_BUFF_SIZE;
	} else if(AUDIO_IC_LM49450 == acodec_type) {	// Registers are 8bit length // 
		i2cBuf[1] = data & 0xFF;
		i2cData.sl_addr7bit = LM49450_SLAVE_ADR;
		i2cData.tx_length = LM49450_CMD_BUFF_SIZE;
	} else while(1);
    i2cData.tx_data = i2cBuf;
	i2cData.rx_data = NULL;
	i2cData.rx_length = 0;
	i2cData.retransmissions_max = ACODEC_RETRANSMISSION_MAX;	
	
	if (I2C_MasterTransferData(AUDIO_CODEC_I2C, &i2cData, I2C_TRANSFER_POLLING) == SUCCESS) {
	    uint16_t dataTmp = 0;
	    if(ACodec_ReadData(acodec_type, reg_adr, &dataTmp) != ACODEC_FUNC_OK) {
				ERRM("ACODEC: RegWRITE(Val:0x%04X to Reg:0x%02X) FAILED\n", data, reg_adr);
				return ACODEC_FUNC_ERR;
	    }
		if(AUDIO_IC_LM49450 == acodec_type) {
			data &= 0xFF;
			dataTmp &= 0xFF;
		}
		if(dataTmp != data) {
			ERRM("ACODEC: RegWRITE(Val:0x%04X to Reg:0x%02X) FAILED\n", data, reg_adr);
			return ACODEC_FUNC_ERR;
		}
		else {
			DEBUGM("ACODEC: RegWRITE(Val:0x%04X to Reg:0x%02X) SUCCESS\n", data, reg_adr);
			return ACODEC_FUNC_OK;
		}
	} 
	else {
		ERRM("LM49450: RegWRITE(Val:0x%04X to Reg:0x%02X) FAILED\n", data, reg_adr);
		return ACODEC_FUNC_ERR;
	}
}


/*********************************************************************//**
 * @brief 		Read data stored in register of Uda1380
 * @param[in]	reg	Register address
 * @param[out] data  point to the buffer which is used for storing data.
 * @return 		ACODEC_FUNC_OK/ACODEC_FUNC_ERR
 **********************************************************************/
int32_t ACodec_ReadData(uint8_t acodec_type, uint8_t reg_adr, uint16_t *data)
{
	I2C_M_SETUP_Type i2cData;
	uint8_t i2cBuf[UDA1380_CMD_BUFF_SIZE];

	if(data == NULL)
		return ACODEC_FUNC_ERR;

	i2cBuf[0] = reg_adr;

	if(AUDIO_IC_UDA1380 == acodec_type) {
		i2cData.sl_addr7bit = UDA1380_SLAVE_ADDR;
		i2cData.rx_length = UDA1380_CMD_BUFF_SIZE - 1;
	} else if(AUDIO_IC_LM49450 == acodec_type) {
		i2cData.sl_addr7bit = LM49450_SLAVE_ADR;
		i2cData.rx_length = LM49450_CMD_BUFF_SIZE - 1;
	} else while(1);
	i2cData.tx_length = 1;
    i2cData.tx_data = i2cBuf;
	i2cData.rx_data = &i2cBuf[1];
	i2cData.retransmissions_max = ACODEC_RETRANSMISSION_MAX;	
	
	if (I2C_MasterTransferData(AUDIO_CODEC_I2C, &i2cData, I2C_TRANSFER_POLLING) == SUCCESS) {
		*data = 0;
		if(AUDIO_IC_UDA1380 == acodec_type) 
		    *data = (i2cBuf[1] << 8) | i2cBuf[2];
		else if(AUDIO_IC_LM49450 == acodec_type)
		    *data = i2cBuf[1];
		DEBUGM("ACODEC: RegREAD(Reg:0x%02X, Val:0x%04X) SUCCESS\n", reg_adr, *data);
		return ACODEC_FUNC_OK;
	}

	ERRM("ACODEC: RegREAD(Reg:0x%02X) FAILED\n", reg_adr);
	return ACODEC_FUNC_ERR;
}
/*********************************************************************//**
 * @brief 		Mute On/Off
 * @param[in]	MuteOn	TRUE: mute signal
 * @return 		ACODEC_FUNC_OK/ACODEC_FUNC_ERR
 **********************************************************************/
int32_t Uda1380_Mute(Bool MuteOn)
{
    uint16_t tmp;
    int32_t ret;
	
    /* backup old value setting of clock */
    ret = ACodec_ReadData(AUDIO_IC_UDA1380, UDA1380_REG_EVALCLK, &tmp);
    if(ret != ACODEC_FUNC_OK)
		return ret;
    /* Use sysclk */
    ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_EVALCLK, tmp & (~EVALCLK_DAC_SEL_WSPLL));
    if(ret != ACODEC_FUNC_OK)
		return ret;

    if(!MuteOn) {
       ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_MSTRMUTE,0x0202);
    } else {
       ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_MSTRMUTE,0x4808);
    }
	if(ret != ACODEC_FUNC_OK)
		return ret;
	
     /* Use sysclk */
    ret = ACodec_WriteData(AUDIO_IC_UDA1380, UDA1380_REG_EVALCLK, tmp);
    if(ret != ACODEC_FUNC_OK)
		return ret;

    return ACODEC_FUNC_OK;
}
#endif	/* */
#endif /*_I2C*/

