#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <pthread.h>

#define BUFFER_OVERFLOW (-1)

typedef struct ringbuffer_s{
    unsigned char *buffer;
    unsigned int size;
    unsigned int fill;
    unsigned char *read;
    unsigned char *write;
    pthread_mutex_t mutex;

} m_ringbuffer_t;

/**	ringbuffer_init =>  initialize ringbuffer
 *
 * 	@param ringbuffer pointer
 * 	@return returns initialized ringbuffer
 */
m_ringbuffer_t* ringbuffer_init(int size);

/**	list_init. init's linked list and return a list head
 *
 * 	@param ringbuffer pointer
 * 	@return returns 0 for success, -1 for failure
 */
int ringbuffer_destroy(struct ringbuffer_s *);

/**	list_init. init's linked list and return a list head
 *
 * 	@param ringbuffer pointer
 * 	@return returns 0 for success, -1 for failure
 */
int ringbuffer_empty(m_ringbuffer_t *rb);

/**	list_init. init's linked list and return a list head
 *
 * 	@param ringbuffer pointer
 * 	@return returns 0 for success, -1 for failure
 */
int ringbuffer_full(m_ringbuffer_t *rb);

/**	list_init. init's linked list and return a list head
 *
 * 	@param ringbuffer pointer
 * 	@param buffer pointer
 * 	@param number of bytes to read from buffer
 * 	@return returns 0 for success, -1 for failure
 */
int ringbuffer_read(m_ringbuffer_t *rb, unsigned char* buf, unsigned int len);


/**	list_init. init's linked list and return a list head
 *
 * 	@param pointer to ringbuffer
 * 	@param buffer pointer
 * 	@param number of bytes to write into buffer
 * 	@return returns 0 for success, -1 for failure
 */
int ringbuffer_write(m_ringbuffer_t *rb, unsigned char* buf, unsigned int len);

/**	list_init. init's linked list and return a list head
 *
 * 	@param pointer to ringbuffer
 * 	@param buffer to feed data into
 * 	@param number of bytes to check
 * 	@return returns 0 for success, -1 for failure
 */
int ringbuffer_peek(m_ringbuffer_t *rb, unsigned char* buf, unsigned int len);



/**	list_init. init's linked list and return a list head
 *
 * 	@param pointer to ringbuffer
 * 	@param number of bytes to discard
 * 	@return returns 0 for success, -1 for failure
 */
int ringbuffer_discard(m_ringbuffer_t *rb, unsigned int len);

/**	list_init. init's linked list and return a list head
 *
 * 	@param pointer to linked list
 * 	@return returns 0 for success, -1 for failure
 */
int ringbuffer_length(m_ringbuffer_t *rb);

#endif
