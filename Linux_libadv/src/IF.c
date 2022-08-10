/***********************************************************************************/
/*                                                                                 */
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/*                                                                                 */
/***********************************************************************************/
#define     ASSERT_MODULE  MODULE_HDMITX
//#define     SYSLOG         1

#include 	<stdint.h>
#include    <string.h>    // for memcmp and memcpy
#include    <stdio.h>
#include    <stdlib.h>
#include 	<errno.h>
#include 	"log.h"
#include 	"../inc/libadv_common.h"
#include 	"../inc/libadv.h"
#include    "IF.h"
#include 	<log.h>

extern mj_log_type libADV_log_instance;

#ifndef GLOBAL
	#define GLOBAL
#endif
#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE
	#define TRUE (!FALSE)
#endif
/************************************************************************/
/*      Local defines                                                   */
/************************************************************************/

/************************************************************************/
/*      Static Function Prototyptes                                     */
/************************************************************************/
static UInt8 sCalc_Checksum( IF_t *p );

/************************************************************************/
/*      Static Variables                                                */
/************************************************************************/
static Int16 sNumInfoFramesAllocated = 0;

/************************************************************************/
/*      Public Functions                                                */
/************************************************************************/
IF_t   * IF_New( IF_Type_t Type )
{
    IF_t *p = NULL;
    Bool ErrorFlag = FALSE;

    TRACEL(libADV_log_instance, "Sizeof(IF_t) = %u\n", sizeof(IF_t));

    // first allocate header bytes
    p = (IF_t *)calloc( sizeof( IF_t ), 1);
    if( NULL==p ) {
    	ERRL(libADV_log_instance, "IF_t object creation FAILED (%s)\n", strerror(errno));
    	return NULL;
    } else {
    	TRACEL(libADV_log_instance, "Created IF-Header Adr(%p)\n", p);
    }

    // generate Type-specific header byte values
    // for now, don't allow vendor-specific InfoFrame (not used by HDX -- but still can pass thru via I2C regs)
    switch( Type ) {
        case IF_TYPE_AVI:
            p->Type = IF_TYPE_AVI;
            p->Version = 2 ;
            p->Length = AVI_MAX_LEN;   // defined in CEA-861B (true for both version 1 & 2)
            break;

        case IF_TYPE_SPD:
            p->Type = IF_TYPE_SPD;
            p->Version = 1 ;  // no other versions defined in CEA-861B
            p->Length = SPD_MAX_LEN;   // defined in CEA-861B
            break;

        case IF_TYPE_AUDIO:
            p->Type = IF_TYPE_AUDIO;
            p->Version = 1 ;  // no other versions defined in CEA-861B
            p->Length = AUD_MAX_LEN;   // defined in CEA-861B
            break;

        case IF_TYPE_MPEG:
            p->Type = IF_TYPE_MPEG;
            p->Version = 1 ;  // no other versions defined in CEA-861B
            p->Length = MPEG_MAX_LEN;   // defined in CEA-861B
            break;

        default:
            ErrorFlag = TRUE;
            break;
    }

    // if valid type was requested, then allocate payload bytes
    if( !ErrorFlag )
    {
        p->Data = (UInt8 *)calloc((p->Length), sizeof(UInt8)); // should I use Calloc instead?  Does it matter?
        if( NULL==p->Data ) {
        	ERRL(libADV_log_instance, "IF_t:Data object creation FAILED (%s)\n", strerror(errno));
            ErrorFlag = TRUE;
        } else {
        	TRACEL(libADV_log_instance, "Created IF-Data Adr(%p)\n", p->Data);
           /* initialize payload to zeros */
            // for( i=0; i<p->Length; ++i )
                // p->Data[i] = 0;

            /* Calculate Check Sum */
            sCalc_Checksum(p);
        }
    }

    if( ErrorFlag ) {
        free( (void *)p );
        return NULL;
    } else {
        TRACEL(libADV_log_instance, "Created Type = %02x\n", Type );
        ++sNumInfoFramesAllocated;
        return p;
    }
}


uint8_t IF_Delete( IF_t *p )
{
    // if null pointer, don't do it
    if( NULL==p->Data || NULL==p )
        return -1;
    else {
        TRACEL(libADV_log_instance, "Deleting Type = %02x\n", p->Type );
        // first free the payload, then the top-level struct
        free( (void *)(p->Data) );
        free( (void *)p );
        --sNumInfoFramesAllocated;
        return 0;
    }
}


Int16 IF_HowManyAllocated( void )
{
    // allow negative return value in case something goes bad...
    return sNumInfoFramesAllocated;
}


UInt8 IF_Calc_Checksum( IF_t *p )
{
    return sCalc_Checksum(p);
}


Bool  IF_Is_Checksum_Valid( IF_t *p )
{
    UInt8 i;
    UInt8 sum = 0;

    // sum of all bytes in InfoFrame (including Checksum byte) should equal zero
    sum += p->Type;
    sum += p->Version;
    sum += p->Length;
    sum += p->Checksum;

    for( i=0; i<p->Length; ++i ) {
        sum += p->Data[i];
    }
    return ( sum ? FALSE : TRUE );
}


void IF_Syslog_Report( IF_t *p )
{
    UInt8 i;
    DEBUGL(libADV_log_instance, "IF_Syslog_Report\n");

    DEBUGL(libADV_log_instance, "Ptr value = %05x", (void *)p );
    DEBUGL(libADV_log_instance, "Type = %02x", p->Type );
    DEBUGL(libADV_log_instance, "Version = %02x", p->Version );
    DEBUGL(libADV_log_instance, "Length = %02x", p->Length );
    DEBUGL(libADV_log_instance, "Checksum = %02x", p->Checksum );
    for( i=0; i<p->Length; ++i )
    	DEBUGL(libADV_log_instance, "Data[%02d] = %02x", i, p->Data[i] );
}

