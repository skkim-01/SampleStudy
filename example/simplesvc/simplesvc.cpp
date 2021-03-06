// project properties
// 
// c/c++ - 언어 - 준수모드 : 사용 안함
// precompiled header : 사용 안함
// 
// TODO: 링커 - 매니페스트파일 - UAC 실행 수준 : requireAdministrator
// cmd 관리자로 실행하여 테스트 하는중
// 
// Ref
// https://docs.microsoft.com/en-us/windows/win32/services/about-services
// 
//

#include <stdio.h>

#include "svc/svc.h"

int main(int argc, char* argv[])
{
	if (argc == 1)
		argv[1] = "-h"
	
	if (0 == lstrcmpiA(argv[1], "-i"))
	{
		_Install();
		return 0;
	}
	else if (0 == lstrcmpiA(argv[1], "-s"))
	{
		_Start();
		return 0;
	}
	else if (0 == lstrcmpiA(argv[1], "-p"))
	{
		_Stop();
		return 0;
	}
	else if (0 == lstrcmpiA(argv[1], "-d"))
	{
		_Delete();
		return 0;
	}
	else if (0 == lstrcmpiA(argv[1], "-h"))
	{
		printf("[usage]\n");
		printf("\trunning_file.exe [param]\n\n");
		printf("[service install]\n");
		printf("\trunning_file.exe -i\n\n");
		printf("[service start]\n");
		printf("\trunning_file.exe -s\n\n");
		printf("[service stop]\n");
		printf("\trunning_file.exe -p\n\n");
		printf("[service delete]\n");
		printf("\trunning_file.exe -d\n\n");
		printf("[help]\n");
		printf("\trunning_file.exe -h\n\n");
	}

	ServiceEntry(argc, argv);

	return 0;
}
