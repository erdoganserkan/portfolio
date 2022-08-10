#ifndef MY_MACRO_H
#define MY_MACRO_H

#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE 
	#define TRUE (!FALSE)
#endif

#ifndef DISABLE
	#define DISABLE 0
#endif
#ifndef ENABLE 
	#define ENABLE (!DISABLE)
#endif

#define ARRAY_MEMBER_COUNT(x)			(sizeof(x)/sizeof(x[0]))
#define SM(x,y)							((x) - (((x)/(y))*(y)))		/* Simple Modus Operation */
#define GET_ALIGNED(num, alignment)		(((num/alignment)+1)*alignment)
#define GET_MAX(x,y)					((x>y)?(x):(y))
#define GET_MAX_INDX(indx,x0,x1,x2)		{ \
	if(x0 > x1) {	\
		if(x0 > x2)	indx = 0;	\
		else indx = 2;	\
	} else { \
		if(x1 > x2) indx = 1; \
		else indx = 2;	\
}				
#ifndef M_PI
	#define M_PI	3.141592653589
#endif

/******************************************************************************
* Function Name  : RGB565CONVERT
* Description	 : 24位转换16位
* Input 		 : - red: R
*				   - green: G 
*				   - blue: B
* Output		 : None
* Return		 : RGB 颜色值
* Attention 	 : None
*******************************************************************************/
#define RGB565CONVERT(red, green, blue)\
		(uint16_t)( (( red	 >> 3 ) << 11 ) | \
		(( green >> 2 ) << 5  ) | \
		( blue	>> 3 ))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif
