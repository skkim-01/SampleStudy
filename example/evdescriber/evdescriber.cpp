#include "pch.h"

#include <locale.h>

#include "EventLoader.h"

// Event log types
int main()
{
	// https://docs.microsoft.com/en-us/windows/win32/eventlog/event-sources
	wchar_t wstrApplication[] = L"Application";
	wchar_t wstrSecurity[] = L"Security";
	wchar_t wstrSystem[] = L"System";

	setlocale(LC_ALL, "ko_KR.UTF-8");
	_wsetlocale(LC_ALL, L"ko_KR.UTF-8");
	
	// Retrive All Event logs
	EventLogReader(wstrApplication);

	return 0;
}