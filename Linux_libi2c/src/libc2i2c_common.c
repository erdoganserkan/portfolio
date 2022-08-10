/*
 * libc2i2c_common.c
 *
 *  Created on: Nov 24, 2014
 *      Author: serkan
 */

#include <string.h>
#include "../inc/libi2c_common.h"

// Setting desired bits must be "1" in the smask parameter //
int16_t libc2i2c_smask_reg8(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr, uint8_t smask)
{
	int ret = 0;
	// read old val //
	int16_t regval = libc2i2c_read_reg8(log_instance, bus_num, mapadr, regadr);
	if(regval < 0) {
		ERRL(log_instance, "libc2i2c_read_reg8(0x%02X, 0x%02X) FAILED\n", mapadr<<1, regadr);
		return -1;
	}
	// set desired bits //
	regval |= smask;
	// write back to device //
	if(0 > (ret = libc2i2c_write_reg8(log_instance, bus_num, mapadr, regadr, regval))) {
		ERRL(log_instance, "libc2i2c_write_reg8(0x%02X, 0x%02X, 0x%02X) FAILED\n", mapadr<<1, regadr, regval);
		return -1;
	}

	return ret;
}

// Clearing desired bits must be "1" in the cmask parameter //
int16_t libc2i2c_cmask_reg8(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr, uint8_t cmask)
{
	int ret = 0;
	// read old val //
	int16_t regval = libc2i2c_read_reg8(log_instance, bus_num, mapadr, regadr);
	if(regval < 0) {
		ERRL(log_instance, "libc2i2c_read_reg8(0x%02X, 0x%02X) FAILED\n", mapadr<<1, regadr);
		return -1;
	}
	// clear desired bits //
	regval &= (~cmask);
	// write back to device //
	if(0 > (ret = libc2i2c_write_reg8(log_instance, bus_num, mapadr, regadr, regval))) {
		ERRL(log_instance, "libc2i2c_write_reg8(%d, %d, %d) FAILED\n", mapadr<<1, regadr, regval);
		return -1;
	}

	return ret;
}

int16_t libc2i2c_write_reg8(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr, uint8_t newval)
{
	// update register content //
	char cmd[7][16];
	char *argv[7];
	volatile uint8_t indx;

	TRACEL(log_instance, "mapadr(0x%02X), regadr(0x%02X), newval(0x%02X)\n", (mapadr<<1), regadr, newval);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd[0],"%s", "i2cset");
	sprintf(cmd[1],"%d", bus_num);	// i2c bus that chip connected //
	sprintf(cmd[2],"0x%02X", mapadr);	// map address (internal subsystem address) //
	sprintf(cmd[3],"0x%02X", regadr);	// register address //
	sprintf(cmd[4],"0x%02X", newval);	// register new value //
	sprintf(cmd[5],"%c", 'b');	// byte length operation //
	for(indx=0 ; indx<6 ; indx++)
		argv[indx] = cmd[indx];
	argv[6] = NULL;

	return i2cset(log_instance, 6, argv);
}

int16_t libc2i2c_read_reg8(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr)
{
	// return register content //
	char cmd[6][16];
	char *argv[6];
	volatile uint8_t indx;

	TRACEL(log_instance, "mapadr(0x%02X), regadr(0x%02X)\n", (mapadr<<1), regadr);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd[0],"%s", "i2cget");
	sprintf(cmd[1],"%d", bus_num);	// i2c bus that chip connected //
	sprintf(cmd[2],"0x%02X", mapadr);	// map address (internal subsystem address) //
	sprintf(cmd[3],"0x%02X", regadr);	// register address //
	sprintf(cmd[4],"%c", 'b');	// byte length operation //
	for(indx=0 ; indx<5 ; indx++)
		argv[indx] = cmd[indx];
	argv[5] = NULL;

	return i2cget(log_instance, 5, argv);
}

int8_t libc2i2c_scan_rage(mj_log_type log_instance, uint8_t bus_num)
{
	int16_t retval = libc2i2c_read_reg8(log_instance, bus_num, 0x24, 0x08);
	DEBUGL(log_instance, "read(0x24, 0x08) = %d\n", retval);

	volatile uint8_t indx;
	// Slave Discovery Loop //
	for(indx=0x77;; indx--) {
		INFOL(log_instance, "Testing address(0x%02X)\n", indx);
		if(0 <= libc2i2c_read_reg8(log_instance, bus_num, indx, 0x00)) {
			INFOL(log_instance, "Address(0x%02X) is EXIST\n\n", indx);
		}
		usleep(10000);
		if(indx==0x03)
			break;
	}

	return retval;

}

int32_t libc2i2c_read_reg16(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr)
{
	// return register content //
	char cmd[6][16];
	char *argv[6];
	volatile uint8_t indx;

	DEBUGL(log_instance, "mapadr(0x%02X), regadr(0x%02X)\n", (mapadr<<1), regadr);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd[0],"%s", "i2cget");
	sprintf(cmd[1],"%d", bus_num);	// i2c bus that chip connected //
	sprintf(cmd[2],"0x%02X", mapadr);	// map address (internal subsystem address) //
	sprintf(cmd[3],"0x%02X", regadr);	// register address //
	sprintf(cmd[4],"%c", 'w');	         // word length operation //
	for(indx=0 ; indx<5 ; indx++)
		argv[indx] = cmd[indx];
	argv[5] = NULL;

	return i2cget(log_instance, 5, argv);
}

int16_t libc2i2c_write_reg16(mj_log_type log_instance, uint8_t bus_num, uint8_t mapadr, uint8_t regadr, uint16_t newval)
{
	// update register content //
	char cmd[7][16];
	char *argv[7];
	volatile uint8_t indx;

	DEBUGL(log_instance, "mapadr(0x%02X), regadr(0x%02X), newval(0x%02X)\n", (mapadr<<1), regadr, newval);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd[0],"%s", "i2cset");
	sprintf(cmd[1],"%d", bus_num);	// i2c bus that chip connected //
	sprintf(cmd[2],"0x%02X", mapadr);	// map address (internal subsystem address) //
	sprintf(cmd[3],"0x%02X", regadr);	// register address //
	sprintf(cmd[4],"0x%02X", newval);	// register new value //
	sprintf(cmd[5],"%c", 'w');	         // byte length operation //
	for(indx=0 ; indx<6 ; indx++)
		argv[indx] = cmd[indx];
	argv[6] = NULL;

	return i2cset(log_instance, 6, argv);
}

