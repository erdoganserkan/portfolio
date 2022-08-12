#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

#include "log.h"

#include "mpegts.h"

typedef struct {
	uint8_t start_indx;
	uint8_t len;
	uint8_t byte;
} rle_ready_block_t;

// ZERO PID //
static uint8_t const pid_0000_frame[] = {
	0x16, 0x00, 0x00, 0xB0, 0x11, 0x00, 0x00, 0xC1, 0x00, 0x00, 0x00, 0x01, 0xE1,
	0x00, 0x00, 0x00, 0xE0, 0x1F, 0xE9, 0x62, 0xE7, 0x63
};

// PMT PID //
static uint8_t const pid_0100_frame[] = {
	0x16, 0x00, 0x02, 0xB0, 0x17, 0x00, 0x01, 0xC1, 0x00, 0x00, 0xE1, 0x01, 0xF0, 0x00,
	0x1B, 0xFF, 0xF0, 0xF0, 0x00, 0x11, 0xFF, 0xF1, 0xF0, 0x00, 0x43, 0x08, 0x9B, 0xE1
};

// PCR PID //
static uint8_t const pid_0101_frame[] = {
	0x20, 0xB7, 0x10, 0x00, 0x6D, 0x68, 0x17, 0x7E, 0x65
};

// SIT PID //
static uint8_t const pid_001F_frame[] = {
	0x1F, 0x00, 0x7F, 0xF0, 0x0F, 0xFF, 0xFF, 0xC1, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x01, 0xC0,
	0x00, 0x39, 0x6C, 0x2B, 0x0E
};

static rle_ready_block_t rle_blocks[10];

// RLE will be applied --ONLY-- to VIDEO, AUDIO and UNKNOWN PID type ts packets //
// SYNC and PID bytes are not filled with this function, only data section of decoded frame will be updated //
// return value : compressed_frame size
__attribute__((optimize("-O3"))) uint8_t decode_rle(uint8_t *ts_dec, uint8_t const *ts_enc)
{
	uint8_t eindx, dindx;
	uint8_t pkt_t = (ts_enc[0] & 0x0F);
	uint8_t pkt_res_bits = ts_enc[0] & (~0x1F);

	switch(pkt_t) {
		case TS_PKT_VIDEO:
		case TS_PKT_UNKNOWN:
		case TS_PKT_AUDIO: {
			// RLE coding has effect on bytes from 3 (first byte after pid bytes) //
			for(dindx=3, eindx=(TS_PKT_UNKNOWN == pkt_t)?3:1; dindx<FMB_TS_PACKET_SIZE_OUT ; ) {
				if(RLE_ENC_SPECIAL_BYTE == ts_enc[eindx]) {
					uint8_t repeat_byte = ts_enc[eindx+1];
					uint8_t repeat_cnt = ts_enc[eindx+2];
					memset(ts_dec+dindx, repeat_byte, repeat_cnt);
					eindx += 3;
					dindx += repeat_cnt;
				} else {
					ts_dec[dindx++] = ts_enc[eindx++];
				}
			}
			return eindx;
		}
		break;
		default:
			fprintf(stderr, "UNEXPECTED TS_PKT(%u)\n", pkt_t);
			return 0;
	}
}

