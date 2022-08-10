/*
 * main.c
 *
 *  Created on: Jan 21, 2014
 *      Author: serkan
 */

#include <stdio.h>
#include <unistd.h>
#include <log.h>

#include "../inc/libadv.h"
#include "../inc/libadv_common.h"
#include "adv7181c.h"

int main(int argc, char* argv[])
{
	uint8_t hdmi=FALSE, as=FALSE, av=FALSE;
	int8_t retval = 0;
	static uint8_t indx = 0;
	if(libc2adv_init(LOG_DBG)) {	// Basic libc2adv resource initialization //
		fprintf(stderr, "libADV_init() FAILED\n");
		return -1;
	}else {
		fprintf(stdout, "libADV_init() COMPLETED\n" );
	}

	if(1 < argc ) {
		if ( strcmp ( argv[1], "all" ) == 0 ) {
			fprintf(stdout, "AS & AV & HDMI init will be executed\n");
			hdmi = as = av = TRUE;
		}
		else if ( strcmp ( argv[1], "hdmi" ) == 0 ) {
			fprintf(stdout, "HDMI init will be executed\n");
			hdmi = TRUE;
		}
		else if ( strcmp ( argv[1], "as" ) == 0 ) {
			fprintf(stdout, "AS init will be executed\n");
			as = TRUE;
		}
		else if ( strcmp ( argv[1], "av" ) == 0 ) {
			fprintf(stdout, "AV init will be executed\n");
			av = TRUE;
		}
	}
	else {
		// default behaviour is HDMI testing //
		fprintf(stdout, "HDMI init will be executed\n");
		hdmi = TRUE;
	}

	if(TRUE == hdmi) {
		retval = hdmi_init(HDMI_AUTO_RESOLUTION, NULL);
		fprintf(stdout, "hdmi_init() retval=%d\n", retval);
	}
	if(TRUE == av) {
		retval = analog_video_init(ADV7181C_CVBS_PAL);
		fprintf(stdout, "AV_init() retval=%d\n", retval);
	}
	if(TRUE == as) {
		retval = analog_sound_init(50, 10, ANALOG_AUDIO_DUAL_MONO);
		fprintf(stdout, "AS_init() retval=%d\n", retval);
	}

	return 0;
}
