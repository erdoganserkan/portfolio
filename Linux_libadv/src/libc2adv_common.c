/*
 * libADVCommon.c
 *
 *  Created on: Jan 17, 2014
 *      Author: serkan
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <log.h>

#include "../inc/libadv_common.h"
#include "adv7181c.h"
#include "adv7611.h"
#include "cs53l21.h"

mj_log_type libADV_log_instance = LOG_INSTANCE_FAILED;

int8_t libc2adv_init(const int log_level)
{
	// init logger //
	if(LOG_INSTANCE_FAILED == libADV_log_instance) {
		libADV_log_instance = ctech_log_init_instance(log_level, LIBC2ADV_LOG_FILE);	// ENABLE LOGs
		if(LOG_INSTANCE_FAILED == libADV_log_instance) {
			fprintf(stderr, "Cannot init logfile_instance (libADV)\r\n");
			return -1;
		}
	}

	return 0;
}



