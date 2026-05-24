#ifndef MISSIONS_UI_H
#define MISSIONS_UI_H

#include <windows.h>

#define MAX_ITEMS 6
#define MAX_MISSIONS 5


/* --- Global UI State References --- */
extern HFONT g_hFont;

extern const int FRAME_H;
extern const int NEW_GRID_START_X;

/* --- Footer Control Window Handles --- */

extern HWND g_hwndTriesLbl;
extern HWND g_hwndTriesEdit;
extern HWND g_hwndMishLbl;
extern HWND g_hwndMishEdit;
extern HWND g_hwndStartAgent;
extern HWND g_hwndStopAgent;
extern HWND g_hwndFullscreen;
extern HWND g_hwndRemoveDups;

/* --- Exposed UI Functions --- */

void DrawGuiMissions(HWND hwnd, HDC hdc); /* Signature accepts zero structs */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif /* MISSIONS_UI_H */