void IF_DumpRegs( IF_t *p )
{
    UInt8 i;

    DEBUGL(libADV_log_instance, "Type     = 0x%02X\n", p->Type);
    DEBUGL(libADV_log_instance, "Version  = 0x%02X\n", p->Version);
    DEBUGL(libADV_log_instance, "Length   = 0x%02X\n", p->Length);
    DEBUGL(libADV_log_instance, "Checksum = 0x%02X\n", p->Checksum);
    DEBUGL(libADV_log_instance, "Data     = \n");
    for( i=0; i<p->Length; ++i )
    	DEBUGL(libADV_log_instance, "0x%02X ",p->Data[i]);
    DEBUGL(libADV_log_instance, "\n");
}


Bool IF_Compare( IF_t *p1, IF_t *p2 )
{
    Bool rtn_val = FALSE;

    // compare headers (but don't compare pointer to payload!!)
    if( 0==memcmp( (void *)p1, (void *)p2, sizeof( IF_t )-sizeof( UInt8 * ) ))
        // now compare payloads
        if( 0==memcmp( (void *)p1->Data, (void *)p2->Data, p1->Length * sizeof( UInt8 ) ))
            rtn_val = TRUE;

    return rtn_val;
}


UInt8 IF_Copy( IF_t *dest, IF_t *src )
{
    if( dest == NULL || src == NULL ) return -1;

    // first make sure they are of the same type
    if( src->Type != dest->Type )
        return -1;
    else
    {
        // Only copy the version, checksum, and payload (Type and Length must be the same)
        // Don't modify the payload pointer!!
        dest->Version = src->Version;
        dest->Checksum = src->Checksum;  // validity is not checked here
        memcpy( (void *)dest->Data, (void *)src->Data, src->Length );
        return 0;
    }
}


UInt8 IF_GetVersion( IF_t *p)
{
    return p->Version;
}


UInt8 *IF_GetPayload( IF_t *p )
{
    if(p) {
    	TRACEL(libADV_log_instance, "Return IF-Data Adr(%p)\n", p->Data);
    	return  (p->Data);
    }
    else
    	return  0;
}

/*****************************************************************/
/*     Local functions                                           */
/*****************************************************************/
static UInt8 sCalc_Checksum( IF_t *p )
{
    // This routine calculates the checksum of the infoFrame packet and writes the
    // result to the Checksum field of the InfoFrame_t struct.

    // from HDMI spec, Sec 5.3.5:
    //  The checksum shall be calculated such that a bytewide sum of all three bytes
    //  of the Packet Header and all valid bytes of the InfoFrame Packet contents
    //  (determined by InfoFrame_length), plus the checksum itself, equals zero.

    uint8_t i;
    uint8_t sum = 0;

    if(p == NULL) return 0;

    sum -= p->Type;
    sum -= p->Version;
    sum -= p->Length;

    for( i=0; i<p->Length; ++i )
        sum -= p->Data[i];

    p->Checksum = sum;
    return sum;
}

HDMIrx_VidFrm_t HDMIrx_getVideoFormat( IF_t *spAVIinfoFrame )
{
    IF_AVIdata *p;
    HDMIrx_VidFrm_t format;
    UInt8 ver = IF_GetVersion( spAVIinfoFrame );

    if( ver < 2 ) {
    	INFOL(libADV_log_instance, "AVI INFO-FRAME version(%u) is lower than expected(%u)\n", ver, 2);
    	format = eHDMIrx_VidFrm_None;
    }
    else
    {
        p = (IF_AVIdata *)IF_GetPayload(spAVIinfoFrame);
        {
        	volatile uint8_t indx = 0;
        	for(indx=0 ; indx<AVI_MAX_LEN ;indx++) {
        		uint8_t *pU8 = (uint8_t *)p;
        		TRACEL(libADV_log_instance, "AVI-IF[%u] = 0x%02X\n", indx, pU8[indx]);
        	}
        }
        DEBUGL(libADV_log_instance, "VideoCode(%u)\n", (UInt8)(p->VideoCode));
        switch((UInt8)(p->VideoCode))
        {
            case FM_576P50_43:
            case FM_576P50:
                format = eHDMIrx_VidFrm_576p50;
                break;
            case FM_480P60_43:
            case FM_480P60:
                format = eHDMIrx_VidFrm_480p60;
                break;
            case FM_576I50_43:
            case FM_576I50:
                format = eHDMIrx_VidFrm_576i50;
                break;
            case FM_480I60_43:
            case FM_480I60:
                format = eHDMIrx_VidFrm_480i60;
                break;
            case FM_720P50:
                format = eHDMIrx_VidFrm_720p50;
                break;
            case FM_720P60:
                format = eHDMIrx_VidFrm_720p60;
                break;
            case FM_1080I50:
                format = eHDMIrx_VidFrm_1080i50;
                break;
            case FM_1080I60:
                format = eHDMIrx_VidFrm_1080i60;
                break;
            case FM_1080P50:
                format = eHDMIrx_VidFrm_1080p50;
                break;
            case FM_1080P60:
                format = eHDMIrx_VidFrm_1080p60;
                break;
            default:
                format = eHDMIrx_VidFrm_UnKnown;
        }
    }
    return format;
}
/** END of File *********************************************************/
