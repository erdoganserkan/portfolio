#ifndef DEVICE_FILE_H_
#define DEVICE_FILE_H_

#ifdef __KERNEL__
	#include <linux/compiler.h> /* __must_check */
	#include <linux/types.h>
#endif

#define DEFAULT_BUFFER_SIZE_MB			4
#define DEFAULT_STREAM_INITIAL_DELAY_S		3
#define RESTARTED_OPEN_INITIAL_DELAY_S		2
#define MAX_INITIAL_DELAY_S				8
#define DEFAULT_DECKLINK_FFMPEG_SKIP_MS	((DEFAULT_STREAM_INITIAL_DELAY_S + 1)*1000UL)		/* If set to "0" ffmpeg is buffering dat before play, o.w. it is playig immediately after reading */
													/* If ffmpeg buffering is DISABLED (set ot NON-ZERO) increase ccmtp.ttl or "Buffer Depth" from LCD-GUI or Server-GUI */

#define MAX_INITIAL_WAIT_S				((MAX_INITIAL_DELAY_S*3)/2)
#define DEFAULT_INITIAL_DELAY_S			MAX_INITIAL_DELAY_S	/* GUI delay is IGNORED, temporaryly */
#define MAX_CIRCULAR_DEVICES_COUNT		10
#define CIRCULAR_FILE_DRIVER_NAME		"Circular-File"	// shared with common/server_common.h //

#define IOCTL_CB_MAGIC_NUM	'k'
#define IOCTL_CB_GET_DELAY 		_IOR(IOCTL_CB_MAGIC_NUM, 1, char *)
#define IOCTL_CB_SET_DELAY 		_IOW(IOCTL_CB_MAGIC_NUM, 2, char *)
#define IOCTL_CB_GET_READ_ERRS 	_IOR(IOCTL_CB_MAGIC_NUM, 3, char *)

#define CIRCULAR_FILE_VERSION	100

#pragma pack(1)
typedef struct {
	uint16_t read_timeout_cnt;
	uint16_t read_slow_cnt;
} read_stat_t;
#pragma pack()

#ifdef __KERNEL__
	__must_check int register_device(void); /* 0 if Ok*/
	void unregister_device(void);
#endif
#endif //DEVICE_FILE_H_
