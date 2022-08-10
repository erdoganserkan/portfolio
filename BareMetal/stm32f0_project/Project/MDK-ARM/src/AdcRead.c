#include <stdio.h>
#include "stm32f0xx.h"
#include "stm32f0xx_adc.h"
#include "stm32f0xx_dma.h"
#include "AdcRead.h"

#define BAT_TEMP_INDX				0
#define BAT_LEVEL_INDX			1
#define BAT_Vtemp_INDX			2		// Chip temperature //
#define BAT_Vref_INDX				3		// Chip VREF bandgap voltage //
#define BAT_Vbat_INDX				4		// Chip VBAT pin voltage //

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint32_t TempSensVoltmv = 0, VrefIntVoltmv = 0, BatTempVoltmV = 0;
__IO int8_t SysTemp = 0;
__IO uint16_t RegularConvData_Tab[5];	// Channel1(bat temp), Channel5(bat level), Vref, Vbat, Vtemp // 
__IO uint8_t BatteryCapacity = 0;
__IO int8_t BatTemp = 0;
__IO uint16_t BatLevelMV = 0;

#define ADC_CALIB_RAW_30C_ADR			(0x1FFFF7B8)
#define ADC_CALIB_RAW_100C_ADR		(0x1FFFF7B8)
#define ADC_CALIB_TEMP_VAL_LENGTH	(2)
#define TEMP_SENSOR_SLOPE_FRACTION	((0x1)<<16)		// Multiplied with 2^16 = 65536 // 
int32_t temp_sensor_slope = 0;

#define CAPACITY_TABLE_COUNT	(11)
typedef struct
{
	uint32_t voltage_mv;
	uint32_t capacity;
	uint32_t berk_out_mv;	// Berk Electronics charger circuit voltage drop level // 
} bat_capacity_type;
static bat_capacity_type const capacity[CAPACITY_TABLE_COUNT] = {
	{ 3300 * SERIAL_CELL_COUNT, 0, 			10690},	
	{ 3500 * SERIAL_CELL_COUNT, 2900, 	11400},
	{ 3600 * SERIAL_CELL_COUNT, 5000,		11770},
	{ 3700 * SERIAL_CELL_COUNT, 8600,		12140},
	{ 3800 * SERIAL_CELL_COUNT, 36000,	12500},
	{ 3900 * SERIAL_CELL_COUNT, 62000,	12865},
	{ 4000 * SERIAL_CELL_COUNT, 73000,	13230},
	{ 4050 * SERIAL_CELL_COUNT, 83000,	13405},
	{ 4100 * SERIAL_CELL_COUNT, 89000,	13585}, 
	{ 4150 * SERIAL_CELL_COUNT, 94000,	13765}, 
	{ 4200 * SERIAL_CELL_COUNT, 100000,	13940}		
}; /* http://www.powerstream.com/lithuim-ion-charge-voltage.htm */

// Module Internal Resources //
static uint16_t Samples[MOVING_AVG_DEPTH];
static uint16_t MAvgIndx = 0;
static uint32_t SamplesSum = 0;
static volatile uint8_t MAVgInitDone = FALSE;
static uint16_t update_mavg(uint16_t raw_adc_val); 

// Module Internal Functions //
static void init_mavg(uint16_t first_sample);
static void ADC1_CH_DMA_Config(void);
static uint16_t calc_bat_levelmV(uint16_t adcval);
static uint16_t update_mavg(uint16_t raw_adc_val);
static uint8_t get_battery_capacity(uint16_t battery_voltage_mv);

void calc_temp_sensor_slope()
{
#if(0)
	int32_t raw_adc_30C = *((uint16_t *)ADC_CALIB_RAW_30C_ADR);
	int32_t raw_adc_100C = *((uint16_t *)ADC_CALIB_RAW_100C_ADR);
	
	(raw_adc_100C - raw_adc_30C)/()
#endif
}
// Must be called periodically // 
void update_adc(void)
{
		/* Test DMA1 TC flag */
	if((DMA_GetFlagStatus(DMA1_FLAG_TC1)) == RESET )
		return;			
	
	/* Clear DMA TC flag */
	DMA_ClearFlag(DMA1_FLAG_TC1);
	
	/* Process Battery Temp */
	BatTempVoltmV  = (uint32_t)((RegularConvData_Tab[0]* ADC_MAX_VOLTAGE_MV) / 0xFFF); 	// Channel 1 //
	//:TODO: Compare NTC value in the table and find matching temperature value //    

	/* Convert Battery Level Channel to Battery Voltage & Capacity */
	{
		uint16_t mavgVal = update_mavg(RegularConvData_Tab[BAT_LEVEL_INDX]);
		BatLevelMV = calc_bat_levelmV(mavgVal);
		BatteryCapacity = get_battery_capacity(BatLevelMV);
	}

	/* Convert temperature sensor voltage value to real temp */
	#define Vt25_uV (1430000U)
	#define Slope_uVC (4300U)
	TempSensVoltmv = (uint32_t)(((uint32_t)RegularConvData_Tab[2]* ADC_MAX_VOLTAGE_MV) / 0xFFF);
	SysTemp = (Vt25_uV - (TempSensVoltmv*1000U))/Slope_uVC + 25;
	
	/* Convert Vref voltage value in mv */
	VrefIntVoltmv  = (uint32_t)((RegularConvData_Tab[3]* 3300) / 0xFFF);  
}

