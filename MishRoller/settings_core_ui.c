#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include "resource.h"
#include "globals.h"
#include "MishRoller.h"

extern char g_MRDir[MAX_PATH];
extern COLORREF g_CustomColors[16];
const LPCWSTR g_SettingsUrl = L"https://github.com/ar1z78/prkmr";
extern HFONT g_hFont;

void ExportSettings(const char* filename);
void ImportSettings(const char* filename);
BOOL GetFile(HWND hWndOwner, BOOL saving, char* buffer, int buffersize);
void _setSliders(int easy_hard, int good_bad, int order_chaos, int open_hidden, int phys_myst, int headon_stealth, int money_xp);
void DrawColorSwatchBlock(LPDRAWITEMSTRUCT lpDrawItem, COLORREF color);
void ApplyDynamicFontSettings(HWND hwndSettings);

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_CREATE: {
						// Apply the global color background dynamically
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

						  // 1. Draw local color
						  if (pDrawItem->CtlID >= IDC_SET_BTN_COLOR_BG && pDrawItem->CtlID <= IDC_SET_BTN_COLOR_ICONBG) {
							  unsigned long colorVal = 0;
							  if (pDrawItem->CtlID == IDC_SET_BTN_COLOR_BG)     colorVal = g_Settings.clrBg;
							  if (pDrawItem->CtlID == IDC_SET_BTN_COLOR_BTN)    colorVal = g_Settings.clrBtn;
							  if (pDrawItem->CtlID == IDC_SET_BTN_COLOR_MATCH)  colorVal = g_Settings.clrMatch;
							  if (pDrawItem->CtlID == IDC_SET_BTN_COLOR_ICONBG) colorVal = g_Settings.clrIconBg;
							  DrawColorSwatchBlock(pDrawItem, (COLORREF)colorVal);
							  return TRUE;
						  }

						  // 2. Route Master Action Push Buttons and URL links explicitly
						  if (pDrawItem->CtlID == IDC_SET_BTN_SLIDERS || pDrawItem->CtlID == IDC_SET_BTN_EXPORT ||
							  pDrawItem->CtlID == IDC_SET_BTN_IMPORT || pDrawItem->CtlID == IDC_SET_BTN_APPLYSETTINGS ||
							  pDrawItem->CtlID == IDC_SET_BTN_LAUNCH_FONT_DIALOG || pDrawItem->CtlID == IDC_SET_LINK_URL)
						  {
							  return DrawSharedFlatButton(pDrawItem);
						  }
						  break;
	}

	case WM_COMMAND:
		// --- CHOOSEFONT DIALOG LAUNCH TRIGGER ---
		if (LOWORD(wp) == IDC_SET_BTN_LAUNCH_FONT_DIALOG) {
			// FIXED: Forced explicit ANSI structures to match  g_Settings string arrays
			CHOOSEFONTA cf = { 0 };
			LOGFONTA lf = { 0 };

			// Pre-seed the dialog structure using current loaded settings
			strcpy_s(lf.lfFaceName, LF_FACESIZE, g_Settings.szFontName);
			lf.lfHeight = -MulDiv(g_Settings.iFontSize, GetDeviceCaps(GetDC(hwnd), LOGPIXELSY), 72); // - ((int)g_Settings.iFontSize);
			lf.lfWeight = g_Settings.bFontBold ? FW_BOLD : FW_NORMAL;

			cf.lStructSize = sizeof(CHOOSEFONTA);
			cf.hwndOwner = hwnd;
			cf.lpLogFont = &lf;
			cf.rgbColors = g_Settings.clrTxt;
			cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;

			// Show the Font Dialog natively using ANSI tracking hooks
			if (ChooseFontA(&cf)) {
				// Extract point size
				g_Settings.iFontSize = cf.iPointSize / 10;
				if (g_Settings.iFontSize > 12) g_Settings.iFontSize = 12;
				if (lf.lfHeight < -16) lf.lfHeight = -16;

				// Decode the weight: if system font weight is bold or higher, save as 1 (True)
				g_Settings.bFontBold = (lf.lfWeight >= FW_BOLD) ? 1 : 0;

				// Copy font name character bytes directly into global configuration struct
				strcpy_s(g_Settings.szFontName, sizeof(g_Settings.szFontName), lf.lfFaceName);

				// Extract the picked text color value from the dialog effects row
				g_Settings.clrTxt = (unsigned long)cf.rgbColors;

				// Rebuild the master application text font object using the selected settings
				if (g_hFont) DeleteObject(g_hFont);
				g_hFont = CreateFontIndirectA(&lf);

				// Update every visible control window on the screen with new font selection
				HWND hChild = GetWindow(g_hwndMishBoard, GW_CHILD);
				while (hChild != NULL) {
					SendMessageA(hChild, WM_SETFONT, (WPARAM)g_hFont, TRUE);
					hChild = GetWindow(hChild, GW_HWNDNEXT);
				}

				// Redraw layout labels and frames completely
				InvalidateRect(hwnd, NULL, TRUE);
				InvalidateRect(g_hwndMishBoard, NULL, TRUE);
				InvalidateRect(g_hwndSearch, NULL, TRUE);
			}
		}
		// --- FIXED CHOOSECOLOR DIALOG INTEGRATION ---
		if (LOWORD(wp) >= IDC_SET_BTN_COLOR_BG && LOWORD(wp) <= IDC_SET_BTN_COLOR_ICONBG) {
			CHOOSECOLORW cc = { 0 };
			unsigned long* pTargetColor = NULL;

			if (LOWORD(wp) == IDC_SET_BTN_COLOR_BG)     pTargetColor = &g_Settings.clrBg;
			if (LOWORD(wp) == IDC_SET_BTN_COLOR_BTN)    pTargetColor = &g_Settings.clrBtn;
			if (LOWORD(wp) == IDC_SET_BTN_COLOR_TXT)    pTargetColor = &g_Settings.clrTxt;
			if (LOWORD(wp) == IDC_SET_BTN_COLOR_MATCH)  pTargetColor = &g_Settings.clrMatch;
			if (LOWORD(wp) == IDC_SET_BTN_COLOR_ICONBG) pTargetColor = &g_Settings.clrIconBg;

			cc.lStructSize = sizeof(CHOOSECOLORW);
			cc.hwndOwner = hwnd;
			cc.lpCustColors = g_CustomColors; // Points directly to static array initialization block
			cc.rgbResult = (COLORREF)*pTargetColor;
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;

			if (ChooseColorW(&cc)) {
				*pTargetColor = (unsigned long)cc.rgbResult;
				// Force immediate redraw to update swatch color
				ApplySharedClassBackground(hwnd);
				ApplySharedClassBackground(g_hwndMishBoard);
				ApplySharedClassBackground(g_hwndSearch);
				InvalidateRect(hwnd, NULL, TRUE);
				InvalidateRect(g_hwndMishBoard, NULL, TRUE);
				InvalidateRect(g_hwndSearch, NULL, TRUE);
				
			}
		}

		// --- SAVE ALL SETTINGS ON APPLY ---
		if (LOWORD(wp) == IDC_SET_BTN_APPLYSETTINGS) {
			int baseSliderID = IDC_SET_EDIT_EASY;
			int idx;

			g_Settings.bStartMinimized = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_MINIMIZED) == BST_CHECKED);
			g_Settings.bAlertBox = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_ALERT) == BST_CHECKED);
			g_Settings.bSelectMatch = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_SELECT) == BST_CHECKED);
			g_Settings.bSounds = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_SOUNDS) == BST_CHECKED);
			g_Settings.bLogging = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_LOGGING) == BST_CHECKED);
			g_Settings.bShowHelp = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_HELP) == BST_CHECKED);
			g_Settings.bAutoExpand = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_EXPAND) == BST_CHECKED);

			g_Settings.dwWaitTime = GetDlgItemInt(hwnd, IDC_SET_EDIT_WAIT, NULL, FALSE);

			for (idx = 0; idx < 7; ++idx) {
				g_Settings.Sliders[idx] = GetDlgItemInt(hwnd, baseSliderID + idx, NULL, FALSE);
			}

			//g_Settings.iBuyMod = GetDlgItemInt(hwnd, IDC_SET_EDIT_BUYMOD, NULL, FALSE);
			int selectedIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SET_EDIT_BUYMOD), CB_GETCURSEL, 0, 0);
			g_Settings.iBuyMod = ((selectedIndex * 3) / 2) + 4;
			g_Settings.bShowXPCR = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_SHOW_XP_CR ) == BST_CHECKED);
			g_Settings.bMatchSingle = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_MATCH_SINGLE) == BST_CHECKED);
			g_Settings.iMatchSingleVal = GetDlgItemInt(hwnd, IDC_SET_EDIT_MATCH_SINGLE, NULL, FALSE);
			g_Settings.bMatchTotal = (IsDlgButtonChecked(hwnd, IDC_SET_CHK_MATCH_TOTAL) == BST_CHECKED);
			g_Settings.iMatchTotalVal = GetDlgItemInt(hwnd, IDC_SET_EDIT_MATCH_TOTAL, NULL, FALSE);

			ExportSettings("LastSettings.mr");
		}

		if (LOWORD(wp) == IDC_SET_BTN_EXPORT) {
			char buffer[2000];
			if (g_hwndMishBoard != NULL && GetFile(g_hwndMishBoard, TRUE, buffer, 2000)) {
				ExportSettings(buffer);
			}
			SetCurrentDirectory(g_MRDir);
		}
		if (LOWORD(wp) == IDC_SET_LINK_URL) {
			ShellExecuteW(hwnd, L"open", g_SettingsUrl, NULL, NULL, SW_SHOWNORMAL);
		}
		if (LOWORD(wp) == IDC_SET_BTN_SLIDERS) {
			_setSliders(g_Settings.Sliders[0], g_Settings.Sliders[1], g_Settings.Sliders[2], g_Settings.Sliders[3], g_Settings.Sliders[4], g_Settings.Sliders[5], g_Settings.Sliders[6]);
		}
		if (LOWORD(wp) == IDC_SET_BTN_IMPORT) {
			char buffer[2000];
			if (g_hwndMishBoard != NULL && GetFile(g_hwndMishBoard, FALSE, buffer, 2000)) {
				ImportSettings(buffer);
				HBRUSH hNewBg = CreateSolidBrush((COLORREF)g_Settings.clrBg);
				SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBg);

				//ApplyDynamicFontSettings(hwnd);
				//ApplyDynamicFontSettings(g_hwndMishBoard);
			}
			SetCurrentDirectory(g_MRDir);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	default:
		return DefWindowProcW(hwnd, msg, wp, lp);
	}
	return 0;
}

