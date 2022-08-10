/****************************************Copyright (c)****************************************************
**                                      
**                                 http://www.powermcu.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               TouchPanel.c
** Descriptions:            The TouchPanel application function
**
**--------------------------------------------------------------------------------------------------------
** Created by:              AVRman
** Created date:            2010-11-7
** Version:                 v1.0
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "GLCD.h"
#include "AppCommon.h"
#include "TouchPanel.h"
#include "TSTask.h"

/* Private define ------------------------------------------------------------*/
#define THRESHOLD 2   /* 差值门限 */

volatile uint8_t ts_calib_done = FALSE;
volatile uint8_t ts_calib_cnt = 0xFF;
volatile uint8_t ts_calib_indx=0;
#if(USED_LCD == LCD_7_0_TFT)
	Coordinate ts_calib_array[5] = {
		{3581, 213},	// Left-Up coordinates // 
		{3581, 3890},	// Right-up Coordinates // 
		{371, 3890},	// Right-Down // 
		{371, 213},	// Left-down // 
		{1976, 2051}	// Middle // 
	};
#elif(USED_LCD == LCD_4_3_TFT)
	Coordinate ts_calib_array[5] = {
		{3612, 235},	// Left-Up coordinates // 
		{3572, 3801},	// Right-up Coordinates // 
		{388, 3813},	// Right-Down // 
		{412, 239},	// Left-down // 
		{1993, 2014}	// Middle // 
	};
#endif

/*******************************************************************************
* Function Name  : TP_Init
* Description    : TSC2046 Initialization
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void TP_Init(void) 
{
    SSP_CFG_Type SSP_ConfigStruct;
	/*
	 * Initialize SPI pin connect
	 * P2.23 - TP_CS - used as GPIO
	 * P2.22 - SCK
	 * P2.26 - MISO
	 * P2.27 - MOSI
	 */

	PINSEL_ConfigPin(2, 23, 0);
	PINSEL_ConfigPin(2, 22, 2);
	PINSEL_ConfigPin(2, 26, 2);
	PINSEL_ConfigPin(2, 27, 2);

    /* P0.16 CS is output */
  GPIO_SetDir(TP_CS_PORT_NUM, (1<<TP_CS_PIN_NUM), 1);
	GPIO_SetValue(TP_CS_PORT_NUM, (1<<TP_CS_PIN_NUM));  

  PINSEL_ConfigPin(2, 11, 0);	  
	GPIO_SetDir(2, (1<<11), 0);	  /* P2.11 TP_INT is input */

	/* initialize SSP configuration structure to default */
	SSP_ConfigStructInit(&SSP_ConfigStruct);

	SSP_ConfigStruct.ClockRate = 250000;

	/* Initialize SSP peripheral with parameter given in structure above */
	SSP_Init(LPC_SSP0, &SSP_ConfigStruct);
	/* Enable SSP peripheral */
	SSP_Cmd(LPC_SSP0, ENABLE);
}

/*******************************************************************************
* Function Name  : DelayUS
* Description    : 延时1us
* Input          : - cnt: 延时值
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void DelayUS(uint32_t cnt)
{
  uint32_t i;
  for(i = 0;i<cnt;i++) {
     uint8_t us = 50; 
     while (us--) {
       ;   
     }
  }
}

/*******************************************************************************
* Function Name  : WR_CMD
* Description    : 向 ADS7843写数据
* Input          : - cmd: 传输的数据
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static uint8_t WR_CMD (uint8_t cmd)  
{ 
	/* wait for current SSP activity complete */
	while (SSP_GetStatus(LPC_SSP0, SSP_STAT_BUSY) ==  SET);

	SSP_SendData(LPC_SSP0, (uint16_t) cmd);

	while (SSP_GetStatus(LPC_SSP0, SSP_STAT_RXFIFO_NOTEMPTY) == RESET);
	return (SSP_ReceiveData(LPC_SSP0));
} 

/*******************************************************************************
* Function Name  : RD_AD
* Description    : 读取ADC值
* Input          : None
* Output         : None
* Return         : ADS7843返回二字节数据
* Attention		 : None
*******************************************************************************/
static int RD_AD(void)  
{ 
  unsigned short buf,temp; 

  temp = WR_CMD(0x00);
  buf = temp<<8; 
  DelayUS(1); 
  temp = WR_CMD(0x00);;
  buf |= temp; 
  buf>>=3; 
  buf&=0xfff; 
  return buf; 
} 

