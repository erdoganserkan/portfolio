#include <linux/fs.h>         /* file stuff */
#include <linux/kernel.h>     /* printk() */
#include <linux/errno.h>      /* error codes */
#include <linux/module.h>     /* THIS_MODULE */
#include <linux/cdev.h>       /* char device stuff */
#include <asm/uaccess.h>      /* copy_to_user() */
#include <linux/init.h>
#include <linux/sched.h>  /* current and everything */
#include <linux/types.h>  /* size_t */
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/version.h>

/* Add this header if you don't already have it included */
#include <linux/kdev_t.h>
#include "circular_file.h"

#define TS_SIZE 188
#define BUFSIZE	(((1024UL * 1024UL * DEFAULT_BUFFER_SIZE_MB)/TS_SIZE)*TS_SIZE)
#define FILL_BUFFER_UP_MAX		((((BUFSIZE * 9)/10)/TS_SIZE)*TS_SIZE)
#define READING_SKIP_STEP_SIZE	((((BUFSIZE)/3)/TS_SIZE)*TS_SIZE)

static int device_major_number = 0;
static const char device_name[] = CIRCULAR_FILE_DRIVER_NAME;

static uint8_t global_buffer[MAX_CIRCULAR_DEVICES_COUNT][BUFSIZE];
static volatile uint32_t tail[MAX_CIRCULAR_DEVICES_COUNT];	// reading pointer //
static volatile uint32_t head[MAX_CIRCULAR_DEVICES_COUNT];	// writing pointer //
static struct file *fileptrs[MAX_CIRCULAR_DEVICES_COUNT][3];
static uint32_t initial_delay_s[MAX_CIRCULAR_DEVICES_COUNT];	// Changes by user process according to delay setting on GUI //
static struct timer_list cw_timers[MAX_CIRCULAR_DEVICES_COUNT];
static uint8_t volatile read_ready[MAX_CIRCULAR_DEVICES_COUNT];
static uint8_t volatile wr_started[MAX_CIRCULAR_DEVICES_COUNT];
read_stat_t read_stats[MAX_CIRCULAR_DEVICES_COUNT];

DECLARE_WAIT_QUEUE_HEAD(wq0);
DECLARE_WAIT_QUEUE_HEAD(wq1);
DECLARE_WAIT_QUEUE_HEAD(wq2);
DECLARE_WAIT_QUEUE_HEAD(wq3);
DECLARE_WAIT_QUEUE_HEAD(wq4);
DECLARE_WAIT_QUEUE_HEAD(wq5);
DECLARE_WAIT_QUEUE_HEAD(wq6);
DECLARE_WAIT_QUEUE_HEAD(wq7);
DECLARE_WAIT_QUEUE_HEAD(wq8);
DECLARE_WAIT_QUEUE_HEAD(wq9);
static wait_queue_head_t *queue_heads[MAX_CIRCULAR_DEVICES_COUNT];	// used for blocking io //

static int dev_open(struct inode *, struct file *);
static int dev_close(struct inode *, struct file *);
static ssize_t dev_file_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_file_write(struct file *, const char __user *, size_t, loff_t *);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int dev_ioctl(struct inode *inode, struct file*filep, unsigned int cmd, unsigned long arg);
#else
	static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);
#endif

static inline uint32_t GET_AVAILABLE_DATA_SIZE(uint8_t minor);
static inline void CHECK_and_CORRECT_AVAILABLE_DATA(uint8_t minor);

static struct file_operations simple_driver_fops =
{
   .owner   = THIS_MODULE,
   .open    = dev_open,
   .release = dev_close,
   .read    = dev_file_read,
   .write   = dev_file_write,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
   .ioctl 	= dev_ioctl
#else
    .unlocked_ioctl = dev_ioctl
#endif
};

void cb_tmr_callback( unsigned long minor )
{
	printk(KERN_INFO "%s: tmr_callback, MINOR(%u), available(%u KB), called (jiffies: %lu).\n", \
		CIRCULAR_FILE_DRIVER_NAME, minor, GET_AVAILABLE_DATA_SIZE(minor)/1024, jiffies );
	if(!wr_started[minor]) {
		printk(KERN_INFO "%s: tmr_callback, MINOR(%u), write_close detected\n");
	} else
		read_ready[minor] = 1;
}

