#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <limits.h>
#include <pthread.h>
#include "logger.h"

#define EPOLL_MAX_FD_COUNT	20
#define IPC_COMM_SOCKET_PATH "/tmp/tcp.comm.sock"

// epoll related resourcess //
static int epfd = -1;
static struct epoll_event *events = NULL;
static int epoll_fds[EPOLL_MAX_FD_COUNT];
static uint32_t epoll_events[EPOLL_MAX_FD_COUNT];

// tcp-server sockets and related resourcess; listen and accept sockets //
static int comm_socks[2] = {-1, -1};
extern log_type logi;

// epoll related resources initialization
uint8_t init_epoll_resourcess(void)
{
    memset(epoll_fds, -1, sizeof(epoll_fds));
    memset(epoll_events, 0, sizeof(epoll_events));

    if(NULL == events) {
        events = malloc (sizeof (struct epoll_event) * MAX_EPOLL_EVENTS);
        if (!events) {
            ERRL (logi, "malloc(epoll_event) FAILED:(%s)\n", strerror(errno));
            goto EPOLL_INIT_FAILED;
        }
    }

    if(-1 == epfd) {
        epfd = epoll_create (MAX_EPOLL_EVENTS); /* number of monitored maximum file descriptors */
        if (epfd < 0) {
            ERRL(logi, "epoll_create() FAILED:(%s)\n", strerror(errno));
            goto EPOLL_INIT_FAILED;
        }
    }
    DEBUGL(logi, "epfd = %d\n", epfd);

    return TRUE;

EPOLL_INIT_FAILED:
    if(NULL != events) {
        free(events);
        events = NULL;
    }
    return FALSE;
}

// Add a new file-descriptor / socket into epoll with desired event type (read, write etc.)
uint8_t event_wait_add(int new_fd, uint32_t event_type)
{
    volatile uint8_t indx;
    struct epoll_event event;
    int ret;
    // add fd to waiting fd listing array //
    for(indx=0; indx<EPOLL_MAX_FD_COUNT; indx++) {
        if(-1 == epoll_fds[indx]) {
            epoll_fds[indx] = new_fd;
            epoll_events[indx] = event_type;

            // add fd to epoll wait resourcess //
            if(-1 != epfd) {
                event.data.fd = new_fd;
                event.events = event_type;
                if ((ret = epoll_ctl (epfd, EPOLL_CTL_ADD, new_fd, &event))) {
                    ERRL(logi, "epoll_ctl(ADD) FAILED:(%s)\n", strerror(errno));
                    goto EPOLL_WAIT_ADD_FAILED;
                }
            } else {
                ERRL(logi, "epoll interface is NOT INITIALIZED\n");
                goto EPOLL_WAIT_ADD_FAILED;
            }
            break;
        }
    }
    if(EPOLL_MAX_FD_COUNT == indx) {
        ERRL(logi, "NO PLACE to ADD new epoll fd(%d)\n", new_fd);
        goto  EPOLL_WAIT_ADD_FAILED;
    }
    return TRUE;

EPOLL_WAIT_ADD_FAILED:
    return FALSE;
}

// Remove a fil-descriptor / socket from epoll
uint8_t event_wait_del(int old_fd)
{
    int ret;
    volatile uint8_t indx;
    struct epoll_event event;

    for(indx=0; indx<EPOLL_MAX_FD_COUNT; indx++) {
        if(old_fd == epoll_fds[indx]) {
            // set temporary epoll object //
            event.data.fd = epoll_fds[indx];
            event.events = epoll_events[indx];
            // clear fd from our buffers //
            epoll_fds[indx] = -1;
            epoll_events[indx] = 0;
            // delete fd from epoll resourcess //
            if(-1 != epfd) {
                if((ret = epoll_ctl (epfd, EPOLL_CTL_DEL, old_fd, &event))) {
                    ERRL(logi, "epoll_ctl(DEL) FAILED:(%s)\n", strerror(errno));
                    goto EVENT_WAIT_DEL_FAILED;
                }
            }
            else {
                ERRL(logi, "epoll interface is NOT INITIALIZED\n");
                goto EVENT_WAIT_DEL_FAILED;
            }
        }
    }
    return TRUE;

EVENT_WAIT_DEL_FAILED:
    return FALSE;
}

