#ifndef LOGGER_H_
#define LOGGER_H_

#include <sys/time.h>
#include <stdarg.h>
#include <string.h>

#include <iostream>
#include <fstream>

//######## definitions of log levels ########
typedef enum
{
	ALL_LOG_LEVEL		= -1,
	TRACE_LEVEL		= 0x0,			// Paranoya Log Level
	DEBUG_LEVEL		= 0x1,			// Just Debug Messages
	INFO_LEVEL		= 0x2,			// Just Info Messages
	ERROR_LEVEL		= 0x3,			// Just Error Messages
	ALERT_LEVEL		= 0x4,			// Very Important Alert Messages
	FATAL_LEVEL		= 0x5,			// App FATAL Errors

	NO_LOG_LEVEL		= 10
}eLogLevel_t;

/** error conditions */
#define	ERR_S			"ERR  ", __FILE__, __LINE__
/** warning conditions */
#define	WARN_S		"WARN ", __FILE__, __LINE__
/** informational */
#define	INFO_S		"INFO ", __FILE__, __LINE__
/** debug-level messages */
#define	DBG_S		"DBG  ", __FILE__, __LINE__
/** trace-level messages */
#define	TRACE_S		"TRACE", __FILE__, __LINE__

#define ERRL(logi, fmt, args...)             \
do{\
    if(logi->is_ok()) \
        logi->Log(ERROR_LEVEL, ERR_S, "" fmt, ##args); \
}while(0);

#define INFOL(logi, fmt, args...)             \
do{\
    if(logi->is_ok()) \
        logi->Log(INFO_LEVEL, INFO_S, "" fmt, ##args); \
}while(0);

#define DEBUGL(logi, fmt, args...)             \
do{\
    if(logi->is_ok()) \
        logi->Log(DEBUG_LEVEL, DBG_S, "" fmt, ##args); \
}while(0);

#define WARNL(logi, fmt, args...)             \
do{\
    if(logi->is_ok()) \
        logi->Log(WARN_LEVEL, WARN_S, "" fmt, ##args); \
}while(0);

#define TRACEL(logi, fmt, args...)             \
do{\
    if(logi->is_ok()) \
        logi->Log(TRACE_LEVEL, TRACE_S, "" fmt, ##args); \
}while(0);

//:TODO: Add a mutex for protecting the global-log-level variable //

class Logger
{
private:

	static eLogLevel_t GlobalLogLevel;

	std::ofstream oFile;

	volatile eLogLevel_t ObjLogThreshold;
public:

	void LogOpen(void) ;
	void LogClose();
	void Log(eLogLevel_t LogLevel,const char *str, const char *file, const int line,const char *pszFormat, ...);
	void SetGlobalLogLevel(eLogLevel_t GLogLevel);
	bool is_ok() const;

	Logger(const char *Path, eLogLevel_t ObjTh=ALL_LOG_LEVEL);
	~Logger();

};

#endif
