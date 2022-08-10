#ifndef LIBC2FJ_H_
#define LIBC2FJ_H_

#include <stdio.h>
#include <stdint.h>
#include <client_common.h>

// return values of is_there_new_comm_pkt() function //
#define EPOLL_TIMEOUT                   (-1)
#define EPOLL_FUNC_FAIL                 (-2)
#define EPOLL_UNEXPECTED_SRC            (-3)
#define EPOLL_EVENT_OK                  (0)

extern uint8_t client_init(const int libc2fj_log_level);	// initialize streaming resources (files, buffer, controlling application etc.)
extern int32_t client_is_there_new_msg(int32_t timeout_ms);
extern void *client_read_new_pkt(int sock_fd, void *inp_buff);
extern int32_t client_socket_read(int32_t sock, void *dest_ptr, int32_t len, uint8_t wait_expected_bytes);
extern uint8_t client_surely_send(int32_t sock, void *src_ptr, int32_t len);
extern void deinit_epoll_resourcess(void);

#endif /* LIBC2FJ_H_ */
