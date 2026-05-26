#include "Platform.h"

#include <stdio.h>
#include <cmath>
#include <string>
#include "MishRoller.h"
#include "CoreUtils.h"
#include "Globals.h"

extern "C" HANDLE g_Mutex;
extern "C" char g_MRDir[ MAX_PATH ];
extern "C" char g_CurrentPacket[ 65536 ];


std::string to_ascii_copy( std::wstring const& input )
{
    int len_buffer = input.length() + 1;
    char* abuffer = (char*)malloc( len_buffer );
    ZeroMemory( abuffer, len_buffer );
    WideCharToMultiByte( CP_ACP, NULL, input.c_str(), input.length(), abuffer, len_buffer, NULL, NULL );
    std::string result( abuffer );
    free( abuffer );
    return result;
}


inline std::string to_ascii_copy( std::string const& input )
{
    return input;
}


bool InjectDLL(DWORD ProcessID, std::string const& dllName)
{
	if (!ProcessID)
	{
		//LOG("No process ID specified.");
		return false;
	}
	//LOG("Attempting to inject '" << dllName << "' into process " << ProcessID);
	// Clear error status.
	SetLastError(0);
	HANDLE Proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);
	if (!Proc)
	{
		//LOG("Failed to open AO client process. Error: " << GetLastError());
		return false;
	}

	std::string dllNameA = to_ascii_copy(dllName);

	// FIX 1: Allocate space including the null terminator (+1)
	SIZE_T totalLength = dllNameA.length() + 1;
	LPVOID RemoteString = (LPVOID)VirtualAllocEx(Proc, NULL, totalLength, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!RemoteString)
	{
		//LOG("Failed to allocate memory in AO client. Error: " << GetLastError());
		CloseHandle(Proc);
		return false;
	}

	// FIX 2: Write the string including the null terminator
	if (!WriteProcessMemory(Proc, (LPVOID)RemoteString, dllNameA.c_str(), totalLength, NULL))
	{
		//LOG("Failed to send DLL name to AO client. Error: " << GetLastError());
		VirtualFreeEx(Proc, RemoteString, 0, MEM_RELEASE);
		CloseHandle(Proc);
		return false;
	}

	LPVOID LoadLibAddy = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	HANDLE hThread = CreateRemoteThread(Proc, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddy, (LPVOID)RemoteString, NULL, NULL);
	if (!hThread)
	{
		//LOG("Failed to start hook in AO client. Error: " << GetLastError());
		VirtualFreeEx(Proc, RemoteString, 0, MEM_RELEASE);
		CloseHandle(Proc);
		return false;
	}

	// FIX 3: Wait for LoadLibraryA to finish executing inside the game client
	WaitForSingleObject(hThread, INFINITE);

	// FIX 4: Check the return value of LoadLibraryA (the thread's exit code)
	DWORD hLibModule = 0;
	GetExitCodeThread(hThread, &hLibModule);

	// Clean up memory and handles
	CloseHandle(hThread);
	VirtualFreeEx(Proc, RemoteString, 0, MEM_RELEASE);
	CloseHandle(Proc);

	// If LoadLibraryA returned NULL (0), the DLL failed to load due to corruption or missing dependencies
	if (hLibModule == 0)
	{
		return false;
	}

	return true;
}


void Inject()
{
	// Tracks if we already alerted the user about an injection failure
	static bool s_AlreadyFailed = false;

	HWND AOWnd = FindWindow("Anarchy client", NULL);//HANDLE AOProcessHnd;
	if (AOWnd)
	{
		DWORD AOProcessId;
		char Temp[MAX_PATH];

		GetWindowThreadProcessId(AOWnd, &AOProcessId);// Get process id
		sprintf(Temp, "%s\\AOHook.dll", g_MRDir);

		if (!InjectDLL(AOProcessId, Temp))
		{
			// Only show the alert if we haven't already flagged a failure
			if (!s_AlreadyFailed)
			{
				ShowErrorMessage("DLL Injection failed. Check if AOHook.dll is missing.");
				s_AlreadyFailed = true; // Block future pop-ups
			}
		}
		else
		{
			// Reset the flag if injection succeeds (e.g., if the game restarts)
			s_AlreadyFailed = false;
		}
	}
}

LRESULT CALLBACK HookWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    COPYDATASTRUCT* pData;

    switch( msg )
    {
    case WM_TIMER:
        Inject();
        break;
    case WM_COPYDATA:
        pData = (PCOPYDATASTRUCT)lParam;
        WaitForSingleObject( g_Mutex, INFINITE );
        memset( g_CurrentPacket, 0, 65536 );
        memcpy( g_CurrentPacket, pData->lpData, pData->cbData );
        ReleaseMutex( g_Mutex );
//fix        puSendAppMessage( MRAM_NEWMISSIONS, 0 );
		PostMessage(g_hwndMishBoard, MRAM_NEWMISSIONS, 0, 0);

        break;
    default:
        return DefWindowProc( hWnd, msg, wParam, lParam );
    }
    return 0;
}


extern "C" DWORD WINAPI HookManagerThread( void* _pParam )
{
    WNDCLASSEX HookWndClass;
    HWND hWnd;
    int RetCode;
    MSG msg;
    UINT Timer;

    //SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

    HookWndClass.cbSize = sizeof( WNDCLASSEX );
    HookWndClass.style = 0;
    HookWndClass.lpfnWndProc = HookWndProc;
    HookWndClass.cbClsExtra = 0;
    HookWndClass.cbWndExtra = 0;
    HookWndClass.hInstance = NULL;
    HookWndClass.hIcon = NULL;
    HookWndClass.hCursor = NULL;
    HookWndClass.hbrBackground = NULL;
    HookWndClass.lpszMenuName = NULL;
    HookWndClass.lpszClassName = "Ar1zMishRollerHookWindowClass";
    HookWndClass.hIconSm = NULL;

    RegisterClassEx( &HookWndClass );

    /* Create window */
    hWnd = CreateWindowEx( 0, "Ar1zMishRollerHookWindowClass", "Ar1zMishRollerHookWindow", WS_MINIMIZE, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL );
    Inject();
    Timer = SetTimer( hWnd, 0, 5000, NULL );
    while( RetCode = GetMessage( &msg, NULL, 0, 0 ) )
    {
        if( RetCode != -1 )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }
    KillTimer( hWnd, Timer );
    DestroyWindow( hWnd );
    return TRUE;
}
