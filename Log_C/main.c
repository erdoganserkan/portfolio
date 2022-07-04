#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "utlist.h"
#include "log.h"

static mj_log_type logi = LOG_INSTANCE_FAILED;


int main(int argc, char **argv) {

	logi = mj_log_init_instance(LOG_INFO, "TestLogFile");		// This will create log file and ad dinitial entry into it // 
	
	INFOL(logi, "FILE(%s) is selected:argc(%d)\n", argv[1], argc);	// First log entry is at INFO level // 


	return 0;
}
