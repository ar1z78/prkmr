#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <windows.h>
#include <stdio.h> // For FILE, sprintf, fprintf
#include <stdarg.h> // For va_list

#ifdef __cplusplus
extern "C" {
#endif

	// Original function
	void ShowErrorMessage(const char* message);
	void ShowInfoMessage(const char* message);

	// Moved from mission.c
	long FindStr(unsigned char *a_xBuf, unsigned long lBufLen, unsigned char *a_xFind, unsigned long lFindLen);
	// Moved from MishRoller.c
	void DebugPacket(void* pData, unsigned int length);
	void WriteLog(const char* Format, ...);
	void WriteDebug(const char* txt);
	void GetFolder(HWND hWndOwner, char *strTitle, char *strPath);
	BOOL GetFile(HWND hWndOwner, BOOL saving, char* buffer, int buffersize);

#ifdef __cplusplus
}
#endif

#endif // CORE_UTILS_H
