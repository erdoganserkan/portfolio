#ifndef MPEGTS_H_
#define MPEGTS_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define GET_PID_MASK	0x1FFF

#define SET_TS_PID(ts_buf, pid) { \
	uint8_t *u8p = (uint8_t *)&pid; 		\
	u8p[0] = ts_buf[2];					\
	u8p[1] = ts_buf[1];					\
}
#define PID_RES_BITS(ts_buf) (ts_buf[1] & (~0x1F))
#define MIN_RLE_BENEFIT_BYTE_CNT	0

typedef enum {
	TS_PKT_VIDEO 	= 0,	// 0x1FF0
	TS_PKT_AUDIO	= 1,	// 0x1FF1
	TS_PKT_ZERO		= 2,	// 0x0000
	TS_PKT_PMT		= 3,	// 0x0100
	TS_PKT_PCR		= 4,	// 0x0101
	TS_PKT_SIT		= 5,	// 0x001F
	TS_PKT_NULLTS	= 6,	// 0x1FFF
	TS_PKT_UNKNOWN	= 7,

	TS_PKT_COUNT
} eTSTYPEs;

#define COMP_RLE_ACTIVE_BIT		4	// It is valid for VIDEO, AUDIO and UNKNOWN pid ts packets //
										// compressing function will set this bit if RLE applied to packet //
#define RLE_MIN_REPEAT_COUNT	4	// DONT CHANGE THIS, face_cost calculation fails if incremented //
#define RLE_ENC_SPECIAL_BYTE	0xAA	// If this bytes exist in a RLE active packet, special action is required //
#define UNIVERSAL_TS_SYNC_BYTE	0x47	/* Never change this */

#pragma pack(1)

#define PID_0000_CHANGE_START_INDX	(3)		/* This byte is changing, others are same during all of stream */
#define PID_0000_CHANGE_BYTES_COUNT	(1)
typedef struct pid_0000_comp_hdr_s {
	uint8_t pkt_t;	// low_nibble: one of eTSTYPEs | high_nible: high three bits of real pid value //
	uint8_t change[PID_0000_CHANGE_BYTES_COUNT];
} pid_0000_comp_t;

#define PID_0100_CHANGE_START_INDX	(3)		/* This byte is changing, others are same during all of stream */
#define PID_0100_CHANGE_BYTES_COUNT	(1)
typedef struct pid_0100_comp_hdr_s {
	uint8_t pkt_t;	// low_nibble: one of eTSTYPEs | high_nible: high three bits of real pid value //
	uint8_t change[PID_0100_CHANGE_BYTES_COUNT];
} pid_0100_comp_t;

#define PID_0101_CHANGE_START_INDX	(6)		/* This bytes[6:11] are changing, others are same during all of stream */
#define PID_0101_CHANGE_BYTES_COUNT	(6)
typedef struct pid_0101_comp_hdr_s {
	uint8_t pkt_t;	// low_nibble: one of eTSTYPEs | high_nible: high three bits of real pid value //
	uint8_t change[PID_0101_CHANGE_BYTES_COUNT];
} pid_0101_comp_t;

#define PID_001F_CHANGE_START_INDX	(3)		/* This byte is changing, others are same during all of stream */
#define PID_001F_CHANGE_BYTES_COUNT	(1)
typedef struct pid_001F_comp_hdr_s {
	uint8_t pkt_t;	// low_nibble: one of eTSTYPEs | high_nible: high three bits of real pid //
	uint8_t change[PID_001F_CHANGE_BYTES_COUNT];
} pid_001F_comp_t;

typedef struct ts_rle_s {
	uint8_t pid_t;	// low_nibble: one of eTSTYPEs | high_nible: high three bits of real pid value and bit:5 is rle flag //
} ts_rle_t;

#pragma pack()

extern uint8_t compress_ts_pkt(uint8_t *ts_comp, uint8_t const *ts_raw) __attribute__((optimize("-O3")));
extern uint8_t decompress_ts_pkt(uint8_t const *ts_comp, uint8_t *ts_raw) __attribute__((optimize("-O3")));;
extern uint8_t encode_rle(uint8_t *dest_p, uint8_t const *src_p, uint8_t pkt_t) __attribute__((optimize("-O3")));
extern uint8_t decode_rle(uint8_t *ts_dec, uint8_t const *ts_enc) __attribute__((optimize("-O3")));

#endif /* MPEGTS_H_ */
