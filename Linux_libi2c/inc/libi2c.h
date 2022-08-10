/*
 * libc2i2c.h
 *
 *  Created on: Nov 24, 2014
 *      Author: serkan
 */

#ifndef LIBC2I2C_H_
#define LIBC2I2C_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <log.h>

#define TRY_READ(log_instance, bus_num, ret, mapadr, regadr, label) \
	if(0 > (ret = libc2i2c_read_reg8(log_instance, bus_num, mapadr, regadr))) {\
		ERRL(log_instance, "libc2i2c_read_reg8(0x%02X, 0x%02X) FAILED (%s)\n", mapadr, regadr, strerror(errno));\
		goto label;\
	}
#define TRY_WRITE(log_instance, bus_num, mapadr, regadr, val, label) \
	if(0 > libc2i2c_write_reg8(log_instance, bus_num, mapadr, regadr, val)) {\
		ERRL(log_instance, "libc2i2c_write_reg8(0x%02X, 0x%02X, 0x%02X) FAILED (%s)\n", mapadr, regadr, val, strerror(errno));\
		goto label;\
	}
#define TRY_SMASK(log_instance, bus_num, mapadr, regadr, smask, label) \
	if(0 > libc2i2c_smask_reg8(log_instance, bus_num, mapadr, regadr, smask)) {\
		ERRL(log_instance, "libc2i2c_smask_reg8(0x%02X, 0x%02X, 0x%02X) FAILED (%s)\n", mapadr, regadr, smask, strerror(errno));\
		goto label;\
	}
#define TRY_CMASK(log_instance, bus_num, mapadr, regadr, cmask, label) \
	if(0 > libc2i2c_cmask_reg8(log_instance, bus_num, mapadr, regadr, cmask)) {\
		ERRL(log_instance, "libc2i2c_cmask_reg8(0x%02X, 0x%02X, 0x%02X) FAILED (%s)\n", mapadr, regadr, cmask, strerror(errno));\
		goto label;\
	}
#define TRY_READ16(log_instance, bus_num, ret, mapadr, regadr, label) \
	if(0 > (ret = libc2i2c_read_reg16(log_instance, bus_num, mapadr, regadr))) {\
		ERRL(log_instance, "libc2i2c_read_reg16(0x%02X, 0x%02X) FAILED (%s)\n", mapadr, regadr, strerror(errno));\
		goto label;\
	}
#define TRY_WRITE16(log_instance, bus_num, mapadr, regadr, val, label) \
	if(0 > libc2i2c_write_reg16(log_instance, bus_num, mapadr, regadr, val)) {\
		ERRL(log_instance, "libc2i2c_write_reg16(0x%02X, 0x%02X, 0x%04X) FAILED (%s)\n", mapadr, regadr, val, strerror(errno));\
		goto label;\
	}

extern int16_t libc2i2c_smask_reg8(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr, uint8_t smask);
extern int16_t libc2i2c_cmask_reg8(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr, uint8_t cmask);
extern int16_t libc2i2c_write_reg8(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr, uint8_t newval);
extern int16_t libc2i2c_read_reg8(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr);
extern int32_t libc2i2c_read_reg16(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr);
extern int16_t libc2i2c_write_reg16(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr, uint16_t newval);
extern int8_t libc2i2c_scan_rage(mj_log_type log_instance, uint8_t bus_num);

#endif /* LIBC2I2C_H_ */
