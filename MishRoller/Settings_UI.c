#include <windows.h>
#include "resource.h"
#include "globals.h"
#include "MishRoller.h"


extern const LPCWSTR g_SettingsUrl;
extern HFONT g_hFont;
LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
int CALLBACK EnumFontFamExProc(const LOGFONTW* lpelfe, const TEXTMETRICW* lpntme, DWORD FontType, LPARAM lParam);
HWND g_hwndSearch = NULL;

void CreateSettingsWindow(HWND hwndParent) {
	HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hwndParent, GWLP_HINSTANCE);
	WNDCLASSEXW wc = { 0 };
	HWND hwndSettings, ctrl;
	int i;
	WCHAR szVersionBuffer[80];

	LPCWSTR optLabels[] = { L"Start minimized", L"Alert box", L"Select Match", L"Sounds", L"Logging", L"Show rolling agent help", L"Auto Expand Team Missions" };
	LPCWSTR sliderLabels[] = { L"Easy/Hard:", L"Good/Bad:", L"Order/Chaos:", L"Open/Hidden:", L"Physical/Mystical:", L"Head On/Stealth:", L"Money/XP:" };
	LPCWSTR colorLabels[] = { L"Background Color:", L"Button Color:", L"Font Color:", L"Match Color:", L"Icon Background:" };

	LPCWSTR warnMsg = L"Choose Mission slider amounts\n0 = full left, 100 = full right\nSliders must be in default positions and the 'More Options' section of the mission window must be OPEN! Press (v) button, otherwise this won't work.";
	LPCWSTR subWarn = L"A match in this section will count as an ITEM MATCH\nMake sure you have those enabled if you want to search for values";

	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = SettingsWndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = L"MishRollerSettingsClass";
	RegisterClassExW(&wc);

	hwndSettings = CreateWindowExW(0, L"MishRollerSettingsClass", L"Settings", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 440, 640, hwndParent, NULL, hInst, NULL);
	if (!hwndSettings) return;

	wsprintfW(szVersionBuffer, L"MishRoller v%hs, by Ar1z for Project RK", MR_VERSION);
	ctrl = CreateWindowExW(0, L"STATIC", szVersionBuffer, WS_CHILD | WS_VISIBLE | SS_CENTER, 5, 8, 415, 14, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	ctrl = CreateWindowExW(0, L"BUTTON", g_SettingsUrl, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 5, 24, 415, 16, hwndSettings, (HMENU)IDC_SET_LINK_URL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	
	/* --- OPTIONS CONTAINER LOOP --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Options", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 45, 200, 175, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	for (i = 0; i < 7; ++i) {
		ctrl = CreateWindowExW(0, L"BUTTON", optLabels[i], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, 65 + (i * 18), 185, 16, hwndSettings, (HMENU)(INT_PTR)(IDC_SET_CHK_MINIMIZED + i), hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	}
	CheckDlgButton(hwndSettings, IDC_SET_CHK_MINIMIZED, g_Settings.bStartMinimized);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_ALERT, g_Settings.bAlertBox);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_SELECT, g_Settings.bSelectMatch);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_SOUNDS, g_Settings.bSounds);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_LOGGING, g_Settings.bLogging);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_HELP, g_Settings.bShowHelp);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_EXPAND, g_Settings.bAutoExpand);

	ctrl = CreateWindowExW(0, L"STATIC", L"Delay (ms):", WS_CHILD | WS_VISIBLE | SS_LEFT, 18, 195, 75, 14, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	ctrl = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 125, 193, 60, 22, hwndSettings, (HMENU)IDC_SET_EDIT_WAIT, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	SetDlgItemInt(hwndSettings, IDC_SET_EDIT_WAIT, g_Settings.dwWaitTime, FALSE);

	/* --- REFACTORED: FONT & COLORS FRAME --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Font & Colors", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 215, 45, 200, 175, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	/* 1. Draw only the 4 remaining background/utility colors (skipping the Text Color index) */
	for (i = 0; i < 4; ++i) {
		/* Adjusted string array references: 0=Background, 1=Button, 2=Match, 3=Icon Background */
		LPCWSTR customColorLabels[] = { L"Background Color:", L"Button Color:", L"Match Color:", L"Icon Background:" };
		int customColorIDs[] = { IDC_SET_BTN_COLOR_BG, IDC_SET_BTN_COLOR_BTN, IDC_SET_BTN_COLOR_MATCH, IDC_SET_BTN_COLOR_ICONBG };

		ctrl = CreateWindowExW(0, L"STATIC", customColorLabels[i], WS_CHILD | WS_VISIBLE | SS_LEFT, 222, 65 + (i * 22), 150, 14, hwndSettings, NULL, hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

		ctrl = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 350, 62 + (i * 22), 32, 16, hwndSettings, (HMENU)(INT_PTR)customColorIDs[i], hInst, NULL);
	}

	/* 2. Put a single master button at the bottom of the container frame to launch the Font Dialog */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Select Font", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 222, 185, 180, 24, hwndSettings, (HMENU)IDC_SET_BTN_LAUNCH_FONT_DIALOG, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);


	/* --- SLIDERS CONTAINER LOOP --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Mission Sliders", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 230, 195, 155, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	for (i = 0; i < 7; ++i) {
		ctrl = CreateWindowExW(0, L"STATIC", sliderLabels[i], WS_CHILD | WS_VISIBLE | SS_LEFT, 18, 248 + (i * 19), 110, 16, hwndSettings, NULL, hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
		ctrl = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 130, 245 + (i * 19), 65, 20, hwndSettings, (HMENU)(INT_PTR)(IDC_SET_EDIT_EASY + i), hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
		SetDlgItemInt(hwndSettings, IDC_SET_EDIT_EASY + i, g_Settings.Sliders[i], FALSE);
	}

	/* --- IMPORTANT INFO SECTIONS & ACTION CONTROLS --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"IMPORTANT", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 210, 230, 210, 155, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	ctrl = CreateWindowExW(0, L"STATIC", warnMsg, WS_CHILD | WS_VISIBLE | SS_LEFT, 218, 245, 200, 100, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	ctrl = CreateWindowExW(0, L"BUTTON", L"Set Sliders Now", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 215, 355, 200, 24, hwndSettings, (HMENU)IDC_SET_BTN_SLIDERS, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	ctrl = CreateWindowExW(0, L"BUTTON", L"Export Settings", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 10, 390, 200, 24, hwndSettings, (HMENU)IDC_SET_BTN_EXPORT, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	ctrl = CreateWindowExW(0, L"BUTTON", L"Import Settings", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 215, 390, 200, 24, hwndSettings, (HMENU)IDC_SET_BTN_IMPORT, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	/* --- VALUE MANAGEMENT SETTINGS --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Item Value Settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 420, 410, 45, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	ctrl = CreateWindowExW(0, L"STATIC", L"Buy Mod:", WS_CHILD | WS_VISIBLE | SS_LEFT, 18, 442, 55, 14, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	//ctrl = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 75, 439, 20, 20, hwndSettings, (HMENU)IDC_SET_EDIT_BUYMOD, hInst, NULL);
	//SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	// 1. Create the Combo Box (Replaces the EDIT control)
	ctrl = CreateWindowA("COMBOBOX", NULL,
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
		75, 438, 110, 200, // Increased width for text and height for dropdown list
		hwndSettings, (HMENU)IDC_SET_EDIT_BUYMOD, hInst, NULL);

	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	// 2. Add the items to the dropdown list
	SendMessageW(ctrl, CB_ADDSTRING, 0, (LPARAM)L"Clan: 4");
	SendMessageW(ctrl, CB_ADDSTRING, 0, (LPARAM)L"Omni: 5");
	SendMessageW(ctrl, CB_ADDSTRING, 0, (LPARAM)L"Trader Shop: 7");

	// 3. Select the default item based on g_Settings.iBuyMod value
	int defaultIndex = 0; // Default to Clan if no match
	if (g_Settings.iBuyMod == 5) defaultIndex = 1;
	else if (g_Settings.iBuyMod == 7) defaultIndex = 2;

	SendMessageW(ctrl, CB_SETCURSEL, defaultIndex, 0);
	

	//ctrl = CreateWindowExW(0, L"STATIC", L"Clan: 4 / Omni: 5 / Trader Shop: 7", WS_CHILD | WS_VISIBLE | SS_LEFT, 170, 442, 200, 20, hwndSettings, NULL, hInst, NULL);
	//SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	ctrl = CreateWindowExW(0, L"BUTTON", L"Show XP/Values in Mission UI", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 200, 439, 180, 22, hwndSettings, (HMENU)IDC_SET_CHK_SHOW_XP_CR, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_SHOW_XP_CR, g_Settings.bShowXPCR);

	/* --- VALUE MATCH SEARCH REGIONS --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Item Value Search Settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 470, 410, 95, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	ctrl = CreateWindowExW(0, L"STATIC", subWarn, WS_CHILD | WS_VISIBLE | SS_CENTER, 15, 488, 390, 30, hwndSettings, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	ctrl = CreateWindowExW(0, L"BUTTON", L"Match Single Item Value:", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 15, 530, 150, 22, hwndSettings, (HMENU)IDC_SET_CHK_MATCH_SINGLE, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_MATCH_SINGLE, g_Settings.bMatchSingle);
	ctrl = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 175, 528, 50, 22, hwndSettings, (HMENU)IDC_SET_EDIT_MATCH_SINGLE, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	SetDlgItemInt(hwndSettings, IDC_SET_EDIT_MATCH_SINGLE, g_Settings.iMatchSingleVal, FALSE);

	ctrl = CreateWindowExW(0, L"BUTTON", L"Match Total Value:", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 235, 530, 115, 22, hwndSettings, (HMENU)IDC_SET_CHK_MATCH_TOTAL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	CheckDlgButton(hwndSettings, IDC_SET_CHK_MATCH_TOTAL, g_Settings.bMatchTotal);
	ctrl = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 365, 528, 50, 22, hwndSettings, (HMENU)IDC_SET_EDIT_MATCH_TOTAL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	SetDlgItemInt(hwndSettings, IDC_SET_EDIT_MATCH_TOTAL, g_Settings.iMatchTotalVal, FALSE);

	ctrl = CreateWindowExW(0, L"BUTTON", L"Apply Settings", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 215, 572, 195, 24, hwndSettings, (HMENU)IDC_SET_BTN_APPLYSETTINGS, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	if (hIcon) {
		SendMessageW(hwndSettings, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessageW(hwndSettings, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	}

	ShowWindow(hwndSettings, SW_SHOW);
	UpdateWindow(hwndSettings);
}
