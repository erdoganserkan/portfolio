#include <log.h>
#include <qos.h>
#include <string.h>

#ifdef QOS_ENABLED

/**************************************************************************
 name	: qos_ts_to_audio
 purpose	: It deletes adaptation field and create audio packet
 input	: stream	-> stream
 ts		-> transport stream
 output	: none
 *************************************************************************/
void qos_ts_to_audio(ts_stream_t *stream, uint8_t *ts)
{
    uint8_t i, j, ffcount;
    uint8_t *buf = stream->buf;
    uint32_t pid;
    uint8_t af_exist;
    uint8_t afl; /* Adaptation field length */
    uint32_t len, last_len;

    if (stream->ready)
        return;

    pid = (((uint32_t)(ts[1]) << 8) | (uint32_t)ts[2]) & 0x1FFF;

    if (pid != stream->pida0 && pid != stream->pida1)
        return;

    af_exist = (ts[3] & 0x20) >> 5;

    /* Get adaptation field length*/

    if (af_exist) {
        if (ts[4] == 0)
            goto last;

        afl = ts[4] - 1;

        last_len = stream->len + TS_PACKET_LENGTH - afl;

        if (last_len > CHUNK_DATA_LEN) {
            stream->ready = 1;
            return;
        }

        ffcount = 0;
        for (i = 0; i < afl; i++)
            if (ts[AUDIO_PACKET_START_LENGTH + i] == 0xFF)
                ffcount++;

        memcpy((uint8_t *)buf + stream->len, (uint8_t *)ts,
            AUDIO_PACKET_START_LENGTH);
        stream->len += AUDIO_PACKET_START_LENGTH;
        memcpy(buf + stream->len, ts + AUDIO_PACKET_START_LENGTH + afl,
            TS_PACKET_LENGTH - (AUDIO_PACKET_START_LENGTH + afl));
        stream->len += TS_PACKET_LENGTH - (AUDIO_PACKET_START_LENGTH + afl);
    }
    else {
        last:
        last_len = stream->len + TS_PACKET_LENGTH;

        if (last_len > CHUNK_DATA_LEN) {
            stream->ready = 1;
            return;
        }

        memcpy(buf + stream->len, ts, TS_PACKET_LENGTH);
        stream->len += TS_PACKET_LENGTH;
    }
}

/**************************************************************************
 name	: qos_ts_to_audio2
 purpose	: It deletes consecutive 0x00 or 0xFF bytes to compress audio data
 input	: stream	-> stream
 ts		-> transport stream
 output	: none
 *************************************************************************/
void qos_ts_to_audio2(ts_stream_t *stream, uint8_t *ts)
{
    uint8_t i, j, ffcount;
    uint8_t *buf = stream->buf;
    uint8_t *ptr;
    uint32_t pid;
    uint8_t af_exist;
    uint8_t afl; /* Adaptation field length */
    uint32_t len, last_len, idx;
    audio_compress_t comp;
    uint8_t data, start, end;

    if (stream->ready)
        return;

    memset((uint8_t *)&comp, 0, sizeof(audio_compress_t));

    start = 0;
    idx = 0;
    len = MIN_COMPRESS_LENGTH;
    for (i = 0; i < TS_PACKET_LENGTH; i++) {
        if ((ts[i] == 0xFF || ts[i] == 0x00) && !start) {
            start = i;
            data = ts[i];
        }

        if (((ts[i] != data) || (i == TS_PACKET_LENGTH - 1)) && start) {
            if ((i == TS_PACKET_LENGTH - 1) && (ts[i] == data))
                end = TS_PACKET_LENGTH;
            else
                end = i;
            if ((end > start) &&
                ((end - start) > MIN_COMPRESS_LENGTH)) {
                len = end - start;
                comp.index[idx].start = start;
                comp.index[idx].end = end;
                comp.index[idx].data = data;
                idx++;
                if (idx == MAX_COMP_COUNT)
                    break;
            }
            start = 0;
            end = 0;

            if ((ts[i] == 0xFF || ts[i] == 0x00) && !start) {
                start = i;
                data = ts[i];
            }
        }
    }

    start = 0;
    ptr = comp.buf;
    for (i = 0; i < MAX_COMP_COUNT; i++) {
        if (comp.index[i].start) {
            memcpy(ptr, ts + start, comp.index[i].start - start);
            comp.len += comp.index[i].start - start;
            ptr += comp.index[i].start - start;
            start = comp.index[i].end;
        }
        else {
            memcpy(ptr, ts + start, TS_PACKET_LENGTH - start);
            comp.len += TS_PACKET_LENGTH - start;
            break;
        }

        if (i == (MAX_COMP_COUNT - 1)) {
            memcpy(ptr, ts + start, TS_PACKET_LENGTH - start);
            comp.len += TS_PACKET_LENGTH - start;
            break;
        }
    }

    last_len = comp.len + sizeof(comp.index) + sizeof(comp.len);

    if ((stream->len + last_len) > CHUNK_DATA_LEN) {
        stream->ready = 1;
        return;
    }

    memcpy(buf + stream->len, (uint8_t *)&comp, last_len);
    stream->len += last_len;
    stream->cnt++;

}

/**************************************************************************
 name	: qos_decompress_audio
 purpose	: It decompresses incoming audio packet.
 It doesn't check audio buffer size
 input	: rch	-> audio chunk
 audio	-> decompressed data will be written there
 output	:
 *************************************************************************/
