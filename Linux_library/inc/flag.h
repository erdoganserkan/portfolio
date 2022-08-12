#ifndef FLAG_H_
#define FLAG_H_

#include <stdint.h>
#include <pthread.h>

#define FLAG_NO_ERROR		0x00
#define FLAG_ERR_TIMEOUT	0x01
#define FLAG_ERR_CLOCK		0x02
#define FLAG_ERR_NULL		0x03

#define WAIT_FLAG_SET_ANY	0x01
#define WAIT_FLAG_CLR_ANY	0x02

#define FLAG_CONSUME		0x80

#define OS_FLAG_SET			0x01
#define OS_FLAG_CLR			0x00

/*50 useconds wait between to flag post*/
#define OS_FLAG_POST_DELAY 		50

typedef struct flag_grp_s
{
    uint32_t flag;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    long usec;
} flag_grp_t;

/*Prototypes*/
flag_grp_t *flag_create(uint32_t flag, uint8_t *err);

void flag_delete(flag_grp_t *flag_grp);

uint32_t flag_pend(flag_grp_t *flag_grp, uint32_t flags,
    uint32_t wait_type, uint32_t timeout_s, uint8_t *err);
uint32_t flag_pend_us(flag_grp_t *flag_grp, uint32_t flags,
    uint32_t wait_type, uint32_t timeout_us, uint8_t *err);

void flag_post(flag_grp_t *flag_grp, uint32_t flags, uint8_t value);

#endif /*ifndef FLAG_H_*/
