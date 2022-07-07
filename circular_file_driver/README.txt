### PC COMPILE ###
* compile this driver on server machine (ubuntu 10.04 ccmtp kernel)
* ln -s Makefile.pc Makefile -f
* make clean all 
* copy *.ko driver into "/home/modeo/conf" directory in target machine 

### OMAP COMPILE ###
* ln -s Makefile.omap Makefile
* make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- clean all

