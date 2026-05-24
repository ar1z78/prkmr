#include "Platform.h"

#include <winuser.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
#include "MishRoller.h"
#include "resource.h"
#include "CoreUtils.h"
#include "rdb.h"
#include "ImpExp.h"
#include "BuyingAgent.h"
#include <commctrl.h>
#include "globals.h"
#include "missions_UI.h"
#include <windows.h>

// Instantiate the global storage variable block with stable application defaults
PROGRAM_SETTINGS g_Settings = {
	5000,                     // dwWaitTime (4.75 seconds)
	0, 1, 1, 1, 0, 1, 0,     // The 7 base option checkbox default states
	{ 50, 50, 50, 50, 50, 50, 50 }, // Array for Sliders: All 7 set to mid-point (50)
	4,                        // iBuyMod (Trader Shop Multiplier default)
	0, 100000,                // Single value search disabled, default 100k
	0, 100000,                 // Total value search disabled, default 100k
	0, 1,				// Items match in Buying Agent/ Highlight
	0, 1,				// Locations match in Buying Agent/ Highlight
	1, 1, 1, 1, 0,		// Mission types
	0, 1,				// Mission types match in Buying Agent/ Highlight
	1,					// Set Window to stay in top of other windows
	10,                // iMaxTries: Default attempt retry threshold 
	1                   // iMaxMishes: Default rolling success counter target 
};

winpos g_MainWndPos = { 0, 0, 320 };

SEARCH_WATCHLISTS g_WatchLists = { 0 };

#pragma comment(lib, "comctl32.lib")

extern unsigned long g_GUIDef[];

unsigned long  g_MainWin;
HFONT g_hFont = NULL;


void _setSliders(int easy_hard, int good_bad, int order_chaos, int open_hidden, int phys_myst, int headon_stealth, int money_xp);

HWND g_hwndMishBoard = NULL;
MISSION_BOARD_DATA g_MishBoardData = { 0 };


unsigned long g_BuyingAgentCount = 0;
unsigned long g_BuyingAgentMissions = 0;
unsigned long g_bFirstRound = TRUE;
unsigned char g_MishNumber = 0, g_FoundMish = -1;
unsigned char g_bFullscreen = 0;



char g_AODir[MAX_PATH] = { 0 };
char g_MRDir[MAX_PATH] = { 0 };

HANDLE g_Mutex = NULL;
HANDLE g_Thread = NULL;
DWORD WINAPI HookManagerThread(void *pParam);


void CleanUp()
{
	// 1. Safely clear the Thread Handle
	if (g_Thread != NULL && g_Thread != INVALID_HANDLE_VALUE)
	{
		TerminateThread(g_Thread, 0);
		CloseHandle(g_Thread);
		g_Thread = NULL; // Explicitly reset to NULL to avoid double-freeing
	}

	// 2. Safely clear the Mutex Handle
	if (g_Mutex != NULL && g_Mutex != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_Mutex);
		g_Mutex = NULL; // Explicitly reset to NULL
	}

}


// ==================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int argc = __argc;
	char** argv = __argv;
