#include "globals.h"
#include "BuyingAgent.h"
#include "MishRoller.h"
#include <commctrl.h>
#include "coreutils.h"
#pragma comment(lib, "comctl32.lib")


HWND hMainWnd = NULL;      // Allocates the true shared handle memory space
UINT_PTR g_TimerID = 0;    // Allocates the single unified global timer variable


// Generate a mouse movement and button click sequence
// to make AO generate new missions.
// All coordinates are hardcoded because I'm too
// lazy to make them configurable.
// However, it's not that much of a problem, since
// I use coordinates relative to AO window.
// So, it will work regardless of where the AO window is,
// and with the mission window in AO snapped to the
// upper left corner.
int BuyingAgent(void)
{
	// Set a timer that will post a WM_TIMER message to the main window 
	// after the delay set in settings 

	int delay = g_Settings.dwWaitTime;
	g_TimerID = SetTimer(g_hwndMishBoard, BUYINGAGENT_TIMER, delay, NULL);
	if (g_TimerID == 0)
	{
		//DisplayErrorMessage("Failed to create timer.", TRUE);
		return FALSE;
	}
	return TRUE;
}


void EndBuyingAgent(void)
{
	if (!g_bFullscreen)
	{
		// Remove keyboard focus

		// Close buying agent window

		// Open main window
	}
}


//slider setting functions

void _dragMouse(int x0, int y0, int x1, int y1)
{
	POINT MousePos;
	LPARAM lParam;
	HWND AOWnd;

	// Find AO window
	if (!(AOWnd = FindWindow("Anarchy client", NULL)))
	{
		ShowErrorMessage( "Anarchy Online is not running." );
		g_BuyingAgentCount = 0;
		g_BuyingAgentMissions = 0;
		return;
	}
	MousePos.x = x0;
	MousePos.y = y0;
	lParam = MousePos.y << 16 | MousePos.x;
	ClientToScreen(AOWnd, &MousePos);
	SetCursorPos(MousePos.x, MousePos.y);
	SendMessage(AOWnd, WM_LBUTTONDOWN, 0, lParam);
	Sleep(50); //was 250
	MousePos.x = x1;
	MousePos.y = y1;
	lParam = MousePos.y << 16 | MousePos.x;
	ClientToScreen(AOWnd, &MousePos);
	SetCursorPos(MousePos.x, MousePos.y);
	SendMessage(AOWnd, WM_MOUSEMOVE, 0, lParam);
	Sleep(50);
	SendMessage(AOWnd, WM_LBUTTONUP, 0, lParam);
	Sleep(50);
}


/*
these coords are from my initial observation with a macro program. they are ofset slightly.
; options button
; 200, 185
; difficulty slider
; 110, 165
; 1st slider
; 110, 210
; then add 15 pixels in Y for each subsequent row
; full left slider
; X = 60
; full right slider
; X = 170
*/


float _linIinterp(float lo, float hi, float ratio)
{
	return (hi - lo)*ratio + lo;
}


void _setSliders(int easy_hard, int good_bad, int order_chaos, int open_hidden, int phys_myst, int headon_stealth, int money_xp)
{
	int ypos = 210;

	//_dragMouse(200, 165, 200, 165);
	if (easy_hard != 50) _dragMouse(102, 160, (int)_linIinterp(64, 141, easy_hard / 100.0f), 160);
	if (good_bad != 50) _dragMouse(102, ypos, (int)_linIinterp(64, 141, good_bad / 100.0f), ypos);
	ypos += 18;
	if (order_chaos != 50) _dragMouse(102, ypos, (int)_linIinterp(64, 141, order_chaos / 100.0f), ypos);
	ypos += 18;
	if (open_hidden != 50) _dragMouse(102, ypos, (int)_linIinterp(64, 141, open_hidden / 100.0f), ypos);
	ypos += 18;
	if (phys_myst != 50) _dragMouse(102, ypos, (int)_linIinterp(64, 141, phys_myst / 100.0f), ypos);
	ypos += 18;
	if (headon_stealth != 50) _dragMouse(102, ypos, (int)_linIinterp(64, 141, headon_stealth / 100.0f), ypos);
	ypos += 18;
	if (money_xp != 50) _dragMouse(102, ypos, (int)_linIinterp(64, 141, money_xp / 100.0f), ypos);
}
