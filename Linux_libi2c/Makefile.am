
lib_LIBRARIES=libi2c.a

LOGGER=$(top_srcdir)/../Log_C

libi2c_a_SOURCES=\
	src/i2cbusses.c \
	src/util.c \
	src/i2cget.c \
	src/i2cset.c \
	src/libc2i2c_common.c

AM_CFLAGS = -lresolv -ldl -Wall -Wstrict-prototypes -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings \
	-Wnested-externs -Winline -W -Wundef -Wmissing-prototypes -I$(LOGGER) 
AM_CPPFLAGS =  -D_GNU_SOURCE -O2 -Wstrict-prototypes -Wall -I./inc -DRESOLVE_HOSTNAMES 
