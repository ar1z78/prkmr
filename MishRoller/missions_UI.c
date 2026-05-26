#include "missions_UI.h"
#include <commctrl.h>
#include <stdio.h>
#include "globals.h"
#include <stdio.h>

// Match the exact handle created in core layout file
extern HFONT g_hFont;
// Concrete definitions of globals to satisfy the linker

const int FRAME_H = 108;
const int BORDER_H = 10;
const int BORDER_W = 0;


const int NEW_GRID_START_X = 245;

// Footer Control Window Handles definitions
HWND g_hwndTriesLbl   = NULL;
HWND g_hwndTriesEdit  = NULL;
HWND g_hwndMishLbl    = NULL;
HWND g_hwndMishEdit   = NULL;
HWND g_hwndStartAgent = NULL;
HWND g_hwndStopAgent  = NULL;
HWND g_hwndFullscreen = NULL;
HWND g_hwndRemoveDups = NULL;


void DrawGuiMissions(HWND hwnd, HDC hdc) {
	RECT clientRect;
	int windowWidth;
	HFONT hOldFont;
	BOOL useSingleColumnMode;
	int m, i;

	GetClientRect(hwnd, &clientRect);
	windowWidth = clientRect.right - clientRect.left;

	hOldFont = (HFONT)SelectObject(hdc, g_hFont);

	SetBkMode(hdc, OPAQUE);
	SetBkColor(hdc, g_Settings.clrBg);

	useSingleColumnMode = (windowWidth < 650);

	// Loop exactly across all 5 structural mission column blocks
	for (m = 0; m < MAX_MISSIONS; m++) {
		// Dynamic track size parameters
		int frame_top = BORDER_H + (m * FRAME_H);
		RECT rcFrame = { BORDER_W, frame_top, windowWidth - BORDER_W, frame_top + FRAME_H - BORDER_H };
		char frame_title[64];
		int LINE_H = (FRAME_H - BORDER_H * 2) / 4;

		UINT boxState = DFCS_BUTTONCHECK | (g_MishBoardData.bMishIsFind[m] ? DFCS_CHECKED : 0);
		int col_w;
		int itemsToRender;
		int BORDER_W2 = BORDER_W + 15;
		int checkBoxX;

		char txt_loc[256], txt_type[256], txt_stats[256], txt_total[64];
		SIZE textSize; // Win32 structure to hold calculated string width (cx) and height (cy)
		int currentY;  // Dynamic vertical layout tracker

		// 1. Paint the classic outer etched bounding box container
		DrawEdge(hdc, &rcFrame, EDGE_ETCHED, BF_RECT);

		sprintf_s(frame_title, sizeof(frame_title), " Mission %d ", m + 1);
		SetTextColor(hdc, g_Settings.clrTxt);
		TextOutA(hdc, 176, frame_top - 6, frame_title, (int)strlen(frame_title));

		// 2. Format live data strings OR Fall back to default placeholders if empty
		if (g_MishBoardData.szMishLoc[m][0] != '\0') {
			sprintf_s(txt_loc, sizeof(txt_loc), "Loc: %s", g_MishBoardData.szMishLoc[m]);
			sprintf_s(txt_type, sizeof(txt_type), "Type: %s", g_MishBoardData.szMishType[m]);
			sprintf_s(txt_stats, sizeof(txt_stats), "XP: %d  |  Cash: %d", g_MishBoardData.iMishXP[m], g_MishBoardData.iMishCash[m]);
			sprintf_s(txt_total, sizeof(txt_total), "Total: %d", g_MishBoardData.iMishTotal[m]);
		}
		else {
			sprintf_s(txt_loc, sizeof(txt_loc), "Loc: -");
			sprintf_s(txt_type, sizeof(txt_type), "Type: -");
			sprintf_s(txt_stats, sizeof(txt_stats), "XP: 0  |  Cash: 0");
			sprintf_s(txt_total, sizeof(txt_total), "Total: 0");
		}

		/* ====================================================================
		DYNAMIC ACCELERATED TEXT MAPPING LAYER (Anti-Overlap Engine)
		==================================================================== */

		// Start drawing slightly inside the frame header
		currentY = frame_top + BORDER_H;

		// LINE 1: Stats (XP/Cash)
		if (g_Settings.bShowXPCR) {
			//GetTextExtentPoint32A(hdc, txt_stats, (int)strlen(txt_stats), &textSize);
			SetTextColor(hdc, g_Settings.clrTxt);
			TextOutA(hdc, BORDER_W2, currentY, txt_stats, (int)strlen(txt_stats));
			// Measure Line 1's height using active font, then advance currentY
			currentY += LINE_H;
			// LINE 2: Total Value Field
			if (g_MishBoardData.bHilightTotal[m]) SetTextColor(hdc, g_Settings.clrMatch);
			else SetTextColor(hdc, g_Settings.clrTxt);
			TextOutA(hdc, BORDER_W2, currentY, txt_total, (int)strlen(txt_total));
			// Dynamically track Line 2 string dimensions to position the checkbox next to it
			GetTextExtentPoint32A(hdc, txt_total, (int)strlen(txt_total), &textSize);
			checkBoxX = BORDER_W2 + textSize.cx + 10; // Placed 10 pixels after the Total string end
		}
		else
		{
			LINE_H = (FRAME_H - BORDER_H * 2) / 3;
			checkBoxX = BORDER_W2;
		}
		

		// LINE 2: Aligned Checkbox Placement
		// Render the actual check-box natively shifted by the dynamic text width (textSize.cx)
		RECT rcBox = { checkBoxX, currentY, checkBoxX + 13, currentY + 13 };
		DrawFrameControl(hdc, &rcBox, DFC_BUTTON, boxState);

		SetTextColor(hdc, g_Settings.clrTxt);
		TextOutA(hdc, checkBoxX + 18, currentY, "Find", 4);

		// Advance currentY based on Line 2's measured height
		currentY += LINE_H;

		// LINE 3: Mission Type
		if (g_MishBoardData.bHilightType[m]) SetTextColor(hdc, g_Settings.clrMatch);
		else SetTextColor(hdc, g_Settings.clrTxt);
		TextOutA(hdc, BORDER_W2, currentY, txt_type, (int)strlen(txt_type));

		//GetTextExtentPoint32A(hdc, txt_type, (int)strlen(txt_type), &textSize);
		currentY += LINE_H;

		// LINE 4: Location
		if (g_MishBoardData.bHilightLoc[m]) SetTextColor(hdc, g_Settings.clrMatch);
		else SetTextColor(hdc, g_Settings.clrTxt);
		TextOutA(hdc, BORDER_W2, currentY, txt_loc, (int)strlen(txt_loc));

		// 3. Paint the static sunken border placeholder frame for item icons
		// We leave the box at a clean fixed position on the right side of the card
		RECT rc48 = { 180 - 2, frame_top + 12 - 2, 228 + 2, frame_top + 60 + 2 };
		DrawEdge(hdc, &rc48, EDGE_SUNKEN, BF_RECT);

		// --- REPAINT ACCELERATED ICONS ENGINES NATIVELY INSTANTLY ---
		if (g_MishBoardData.bHasIcon[m] && g_MishBoardData.hMishIconBmp[m] != NULL)
		{
			HDC hdcMem = CreateCompatibleDC(hdc);
			if (hdcMem) {
				HGDIOBJ hOldBmp = SelectObject(hdcMem, g_MishBoardData.hMishIconBmp[m]);
				BitBlt(hdc, 180, frame_top + 12, 48, 48, hdcMem, 0, 0, SRCCOPY);
				SelectObject(hdcMem, hOldBmp);
				DeleteDC(hdcMem);
			}
		}

		// Calculate column sizing grid boundaries for Rewards Items
		if (useSingleColumnMode) {
			col_w = windowWidth - NEW_GRID_START_X - 25;
		}
		else {
			col_w = (windowWidth - NEW_GRID_START_X - 20) / 3;
			if (col_w < 140) col_w = 140;
		}

		// 4. LOOP AND REPLICATE REWARD ITEM SLOTS DYNAMICALLY
		int start_grid_y = frame_top + BORDER_H;

		if (g_MishBoardData.szMishLoc[m][0] != '\0') {
			itemsToRender = g_MishBoardData.iMishItemCount[m];
			if (useSingleColumnMode && itemsToRender > 2) itemsToRender = 2;
		}
		else {
			itemsToRender = 1;
		}

		if (itemsToRender > MAX_ITEMS) itemsToRender = MAX_ITEMS;

		for (i = 0; i < itemsToRender; i++) {
			int col, row, cell_x, cell_y;
			RECT rcLineTop, rcLineBottom;
			char line_top[64];
			char line_bot[256];

			if (useSingleColumnMode) {
				col = 0;
				row = i;
			}
			else {
				col = i / 2;
				row = i % 2;
			}

			cell_x = NEW_GRID_START_X + (col * col_w);

			// Measure a test string using the active font to compute dynamic column spacing row jumps
			GetTextExtentPoint32A(hdc, "QL: 000 Val: 000000", 20, &textSize);
			int rowHeightShift = (textSize.cy * 2) + 16; // Double the text height + padding
			cell_y = start_grid_y + (row * rowHeightShift);

			if (g_MishBoardData.szMishLoc[m][0] != '\0') {
				if (g_Settings.bShowXPCR) {
					sprintf_s(line_top, sizeof(line_top), "QL: %d  Val: %d", g_MishBoardData.iMishItemQL[m][i], g_MishBoardData.iMishItemVal[m][i]);
					sprintf_s(line_bot, sizeof(line_bot), "%s", g_MishBoardData.szMishItemName[m][i]);
				}
				else{
					sprintf_s(line_top, sizeof(line_top), "QUALITY: %d", g_MishBoardData.iMishItemQL[m][i]);
					sprintf_s(line_bot, sizeof(line_bot), "%s", g_MishBoardData.szMishItemName[m][i]);
				}
			}
			else {
				sprintf_s(line_top, sizeof(line_top), "QL:  -----");
				sprintf_s(line_bot, sizeof(line_bot), "-");
			}

			// Paint Top Metadata Row
			if (g_MishBoardData.szMishLoc[m][0] != '\0' && g_MishBoardData.bHilightItemVal[m][i]) {
				SetTextColor(hdc, g_Settings.clrMatch);
			}
			else {
				SetTextColor(hdc, g_Settings.clrTxt);
			}
			rcLineTop.left = cell_x; rcLineTop.top = cell_y;
			rcLineTop.right = cell_x + col_w - 5; rcLineTop.bottom = cell_y + textSize.cy;
			DrawTextA(hdc, line_top, -1, &rcLineTop, DT_LEFT | DT_SINGLELINE);

			// Paint Bottom Description Row
			if (g_MishBoardData.szMishLoc[m][0] != '\0' && g_MishBoardData.bHilightItemName[m][i]) {
				SetTextColor(hdc, g_Settings.clrMatch);
			}
			else {
				SetTextColor(hdc, g_Settings.clrTxt);
			}
			rcLineBottom.left = cell_x; rcLineBottom.top = cell_y + textSize.cy + 2;
			rcLineBottom.right = cell_x + col_w - 5; rcLineBottom.bottom = cell_y + (textSize.cy * 2) + 4;
			DrawTextA(hdc, line_bot, -1, &rcLineBottom, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
		}

		// Render "+ X more items" text block indicator in compressed screen state
		if (useSingleColumnMode && g_MishBoardData.szMishLoc[m][0] != '\0' && g_MishBoardData.iMishItemCount[m] > 2) {
			char txt_more[64];
			sprintf_s(txt_more, sizeof(txt_more), "+ %d more items", g_MishBoardData.iMishItemCount[m] - 2);
			SetTextColor(hdc, RGB(128, 128, 128));
			TextOutA(hdc, NEW_GRID_START_X, frame_top + 68, txt_more, (int)strlen(txt_more));
		}
	}
	SelectObject(hdc, hOldFont);
}


