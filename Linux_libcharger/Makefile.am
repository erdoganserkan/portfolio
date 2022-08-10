
lib_LIBRARIES=libcharger.a

LOGGER=$(top_srcdir)/../Log_C
LIBCONFIG=$(top_srcdir)/../thirdparty/libconfig
LIBI2C=$(top_srcdir)/../Linux_libi2c

libcharger_a_SOURCES= \
	src/libc2charger.c \
	src/battery_values.c

#AM_CFLAGS = -lresolv -ldl -libmodeo.a
AM_CFLAGS = -lresolv -ldl -Wall -Wstrict-prototypes -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings \
	-I$(LOGGER) -I$(LIBCONFIG)/inc -I$(LIBCONFIG)/lib -Wnested-externs -Winline -W -Wundef -Wmissing-prototypes
AM_CPPFLAGS =  -D_GNU_SOURCE -O2 -Wstrict-prototypes -Wall -I./inc -I$(LIBI2C)/inc \
	-I$(LIBCONFIG)/inc -I$(LIBCONFIG)/lib -DRESOLVE_HOSTNAMES 
