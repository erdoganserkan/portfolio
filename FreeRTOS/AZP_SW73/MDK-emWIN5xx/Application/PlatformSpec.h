#ifndef UMD_PLATFORM_SPEC_H
#define UMD_PLATFORM_SPEC_H

#include <stdio.h>
#include <stdint.h>

#define UMD_DISPLAY_SOFT_VERSION		(100)	// Display yaziliminin versiyonu //
#define UMD_DISPLAY_HARD_VERSION		(100)	// Display yaziliminin gerektirdigi hardware versiyonu // 

// Types for using @ UMDShared.h File // 
// Every developer must define types for his/her mcu platform if supported // 
typedef unsigned char 			U8_;
typedef signed char 				S8_;
typedef unsigned short 			U16_;
typedef signed short 				S16_;
typedef unsigned int				U32_;
typedef signed int 					S32_;
typedef unsigned long long 	U64_;	// Maybe not supported @ 8bit mcu platforms //
typedef signed long long 		S64_;	// Maybe not supported @ 8bit mcu platforms // 


#endif
