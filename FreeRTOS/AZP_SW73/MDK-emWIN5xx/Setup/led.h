#ifndef LED_H
#define LED_H

#include <stdint.h>

// LED INDEXES for set_led() function param1(led_indx) // 
#define LED0	0
#define LED1	1
#define LED2	2
#define LED3	3
#define LED_COUNT		4

extern void LedInit(void);
extern void set_led(uint8_t led_indx, uint8_t state);


#endif
