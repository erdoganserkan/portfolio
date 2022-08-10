#ifndef LIBADV7611_COMMON_H
#define LIBADV7611_COMMON_H

#include <stdio.h>
#include <stdint.h>

#ifndef FALSE
	#define FALSE		(0)
#endif
#ifndef TRUE
	#define TRUE		(1)
#endif

#define ADV7611_INIT	TRUE
#define ADC7181C_INIT	TRUE
#define CS53L21_INIT	TRUE

#define DelayMS(x)	usleep((unsigned int)x*1000);

typedef unsigned char Bool;

#define I2C_BUS_LIBADV			(3)


#endif	/* End of LIBADV7611_COMMON_H */
