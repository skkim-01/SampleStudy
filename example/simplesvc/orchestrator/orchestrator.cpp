#include "orchestrator.h"

#include <Windows.h>

#include "logger/dailylogger.h"
#include "util/stringutil.h"

long _Orchestrator::_init()
{
	long		lError = ERROR_SUCCESS;

	do {
		// start log manager
		lError = DailyLogger::GetInstance()->_init();
		if (ERROR_SUCCESS != lError)		break;

		DailyLogger::GetInstance()->Write("=============================");
		DailyLogger::GetInstance()->Write(" start sample service");		
		DailyLogger::GetInstance()->Write("  version : 1.0.0 ");
		DailyLogger::GetInstance()->Write("  release : 2022.03.10");
		DailyLogger::GetInstance()->Write("=============================");

	} while (false);

	return lError;
}

long _Orchestrator::_uninit()
{
	long		lError = ERROR_SUCCESS;

	do {
		// stop log manager
		DailyLogger::GetInstance()->Write("[fin] service close...");
		lError = DailyLogger::GetInstance()->_uninit();
	} while (false);

	return lError;
}
