#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <flag.h>
#include <log.h>

/**************************************************************************
 name	: flag_create
 purpose	: This function create flag group
 input	: flag -> initial value of flags,
 err -> return error
 output	: pointer of flag_grp_t
 *************************************************************************/
flag_grp_t *flag_create(uint32_t flag, uint8_t *err)
{
    flag_grp_t *flag_grp = (flag_grp_t *)calloc(sizeof(1,flag_grp_t));

    if (flag_grp == NULL) {
        *err = FLAG_ERR_NULL;
        ERRL(logi, "flag_create no allocate memory\n");
        return NULL;
    }
    flag_grp->usec = 0;
    flag_grp->flag = flag;

    pthread_cond_init(&flag_grp->cond, NULL);
    pthread_mutex_init(&flag_grp->mutex, NULL);

    return flag_grp;
}

/**************************************************************************
 name	: flag_pend
 purpose : This function is called to wait for a combination of bits to
 be set in an event flag group
 input	: flag_grp -> pointer of flag,
 event -> events of flag, timeout -> for response
 output	: flag group value
 *************************************************************************/
uint32_t flag_pend(flag_grp_t *flag_grp, uint32_t flags,
    uint32_t wait_type, uint32_t timeout_s, uint8_t *err)
{
    uint32_t tmp_flag_grp;
    int ret = 0;
    struct timespec ts;

    if (flag_grp == NULL) {
        *err = FLAG_ERR_NULL;
        ERRL(logi, "flag_pend null operation\n");
        return 0;
    }

    pthread_cond_t *cond = &(flag_grp->cond);
    pthread_mutex_t *mutex = &(flag_grp->mutex);
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        *err = FLAG_ERR_CLOCK;
        ERRL(logi, "clock_gettime error(%s)\n", strerror(errno));

    }

    ts.tv_sec += timeout_s + 1;
    ts.tv_nsec = 0;

    pthread_mutex_lock(mutex);

    if (wait_type & WAIT_FLAG_SET_ANY) {
        while ((ret != ETIMEDOUT) && !(flag_grp->flag & flags)) {
            if (timeout_s > 0)
                ret = pthread_cond_timedwait(cond, mutex, &ts);
            else
                pthread_cond_wait(cond, mutex);
        }
    }
    else if (wait_type & WAIT_FLAG_CLR_ANY) {
        while ((ret != ETIMEDOUT) && (flags == (flag_grp->flag & flags))) {
            if (timeout_s > 0)
                ret = pthread_cond_timedwait(cond, mutex, &ts);
            else
                pthread_cond_wait(cond, mutex);
        }
    }

    tmp_flag_grp = flag_grp->flag;

    if (wait_type & FLAG_CONSUME)
        flag_grp->flag &= ~flags;

    if (ret == ETIMEDOUT)
        *err = FLAG_ERR_TIMEOUT;
    else
        *err = FLAG_NO_ERROR;

    pthread_mutex_unlock(mutex);

    return tmp_flag_grp;
}

/**************************************************************************
 name   : flag_pend_us
 purpose : This function is called to wait for a combination of bits to
 be set in an event flag group
 input  : flag_grp -> pointer of flag,
 event -> events of flag, timeout -> for response
 output : flag group value
 *************************************************************************/
uint32_t flag_pend_us(flag_grp_t *flag_grp, uint32_t flags,
    uint32_t wait_type, uint32_t timeout_us, uint8_t *err)
{
    uint32_t tmp_flag_grp;
    int ret = 0;
    struct timespec ts;
    uint64_t tempU64;

    if (flag_grp == NULL) {
        *err = FLAG_ERR_NULL;
        ERRL(logi, "flag_pend null operation\n");
        return 0;
    }

    pthread_cond_t *cond = &(flag_grp->cond);
    pthread_mutex_t *mutex = &(flag_grp->mutex);
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        *err = FLAG_ERR_CLOCK;
        ERRL(logi, "clock_gettime error\n");
    }

    // calculate the desired absolute time //
    tempU64 = ((uint64_t)timeout_us*1000UL) + (uint64_t)ts.tv_nsec;
    if(tempU64 > 1000000000UL) {
        ts.tv_nsec = tempU64 - 1000000000UL;
        ts.tv_sec += 1;
    } else {
        ts.tv_nsec = tempU64;
    }

    pthread_mutex_lock(mutex);

    if (wait_type & WAIT_FLAG_SET_ANY) {
        while ((ret != ETIMEDOUT) && !(flag_grp->flag & flags)) {
            if (timeout_us > 0)
                ret = pthread_cond_timedwait(cond, mutex, &ts);
            else
                pthread_cond_wait(cond, mutex);
        }
    }
    else if (wait_type & WAIT_FLAG_CLR_ANY) {
        while ((ret != ETIMEDOUT) && (flags == (flag_grp->flag & flags))) {
            if (timeout_us > 0)
                ret = pthread_cond_timedwait(cond, mutex, &ts);
            else
                pthread_cond_wait(cond, mutex);
        }
    }

    tmp_flag_grp = flag_grp->flag;

    if (wait_type & FLAG_CONSUME)
        flag_grp->flag &= ~flags;

    if (ret == ETIMEDOUT)
        *err = FLAG_ERR_TIMEOUT;
    else
        *err = FLAG_NO_ERROR;

    pthread_mutex_unlock(mutex);

    return tmp_flag_grp;
}


/**************************************************************************
 name	: flag_post
 purpose	: set or clear flag
 input	: address of flag, value -> set or clear
 output	:
 *************************************************************************/
void flag_post(flag_grp_t *flag_grp, uint32_t flags, uint8_t value)
{
    struct timeval tm;
    long usec = 0;

    if (flag_grp == NULL) {
        ERRL(logi, "flag_post null operation\n");
        return;
    }

    gettimeofday(&tm, NULL);
    usec = (tm.tv_sec * 1000000) + tm.tv_usec;

    if ((usec - flag_grp->usec) < OS_FLAG_POST_DELAY) {
        usleep(OS_FLAG_POST_DELAY);
        gettimeofday(&tm, NULL);
        usec += OS_FLAG_POST_DELAY;
    }

    gettimeofday(&tm, NULL);
    usec = (tm.tv_sec * 1000000) + tm.tv_usec;
    flag_grp->usec = usec + OS_FLAG_POST_DELAY;

    pthread_cond_t *cond = &(flag_grp->cond);
    uint32_t tmp = flag_grp->flag;

    if (value == OS_FLAG_SET)
        flag_grp->flag |= flags;
    else
        flag_grp->flag &= ~flags;
    pthread_cond_signal(cond);
}

/**************************************************************************
 name	: flag_delete
 purpose	: This function delete flag group
 input	: flag_grp -> flag_grp that will be deleted
 output	: none
 *************************************************************************/
void flag_delete(flag_grp_t *flag_grp)
{
	if(!flag_grp)
		return;
    pthread_cond_destroy(&(flag_grp->cond));
    pthread_mutex_destroy(&(flag_grp->mutex));
    free(flag_grp);
    flag_grp = NULL;
}