/*******************************************************************************
* Function Name  : Read_X
* Description    : 读取ADS7843通道X+的ADC值 
* Input          : None
* Output         : None
* Return         : ADS7843返回通道X+的ADC值
* Attention		 : None
*******************************************************************************/
int Read_X(void)  
{  
  int i; 
  TP_CS_LOW(); 
  DelayUS(1); 
  WR_CMD(CHX); 
  DelayUS(1); 
  i=RD_AD(); 
  TP_CS_HIGH(); 
  return i;    
} 

/*******************************************************************************
* Function Name  : Read_Y
* Description    : 读取ADS7843通道Y+的ADC值
* Input          : None
* Output         : None
* Return         : ADS7843返回通道Y+的ADC值
* Attention		 : None
*******************************************************************************/
int Read_Y(void)  
{  
  int i; 
  TP_CS_LOW(); 
  DelayUS(1); 
  WR_CMD(CHY); 
  DelayUS(1); 
  i=RD_AD(); 
  TP_CS_HIGH(); 
  return i;     
} 

/*******************************************************************************
* Function Name  : TP_GetAdXY
* Description    : 读取ADS7843 通道X+ 通道Y+的ADC值
* Input          : None
* Output         : None
* Return         : ADS7843返回 通道X+ 通道Y+的ADC值 
* Attention		 : None
*******************************************************************************/
void TP_GetAdXY(int *x,int *y)  
{ 
  int adx,ady; 
  adx=Read_X(); 
  DelayUS(1); 
  ady=Read_Y(); 
  *x=adx; 
  *y=ady; 
} 

/*******************************************************************************
* Function Name  : Read_Ads7846
* Description    : 得到滤波之后的X Y
* Input          : None
* Output         : None
* Return         : Coordinate结构体地址
* Attention		 : None
*******************************************************************************/
Coordinate *Read_Ads7846(void)
{
  static Coordinate  screen;
  int m0,m1,m2,TP_X[1],TP_Y[1],temp[3];
  uint8_t count=0;
#if((USED_LCD == LCD_7_0_TFT) || (USED_LCD == LCD_4_3_TFT))
	if(FALSE == ts_calib_done) { 
		if(0 != ts_calib_cnt) {
			if(0 == (--ts_calib_cnt)) {
				ts_calib_cnt = 0xFF;
				screen = ts_calib_array[ts_calib_indx++];
				if(5 == ts_calib_indx)
					ts_calib_done = TRUE;
				return &screen;
			}
			else {
				return 0;
			}
		}
		else {
			return 0;
		}
	}
	else 
#endif
	{		
		int buffer[2][9]={{0},{0}};  
		do					       
		{		   
			TP_GetAdXY(TP_X,TP_Y);  
			buffer[0][count]=TP_X[0];  
			buffer[1][count]=TP_Y[0];
			count++;  
		}
		while(!TP_INT_IN&& count<9);  /* TP_INT_IN为触摸屏中断引脚,当用户点击触摸屏时TP_INT_IN会被置低 */
		if(count==9)   /* 成功采样9次,进行滤波 */ 
		{  
			/* 为减少运算量,分别分3组取平均值 */
			temp[0]=(buffer[0][0]+buffer[0][1]+buffer[0][2])/3;
			temp[1]=(buffer[0][3]+buffer[0][4]+buffer[0][5])/3;
			temp[2]=(buffer[0][6]+buffer[0][7]+buffer[0][8])/3;
			/* 计算3组数据的差值 */
			m0=temp[0]-temp[1];
			m1=temp[1]-temp[2];
			m2=temp[2]-temp[0];
			/* 对上述差值取绝对值 */
			m0=m0>0?m0:(-m0);
				m1=m1>0?m1:(-m1);
			m2=m2>0?m2:(-m2);
			/* 判断绝对差值是否都超过差值门限，如果这3个绝对差值都超过门限值，则判定这次采样点为野点,抛弃采样点，差值门限取为2 */
			if( m0>THRESHOLD  &&  m1>THRESHOLD  &&  m2>THRESHOLD ) return 0;
			/* 计算它们的平均值，同时赋值给screen */ 
			if(m0<m1)
			{
				if(m2<m0) 
					screen.x=(temp[0]+temp[2])/2;
				else 
					screen.x=(temp[0]+temp[1])/2;	
			}
			else if(m2<m1) 
				screen.x=(temp[0]+temp[2])/2;
			else 
				screen.x=(temp[1]+temp[2])/2;

		/* 同上 计算Y的平均值 */
		temp[0]=(buffer[1][0]+buffer[1][1]+buffer[1][2])/3;
		temp[1]=(buffer[1][3]+buffer[1][4]+buffer[1][5])/3;
		temp[2]=(buffer[1][6]+buffer[1][7]+buffer[1][8])/3;
		m0=temp[0]-temp[1];
		m1=temp[1]-temp[2];
		m2=temp[2]-temp[0];
		m0=m0>0?m0:(-m0);
		m1=m1>0?m1:(-m1);
		m2=m2>0?m2:(-m2);
		if(m0>THRESHOLD&&m1>THRESHOLD&&m2>THRESHOLD) return 0;

		if(m0<m1) {
			if(m2<m0) 
				screen.y=(temp[0]+temp[2])/2;
			else 
				screen.y=(temp[0]+temp[1])/2;	
			}
		else if(m2<m1) 
			 screen.y=(temp[0]+temp[2])/2;
		else
			 screen.y=(temp[1]+temp[2])/2;

			if(1){
				char temp[64];
				memset(temp, 0, sizeof(temp));
				sprintf(temp, "x:%d	:	y:%d\n", screen.x, screen.y);
				GUI_Text(150, 150, temp,RGB565CONVERT(184,158,131), RGB565CONVERT(0,0,0));
			}

			// Invoke TSTask about the coordinates of new touch envent // 
			new_ts = TRUE;

			// return the value // 
			return &screen;
			
		}  
	}
  return 0; 
}

