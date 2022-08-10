#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <linux/un.h>
#include <limits.h>
#include <pthread.h>
#include <logger.h>
#include <tcp_client.h>

#define MAX_EPOLL_EVENTS    64
#define CLIENT_SEND_ERR_MAX	5
#define COMM_DOMAIN_SOCK_PATH	"/tmp/sock.tcp.comm"
#define CLIENT_CONNECT_WAIT_MS  1000

static volatile int comm_socket = -1;
mj_log_type logi = LOG_INSTANCE_FAILED;

// epoll related resourcess //
static int epfd = -1;
static struct epoll_event *events = NULL;

static uint8_t init_epoll_resourcess(int sock_fd)
{
    int ret;
	struct epoll_event event;

    if(NULL == events) {
        events = malloc (sizeof (struct epoll_event) * MAX_EPOLL_EVENTS);
        if (!events) {
            ERRL(logi, "malloc(epoll_event) FAILED:(%s)\n", strerror(errno));
            goto EPOLL_INIT_FAILED;
        }
    }

    if(-1 == epfd) {
        epfd = epoll_create (MAX_EPOLL_EVENTS); /* number of monitored maximum file descriptors */
        if (epfd < 0) {
            ERRL(logi, "epoll_create() FAILED:(%s)\n", strerror(errno));
            goto EPOLL_INIT_FAILED2;
        }
    }

    DEBUGL(logi, "client_epfd = %d\n", epfd);

	event.data.fd = sock_fd;
	event.events = EPOLLIN;
	if ((ret = epoll_ctl (epfd, EPOLL_CTL_ADD, sock_fd, &event))) {
		ERRL(logi, "epoll_ctl(ADD) FAILED:(%s)\n", strerror(errno));
		goto EPOLL_INIT_FAILED2;
	}

    return TRUE;

EPOLL_INIT_FAILED2:
    if(NULL != events) {
        free(events);
        events = NULL;
    }

    EPOLL_INIT_FAILED:
    return FALSE;
}

void deinit_epoll_resourcess(void)
{
	INFOL(logi, "%s()-> CALLED\n", __FUNCTION__);

    if(-1 != epfd) {
    	struct epoll_event event;
    	event.data.fd = comm_socket;
    	event.events = EPOLLIN;
        epoll_ctl (epfd, EPOLL_CTL_DEL, comm_socket, &event);
    }
    if(NULL != events) {
        free(events);
        events = NULL;
    }
}

uint8_t client_surely_send(int32_t sock, void *src_ptr, int32_t len) {
    static uint8_t send_err_cnt = 0;
    int32_t send_sum = 0, wlen;
    volatile uint8_t real_write_err = FALSE;
    uint8_t max_trials = 10;

RETRY_WRITE:
    if ((wlen = send(sock, ((uint8_t *)src_ptr) + send_sum, len - send_sum, 0)) != (len - send_sum)) {
        if (wlen > 0) {
            real_write_err = FALSE;
            send_err_cnt = 0;
            send_sum += wlen;
            INFOL(logi,"socket partial WRITE detected:(expected=%u, writed=%d)\n", len, wlen);
            goto RETRY_WRITE;
        }
        else if(0 == wlen){
            if((EAGAIN == errno) || (EINTR == errno) || (EINPROGRESS == errno)) {
            	//:TOOD://
					// 1- test making this sockets NON-BLOCKING 

        		usleep(1000);
            	if(0 < (--max_trials)) {
                    real_write_err = FALSE;
			    	goto RETRY_WRITE;
            	} else {
            		max_trials = 10;
                    real_write_err = TRUE;
                    ERRL(logi, "MaxTrials Count Reached (sock=%d, wlen=%d, errno=%d), R(%s)\n", sock, wlen, errno, strerror(errno));
            	}
            }
			else {
			    real_write_err = TRUE;
                ERRL(logi, "RealWriteErr occured (sock=%d, wlen=%d, errno=%d), R(%s)\n", sock, wlen, errno, strerror(errno));
			}
        }
        else {
            real_write_err = TRUE;
            ERRL(logi, "RealWriteErr occured (sock=%d, wlen=%d, errno=%d, R(%s))\n", sock, wlen, errno, strerror(errno));
        }
        if(TRUE == real_write_err) {
            if(CLIENT_SEND_ERR_MAX != (++send_err_cnt)) {
                INFOL(logi, "RETRYING socket send(send_error_count=%u)\n", send_err_cnt);
                goto RETRY_WRITE;
            }
            else {
                ERRL(logi, "send_error is MAX(%u)\n", CLIENT_SEND_ERR_MAX);
                return FALSE;
            }
        }
    } else {
        send_err_cnt = 0;
        send_sum += wlen;
        real_write_err = FALSE;
    }

    return TRUE;
}

