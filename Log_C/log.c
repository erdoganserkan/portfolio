#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include "log.h"

#define FILE_NAME_LEN	256
#define MAX_FILE_SIZE	(5UL * 1024UL * 1024UL)
#define DATE_CHAR_LEN		(128)
/***
 * Logging Object Instance Internal resource holding structure
 */
typedef struct {
    uint8_t mutex_init;
    unsigned int logfile_n;
    unsigned int loglevel;
    volatile unsigned int logfile_len;
    FILE * logfile;
    pthread_mutex_t log_mutex;
    char logfile_name[FILE_NAME_LEN];
} mj_log_ts;

static FILE *logfile = NULL;
static char logfile_name[FILE_NAME_LEN];
static unsigned volatile int logfile_len = 0;
static unsigned int logfile_n;
static unsigned int loglevel;
static char VERSION_INFO[FILE_NAME_LEN];

static pthread_mutex_t log_mutex;
static mj_log_ts *log_instances[MAXIMUM_LOG_INTANCEs] = {NULL};

// Function Prototypes //
static int __change_file_instance(mj_log_type log_instance);


/**************************************************************************
 name   : mj_log_free
 purpose    :
 input  : none
 output : none
 ***************************************************************************/
int mj_log_free_instance(mj_log_type log_instance)
{
    if (NULL == log_instances[log_instance]->logfile)
        return -1;

    if (log_instances[log_instance]->logfile == stdout)
        return 0;

    pthread_mutex_lock(&(log_instances[log_instance]->log_mutex));
    fclose(log_instances[log_instance]->logfile);
    log_instances[log_instance]->logfile = stdout;
    pthread_mutex_unlock(&(log_instances[log_instance]->log_mutex));

    return 0;
}

/* close the log file and switch to the other file	*/
/**************************************************************************
 name	: __change_file
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static int __change_file(void)
{
    char file[FILE_NAME_LEN];

    if (logfile == stdout)
        return 0;

    if (logfile_n == 0)
        logfile_n = 1;
    else
        logfile_n = 0;

    snprintf(file, FILE_NAME_LEN, "%s%d.log",
        logfile_name, logfile_n);
    fclose(logfile);
    logfile = fopen(file, "w");
    if (logfile == NULL) {
        fprintf(stderr, "Cannot open %s\r\n", file);
        logfile = stdout;
        return -1;
    }

    logfile_len = 0;

    return 0;
}

/**************************************************************************
 name	: __get_date
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
static char *__get_date(void)
{
    char *ret;
    struct tm date;
    time_t lt;

    ret = (char *)malloc(DATE_CHAR_LEN * sizeof(char));
    lt = time(&lt);
    localtime_r(&lt, &date);
    asctime_r(&date, ret);
    /*	delete the last '\n' character	*/
    ret[strlen(ret) - 1] = '\0';

    return ret;
}

/***
 * --------- Reference @ mj_log.h --------
 * @param level
 * @param filename: only file name, DONT INCLUDE DIRECTORIES or ANY PATH DATA
 * @return
 */
