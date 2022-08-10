#include "system_LPC177x_8x.h"
#include <math.h>
#include "lpc177x_8x_systick.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_clkpwr.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include <task.h>
#include <queue.h>
#include "BSP.h"
#include "GLCD.h"
#include "GUIDEMO.h"
#include "UMDShared.h"
#include "AppCommon.h"
#include "AppSettings.h"
#include "AppFont.h"
#include "debug_frmwrk.h"
#include "Serial.h"
#include "monitor.h"
#include "GuiConsole.h"
#include "led.h"
#include "Communication.h"
#include "RuntimeLoader.h"
#include "UartInt.h"
#include "GB.h"
#include "OTOSearch.h"
#include "MAMinSTDSearch.h"
#include "DepthCalc.h"
#include "SYSSettings.h"
#include "ATRadar.h"
#include "APlay.h"
#include "Analog.h"
#include "Intro2.h"
#include "ATMenu.h"
#include "ATAutoFreq.h"
#include "ATManuelFreq.h"
#include "ATLoading.h"
#include "ATDistance.h"
#include "TSTask.h"
#include "Dac.h"
#include "ATBrightVol.h"
#include "ATLang.h"
#include "SYSLoading.h"
#include "DevSelect.h"
#include "AZPLoading.h"
#include "AZPAllMetal.h"
#include "AZPDisc.h"
#include "AZPFast.h"
#include "AZPMenu.h"

/* External Resources -------------------------*/
extern uint8_t RadialMenu(void); 
extern void fatFS_SysTick_Handler_1ms (void);	// fatFS scheduler function // 
extern void BSP_Init_IRQ_Priorities(void);	// BSP.c // 

/* Private define ------------------------------------------------------------*/
static void vGUITask(void *pvArg);

static TaskHandle_t giu_task_handle = NULL;
EventGroupHandle_t xRendezvousBits = NULL;	// Used for Task Rendezvous //
uint8_t active_page = AZP_LOADING_SCR;
uint8_t prev_page = 0xFF;
int detector_power_on_time_ms = 0;
uint8_t SearchState = SEARCH_IDLE;

/* Shared Definies -------------------------*/
xQueueHandle	GUI_Task_Queue = (xQueueHandle)NULL;
uint8_t expo_gauges[UMD_GAUGE_MAX+1];

#define EXPO_GAUGE_START_GAUGE 10
static void fill_expo_gauges(void) {
	uint8_t volatile indx;
	for(indx=1; indx <= UMD_GAUGE_MAX ; indx++) {
		uint8_t res = (UMD_GAUGE_MAX+1) * sin(((double)(indx+10))/((double)(22.5*M_PI))); 
		if(res > UMD_GAUGE_MAX)
			res = UMD_GAUGE_MAX;
		expo_gauges[indx] = res;
	}
	for(indx=EXPO_GAUGE_START_GAUGE ; indx ; indx--){
		uint8_t diff = ((expo_gauges[indx] - indx) * (EXPO_GAUGE_START_GAUGE-indx))/EXPO_GAUGE_START_GAUGE;
		expo_gauges[indx] -= diff;
	}
	expo_gauges[0]=0;
	expo_gauges[UMD_GAUGE_MAX]=UMD_GAUGE_MAX;
}