int32_t client_socket_read(int32_t sock, void *dest_ptr, int32_t len, uint8_t wait_expected_bytes) {
    int32_t read_sum = 0, rlen;
    uint8_t max_trials = 10;
    // TRACEL(logi, "%s()-> CALLED with socket(%d) desired_len(%u) wait_expected(%s)\n", \
        __FUNCTION__, sock, len, (TRUE == wait_expected_bytes)?"TRUE":"FALSE");
RETRY_READ:
    if ((rlen = read(sock, ((uint8_t *)dest_ptr) + read_sum, len - read_sum)) != (len - read_sum)) {
        if (rlen > 0) {
            DEBUGL(logi,"socket(%d) partial READ detected:(expected=%u, read=%d)\n", sock, len - read_sum, rlen);
            read_sum += rlen;
            if(TRUE == wait_expected_bytes)
                goto RETRY_READ;
            else {
                //DEBUGL(logi, "read lenght = %d\n", rlen);
                return rlen;
            }
        }
        else {
            if((EAGAIN == errno) || (EINTR == errno) || (EINPROGRESS == errno)) {
            	//:TOOD://
					// making this sockets NON-BLOCKING (cpu usage is %100 and so DONT DO THIS)
            	if(0 < (--max_trials)) {
            		usleep(1000);
			    	goto RETRY_READ;
            	} else
            		return -1;
            }
			else {
                ERRL(logi, "socket(%d) READ error:(%s)\n", sock, strerror(errno));
                return -1;
            }
        }
    } else
        read_sum += rlen;

    //DEBUGL(logi, "total read lenght = %d\n", read_sum);
    return read_sum;
}

// waits for a new msg from server until timeout_ms passed //
int32_t client_is_there_new_msg(int32_t timeout_ms)
{
    if(-1 == comm_socket) {
        ERRL(logi, "IGNORING CHECK NEW PKT\n");
        return EPOLL_FUNC_FAIL;
    }

    int nr_events = epoll_wait (epfd, events, MAX_EPOLL_EVENTS, timeout_ms);
    if (0 > nr_events) {
        ERRL(logi, "epoll_wait() FAILED:(%s)\n", strerror(errno));
        return EPOLL_FUNC_FAIL;
    } else if(0 == nr_events) { // Server FAILED to init fujitsu lsi in time //
        TRACEL(logi, "epoll_wait() TIMEOUT OCCURED\n");
        return EPOLL_TIMEOUT;
    }
    TRACEL(logi, "%u file descriptors has EVENT\n", nr_events);

    volatile uint8_t indx;
    for (indx = 0; indx < nr_events; indx++) {
        TRACEL (logi, "OCCURED EVENT=0x%X on FD=%d\n", events[indx].events, events[indx].data.fd);
        /* We now can, per events[i].events, operate on events[i].data.fd without blocking. */
        if(comm_socket == events[indx].data.fd) {
        	// check if event is EPOLLHUP //
        	if((EPOLLERR == events[indx].events) || (EPOLLHUP == events[indx].events)) {
                ERRL(logi, "EPOLLHUP/EPOLLERR occcured on comm_socket\n");
                return EPOLL_FUNC_FAIL;
        	}

            TRACEL(logi, "EXPECTED EVENT OCCURED\n");
            return EPOLL_EVENT_OK;
        }
    }

    ERRL(logi, "epoll event from UNEXPECTED SOURCE(fd=%d)\n", events[indx].data.fd);
    return EPOLL_UNEXPECTED_SRC;
}


