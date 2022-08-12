#ifndef _CORESPI_H_
#define _CORESPI_H_

#include <stdio.h>
#include <stdint.h>
#include <log.h>

#define DMA_RX_LAST_WORD_EMPTY_PATCH TRUE   // OMAP3530 MCSPI LAstWOrd Error WORK AROUND

typedef struct {
	uint16_t read_words;
	uint16_t write_words;
	uint16_t *read_buf;
	uint16_t *write_buf;
} corespi_transfer_ts;

extern uint8_t corespi_transfer(mj_log_type log_device, corespi_transfer_ts *SpiTrDetails);
extern int corespi_init(mj_log_type log_device);
extern void corespi_deinit(void);

#endif /* CORESPI_H_ */
