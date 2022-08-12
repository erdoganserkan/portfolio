#include <ftp_client.h>
#include <string.h>
#include <common.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

static pthread_t _ftp_threads[2];

static int8_t __ftp_session_close(void);
#ifdef FTP_PASSIVE_MODE
static int32_t __ftp_init_data_connection(uint8_t *cmd);
#else
static int32_t __ftp_init_data_connection(void);
#endif
static int32_t __ftp_init_command_connection(uint8_t *host, uint32_t port);
static void __ftp_authentication(uint8_t type, uint8_t *ptr);
static void __ftp_send_command(uint8_t command, uint8_t *str);
static void *__ftp_read_command_thread(void *arg);
static void *__ftp_read_data_thread(void *arg);
static inline uint32_t __ftp_read_data_size(void);

void (*__send_ftp_message)(int32_t sock, uint8_t msg_id, uint8_t msg_type,
    uint8_t *msg);
void (*__send_chunk_immediately)(struct chunk_s *ch, int sock);

static ftp_client_t _ftp_client;
static ftp_filenames_t _ftp_filename;

uint8_t FTP_SESSION_MESSAGE[5][FTP_MSG_LENGTH] =
    {
                    "\0", /* FTP_SESSION_REQ */
                    "\0", /* FTP_SESSION_INIT */
                    "FTP Session is established Successfully.\0", /* FTP_SESSION_SUCCESS */
                    "\0", /* FTP_SESSION_CLOSE */
                    "FTP Sesison is closed.\0" /* FTP_SESSION_CLOSED */
    };

uint8_t FTP_COMMAND_MESSAGE[6][FTP_MSG_LENGTH] =
    {
                    "\0", /* FTP_CSTATUS_NULL */
                    "FTP Service is ready.", /* FTP_CSTATUS_SERVICE_READY */
                    "FTP Authentication error.\0", /* FTP_CSTATUS_AUTH */
                    "FTP Transfer is ready.\0", /* FTP_CSTATUS_TRANSFER_READY */
                    "FTP Transfer is closed.\0" /* FTP_CSTATUS_TRANSFER_CLOSED */
                        "FTP File not found!!!" /* FTP_CSTATUS_FILE_NOT_FOUND */
    };