uint8_t client_init(const int client_log_level)
{
    uint8_t err_cnt_comm = 0;
    int fd_comm = -1;

    if(LOG_INSTANCE_FAILED == logi) {
        logi = log_init_instance(client_log_level, client_LOG_FILE);   // ENABLE LOGs
        if(LOG_INSTANCE_FAILED == logi) {
            fprintf(stderr, "Cannot init logfile_instance (client)\n");
            return FALSE;
        }
    }

    // 1- connect to ts-tcp-server, if fails return FALSE //
    if ((-1 == fd_comm) && ((fd_comm = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)) {
        ERRL(logi, "socket(comm) FAILED:(%s)\n", strerror(errno));
        goto CONNECT_FAILED;
    }

    memset(&addr_comm, 0, sizeof(addr_comm));
    addr_comm.sun_family = AF_UNIX;
    strncpy(addr_comm.sun_path, COMM_DOMAIN_SOCK_PATH, sizeof(addr_comm.sun_path)-1);

RETRY_COMM_CONNECT:
    if (connect(fd_comm, (struct sockaddr*)&addr_comm, sizeof(addr_comm)) == -1) {
        ERRL(logi, "connect(comm) FAILED(error count = %u):(%s)\n", err_cnt_comm, strerror(errno));
        surely_msleep(CLIENT_CONNECT_WAIT_MS);
        err_cnt_comm++;
        if(CLIENT_CONNECT_MAX_ERR == (err_cnt_comm)) {
            ERRL(logi, "max connect error reached(%d) (comm)\n", CLIENT_CONNECT_MAX_ERR);
            close(fd_comm);
            comm_socket = -1;
            goto CONNECT_FAILED;
        } else
            goto RETRY_COMM_CONNECT;
    }
    comm_socket = fd_comm;
    // set comm socket non-blocking mode //
    if(FALSE == set_non_blocking(comm_socket, TRUE)) {
        ERRL(logi, "set_non_blocking(comm_socket, TRUE) FAILED\n");
        close(comm_socket);
        comm_socket = -1;
        goto CONNECT_FAILED;
    }
    DEBUGL(logi, "comm_socket = %d CONNECTED to SERVER\n", comm_socket);

    // 1.5- set communication socket init epoll resourcess //
    // setNonblocking(comm_socket, TRUE);
    if(FALSE == init_epoll_resourcess(comm_socket)) {
        ERRL(logi, "init_epoll_resourcess() FAILED\n");
        goto CONNECT_FAILED;
    }

    // 1- connect to ts-tcp-server, if fails return FALSE //
    if ((-1 == comm_socket) && ((comm_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)) {
        ERRL(logi, "socket(comm) FAILED:(%s)\n", strerror(errno));
        goto CONNECT_FAILED;
    }

    memset(&addr_comm, 0, sizeof(addr_comm));
    addr_comm.sun_family = AF_UNIX;
    strncpy(addr_comm.sun_path, COMM_DOMAIN_SOCK_PATH, sizeof(addr_comm.sun_path)-1);

RETRY_COMM_CONNECT:
    if (connect(comm_socket, (struct sockaddr*)&addr_comm, sizeof(addr_comm)) == -1) {
        ERRL(logi, "connect(comm) FAILED(error count = %u):(%s)\n", err_cnt_comm, strerror(errno));
        surely_msleep(CLIENT_CONNECT_WAIT_MS);
        err_cnt_comm++;
        if(CLIENT_CONNECT_MAX_ERR == (err_cnt_comm)) {
            ERRL(logi, "max connect error reached(%d) (comm)\n", CLIENT_CONNECT_MAX_ERR);
            close(comm_socket);
            comm_socket = -1;
            goto CONNECT_FAILED;
        } else
            goto RETRY_COMM_CONNECT;
    }
    DEBUGL(logi, "comm_socket(%d) CONNECTED to SERVER\n", comm_socket);

    return TRUE;

CONNECT_FAILED:
	ERRL(logi, "comm_socket(%d) CONNECTION FAILED\n", comm_socket);
    return FALSE;
}



