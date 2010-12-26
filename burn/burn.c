#include <windows.h>
#include <stdio.h>	//printf
#include "V:\WinDDK\7600.16385.1\inc\api\winioctl.h"

int
__cdecl
main(int argc,__nullterminated char *argv[])
{
	HANDLE fileHandle = NULL;
	ULONG	errorCode = 0;
//	StringCbPrintf(string, sizeof(string), "\\\\.\\%s", argv[1]);
	fileHandle = CreateFile("\\\\.\\F:", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	printf("Error opening. Error: %d\n", fileHandle);
	CloseHandle(fileHandle);
}