void Init_ADC(void)
{
	// TIM2 will be used for channel conversion timing // 
	// battery temp (PA1, ADC_IN1) & battery level(PA5, ADC_IN5) reading init // 
	// integrated temperature sensor reading // 
	ADC1_CH_DMA_Config();
}

/************************************/
/************************************
			STATIC FUNCTIONs 
*************************************/
/************************************/
static uint8_t get_battery_capacity(uint16_t battery_voltage_mv)
{
	volatile uint8_t indx = 0;
	uint32_t temp;
	
	// Apply cable loss voltage drop to input value // 
	#if(TRUE == CABLE_LOSS_COMPANSATION)
		battery_voltage_mv += CABLE_LOSS_DECREASE_MV;
		//SLOG("adcval @ BATTERY (%u mV) with CBL , (Pure : %u mV)\n", battery_voltage_mv, \
			battery_voltage_mv - CABLE_LOSS_DECREASE_MV);
	#endif

	SLOG("input battery_voltage_mv(%u)\n", battery_voltage_mv);
	if(capacity[CAPACITY_TABLE_COUNT-1].berk_out_mv <= battery_voltage_mv) {
		temp = 100;	/* Maximum value reached */
		goto FUNC_EXIT;
	}
	else if(capacity[0].berk_out_mv >= battery_voltage_mv ) {
		temp = 0;	/* Minimum Value */
		goto FUNC_EXIT;
	}

	#if(TRUE == BERK_VOLTAGE_DROP_COMPENSATION)
		// Calculate for voltage_vs_capacity table index for VOLTAGE DROP //
		for(indx=0 ;; indx++ ) {
			if(battery_voltage_mv < capacity[indx].berk_out_mv) {
				indx--;
				//SLOG("PASED_INDX(%u) for VOLTAGE_DROP \n", indx);
				break;
			}
		}
		
		// Calculate Real Voltage Drop using voltage_vs_capacity table //
		{
			uint32_t prev_drop = capacity[indx].voltage_mv - capacity[indx].berk_out_mv;
			uint32_t next_drop = capacity[indx+1].voltage_mv - capacity[indx+1].berk_out_mv;
			//SLOG("prev voltage drop = %u ;; next voltage drop = %u mV \n", prev_drop, next_drop);
			temp = (battery_voltage_mv - capacity[indx].berk_out_mv);
			temp *= (next_drop - prev_drop);
			temp /= (capacity[indx+1].berk_out_mv - capacity[indx].berk_out_mv);
			temp = temp + prev_drop;
			//SLOG("calculated voltage_drop = %u mV\n", temp);
		}

		if(NOT_CHARGING_STATE == charge_state) {
			battery_voltage_mv += temp;
			//SLOG("BATTERY (%u mV) with BERK VD, (Pure : %u mV)\n", battery_voltage_mv, \
				battery_voltage_mv - temp);
		}
		else {
			//SLOG("BATTERY (%u mV) without BERK VD\n", battery_voltage_mv);
		}	
	#else
		SLOG("BATTERY (%u mV) without BERK VD\n", battery_voltage_mv);
	#endif

	//------------------------------------------------------------------------//
	// Calculate for voltage_vs_capacity table index for CAPACITY CALCULATION //
	//------------------------------------------------------------------------//
	if(capacity[CAPACITY_TABLE_COUNT-1].voltage_mv <= battery_voltage_mv) {
		temp = 100;	/* Maximum value reached */
		goto FUNC_EXIT;
	}
	else if(capacity[0].voltage_mv >= battery_voltage_mv ) {
		temp = 0;	/* Minimum Value */
		goto FUNC_EXIT;
	}
	for(indx=0 ;; indx++ ) {
		if(battery_voltage_mv < capacity[indx].voltage_mv) {
			indx--;
			//SLOG("PASED_INDX(%u) for CAPACITY \n", indx);
			break;
		}
	}

	temp = (battery_voltage_mv - capacity[indx].voltage_mv);
	temp = temp*(capacity[indx+1].capacity - capacity[indx].capacity);
	temp /= (capacity[indx+1].voltage_mv - capacity[indx].voltage_mv);
	temp = temp + capacity[indx].capacity;
	temp /= 1000;
	
FUNC_EXIT:	
	SLOG("CALCULATED CAPACITY(%u)\n", temp);
	return temp;
}