/**************************************************************************
 name	: ftp_init
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
void ftp_init(void *callback1, void *callback2)
{
    memset((uint8_t *)&_ftp_client, 0, sizeof(ftp_client_t));
    _ftp_client.session = FTP_SESSION_CLOSED;
    __send_ftp_message = (void *)callback1;
    __send_chunk_immediately = (void *)callback2;
}

/**************************************************************************
 name	: ftp_get_session_status
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint8_t ftp_get_session_status(void)
{
    return _ftp_client.session;
}

/**************************************************************************
 name	: __ftp_set_session_status
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static void __ftp_set_session_status(uint8_t session)
{
    _ftp_client.session = session;
    __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_SESSION, session,
        FTP_SESSION_MESSAGE[session]);
}

/**************************************************************************
 name	: __ftp_set_command_status
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static void __ftp_set_command_status(uint8_t status)
{
    _ftp_client.cstatus = status;
    __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_CSTATUS, status,
        FTP_COMMAND_MESSAGE[status]);
}

/**************************************************************************
 name	: __ftp_get_command_status
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint8_t __ftp_get_command_status(void)
{
    return _ftp_client.cstatus;
}

/**************************************************************************
 name	: __ftp_set_data_status
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static void __ftp_set_data_status(uint8_t status)
{
    _ftp_client.dstatus = status;
}

/**************************************************************************
 name	: __ftp_get_data_status
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint8_t __ftp_get_data_status(void)
{
    return _ftp_client.dstatus;
}

/**************************************************************************
 name	: ftp_get_file_list
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint8_t ftp_get_file_list(ftp_filenames_t *ftp)
{
    uint8_t err;
    if (ftp_get_session_status() == FTP_SESSION_SUCCESS &&
        __ftp_get_data_status() == FTP_DSTATUS_NULL) {
        memset((uint8_t *)ftp, 0, sizeof(ftp_filenames_t));
        memset((uint8_t *)&_ftp_filename, 0, sizeof(ftp_filenames_t));
#ifdef FTP_PASSIVE_MODE
        _ftp_client.data_command = FTP_DATA_COMMAND_FILE_LIST;
        __ftp_send_command(FTP_COMMAND_PASV, NULL);
#else
        __ftp_send_command(FTP_COMMAND_GET_FILE_LIST, NULL);
#endif
    }

    flag_pend(_ftp_client.dflag, FLAG_FTP_DATA_OK,
        WAIT_FLAG_SET_ANY + FLAG_CONSUME, 0x05, &err);

    memcpy((uint8_t *)ftp, (uint8_t *)&_ftp_filename, sizeof(ftp_filenames_t));

    if (err == FLAG_ERR_TIMEOUT)
        return 0;
    else
        return 1;
}

/**************************************************************************
 name	: ftp_file_size
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint8_t ftp_file_size(uint8_t *file_name, uint32_t *file_size)
{
    uint32_t i;
    for (i = 0; i < _ftp_filename.count; i++) {
#if 0
        printf("%s, %s %d\n",_ftp_filename.name[i], file_name, _ftp_filename.size[i]);
#endif
        if (compare_strings(_ftp_filename.name[i], file_name,
            strlen(_ftp_filename.name[i]))) {
            *file_size = _ftp_filename.size[i];
            return 1;
        }
    }

    return 0;
}

/**************************************************************************
 name	: ftp_get_file
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
void ftp_get_file(uint8_t *file_name)
{
    uint8_t err;
    send_file_t file_send;
    int32_t file_size = 0;

    if (ftp_get_session_status() == FTP_SESSION_SUCCESS &&
        __ftp_get_data_status() == FTP_DSTATUS_NULL) {
#ifdef FTP_PASSIVE_MODE
        _ftp_client.data_command = FTP_DATA_COMMAND_FILE;
        memset(_ftp_client.file_name, 0, sizeof(_ftp_client.file_name));
        memcpy(_ftp_client.file_name, file_name, strlen(file_name));
        __ftp_send_command(FTP_COMMAND_PASV, NULL);
#else
        __ftp_send_command(FTP_COMMAND_GET_FILE, file_name);
#endif
        flag_pend(_ftp_client.dflag, FLAG_FTP_DATA_OK,
            WAIT_FLAG_SET_ANY + FLAG_CONSUME, 0x00, &err);
    }
}

/**************************************************************************
 name	: __ftp_send_command
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static void __ftp_send_command(uint8_t command, uint8_t *str)
{
    uint8_t err;
    uint8_t cmd[CHUNK_DATA_LEN] = { 0 };
    uint8_t buf[CHUNK_DATA_LEN] = { 0 };
    struct sockaddr_in client;
    uint32_t lport, slen;

#ifndef FTP_PASSIVE_MODE
    slen=sizeof(struct sockaddr_in);
    getsockname(_ftp_client.csock, (struct sockaddr*)&client, &slen);
    lport = _ftp_client.lport;
    snprintf (cmd,sizeof(cmd),"PORT %d,%d,%d,%d,%d,%d\r\n",
        (client.sin_addr.s_addr & 0x000000FF),
        (client.sin_addr.s_addr & 0x0000FF00) >> 8,
        (client.sin_addr.s_addr & 0x00FF0000) >> 16,
        (client.sin_addr.s_addr & 0xFF000000) >> 24,
        (lport & 0xFF00) >> 8,
        (lport & 0x00FF));

    send_tcp_socket(_ftp_client.csock, cmd, strlen(cmd));
#endif
    switch (command)
    {
    case FTP_COMMAND_GET_FILE_LIST:
        snprintf(buf, sizeof(buf), "MLSD\r\n");
        break;
    case FTP_COMMAND_GET_FILE:
        snprintf(buf, sizeof(buf), "RETR %s\r\n", str);
        break;
    case FTP_COMMAND_QUIT:
        snprintf(buf, sizeof(buf), "QUIT\r\n");
        break;
    case FTP_COMMAND_MODE:
        snprintf(buf, sizeof(buf), "MODE %s\r\n", str);
        break;
    case FTP_COMMAND_TYPE:
        snprintf(buf, sizeof(buf), "TYPE %s\r\n", str);
        break;
    case FTP_COMMAND_SYST:
        snprintf(buf, sizeof(buf), "SYST\r\n");
        break;
    case FTP_COMMAND_FEAT:
        snprintf(buf, sizeof(buf), "FEAT\r\n");
        break;
    case FTP_COMMAND_PWD:
        snprintf(buf, sizeof(buf), "PWD\r\n");
        break;
    case FTP_COMMAND_PASV:
        snprintf(buf, sizeof(buf), "PASV\r\n");
        break;
    }
    _ftp_client.command = command;
#if 0
    printf("buf:%s\n",buf);
#endif
    send_tcp_socket(_ftp_client.csock, buf, strlen(buf));
#if 0
    flag_pend(_ftp_client.cflag, FLAG_FTP_COMMAND_OK,
        WAIT_FLAG_SET_ANY+FLAG_CONSUME, 0x05, &err);

    if (err == FLAG_ERR_TIMEOUT) {
        printf("__ftp_send_command timeout\n");
    }
    else if (err == FLAG_NO_ERROR) {
        printf("__ftp_send_command OKKKKKK\n");
    }
#endif
}

/**************************************************************************
 name	: __ftp_authentication
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static void __ftp_authentication(uint8_t type, uint8_t *ptr)
{
    uint8_t buf[100] = { 0 };

    if (type == FTP_AUTH_USER)
        snprintf(buf, sizeof(buf), "USER %s\r\n", ptr);
    else if (type == FTP_AUTH_PASS)
        snprintf(buf, sizeof(buf), "PASS %s\r\n", ptr);

    send_tcp_socket(_ftp_client.csock, buf, strlen(buf));
}

/**************************************************************************
 name	: __ftp_session_init
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
int8_t ftp_session_init(int32_t sock, ftp_config_t *ftp)
{
    uint8_t err;

    if (ftp_get_session_status() != FTP_SESSION_CLOSED)
        return FAILURE;

    __ftp_set_session_status(FTP_SESSION_INIT);

    memset((uint8_t *)&_ftp_client, 0, sizeof(ftp_client_t));
    _ftp_client.dflag = NULL;
    _ftp_client.cflag = NULL;
    _ftp_client.sock = sock;

    memcpy((uint8_t *)&_ftp_client.config, (uint8_t *)ftp,
        sizeof(ftp_config_t));
#if 0
    printf("HOST:%s, U:%s, P:%s, Port:%d\n", _ftp_client.config.host,
        _ftp_client.config.user,
        _ftp_client.config.pass,
        _ftp_client.config.port);
#endif
    /* It's client socket to connect to ftp server*/
    if (__ftp_init_command_connection(_ftp_client.config.host,
        _ftp_client.config.port) == FAILURE) {
        goto error;
    }

