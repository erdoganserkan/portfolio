/*
    i2cget.c - A user-space program to read an I2C register.
    Copyright (C) 2005-2010  Jean Delvare <khali@linux-fr.org>

    Based on i2cset.c:
    Copyright (C) 2001-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2005  Jean Delvare <khali@linux-fr.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <log.h>

#include "../../Linux_libi2c/inc/i2cbusses.h"
#include "../../Linux_libi2c/inc/util.h"

static int help(mj_log_type log_instance);

static int help(mj_log_type log_instance)
{
	ERRL(log_instance,
		"Usage: i2cget [-f] [-y] I2CBUS CHIP-ADDRESS [DATA-ADDRESS [MODE]]\n"
		"  I2CBUS is an integer or an I2C bus name\n"
		"  ADDRESS is an integer (0x03 - 0x77)\n"
		"  MODE is one of:\n"
		"    b (read byte data, default)\n"
		"    w (read word data)\n"
		"    c (write byte/read byte)\n"
		"    Append p for SMBus PEC\n");
	return -1;
}

static int check_funcs(mj_log_type log_instance, int file, int size, int daddress, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		ERRL(log_instance, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "SMBus receive byte");
			return -1;
		}
		if (daddress >= 0
		 && !(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "SMBus read byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "SMBus read word");
			return -1;
		}
		break;
	}

	if (pec
	 && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
		ERRL(log_instance, "Warning: Adapter does "
			"not seem to support PEC\n");
	}

	return 0;
}

static int confirm(mj_log_type log_instance, const char *filename, int address, int size, int daddress,
		   int pec)
{
	int dont = 0;

	TRACEL(log_instance, "WARNING! This program can confuse your I2C "
		"bus, cause data loss and worse!\n");

	/* Don't let the user break his/her EEPROMs */
	if (address >= 0x50 && address <= 0x57 && pec) {
		ERRL(log_instance, "STOP! EEPROMs are I2C devices, not "
			"SMBus devices. Using PEC\non I2C devices may "
			"result in unexpected results, such as\n"
			"trashing the contents of EEPROMs. We can't "
			"let you do that, sorry.\n");
		return 0;
	}

	if (size == I2C_SMBUS_BYTE && daddress >= 0 && pec) {
		ERRL(log_instance, "WARNING! All I2C chips and some SMBus chips "
			"will interpret a write\nbyte command with PEC as a"
			"write byte data command, effectively writing a\n"
			"value into a register!\n");
		dont++;
	}
	{
		char temp[32]; memset(temp, 0, sizeof(temp));
		char logbuf[128]; memset(logbuf, 0, sizeof(logbuf));
		sprintf(logbuf, "I will read from device file %s, chip "
			"address 0x%02x, ", filename, address);
		if (daddress < 0) {
			strcat(logbuf, "This is current data : address");
		} else {
			sprintf(logbuf, "This is data address : 0x%02x", daddress);
			strcat(logbuf, temp);
		}
		sprintf(temp, ", using %s.\n",(size == I2C_SMBUS_BYTE) ? \
			((daddress < 0) ? "read byte" : "write byte/read byte") : \
				((size == I2C_SMBUS_BYTE_DATA) ? "read byte data" : "read word data"));
		strcat(logbuf, temp);
		TRACEL(log_instance,"%s", logbuf);
		if (pec)
			ERRL(log_instance, "PEC checking enabled.\n");
	}
#if(0)
	ERRL(log_instance, "Continue? [%s] ", dont ? "y/N" : "Y/n");
	if (!user_ack(!dont)) {
		ERRL(log_instance, "Aborting on user request.\n");
		return 0;
	}
#endif
	return 1;
}

int i2cget(mj_log_type log_instance, int argc, char *argv[])
{
	char *end;
	int res, i2cbus, address, size, file;
	int daddress;
	char filename[20];
	int pec = 0;
	int flags = 0;
	int force = 0, yes = 0, version = 0;

	/* handle (optional) flags first */
	while (1+flags < argc && argv[1+flags][0] == '-') {
		switch (argv[1+flags][1]) {
		case 'V': version = 1; break;
		case 'f': force = 1; break;
		case 'y': yes = 1; break;
		default:
			ERRL(log_instance, "Error: Unsupported option \"%s\"!\n", argv[1+flags]);
			return help(log_instance);
		}
		flags++;
	}

	if (version) {
		ERRL(log_instance, "i2cget version %s\n", VERSION);
		return -1;
	}

	if (argc < flags + 3)
		return  help(log_instance);

	i2cbus = lookup_i2c_bus(log_instance, argv[flags+1]);
	if (i2cbus < 0)
		return  help(log_instance);

	address = parse_i2c_address(log_instance, argv[flags+2]);
	if (address < 0)
		return  help(log_instance);

	if (argc > flags + 3) {
		size = I2C_SMBUS_BYTE_DATA;
		daddress = strtol(argv[flags+3], &end, 0);
		if (*end || daddress < 0 || daddress > 0xff) {
			ERRL(log_instance, "Error: Data address invalid!\n");
			return  help(log_instance);
		}
	} else {
		size = I2C_SMBUS_BYTE;
		daddress = -1;
	}

	if (argc > flags + 4) {
		switch (argv[flags+4][0]) {
		case 'b': size = I2C_SMBUS_BYTE_DATA; break;
		case 'w': size = I2C_SMBUS_WORD_DATA; break;
		case 'c': size = I2C_SMBUS_BYTE; break;
		default:
			ERRL(log_instance, "Error: Invalid mode!\n");
			return  help(log_instance);
		}
		pec = argv[flags+4][1] == 'p';
	}

	file = open_i2c_dev(log_instance, i2cbus, filename, sizeof(filename), 0);
	if (file < 0
	 || check_funcs(log_instance, file, size, daddress, pec)
	 || set_slave_addr(log_instance, file, address, force))
		return -1;

	if (!yes && !confirm(log_instance, filename, address, size, daddress, pec))
		return -1;

	if (pec && ioctl(file, I2C_PEC, 1) < 0) {
		ERRL(log_instance, "Error: Could not set PEC: %s\n",
			strerror(errno));
		close(file);
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (daddress >= 0) {
			res = i2c_smbus_write_byte(file, daddress);
			if (res < 0)
				ERRL(log_instance, "Warning - write failed\n");
		}
		res = i2c_smbus_read_byte(file);
		break;
	case I2C_SMBUS_WORD_DATA:
		res = i2c_smbus_read_word_data(file, daddress);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		res = i2c_smbus_read_byte_data(file, daddress);
	}
	close(file);

	if (res < 0) {
		ERRL(log_instance, "Error: Read failed\n");
		return -2;
	}

	TRACEL(log_instance,"0x%0*x\n", size == I2C_SMBUS_WORD_DATA ? 4 : 2, res);

	return res;
}
