#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "logger.h"

eLogLevel_t Logger::GlobalLogLevel = ALL_LOG_LEVEL;

using namespace std;

bool Logger::is_ok() const {
	return oFile.is_open();
}

Logger::Logger(const char *Path, eLogLevel_t ObjTh)
{
	// If object log level is disabled @ runtime return immediately //
	ObjLogThreshold = ObjTh;
	if(ObjLogThreshold == NO_LOG_LEVEL)
		return;

	string fname{Path};
	fname += ".log";
	oFile.open(fname);
	if(oFile.is_open())
		DEBUGL(this, "### LOG FILE IS BEING OPENED ###\n");
}

Logger::~Logger()
{
	// Because of if object logging is disabled, there is no initialization so
	// there is no need to do anything //
	if(ObjLogThreshold == NO_LOG_LEVEL)
		return;
	if(oFile.is_open()) {
		DEBUGL(this, "### LOG FILE IS BEING CLOSED ###\n");
		oFile.close();
	}
}


void Logger::SetGlobalLogLevel(eLogLevel_t GLogLevel)
{
	Logger::GlobalLogLevel = GLogLevel;
}


void Logger::Log(eLogLevel_t LogLevel, const char *str, const char *files, const int lines,const char *pszFormat, ...)
{
//	printf("LogLevle(%d), str(%s), file(%s), lines(%d), pszFormat(%s)\n", LogLevel, str, files, lines,pszFormat);
	// Return immediately if log is disabled @ runtime or requested log is lower than threshold //
	if((ObjLogThreshold == NO_LOG_LEVEL) || (ObjLogThreshold > LogLevel) || (LogLevel < GlobalLogLevel))
		return;
	if(oFile.is_open()) {
		va_list args;
		va_start(args, pszFormat);

		char buffer[512];
		memset(buffer,0,sizeof(buffer));
		const int nLen = vsnprintf(buffer, sizeof(buffer), pszFormat, args);

		if(0 < nLen) {

			struct timeval tv;
			struct tm* ptm;
			char tstamp[128];
			memset(tstamp,0,sizeof(tstamp));
			gettimeofday (&tv, NULL);
			ptm = localtime (&tv.tv_sec);
			strftime (tstamp, sizeof (tstamp), "%H:%M:%S", ptm);

			std::stringstream usecStrStream;
			usecStrStream << std::to_string(tv.tv_usec/1000);

			string cstr{str};
			string file{files};
			string line{std::to_string(lines)};

			string output {cstr + "#" + file + ":" + line + "_" + string{tstamp} + "." + usecStrStream.str() + ":" + string{buffer}};

			oFile << output;
			oFile.flush();
		}

		va_end(args);
	}
}
