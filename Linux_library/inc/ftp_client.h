#include <common.h>

#ifndef FTP_CLIENT_H_
#define FTP_CLIENT_H_

#define FTP_PASSIVE_MODE

#define FTP_AUTH_USER	0x00
#define FTP_AUTH_PASS	0x01

#define FLAG_FTP_DATA_ALL	0x03
#define FLAG_FTP_DATA_READ	0x01
#define FLAG_FTP_DATA_EXIT	0x02
#define FLAG_FTP_DATA_OK	0x04

#define FLAG_FTP_COMMAND_ALL		0x03
#define FLAG_FTP_COMMAND_OK			0x01
#define FLAG_FTP_COMMAND_EXIT		0x02

typedef enum ftp_data_command_list_e
{
    FTP_DATA_COMMAND_FILE_LIST = 1,
    FTP_DATA_COMMAND_FILE
} ftp_data_command_list_t;

typedef enum ftp_command_list_e
{
    FTP_COMMAND_GET_FILE_LIST = 1,
    FTP_COMMAND_GET_FILE,
    FTP_COMMAND_QUIT,
    FTP_COMMAND_MODE,
    FTP_COMMAND_TYPE,
    FTP_COMMAND_SYST,
    FTP_COMMAND_FEAT,
    FTP_COMMAND_PWD,
    FTP_COMMAND_PASV
} ftp_command_list_t;

extern uint8_t FTP_SESSION_MESSAGE[5][FTP_MSG_LENGTH];
extern uint8_t FTP_COMMAND_MESSAGE[6][FTP_MSG_LENGTH];

typedef struct ftp_client_s
{
    int32_t sock; /* CCMTP socket connection*/
    int32_t dsock; /* data socket*/
    int32_t csock; /* communication socket*/
    uint8_t dtype;
    uint8_t ctype;
    flag_grp_t *dflag;
    flag_grp_t *cflag;
    uint32_t lport;
    uint8_t command;
    uint8_t dbuf[CHUNK_DATA_LEN];
    uint32_t didx;
    uint8_t cbuf[CHUNK_DATA_LEN];
    uint32_t cidx;
    uint8_t session;
    uint8_t dstatus;
    uint8_t cstatus;
    ftp_config_t config;
    uint8_t data_command;
    uint8_t file_name[MAX_FILENAME_LENGTH + 1];
} ftp_client_t;

typedef struct ftp_filenames_s
{
    uint32_t count;
    uint8_t name[MAX_FILE_LIMIT][MAX_FILENAME_LENGTH + 1];
    uint32_t size[MAX_FILE_LIMIT];

} ftp_filenames_t;

void ftp_init(void *callback1, void *callback2);
int8_t ftp_session_init(int32_t sock, ftp_config_t *ftp);
void ftp_data_socket_close(void);
void ftp_session_close(int32_t sock);
uint8_t ftp_get_file_list(ftp_filenames_t *ftp);
uint8_t ftp_file_size(uint8_t *file_name, uint32_t *file_size);
void ftp_get_file(uint8_t *file_name);
uint8_t ftp_get_session_status(void);

#endif /* FTP_CLIENT_H_ */
