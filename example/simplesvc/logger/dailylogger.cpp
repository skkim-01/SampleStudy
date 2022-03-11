#include "DailyLogger.h"
#include "util/StringUtil.h"

#include <Windows.h>
#include <io.h>
#include <direct.h>

#define			LOG_BUFFER_SIZE			4096

DailyLogger* DailyLogger::s_pInstance = NULL;

CRITICAL_SECTION	g_csLog;

DailyLogger::DailyLogger()
{
}

DailyLogger::~DailyLogger()
{
}

DailyLogger* DailyLogger::GetInstance()
{
	if (!s_pInstance)
	{
		s_pInstance = new DailyLogger();
	}
	return s_pInstance;
}

long DailyLogger::_init()
{
	::InitializeCriticalSection(&g_csLog);

	return 0;
}

long DailyLogger::_uninit()
{
	::DeleteCriticalSection(&g_csLog);

	return 0;
}

std::string DailyLogger::GetLogPath()
{
	std::string modulePath = StringUtil::GetModulePath();
	std::string logPath = modulePath + "\\logs";
	return logPath;
}

std::string DailyLogger::GetLogFile()
{
	std::string currentDate = StringUtil::GetCurrentDate();
	std::string logFile = GetLogPath() + "\\samples." + currentDate + ".log";
	return logFile;
}

FILE* DailyLogger::GetLogFilePtr()
{
	std::string logPath = GetLogPath();
	if (0 != _access(logPath.c_str(), 0))
	{
		_mkdir(logPath.c_str());
	}

	FILE* pLog;
	fopen_s(&pLog, GetLogFile().c_str(), "a+");
	return pLog;
}

void DailyLogger::Write(const char* szArg, ...)
{
	char		chBuffer[LOG_BUFFER_SIZE] = { NULL, };
	int 		nRet = 0;
	va_list 	sArgs;

	va_start(sArgs, szArg);
#ifdef WIN32
	nRet = vsprintf_s(chBuffer, LOG_BUFFER_SIZE, szArg, sArgs);
#else
	//vsnprintf(szOut, LOG_BUFFER_SIZE, szArg, sArgs);
#endif
	va_end(sArgs);

	std::string curTime = StringUtil::GetCurrentDateTime();

	::EnterCriticalSection(&g_csLog);
	FILE* pLog = GetLogFilePtr();
	fprintf(pLog, "[%s] %s\n", curTime.c_str(), chBuffer);
	fclose(pLog);
	::LeaveCriticalSection(&g_csLog);
}

