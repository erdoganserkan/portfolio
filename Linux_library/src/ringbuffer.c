#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "ringbuffer.h"


/**************************************************************************
name	: m_ringbuffer_init
purpose	: initialize ringbuffer
input	: none
output	: none
***************************************************************************/
struct m_ringbuffer_s * m_ringbuffer_init(int size)
{
  struct m_ringbuffer_s* rb = NULL;

  rb = (struct m_ringbuffer_s*)malloc(sizeof(struct m_ringbuffer_s));
  rb->size = size;
  rb->fill = 0;
  rb->buffer = (unsigned char*)malloc(sizeof(unsigned char)*rb->size);
  rb->read = rb->buffer;
  rb->write = rb->buffer;
  pthread_mutex_init(&(rb->mutex), NULL);


  return rb;
}

/**************************************************************************
name	: m_ringbuffer_destroy
purpose	: free ringbuffer resources and destroy related info
input	: none
output	: none
***************************************************************************/
int m_ringbuffer_destroy(struct m_ringbuffer_s *rb)
{
  free(rb->buffer);
  pthread_mutex_destroy(&(rb->mutex));
  free(rb);
}


/**************************************************************************
name	: m_ringbuffer_empty
purpose	: determine whether ringbuffer is empty or not
input	: none
output	: none
***************************************************************************/
int m_ringbuffer_empty(m_ringbuffer_t *rb)
{
  int ret;
  pthread_mutex_lock(&(rb->mutex));
	/* It's empty when the read and write pointers are the same. */
	if (0 == rb->fill) {
		ret = 1;
	}else {
		ret = 0;
	}

  pthread_mutex_unlock(&(rb->mutex));
  return ret;
}

/**************************************************************************
name	: m_ringbuffer_full
purpose	: determine whether ringbuffer is full or not
input	: none
output	: none
***************************************************************************/
int m_ringbuffer_full(m_ringbuffer_t *rb)
{
  int ret;
  pthread_mutex_lock(&(rb->mutex));
	/* It's full when the write ponter is 1 element before the read pointer*/
	if (rb->size == rb->fill) {
		ret = 1;
	}else {
		ret = 0;
	}

  pthread_mutex_unlock(&(rb->mutex));
  return ret;
}

/**************************************************************************
name	: m_ringbuffer_length
purpose	: get ringbuffer length
input	: none
output	: none
***************************************************************************/
int m_ringbuffer_length(m_ringbuffer_t *rb)
{
  return rb->fill;
}

/**************************************************************************
name	: m_ringbuffer_peek
purpose	: peek into ringbuffer contents without modifying read pointer
input	: none
output	: none
***************************************************************************/
int m_ringbuffer_peek(m_ringbuffer_t *rb, unsigned char* buf, unsigned int len)
{
  unsigned char *origin = rb->read;
  int ret;

  pthread_mutex_lock(&(rb->mutex));
  if (rb->fill >= len) {
		// in one direction, there is enough data for retrieving
		if (rb->write > rb->read) {
			memcpy(buf, rb->read, len);
			rb->read += len;
		}else if (rb->write < rb->read) {
			int len1 = rb->buffer + rb->size - 1 - rb->read + 1;
			if (len1 >= len) {
				memcpy(buf, rb->read, len);
				rb->read += len;
			} else {
				int len2 = len - len1;
				memcpy(buf, rb->read, len1);
				memcpy(buf + len1, rb->buffer, len2);
				rb->read = rb->buffer + len2; // Wrap around
			}
		}
		
    rb->read = origin;  // keep everything intact
    //rb-> fill -= len;
		ret = len;

	} else	{
		ret = 0;
	}

  pthread_mutex_unlock(&(rb->mutex));
  return ret;
}

/**************************************************************************
name	: m_ringbuffer_discard
purpose	: remove bytes from ringbuffer by len
input	: none
output	: none
***************************************************************************/
int m_ringbuffer_discard(m_ringbuffer_t *rb, unsigned int len)
{
  int ret;
  pthread_mutex_lock(&(rb->mutex));
  if( (len > 0) && (rb->fill >= len)){    
    if(rb->write > rb->read){
      rb->read += len;
    } else {
      int len1 = rb->buffer + rb->size - 1 - rb->read + 1;
      if(len1 >= len){
        rb->read += len;
      } else {
        int len2 = len -len1;
        rb->read = rb->buffer + len2;
      }
    }

    rb->fill -= len;
    ret =  len;

  }else{
    ret = -1;
  }

  pthread_mutex_unlock(&(rb->mutex));
  return ret;
}



/**************************************************************************
name	: m_ringbuffer_read
purpose	: read ringbuffer contents into buffer
input	: none
output	: none
***************************************************************************/
int m_ringbuffer_read(m_ringbuffer_t *rb, unsigned char* buf, unsigned int len)
{
  int ret;
	assert(len>0);

  pthread_mutex_lock(&(rb->mutex));
	if (rb->fill >= len) {
		// in one direction, there is enough data for retrieving
		if (rb->write > rb->read) {
			memcpy(buf, rb->read, len);
			rb->read += len;
		}else if (rb->write < rb->read) {
			int len1 = rb->buffer + rb->size - 1 - rb->read + 1;
			if (len1 >= len) {
				memcpy(buf, rb->read, len);
				rb->read += len;
			} else {
				int len2 = len - len1;
				memcpy(buf, rb->read, len1);
				memcpy(buf + len1, rb->buffer, len2);
				rb->read = rb->buffer + len2; // Wrap around
			}
		}
		rb-> fill -= len;
		ret =  len;
	} else	{
		ret =  0;
	}

  pthread_mutex_unlock(&(rb->mutex));
  return ret;
}

/**************************************************************************
name	: m_ringbuffer_write
purpose	: write buffer contents into ringbuffer
input	: none
output	: none
***************************************************************************/
int m_ringbuffer_write(m_ringbuffer_t *rb, unsigned char* buf, unsigned int len)
{
  
  int ret;
	assert(len > 0);


  pthread_mutex_lock(&(rb->mutex));
	if (rb->size - rb->fill < len) {
		ret =  0;
	}
	else {
		if (rb->write >= rb->read) {
			int len1 = rb->buffer + rb->size - rb->write;

			if (len1 >= len) {
				memcpy(rb->write, buf, len);
				rb->write += len;
			} else {
				int len2 = len - len1;
				memcpy(rb->write, buf, len1);
				memcpy(rb->buffer, buf+len1, len2);
				rb->write = rb->buffer + len2; // Wrap around
			}
		} else {
			memcpy(rb->write, buf, len);
			rb->write += len;
		}
		rb->fill += len;
		ret =  len;
	}

  pthread_mutex_unlock(&(rb->mutex));
  return ret;
}
