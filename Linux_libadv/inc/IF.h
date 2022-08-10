/***********************************************************************************/
/*                                                                                 */
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/*                                                                                 */
/***********************************************************************************/
#ifndef _IF_H
#define _IF_H

#include	<stdint.h>
#include <libadv_common.h>

typedef uint8_t UInt8;
typedef int8_t Int8;
typedef uint16_t UInt16;
typedef int16_t Int16;

/************************************************************************/
/*      Error Messages                                                  */
/************************************************************************/
#define		IF_ERR_UNKNOWN          (ERROR_IF | 0x00)

/************************************************************************/
/*      Public Defines                                                  */
/************************************************************************/
typedef UInt8 IF_Type_t;

// can't use enum because we need byte-size data type
//  to match up with data field in InfoFrame

// these are the valid entries for the InfoFrame Type field
// NOTE: while CEA-861B identifies the Type field codes as 01, 02, etc, HDMI spec requires bit 7 to be set
//       to distinguish InfoFrame packets from other Data Island packet types.


#define IF_TYPE_VENDOR_SPECIFIC  0x81   // we don't use this (IF_New() will return NULL)
#define IF_TYPE_AVI              0x82
#define IF_TYPE_SPD              0x83
#define IF_TYPE_AUDIO            0x84
#define IF_TYPE_MPEG             0x85

/* the start of infoframe register block */

#define CP_BYTE1_ADDR   	(IO_Addr_t)0x1DF

/* infoframe updated ? */
#define INT_STATUS_3_ADDR   (IO_Addr_t)0x073
    #define NEW_AVI         1
    #define NEW_AUD         4

/* payload length */
#define AVI_MAX_LEN         13
#define SPD_MAX_LEN         25
#define AUD_MAX_LEN         10
#define MPEG_MAX_LEN        10

#define VEN_NAME_LEN		8
#define PRODUCT_NAME_LEN	16


/* The missing values learned from : "http://www.rohde-schwarz-av.com/_pdf/Application%20Notes/HDMI/HDMI-Overview.pdf" */
/* video formats per eia/cea 861b */
#define FM_VGA60            1
#define FM_480P60_43        2	/* 480p 4/3 */
#define FM_480P60           3	/* 480p 16/9 */
#define FM_720P60           4
#define FM_1080I60          5
#define FM_480I60_43        6	/* 480i 4/3 */
#define FM_480I60           7	/* 480i 16/9 */
#define FM_1080P60          16
#define FM_576P50_43        17
#define FM_576P50           18
#define FM_720P50           19
#define FM_1080I50          20
#define FM_576I50_43        21
#define FM_576I50           22
#define FM_1080P50          31
#define FM_1080P24          32
#define FM_1080P25          33
#define VIDEO_CODE_MAX		34

/* aspect ratios */
#define RA4_3				1
#define R16_9				2

#define AF_LIN_PCM			1	// same as "IEC60958 PCM" as listed in EIA/CEA-861B
#define AF_AC3				2	// also known as "Dolby Digital" or "AC-3"
#define AF_MPEG1			3	// "MPEG1, Layers 1 & 2"
#define AF_MP3				4	// "MPEG1, Layer3"
#define AF_MPEG2			5	// "MPEG2 (multichannel)"
#define AF_AAC				6	// "Advanced Audio Coding"
#define AF_DTS				7	//
#define AF_ATRAC  			8


#define AUDCC_2				1	// 2 channels
#define AUDCC_3				2	// 3 channels
#define AUDCC_4				3	// 4 channels
#define AUDCC_5				4	// 5 channels
#define AUDCC_6     		5 	// 6 channels
#define AUDCC_7     		6  	// 7 channels
#define AUDCC_8     		7   // 8 channels

#define AUDSR_32K   		1   // 32 kHz
#define AUDSR_44K1   		2   // 44.1 kHz
#define AUDSR_48K    		3   // 48 kHz
#define AUDSR_88K2   		4   // 88.2 kHz
#define AUDSR_96K    		5   // 96 kHz
#define AUDSR_176K4  		6   // 176.4 kHz
#define AUDSR_192K   		7   // 192 kHz

#define AUDSS_16BITS		1
#define AUDSS_20BITS		2
#define AUDSS_24BITS		3

//   functions that pass around InfoFrames should return one of these enumerated codes
//   0 : Successfuly transferred a valid InfoFrame
//   1 : Invalid Type requested (did nothing)
//   2 : Error in Type field (assume other regs are invalid, so ignored)
//   3 : InfoFrame was transferred, Type was correct, but checksum failed

