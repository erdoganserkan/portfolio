#ifndef HD_SERIAL_H
#define HD_SERIAL_H

#include <stdint.h>
#if   defined ( __CC_ARM )
	#pragma anon_unions
#endif

// ----- PACKET CONTENT DEFINITIONs ----- //
#define SYNC_START_INDX		(0)		// START SYNC BYTEs : 0, 1, 2, 3 // 
#define SYNC_STOP_INDX		(18)	// STOP SYNC BYTEs : 18, 19, 20, 21 //
#define DATA_LEN_INDX			(4)
#define CMD_INDX					(5)
#define DATA_START_INDX		(6)
// CRC(Uint8) byte is last byte of packet // 

#define DEF_LCD_BACKLIGHT_FREQ_HZ				(50000)
#define DEF_LCD_BACKLIGHT_DUTY					(100)
#if(MAIN_HW_TYPE == HW_3GBONDING_MB)
	#define DEF_FAN_PWM_DUTY								(40)		
#elif(MAIN_HW_TYPE == HW_FUJITSU_HD_MB)
	#define DEF_FAN_PWM_DUTY								(100)		// Below %80, Thermal stability is getting lower because 
																									// of higher heat generation of 3g modems. 
#endif
#define MODEM_COUNT											(6)
#define MESSAGE_SEND_TIMEOUT_MS					(750)
#define STM_FIXED_LENGTH_PACKETS				(1)

#define LINUX_MCU_CHECK_INTERVAL_MS     750
#define LINUX_MCU_CHECK_COUNT           10

typedef enum
{
	OP_COMPLETED	 = 0,
	OP_FAILED,
	OP_PENDING,
	
	OP_STATE_COUNT
} OP_STATEs;

typedef enum
{
	BAT_CHARGING = 0,		// Battery is charging now // 
	BAT_NOT_CHARGING,		// Battery ONLY discharging // 
	BAT_CHARGE_COMPLETE,	// Battery charging completed // 
	BAT_CHARGE_ERR,			// Battery has charging error //
			
	BAT_STATE_COUNT
} BAT_STATEs;

typedef enum
{
	STATE_DISABLE = 0,
	STATE_ENABLE
} STATEs;