#ifndef FTP_PASSIVE_MODE
    /* It's server socket for ftp server to connect to us*/
    if (__ftp_init_data_connection() == FAILURE) {
        goto error;
    }
#endif

    /*Create flag for data communication*/
    _ftp_client.dflag = flag_create(0x00, &err);
#if 0
    /*Create flag for command communication*/
    _ftp_client.cflag = flag_create(0x00, &err);
#endif

    if (pthread_create(&_ftp_threads[0], NULL, __ftp_read_data_thread, NULL)) {
        goto error;
    }

    if (pthread_create(&_ftp_threads[1], NULL, __ftp_read_command_thread,
        NULL)) {
        goto error;
    }
#if 0
    printf("FTP_SESSION_SUCCESS\n");
#endif
    __ftp_set_session_status(FTP_SESSION_SUCCESS);
    return SUCCESS;

    error:
    #if 0
    printf("__ftp_session_init error\n");
#endif
    __ftp_session_close();
    return FAILURE;
}

/**************************************************************************
 name	: ftp_data_socket_close
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
void ftp_data_socket_close(void)
{
    if (_ftp_client.dsock) {
        close(_ftp_client.dsock);
        _ftp_client.dsock = 0;
    }
}

/**************************************************************************
 name	: ftp_session_close
 purpose	: close ftp session
 input	: none
 output	: none
 ***************************************************************************/
void ftp_session_close(int32_t sock)
{
    _ftp_client.sock = sock;
    if (ftp_get_session_status() == FTP_SESSION_SUCCESS)
        __ftp_send_command(FTP_COMMAND_QUIT, NULL);
    else if (ftp_get_session_status() == FTP_SESSION_CLOSED)
        __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_SESSION,
            ftp_get_session_status(),
            FTP_COMMAND_MESSAGE[ftp_get_session_status()]);
}

/**************************************************************************
 name	: __ftp_session_close
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static int8_t __ftp_session_close(void)
{
    if (_ftp_client.dsock)
        close(_ftp_client.dsock);
    if (_ftp_client.csock)
        close(_ftp_client.csock);

    if (_ftp_client.dflag) {
        flag_post(_ftp_client.dflag, FLAG_FTP_DATA_EXIT, OS_FLAG_SET);
        /* Wait certain time to prevent seg fault. */
        usleep(100);
        free(_ftp_client.dflag);
        _ftp_client.dflag = NULL;
    }