//	void* pMissionData;
//	unsigned long MissionControls[5];
	FILE* fp;
	char AOExePath[256];
	DWORD dwThreadID;
	HANDLE hOrigDB;
	int bUpdateDB = FALSE;
	char DBPath[256 * 2];
	HWND hMainWnd = NULL;       // The old window frame handle
	HWND hwndMishBoard = NULL;  // The NEW window terminal handle running in parallel

	// Set main thread of MishRoller on a priority above normal
	// Helps a lot. Refreshing of missions infos is much faster.
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

	// Get current directory
	GetCurrentDirectory(MAX_PATH, g_MRDir);

	// Import ALL Settings
	ImportSettings("LastSettings.mr");

	// --- DYNAMIC FONT INITIALIZATION BLOCK ---
	HDC hdc = GetDC(NULL); // Get device context for the primary screen
	int dpiY = GetDeviceCaps(hdc, LOGPIXELSY); // Typically 96, 120, 144, or 192
	ReleaseDC(NULL, hdc);

	g_hFont = CreateFontA(
		-MulDiv(g_Settings.iFontSize, dpiY, 72),       // Height (Negative matches cell height in points)
		0, 0, 0,                     // Width and orientation tracking parameters
		g_Settings.bFontBold ? FW_BOLD : FW_NORMAL,                  // BOLD or NORMAL weight value
		FALSE, FALSE, FALSE,         // Italic, Underline, Strikeout
		DEFAULT_CHARSET,             // Character registry mapping
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		g_Settings.szFontName               // Font name
		);
	/* ============================================================================
	INITIALIZE THE NEW MISSIONS WINDOW (MAIN)
	============================================================================ */
	WNDCLASSEXA wcm = { 0 };
	wcm.cbSize = sizeof(WNDCLASSEXA);
	wcm.lpfnWndProc = WndProc; // Hooks to split missions_core_UI.c
	wcm.hInstance = GetModuleHandle(NULL);
	wcm.hCursor = LoadCursor(NULL, IDC_ARROW);
	// Natively forces the main board window background canvas to honor clrBg
	wcm.hbrBackground = CreateSolidBrush((COLORREF)g_Settings.clrBg);
	wcm.lpszClassName = "MishRollerMissionsBoardClass";

	if (RegisterClassExA(&wcm)) {

		// Create the new window as a separate top-level window
		hwndMishBoard = CreateWindowExA(
			0, "MishRollerMissionsBoardClass", "PRK MishRoller by Ar1z",
			WS_OVERLAPPEDWINDOW,
			g_MainWndPos.winX, g_MainWndPos.winY, g_MainWndPos.winW, 685,
			NULL, NULL, GetModuleHandle(NULL), NULL
			);

		if (hwndMishBoard) {
			// FIX: Assign the global cross-reference hook
			g_hwndMishBoard = hwndMishBoard;

			// Apply shell window taskbar icons
			if (hIcon) {
				SendMessage(hwndMishBoard, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
				SendMessage(hwndMishBoard, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			}


		}
	}

	sprintf_s(AOExePath, sizeof(AOExePath), "%s\\anarchy.exe", g_AODir);

	// 1. Allocate a local buffer for the prompt text so it can change
	char promptMessage[256];
	strcpy_s(promptMessage, sizeof(promptMessage), "Please locate the PRK folder, where Anarchy.exe resides.");

	while (!(fp = fopen(AOExePath, "r")))
	{
		GetFolder(NULL, promptMessage, g_AODir);

		// 2. FIXED: Check if the string is empty (User clicked Cancel)
		if (g_AODir[0] == '\0')
		{
			CleanUp();
			return -1;
		}

		sprintf_s(AOExePath, sizeof(AOExePath), "%s\\anarchy.exe", g_AODir);

		// 3. FIXED: Copy the new warning text into the buffer for the next loop
		if (!(fp = fopen(AOExePath, "r")))
		{
			strcpy_s(promptMessage, sizeof(promptMessage), "This is not PRK's directory. It doesn't contain Anarchy.exe. Please try again.");
		}
	}

	fclose(fp);


	// 1. Create Mutex FIRST to isolate and guarantee GetLastError() accuracy
	g_Mutex = CreateMutex(NULL, FALSE, "Ar1zMishRoller");
	DWORD dwMutexError = GetLastError(); // Immediately cache the error status

	if (g_Mutex == NULL)
	{
		ShowErrorMessage("Couldn't create mutex.");

		CleanUp();
		return -1;
	}

	if (dwMutexError == ERROR_ALREADY_EXISTS)
	{
		HWND hWnd = FindWindow("Ar1zMishRollerHookWindowClass", "Ar1zMishRollerHookWindow");
		// Send message to original window if needed
		if (hWnd)
		{
			// If the tool was minimized, restore it
			if (IsIconic(hWnd)) {
				ShowWindow(hWnd, SW_RESTORE);
			}

			// Force the window to the top layer temporarily to break through focus locks
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

			// Bring it to the absolute front of the screen
			SetForegroundWindow(hWnd);

			// If "Stay ON TOP" setting is turned OFF, drop it back to a normal layer
			if (!g_Settings.bGuiOnTop) {
				SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
		}

		ShowErrorMessage("MishRoller is already running.");

		CleanUp();
		return -1;
	}

	// 2. Construct the path to check if rdb.db exists
	sprintf_s(DBPath, sizeof(DBPath), "%s\\cd_image\\rdb.db", g_AODir);
	hOrigDB = CreateFileA(DBPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hOrigDB == INVALID_HANDLE_VALUE) {
		char err[100];
		sprintf_s(err, sizeof(err), "System Error Code: %d \nPath: %s", GetLastError(), DBPath);
		ShowErrorMessage(err);
		CleanUp();
		return -1;
	}
	CloseHandle(hOrigDB);

	// 3. Initialize SQLite and Prepare statements
	if (!OpenLocalDB()) {
		ShowErrorMessage("Couldn't open the AO database (rdb.db).");
		CleanUp();
		return -1;
	}

	// 4. Starts dll hook management thread (Fixed INVALID_HANDLE_VALUE bug to NULL)
	g_Thread = CreateThread(NULL, 0, &HookManagerThread, NULL, 0, &dwThreadID);
	if (g_Thread == NULL)
	{
		ShowErrorMessage("Couldn't create hook thread.");
		ReleaseAODatabase();
		CleanUp();
		return -1;
	}

	// Show the main window 
	if (g_hwndMishBoard != NULL && g_Settings.bStartMinimized) {
		ShowWindow(g_hwndMishBoard, SW_MINIMIZE);
	}
	else if (g_hwndMishBoard != NULL) {
		ShowWindow(g_hwndMishBoard, SW_SHOW);
	}

	if (g_hwndMishBoard != NULL) {
		UpdateWindow(g_hwndMishBoard);

	}

	// 2. Overwrite the background color AFTER ImportSettings
	if (g_hwndMishBoard != NULL) {
		HBRUSH hNewBgBrush = CreateSolidBrush((COLORREF)g_Settings.clrBg);

		// Changes the background brush for the window handle directly in memory
		SetClassLongPtrA(g_hwndMishBoard, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBgBrush);

		// Force the window frame to clear the canvas and paint the new color instantly
		InvalidateRect(g_hwndMishBoard, NULL, TRUE);
		UpdateWindow(g_hwndMishBoard);
	}

	// 3. Recursively push the new font to every child element inside our window frame
	if (g_hwndMishBoard != NULL && g_hFont != NULL) {
		HWND hChild = GetWindow(g_hwndMishBoard, GW_CHILD);
		while (hChild != NULL) {
			// Tell the child control to load the font and force a clean screen layout refresh
			SendMessageW(hChild, WM_SETFONT, (WPARAM)g_hFont, TRUE);
			hChild = GetWindow(hChild, GW_HWNDNEXT);
		}
		// Force the main settings dialog canvas to clear and re-render
		InvalidateRect(g_hwndMishBoard, NULL, TRUE);
		UpdateWindow(g_hwndMishBoard);
	}
	if (g_Settings.bGuiOnTop)
	{
		// Stay On Top is checked: Force window to the top
		SetWindowPos(g_hwndMishBoard, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	WriteDebug(NULL);

	ReleaseAODatabase();
	CleanUp();
	return 0;
}