void cb_tmr_callback2( struct timer_list *tmrs )
{
	int minor = (tmrs - cw_timers) ;
	printk(KERN_INFO "%s: tmr_callback, MINOR(%u), available(%u KB), called (jiffies: %lu).\n", \
		CIRCULAR_FILE_DRIVER_NAME, minor, GET_AVAILABLE_DATA_SIZE(minor)/1024, jiffies );
	if(!wr_started[minor]) {
		printk(KERN_INFO "%s: tmr_callback, MINOR(%u), write_close detected\n");
	} else
		read_ready[minor] = 1;
}

static void start_timer(uint8_t minor, uint32_t forced_delay_ms) {
	uint32_t delayms;
	int ret;

	if(0xFF == forced_delay_ms)
		delayms = initial_delay_s[minor]*1000;
	else
		delayms = forced_delay_ms;
	printk(KERN_INFO "%s: MINOR(%u), Starting timer for (%u msec), current jiffies(%lu)\n", \
		CIRCULAR_FILE_DRIVER_NAME, minor, delayms, jiffies);
	ret = mod_timer( &(cw_timers[minor]), jiffies + msecs_to_jiffies(delayms) );
	if (ret)
		printk("%s: MINOR(%u), Error in mod_timer\n", CIRCULAR_FILE_DRIVER_NAME, minor);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int dev_ioctl(struct inode *inode, struct file*filep, unsigned int cmd, unsigned long arg)
#else
static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
#endif
{
	uint32_t minor = iminor(filep->f_path.dentry->d_inode);
	//printk(KERN_INFO "%s: MINOR(%u) IOCTL(%u)\n", CIRCULAR_FILE_DRIVER_NAME, minor, cmd);

    switch (cmd) {
		case IOCTL_CB_SET_DELAY: {
			unsigned long failed_bytes;
			void *user_buffer = (void *)arg;
			uint32_t tr_size = sizeof(uint32_t);
			uint32_t delay;

			failed_bytes = copy_from_user(&delay, user_buffer, tr_size);
			if(!failed_bytes) {
				if(MAX_INITIAL_DELAY_S < delay) {
					printk(KERN_INFO "desired delay(%u) > MAX(%u), TRUNCATING\n", delay, MAX_INITIAL_DELAY_S);
					delay = MAX_INITIAL_DELAY_S;
				}
				initial_delay_s[minor] = delay;
				printk(KERN_INFO "%s: MINOR(%u), IOCTL(%u): new delay(%u sec)\n", \
					CIRCULAR_FILE_DRIVER_NAME, minor, cmd, delay);
			} else {
				printk(KERN_ERR "%s: MINOR(%u), IOCTL(%u) copy_from_user(%u/%u bytes) FAILED\n", \
					CIRCULAR_FILE_DRIVER_NAME, minor, cmd, failed_bytes, tr_size);
				return -EINVAL;
			}
		}
		break;
    case IOCTL_CB_GET_DELAY: {
		unsigned long failed_bytes;
		void *user_buffer = (void *)arg;
		uint32_t tr_size = sizeof(uint32_t);
		uint32_t delay = initial_delay_s[minor];

		failed_bytes = copy_to_user(user_buffer, &delay, tr_size);
		if(failed_bytes) {
			printk(KERN_ERR "%s: MINOR(%u), IOCTL(%u) copy_to_user(%u/%u bytes) FAILED\n", \
				CIRCULAR_FILE_DRIVER_NAME, minor, cmd, failed_bytes, tr_size);
			return -EINVAL;
		}
    }
    break;
    case IOCTL_CB_GET_READ_ERRS: {
		unsigned long failed_bytes;
		void *user_buffer = (void *)arg;
		uint32_t tr_size = sizeof(read_stat_t);

		failed_bytes = copy_to_user(user_buffer, &(read_stats[minor]), tr_size);
		if(failed_bytes) {
			printk(KERN_ERR "%s: MINOR(%u), IOCTL(%u) copy_to_user(%u/%u bytes) FAILED\n", \
				CIRCULAR_FILE_DRIVER_NAME, minor, cmd, failed_bytes, tr_size);
			return -EINVAL;
		}
    }
   	break;
    default:
    	printk(KERN_ERR "%s: UNEXPECTED IOCTL CMD(%u)\n", CIRCULAR_FILE_DRIVER_NAME, cmd);
        break;
    }
    return 0;
}

static int dev_open(struct inode *inodep, struct file *filep)
{
	unsigned int minor = MINOR(inodep->i_rdev);

	if(O_NONBLOCK & filep->f_flags) {	// If device node is opened for reading purpose //
		//printk(KERN_INFO "%s: MINOR(%u) IOCTL open detected\n", CIRCULAR_FILE_DRIVER_NAME, minor);
		fileptrs[minor][2] = filep;
	} else {
		printk(KERN_INFO "%s: device opened with flasgs(0x%X).\n", CIRCULAR_FILE_DRIVER_NAME, filep->f_flags);
		printk(KERN_INFO "%s: driver open() process is \"%s\" (pid %i)\n",
			CIRCULAR_FILE_DRIVER_NAME, current->comm, current->pid);
		printk(KERN_INFO "%s: current MINOR(%u), FILEP(%p)\n", CIRCULAR_FILE_DRIVER_NAME, minor, filep);
		if(O_RDONLY == (filep->f_flags & 0x3)) {	// If device node is opened for reading purpose //
			printk(KERN_INFO "%s: MINOR(%u) read_open\n", CIRCULAR_FILE_DRIVER_NAME, minor);
			fileptrs[minor][0] = filep;
			read_stats[minor].read_timeout_cnt = 0;
			read_stats[minor].read_slow_cnt = 0;
			if(!read_ready[minor]) {
				printk(KERN_INFO "%s: This is INITIAL READ OPEN\n", CIRCULAR_FILE_DRIVER_NAME);
				tail[minor] = 0;
			} else {
				printk(KERN_INFO "%s: This is RESTARTED READ OPEN\n", CIRCULAR_FILE_DRIVER_NAME);
				tail[minor] = head[minor];
				read_ready[minor]=0;
				start_timer(minor, RESTARTED_OPEN_INITIAL_DELAY_S*1000U);
			}
			//read_ready[minor] = 0;
		} else {
			printk(KERN_INFO "%s: MINOR(%u) write_open\n", CIRCULAR_FILE_DRIVER_NAME, minor);
			fileptrs[minor][1] = filep;
			head[minor] = 0;
			wr_started[minor] = 0;
			read_ready[minor] = 0;
			read_stats[minor].read_slow_cnt = 0;
		}
	}

   return 0;
}

static int dev_close(struct inode *inodep, struct file *filep)
{
	unsigned int minor = MINOR(inodep->i_rdev);
	//printk(KERN_INFO "%s: dev_close request, MINOR(%u), filep(%p)\n", \
		CIRCULAR_FILE_DRIVER_NAME, minor, filep);

	// check if this is a write_clse or read_close //
	if(fileptrs[minor][2] == filep) {
		//printk(KERN_INFO "%s: MINOR(%u) IOCTL close detected\n", CIRCULAR_FILE_DRIVER_NAME, minor);
		return 0;
	} else if(fileptrs[minor][1] == filep) {
		fileptrs[minor][1] = NULL;
		head[minor] = 0;
		//wr_started[minor]=0;
		printk(KERN_INFO "%s: MINOR(%u) write_close\n", CIRCULAR_FILE_DRIVER_NAME, minor);
		return 0;
	}
	else if(fileptrs[minor][0] == filep) {
		fileptrs[minor][0] = NULL;
		tail[minor] = 0;
		//read_ready[minor]=0;
		printk(KERN_INFO "%s: MINOR(%u) read_close\n", CIRCULAR_FILE_DRIVER_NAME, minor);
		return 0;
	}

   printk(KERN_ERR "%s: read/write_close detection FAILED(MINOR(%u), pfilep(%p))", \
		   CIRCULAR_FILE_DRIVER_NAME, minor, filep);

   return 0;
}