// RLE will be applied --ONLY-- to VIDEO, AUDIO and UNKNOWN PID type ts packets //
// return total size of encoded ts packet
__attribute__((optimize("-O3"))) uint8_t encode_rle(uint8_t *dest_p, uint8_t const *src_p, uint8_t pkt_t)
{
	uint8_t sindx, dindx, rindx, rle_block_cnt;
	int16_t fake_cost, rle_benefit;
	uint8_t repeat_cnt = 1;
	uint8_t byte = src_p[3];

	// search for rle encode suitable blocks, start from the byte after real pid //
	// memset(rle_blocks, 0, sizeof(rle_blocks));
	for(sindx=4, rle_block_cnt=0, fake_cost=0, rle_benefit=0 ; sindx<FMB_TS_PACKET_SIZE_OUT ;sindx++) {
		if(byte != src_p[sindx]) {
			if((RLE_MIN_REPEAT_COUNT <= repeat_cnt) || (RLE_ENC_SPECIAL_BYTE == byte)) {
				if(RLE_ENC_SPECIAL_BYTE == byte) {
					fake_cost = (fake_cost + 3) - repeat_cnt;
				} else
					rle_benefit = (rle_benefit - 3) + repeat_cnt;
				rle_blocks[rle_block_cnt].byte = byte;
				rle_blocks[rle_block_cnt].len = repeat_cnt;
				rle_blocks[rle_block_cnt].start_indx = sindx-repeat_cnt;
				if(ARRAY_MEMBER_COUNT(rle_blocks) == (++rle_block_cnt)) {	// If rle_block maximum count is reached CANCEL RLE //
					rle_block_cnt = 0;
					goto IS_SUITABLE_FOR_RLE;
				}
			}
			byte = src_p[sindx];
			repeat_cnt = 1;
		} else
			repeat_cnt++;
	}
	if(RLE_MIN_REPEAT_COUNT <= repeat_cnt) {	// process repeated bytes to the end of TS_PACKET //
		rle_blocks[rle_block_cnt].byte = byte;
		rle_blocks[rle_block_cnt].len = repeat_cnt;
		rle_blocks[rle_block_cnt].start_indx = sindx-repeat_cnt;
	}
	//fprintf(stdout, "face_cost(%u), rle_benefit(%u)\n", fake_cost, rle_benefit);
	if(rle_benefit < (fake_cost + MIN_RLE_BENEFIT_BYTE_CNT)) {	// If cost of RLE is LOWER than benefit, cancel RLE //
		rle_block_cnt = 0;
		goto IS_SUITABLE_FOR_RLE;
	}
IS_SUITABLE_FOR_RLE:
	if(0 == rle_block_cnt) {
		// RLE is not applicaple for this TS packet //
		if(TS_PKT_UNKNOWN == pkt_t) {
			dest_p[0] = (pkt_t & 0x0F);								// change sync byte //
			memcpy(dest_p+1, src_p+1, FMB_TS_PACKET_SIZE_OUT-1);	// send pid bytes too //
			return FMB_TS_PACKET_SIZE_OUT;
		} else {
			dest_p[0] = (pkt_t & 0x0F) | (PID_RES_BITS(src_p));	// change sync byte //
			memcpy(dest_p+1, src_p+3, FMB_TS_PACKET_SIZE_OUT-3);	// dont send pid bytes, start from data //
			return (FMB_TS_PACKET_SIZE_OUT-2);
		}
	}  else {
		// RLE can be applied to this TS packet //
		if(TS_PKT_UNKNOWN == pkt_t) {
			dest_p[0] = (pkt_t & 0x0F) | (0x1<<COMP_RLE_ACTIVE_BIT);	// change sync byte with RLE Flag //
			*((uint16_t *)(dest_p+1)) = *((uint16_t *)(src_p+1));		// send unknown pid value without any change //
		} else {
			dest_p[0] = (pkt_t & 0x0F) | (0x1<<COMP_RLE_ACTIVE_BIT) | (PID_RES_BITS(src_p));	// change sync byte with RLE flag //
		}
	}

	// Fill destination buffer with taking rle_blocks in to consideration //
	for(dindx=(TS_PKT_UNKNOWN == pkt_t)?3:1 , rindx=0, sindx=3 ; sindx<FMB_TS_PACKET_SIZE_OUT ;) {
		if((rindx < rle_block_cnt) && (sindx == rle_blocks[rindx].start_indx)) {
			dest_p[dindx++] = RLE_ENC_SPECIAL_BYTE;	// special byte //
			dest_p[dindx++] = rle_blocks[rindx].byte;	// repeated byte //
			dest_p[dindx++] = rle_blocks[rindx].len;	// repeat count //

			sindx += rle_blocks[rindx].len;
			rindx++;
		} else {
			if(RLE_ENC_SPECIAL_BYTE == src_p[sindx]) {
				dest_p[dindx++] = RLE_ENC_SPECIAL_BYTE;	// RLE start indicator //
				dest_p[dindx++] = RLE_ENC_SPECIAL_BYTE;	// repeated byte //
				dest_p[dindx++] = 1;	// repeat count //
				sindx++;
			} else
				dest_p[dindx++] = src_p[sindx++];
		}
	}

	return dindx;	// Destination packet index is the size of total compression bytes //
}