#if 0
    if (_ftp_client.cflag) {
        flag_post(_ftp_client.cflag, FLAG_FTP_COMMAND_EXIT, OS_FLAG_SET);
        /* Wait certain time to prevent seg fault. */
        usleep(100);
        free(_ftp_client.cflag);
        _ftp_client.cflag = NULL;
    }
#endif

    __ftp_set_session_status(FTP_SESSION_CLOSED);

    return SUCCESS;
}

/**************************************************************************
 name	: __ftp_init_command_connection
 purpose	: open command connection with ftp server
 input	: none
 output	: none
 ***************************************************************************/
static int32_t __ftp_init_command_connection(uint8_t *host, uint32_t port)
{
    int32_t sd;
    int8_t ip[INET6_ADDRSTRLEN];
    struct sockaddr_in server;
    struct hostent *h;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return FAILURE;
    }

    if (!(h = gethostbyname((const char *)host))) {
        close(sd);
        return FAILURE;
    }

    snprintf(ip, sizeof(ip), "%u.%u.%u.%u",
        h->h_addr[0] & 0xFF, h->h_addr[1] & 0xFF, h->h_addr[2] & 0xFF,
        h->h_addr[3] & 0xFF);

    server.sin_family = h->h_addrtype;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (connect(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) < 0) {
        __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_CSTATUS, 0,
            "FTP Server connection error!!!");
        close(sd);
        return FAILURE;
    }

    _ftp_client.csock = sd;

    return SUCCESS;
}

/**************************************************************************
 name	: __ftp_init_data_connection
 purpose	: open data connection with ftp server
 input	: Example : "227 Entering Passive Mode (192,168,2,113,4,196)"
 output	: none
 ***************************************************************************/
#ifdef FTP_PASSIVE_MODE
static int32_t __ftp_init_data_connection(uint8_t *cmd)
{
    uint8_t i, cnt;
    uint8_t host[IP_LENGTH] = { 0 };
    uint8_t line1[50] = { 0 };
    uint8_t line2[50] = { 0 };
    uint8_t line3[50] = { 0 };
    uint32_t high, low, port;
    int32_t sd;
    int8_t ip[INET6_ADDRSTRLEN];
    struct sockaddr_in server;
    struct hostent *h;

    clear_string_new_line_character(cmd, strlen(cmd));
    get_parse_string_value(cmd, line1, strlen(cmd), 1, '(', 50);
    get_parse_string_value(line1, line2, strlen(line1), 0, ')', 50);

    cnt = 0;
    for (i = 0; i < strlen(line2); i++) {
        if (line2[i] == ',') {
            if (cnt != 3)
                line2[i] = '.';
            cnt++;
        }
    }

    get_parse_string_value(line2, host, strlen(line2), 0, ',', IP_LENGTH);
    get_parse_string_value(line2, line3, strlen(line2), 1, ',', IP_LENGTH);
    high = get_parse_int_value(line3, strlen(line3), 0, '.');
    low = get_parse_int_value(line3, strlen(line3), 1, '.');
    port = (high << 8) | low;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return FAILURE;
    }

    if (!(h = gethostbyname((const char *)host))) {
        close(sd);
        return FAILURE;
    }

    snprintf(ip, sizeof(ip), "%u.%u.%u.%u",
        h->h_addr[0] & 0xFF, h->h_addr[1] & 0xFF, h->h_addr[2] & 0xFF,
        h->h_addr[3] & 0xFF);

    server.sin_family = h->h_addrtype;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (connect(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) < 0) {
#if 0
        printf("data connect error\n");
#endif
        close(sd);
        return FAILURE;
    }
#if 0
    printf("data connect ok\n");
#endif
    _ftp_client.dsock = sd;

    return SUCCESS;
}
#else
static int32_t __ftp_init_data_connection(void)
{
    int32_t sock;
    struct sockaddr_in server;

    _ftp_client.lport=(rand()%64511)+1024;

    server.sin_family=AF_INET;
    server.sin_port=htons((uint32_t) _ftp_client.lport);
    server.sin_addr.s_addr=INADDR_ANY;

    if ((sock=socket(AF_INET,SOCK_STREAM,0))<0) {
        return FAILURE;
    }

    if (bind(sock, (struct sockaddr*)&server, sizeof(server))<0) {
        close(sock);
        return FAILURE;
    }

    if (listen(sock, 1) < 0) {
        close(sock);
        return FAILURE;
    }

    _ftp_client.dsock = sock;

    return SUCCESS;
}
#endif

