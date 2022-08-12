#ifndef CHECK_H_
#define CHECK_H_

#include <common.h>
#include <m_list.h>

typedef struct ts_packet_s
{
    uint32_t pid;
    uint32_t cont;
    uint32_t payload;
    uint8_t data[TS_PACKET_LENGTH];
} ts_packet_t;

/***************************************
 * Function prototypes.
 ***************************************/

m_list_t *init_ts_pid_list(void);
void destroy_ts_pid_list(m_list_t *ts_list);
inline uint32_t get_ts_pid(uint8_t *ts);
inline uint32_t get_ts_continuity(uint8_t *ts);
inline uint32_t get_ts_payload(uint8_t *ts);
ts_packet_t *get_last_ts_from_list(uint32_t pid, m_list_t *ts_list);
uint8_t check_ts_pid_continuity(uint32_t pid, uint32_t cont,
    uint32_t payload, uint8_t *data,
    m_list_t *ts_list);
void list_all_ts_pid_data(m_list_t *ts_list);

#endif