static inline uint32_t GET_AVAILABLE_DATA_SIZE(uint8_t minor) {
	uint32_t head_ = head[minor];
	uint32_t available = 0;
	uint32_t tail_ = tail[minor];

	if(head_ > tail_)
		available = (head_ - tail_);
	else if(tail_ > head_)
		available = head_ + (BUFSIZE - tail_);

	return available;
}

static inline void CHECK_and_CORRECT_AVAILABLE_DATA(uint8_t minor)  {
	uint32_t available;

	available = GET_AVAILABLE_DATA_SIZE(minor);
	if(0 == available)
		return;

	if(FILL_BUFFER_UP_MAX <= available) {
		uint32_t backup = available;
		available -= READING_SKIP_STEP_SIZE;
		printk(KERN_INFO "%s: MINOR(%u), REDUCING buffer_fill(%u -> %u)", \
			CIRCULAR_FILE_DRIVER_NAME, minor, backup, available);
		tail[minor] += READING_SKIP_STEP_SIZE;
		if(tail[minor] >= BUFSIZE)
			tail[minor] -= BUFSIZE;
	}
}

static ssize_t dev_file_read(struct file *filep, char __user *user_buffer, size_t count, loff_t *offset)
{
	uint32_t head_;
	unsigned long failed_bytes;
	uint8_t volatile read_ready_timeout_cnt = 0;
	uint8_t *rd_buffer = NULL;
	//printk( KERN_NOTICE "Circular-File: device file is read at offset = %i, read bytes count = %u", (int)*offset, (unsigned int)count );
	//printk(KERN_NOTICE "Circular-File: head(%u) tail(%u)\n", head, tail);

	uint32_t minor = iminor(filep->f_path.dentry->d_inode);

CHECK_READ_READY:
	if(!read_ready[minor]) {
		int ret;
		if (filep->f_flags & O_NONBLOCK)
			return -EAGAIN;
		ret = wait_event_interruptible_timeout((*queue_heads[minor]), 0 != read_ready[minor], schedule_timeout(HZ));
		if(ret > 0) {
			goto READ_IS_READY;	// condition TRUE after/before timeout, WE CAN GOON (READ is READY) //
		} else if(0 == ret) {
			read_ready_timeout_cnt++;
			printk(KERN_INFO "%s: MINOR(%u), read_READY_timeout, cnt(%u)\n", CIRCULAR_FILE_DRIVER_NAME, minor, read_ready_timeout_cnt);	// condition FALSE, after timeout //
			if(MAX_INITIAL_WAIT_S != read_ready_timeout_cnt)
				goto CHECK_READ_READY;
			else {
				printk(KERN_INFO "%s: MINOR(%u), read_READY_timeout_cnt is MAX(%u)\n", \
					CIRCULAR_FILE_DRIVER_NAME, minor, MAX_INITIAL_WAIT_S);
				return -ENODATA;
			}
		} else if (ret == (-ERESTARTSYS)) {
			printk(KERN_INFO "%s: MINOR(%u), read_ready_timeout interrupted by signal\n", CIRCULAR_FILE_DRIVER_NAME, minor);
			return ret;
		}
		else {
			printk(KERN_INFO "%s: MINOR(%u), unexpected read_driver return(%d)\n", CIRCULAR_FILE_DRIVER_NAME, minor, ret);
			return 0;
		}
	}

READ_IS_READY:
	// find which buffer to write by using filep //
	#if(0)
		uint8_t indx;
		for(indx=0; indx<MAX_CIRCULAR_DEVICES_COUNT; indx++) {
			if(fileptrs[indx][1] == filep) {
				wr_buffer = buffer[indx];
				minor = indx;
				break;
			}
		}
		if(NULL != rd_buffer) {
			printk(KERN_ERR, "%s: rd_buffer NOT FOUND\n", CIRCULAR_FILE_DRIVER_NAME);
			return -1;
		}
	#else
		rd_buffer = global_buffer[minor];
	#endif
	//printk(KERN_INFO "%s: READING FILEP(%p), MINOR(%u)\n", CIRCULAR_FILE_DRIVER_NAME, filep, minor);

RETRY_READ:
	//CHECK_and_CORRECT_AVAILABLE_DATA(minor);
	head_ = head[minor];	// get a safe copy of head (writing pointer), it can be changed by a write operation //
						// which is being done by another process //

	if(head_ > tail[minor]) {
		read_stats[minor].read_timeout_cnt = 0;
		uint32_t available = (head_ - tail[minor]);
		if(count > available)
			count = available;

		//printk(KERN_NOTICE "Circular-File: available = %d\n", available);
		// Do a single transfer //
		failed_bytes = copy_to_user(user_buffer, rd_buffer + tail[minor], count);
		if(0 < failed_bytes) {
			printk(KERN_ERR "%s: MINOR(%u), copy_to_user(%d/%d bytes) FAILED\n", \
				CIRCULAR_FILE_DRIVER_NAME, minor, count - failed_bytes, count);
			goto READ_FAILED;
		}
	} else if(tail[minor] > head_) {
		read_stats[minor].read_timeout_cnt = 0;
		uint32_t part1, part2;
		uint32_t available = head_ + (BUFSIZE - tail[minor]);
		if(count > available)
			count = available;
		// transfer first part from tail to end_of_buffer //
		if(count > (BUFSIZE - tail[minor])) {
			part1 = (BUFSIZE - tail[minor]);
			part2 = count - part1;
		} else {
			part1 = count;
			part2 = 0;
		}
		// Do part1 transfer //
		failed_bytes = copy_to_user(user_buffer, rd_buffer + tail[minor], part1);
		if(0 < failed_bytes) {
			printk(KERN_ERR "%s: MINOR(%u), copy_to_user(%u/%u bytes) FAILED\n", \
				CIRCULAR_FILE_DRIVER_NAME, minor, part1-failed_bytes, part1);
			goto READ_FAILED;
		}
		// Do part2 transfer //
		if(0 != part2) {
			failed_bytes = copy_to_user(user_buffer + part1, rd_buffer, part2);
			if(0 < failed_bytes) {
				printk(KERN_ERR "%s: MINOR(%u), copy_to_user(%u/%u bytes) FAILED\n", \
					CIRCULAR_FILE_DRIVER_NAME, minor, part2-failed_bytes, part2);
				goto READ_FAILED;
			}
		}
	} else {
		int ret;
		if (filep->f_flags & O_NONBLOCK)
			return -EAGAIN;
		ret = wait_event_interruptible_timeout((*queue_heads[minor]), head[minor] != tail[minor], schedule_timeout(HZ));
		if(ret > 0) {
			goto RETRY_READ;	// condition TRUE, after/before timeout //
		} else if(0 == ret) {
			// condition FALSE, after timeout //
			read_stats[minor].read_timeout_cnt++;

			read_ready[minor]=0;
			start_timer(minor, RESTARTED_OPEN_INITIAL_DELAY_S * 1000U);

			printk(KERN_INFO "%s: MINOR(%u), read NORMAL timeout(cnt:%u)\n", CIRCULAR_FILE_DRIVER_NAME, minor, read_stats[minor].read_timeout_cnt);
			return 0;
		} else if (ret == (-ERESTARTSYS)) {
			printk(KERN_INFO "%s: MINOR(%u), read interrupted by signal\n", CIRCULAR_FILE_DRIVER_NAME, minor);
			return ret;
		}
		else {
			printk(KERN_INFO "%s: MINOR(%u), unexpected read_driver return(%d)\n", CIRCULAR_FILE_DRIVER_NAME, minor, ret);
			return 0;
		}
	}

	tail[minor] += count;
	if(tail[minor] >= BUFSIZE)
		tail[minor] -= BUFSIZE;

	//printk(KERN_INFO "Circular-File: new tail(%u)\n", tail);

	return count;

READ_FAILED:
	return -EINVAL;
}

