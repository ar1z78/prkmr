#include <windows.h> /* REQUIRED FOR HWND AND WIN32 API TYPES */
#include <commctrl.h> /* REQUIRED FOR SUBCLASSING FUNCTIONS */
#include <string.h>
#include "resource.h"
#include "globals.h"

#pragma comment(lib, "comctl32.lib")

extern HFONT g_hFont; /* Share the classic global UI font from main.cpp */

/* Forward Declaration for Standard C Compilation Linearity */
LRESULT CALLBACK SearchWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/* Helper function to split pasted text by lines and insert into target ListBox */
void ParseAndAddPastedText(HWND hwndListBox, const char* szText) {
	char lineBuffer[256];
	int lineIdx = 0;

	while (*szText != '\0') {
		/* Strip out carriage returns or copy characters until a newline boundary is hit */
		if (*szText == '\n' || *szText == '\r') {
			if (lineIdx > 0) {
				lineBuffer[lineIdx] = '\0';
				SendMessageA(hwndListBox, LB_ADDSTRING, 0, (LPARAM)lineBuffer);
				lineIdx = 0;
			}
		}
		else {
			if (lineIdx < 255) {
				lineBuffer[lineIdx++] = *szText;
			}
		}
		szText++;
	}
	/* Catch any trailing text that didn't end with a final newline character */
	if (lineIdx > 0) {
		lineBuffer[lineIdx] = '\0';
		SendMessageA(hwndListBox, LB_ADDSTRING, 0, (LPARAM)lineBuffer);
	}
}

/* Grabs text data directly out of the raw Windows OS clipboard segment */
void HandleClipboardPaste(HWND hwndTextBox, HWND hwndListBox) {
	if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(hwndTextBox)) {
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (hData != NULL) {
			const char* szClipboardText = (const char*)GlobalLock(hData);
			if (szClipboardText != NULL) {
				/* If it contains a newline, bypass default paste and split it manually */
				if (strchr(szClipboardText, '\n') != NULL || strchr(szClipboardText, '\r') != NULL) {
					ParseAndAddPastedText(hwndListBox, szClipboardText);
					SetWindowTextA(hwndTextBox, ""); /* Clear box to look tidy */
				}
				else {
					/* Let normal single-line text paste act standardly */
					GlobalUnlock(hData);
					CloseClipboard();
					DefSubclassProc(hwndTextBox, WM_PASTE, 0, 0);
					return;
				}
				GlobalUnlock(hData);
			}
		}
		CloseClipboard();
	}
}

LRESULT CALLBACK TextBoxSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	HWND hwndParent = GetParent(hwnd);
	HWND hwndTargetListBox = GetDlgItem(hwndParent, (uIdSubclass == IDC_SEARCH_ITEM_EDITBOX) ? IDC_SEARCH_ITEM_LIST : IDC_SEARCH_LOC_LIST);

	/* 1. Catch right-click context menu "Paste" choices */
	if (msg == WM_PASTE) {
		HandleClipboardPaste(hwnd, hwndTargetListBox);
		return 0; /* Block default textbox processing */
	}

	/* 2. Catch hitting the Enter key to add an individual item silently */
	if (msg == WM_CHAR && wp == VK_RETURN) {
		SendMessageA(hwndParent, WM_COMMAND, MAKEWPARAM((uIdSubclass == IDC_SEARCH_ITEM_EDITBOX) ? IDC_SEARCH_ITEM_ADD : IDC_SEARCH_LOC_ADD, BN_CLICKED), 0);
		return 0;
	}

	/* 3. Optional: explicitly catch Ctrl+V key combination fallback hooks */
	if (msg == WM_KEYDOWN && wp == 'V' && (GetKeyState(VK_CONTROL) & 0x8000)) {
		HandleClipboardPaste(hwnd, hwndTargetListBox);
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wp, lp);
}

