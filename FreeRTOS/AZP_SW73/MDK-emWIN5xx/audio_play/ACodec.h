#ifndef _UDA1380_H_
#define _UDA1380_H_

#include "lpc_types.h"
#include "UMDShared.h"

/** @defgroup  UDA_1380	UDA 1380
 * @ingroup LPC177x_8xCMSIS_Board_Support
 * @{
 */

#define UDA1380_SYSCLK_USED 		0
#define UDA1380_SLAVE_ADDR  		0x1A
#define UDA1380_CMD_BUFF_SIZE 		3
#define ACODEC_RETRANSMISSION_MAX	3

/** UDA1380 Registers */
#define UDA1380_REG_EVALCLK	0x00
#define UDA1380_REG_I2S		0x01
#define UDA1380_REG_PWRCTRL	0x02
#define UDA1380_REG_ANAMIX	0x03
#define UDA1380_REG_HEADAMP	0x04
#define UDA1380_REG_MSTRVOL	0x10
#define UDA1380_REG_MIXVOL	0x11
#define UDA1380_REG_MODEBBT	0x12
#define UDA1380_REG_MSTRMUTE  0x13
#define UDA1380_REG_MIXSDO	  0x14
#define UDA1380_REG_DECVOL	  0x20
#define UDA1380_REG_PGA		  0x21
#define UDA1380_REG_ADC		  0x22
#define UDA1380_REG_AGC		  0x23
#define UDA1380_REG_L3		  0x7f
#define UDA1380_REG_HEADPHONE 0x18
#define UDA1380_REG_DEC		  0x28

// LM49450 Definitions // 
#define LM49450_VOLUME_LEVELs	32	/* HW Dependent */
#define LM49450_SLAVE_ADR		0x7D
#define LM49450_CMD_BUFF_SIZE	2

/** LM49450 Registers **/
#define LM49450_MODE_CONTROL_REG	0x00
#define LM49450_CLOCK_REG			0x01
#define LM49450_CP_FREQ_REG			0x02
#define LM49450_I2S_MODE_REG		0x03
#define LM49450_I2S_CLOCK_REG		0x04
#define LM49450_HP_3D_CONTROL_REG	0x05
#define LM49450_SPK_3D_CONTROL_REG	0x06
#define LM49450_HP_VOL_CONTROL_REG	0x07
#define LM49450_SPK_VOL_CONTROL_REG	0x08

// LM49450 DAC Modes (Oversampling Rate) //
#define LM49450_DAC_OSR_32		0x3		
#define LM49450_DAC_OSR_64		0x2		
#define LM49450_DAC_OSR_128		0x1		
#define LM49450_DAC_OSR_125		0x0		

#define LM49450_DAC_OSR_BITs_POS		5	
#define LM49450_DAC_OSR_BITs_MASK		0x3

#define RDIV_SETTIMG_MASK			((0x1<<6)-1)
#define RDIV_SETTING_BIT_POST		0
#define RDIV_SETTING_BIT_LENGTH		6

// UDA1380_REG_EVALCLK bit defines
#define EVALCLK_ADC_EN           0x0800  // Enable ADC clock
#define EVALCLK_DEC_EN           0x0400  // Enable decimator clock
#define EVALCLK_DAC_EN           0x0200  // Enable DAC clock
#define EVALCLK_INT_EN           0x0100  // Enable interpolator clock
#define EVALCLK_ADC_SEL_WSPLL   0x0020  // Select SYSCLK input for ADC clock
#define EVALCLK_ADC_SEL_SYSCLK    0x0000  // Select WSPLL clock for ADC clock
#define EVALCLK_DAC_SEL_WSPLL   0x0010  // Select SYSCLK input for DAC clock
#define EVALCLK_DAC_SEL_SYSCLK    0x0000  // Select WSPLL clock for DAC clock
#define EVALCLK_SYSDIV_SEL(n)    ((n) << 2) // System clock input divider select
#define EVALCLK_WSPLL_SEL6_12K   0x0000  // WSPLL input freq selection = 6.25 to 12.5K
#define EVALCLK_WSPLL_SEL12_25K  0x0001  // WSPLL input freq selection = 12.5K to 25K
#define EVALCLK_WSPLL_SEL25_50K  0x0002  // WSPLL input freq selection = 25K to 50K
#define EVALCLK_WSPLL_SEL50_100K 0x0003  // WSPLL input freq selection = 50K to 100K

// UDA1380_REG_I2S
#define I2S_SFORI_I2S 0x0000
#define I2S_SFORI_LSB16 0x0100
#define I2S_SFORI_LSB18 0x0200
#define I2S_SFORI_LSB20 0x0300
#define I2S_SFORI_MSB 0x0500
#define I2S_SFORI_MASK 0x0700
#define I2S_SFORO_I2S 0x0000
#define I2S_SFORO_LSB16 0x0001
#define I2S_SFORO_LSB18 0x0002
#define I2S_SFORO_LSB20 0x0003
#define I2S_SFORO_LSB24 0x0004
#define I2S_SFORO_MSB 0x0005
#define I2S_SFORO_MASK 0x0007
#define I2S_SEL_SOURCE 0x0040
#define I2S_SIM 0x0010

// UDA1380_REG_PWRCTRL bit defines 
#define PWR_PON_PLL_EN           0x8000  // WSPLL enable
#define PWR_PON_HP_EN            0x2000  // Headphone driver enable
#define PWR_PON_DAC_EN           0x0400  // DAC power enable
#define PWR_PON_BIAS_EN          0x0100  // Power on bias enable (for ADC, AVC, and FSDAC)
#define PWR_EN_AVC_EN            0x0080  // Analog mixer enable
#define PWR_PON_AVC_EN           0x0040  // Analog mixer power enable
#define PWR_EN_LNA_EN            0x0010  // LNA and SDC power enable
#define PWR_EN_PGAL_EN           0x0008  // PGA left power enable
#define PWR_EN_ADCL_EN           0x0004  // ADC left power enable
#define PWR_EN_PGAR_EN           0x0002  // PGA right power enable
#define PWR_EN_ADCR_EN           0x0001  // ADC right power enable

// UDA1380_REG_MSTRMUTE bit defines
#define MSTRMUTE_MTM_MUTE_EN     0x4000  // Master mute enable
#define MSRTMUTE_CHANNEL2_MUTE_EN 0x0800 
#define MSRTMUTE_CHANNEL1_MUTE_EN 0x0008


// UDA1380_REG_MODEBBT bit defines
#define MODEBBT_BOOST_FLAT       0x0000  // Bits for selecting flat boost
#define MODEBBT_BOOST_FULL       0xC000  // Bits for selecting maximum boost
#define MODEBBT_BOOST_MASK       0xC000  // Bits for selecting boost mask

#define ACODEC_FUNC_OK          0
#define ACODEC_FUNC_ERR         -1


int32_t LM49450_Init(uint32_t i2cClockFreq, uint32_t i2sClockFreq);
int8_t LM49450_volume_set(uint8_t new_val, uint8_t play_sample);
int32_t Uda1380_Init(uint32_t i2cClockFreq, uint32_t i2sClockFreq);
int32_t ACodec_WriteData(uint8_t acodec_type, uint8_t reg, uint16_t data);
int32_t ACodec_ReadData(uint8_t acodec_type, uint8_t reg, uint16_t *data);
int32_t Uda1380_Mute(Bool MuteOn);
/**
 * @}
 */

#endif /* _UDA1380_H_ */
