#include <crc32.h>

/* Table of CRCs of all 8-bit messages. */
static uint32_t __crc_table[256];
/* has the table been computed? Initially false. */
static uint8_t __crc_table_computed = 0;

/**************************************************************************
 name	: __make_crc_table
 purpose	: Make the table for a fast CRC.
 input	: none
 output	: none
 *************************************************************************/
static void __make_crc_table(void)
{
    uint32_t c;
    uint16_t i, j;

    for (i = 0; i < 256; i++) {
        c = (uint32_t)i;
        for (j = 0; j < 8; j++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        __crc_table[i] = c;
    }
    __crc_table_computed = 1;
}

/**************************************************************************
 name	: calculate_crc32
 purpose	: calculates crc of the byte array
 input	: crc -> initial value of the crc
 buf -> pointer address of the buffer
 len -> length of the buffer
 output	: crc
 *************************************************************************/
uint32_t calculate_crc32(uint32_t crc, uint8_t *buf, uint32_t len)
{
    uint32_t c = crc;
    uint32_t i;

    if (!__crc_table_computed)
        __make_crc_table();

    for (i = 0; i < len; i++) {
        c = __crc_table[(c ^ buf[i]) & 0xff] ^ (c >> 8);
    }
    return c;
}
