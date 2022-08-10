#include "lpc_types.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "AppCommon.h"
#include "led.h"

extern EventGroupHandle_t xRendezvousBits;	// Used for Task Rendezvous //
static void vLEDTask(void * pvArg);

__packed typedef struct
{
	uint8_t port;
	uint8_t pin;
} led_type;
/* LED1   P2.21 CORE and SDK Both*/
/* LED2   P1.13 SDK Only */      
/* LED3   P5.0  SDK Only */        
/* LED4   P5.1  SDK Only */    		
static led_type const ledpos[4]={{2,21},{1,13},{5,0},{5,1}};

void LedInit(void)
{
	volatile uint8_t led_indx;
	for(led_indx=0;led_indx<4;led_indx++) {
		PINSEL_ConfigPin(ledpos[led_indx].port,ledpos[led_indx].pin,0);	   /* P2.21 - GPIO */
		GPIO_SetDir(ledpos[led_indx].port, (1<<(ledpos[led_indx].pin)), 1);
		GPIO_SetValue( (ledpos[led_indx].port), (1<<(ledpos[led_indx].pin)) );  
	}
	xTaskCreate( vLEDTask , "LedTask" , LED_TASK_STACK_SIZE , NULL , LED_TASK_PRIORITY , NULL );
}

void set_led(uint8_t led_indx, uint8_t state) 
{
	led_indx = led_indx%4;
	if(!state)	// leds are connected as sink current from pin // 
		GPIO_SetValue( (ledpos[led_indx].port), (1<<(ledpos[led_indx].pin)) );  
	else
		GPIO_ClearValue( (ledpos[led_indx].port), (1<<(ledpos[led_indx].pin)) );  
}

/*******************************************************************************
* Function Name  : vLEDTask
* Description    : LED Task
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void vLEDTask(void * pvArg)
{
	uint8_t direction = TRUE;
	volatile uint8_t indx = 0;

	// Wait for GUI & Other tasks to be ready before entering to main processing loop // 
	if(NULL != xRendezvousBits)
		xEventGroupSync( xRendezvousBits, (0x1<<UMD_TASK_LED), ALL_TASK_BITs, portMAX_DELAY );
	else
		while(TODO_ON_ERR);
	
	while(1)
	{
		for(indx = (direction?0:LED_COUNT-1) ; \
			direction?(indx!=LED_COUNT):(indx!=0xFF) ;\
				direction?indx++:indx--) {
			set_led( indx, 1); 
			vTaskDelay(50 * (configTICK_RATE_HZ/1000));	// 100ms delay // 
			set_led( indx, 0); 
			vTaskDelay(1 * (configTICK_RATE_HZ/1000));	// 100ms delay // 
		}
		direction = (!direction);
	}
}

