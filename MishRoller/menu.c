#include <windows.h>
#include "resource.h"
#include "globals.h"

// 1. Allocate the actual, concrete global physical structures right here
MenuStringData mnuFile = { "&File" };
MenuStringData mnuWindows = { "&Windows" };
MenuStringData mnuExit = { "E&xit" };
MenuStringData mnuSearch = { "&Search..." };
MenuStringData mnuSettings = { "Se&ttings..." };

// External window generation callbacks housed inside MishRoller.c
void CreateSearchWindow(HWND hwndParent);
void CreateSettingsWindow(HWND hwndParent);

void ShowSearchWindow(HWND hwndParent) {
	CreateSearchWindow(hwndParent);
}

void ShowSettingsWindow(HWND hwndParent) {
	CreateSettingsWindow(hwndParent);
}

void CreateMainMenu(HWND hwnd) {
	HMENU hMenu = CreateMenu();
	HMENU hFileMenu = CreatePopupMenu();
	HMENU hWindowMenu = CreatePopupMenu();

	// 1. Build Dropdown Items (Owner-Drawn natively from birth)
	AppendMenuA(hFileMenu, MF_OWNERDRAW, ID_FILE_EXIT, (LPCSTR)&mnuExit);
	AppendMenuA(hWindowMenu, MF_OWNERDRAW, ID_WINDOWS_SEARCH, (LPCSTR)&mnuSearch);
	AppendMenuA(hWindowMenu, MF_OWNERDRAW, ID_WINDOWS_SETTINGS, (LPCSTR)&mnuSettings);

	// 2 & 3. FIXED: Create top-level items directly as MFT_OWNERDRAW with attached popups
	MENUITEMINFOA miiTop = { 0 };
	miiTop.cbSize = sizeof(MENUITEMINFOA);
	miiTop.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_SUBMENU | MIIM_ID;
	miiTop.fType = MFT_OWNERDRAW;

	// Inject File Menu (Index position 0 on the bar)
	miiTop.wID = 50001; // Unique ID so Windows doesn't conflict
	miiTop.hSubMenu = hFileMenu;
	miiTop.dwItemData = (ULONG_PTR)&mnuFile;
	InsertMenuItemA(hMenu, 0, TRUE, &miiTop);

	// Inject Windows Menu (Index position 1 on the bar)
	miiTop.wID = 50002; // Unique ID so Windows doesn't conflict
	miiTop.hSubMenu = hWindowMenu;
	miiTop.dwItemData = (ULONG_PTR)&mnuWindows;
	InsertMenuItemA(hMenu, 1, TRUE, &miiTop);

	// 4. Attach the fully initialized canvas bar to window handle frame
	SetMenu(hwnd, hMenu);

	// Set background color configurations natively for the main bar frame canvas
	MENUINFO mi = { 0 };
	HBRUSH hMenuBarBrush = CreateSolidBrush((COLORREF)g_Settings.clrBg);
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
	mi.hbrBack = hMenuBarBrush;
	SetMenuInfo(hMenu, &mi);
}

