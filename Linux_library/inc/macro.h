#ifndef LINUXLIBRARY_MACRO_H_
#define LINUXLIBRARY_MACRO_H_

#define SIZEOF_MEMBER(struct_type, field) 	(sizeof( ((struct_type*)0)->field ))
#define ARRAY_MEMBER_COUNT(x)				(sizeof(x)/sizeof(x[0]))
#define GET_CEIL(x,y)	(((x%y)==0)?(x):(((x/y)+1))*y)
#define GET_FLOOR(x,y)	(((x%y)==0)?(x):((x/y)*y))


#ifndef TRUE
	#define TRUE	(1)
#endif
#ifndef FALSE
	#define FALSE (!TRUE)
#endif

#define SWAP_U16(u16) {	\
	uint8_t *u8p = (uint8_t *)&(u16);	\
	uint8_t temp_u8 = u8p[0];	\
	u8p[0] = u8p[1];	\
	u8p[1] = temp_u8;	\
}

#define SWAP_U32(u32) {	\
	uint8_t *u8p = (uint8_t *)&(u32);	\
	/* swap bytes 0 and 3 */	\
	uint8_t temp_u8 = u8p[0];	\
	u8p[0] = u8p[3];	\
	u8p[3] = temp_u8;	\
	/* swap bytes 1 and 2 */	\
	temp_u8 = u8p[1];	\
	u8p[1] = u8p[2];	\
	u8p[2] = temp_u8;	\
}



#endif /* LINUXLIBRARY_MACRO_H_ */