// Release epoll related resourcess
static uint8_t deinit_epoll_resourcess(void)
{
    INFOL(logi, "%s()-> CALLED\n", __FUNCTION__);

    volatile uint8_t indx;
    int ret = TRUE;

    for(indx=0 ; indx<EPOLL_MAX_FD_COUNT ; indx++) {
        if((-1 != epoll_fds[indx]) && (FALSE == event_wait_del(epoll_fds[indx]))) {
            ERRL(logi, "event_wait_del() FAILED\n");
            ret = FALSE;
        }
    }

    memset(epoll_fds, -1, sizeof(epoll_fds));
    memset(epoll_events, 0, sizeof(epoll_events));
    //epfd = -1;
    if(NULL != events) {
        free(events);
        events = NULL;
    }

    return ret;
}

// called when a client wants to connect to server
// accepts connection and adds new socket that is assigned to new client into epoll
static inline uint8_t on_connect_request(int *listen_fd_ptr, int *accept_fd_ptr, uint8_t add2epoll, uint32_t epoll_event_type, uint8_t nonblocking) {
    uint8_t retval = TRUE;

    // accept new conecction, init new socket and add it into epoll wait //
    if ( (*accept_fd_ptr = accept(*listen_fd_ptr, NULL, NULL)) == -1) {
        ERRL(logi, "accept(listen_fd) FAILED:(%s)\n", strerror(errno));
        *accept_fd_ptr = -1;
        retval = FALSE;
        goto ON_CONNECT_REQ_FAILED;
    }
#if(1)
    struct linger so_linger = {
            .l_onoff = 1,   /* Enable linger*/
            .l_linger = 1   /* Set timeout as one second to close connection*/
    };
    struct timeval tv = {	// sending timeout is 1 seconds // 
            .tv_sec = 1,
            .tv_usec = 0
    };

    int ret = setsockopt(*accept_fd_ptr, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
    if (ret < 0) {
        ERRL(logi, "setsockopt(accept_comm, SO_SNDTIMEO) FAILED:(%s)\n", strerror(errno));
        retval = FALSE;
    }

    ret = setsockopt(*accept_fd_ptr, SOL_SOCKET, SO_LINGER, (char *)&so_linger, sizeof(struct linger));
    if (ret < 0) {
        ERRL(logi, "setsockopt(accept_comm, SO_LINGER) FAILED:(%s)\n", strerror(errno));
        retval = FALSE;
    }
#endif
    // set stream sockets non-blocking //
    if(nonblocking) {
        if(FALSE == set_non_blocking(*accept_fd_ptr, TRUE)) {
            ERRL(logi, "set_non_blocking(accept_sock, TRUE) FAILED\n");
            retval = FALSE;
        }
    }

    if(TRUE == add2epoll) {
        if(FALSE == event_wait_add(*accept_fd_ptr, epoll_event_type)) {
            ERRL(logi, "event_wait_add(accept_ptr) FAILED\n");
            retval = FALSE;
        }
    }

ON_CONNECT_REQ_FAILED:
    return retval;
}

// create and listen sockets, add sockets into epoll interface //
// init_epoll_resourcess() must be called before this function //
uint8_t init_listening_sockets(void) // initialize streaming resources (files, buffer, controlling application etc.)
{
    struct sockaddr_un addr_comm;
    int listen_comm = -1;
    char const *comm_socket_path = IPC_COMM_SOCKET_PATH;

    if ( (listen_comm = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        ERRL(logi, "socket(comm) FAILED:(%s)\n", strerror(errno));
        goto INIT_SOCK_LISTEN_FAILED;
    }
    DEBUGL(logi, "listen_comm = %d\n", listen_comm);
    if(FALSE == set_non_blocking(listen_comm, TRUE)) {
        ERRL(logi, "set_non_blocking(comm) FAILED:(%s)\n", strerror(errno));
        goto INIT_SOCK_LISTEN_FAILED;
    }

    memset(&addr_comm, 0, sizeof(addr_comm));
    addr_comm.sun_family = AF_UNIX;
    strncpy(addr_comm.sun_path, comm_socket_path, sizeof(addr_comm.sun_path)-1);

    if (bind(listen_comm, (struct sockaddr *)&addr_comm, sizeof(addr_comm)) == -1) {
        ERRL(logi, "bind(comm) FAILED:(%s)\n", strerror(errno));
        goto INIT_SOCK_LISTEN_FAILED;
    }

    if (listen(listen_comm, SOMAXCONN) < 0) {
        ERRL(logi, "listen(comm) FAILED:(%s)\n", strerror(errno));
        goto INIT_SOCK_LISTEN_FAILED;
    }
    comm_socks[0] = listen_comm;
    if(FALSE == event_wait_add(comm_socks[0], EPOLLIN)) {  // we will wait CLIENT to call accept() function on this socket //
        ERRL(logi, "event_wait_add(listen_comm) FAILED\n");
        goto INIT_SOCK_LISTEN_FAILED;
    }

    return TRUE;

INIT_SOCK_LISTEN_FAILED:
    if(-1 != listen_comm) {
        close(listen_comm);
        comm_socks[0] = -1;
    }

    return FALSE;
}

static uint8_t deinit_listening_sockets(void)
{
    INFOL(logi, "%s()-> CALLED\n", __FUNCTION__);
    if(-1 != comm_socks[0]) {
        event_wait_del(comm_socks[0]);
        close(comm_socks[0]);
        comm_socks[0] = -1;
    }
    return TRUE;
}

// to be called when client dissconnected //
static uint8_t deinit_comm_sockets(uint8_t comm_sock)
{
    INFOL(logi, "%s()-> CALLED\n", __FUNCTION__);
    if((comm_sock) && (-1 != comm_socks[0])) {
        event_wait_del(comm_socks[0]);
        close(comm_socks[0]);
        comm_socks[0] = -1;
    }
    return TRUE;
}

int32_t __socket_surely_send(int32_t sock, void *src_ptr, int32_t len) {
#define MAX_TRIALS_SEND_ERR	1
    uint8_t send_err_cnt = 0;
    int32_t send_sum = 0, wlen;
    volatile uint8_t real_write_err = FALSE;
    uint8_t max_trials = MAX_TRIALS_SEND_ERR;

RETRY_WRITE:
    if ((wlen = send(sock, ((uint8_t *)src_ptr) + send_sum, len - send_sum, 0)) != (len - send_sum)) {
        if (wlen > 0) {
            real_write_err = FALSE;
            send_err_cnt = 0;
            send_sum += wlen;
            INFOL(logi,"socket(%d) partial WRITE detected:(expected=%u, writed=%d)\n", sock, len, wlen);
            goto RETRY_WRITE;
        } else if(0 >= wlen) {
            if((EWOULDBLOCK == errno) || (EAGAIN == errno) || (EINTR == errno) || (EINPROGRESS == errno)) {
                //:TODO://
                    // 1- test making accept sockets NON-BLOCKING
        		usleep(1);
            	if(0 < (--max_trials)) {
                    real_write_err = FALSE;
			    	goto RETRY_WRITE;
            	} else {
            		max_trials = MAX_TRIALS_SEND_ERR;
                    real_write_err = TRUE;
                    ERRL(logi, "MaxTrials Count Reached (sock=%d, wlen=%d, errno=%d), R(%s)\n", sock, wlen, errno, strerror(errno));
            	}
            } else {
                real_write_err = TRUE;
                ERRL(logi, "RealWriteErr occured (sock=%d, wlen=%d, errno=%d), R(%s)\n", sock, wlen, errno, strerror(errno));
            }
        }
        if(TRUE == real_write_err) {
            if(LSI_SERVER_SEND_ERR_MAX != (++send_err_cnt)) {
                INFOL(logi, "RETRYING socket(%d) send(send_error_count=%u)\n", sock, send_err_cnt);
                goto RETRY_WRITE;
            }
            else {
                ERRL(logi, "socket(%d) send_error is MAX(%u), closing SOCKET\n", sock, LSI_SERVER_SEND_ERR_MAX);
                return wlen;
            }
        }
    } else {
        send_err_cnt = 0;
        send_sum += wlen;
        real_write_err = FALSE;
    }

    return send_sum;
}

int32_t __socket_read(int32_t sock, void *dest_ptr, int32_t len, uint8_t wait_expected_bytes) {
    int32_t read_sum = 0, rlen;
    uint8_t max_trials = 10;
RETRY_READ:
    if ((rlen = read(sock, ((uint8_t *)dest_ptr) +  + read_sum, len - read_sum)) != (len - read_sum)) {
        if (rlen > 0) {
            INFOL(logi,"socket partial READ detected:(expected=%u, read=%d)\n", len, rlen);
            read_sum += rlen;
            if(TRUE == wait_expected_bytes) {
            	usleep(1000);
            	goto RETRY_READ;
            }
            else
                return rlen;
        }
        else {
            if((EAGAIN == errno) || (EINTR == errno) || (EINPROGRESS == errno)) {
                //:TODO://
					// 1- test making accept sockets NON-BLOCKING 
            	if(0 < (--max_trials)) {
            		usleep(1000);
			    	goto RETRY_READ;
            	} else
            		return -1;
            }
			else {
                ERRL(logi, "socket READ error:(%s), closing SOCKET\n", strerror(errno));
                return -1;
            }
        }
    } else
    	read_sum += rlen;

    return read_sum;
}

uint8_t app_event_loop(void)
{
    uint8_t exit_requested = FALSE;
    int nr_events;
    if(-1 == epfd) {
        ERRL(logi, "epoll interface is NOT INITIALIZED\n");
        goto APP_EVENT_LOOP_FAILED;
    }

WAIT_FOR_CLIENT_EVENTs:
    while (1) {
        nr_events = epoll_wait (epfd, events, MAX_EPOLL_EVENTS, 1000);
        if (0 > nr_events) {
            ERRL(logi, "epoll_wait() FAILED:(%s)\n", strerror(errno));
            goto APP_EVENT_LOOP_RESTART;
        } else if(0 == nr_events) { // Server FAILED to init fujitsu lsi in time //
            TRACEL(logi, "epoll_wait() TIMEOUT OCCURED\n");
            goto WAIT_FOR_CLIENT_EVENTs;
        }
        TRACEL(logi, "%u file descriptors has EVENT\n", nr_events);

        volatile uint8_t indx;
        for (indx = 0; indx < nr_events; indx++) {
            DEBUGL (logi, "OCCURED EVENT_TYPE=0x%X on FD=%d\n", events[indx].events, events[indx].data.fd);
            if(comm_socks[0] == events[indx].data.fd) {
                DEBUGL(logi, "listen_comm socket EVENT OCCURED\n");
                if(-1 != comm_socks[1]) {
                    ERRL(logi, "accept_comm socket is INITIALIZED, closing previous one\n");
                    deinit_comm_sockets(TRUE);
                }
                // Init accept_comm socket and add it into epoll wait, we will read client commands from it //
                if(FALSE == on_connect_request(&(comm_socks[0]), &(comm_socks[1]), TRUE, EPOLLIN, TRUE)) {
                    ERRL(logi, "on_connect_request(listen_comm) FAILED:(%s)\n", strerror(errno));
                    goto APP_EVENT_LOOP_RESTART;
                }
                DEBUGL(logi, "accept_comm = %d\n", comm_socks[1]);
            }
            if(comm_socks[1] == events[indx].data.fd) {
            	// check if event is EPOLLHUP //
            	if((EPOLLERR == events[indx].events) || (EPOLLHUP == events[indx].events)) {
                    ERRL(logi, "EPOLLHUP/EPOLLERR occcured on comm_accept, CLOSING SOCKET CONNECTION\n");
                    goto APP_EVENT_LOOP_RESTART;
            	}
            	//:TODO: Call __socket_read() to read the pending data from accept socket //
            }
        }
    }

APP_EVENT_LOOP_EXIT:
	fjserver_stop_stream(TRUE);
    return TRUE;

APP_EVENT_LOOP_RESTART:
	// close accepted sockets related resourcess (sockets & epoll) and return to wait loop again //
	deinit_comm_sockets(TRUE);
    // listen sockets will be still waiting for new connections //
    goto WAIT_FOR_CLIENT_EVENTs;

APP_EVENT_LOOP_FAILED:
    return FALSE;
}

uint8_t server_comm_init(void)
{
    if(FALSE == init_epoll_resourcess()) {
        ERRL(logi, "init_epoll_resourcess() FAILED\n");
        goto FJSERVER_COMM_INIT_FAILED;
    }
    if(FALSE == init_listening_sockets()) {
        ERRL(logi, "init_listening_sockets() FAILED\n");
        goto FJSERVER_COMM_INIT_FAILED;
    }
    INFOL(logi, "COMMUNICATION INIT COMPLETED\n");
    return TRUE;

FJSERVER_COMM_INIT_FAILED:
    ERRL(logi, "COMMUNICATION INIT FAILED\n");
    return FALSE;
}
