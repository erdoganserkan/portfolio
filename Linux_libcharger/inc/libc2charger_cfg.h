/*
 * libc2charger_cfg.h
 *
 *  Created on: Dec 8, 2014
 *      Author: serkan
 */

#ifndef LIBC2CHARGER_CFG_H_
#define LIBC2CHARGER_CFG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define LIBC2CHARGER_CFG_FILE_PATH	"/root/libc2charger.cfg"

typedef enum
{
	POWER_SWITCH_MV	= 0,
	BATTERY_SAMPLE_MV,
	VBAT_POINT_MV,
	PACK_2S
}libc2charger_config_str_type;
static const char *libc2charger_config_strs[] = {"power_switchMV", "battery_sampleMV", "vbat_pointMV", "pack_2s"};


#endif /* LIBC2CHARGER_CFG_H_ */