// ts_comp and ts_raw must be valid pointers //
__attribute__((optimize("-O3"))) uint8_t compress_ts_pkt(uint8_t *ts_comp, uint8_t const *ts_raw) {
	uint16_t pid, pid_masked;
	SET_TS_PID(ts_raw, pid);
	pid_masked = pid & GET_PID_MASK;

	//fprintf(stdout, "pid:0x%04X, pid_masked:0x%04X\n", pid, pid_masked);

	// Analyse ts packet type //
		// If it is video(0x1FF0) or audio(0x1FF1) packet do RLE if possible //
		// If it is a KNOWN TYPE TS (PMT:0x0100, PCR:0x0101, SIT:0x001F, ZERO:0x0000, NULLTS:0x1FFF)
			// do fixed optimizations
		// If it is unknown type log it and do RLE if possible

	switch(pid_masked) {
		case TS_PID_VIDEO:
			return encode_rle(ts_comp, ts_raw, TS_PKT_VIDEO);
		case TS_PID_AUDIO:
			return encode_rle(ts_comp, ts_raw, TS_PKT_AUDIO);
		case TS_PID_ZERO: {
			pid_0000_comp_t *pid_0000_pkt = (pid_0000_comp_t *)ts_comp;
			pid_0000_pkt->pkt_t = ((TS_PKT_ZERO & 0x0F) | PID_RES_BITS(ts_raw));
			pid_0000_pkt->change[0] = ts_raw[PID_0000_CHANGE_START_INDX];
			return sizeof(pid_0000_comp_t);
		}
		break;
		case TS_PID_PMT:{
			pid_0100_comp_t *pid_0100_pkt = (pid_0100_comp_t *)ts_comp;
			pid_0100_pkt->pkt_t = ((TS_PKT_PMT & 0x0F) | PID_RES_BITS(ts_raw));
			pid_0100_pkt->change[0] = ts_raw[PID_0100_CHANGE_START_INDX];
			return sizeof(pid_0100_comp_t);
		}
		break;
		case TS_PID_PCR:{
			uint8_t indx;
			pid_0101_comp_t *pid_0101_pkt = (pid_0101_comp_t *)ts_comp;
			pid_0101_pkt->pkt_t = ((TS_PKT_PCR & 0x0F) | PID_RES_BITS(ts_raw));
			for(indx=0 ; indx<PID_0101_CHANGE_BYTES_COUNT ; indx++)
				pid_0101_pkt->change[indx] = ts_raw[PID_0101_CHANGE_START_INDX + indx];
			return sizeof(pid_0101_comp_t);
		}
		break;
		case TS_PID_SIT:{
			pid_001F_comp_t *pid_001F_pkt = (pid_001F_comp_t *)ts_comp;
			pid_001F_pkt->pkt_t = ((TS_PKT_SIT & 0x0F) | PID_RES_BITS(ts_raw));
			pid_001F_pkt->change[0] = ts_raw[PID_001F_CHANGE_START_INDX];
			return sizeof(pid_001F_comp_t);
		}
		break;
		case TS_PID_NULLTS:
			return 0;	// Don't use this TS packet, completely ignore it //
		default:
			return encode_rle(ts_comp, ts_raw, TS_PKT_UNKNOWN);
	}

	return 0;
}