uint32_t qos_decompress_audio(chunk_t *rch, uint8_t *audio)
{
    uint8_t *buf = rch->data;
    uint32_t len, audio_len, i, j;
    uint8_t af_exist;
    uint8_t afl = 0; /* Adaptation field length */

    audio_len = 0;
    len = rch->len;
    i = 0;
    while (i < len) {
        if (buf[i] == SYNC_BYTE) {
            af_exist = (buf[i + 3] & 0x20) >> 5;
            if (af_exist) {
                if (buf[i + 4] == 0)
                    goto last;

                afl = buf[i + 4] - 1; /* -1 means 0x00 value start of the 0xFFs*/
                memcpy(audio + audio_len, buf + i, AUDIO_PACKET_START_LENGTH);
                audio_len += AUDIO_PACKET_START_LENGTH;
                for (j = 0; j < afl; j++) /* -1 is 0x00*/
                    audio[audio_len + j] = 0xFF;

                audio_len += afl;
                memcpy(audio + audio_len, buf + i + AUDIO_PACKET_START_LENGTH,
                    TS_PACKET_LENGTH - (AUDIO_PACKET_START_LENGTH + afl));
                i += TS_PACKET_LENGTH - afl;
                audio_len += TS_PACKET_LENGTH
                    - (AUDIO_PACKET_START_LENGTH + afl);
            }
            else {
                last:
                memcpy(audio + audio_len, buf + i, TS_PACKET_LENGTH);
                i += TS_PACKET_LENGTH;
                audio_len += TS_PACKET_LENGTH;
            }
        }
        else {
#if 0
            FILE *fp = fopen("/home/user/conf/audio_error", "wb");
            fwrite(rch->data, 1, rch->len, fp);
            fclose(fp);
#endif
            ERRL(logi, "Audio decompressed error\n");
            return 0;
        }
    }

    return audio_len;
}

/**************************************************************************
 name	: qos_decompress_audio2
 purpose	: It decompresses incoming audio packet.
 It doesn't check audio buffer size
 input	: rch	-> audio chunk
 audio	-> decompressed data will be written there
 output	:
 *************************************************************************/
uint32_t qos_decompress_audio2(chunk_t *rch, uint8_t *audio)
{
    uint32_t audio_len = 0;
    uint32_t len = 0;
    uint8_t i, start;
    uint8_t pos = 0;
    uint32_t rch_len = 0;
    uint8_t cnt = 0;
    audio_compress_t *comp;

    while (rch_len < rch->len) {
        cnt++;
        comp = (audio_compress_t *)&rch->data[rch_len];
        pos = 0;
        start = 0;
        for (i = 0; i < MAX_COMP_COUNT; i++) {
            if (comp->index[i].start) {
                memcpy(audio + audio_len, comp->buf + pos,
                    comp->index[i].start - start);
                pos += comp->index[i].start - start;
                audio_len += comp->index[i].start - start;
                memset(audio + audio_len, comp->index[i].data,
                    comp->index[i].end - comp->index[i].start);
                audio_len += comp->index[i].end - comp->index[i].start;
                start = comp->index[i].end;
            }
            else {
                memcpy(audio + audio_len, comp->buf + pos,
                    TS_PACKET_LENGTH - start);
                audio_len += TS_PACKET_LENGTH - start;
                break;
            }

            if (i == (MAX_COMP_COUNT - 1)) {
                memcpy(audio + audio_len, comp->buf + pos,
                    TS_PACKET_LENGTH - start);
                audio_len += TS_PACKET_LENGTH - start;
                break;
            }
        }
        rch_len += comp->len + sizeof(comp->index) + sizeof(comp->len);
    }

    return audio_len;
}

/**************************************************************************
 name	: __add_ts_to_video
 purpose	: It deletes adaptation field and create audio packet
 input	: stream	-> stream
 ts		-> transport stream
 output	: none
 *************************************************************************/
void qos_ts_to_video(ts_stream_t *stream, uint8_t *ts)
{
    uint8_t *buf = stream->buf;
    uint32_t last_len;
    uint32_t pid;

    pid = (((uint32_t)(ts[1]) << 8) | (uint32_t)ts[2]) & 0x1FFF;

    if (stream->ready)
        return;

    switch (pid)
    {
    case 0x00:
        stream->pid0_timeout = get_time();
        break;
    case 0x50:
        stream->pid50_timeout = get_time();
        break;
    case 0x90:
        stream->pid90_timeout = get_time();
        break;
#if defined(TEST)
    case VIDEO_PID:
        break;
#endif
    default:
        break;
    }

    last_len = stream->len + TS_PACKET_LENGTH;

    if (last_len > CHUNK_DATA_LEN) {
        stream->ready = 1;
        return;
    }

    memcpy((uint8_t *)buf + stream->len, (uint8_t *)ts, TS_PACKET_LENGTH);
    stream->len += TS_PACKET_LENGTH;
}

/**************************************************************************
 name	: get_priority_timeout
 purpose	: Return timeout value of related priority
 input	: stream	-> stream
 ts		-> transport stream
 output	: none
 *************************************************************************/
uint32_t get_priority_timeout(general_config_t *conf, uint8_t prio)
{
    uint32_t timeout = 0;
#if 0
    switch(prio)
    {
        case QOS_AUDIO_PRIORITY : timeout = conf->video_delay-2000; break;
        case QOS_VIDEO_PRIORITY : timeout = conf->video_delay-2000; break;
        case QOS_DATA_LOW_PRIORITY : timeout = conf->data_low_prio_timeout; break;
        case QOS_DATA_HIGH_PRIORITY : timeout = conf->data_high_prio_timeout; break;
    }
#endif
    return timeout;
}

#endif
