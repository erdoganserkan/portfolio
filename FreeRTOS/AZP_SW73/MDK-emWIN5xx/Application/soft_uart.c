#include <stdint.h>

#define MAKE_SUART_PIN_OUTPUT()	 	{}
#define SET_SUART_PIN(x)	 		{x}
#define WAIT_US(x)					{x}

static uint16_t bit_wait_us = 0;

void init_suart(uint16_t baud)
{
	bit_wait_us = 1000000UL / baud;
	MAKE_SUART_PIN_OUTPUT();
	SET_SUART_PIN(1);
}

void suart_print_ch(uint8_t ch)
{
	volatile uint8_t indx;

	// Send start bit // 
	SET_SUART_PIN(0);
	WAIT_US(bit_wait_us);

	// Send real data // 
	for(indx=0 ; indx<8 ; indx++) {
		WAIT_US(bit_wait_us>>1);	// Wait Half bit length before REAL-BIT // 
		SET_SUART_PIN((ch>>indx) & 0x01);	// Send REAL-BIT // 
		WAIT_US(bit_wait_us>>1);	// Wait Half bit length after REAL-BIT // 
	}

	// send STOP bit // 
	SET_SUART_PIN(1);
	WAIT_US(bit_wait_us);
}

void suart_print_hex2ascii(uint8_t hex_buffer*, uint8_t len)
{
	volatile uint8_t indx=0;
	char low_nibble, high_nibble;
	char ch;

	while(0 != len) {
		// calculate high and low nibbles // 
		high_nibble = ((hex_buffer[indx]& 0xF0)>>4);
		low_nibble = (hex_buffer[indx] & 0x0F);

		// print high nibble // 
		if(high_nibble > 9)
			ch = (high_nibble - 10) + 'A';
		else
			ch = (high_nibble) + '0';
		suart_print_ch(ch);

		// print low nibble // 
		if(low_nibble > 9)
			ch = (low_nibble - 10) + 'A';
		else
			ch = (low_nibble) + '0';
		suart_print_ch(ch);

		// get prepared for next byte // 
		indx++;
		len--;
	}	
}


#define SUART_MAX_STR_LEN	(30)
void suart_print_str(char *str)
{
	volatile uint8_t indx = 0;
	if(NULL == str)
		return;
	while(('\0' != (*str)) && (SUART_MAX_STR_LEN != indx)) {
		suart_print_ch(*str++);
		indx++;
	}
}