// ts_comp and ts_raw must be valid pointers //
// return value is compressed_pkt_size //
__attribute__((optimize("-O3"))) uint8_t decompress_ts_pkt(uint8_t const *ts_comp, uint8_t *ts_raw)
{
	uint8_t pkt_t = (ts_comp[0] & 0x0F);
	uint8_t pkt_res_bits = ts_comp[0] & (~0x1F);
	uint8_t rle_exist = ts_comp[0] & (0x1<<COMP_RLE_ACTIVE_BIT);
	uint8_t compressed_pkt_size;

	ts_raw[0] = UNIVERSAL_TS_SYNC_BYTE;	// Universal TS SYNC BYTE //

	switch(pkt_t) {
		case TS_PKT_VIDEO:
			ts_raw[1] = ((TS_PID_VIDEO>>8) & 0x1F) | pkt_res_bits;
			ts_raw[2] = (TS_PID_VIDEO & 0xFF);
			if(rle_exist) {
				if(0 == (compressed_pkt_size = decode_rle(ts_raw, ts_comp))) {
					fprintf(stderr, "decode_rle() FAILED\n");
				}
			} else {
				memcpy(ts_raw+3, ts_comp+1, FMB_TS_PACKET_SIZE_OUT-3);
				compressed_pkt_size = FMB_TS_PACKET_SIZE_OUT - 2;
			}
			break;
		case TS_PKT_AUDIO:
			ts_raw[1] = ((TS_PID_AUDIO>>8) & 0x1F) | pkt_res_bits;
			ts_raw[2] = (TS_PID_AUDIO & 0xFF);
			if(rle_exist) {
				if(0 == (compressed_pkt_size = decode_rle(ts_raw, ts_comp))) {
					fprintf(stderr, "decode_rle() FAILED\n");
				}
			} else {
				memcpy(ts_raw+3, ts_comp+1, FMB_TS_PACKET_SIZE_OUT-3);
				compressed_pkt_size = FMB_TS_PACKET_SIZE_OUT - 2;
			}
			break;
		case TS_PKT_UNKNOWN:
			if(rle_exist) {
				*((uint16_t *)(ts_raw+1)) = *((uint16_t *)(ts_comp+1));		// send unknown pid value without any change //
				if(0 == (compressed_pkt_size = decode_rle(ts_raw, ts_comp))) {
					fprintf(stderr, "decode_rle() FAILED\n");
				}
			} else {
				memcpy(ts_raw+1, ts_comp+1, FMB_TS_PACKET_SIZE_OUT-1);
				compressed_pkt_size = FMB_TS_PACKET_SIZE_OUT;	// There is NO-COMPRESSION //
			}
			break;
		case TS_PKT_ZERO: {
			uint8_t indx;
			pid_0000_comp_t *pkt_0000_p = (pid_0000_comp_t *)ts_comp;
			ts_raw[1] = ((TS_PID_ZERO >> 8) & 0x1F) | pkt_res_bits;
			ts_raw[2] = (TS_PID_ZERO & 0xFF);
			memcpy(ts_raw+3, pid_0000_frame, sizeof(pid_0000_frame));
			for(indx=0 ; indx<PID_0000_CHANGE_BYTES_COUNT ; indx++)
				ts_raw[PID_0000_CHANGE_START_INDX + indx] = pkt_0000_p->change[indx];
			indx = 3 + sizeof(pid_0000_frame);
			memset(ts_raw+indx, 0xFF, FMB_TS_PACKET_SIZE_OUT - indx);
			compressed_pkt_size = sizeof(pid_0000_comp_t);
		}
		break;
		case TS_PKT_PMT:{
			uint8_t indx;
			pid_0100_comp_t *pkt_0100_p = (pid_0100_comp_t *)ts_comp;
			ts_raw[1] = ((TS_PID_PMT >> 8) & 0x1F) | pkt_res_bits;
			ts_raw[2] = (TS_PID_PMT & 0xFF);
			memcpy(ts_raw+3, pid_0100_frame, sizeof(pid_0100_frame));
			for(indx=0 ; indx<PID_0100_CHANGE_BYTES_COUNT ; indx++)
				ts_raw[PID_0100_CHANGE_START_INDX + indx] = pkt_0100_p->change[indx];
			indx = 3 + sizeof(pid_0100_frame);
			memset(ts_raw+indx, 0xFF, FMB_TS_PACKET_SIZE_OUT - indx);
			compressed_pkt_size = sizeof(pid_0100_comp_t);
		}
		break;
		case TS_PKT_PCR:{
			uint8_t indx;
			pid_0101_comp_t *pkt_0101_p = (pid_0101_comp_t *)ts_comp;
			ts_raw[1] = ((TS_PID_PCR >> 8) & 0x1F) | pkt_res_bits;
			ts_raw[2] = (TS_PID_PCR & 0xFF);
			memcpy(ts_raw+3, pid_0101_frame, sizeof(pid_0101_frame));
			for(indx=0 ; indx<PID_0101_CHANGE_BYTES_COUNT ; indx++)
				ts_raw[PID_0101_CHANGE_START_INDX + indx] = pkt_0101_p->change[indx];
			indx = 3 + sizeof(pid_0101_frame);
			memset(ts_raw + indx, 0xFF, FMB_TS_PACKET_SIZE_OUT - indx);
			compressed_pkt_size = sizeof(pid_0101_comp_t);
		}
		break;
 		case TS_PKT_SIT:{
			uint8_t indx;
			pid_001F_comp_t *pkt_001F_p = (pid_001F_comp_t *)ts_comp;
			ts_raw[1] = ((TS_PID_SIT >> 8) & 0x1F) | pkt_res_bits;
			ts_raw[2] = (TS_PID_SIT & 0xFF);
			memcpy(ts_raw+3, pid_001F_frame, sizeof(pid_001F_frame));
			for(indx=0 ; indx<PID_001F_CHANGE_BYTES_COUNT ; indx++)
				ts_raw[PID_001F_CHANGE_START_INDX + indx] = pkt_001F_p->change[indx];
			indx = 3 + sizeof(pid_001F_frame);
			memset(ts_raw+indx, 0xFF, FMB_TS_PACKET_SIZE_OUT - indx);
			compressed_pkt_size = sizeof(pid_001F_comp_t);
		}
		break;
		default:
			fprintf(stderr, "UNEXPECTED TS_PKT_TYPE(%u)\n", pkt_t);
			return 0;
	}

	return compressed_pkt_size;
}