static ssize_t dev_file_write(struct file *filep, const char __user *user_buffer, size_t count, loff_t *offset)
{
	unsigned long failed_bytes;
	uint8_t *wr_buffer;
	uint32_t head_prev;
	uint8_t wrbuf[1500];

	//printk( KERN_NOTICE "Circular-File: write %zu bytes.\n", count);

	uint32_t minor = iminor(filep->f_path.dentry->d_inode);
	if(!wr_started[minor]) {
		wr_started[minor] = 1;
		start_timer(minor, 0xFF);
	}

	// find which buffer to write by using filep //
	wr_buffer = NULL;
	#if(0)
		uint8_t indx;
		for(indx=0; indx<MAX_CIRCULAR_DEVICES_COUNT; indx++) {
			if(fileptrs[indx][1] == filep) {
				wr_buffer = buffer[indx];
				minor = indx;
				break;
			}
		}
		if(NULL != wr_buffer) {
			printk(KERN_ERR, "%s: wr_buffer NOT FOUND\n", CIRCULAR_FILE_DRIVER_NAME);
			return -1;
		}
	#else
		wr_buffer = global_buffer[minor];
	#endif
	//printk(KERN_INFO "%s: WRITING FILEP(%p) MINOR(%u)\n", CIRCULAR_FILE_DRIVER_NAME, filep, minor);

	if(count > BUFSIZE) {
		printk(KERN_ERR "%s: writing overflow", CIRCULAR_FILE_DRIVER_NAME);
		return -EINVAL;
	}
	if(count > sizeof(wrbuf)) {
		printk(KERN_INFO "%s: writing truncating", CIRCULAR_FILE_DRIVER_NAME);
		count = sizeof(wrbuf);
	}
	head_prev = head[minor];

	// do write transfer from userbuffer to temporary buffer //
	failed_bytes = copy_from_user(wrbuf, user_buffer, count);
	if(!failed_bytes) {
		// Nothing todo, just be happy //
	} else {
		printk(KERN_ERR "%s: MINOR(%u), copy_from_user(%u/%u bytes) FAILED\n", \
				CIRCULAR_FILE_DRIVER_NAME, minor, failed_bytes, count);
		count -= failed_bytes;
	}

	if((head[minor] + count) >= BUFSIZE) {
		uint32_t some_count = BUFSIZE - head[minor];
		memcpy(wr_buffer+head[minor], wrbuf, some_count);	// copy some bytes to END of BUFFER //
		memcpy(wr_buffer, wrbuf+some_count, count - some_count);	// copy rest of bytes from start of buffer //
		head[minor] = count - some_count;
	} else {
		memcpy(wr_buffer + head[minor], wrbuf, count);
		head[minor] += count;
	}

	wake_up_interruptible(queue_heads[minor]);	// send address as parameter //

	if((NULL != fileptrs[minor][0]) && (head_prev < tail[minor]) && (head[minor] > tail[minor])) {
		read_stats[minor].read_slow_cnt++;
		printk(KERN_INFO "%s: MINOR(%u), READ is slower than WRITE detected(cnt:%u)\n", \
			CIRCULAR_FILE_DRIVER_NAME, minor, read_stats[minor].read_slow_cnt);
		tail[minor] = head[minor] + READING_SKIP_STEP_SIZE;
		if(tail[minor] >= BUFSIZE)
			tail[minor] -= BUFSIZE;
	}

	return count;
}

