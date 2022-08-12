#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

#define SERIAL_READ_ERROR   0x01
#define SERIAL_WRITE_ERROR  0x02

#define SERIAL_PORT_CLOSED  -1

int32_t open_serial_port(int8_t *dev);
void init_serial_port(int32_t fd);
int close_serial_port(int32_t fd);
int32_t read_serial_port(int32_t fd, uint8_t *result, uint32_t len);
int32_t write_serial_port(int32_t fd, uint8_t *buf, int32_t len);
int32_t get_baud_rate(int32_t fd);

#endif /* SERIAL_H_ */
