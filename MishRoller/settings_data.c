#include <windows.h>
#include <commdlg.h>
#include "globals.h"

/* Allocated block array to securely shield picker memory state across dialog paths */
COLORREF g_CustomColors[16] = { 0 };

/* Draws individual colored control buttons acting as visual swatches */
void DrawColorSwatchBlock(LPDRAWITEMSTRUCT lpDrawItem, COLORREF color) {
	HBRUSH hBrush = CreateSolidBrush(color);
	FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, hBrush);
	DeleteObject(hBrush);
	DrawEdge(lpDrawItem->hDC, &lpDrawItem->rcItem, EDGE_SUNKEN, BF_RECT);
}

