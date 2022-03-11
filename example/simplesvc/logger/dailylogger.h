#pragma once

#include <stdio.h>
#include <string>

class DailyLogger
{
public:
	DailyLogger();
	virtual ~DailyLogger();

	// instance
private:
	static DailyLogger* s_pInstance;
public:
	static DailyLogger* GetInstance();

	// control
public:
	long					_init();
	long					_uninit();

	// functions
private:
	std::string				GetLogPath();
	std::string				GetLogFile();
	FILE* 					GetLogFilePtr();

public:
	void					Write(const char* szArg, ...);
};