// Dynamically recreates the master global font token and pushes it to all child windows
void ApplyDynamicFontSettings(HWND hwndSettings) {
	// 1. Prevent memory leaks by safely deleting the old font instance if it exists
	if (g_hFont != NULL) {
		DeleteObject(g_hFont);
		g_hFont = NULL;
	}

	// 2. Build a fresh Win32 font using our active configuration properties
	g_hFont = CreateFontA(
		-(int)g_Settings.iFontSize,         // Height (Negative matches cell height in points)
		0,                                  // Width (0 auto-calculates correct proportions)
		0, 0,                               // Escapement, Orientation
		g_Settings.bFontBold ? FW_BOLD : FW_NORMAL, // Weight parameter configuration
		FALSE, FALSE, FALSE,                // Italic, Underline, Strikeout
		DEFAULT_CHARSET,                    // Character mapping set
		OUT_DEFAULT_PRECIS,                 // Output precision
		CLIP_DEFAULT_PRECIS,                // Clipping precision
		DEFAULT_QUALITY,                    // Rendering rendering quality
		DEFAULT_PITCH | FF_DONTCARE,        // Pitch and Family flags
		g_Settings.szFontName               // Natively extracted typeface name string
		);

	// 3. Recursively push the new font to every child element inside our window frame
	if (hwndSettings != NULL && g_hFont != NULL) {
		HWND hChild = GetWindow(hwndSettings, GW_CHILD);
		while (hChild != NULL) {
			// Tell the child control to load the font and force a clean screen layout refresh
			SendMessageW(hChild, WM_SETFONT, (WPARAM)g_hFont, TRUE);
			hChild = GetWindow(hChild, GW_HWNDNEXT);
		}
		// Force the main settings dialog canvas to clear and re-render completely
		InvalidateRect(hwndSettings, NULL, TRUE);
		UpdateWindow(hwndSettings);
	}
}