/*********************************************************************
*
*       MainTask
*/
int main(void) 
{
	if((SDRAM_MAX_ADDR_ - SDRAM_BASE_ADDR) > SDRAM_SIZE) {
		// "SDRAM USAGE is OUT OF RANGE(32MB)" //
		while(1);
	}
	fill_expo_gauges();
	
	monitor_init();	// GLOBAL Debugging resource (printf over seral port or LCD) initialization // 
	SerialInit();	// Debug framework (terminal) related task @ UART0 //
	
	// If debug messages are printed on LCD, call these functions as earlier as possible // 
  	WM_SetCreateFlags(WM_CF_MEMDEV);		// Can be called before GUI_init() // 
	GUI_Init();		// Call this function ONLY ONCE during application lifecycle //

	ResLoadInit();	// Runtime Resource Loading Initialization //
	App_ReloadSettings(NULL, RELOAD_FROM_NVMEM);	// Init stored data related resources from flash-memory // 
	if(APP_IS_DETECTOR == APP_GetValue(ST_DEV_TYPE))
		BSP_PWMSet(0, BSP_PWM_LCD, APP_GetValue(ST_BRIGHT));	// Apply stored LCD Backlight level // 
	else 
		BSP_PWMSet(0, BSP_PWM_LCD, APP_GetValue(ST_AT_BRIGTH));	// Apply stored LCD Backlight level // 
	App_SetHWTypeStates(FALSE, FALSE);	// power off filed scanner and enable dedector // 

	TRACEM("ALL_TASK_BITs (0x%X)\n", ALL_TASK_BITs);
	KS_init();		// Keypadd scanning and event generation task //
	#if(TRUE == TS_TASK_STATE)
		TSTaskInit();	// Touch Screen Polling Task // 
	#endif
	#if(TRUE == LED_TASK_STATE)
		LedInit();		// Led blinking related debug purposed task //
	#endif
	CommInit();		// Detector communication task @UART1 (Binary message send & receive) //  

	xRendezvousBits = xEventGroupCreate();	// Call before scheduler starts // 
	xTaskCreate( vGUITask , "GUITask" , GUI_TASK_STACK_SIZE , NULL , GUI_TASK_PRIORITY , &giu_task_handle );
	/* Start the scheduler. */
	vTaskStartScheduler();	// WARNING: If there is not enough heap memory to create IDLE task this function returns // 
								// It should never returns at normal condition // 
	for(;;)
		CLKPWR_Sleep();
}

	static void vGUITask(void *pvArg)
{
	GUI_Task_Queue = xQueueCreate(GUI_TASK_MESSAGE_QUEUE_SIZE, UMD_FIXED_PACKET_LENGTH);
	if(NULL == GUI_Task_Queue) {
		while(1);	// GUI task message queue creation failed due to unsufficient heap size // 
	}
	if(NULL != xRendezvousBits) {
		EventBits_t uxReturn;
		TickType_t xTicksToWait = RENDEZVOUS_WAIT_MS / portTICK_PERIOD_MS;
		uxReturn = xEventGroupSync( xRendezvousBits, (0x1<<UMD_TASK_GUI), ALL_TASK_BITs, xTicksToWait );
		if(ALL_TASK_BITs == (uxReturn & ALL_TASK_BITs)) {
			/* All three tasks reached the synchronisation point before the call
			to xEventGroupSync() timed out. */
		}
		else
			while(TODO_ON_ERR);	// All TASKs failed to reach RENDEZVOUS point in expected time // 
	}
	else
		while(TODO_ON_ERR);	// There is NO necessary heap memory // 
	
	#if(TRUE == INTRO_STATE)
		// ?? // 
	#endif	
	
 	GUI_UC_SetEncodeUTF8();	// Enable UTF-8 encoding, required for language support //
	GUI_UC_EnableBIDI(1);	// Arabic & Persian languages requires bidirectional rendering support 

	// Initialize all screen shared objects // 
	if(0 != InitGroupRes(RUNTIME_FONTs, 0xFF))	
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(AUDIO_TRACs, 0xFF))	
		while(TODO_ON_ERR);
	audio_play_init();	// Because of noise on DAC @ startup, we do DAC init here // 
		
	detector_power_on_time_ms = GUI_X_GetTime();
	App_SetHWTypeStates(TRUE, FALSE);	// power off filed scanner and enable dedector // 

	while(1) {
		switch(active_page) {
			case AZP_ALL_METAL_SCR:
				active_page = AZP_AllMetal();
				prev_page = AZP_ALL_METAL_SCR;
				break;
			case AZP_DISC_SCR: // Grafik Ayrimi Ekrani // 
				active_page = AZP_Disc();
				prev_page = AZP_DISC_SCR;
				break;
			case AZP_FAST_SCR: // Rakamli Ayrim Ekrani // 
				active_page = AZP_Fast();
				prev_page = AZP_FAST_SCR;
				break;
			case AZP_MENU_SCR: // Ayarlar Ekrani // 
				active_page = AZP_Menu();
				prev_page = AZP_MENU_SCR;
				break;
			case AZP_LOADING_SCR:
				active_page = AZP_Loading();
				prev_page = AZP_LOADING_SCR;
				break;
			default:
				while(STALLE_ON_ERR);
				break;
		}
	} 
}

// This function is called by FreeRTOS kernel @ every time systick interrupts occurs // 
void vApplicationTickHook( void )
{
	extern volatile int OS_TimeMS;	// Defined in GUI_X.c // 
	OS_TimeMS++;	// EmWin required job on each systick ///
	fatFS_SysTick_Handler_1ms();	// FatFS required job on each systick // 
	handle_uart_rcv_timeout();
}

	#if(0) 
		if(1) {
			DAC_test();
		}
		{
			static uint8_t type = 0;
			static uint16_t wave_f=0, modulator_f=0;
			WAVE_Generator_init(STD_SEARCH_TYPE);
			//audio_play_init();
			WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ,DAC_DEFAULT_AMP);
			//App_waitMS(5);
			while(1) {
				WAVE_Update_FreqAmp_Gauge(rand()%UMD_GAUGE_MAX);
				App_waitMS(1);
			}
			{
				for(modulator_f = 50 ; modulator_f < 150 ; modulator_f += 10) {
					//WAVE_Modulator_Update_Freq(modulator_f);
					for(wave_f = 300; wave_f < 321 ; wave_f += 2) {
						//WAVE_Update_Freq(wave_f);
						WAVE_Generator_start(UMD_GAUGE_MIN, 0, DAC_DEFAULT_AMP);
						App_waitMS(250);
						WAVE_Generator_stop(FALSE, FALSE,TRUE);
						//WAVE_Update_Freq(wave_f);
					}
					continue;
					for(wave_f = 320; wave_f > 300 ; wave_f -= 2) {
						//WAVE_Update_Freq(wave_f);
						WAVE_Generator_start(UMD_GAUGE_MIN, 0, DAC_DEFAULT_AMP);
						App_waitMS(150);
						WAVE_Generator_stop(FALSE, FALSE, TRUE);
						//WAVE_Update_Freq(wave_f);
					}
				}
				App_waitMS(5000);
			} 
		}
	#endif

/*************************** End of file ****************************/

