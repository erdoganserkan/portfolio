/*
    i2cset.c - A user-space program to write an I2C register.
    Copyright (C) 2001-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2010  Jean Delvare <khali@linux-fr.org>

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
	ERRL(log_instance,"Usage: i2cset [-f] [-y] [-m MASK] I2CBUS CHIP-ADDRESS DATA-ADDRESS [VALUE] ... [MODE]\n"
		"  I2CBUS is an integer or an I2C bus name\n"
		"  ADDRESS is an integer (0x03 - 0x77)\n"
		"  MODE is one of:\n"
		"    c (byte, no value)\n"
		"    b (byte data, default)\n"
		"    w (word data)\n"
		"    i (I2C block data)\n"
		"    s (SMBus block data)\n"
		"    Append p for SMBus PEC\n");
	return -1;
}

static int check_funcs(mj_log_type log_instance, int file, int size, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		ERRL(log_instance,"Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "SMBus write byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "SMBus write word");
			return -1;
		}
		break;

	case I2C_SMBUS_BLOCK_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "SMBus block write");
			return -1;
		}
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) {
			ERRL(log_instance, MISSING_FUNC_FMT, "I2C block write");
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
		   int value, int vmask, const unsigned char *block, int len,
		   int pec)
{
	int dont = 0;

	TRACEL(log_instance, "WARNING! This program can confuse your I2C "
		"bus, cause data loss and worse!\n");

	if (address >= 0x50 && address <= 0x57) {
		ERRL(log_instance, "DANGEROUS! Writing to a serial "
			"EEPROM on a memory DIMM\nmay render your "
			"memory USELESS and make your system "
			"UNBOOTABLE!\n");
		dont++;
	}

	TRACEL(log_instance, "I will write to device file %s, chip address "
		"0x%02x, data address\n0x%02x, ", filename, address, daddress);
	if (size == I2C_SMBUS_BYTE) {
		ERRL(log_instance, "no data.\n");
	}
	else if ((size == I2C_SMBUS_BLOCK_DATA) || (size == I2C_SMBUS_I2C_BLOCK_DATA)) {
		int i;
		char temp[32]; memset(temp, 0, sizeof(temp));
		char logbuf[128]; memset(logbuf, 0, sizeof(logbuf));
		sprintf(logbuf, "data ");
		for (i = 0; i < len; i++) {
			sprintf(temp, " 0x%02x", block[i]);
			 strcat(logbuf, temp);
		}
		sprintf(temp, ", mode %s.\n", (size == I2C_SMBUS_BLOCK_DATA) ? "smbus block" : "i2c block");
		strcat(logbuf, temp);
		TRACEL(log_instance,"%s\n", logbuf);
	} else
		TRACEL(log_instance, "data 0x%02x%s, mode %s.\n", value,
			vmask ? " (masked)" : "", (size == I2C_SMBUS_BYTE_DATA) ? "byte" : "word");
	if (pec)
		TRACEL(log_instance, "PEC checking enabled.\n");

#if(0)
	ERRL(log_instance, "Continue? [%s] ", dont ? "y/N" : "Y/n");
	if (!user_ack(!dont)) {
		ERRL(log_instance, "Aborting on user request.\n");
		return 0;
	}
#endif

	return 1;
}

int i2cset(mj_log_type log_instance, int argc, char *argv[])
{
	char *end;
	const char *maskp = NULL;
	int res, i2cbus, address, size, file;
	int value, daddress, vmask = 0;
	char filename[20];
	int pec = 0;
	int flags = 0;
	int force = 0, yes = 0, version = 0, readback = 0;
	unsigned char block[I2C_SMBUS_BLOCK_MAX];
	int len;

	/* handle (optional) flags first */
	while (1+flags < argc && argv[1+flags][0] == '-') {
		switch (argv[1+flags][1]) {
		case 'V': version = 1; break;
		case 'f': force = 1; break;
		case 'y': yes = 1; break;
		case 'm':
			if (2+flags < argc)
				maskp = argv[2+flags];
			flags++;
			break;
		case 'r': readback = 1; break;
		default:
			ERRL(log_instance, "Error: Unsupported option "
				"\"%s\"!\n", argv[1+flags]);
			return help(log_instance);
		}
		flags++;
	}

	if (version) {
		ERRL(log_instance, "i2cset version %s\n", VERSION);
		return (0);
	}

	if (argc < flags + 4)
		return help(log_instance);

	i2cbus = lookup_i2c_bus(log_instance, argv[flags+1]);
	if (i2cbus < 0)
		return help(log_instance);

	address = parse_i2c_address(log_instance, argv[flags+2]);
	if (address < 0)
		return help(log_instance);

	daddress = strtol(argv[flags+3], &end, 0);
	if (*end || daddress < 0 || daddress > 0xff) {
		ERRL(log_instance, "Error: Data address invalid!\n");
		return help(log_instance);
	}

	/* check for command/mode */
	if (argc == flags + 4) {
		/* Implicit "c" */
		size = I2C_SMBUS_BYTE;
	} else if (argc == flags + 5) {
		/* "c", "cp",  or implicit "b" */
		if (!strcmp(argv[flags+4], "c")
		 || !strcmp(argv[flags+4], "cp")) {
			size = I2C_SMBUS_BYTE;
			pec = argv[flags+4][1] == 'p';
		} else {
			size = I2C_SMBUS_BYTE_DATA;
		}
	} else {
		/* All other commands */
		if (strlen(argv[argc-1]) > 2
		    || (strlen(argv[argc-1]) == 2 && argv[argc-1][1] != 'p')) {
			ERRL(log_instance, "Error: Invalid mode '%s'!\n", argv[argc-1]);
			return help(log_instance);
		}
		switch (argv[argc-1][0]) {
		case 'b': size = I2C_SMBUS_BYTE_DATA; break;
		case 'w': size = I2C_SMBUS_WORD_DATA; break;
		case 's': size = I2C_SMBUS_BLOCK_DATA; break;
		case 'i': size = I2C_SMBUS_I2C_BLOCK_DATA; break;
		default:
			ERRL(log_instance, "Error: Invalid mode '%s'!\n", argv[argc-1]);
			return help(log_instance);
		}
		pec = argv[argc-1][1] == 'p';
		if (size == I2C_SMBUS_BLOCK_DATA || size == I2C_SMBUS_I2C_BLOCK_DATA) {
			if (pec && size == I2C_SMBUS_I2C_BLOCK_DATA) {
				ERRL(log_instance, "Error: PEC not supported for I2C block writes!\n");
				return help(log_instance);
			}
			if (maskp) {
				ERRL(log_instance, "Error: Mask not supported for block writes!\n");
				return help(log_instance);
			}
			if (argc > (int)sizeof(block) + flags + 5) {
				ERRL(log_instance, "Error: Too many arguments!\n");
				return help(log_instance);
			}
		} else if (argc != flags + 6) {
			ERRL(log_instance, "Error: Too many arguments!\n");
			return help(log_instance);
		}
	}

	len = 0; /* Must always initialize len since it is passed to confirm() */

	/* read values from command line */
	switch (size) {
	case I2C_SMBUS_BYTE_DATA:
	case I2C_SMBUS_WORD_DATA:
		value = strtol(argv[flags+4], &end, 0);
		if (*end || value < 0) {
			ERRL(log_instance, "Error: Data value invalid!\n");
			return help(log_instance);
		}
		if ((size == I2C_SMBUS_BYTE_DATA && value > 0xff)
		    || (size == I2C_SMBUS_WORD_DATA && value > 0xffff)) {
			ERRL(log_instance, "Error: Data value out of range!\n");
			return help(log_instance);
		}
		break;
	case I2C_SMBUS_BLOCK_DATA:
	case I2C_SMBUS_I2C_BLOCK_DATA:
		for (len = 0; len + flags + 5 < argc; len++) {
			value = strtol(argv[flags + len + 4], &end, 0);
			if (*end || value < 0) {
				ERRL(log_instance, "Error: Data value invalid!\n");
				return help(log_instance);
			}
			if (value > 0xff) {
				ERRL(log_instance, "Error: Data value out of range!\n");
				return help(log_instance);
			}
			block[len] = value;
		}
		value = -1;
		break;
	default:
		value = -1;
		break;
	}

	if (maskp) {
		vmask = strtol(maskp, &end, 0);
		if (*end || vmask == 0) {
			ERRL(log_instance, "Error: Data value mask invalid!\n");
			return help(log_instance);
		}
		if (((size == I2C_SMBUS_BYTE || size == I2C_SMBUS_BYTE_DATA)
		     && vmask > 0xff) || vmask > 0xffff) {
			ERRL(log_instance, "Error: Data value mask out of range!\n");
			return help(log_instance);
		}
	}

	file = open_i2c_dev(log_instance, i2cbus, filename, sizeof(filename), 0);
	if (file < 0
	 || check_funcs(log_instance, file, size, pec)
	 || set_slave_addr(log_instance, file, address, force))
		return -1;

	if (!yes && !confirm(log_instance, filename, address, size, daddress,
			     value, vmask, block, len, pec))
		return (0);

	if (vmask) {
		int oldvalue;

		switch (size) {
		case I2C_SMBUS_BYTE:
			oldvalue = i2c_smbus_read_byte(file);
			break;
		case I2C_SMBUS_WORD_DATA:
			oldvalue = i2c_smbus_read_word_data(file, daddress);
			break;
		default:
			oldvalue = i2c_smbus_read_byte_data(file, daddress);
		}

		if (oldvalue < 0) {
			ERRL(log_instance, "Error: Failed to read old value\n");
			return -1;
		}

		value = (value & vmask) | (oldvalue & ~vmask);

		if (!yes) {
			ERRL(log_instance, "Old value 0x%0*x, write mask "
				"0x%0*x: Will write 0x%0*x to register "
				"0x%02x\n",
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, oldvalue,
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, vmask,
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, value,
				daddress);

			ERRL(log_instance, "Continue? [Y/n] ");
			if (!user_ack(1)) {
				ERRL(log_instance, "Aborting on user request.\n");
				return -1;
			}
		}
	}

	if (pec && ioctl(file, I2C_PEC, 1) < 0) {
		ERRL(log_instance, "Error: Could not set PEC: %s\n",
			strerror(errno));
		close(file);
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		res = i2c_smbus_write_byte(file, daddress);
		break;
	case I2C_SMBUS_WORD_DATA:
		res = i2c_smbus_write_word_data(file, daddress, value);
		break;
	case I2C_SMBUS_BLOCK_DATA:
		res = i2c_smbus_write_block_data(file, daddress, len, block);
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		res = i2c_smbus_write_i2c_block_data(file, daddress, len, block);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		res = i2c_smbus_write_byte_data(file, daddress, value);
		break;
	}
	if (res < 0) {
		ERRL(log_instance, "Error: Write failed\n");
		close(file);
		return -1;
	}

	if (pec) {
		if (ioctl(file, I2C_PEC, 0) < 0) {
			ERRL(log_instance, "Error: Could not clear PEC: %s\n",
				strerror(errno));
			close(file);
			return -1;
		}
	}

	if (!readback) { /* We're done */
		close(file);
		return (0);
	}

	res = -1;
	switch (size) {
	case I2C_SMBUS_BYTE:
		res = i2c_smbus_read_byte(file);
		value = daddress;
		break;
	case I2C_SMBUS_WORD_DATA:
		res = i2c_smbus_read_word_data(file, daddress);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		res = i2c_smbus_read_byte_data(file, daddress);
	}
	close(file);

	if (res < 0) {
		TRACEL(log_instance,"Warning - readback failed\n");
	} else
	if (res != value) {
		TRACEL(log_instance,"Warning - data mismatch - wrote "
		       "0x%0*x, read back 0x%0*x\n",
		       size == I2C_SMBUS_WORD_DATA ? 4 : 2, value,
		       size == I2C_SMBUS_WORD_DATA ? 4 : 2, res);
	} else {
		TRACEL(log_instance,"Value 0x%0*x written, readback matched\n",
		       size == I2C_SMBUS_WORD_DATA ? 4 : 2, value);
	}

	return (res < 0)?res:0;
}