// Returns battery voltage in means of mV // 
// Use fixed point math as much as possible, stay away from float/double //
static uint16_t calc_bat_levelmV(uint16_t adcval) 
{
	uint32_t adcvalU32 = (uint32_t )adcval;
	
	// Calculate voltage @ adc pin from raw adc data //
	adcvalU32 = (adcvalU32*ADC_MAX_VOLTAGE_MV)>>ADC_RES;	
	//SLOG("adcval @ PIN (%u mV)\n", adcvalU32);
	// Calculate real battery voltage from the voltage @ adc_pin by using voltage division ratio //
	adcvalU32 = (adcvalU32*(ADC_HIGH_RES_OHM + ADC_LOW_RES_OHM))/ADC_LOW_RES_OHM;	
	
	if(adcvalU32 > BATTERY_MAX_VAL_MV) {
		//SLOG("battery voltage(%u) bigger than MAX(%u), truncating it\n", adcvalU32, BATTERY_MAX_VAL_MV);
		adcvalU32 = BATTERY_MAX_VAL_MV;
	}
	
	return (uint16_t)adcvalU32;
}

// Update mAvg data with new raw adc sample // 
static uint16_t update_mavg(uint16_t raw_adc_val) 
{
	uint16_t result;
	
	// Do FIRST Initialization of MAvg // 
	if(MAVgInitDone == FALSE) {
		MAVgInitDone = TRUE;
		init_mavg(raw_adc_val);
	}
	
	//SLOG("%s()-> CALLED with RAWValue(%u)\n", __FUNCTION__, new_sample);
	SamplesSum -= Samples[MAvgIndx];		// Remove last sample from sum //
	SamplesSum += raw_adc_val;				// Add new sample to Sum //	
	Samples[MAvgIndx] = raw_adc_val;		// Store new sample to buffer //
	if(++MAvgIndx == MOVING_AVG_DEPTH)	// increment circular buffer index with checking overflow //
		MAvgIndx = 0;
	
	result = SamplesSum / MOVING_AVG_DEPTH;
	//SLOG("RAWmAvg value(%u)\n", result);
	
	return result; // Return new moving average //
}

static void init_mavg(uint16_t first_sample)
{
	volatile uint16_t indx;
	
	//my_printf1("%s()-> CALLED with RAWValue(%u)\n", __FUNCTION__, first_sample);
	for(indx=MOVING_AVG_DEPTH-1 ;; indx--) {
		Samples[indx] = first_sample;
		if(indx == 0)
			break;
	}
	SamplesSum = (uint32_t)MOVING_AVG_DEPTH*((uint32_t)first_sample);
}

/**
  * @brief  ADC1 channel with DMA configuration
  * @param  None
  * @retval None
  */
static void ADC1_CH_DMA_Config(void)
{
  ADC_InitTypeDef     ADC_InitStructure;
  DMA_InitTypeDef     DMA_InitStructure;
  GPIO_InitTypeDef  GPIO_InitStructure;
 
  /* ADC1 DeInit */  
  ADC_DeInit(ADC1);
  
  /* ADC1 Periph clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  
  /* DMA1 clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);
	
	  /* Configure ADC Channel1 & Channel5 as analog input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* DMA1 Channel1 Config */
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)RegularConvData_Tab;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 5;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  
  /* DMA1 Channel1 enable */
  DMA_Cmd(DMA1_Channel1, ENABLE);
  
  /* ADC DMA request in circular mode */
  ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_Circular);
  
  /* Enable ADC_DMA */
  ADC_DMACmd(ADC1, ENABLE);  
  
  /* Initialize ADC structure */
  ADC_StructInit(&ADC_InitStructure);
  
  /* Configure the ADC1 in continous mode withe a resolutuion equal to 12 bits  */
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; 
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_ScanDirection = ADC_ScanDirection_Backward;
  ADC_Init(ADC1, &ADC_InitStructure); 
 
  /* Convert the ADC1 channel1(battery temp) with 55.5 Cycles as sampling time */ 
  ADC_ChannelConfig(ADC1, ADC_Channel_1 , ADC_SampleTime_55_5Cycles);  
  ADC_TempSensorCmd(ENABLE);

  /* Convert the ADC1 channel5(battery level) with 55.5 Cycles as sampling time */ 
  ADC_ChannelConfig(ADC1, ADC_Channel_5 , ADC_SampleTime_55_5Cycles);  
  ADC_VbatCmd(ENABLE);

	/* Convert the ADC1 temperature sensor  with 55.5 Cycles as sampling time */ 
  ADC_ChannelConfig(ADC1, ADC_Channel_TempSensor , ADC_SampleTime_55_5Cycles);  
  ADC_TempSensorCmd(ENABLE);
  
  /* Convert the ADC1 Vref  with 55.5 Cycles as sampling time */ 
  ADC_ChannelConfig(ADC1, ADC_Channel_Vrefint , ADC_SampleTime_55_5Cycles); 
  ADC_VrefintCmd(ENABLE);
  
  /* Convert the ADC1 Vbat  with 55.5 Cycles as sampling time */ 
  ADC_ChannelConfig(ADC1, ADC_Channel_Vbat , ADC_SampleTime_55_5Cycles); 
	ADC_VbatCmd(ENABLE);
	
  /* ADC Calibration */
  ADC_GetCalibrationFactor(ADC1);
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);     
  
  /* Wait the ADCEN falg */
  while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_ADEN)); 
  
  /* ADC1 regular Software Start Conv */ 
  ADC_StartOfConversion(ADC1);
}