/*******************************************************************************
* Function Name  : TP_DrawPoint
* Description    : 在指定座标画点
* Input          : - Xpos: Row Coordinate
*                  - Ypos: Line Coordinate 
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
// This function is automatically called if there is a TOUCH EVENT by calibrate() function // 
void TP_DrawPoint(uint16_t Xpos,uint16_t Ypos)
{
	if((TRUE == new_ts) && (Xpos < GLCD_X_SIZE) && (Ypos < GLCD_Y_SIZE)){
		ts_point.points[0] = Xpos;
		ts_point.points[1] = Ypos;
		new_ts2 = TRUE;
	} else {
		new_ts = FALSE;	// Clear first step result, it is a bug from library // 
	}
	
#if(0)
  GLCD_SetPixel_16bpp_0(Xpos,Ypos,0xf800);  
	GLCD_SetPixel_16bpp_0(Xpos+1,Ypos,0xf800);
	GLCD_SetPixel_16bpp_0(Xpos,Ypos+1,0xf800);
	GLCD_SetPixel_16bpp_0(Xpos+1,Ypos+1,0xf800);	
#endif
}	

/*******************************************************************************
* Function Name  : DrawCross
* Description    : 在指定座标画十字准星
* Input          : - Xpos: Row Coordinate
*                  - Ypos: Line Coordinate 
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void DrawCross(uint16_t Xpos,uint16_t Ypos)
{	
	GLCD_DrawLine(Xpos-15,Ypos,Xpos-2,Ypos,0xffff);
	GLCD_DrawLine(Xpos+2,Ypos,Xpos+15,Ypos,0xffff);
	GLCD_DrawLine(Xpos,Ypos-15,Xpos,Ypos-2,0xffff);
	GLCD_DrawLine(Xpos,Ypos+2,Xpos,Ypos+15,0xffff);
	
	GLCD_DrawLine(Xpos-15,Ypos+15,Xpos-7,Ypos+15,RGB565CONVERT(184,158,131));
	GLCD_DrawLine(Xpos-15,Ypos+7,Xpos-15,Ypos+15,RGB565CONVERT(184,158,131));
	
	GLCD_DrawLine(Xpos-15,Ypos-15,Xpos-7,Ypos-15,RGB565CONVERT(184,158,131));
	GLCD_DrawLine(Xpos-15,Ypos-7,Xpos-15,Ypos-15,RGB565CONVERT(184,158,131));
	
	GLCD_DrawLine(Xpos+7,Ypos+15,Xpos+15,Ypos+15,RGB565CONVERT(184,158,131));
	GLCD_DrawLine(Xpos+15,Ypos+7,Xpos+15,Ypos+15,RGB565CONVERT(184,158,131));
	
	GLCD_DrawLine(Xpos+7,Ypos-15,Xpos+15,Ypos-15,RGB565CONVERT(184,158,131));
	GLCD_DrawLine(Xpos+15,Ypos-15,Xpos+15,Ypos-7,RGB565CONVERT(184,158,131));	  	
}	

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