LRESULT CALLBACK ListBoxSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	/* Catch when the user presses Ctrl+V while the ListBox has keyboard focus */
	if (msg == WM_KEYDOWN && wp == 'V' && (GetKeyState(VK_CONTROL) & 0x8000)) {
		/* Open the Windows clipboard */
		if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(hwnd)) {
			HANDLE hData = GetClipboardData(CF_TEXT);
			if (hData != NULL) {
				const char* szClipboardText = (const char*)GlobalLock(hData);
				if (szClipboardText != NULL) {
					/* Call parsing helper function to split line-by-line */
					ParseAndAddPastedText(hwnd, szClipboardText);
					GlobalUnlock(hData);
				}
			}
			CloseClipboard();
		}
		return 0; /* Handled, block further processing */
	}
	return DefSubclassProc(hwnd, msg, wp, lp);
}

LRESULT CALLBACK SearchWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_CREATE: {
						/* Apply the global color background dynamically */
						ApplySharedClassBackground(hwnd);
						return 0;
	}

	case WM_CTLCOLORDLG:
		return HandleSharedDlgColor();

	case WM_CTLCOLORSTATIC:
		return HandleSharedStaticColor(wp);

	case WM_CTLCOLOREDIT:
		return HandleSharedEditColor(wp);
	case WM_DRAWITEM: {
						  LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT)lp;

						  /* 1. Identify Button Groups (Add, Update, Remove, Clear, and Apply actions) */
						  if (pDrawItem->CtlID == IDC_SEARCH_ITEM_ADD || pDrawItem->CtlID == IDC_SEARCH_ITEM_EDITBTN ||
							  pDrawItem->CtlID == IDC_SEARCH_ITEM_REMOVE || pDrawItem->CtlID == IDC_SEARCH_ITEM_CLEAR ||
							  pDrawItem->CtlID == IDC_SEARCH_LOC_ADD || pDrawItem->CtlID == IDC_SEARCH_LOC_EDITBTN ||
							  pDrawItem->CtlID == IDC_SEARCH_LOC_REMOVE || pDrawItem->CtlID == IDC_SEARCH_LOC_CLEAR ||
							  pDrawItem->CtlID == IDC_SEARCH_BTN_APPLY)
						  {
							  return DrawSharedFlatButton(pDrawItem);
						  }


						  break;
	}

	case WM_COMMAND: {
						 int selIdx = -1;
						 HWND hItemBox = GetDlgItem(hwnd, IDC_SEARCH_ITEM_LIST);
						 HWND hLocBox = GetDlgItem(hwnd, IDC_SEARCH_LOC_LIST);
						 char szBuffer[128] = { 0 };

						 /* --- ITEM LIST OPERATIONS --- */
						 if (LOWORD(wp) == IDC_SEARCH_ITEM_LIST && HIWORD(wp) == LBN_SELCHANGE)
						 {
							 selIdx = (int)SendMessageA(hItemBox, LB_GETCURSEL, 0, 0);
							 if (selIdx != LB_ERR)
							 {
								 SendMessageA(hItemBox, LB_GETTEXT, (WPARAM)selIdx, (LPARAM)szBuffer);
								 SetDlgItemTextA(hwnd, IDC_SEARCH_ITEM_EDITBOX, szBuffer);
							 }
						 }
						 if (LOWORD(wp) == IDC_SEARCH_ITEM_ADD)
						 {
							 GetDlgItemTextA(hwnd, IDC_SEARCH_ITEM_EDITBOX, szBuffer, 128);
							 if (szBuffer[0] != '\0')
							 {
								 SendMessageA(hItemBox, LB_ADDSTRING, 0, (LPARAM)szBuffer);
								 SetDlgItemTextA(hwnd, IDC_SEARCH_ITEM_EDITBOX, ""); /* Clear box */
							 }
							 /* Force focus back into the textbox for immediate typing loop */
							 SetFocus(GetDlgItem(hwnd, IDC_SEARCH_ITEM_EDITBOX));
						 }
						 if (LOWORD(wp) == IDC_SEARCH_ITEM_EDITBTN)
						 {
							 selIdx = (int)SendMessageA(hItemBox, LB_GETCURSEL, 0, 0);
							 GetDlgItemTextA(hwnd, IDC_SEARCH_ITEM_EDITBOX, szBuffer, 128);
							 if (selIdx != LB_ERR && szBuffer[0] != '\0')
							 {
								 SendMessageA(hItemBox, LB_DELETESTRING, (WPARAM)selIdx, 0);
								 SendMessageA(hItemBox, LB_INSERTSTRING, (WPARAM)selIdx, (LPARAM)szBuffer);
							 }
							 SetDlgItemTextA(hwnd, IDC_SEARCH_ITEM_EDITBOX, "");
							 SetFocus(GetDlgItem(hwnd, IDC_SEARCH_ITEM_EDITBOX));
						 }
						 if (LOWORD(wp) == IDC_SEARCH_ITEM_REMOVE)
						 {
							 selIdx = (int)SendMessageA(hItemBox, LB_GETCURSEL, 0, 0);
							 if (selIdx != LB_ERR)
							 {
								 SendMessageA(hItemBox, LB_DELETESTRING, (WPARAM)selIdx, 0);
								 SetDlgItemTextA(hwnd, IDC_SEARCH_ITEM_EDITBOX, "");
							 }
							 SetFocus(GetDlgItem(hwnd, IDC_SEARCH_ITEM_EDITBOX));
						 }
						 if (LOWORD(wp) == IDC_SEARCH_ITEM_CLEAR)
						 {
							 SendMessageA(hItemBox, LB_RESETCONTENT, 0, 0);
							 SetDlgItemTextA(hwnd, IDC_SEARCH_ITEM_EDITBOX, "");
							 SetFocus(GetDlgItem(hwnd, IDC_SEARCH_ITEM_EDITBOX));
						 }

						 /* --- LOCATION LIST OPERATIONS --- */
						 if (LOWORD(wp) == IDC_SEARCH_LOC_LIST && HIWORD(wp) == LBN_SELCHANGE)
						 {
							 selIdx = (int)SendMessageA(hLocBox, LB_GETCURSEL, 0, 0);
							 if (selIdx != LB_ERR)
							 {
								 SendMessageA(hLocBox, LB_GETTEXT, (WPARAM)selIdx, (LPARAM)szBuffer);
								 SetDlgItemTextA(hwnd, IDC_SEARCH_LOC_EDITBOX, szBuffer);
							 }
						 }
						 if (LOWORD(wp) == IDC_SEARCH_LOC_ADD)
						 {
							 GetDlgItemTextA(hwnd, IDC_SEARCH_LOC_EDITBOX, szBuffer, 128);
							 if (szBuffer[0] != '\0')
							 {
								 SendMessageA(hLocBox, LB_ADDSTRING, 0, (LPARAM)szBuffer);
								 SetDlgItemTextA(hwnd, IDC_SEARCH_LOC_EDITBOX, ""); /* Clear box */
							 }
							 /* Force focus back into the textbox for immediate typing loop */
							 SetFocus(GetDlgItem(hwnd, IDC_SEARCH_LOC_EDITBOX));
						 }
						 if (LOWORD(wp) == IDC_SEARCH_LOC_EDITBTN)
						 {
							 selIdx = (int)SendMessageA(hLocBox, LB_GETCURSEL, 0, 0);
							 GetDlgItemTextA(hwnd, IDC_SEARCH_LOC_EDITBOX, szBuffer, 128);
							 if (selIdx != LB_ERR && szBuffer[0] != '\0')
							 {
								 SendMessageA(hLocBox, LB_DELETESTRING, (WPARAM)selIdx, 0);
								 SendMessageA(hLocBox, LB_INSERTSTRING, (WPARAM)selIdx, (LPARAM)szBuffer);
							 }
							 SetFocus(GetDlgItem(hwnd, IDC_SEARCH_LOC_EDITBOX));
						 }
						 if (LOWORD(wp) == IDC_SEARCH_LOC_REMOVE)
						 {
							 selIdx = (int)SendMessageA(hLocBox, LB_GETCURSEL, 0, 0);
							 if (selIdx != LB_ERR)
							 {
								 SendMessageA(hLocBox, LB_DELETESTRING, (WPARAM)selIdx, 0);
								 SetDlgItemTextA(hwnd, IDC_SEARCH_LOC_EDITBOX, "");
							 }
							 SetFocus(GetDlgItem(hwnd, IDC_SEARCH_LOC_EDITBOX));
						 }
						 if (LOWORD(wp) == IDC_SEARCH_LOC_CLEAR)
						 {
							 SendMessageA(hLocBox, LB_RESETCONTENT, 0, 0);
							 SetDlgItemTextA(hwnd, IDC_SEARCH_LOC_EDITBOX, "");
							 SetFocus(GetDlgItem(hwnd, IDC_SEARCH_LOC_EDITBOX));
						 }

						 /* --- SAVE / APPLY ALL SEARCH CONFIGURATIONS --- */
						 if (LOWORD(wp) == IDC_SEARCH_BTN_APPLY)
						 {
							 int idx;
							 /* Capture counts into separate tracking structure */
							 g_WatchLists.itemWatchCount = (int)SendMessageW(hItemBox, LB_GETCOUNT, 0, 0);
							 g_WatchLists.locWatchCount = (int)SendMessageW(hLocBox, LB_GETCOUNT, 0, 0);

							 if (g_WatchLists.itemWatchCount > MAX_WATCH_ITEMS) g_WatchLists.itemWatchCount = MAX_WATCH_ITEMS;
							 if (g_WatchLists.locWatchCount > MAX_WATCH_LOCATIONS) g_WatchLists.locWatchCount = MAX_WATCH_LOCATIONS;

							 /* Extract text rows natively as 8-bit ANSI strings into memory */
							 for (idx = 0; idx < g_WatchLists.itemWatchCount; ++idx) {
								 SendMessageA(hItemBox, LB_GETTEXT, (WPARAM)idx, (LPARAM)g_WatchLists.itemWatchList[idx]);
							 }

							 for (idx = 0; idx < g_WatchLists.locWatchCount; ++idx) {
								 SendMessageA(hLocBox, LB_GETTEXT, (WPARAM)idx, (LPARAM)g_WatchLists.locWatchList[idx]);
							 }

							 /* 3. Extract Item and Location targeting rules */
							 g_Settings.bItemBuyAgent = (IsDlgButtonChecked(hwnd, IDC_SEARCH_ITEM_CHK_BUY) == BST_CHECKED);
							 g_Settings.bItemHighlight = (IsDlgButtonChecked(hwnd, IDC_SEARCH_ITEM_CHK_HIGH) == BST_CHECKED);
							 g_Settings.bLocBuyAgent = (IsDlgButtonChecked(hwnd, IDC_SEARCH_LOC_CHK_BUY) == BST_CHECKED);
							 g_Settings.bLocHighlight = (IsDlgButtonChecked(hwnd, IDC_SEARCH_LOC_CHK_HIGH) == BST_CHECKED);

							 /* 4. Extract Mission Type mission matching checkboxes */
							 g_Settings.bMatchRepair = (IsDlgButtonChecked(hwnd, IDC_SEARCH_CHK_REPAIR) == BST_CHECKED);
							 g_Settings.bMatchReturn = (IsDlgButtonChecked(hwnd, IDC_SEARCH_CHK_RETURN) == BST_CHECKED);
							 g_Settings.bMatchPerson = (IsDlgButtonChecked(hwnd, IDC_SEARCH_CHK_PERSON) == BST_CHECKED);
							 g_Settings.bMatchItem = (IsDlgButtonChecked(hwnd, IDC_SEARCH_CHK_ITEM) == BST_CHECKED);
							 g_Settings.bMatchKill = (IsDlgButtonChecked(hwnd, IDC_SEARCH_CHK_KILL) == BST_CHECKED);

							 /* Extract Mission Search Options behavior rules */
							 g_Settings.bMishBuyAgent = (IsDlgButtonChecked(hwnd, IDC_SEARCH_MISH_CHK_BUY) == BST_CHECKED);
							 g_Settings.bMishlHighlight = (IsDlgButtonChecked(hwnd, IDC_SEARCH_MISH_CHK_HIGH) == BST_CHECKED);
						 }
						 break;
	}
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	default:
		return DefWindowProcW(hwnd, msg, wp, lp);
	}
	return 0;
}

