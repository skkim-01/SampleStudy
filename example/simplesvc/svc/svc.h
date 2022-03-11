#pragma once

#include <Windows.h>

// function pre-define
VOID		ServiceEntry(int argc, char* argv[]);
VOID WINAPI SvcMain(int argc, char* argv[]);
VOID		SvcInit(int argc, char* argv[]);

VOID		ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl);

VOID __stdcall _Install(void);
VOID __stdcall _Start(void);
VOID __stdcall _Stop(void);
VOID __stdcall _Delete(void); 
BOOL __stdcall StopDependentServices();

// Start of CI Translator program!
void		_init();

// End of CI Translator program!
void		_fin();