/**************************************************************************
 name	: __ftp_read_data_size
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static inline uint32_t __ftp_read_data_size(void)
{
    if (_ftp_client.command == FTP_COMMAND_GET_FILE_LIST)
        return 1;
    else if (_ftp_client.command == FTP_COMMAND_GET_FILE)
        return CHUNK_DATA_LEN - _ftp_client.didx;

    return 1;
}

/**************************************************************************
 name	: __check_ftp_file_names
 purpose	:
 input	: Example : "type=file;modify=20130703101556;size=22; a.txt"
 output	: none
 ***************************************************************************/
static void __check_ftp_file_names(uint8_t *buf)
{
    uint16_t i;
    uint16_t last_comma = 0;
    uint8_t *ptr;
    if (strstr(buf, "type=file") != NULL) {

        clear_string_new_line_character(buf, strlen(buf));
        ptr = strstr(buf, "size=\0");
        ptr += strlen("size=\0");
        _ftp_filename.size[_ftp_filename.count] = atoi(ptr);

        for (i = 0; i < strlen(buf); i++) {
            if (buf[i] == ';')
                last_comma = i;
        }

        last_comma += 2; //skip space character and go to first character of the file name
        memcpy(_ftp_filename.name[_ftp_filename.count++], buf + last_comma,
            strlen(buf) - last_comma);
#if 0
        printf("%s %d\n", _ftp_filename.name[_ftp_filename.count-1], _ftp_filename.size[_ftp_filename.count-1]);
#endif
    }
}

/**************************************************************************
 name	: __ftp_read_command_thread
 purpose	: read socket from command connection of the ftp server
 input	: none
 output	: none
 ***************************************************************************/
static void *__ftp_read_command_thread(void *arg)
{
    uint8_t ch;
    uint32_t command = 0;

    while (recv(_ftp_client.csock, &ch, 1, 0) > 0) {
        _ftp_client.cbuf[_ftp_client.cidx++] = ch;

        if (_ftp_client.cbuf[_ftp_client.cidx - 1] == '\n'
            && _ftp_client.cbuf[_ftp_client.cidx - 2] == '\r') {
#if 0
            printf("command:%s\n",_ftp_client.cbuf);
#endif
            ch = _ftp_client.cbuf[3];
            _ftp_client.cbuf[3] = 0;
            command = atoi(_ftp_client.cbuf);
            _ftp_client.cbuf[3] = ch;
            if (command == 220) { /* Service ready*/
                __ftp_set_command_status(FTP_CSTATUS_SERVICE_READY);
                __ftp_authentication(FTP_AUTH_USER, _ftp_client.config.user);
            }
            else if (command == 331) { /* Password required */
                __ftp_set_command_status(FTP_CSTATUS_AUTH);
                __ftp_authentication(FTP_AUTH_PASS, _ftp_client.config.pass);
            }
            else if (command == 230) { /* Ready to transfer files */
                __ftp_set_command_status(FTP_CSTATUS_TRANSFER_READY);
                __ftp_send_command(FTP_COMMAND_SYST, NULL);
            }
            else if (command == 227) { /* Entering passive mode */
#ifdef FTP_PASSIVE_MODE
                /* It's server socket for ftp server to connect to us*/
                if (__ftp_init_data_connection(_ftp_client.cbuf) == SUCCESS) {
                    if (_ftp_client.data_command == FTP_DATA_COMMAND_FILE_LIST)
                        __ftp_send_command(FTP_COMMAND_GET_FILE_LIST, NULL);
                    else if (_ftp_client.data_command == FTP_DATA_COMMAND_FILE)
                        __ftp_send_command(FTP_COMMAND_GET_FILE,
                            _ftp_client.file_name);
                }
#endif
            }
            else if (command == 215) { /* Name system type */
                __ftp_send_command(FTP_COMMAND_TYPE, "I");
            }
            else if (command == 200) { /* Command OK*/
            }
            else if (command == 226) { /* Closing Data Connection. Requested file action successful. File transfer or file abort*/
                __ftp_set_command_status(FTP_CSTATUS_TRANSFER_CLOSED);
                __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_CSTATUS, 0,
                    "Closed Data Connection.");
            }
            else if (command == 426) { /* Connection closed; transfer aborted*/
                __ftp_set_command_status(FTP_CSTATUS_TRANSFER_CLOSED);
                __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_CSTATUS, 0,
                    "Transfer aborted.");
            }
            else if (command == 421) { /* Connection timed out */
                __ftp_set_command_status(FTP_CSTATUS_TRANSFER_CLOSED);
                __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_CSTATUS, 0,
                    "Connection timed out.");
            }
            else if (command == 550) {/* Could not open input file: No such file or directory */
                __ftp_set_command_status(FTP_CSTATUS_FILE_NOT_FOUND);
                __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_CSTATUS, 0,
                    "File not found!!!.");
            }
            else if (command == 221) { /* Server closing control connection */
                __ftp_set_command_status(FTP_CSTATUS_NULL);
                break;
            }
            else if (command == 150) { /* Opened data connection */
                flag_post(_ftp_client.dflag, FLAG_FTP_DATA_READ, OS_FLAG_SET);
                __send_ftp_message(_ftp_client.sock, FTP_MSG_ID_CSTATUS, 0,
                    "Opened Data Connection");
            }
            memset(_ftp_client.cbuf, 0, CHUNK_DATA_LEN);
            _ftp_client.cidx = 0;
        }
    }

    __ftp_session_close();
