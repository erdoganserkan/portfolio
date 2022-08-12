#include <log.h>
#include <check_ts.h>
#include <common.h>
#include <string.h>

/**************************************************************************
 name	: init_ts_pid_list
 purpose	: create linked list for PIDs
 input	: none
 output	: m_list_t
 *************************************************************************/
m_list_t *init_ts_pid_list(void)
{
    /*initialize list for pid list*/
    m_list_t *list = m_list_init();
    return list;
}

/**************************************************************************
 name	: destroy_ts_pid_list
 purpose	: create linked list for PIDs
 input	: none
 output	: none
 *************************************************************************/
void destroy_ts_pid_list(m_list_t *ts_list)
{
    /*destroy list for pid list*/
    m_list_destroy(ts_list);
}

/*******************************************************************************
 * This routine gets pid of TS Packet
 ******************************************************************************/
inline uint32_t get_ts_pid(uint8_t *ts)
{
    uint32_t pid = (((uint32_t)(ts[1]) << 8) |
        (uint32_t)ts[2]) & 0x1FFF;
    return pid;
}

/*******************************************************************************
 * This routine gets continuity of TS Packet
 ******************************************************************************/
inline uint32_t get_ts_continuity(uint8_t *ts)
{
    uint32_t continuity = ts[3] & 0x0F;
    return continuity;
}

/*******************************************************************************
 * This routine gets payload of TS Packet
 ******************************************************************************/
inline uint32_t get_ts_payload(uint8_t *ts)
{
    uint32_t payload = (ts[3] >> 4) & 1;
    return payload;
}

/**************************************************************************
 name	: get_last_ts_from_list
 purpose	: get last ts information thathas same pid from ts_list
 input	: pid 		-> pid value
 ts_list	-> list
 output	: ts_packet_t *
 *************************************************************************/
ts_packet_t *get_last_ts_from_list(uint32_t pid, m_list_t *ts_list)
{
    ts_packet_t *ts;
    m_list_t *list = m_list_next_elem(ts_list);

    while (m_list_elem_data(list) != NULL)
    {
        ts = (ts_packet_t *)m_list_elem_data(list);
        if (ts->pid == pid)
            return ts;

        list = m_list_next_elem(list);
    }

    return NULL;
}

/**************************************************************************
 name	: check_pid_continuity
 purpose	: check whether continuity counters of ts packets is right
 input	: pid 		-> pid value of ts packet
 cont		-> continuity counter of ts packet
 payload	-> payload value of ts packet
 output	: OK -> 1, NOT -> 0
 *************************************************************************/
uint8_t check_ts_pid_continuity(uint32_t pid, uint32_t cont, uint32_t payload,
    uint8_t *data, m_list_t *ts_list)
{
    uint8_t res = 1;
    uint8_t flag = 0;
    ts_packet_t *ts;
    m_list_t *list = NULL;

    if (pid == 0x1FFF)
        return 1;

    if (!m_list_empty(ts_list))
        list = m_list_next_elem(ts_list);

    while ((list != NULL) && (list != ts_list)) {
        ts = (ts_packet_t *)m_list_elem_data(list);

        if (ts->pid == pid) {
            if (payload == 1) {
                if ((ts->pid == pid) &&
                    (((ts->cont + 1) & 0x0F) != cont)) {
                    ERRL(logi, "Cont Err:%d %d %d\n", ts->pid,
                        ts->cont, cont);

                    res = 0;
                }
            }
            /* if payload is zero , continuity must be same*/
            else {
                if ((ts->pid == pid) && ((ts->cont) != cont)) {
                    ERRL(logi, "Cont Err:%d %d %d\n", ts->pid,
                        ts->cont, cont);
                    res = 0;
                }
            }

            /*if pid exists and if ts is correct, refresh your continuity counter of pid*/
            if (res) {
                ts->payload = payload;
                ts->cont = cont;
                memcpy(ts->data, data, TS_PACKET_LENGTH);
            }

            flag = 1;
            break;
        }
        list = m_list_next_elem(list);
    }

    if (!flag) { /*if flag is 0, this pid is new and add it into the list*/

        ts_packet_t *new = m_malloc(sizeof(ts_packet_t));

        if (new != NULL) {
            new->pid = pid;
            new->cont = cont;
            new->payload = payload;
            memcpy(new->data, data, TS_PACKET_LENGTH);
        }

        m_list_add(ts_list, (void *)new);
    }
    return res;
}

/**************************************************************************
 name	: list_all_ts_pid_data
 purpose	: printout all PIDs of the TS packets
 input	: none
 output	: none
 *************************************************************************/
void list_all_ts_pid_data(m_list_t *ts_list)
{
    m_list_t *list;
    ts_packet_t *pid_data;

    if (ts_list == NULL)
        return;

    list = m_list_next_elem(ts_list);

    while ((m_list_elem_data(list) != NULL)) {
        pid_data = (ts_packet_t *)m_list_elem_data(list);
        DEBUGL(logi, "%d\n", pid_data->pid);
        list = m_list_next_elem(list);
    }
}
