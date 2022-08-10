/*
 * libc2i2c_common.h
 *
 *  Created on: Nov 24, 2014
 *      Author: serkan
 */

#ifndef LIBC2I2C_COMMON_H_
#define LIBC2I2C_COMMON_H_

#include "libi2c.h"

#ifndef FALSE
	#define FALSE		(0)
#endif
#ifndef TRUE
	#define TRUE		(1)
#endif

int i2cget(mj_log_type log_instance, int argc, char *argv[]);
int i2cset(mj_log_type log_instance, int argc, char *argv[]);

#endif /* LIBC2I2C_COMMON_H_ */