#if 0
    printf("__ftp_read_command_thread finished\n");
#endif
    pthread_exit(0);
    return NULL;
}

/**************************************************************************
 name	: __ftp_read_data_thread
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static void *__ftp_read_data_thread(void *arg)
{
    int32_t recv_size;
    uint32_t flag;
    int32_t sock;
    int32_t len = sizeof(struct sockaddr);
    uint8_t *ptr;
    struct sockaddr_in client;
    uint8_t err;
#ifndef FTP_PASSIVE_MODE
    if (listen(_ftp_client.dsock, 1)<0)
    pthread_exit(&err);
#endif

    while (1) {
        flag = flag_pend(_ftp_client.dflag, FLAG_FTP_DATA_ALL,
            WAIT_FLAG_SET_ANY + FLAG_CONSUME, 0x00, &err);
        if (flag & FLAG_FTP_DATA_READ) {
#ifndef FTP_PASSIVE_MODE
            printf("ACCEPT:%.4X\n",flag);
            if ((sock=accept(_ftp_client.dsock, (struct sockaddr*) &client, &len))<0)
            break;
#else
            sock = _ftp_client.dsock;
#endif
            __ftp_set_data_status(FTP_DSTATUS_IN_PROGRESS);

            memset(_ftp_client.dbuf, 0, CHUNK_DATA_LEN);
            _ftp_client.didx = 0;
            ptr = _ftp_client.dbuf;

            while ((recv_size = recv(sock, ptr, __ftp_read_data_size(), 0)) > 0) {
                _ftp_client.didx += recv_size;
                ptr = _ftp_client.dbuf + _ftp_client.didx;
                if (_ftp_client.data_command == FTP_DATA_COMMAND_FILE_LIST) {
                    if ((_ftp_client.dbuf[_ftp_client.didx - 1] == '\n'
                        && _ftp_client.dbuf[_ftp_client.didx - 2] == '\r') ||
                        (_ftp_client.didx == CHUNK_DATA_LEN)) {
#if 0
                        printf("DATA:%s----\n",_ftp_client.dbuf);
#endif
                        __check_ftp_file_names(_ftp_client.dbuf);

                        memset(_ftp_client.dbuf, 0, CHUNK_DATA_LEN);
                        _ftp_client.didx = 0;
                        ptr = _ftp_client.dbuf;
                    }

                }
                else if (_ftp_client.data_command == FTP_DATA_COMMAND_FILE) {
                    if (_ftp_client.didx == CHUNK_DATA_LEN) {
                    	//:TODO: send data over ftp socket //

                        memset(_ftp_client.dbuf, 0, CHUNK_DATA_LEN);
                        _ftp_client.didx = 0;
                        ptr = _ftp_client.dbuf;
                    }
                }
            }

            if (_ftp_client.data_command == FTP_DATA_COMMAND_FILE) {
            	//:TODO: send data over ftp socket //
                memset(_ftp_client.dbuf, 0, CHUNK_DATA_LEN);
                _ftp_client.didx = 0;
                ptr = _ftp_client.dbuf;
            }

            close(sock);
            __ftp_set_data_status(FTP_DSTATUS_NULL);
            flag_post(_ftp_client.dflag, FLAG_FTP_DATA_OK, OS_FLAG_SET);
        }

        if (flag & FLAG_FTP_DATA_EXIT) {
            break;
        }
    }
#if 0
    printf("__ftp_read_data_thread finished\n");
#endif
    pthread_exit(0);
    return NULL;
}
