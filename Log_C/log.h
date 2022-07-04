#ifndef mj_LOG_H
#define mj_LOG_H

/**	log levels	*/
enum LOG_LEVEL
{
    LOG_ERR     = 0,
    LOG_WARN    = 1,
    LOG_INFO    = 2,
    LOG_DBG     = 3,
    LOG_TRACE   = 4,

	LOG_BIGGEST_LEVEL = LOG_TRACE,
	LOG_LEVEL_COUNT
};

/** error conditions */
#define	FLOG_ERR		"ERR  ", __FILE__, __LINE__, LOG_ERR
/** warning conditions */
#define	FLOG_WARN		"WARN ", __FILE__, __LINE__, LOG_WARN
/** informational */
#define	FLOG_INFO		"INFO ", __FILE__, __LINE__, LOG_INFO
/** debug-level messages */
#define	FLOG_DBG		"DBG  ", __FILE__, __LINE__, LOG_DBG
/** trace-level messages */
#define	FLOG_TRACE		"TRACE", __FILE__, __LINE__, LOG_TRACE

#define LOG_INSTANCE_FAILED         (-1)    /* Log instance creation FAILED */
#define LOG_INSTANCE_NA		        LOG_INSTANCE_FAILED
#define MAXIMUM_LOG_INTANCEs        (30)

typedef int mj_log_type; /* Main handle for log instance based logging */

#define ERRL(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_ERR, "" fmt, ##args); \
}while(0);

#define INFOL(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_INFO, "" fmt, ##args); \
}while(0);

#define DEBUGL(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_DBG, "" fmt, ##args); \
}while(0);

#define WARNL(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_WARN, "" fmt, ##args); \
}while(0);

#define TRACEL(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_TRACE, "" fmt, ##args); \
}while(0);


#define ERRL2(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_ERR, "" fmt, ##args); \
    else \
        mj_log(FLOG_ERR, "" fmt, ##args); \
}while(0);

#define INFOL2(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_INFO, "" fmt, ##args); \
    else \
        mj_log(FLOG_INFO, "" fmt, ##args); \
}while(0);

#define DEBUGL2(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_DBG, "" fmt, ##args); \
    else \
        mj_log(FLOG_DBG, "" fmt, ##args); \
}while(0);

#define WARNL2(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_WARN, "" fmt, ##args); \
    else \
        mj_log(FLOG_WARN, "" fmt, ##args); \
}while(0);

#define TRACEL2(log_handle, fmt, args...)             \
do{\
    if(LOG_INSTANCE_FAILED != log_handle) \
        mj_log_instance(log_handle, FLOG_TRACE, "" fmt, ##args); \
    else \
        mj_log(FLOG_TRACE, "" fmt, ##args); \
}while(0);



#define ON_FAILURE(log_handle, err_str, jump_point) \
do{ \
    ERRL(log_handle, "%s", err_str); \
    goto jump_point;\
}while(0);

#define IF_FAILURE(check_case, log_handle, err_str, jump_point) \
do{ \
    if(FALSE == (check_case)) { \
    	if(NULL != err_str) { \
    		ERRL(log_handle, "%s", err_str); \
    	} \
    	goto jump_point;\
    } \
}while(0);


/**	mj_log_free. closes all log elements, stop logging on file
 */
int mj_log_free_instance(mj_log_type log_instance);


/***
 * Intialization of new log interface
 * @param level : New default log level to filter logging requests
 * @param filename : New log file for output of all logging requests
 * @return NULL: if fails, the handle of new log device o.w.
 */
mj_log_type mj_log_init_instance(const int level, const char *filename);


/***
 * Changes the filtering log level of logging instance previously created
 * @param log_instance : The logging instance handle address
 * @param level : New filtering log level desired to be applied
 * @return
 */
int mj_change_log_level_instance(mj_log_type log_instance, const int level);


/***
 * Logging desired strings on the logging output of and instance
 * @param log_instance : The log instance handle
 * @param str Desired characters to add to log
 * @param file : Filename to add to logging line output
 * @param line : Line number to add to log
 * @param level : logging level of current logging request
 * @param fmt : Logging format for current logging request
 * @return -1: if fails, 0: if logging request ignored, "logging file length in bytes" if everthing is OK
 */
int mj_log_instance(mj_log_type log_instance, const char *str, const char *file, const int line,
        const int level, const char *fmt, ...);

#endif
