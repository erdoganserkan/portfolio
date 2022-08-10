lib_LIBRARIES=libadv.a

LIBI2C=$(top_srcdir)/../Linux_libi2c
LOGGER=$(top_srcdir)/../Log_C

libadv_a_SOURCES=\
	src/libc2adv_common.c \
	src/adv7611.c \
	src/IF.c \
	src/adv7181c.c \
	src/cs53l21.c

#AM_CFLAGS = -lresolv -ldl -libmodeo.a
AM_CFLAGS = -lresolv -ldl -lc2i2c -Wall -Wstrict-prototypes -Wshadow -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wnested-externs -Winline -W -Wundef -Wmissing-prototypes
AM_CPPFLAGS =  -D_GNU_SOURCE -O2 -Wstrict-prototypes -Wall -I./inc -I$(LOGGER) -I$(LIBI2C)/inc \
	-L./lib -DRESOLVE_HOSTNAMES 
