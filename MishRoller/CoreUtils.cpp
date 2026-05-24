#include "globals.h"
#include "MishRoller.h"
#include "CoreUtils.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <shlobj.h> // Required for SHBrowseForFolder


extern "C" {
	void ShowErrorMessage(const char* message)
	{
		MessageBoxA(NULL, message, "MishRoller Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
	}

	void ShowInfoMessage(const char* message)
	{
		MessageBoxA(NULL, message, "MishRoller Info", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
	}

	long FindStr(unsigned char *a_xBuf, unsigned long lBufLen, unsigned char *a_xFind, unsigned long lFindLen)
	{
		long lLoop;
		long lMax;
		unsigned char *a_xBufPtr;

		a_xBufPtr = a_xBuf;
		lMax = (long)lBufLen - (long)lFindLen;
		for (lLoop = 0; lLoop <= lMax; lLoop += 1)
		{
			if (memcmp(a_xBufPtr, a_xFind, lFindLen) == 0)
			{
				break;
			}
			a_xBufPtr += 1;
		}
		if (lLoop > lMax)
		{
			return -1;
		}
		else
		{
			return lLoop;
		}
	}
}

void DebugPacket(void* pData, unsigned int length)
{
	unsigned int x;
	unsigned char *data = (unsigned char *)pData; // Fixed cast to unsigned char*
	char ps[70];
	for (x = 0; x < length; x++)
	{
		sprintf(&(ps[x % 16 * 3]), "%02X", data[x]);
		ps[x % 16 * 3 + 2] = ' ';
		ps[x % 16 + 48] = (data[x] >= 32 && data[x] <= 127 ? data[x] : '.');
		ps[x % 16 + 49] = '\n';
		ps[x % 16 + 50] = 0;
		if (x % 16 == 15) WriteDebug(ps);
	}

	if (x % 16 != 0)
	{
		for (x = x % 16; x < 16; x++)
		{
			sprintf(&(ps[x % 16 * 3]), "  ");
			ps[x % 16 * 3 + 2] = ' ';
		}
		WriteDebug(ps);
	}
}


void WriteLog(const char* Format, ...)
{
	/**/
	va_list argptr;
	static FILE *fp = NULL;
	if (Format == NULL)
	{
		if (fp)
		{
			fclose(fp);
			fp = NULL;
		}
		return;
	}
	if (g_Settings.bLogging)
	{
		if (!fp)
		{
			fp = fopen("MishRoller.log", "a");
		}
		va_start(argptr, Format);
		vfprintf(fp, Format, argptr);
		va_end(argptr);
	}
	/**/
}


void WriteDebug(const char* txt)
{
#ifdef _DEBUG
	static FILE *fp = NULL;
	if (txt == NULL)
	{
		if (fp)
		{
			fclose(fp);
			fp = NULL;
		}
		return;
	}
	if (!fp)
	{
		fp = fopen("MishRoller.debug", "a");
	}
	fprintf(fp, "%s", txt);
#endif // _DEBUG
}

/* Prompt user for folder
(from AOMD)
*/
void GetFolder(HWND hWndOwner, char *strTitle, char *strPath)
{
	BROWSEINFO udtBI;
	ITEMIDLIST *udtIDList;

	/* Initialise */
	udtBI.hwndOwner = hWndOwner;
	udtBI.pidlRoot = NULL;
	udtBI.pszDisplayName = NULL;
	udtBI.lpszTitle = strTitle;
	udtBI.ulFlags = BIF_RETURNONLYFSDIRS;
	udtBI.lpfn = NULL;
	udtBI.lParam = 0;
	udtBI.iImage = 0;

	/* Prompt user for folder */
	udtIDList = SHBrowseForFolder(&udtBI);

	if (udtIDList != NULL)
	{
		/* Extract pathname */
		if (!SHGetPathFromIDList(udtIDList, strPath))
		{
			strPath[0] = 0; // Zero-length if failure
		}

		/* Free the memory allocated by shell */
		CoTaskMemFree(udtIDList);
	}
	else
	{
		strPath[0] = 0; // User clicked Cancel
	}
}



BOOL GetFile(HWND hWndOwner, BOOL saving, char* buffer, int buffersize)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));

	/* Initialise */
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWndOwner;
	if (saving)
	{
		ofn.Flags = OFN_HIDEREADONLY;
	}
	else
	{
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	}

	ofn.lpstrFilter = "MishRoller Files\0*.MR\0";
	ofn.lpstrFile = buffer;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = buffersize;
	ofn.nFilterIndex = 0;
	ofn.lpstrInitialDir = ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;

	/* Prompt user for folder */
	if (saving)
	{
		return GetSaveFileName(&ofn);
	}
	else
	{
		return GetOpenFileName(&ofn);
	}

	return FALSE;
}
