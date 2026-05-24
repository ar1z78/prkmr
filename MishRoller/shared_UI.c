#include <windows.h>
#include "globals.h"

extern HFONT g_hFont;
// --- SHARED WINDOW BACKGROUND OVERRIDES ---

// apply the base class background brush
void ApplySharedClassBackground(HWND hwnd) {
	HBRUSH hClassBrush = CreateSolidBrush((COLORREF)g_Settings.clrBg);
	SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hClassBrush);
}

// WM_CTLCOLORDLG
INT_PTR HandleSharedDlgColor(void) {
	static HBRUSH hDlgBrush = NULL;
	if (hDlgBrush != NULL) DeleteObject(hDlgBrush);
	hDlgBrush = CreateSolidBrush((COLORREF)g_Settings.clrBg);
	return (INT_PTR)hDlgBrush;
}

// SHARED BACKGROUND COLOR HANDLING OVERRIDES
INT_PTR HandleSharedStaticColor(WPARAM wp) {
	HDC hdcStatic = (HDC)wp;
	SetTextColor(hdcStatic, (COLORREF)g_Settings.clrTxt);
	SetBkColor(hdcStatic, (COLORREF)g_Settings.clrBg);

	static HBRUSH hStaticBrush = NULL;
	if (hStaticBrush) DeleteObject(hStaticBrush);
	hStaticBrush = CreateSolidBrush((COLORREF)g_Settings.clrBg);
	return (INT_PTR)hStaticBrush;
}

INT_PTR HandleSharedEditColor(WPARAM wp) {
	HDC hdcEdit = (HDC)wp;
	SetTextColor(hdcEdit, (COLORREF)g_Settings.clrTxt);
	SetBkColor(hdcEdit, (COLORREF)g_Settings.clrBg);

	static HBRUSH hEditBrush = NULL;
	if (hEditBrush) DeleteObject(hEditBrush);
	hEditBrush = CreateSolidBrush((COLORREF)g_Settings.clrBg);
	return (INT_PTR)hEditBrush;
}

// SHARED FLAT OWNER-DRAWN BUTTON SURFACE RENDERER
BOOL DrawSharedFlatButton(LPDRAWITEMSTRUCT pDrawItem) {
	// 1. Handle visual click compression feedback loop
	if (pDrawItem->itemState & ODS_SELECTED) {
		COLORREF rawBtn = (COLORREF)g_Settings.clrBtn;
		COLORREF pressedColor = RGB(GetRValue(rawBtn) * 3 / 4, GetGValue(rawBtn) * 3 / 4, GetBValue(rawBtn) * 3 / 4);
		HBRUSH hPressedBrush = CreateSolidBrush(pressedColor);
		FillRect(pDrawItem->hDC, &pDrawItem->rcItem, hPressedBrush);
		DeleteObject(hPressedBrush);
	}
	else {
		HBRUSH hNormalBrush = CreateSolidBrush((COLORREF)g_Settings.clrBtn);
		FillRect(pDrawItem->hDC, &pDrawItem->rcItem, hNormalBrush);
		DeleteObject(hNormalBrush);
	}

	// 2. Paint crisp flat frame border matching text choice rules
	HBRUSH hBorderBrush = CreateSolidBrush((COLORREF)g_Settings.clrTxt);
	FrameRect(pDrawItem->hDC, &pDrawItem->rcItem, hBorderBrush);
	DeleteObject(hBorderBrush);

	// 3. Render centered typography layer
	SetTextColor(pDrawItem->hDC, (COLORREF)g_Settings.clrTxt);
	SetBkMode(pDrawItem->hDC, TRANSPARENT);

	HGDIOBJ hOldFont = SelectObject(pDrawItem->hDC, g_hFont);

	char buttonText[128];
	GetWindowTextA(pDrawItem->hwndItem, buttonText, 128);
	DrawTextA(pDrawItem->hDC, buttonText, -1, &pDrawItem->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	SelectObject(pDrawItem->hDC, hOldFont);
	return TRUE;
}
// SHARED FLAT OWNER-DRAWN CHECKBOX SURFACE RENDERER
BOOL DrawSharedFlatCheckbox(LPDRAWITEMSTRUCT pDrawItem) {
	// 1. Clear checkbox block area matching global background setting
	HBRUSH hBackBrush = CreateSolidBrush((COLORREF)g_Settings.clrBg);
	FillRect(pDrawItem->hDC, &pDrawItem->rcItem, hBackBrush);
	DeleteObject(hBackBrush);

	// 2. Define geometry for the physical inner selection square bounding box
	RECT boxRect = {
		pDrawItem->rcItem.left,
		pDrawItem->rcItem.top + ((pDrawItem->rcItem.bottom - pDrawItem->rcItem.top) - 14) / 2,
		pDrawItem->rcItem.left + 14,
		0
	};
	boxRect.bottom = boxRect.top + 14;

	// 3. Paint matching flat background inside the small checkbox square frame
	HBRUSH hNormalBrush = CreateSolidBrush((COLORREF)g_Settings.clrBtn);
	FillRect(pDrawItem->hDC, &boxRect, hNormalBrush);
	DeleteObject(hNormalBrush);

	// 4. Draw flat square border outline matching custom theme rule
	HBRUSH hBorderBrush = CreateSolidBrush((COLORREF)g_Settings.clrTxt);
	FrameRect(pDrawItem->hDC, &boxRect, hBorderBrush);
	DeleteObject(hBorderBrush);

	// 5. Render custom inline checkmark if control state matches open check trigger
	if (pDrawItem->itemState & ODS_CHECKED) {
		HPEN hCheckPen = CreatePen(PS_SOLID, 2, (COLORREF)g_Settings.clrTxt);
		HGDIOBJ hOldPen = SelectObject(pDrawItem->hDC, hCheckPen);

		MoveToEx(pDrawItem->hDC, boxRect.left + 3, boxRect.top + 6, NULL);
		LineTo(pDrawItem->hDC, boxRect.left + 6, boxRect.top + 10);
		LineTo(pDrawItem->hDC, boxRect.left + 11, boxRect.top + 3);

		SelectObject(pDrawItem->hDC, hOldPen);
		DeleteObject(hCheckPen);
	}

	// 6. Draw adjacent label text string offset safely to the right
	RECT textRect = pDrawItem->rcItem;
	textRect.left += 20;

	SetTextColor(pDrawItem->hDC, (COLORREF)g_Settings.clrTxt);
	SetBkMode(pDrawItem->hDC, TRANSPARENT);

	HGDIOBJ hOldFont = SelectObject(pDrawItem->hDC, g_hFont);

	char cbText[128];
	GetWindowTextA(pDrawItem->hwndItem, cbText, 128);
	DrawTextA(pDrawItem->hDC, cbText, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	SelectObject(pDrawItem->hDC, hOldFont);
	return TRUE;
}