typedef enum
{
    eIFrame_Hndlr_Success       = 0,
    eIFrame_Hndlr_BadRequest    = 1,
    eIFrame_Hndlr_TypeError     = 2,
    eIFrame_Hndlr_ChksumError   = 3

} IFrame_Handler_rtn_t;

#pragma pack(1)

/************************************************************************/
/*      Data Types                                                      */
/************************************************************************/

// this struct will likely be made private as code is developed further.  For now, it is
// easier to allow calling modules to read/write infoframe fields directly.

typedef struct IF_s
{
    // Header bytes
    UInt8  Type; // AVI, SPD, Audio, or MPEG (use #defines above)
    UInt8  Version;
    UInt8  Length;  // number of data bytes (doesn't include 4 header bytes)
    UInt8  Checksum;

    // Payload

    UInt8 *Data;
    // pointer to byte array of size Length

} IF_t;

/* avi infoframe payload layout per eia/cea 861b */

typedef struct _avi_fields
{
	uint8_t				Scan:2,
                        Bardata:2,
                        Active:1,
                        ColFormat:2,
                        FillerA:1;
	uint8_t             ActiveRatio:4,
                        BasicRatio:2,
                        Colorimetry:2;

	uint8_t				Scaling:2,
                        RGBQuant:2,
                        FillerB:4;
	uint8_t 			Padding;	// Inserted BUT i am NOT SURE //
	uint8_t             VideoCode:7,
                        FillerC:1;

	uint8_t 			PixelRepeat:4,
                        FillerD:4;
	uint8_t             EndOfTopBarLo;

	uint8_t  			EndOfTopBarHi;
	uint8_t		        StartOfBottomBarLo;

	uint8_t 			StartOfBottomBarHi;
    uint8_t             EndOfLeftBarLo;

	uint8_t				EndOfLeftBarHi;
	uint8_t				StartOfRightBarLo;

	uint16_t			StartOfRightBarHi;
    uint8_t				FillerE;

} IF_AVIdata;

/* audio infoframe payload layout per eia/cea 861b */


typedef struct _aud_fields
{
	unsigned int	NumOfChannels:3,
                        FillerA:1,
                        Format:4,
                        SampleSize:2,
                        SampleRate:3,
                        FillerB:3;

	unsigned int	MaxRate:8,
                        Byte4:8;

	unsigned int	Byte5:8,
                        FillerC:8;

} IF_AUDdata;

typedef enum HDMIrx_VidFrm_e
{
    eHDMIrx_VidFrm_None	= 0,
    eHDMIrx_VidFrm_576i50,
    eHDMIrx_VidFrm_576p50,
    eHDMIrx_VidFrm_480i60,
    eHDMIrx_VidFrm_480p60,
    eHDMIrx_VidFrm_720p50,
    eHDMIrx_VidFrm_720p60,
    eHDMIrx_VidFrm_1080i50,
    eHDMIrx_VidFrm_1080i60,
    eHDMIrx_VidFrm_1080p50,
    eHDMIrx_VidFrm_1080p60,
    eHDMIrx_VidFrm_UnKnown
} HDMIrx_VidFrm_t;

#pragma pack()

/************************************************************************/
/*  Function Prototypes                                                 */
/************************************************************************/

// InfoFrame_New
//   this function allocates memory, initializes the header, and returns a pointer
//   to an InfoFrame with zeroed data and invalid checksum
//   Use InfoFrame_Set... functions to validate the data
//      then use InfoFrame_Calc_Checksum to validate checksum

IF_t*    IF_New( IF_Type_t Type );

// IF_Delete
//   this function deletes an InfoFrame object and frees the memory it used
//   returns -1 if p or p->Data is NULL

UInt8   IF_Delete( IF_t *p );

// Query funtion to find out how many are allocated (also a crude memory monitor to test for "leaks")

Int16    IF_HowManyAllocated( void );

// Checksum utilities

UInt8	IF_Calc_Checksum( IF_t *p );
Bool	IF_Is_Checksum_Valid( IF_t *p );

// Compare and copy utilities

Bool	IF_Compare( IF_t *p1, IF_t *p2 );  // returns true if they are equal
UInt8	IF_Copy( IF_t *dest, IF_t *src ); // returns 0 on success

void	IF_Syslog_Report( IF_t *p );
void     IF_DumpRegs( IF_t *p );

/* methods to access infoframe structure members */
UInt8   *IF_GetPayload( IF_t *p );
UInt8	IF_GetVersion( IF_t *p);

/* returning detected HDMI bit */
Bool	IF_IsHDMIsource(void);
HDMIrx_VidFrm_t HDMIrx_getVideoFormat( IF_t *spAVIinfoFrame );


#endif /* _IF_H */
