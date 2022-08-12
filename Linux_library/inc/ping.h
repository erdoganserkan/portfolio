#ifndef PING_H_
#define PING_H_

#include <stdint.h>

#define	DEFDATALEN	(64-ICMP_MINLEN)	/* default data length */
#define	MAXIPLEN	60
#define	MAXICMPLEN	76
#define	MAXPACKET	(65536 - 60 - ICMP_MINLEN)/* max packet size */

int32_t ping(int8_t *target, int8_t *interface);
uint16_t in_cksum(uint16_t *addr, uint16_t len);

#endif /* PING_H_ */