typedef enum
{
	CMD_SEND_BATTEMP	= 0x0,	// omap -> f050 : read temp and send //
	RSP_SEND_BATTEMP	= 0x1,			// f050 -> omap : 
												// data:uint16_t (LE) as mC // 
	
	CMD_SEND_BATPERCENT	= 0x2,		// omap -> f050 : read temp and send //
	RSP_SEND_BATPERCENT	= 0x3,		// f050 -> omap : 
														// data:uint8_t // 

	CMD_SEND_BATSTATE	= 0x4,			// omap -> f050 : read temp and send //
	RSP_SEND_BATSTATE	= 0x5,			// f050 -> omap : 
														// data:uint8_t // 

	CMD_SEND_UNIC_ID	= 0x6,		// omap -> f050 : read unicID of temp sensor & send //
	RSP_SEND_UNIC_ID	= 0x7,		// f050 -> omap : 
												// data = uint32_t (LE) //
	
	CMD_SET_MODEM_STATE	= 0x8,	// omap -> f050 : set modem module new state : 
												// data1 = uint8_t, modem index (1-6)
												// data2 = uint8_t, new modem state, STATEs // 
	RSP_SET_MODEM_STATE	= 0x9,	// f050 -> omap : get modem state operation state report // 
												// data1 = uint8_t, OP_STATEs
												// data2 = uint8_t, modem index (1-6)
												// data3 = uint8_t, new modem state, STATEs // 
	
	CMD_POWER_CYCLE	= 0xA,	// omap -> f050 : set new power state of video subsystem :
																	// data1 = uint8_t, POWER_STATEs 
	RSP_POWER_CYCLE	= 0xB,	// f050 -> omap : get new power state setting operation report // 
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, POWER_STATEs (Last desired one)

	CMD_RESET_MODEM_HUB	= 0xC,
	RSP_RESET_MODEM_HUB	= 0xD,

	CMD_RESET_OTG_HUB	= 0xE,
	RSP_RESET_OTG_HUB	= 0xF,

	CMD_RESET_USBETH	= 0x10,
	RSP_RESET_USBETH	= 0x11,
	
	CMD_SET_LCD_POWER_STATE	= 0x12,	// omap -> f050 : set new power state of LCD 
																	// data1 = uint8_t, POWER_STATEs 
	RSP_SET_LCD_POWER_STATE	= 0x13,	// f050 -> omap : get new lcd power state setting operation report // 
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, POWER_STATEs (Last desired one)

	
	CMD_SET_HB_STATE	= 0x14,	// omap -> f050 : set new state of heart-beat detection :
																	// data1 = uint8_t, STATEs 
	RSP_SET_HB_STATE	= 0x15,	// f050 -> omap : get new hb state setting operation report // 
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, STATEs (Last desired one)
	
	CMD_RESET_ENCODER	= 0x16,	// omap -> f050 : set encoder state :
																	// data1 = uint8_t, STATEs 
	RSP_RESET_ENCODER = 0x17,	// f050 -> omap : get new encoder setting operation report // 
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, STATEs (Last desired one)

	CMD_SET_FPGA_STATE	= 0x18,						// omap -> f050 : set FPGA state :
																	// data1 = uint8_t, STATEs 
	RSP_SET_FPGA_STATE	= 0x19,						// f050 -> omap : get new FPGA state setting operation report // 
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, STATEs (Last desired one)

	CMD_SET_USBETH_STATE	= 0x1A,						// omap -> f050 : set USBETH state :
																	// data1 = uint8_t, STATEs 
	RSP_SET_USBETH_STATE	= 0x1B,						// f050 -> omap : get new USBETH state setting operation report // 
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, STATEs (Last desired one)
																	
	CMD_SET_BACKLIGHT	= 0x1C,						// omap -> f050 : set new lcd backlight level //
																	// data1 = uint8_t, [0-100]
	RSP_SET_BACKLIGHT	= 0x1D,						// f050 -> omap : get new lcd backlight set report //
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, [0-100] (Last desired value)

	CMD_SET_FAN_DUTY	= 0x1E,						// omap -> f050 : set new fan pwm duty level //
																	// data1 = uint8_t, [0-100]
	RSP_SET_FAN_DUTY	= 0x1F,						// f050 -> omap : get new fan pwm duty set report //
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, [0-100] (Last desired value)

	CMD_SET_WIFI_STATE	= 0x20,	// omap -> f050 : set WIFI module new power state : 
												// data1 = uint8_t, new WIFI power state, STATE_ENABLE or STATE_DISABLE // 
	RSP_SET_WIFI_STATE	= 0x21,	// f050 -> omap : get WIFI state set operation report // 
												// data1 = uint8_t, OP_STATEs
												// data2 = uint8_t, new WIFI POWER state, STATEs // 

	CMD_SET_ENCODER_STATE	= 0x22,	// omap -> f050 : set encoder state :
																	// data1 = uint8_t, power state, STATE_ENABLE or STATE_DISABLE //
	RSP_SET_ENCODER_STATE = 0x23,	// f050 -> omap : get new encoder setting operation report //
																	// data1 = uint8_t, OP_STATEs
																	// data2 = uint8_t, STATEs (Last desired one)

	CMD_CHECK_LINUX_MCU   = 0x24,   // omap -> f050 : check linux mcu
	CMD_IS_LINUX_MCU_OK = 0x25,     // f050 -> omap : is linux msu ok ?
	RSP_LINUX_MCU_OK    = 0x26,     // omap -> f050 : data1, uint8_t : MCU_STATE_OK, MCU_STATE_FAIL

	CMD_SEND_SOFT_VERSION	= 0x27,		// omap -> f050 : send mcu software version
																	// f050 -> omap : send linux mcu software version
    RSP_SEND_SOFT_VERSION   = 0x28,     // omap <-> f050
                                                                    // data1 -> uint16_t : lnux mcu version
                                                                    // data1 -> uint8_t : f050 version
	CMD_POWER_OFF		= 0x29,					// omap -> f050 : power off all 
	RSP_POWER_OFF		= 0x2A,					// f050 -> omap : ok or fail, response 
	
	CMD_SET_SDIPATCH_MUTE_STATE	= 0x2B,		// omap -> f050 :	STATE_DISABLE (UNMUTE output), STATE_ENABLE(MUTE output) 
	RSP_SET_SDIPATCH_MUTE_STATE	= 0x2C,		// F050 -> omap : operation status 
	
	
	OMAP_F050_CMD_COUNT
} HD_OMAP_F050_CMDs;


#define STM_PID_LENGTH      4
#define STM_MAX_DATA_LENGTH 12
#define STM_START_ID       ('P'<<24) + ('M'<<16) + ('U'<<8) + ('F') +'\0'
#define STM_STOP_ID        ('P'<<24) + ('M'<<16) + ('U'<<8) + ('L') +'\0'
#pragma pack(1)
typedef struct stm_comm_s {
    union {
			uint8_t start_buf[STM_PID_LENGTH];
			uint32_t startU32;
		};
    uint8_t len;
    uint8_t cmd;
    uint8_t data[STM_MAX_DATA_LENGTH];
    union {
			uint8_t stop_buf[STM_PID_LENGTH];
			uint32_t stopU32;
		};
} stm_comm_t;
#pragma pack()

#define CALC_CRC(result, adr, len) \
{\
	uint8_t crc8 = 0;\
	volatile uint8_t indx = len-1;\
	uint8_t *ptrU8 = (uint8_t *)adr;\
	for(;; indx--) {\
		crc8 ^= ptrU8[indx];\
		if(0 == indx)\
			break;\
	}\
	result = crc8;\
}
#endif
