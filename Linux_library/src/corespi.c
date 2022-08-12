#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "log.h"
#include "common.h"
#include "corespi.h"

#define SPI_TRANSFER_MAX_WORDs          (32)
#define SPI_DUMMY_BYTE                  ((uint8_t)0xA5)
#define SPI_DUMMY_WORD                  ((uint16_t)0xA5A5)

#define SPI_MODE_DEF            0
#define SPI_BITS_DEF            16
#define SPI_SPEED_DEF           500000UL
#define SPI_DELAY_USEC_DEF      0

#define SPI_DEV_NODE0           "/dev/spidev3.0"
#define SPI_DEV_NODE1           "/dev/spidev3.1"
#define SPI_DEV_MAJOR           153     // Learned from working system by usage of "ls -l /dev" //

static const char *device = SPI_DEV_NODE0;	// specific SPI peripheral connected to GS2971A //

static int spifd = -1;
static uint8_t mode = SPI_MODE_DEF;
static uint8_t bits = SPI_BITS_DEF;
static uint32_t speed = SPI_SPEED_DEF;
static uint16_t delay_usec = SPI_DELAY_USEC_DEF;


int corespi_init(mj_log_type log_device)
{
	int ret;

	/* load "user space spi driver" module into kernel */
	// DONT Check if "spidev.ko" is installed into kernel with lsmod, if you install twice it will give error and ignore the request //
	 system("insmod /home/user/spidev.ko 1>/dev/null 2>&1");
	// Check the driver node availability in "/dev" directory //
	if( access(SPI_DEV_NODE0, F_OK) == -1 ) {
		char temp_str[64];
		memset(temp_str, 0, sizeof(temp_str));
		sprintf(temp_str, "mknod -m 666 %s c %u 0 ", SPI_DEV_NODE0 , SPI_DEV_MAJOR);
		system((const char *)temp_str);
	}
	if( access(SPI_DEV_NODE1, F_OK) == -1 ) {
		char temp_str[64];
		memset(temp_str, 0, sizeof(temp_str));
		sprintf(temp_str, "mknod -m 666 %s c %u 1", SPI_DEV_NODE1 , SPI_DEV_MAJOR);
		system((const char *)temp_str);
	}

	/* open spi device node */
	if(0 > spifd)
	    spifd = open(device, O_RDWR);

	if (0 > spifd) {
        ERRL(log_device, "open(%s) FAILED (%s)\n", device, strerror( errno ));
		ON_FAILURE(log_device, "can't open device\n", SPI_Init_Failure);
	}
	/* spi mode setting */
	ret = ioctl(spifd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		ON_FAILURE(log_device, "can't set spi mode\n", SPI_Init_Failure);

	ret = ioctl(spifd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		ON_FAILURE(log_device, "can't get spi mode\n", SPI_Init_Failure);
	if(mode != SPI_MODE_DEF)
		ON_FAILURE(log_device, "spi_mode set & read mismatch\n", SPI_Init_Failure);

	/* bits per word */
	ret = ioctl(spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		ON_FAILURE(log_device, "can't set bits per word\n", SPI_Init_Failure);

	ret = ioctl(spifd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		ON_FAILURE(log_device, "can't get bits per word\n", SPI_Init_Failure);
	if(bits != SPI_BITS_DEF)
		ON_FAILURE(log_device, "spi_bits set & read mismatch\n", SPI_Init_Failure);

	/*
	 * max speed hz
	 */
	ret = ioctl(spifd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		ON_FAILURE(log_device, "can't set max speed hz\n", SPI_Init_Failure);

	ret = ioctl(spifd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		ON_FAILURE(log_device, "can't get max speed hz\n", SPI_Init_Failure);
	if(speed != SPI_SPEED_DEF)
		ON_FAILURE(log_device, "spi_speed set & read mismatch\n", SPI_Init_Failure);

	DEBUGL(log_device, "spi mode: %d\n", mode);
	DEBUGL(log_device, "bits per word: %d\n", bits);
	DEBUGL(log_device, "max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	return 0;

SPI_Init_Failure:
	if(spifd >= 0) {
		close(spifd);
		spifd = -1;
	}
	return -1;
}

void corespi_deinit(void)
{
	if(spifd >= 0) {
		close(spifd);
		spifd = -1;
	}
}

uint8_t corespi_transfer(mj_log_type log_device, corespi_transfer_ts *spi_tr_info)
{
	uint32_t real_tr_len;
	struct spi_ioc_transfer	xfer;
	uint16_t rxbuf[SPI_TRANSFER_MAX_WORDs] ,txbuf[SPI_TRANSFER_MAX_WORDs];
	int	status;

	TRACEL(log_device, "read_buf = %p, read_words = %d, write_buf = %p, write_words = %u\n", \
		spi_tr_info->read_buf, spi_tr_info->read_words, spi_tr_info->write_buf, spi_tr_info->write_words);
	// Check INPUT parameters' validity //
	if(spi_tr_info == NULL) {
		ERRL(log_device, "Input Parameter is NULL\n");
		return FALSE;
	}
	if((spi_tr_info->write_buf == NULL) && (spi_tr_info->read_buf == NULL)) {
		ERRL(log_device, "At Leaast One of the Read or Write Buffers must be valid\n");
		return FALSE;
	}

	// Check the Read & Write Bytes Count //
	if (spi_tr_info->read_words > SPI_TRANSFER_MAX_WORDs)
		spi_tr_info->read_words = SPI_TRANSFER_MAX_WORDs;
	if (spi_tr_info->write_words > SPI_TRANSFER_MAX_WORDs)
		spi_tr_info->write_words = SPI_TRANSFER_MAX_WORDs;

	/* Fill SPI Transfer Details */
	// The bigger one of read_words or write_words will be real transfer WORDs //
	#if(DMA_RX_LAST_WORD_EMPTY_PATCH == TRUE)
		real_tr_len = (spi_tr_info->write_words > spi_tr_info->read_words) ? \
			((spi_tr_info->write_words)*2) : ((spi_tr_info->read_words)*2);
		xfer.len = real_tr_len + 2;
	#else
		real_tr_len = (spi_tr_info->write_words > spi_tr_info->read_words) ? \
			((spi_tr_info->write_words)*2) : ((spi_tr_info->read_words)*2);
	#endif
	TRACEL(log_device, "SETTO => ReadTransferLength = %d\n", real_tr_len);
	// Clear LocalRX Buffer Content //
	memset(rxbuf, 0, sizeof(rxbuf));
	xfer.rx_buf = (unsigned long)rxbuf;
	// Fill TX Buffer Content with supplied WORDs or DUMMY WORDs //
	if(spi_tr_info->write_buf != NULL) {
		// Copy the desired data to TX buffer //
		volatile uint16_t indx;
		memset(txbuf, 0, sizeof(txbuf));
		memcpy(txbuf, spi_tr_info->write_buf, (sizeof spi_tr_info->write_words)*2);
		// If write_words are less than TransferWORDs, fill rest of them with DUMMY WORD //
		if(spi_tr_info->write_words < (real_tr_len/2))
			for(indx=spi_tr_info->write_words; indx<(real_tr_len/2) ; indx++)
				txbuf[indx] = SPI_DUMMY_WORD;
	}
	else {
		// Fill TX buffer with DUMMY data //
		volatile uint16_t indx;
		for(indx=0 ; indx<(real_tr_len/2) ; indx++)
			txbuf[indx] = SPI_DUMMY_WORD;
	}
	xfer.tx_buf = (unsigned long)txbuf;
	xfer.bits_per_word = bits;
	xfer.delay_usecs = delay_usec;
	xfer.speed_hz = speed;

	/* Do Real SPI Transfer */
	status = ioctl(spifd, SPI_IOC_MESSAGE(1), &xfer);
	if (status < 0) {
		ERRL(log_device, "SPI_IOC_MESSAGE IOCTL Failed (%s)\n", strerror(errno));
		return FALSE;
	}
	else {
		spi_tr_info->read_words = spi_tr_info->write_words = status;
	}

	/* If ReadBytes is NOT IGNORED copy them to user supplied transfer details structure */
	if(spi_tr_info->read_buf != NULL) {
		memset(spi_tr_info->read_buf, 0, spi_tr_info->read_words);
		memcpy(spi_tr_info->read_buf, rxbuf, spi_tr_info->read_words);
	}

	/* Logging of Received Bytes if Desired */
	#if(1)
	{
		uint16_t *bp = (uint16_t *)rxbuf;
		uint16_t Length = real_tr_len/2;
		TRACEL(log_device, "response(%2d, %2d): ", Length, status);
		for (; Length; Length--)
			TRACEL(log_device, " %04x", *bp++);
		TRACEL(log_device, "\n");
	}
	#endif

	return TRUE;
}