int register_device(void)
{
      int result = 0;
      volatile uint8_t indx;

      printk( KERN_NOTICE "%s: register_device(version: %u) is called\n", CIRCULAR_FILE_DRIVER_NAME, CIRCULAR_FILE_VERSION);

      result = register_chrdev( 0, device_name, &simple_driver_fops );
      if( result < 0 ) {
         printk( KERN_WARNING "%s:  can\'t register character device with errorcode = %i\n", CIRCULAR_FILE_DRIVER_NAME, result);
         return result;
      }

      device_major_number = result;
      printk( KERN_NOTICE "%s: registered character device with major number = %i and minor numbers 0...255\n", CIRCULAR_FILE_DRIVER_NAME, device_major_number );

      for(indx=0 ; indx<MAX_CIRCULAR_DEVICES_COUNT ; indx++) {
		fileptrs[indx][0] = fileptrs[indx][1] = NULL;
		tail[indx] = head[indx] = 0;	// clear buffer //
		initial_delay_s[indx] = MAX_INITIAL_DELAY_S;

		// my_timer.function, my_timer.data
		timer_setup( &(cw_timers[indx]), cb_tmr_callback2, indx );
      }
      queue_heads[0] = &wq0;
      queue_heads[1] = &wq1;
      queue_heads[2] = &wq2;
      queue_heads[3] = &wq3;
      queue_heads[4] = &wq4;
      queue_heads[5] = &wq5;
      queue_heads[6] = &wq6;
      queue_heads[7] = &wq7;
      queue_heads[8] = &wq8;
      queue_heads[9] = &wq9;

	  printk(KERN_INFO "%s: active each buffer size %u KB\n", CIRCULAR_FILE_DRIVER_NAME, BUFSIZE/1024);

      return 0;
}

void unregister_device(void)
{
	volatile uint8_t indx;

	printk( KERN_NOTICE "%s: unregister_device() is called\n", CIRCULAR_FILE_DRIVER_NAME);
	if(device_major_number != 0) {
		unregister_chrdev(device_major_number, device_name);
	}
	for(indx=0 ; indx<MAX_CIRCULAR_DEVICES_COUNT ; indx++) {
		int ret = del_timer( &(cw_timers[indx]) );
		if (ret)
			printk(KERN_ERR "%s: timer(%u) is still in use...\n", CIRCULAR_FILE_DRIVER_NAME, indx);
	}

}