mj_log_type mj_log_init_instance(const int level, const char *filename)
{
    volatile uint8_t indx;
    size_t len;
    char file[FILE_NAME_LEN];

    if(filename == NULL)
        return LOG_INSTANCE_FAILED;

    // Find a free location for new object to create //
    for(indx=0 ; indx<MAXIMUM_LOG_INTANCEs ; indx++)
        if(NULL == log_instances[indx])
            break;
    if(MAXIMUM_LOG_INTANCEs == indx) {
        fprintf(stderr, "There is NOT free resource for new log_instance\n");
        return LOG_INSTANCE_FAILED;
    }

    // Initialize instance object //
    log_instances[indx] = calloc(1, sizeof(mj_log_ts));
    if(NULL == log_instances[indx]) {
        fprintf(stderr, "Cannot create logfile_instance(%s)\r\n", strerror(errno));
        return LOG_INSTANCE_FAILED;
    }

    len = strlen(filename);
    /*  set log level   */
    mj_change_log_level_instance((mj_log_type)indx, level);
    //VERSION_INFO[0] = '\0';

    /*  control file name   */
    if (len > FILE_NAME_LEN) {
        fprintf(stderr, "Cannot init logfile_name\r\n");
        log_instances[indx]->logfile = stdout;
        free(log_instances[indx]);
        log_instances[indx] = NULL;
        return LOG_INSTANCE_FAILED;
    }
    else if (len == 0) {
        fprintf(stdout, "Using stdout instead of file\r\n");
        log_instances[indx]->logfile = stdout;
        return (mj_log_type)indx;
    }

    /*  copy log file name and create   */
    strncpy(log_instances[indx]->logfile_name, filename, len);
    snprintf(file, FILE_NAME_LEN, "%s0.log", log_instances[indx]->logfile_name);
    log_instances[indx]->logfile_n = 0;

    /*  open log file for writing   */
    log_instances[indx]->logfile = fopen(file, "w");
    if (log_instances[indx]->logfile == NULL) {
        fprintf(stderr, "Cannot open %s : using stdout\r\n", file);
        log_instances[indx]->logfile = stdout;
        return (mj_log_type)indx;
    }

    if (!log_instances[indx]->mutex_init)
        pthread_mutex_init(&(log_instances[indx]->log_mutex), NULL);
    log_instances[indx]->mutex_init=1;
    mj_log_instance(indx, FLOG_INFO, "mj_log_init SUCCESSed\r\n");

    return (mj_log_type)indx;
}


/***
 * --------- Reference @ mj_log.h --------
 * @param log_instance
 * @param level
 * @return
 */
int mj_change_log_level_instance(mj_log_type log_instance, const int level)
{
    if(NULL == log_instances[(int)log_instance])
        return -1;
    log_instances[log_instance]->loglevel = level;

    return 0;
}


/***
 * Changing the logging output file to another one
 * @param log_instance : The logging instance handle
 * @return SUCCESS:if everthing is OK, FAILURE: if error occurs
 */
static int __change_file_instance(mj_log_type log_instance)
{
    char file[FILE_NAME_LEN];

    if(NULL == log_instances[log_instance])
        return -1;
    if (log_instances[log_instance]->logfile == stdout)
        return 0;

    if (log_instances[log_instance]->logfile_n == 0)
        log_instances[log_instance]->logfile_n = 1;
    else
        log_instances[log_instance]->logfile_n = 0;

    snprintf(file, FILE_NAME_LEN, "%s%d.log",
        log_instances[log_instance]->logfile_name, log_instances[log_instance]->logfile_n);
    fclose(log_instances[log_instance]->logfile);
    log_instances[log_instance]->logfile = fopen(file, "w");
    if (NULL == log_instances[log_instance]->logfile) {
        fprintf(stderr, "Cannot open %s\r\n", file);
        log_instances[log_instance]->logfile = stdout;
        return -1;
    }

    log_instances[log_instance]->logfile_len = 0;

    return 0;
}


/***
 * ----- Reference @ mj_log.h -----
 * @param log_instance
 * @param str
 * @param file
 * @param line
 * @param level
 * @param fmt
 * @return
 */
int mj_log_instance(mj_log_type log_instance, const char *str, const char *file, const int line,
        const int level, const char *fmt, ...)
{
    va_list args;
    char *date;

    if(NULL == log_instances[log_instance])
        return -1;

    if (NULL == log_instances[log_instance]->logfile)
        return log_instances[log_instance]->logfile_len;

    if (level > log_instances[log_instance]->loglevel)
        return 0;

    date = __get_date();
    log_instances[log_instance]->logfile_len += \
        fprintf(log_instances[log_instance]->logfile, "%s ", date);
    free(date);
    log_instances[log_instance]->logfile_len += \
        fprintf(log_instances[log_instance]->logfile, "%s%s:%d ", str, file, line);

    va_start(args, fmt);
    log_instances[log_instance]->logfile_len += \
        vfprintf(log_instances[log_instance]->logfile, fmt, args);
    va_end(args);

    fflush(log_instances[log_instance]->logfile);
    pthread_mutex_lock(&(log_instances[log_instance]->log_mutex));
    if (log_instances[log_instance]->logfile_len >= MAX_FILE_SIZE)
        __change_file_instance(log_instance);
    pthread_mutex_unlock(&(log_instances[log_instance]->log_mutex));


    return log_instances[log_instance]->logfile_len;
}