void CreateSearchWindow(HWND hwndParent) {
	HINSTANCE hInst;
	WNDCLASSEXW wc = { 0 };
	HWND hwndSearch, ctrl;
	int i, b, idx;

	/* --- ALL ARRAYS AND LOOP TRACKERS DECLARED AT TOP FOR STRIC T C89 COMPLIANCE --- */
	LPCWSTR groupTitles[2];
	int baseListIDs[2];
	int baseEditIDs[2];
	int baseBtnAddIDs[2];
	int baseChkBuyIDs[2];
	int baseChkHighIDs[2];

	LPCWSTR mTypeLabels[5];
	int mTypeIDs[5];
	int mTypeWidths[5];
	int mTypeOffsets[5];

	LPCWSTR mOptLabels[2];
	int mOptIDs[2];
	int mOptWidths[2];
	int mOptOffsets[2];

	LPCWSTR actionLabels[4];
	int btnWidths[4];
	int btnOffsets[4];

	LPCWSTR subWarn = L"A match in this section will count as an ITEM MATCH\nMake sure you have those enabled if you want to search for values";

	/* Initialize Dual-Column Arrays Explicitly */
	groupTitles[0] = L"Item list";     groupTitles[1] = L"Location list";
	baseListIDs[0] = IDC_SEARCH_ITEM_LIST;     baseListIDs[1] = IDC_SEARCH_LOC_LIST;
	baseEditIDs[0] = IDC_SEARCH_ITEM_EDITBOX;  baseEditIDs[1] = IDC_SEARCH_LOC_EDITBOX;
	baseBtnAddIDs[0] = IDC_SEARCH_ITEM_ADD;     baseBtnAddIDs[1] = IDC_SEARCH_LOC_ADD;
	baseChkBuyIDs[0] = IDC_SEARCH_ITEM_CHK_BUY;  baseChkBuyIDs[1] = IDC_SEARCH_LOC_CHK_BUY;
	baseChkHighIDs[0] = IDC_SEARCH_ITEM_CHK_HIGH; baseChkHighIDs[1] = IDC_SEARCH_LOC_CHK_HIGH;

	/* Initialize Action Row Buttons Layout Arrays */
	actionLabels[0] = L"Add";    actionLabels[1] = L"Update"; actionLabels[2] = L"Remove"; actionLabels[3] = L"Clear";
	btnWidths[0] = 86;       btnWidths[1] = 86;       btnWidths[2] = 86;        btnWidths[3] = 90;
	btnOffsets[0] = 12;      btnOffsets[1] = 102;     btnOffsets[2] = 192;      btnOffsets[3] = 282;

	/* Initialize Mission Type Checklist Arrays */
	mTypeLabels[0] = L"Repair"; mTypeLabels[1] = L"Return"; mTypeLabels[2] = L"Person"; mTypeLabels[3] = L"Find Item"; mTypeLabels[4] = L"Kill Person";
	mTypeIDs[0] = IDC_SEARCH_CHK_REPAIR; mTypeIDs[1] = IDC_SEARCH_CHK_RETURN; mTypeIDs[2] = IDC_SEARCH_CHK_PERSON; mTypeIDs[3] = IDC_SEARCH_CHK_ITEM; mTypeIDs[4] = IDC_SEARCH_CHK_KILL;
	mTypeWidths[0] = 65; mTypeWidths[1] = 65; mTypeWidths[2] = 65; mTypeWidths[3] = 70; mTypeWidths[4] = 75;
	mTypeOffsets[0] = 15; mTypeOffsets[1] = 80; mTypeOffsets[2] = 145; mTypeOffsets[3] = 210; mTypeOffsets[4] = 290;

	/* Initialize Mission Global Configuration Arrays */
	mOptLabels[0] = L"Match in Rolling Agent"; mOptLabels[1] = L"Highlight Matches";
	mOptIDs[0] = IDC_SEARCH_MISH_CHK_BUY; mOptIDs[1] = IDC_SEARCH_MISH_CHK_HIGH;
	mOptWidths[0] = 160; mOptWidths[1] = 150;
	mOptOffsets[0] = 15; mOptOffsets[1] = 215;

	hInst = (HINSTANCE)GetWindowLongPtrW(hwndParent, GWLP_HINSTANCE);

	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = SearchWndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = L"MishRollerSearchClass";
	RegisterClassExW(&wc);

	hwndSearch = CreateWindowExW(0, L"MishRollerSearchClass", L"Search Options", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 400, 610, hwndParent, NULL, hInst, NULL);
	g_hwndSearch = hwndSearch;
	if (!hwndSearch) return;

	/* --- COLUMN 1 & COLUMN 2 SEAMLESS REUSABLE LOOP (Item & Location Sections) --- */
	for (i = 0; i < 2; ++i) {
		int yShift = i * 200;

		/* Group Box Header Frame */
		ctrl = CreateWindowExW(0, L"BUTTON", groupTitles[i], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 5 + yShift, 375, 195, hwndSearch, NULL, hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

		/* ListBox Elements */
		ctrl = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP, 12, 22 + yShift, 360, 95, hwndSearch, (HMENU)(INT_PTR)baseListIDs[i], hInst, NULL);
		SendMessageA(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
		SetWindowSubclass(ctrl, ListBoxSubclassProc, baseListIDs[i], 0);

		/* Edit Control Fields */
		ctrl = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 12, 122 + yShift, 360, 20, hwndSearch, (HMENU)(INT_PTR)baseEditIDs[i], hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
		SetWindowSubclass(ctrl, TextBoxSubclassProc, baseEditIDs[i], 0);

		/* Master Action Row Buttons (Add, Update, Remove, Clear) set to BS_OWNERDRAW */
		int itemBtnIDs[] = { IDC_SEARCH_ITEM_ADD, IDC_SEARCH_ITEM_EDITBTN, IDC_SEARCH_ITEM_REMOVE, IDC_SEARCH_ITEM_CLEAR };
		int locBtnIDs[] = { IDC_SEARCH_LOC_ADD, IDC_SEARCH_LOC_EDITBTN, IDC_SEARCH_LOC_REMOVE, IDC_SEARCH_LOC_CLEAR };
		LPCWSTR actionLabels[] = { L"Add", L"Update", L"Remove", L"Clear" };
		int btnWidths[] = { 86, 86, 86, 90 };
		int btnOffsets[] = { 12, 102, 192, 282 };

		for (b = 0; b < 4; ++b) {
			/* Pick the exact targeted ID array based on whether we are on row 0 (Item) or row 1 (Location) */
			int targetButtonID = (i == 0) ? itemBtnIDs[b] : locBtnIDs[b];

			ctrl = CreateWindowExW(0, L"BUTTON", actionLabels[b], WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
				btnOffsets[b], 144 + yShift, btnWidths[b], 23, hwndSearch, (HMENU)(INT_PTR)targetButtonID, hInst, NULL);
			SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
		}


		/* Configuration Target Checkboxes (Swapped to BS_OWNERDRAW and expanded to Height=20 to prevent clipping) */
		ctrl = CreateWindowExW(0, L"BUTTON", L"Match in Rolling Agent /", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 15, 172 + yShift, 150, 20, hwndSearch, (HMENU)(INT_PTR)baseChkBuyIDs[i], hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

		ctrl = CreateWindowExW(0, L"BUTTON", L"Highlight Matches", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 215, 172 + yShift, 130, 20, hwndSearch, (HMENU)(INT_PTR)baseChkHighIDs[i], hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	}

	/* --- SECTION 3: MISSION TYPES CONTAINER FRAME --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Mission types", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 405, 375, 65, hwndSearch, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	for (i = 0; i < 5; ++i) {
		ctrl = CreateWindowExW(0, L"BUTTON", mTypeLabels[i], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, mTypeOffsets[i], 425, mTypeWidths[i], 20, hwndSearch, (HMENU)(INT_PTR)mTypeIDs[i], hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	}

	/* --- SECTION 4: MISSION SEARCH SELECTION CONTAINER FRAME --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Mission search options", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 475, 375, 45, hwndSearch, NULL, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	for (i = 0; i < 2; ++i) {
		ctrl = CreateWindowExW(0, L"BUTTON", mOptLabels[i], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, mOptOffsets[i], 495, mOptWidths[i], 20, hwndSearch, (HMENU)(INT_PTR)mOptIDs[i], hInst, NULL);
		SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	}

	/* --- SECTION 5: MASTER CONFIRMATION FOOTER APPLY BUTTON (Swapped to BS_OWNERDRAW) --- */
	ctrl = CreateWindowExW(0, L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 5, 530, 375, 25, hwndSearch, (HMENU)IDC_SEARCH_BTN_APPLY, hInst, NULL);
	SendMessageW(ctrl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

	/* Populate Dialog Buttons based on current global settings state values */
	CheckDlgButton(hwndSearch, IDC_SEARCH_ITEM_CHK_BUY, g_Settings.bItemBuyAgent ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSearch, IDC_SEARCH_ITEM_CHK_HIGH, g_Settings.bItemHighlight ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSearch, IDC_SEARCH_LOC_CHK_BUY, g_Settings.bLocBuyAgent ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSearch, IDC_SEARCH_LOC_CHK_HIGH, g_Settings.bLocHighlight ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(hwndSearch, IDC_SEARCH_CHK_REPAIR, g_Settings.bMatchRepair ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSearch, IDC_SEARCH_CHK_RETURN, g_Settings.bMatchReturn ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSearch, IDC_SEARCH_CHK_PERSON, g_Settings.bMatchPerson ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSearch, IDC_SEARCH_CHK_ITEM, g_Settings.bMatchItem ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSearch, IDC_SEARCH_CHK_KILL, g_Settings.bMatchKill ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(hwndSearch, IDC_SEARCH_MISH_CHK_BUY, g_Settings.bMishBuyAgent ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndSearch, IDC_SEARCH_MISH_CHK_HIGH, g_Settings.bMishlHighlight ? BST_CHECKED : BST_UNCHECKED);

	/* Populate Lists from runtime memory pools */
	{
		for (idx = 0; idx < g_WatchLists.itemWatchCount; ++idx) {
			SendDlgItemMessageA(hwndSearch, IDC_SEARCH_ITEM_LIST, LB_ADDSTRING, 0, (LPARAM)g_WatchLists.itemWatchList[idx]);
		}
		for (idx = 0; idx < g_WatchLists.locWatchCount; ++idx) {
			SendDlgItemMessageA(hwndSearch, IDC_SEARCH_LOC_LIST, LB_ADDSTRING, 0, (LPARAM)g_WatchLists.locWatchList[idx]);
		}
	}
	{
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
		if (hIcon != NULL) {
			SendMessageW(hwndSearch, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessageW(hwndSearch, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}
	}
	ShowWindow(hwndSearch, SW_SHOW);
	UpdateWindow(hwndSearch);
}
