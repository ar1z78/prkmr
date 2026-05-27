#include "missions_UI.h"
#include <commctrl.h>
#include "resource.h"
#include "globals.h"
#include "MishRoller.h" // CRITICAL: Includes  newly converted MRAM_ defines!
#include "coreutils.h"
#include "buyingagent.h"
#include "ImpExp.h"


extern HFONT g_hFont; // Pulled dynamically from main data objects
void CreateMainMenu(HWND hwnd);
void CreateSettingsWindow(HWND hwndParent);
void ShowSettingsWindow(HWND hwndParent);
void CreateSearchWindow(HWND hwndParent);
void ShowSearchWindow(HWND hwndParent);

char g_CurrentPacket[65536];
extern HANDLE g_Mutex;
extern MissionClassData g_MissionSlots; // Tracks the 5 raw parsed mission records


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE: {
						g_hwndMishBoard = hwnd;
						HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
						InitCommonControls();

						// One clean call completely creates and skins owner-draw menu
						CreateMainMenu(hwnd);
						char szTriesBuf[32];
						char szMishBuf[32];

						// Group Box Header Frame
						g_hwndTriesLbl = CreateWindowA("BUTTON", "Mission Rolling Agent", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 545, 425, 80, hwnd, NULL, hInst, NULL);
						SendMessageW(g_hwndTriesLbl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

						// 1. Format loaded settings numbers directly into separate string buffers
						sprintf_s(szTriesBuf, sizeof(szTriesBuf), "%u", g_Settings.iMaxTries);
						sprintf_s(szMishBuf, sizeof(szMishBuf), "%u", g_Settings.iMaxMishes);

						// 2. Create the labels normally
						g_hwndTriesLbl = CreateWindowA("STATIC", "Maximum Tries:", WS_VISIBLE | WS_CHILD, 25, 570, 120, 20, hwnd, NULL, hInst, NULL);

						// 3. Create the Edit box and pass 'szTriesBuf' DIRECTLY as the initial text parameter!
						g_hwndTriesEdit = CreateWindowA("EDIT", szTriesBuf, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 130, 568, 45, 22, hwnd, (HMENU)105, hInst, NULL);

						g_hwndMishLbl = CreateWindowA("STATIC", "# of Mish:", WS_VISIBLE | WS_CHILD, 190, 567, 70, 20, hwnd, NULL, hInst, NULL);

						// 4. Create the second Edit box passing 'szMishBuf' DIRECTLY as the initial text parameter!
						g_hwndMishEdit = CreateWindowA("EDIT", szMishBuf, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 260, 565, 45, 22, hwnd, (HMENU)106, hInst, NULL);


						g_hwndStartAgent = CreateWindowA("BUTTON", "Start Agent", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 325, 560, 90, 24, hwnd, (HMENU)101, hInst, NULL);
						g_hwndStopAgent = CreateWindowA("BUTTON", "Stop Agent", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 325, 590, 90, 24, hwnd, (HMENU)102, hInst, NULL);

						g_hwndFullscreen = CreateWindowA("BUTTON", "Stay ON TOP", WS_VISIBLE | WS_CHILD| BS_AUTOCHECKBOX, 25, 595, 120, 24, hwnd, (HMENU)103, hInst, NULL);
						SendMessageA(g_hwndFullscreen, BM_SETCHECK, g_Settings.bGuiOnTop ? BST_CHECKED : BST_UNCHECKED, 0);

						g_hwndRemoveDups = CreateWindowA("BUTTON", "Set Sliders Now", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 180, 595, 120, 22, hwnd, (HMENU)104, hInst, NULL);



						// Apply the global color background dynamically
						ApplySharedClassBackground(hwnd);
						break;
	}

	case WM_CTLCOLORDLG:
		return HandleSharedDlgColor();

	case WM_CTLCOLORSTATIC:
		return HandleSharedStaticColor(wParam);

	case WM_CTLCOLOREDIT:
		return HandleSharedEditColor(wParam);
	case WM_MEASUREITEM: {
							 LPMEASUREITEMSTRUCT lpMeasure = (LPMEASUREITEMSTRUCT)lParam;
							 if (lpMeasure->CtlType == ODT_MENU) {
								 // Set standard comfortable padding heights and item cell box dimensions
								 lpMeasure->itemHeight = 22; // 22 pixels tall for clean font spacing clearance
								 lpMeasure->itemWidth = 60; // 120 pixels generic base item layout boundary width
								 return TRUE;
							 }
							 break;
	}
	case WM_DRAWITEM: {
						  LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT)lParam;
						  if (pDrawItem->CtlID == 101 || pDrawItem->CtlID == 102 || pDrawItem->CtlID == 104)
						  {
							  return DrawSharedFlatButton(pDrawItem);
						  }
						  if (pDrawItem->CtlType == ODT_MENU) {
							  HBRUSH hBackBrush;
							  RECT rcText = pDrawItem->rcItem;

							  // Cast itemData directly back to the shared global MenuStringData layout
							  MenuStringData* pItem = (MenuStringData*)pDrawItem->itemData;

							  // Correct hex color transformations (0xRRGGBB -> Win32 0xBBGGRR)
							  // FIXED: Use native GDI extraction macros to pull the absolute RGB integers
							  COLORREF cBg = RGB(GetRValue(g_Settings.clrBg), GetGValue(g_Settings.clrBg), GetBValue(g_Settings.clrBg));
							  COLORREF cBtn = RGB(GetRValue(g_Settings.clrBtn), GetGValue(g_Settings.clrBtn), GetBValue(g_Settings.clrBtn));
							  COLORREF cTxt = RGB(GetRValue(g_Settings.clrTxt), GetGValue(g_Settings.clrTxt), GetBValue(g_Settings.clrTxt));


							  if (pDrawItem->itemState & ODS_SELECTED) {
								  hBackBrush = CreateSolidBrush(cBtn);
								  SetTextColor(pDrawItem->hDC, cTxt);
							  }
							  else {
								  hBackBrush = CreateSolidBrush(cBg);
								  SetTextColor(pDrawItem->hDC, cTxt);
							  }

							  FillRect(pDrawItem->hDC, &pDrawItem->rcItem, hBackBrush);
							  DeleteObject(hBackBrush);

							  SetBkMode(pDrawItem->hDC, TRANSPARENT);
							  HGDIOBJ hOldFont = SelectObject(pDrawItem->hDC, g_hFont);

							  rcText.left += 12;

							  if (pItem != NULL && pItem->text[0] != '\0') {
								  DrawTextA(pDrawItem->hDC, pItem->text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
							  }

							  SelectObject(pDrawItem->hDC, hOldFont);

							  return TRUE;
						  }
						  break;
	}



	case WM_COMMAND: {
						 int wmId = LOWORD(wParam);
						 int wmEvent = HIWORD(wParam); // Extract the high-word notification code

						 /* ====================================================================
						 LIVE UI-TO-STRUCT INPUT FIELD DATA SYNCHRONIZATION
						 ==================================================================== */
						 if (wmEvent == EN_CHANGE) {
							 char szBuffer[32] = { 0 };

							 if (wmId == 105) { // Maximum Tries Edit Box changed
								 GetWindowTextA(g_hwndTriesEdit, szBuffer, sizeof(szBuffer)-1);

								 // FIX: Only convert and save if the edit box actually contains text
								 if (szBuffer[0] != '\0') {
									 g_Settings.iMaxTries = (int)strtoul(szBuffer, NULL, 10);
								 }
								 return 0;
							 }
							 else if (wmId == 106) { // # of Mish Edit Box changed
								 GetWindowTextA(g_hwndMishEdit, szBuffer, sizeof(szBuffer)-1);

								 // FIX: Protect against the empty string creation-trap here too
								 if (szBuffer[0] != '\0') {
									 g_Settings.iMaxMishes = (int)strtoul(szBuffer, NULL, 10);
								 }
								 return 0;
							 }
						 }

						 // CATCH THE TOOLBAR MENU CLICKS HERE
						 switch (wmId) {
						 case ID_WINDOWS_SETTINGS:
							 ShowSettingsWindow(hwnd); // Opens  settings UI panel
							 return 0;

						 case ID_WINDOWS_SEARCH:
							 ShowSearchWindow(hwnd);   // Opens  search configuration panel
							 return 0;

						 case ID_FILE_EXIT:
							 SendMessage(hwnd, WM_CLOSE, 0, 0);       // Cleanly shuts down the app
							 return 0;

							 // INTERCEPT THE BOTTOM INTERACTIVE CONTROL BUTTON CLICKS NATIVELY
						 case 101: // "Start Buying Agent" Button
							 PostMessage(hwnd, MRAM_STARTBUYINGAGENT, 0, 0);
							 return 0;

						 case 102: // "Stop" Button
							 PostMessage(hwnd, MRAM_STOPBUYINGAGENT, 0, 0);
							 return 0;

						 case 103: 
							 if (HIWORD(wParam) == BN_CLICKED)
							 {
								 // Scoped block wrapper prevents local variable definition errors in switch statements
								 LRESULT checkState = SendMessageA((HWND)lParam, BM_GETCHECK, 0, 0);

								 if (checkState == BST_CHECKED)
								 {
									 g_Settings.bGuiOnTop = 1; // Update settings flag
									 // Checkbox is checked: Force window to the top
									 SetWindowPos(g_hwndMishBoard, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
								 }
								 else
								 {
									 g_Settings.bGuiOnTop = 0; // Update settings flag
									 // Checkbox is unchecked: Return window to normal layout
									 SetWindowPos(g_hwndMishBoard, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
								 }
							 }
						 
							 return 0;

						 case 104: // "set Sliders" Button
							 _setSliders(g_Settings.Sliders[0], g_Settings.Sliders[1], g_Settings.Sliders[2], g_Settings.Sliders[3], g_Settings.Sliders[4], g_Settings.Sliders[5], g_Settings.Sliders[6]);
							 return 0;
						 }
						 break;
	}

	case WM_TIMER: {
					   if (wParam == BUYINGAGENT_TIMER) {
						   // 1. Kill the timer instantly so it doesn't fire repeatedly
						   KillTimer(hwnd, BUYINGAGENT_TIMER);
						   g_TimerID = 0;

						   // 2. Post the application event straight into our local message queue
						   PostMessage(hwnd, MRAM_BUYINGAGENT_DOMISSION, 0, 0);
						   return 0;
					   }
					   break;
	}
		/* ====================================================================
		CATCH THE BACKGROUND EVENT NOTIFICATIONS DISPATCHED FROM HOOK.CPP
		==================================================================== */
	case MRAM_STOPBUYINGAGENT: {
								   // Kill any pending background waiting timers safely
								   if (g_TimerID) {
									   KillTimer(hwnd, g_TimerID); // FIX: targeting 'hwnd' natively
									   g_TimerID = 0;
								   }
								   g_BuyingAgentCount = 0;
								   g_BuyingAgentMissions = 0;
								   EndBuyingAgent();

								   // Fall through into execution layer cleanly
	}

	case MRAM_BUYINGAGENT_DOMISSION: {
										 // Send the click if we have attempts remaining
										 if (g_BuyingAgentCount > 0)
										 {
											 // Find AO window
											 HWND AOWnd = FindWindowA("Anarchy client", NULL);
											 if (AOWnd)
											 {
												 DWORD foregroundThreadID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
												 DWORD targetThreadID = GetWindowThreadProcessId(AOWnd, NULL);

												 // Attach threads to reliably steal focus across windows
												 if (foregroundThreadID != targetThreadID) {
													 AttachThreadInput(foregroundThreadID, targetThreadID, TRUE);
												 }

												 ShowWindow(AOWnd, SW_RESTORE);
												 SetForegroundWindow(AOWnd);
												 SetFocus(AOWnd);

												 if (foregroundThreadID != targetThreadID) {
													 AttachThreadInput(foregroundThreadID, targetThreadID, FALSE);
												 }

												 // Hardcoded relative coordinates for the in-game terminal button
												 POINT MousePos = { 99, 180 };
												 LPARAM lParam = (MousePos.y << 16) | MousePos.x;

												 if (g_bFirstRound) {
													 ClientToScreen(AOWnd, &MousePos);
													 SetCursorPos(MousePos.x, MousePos.y);
													 g_bFirstRound = FALSE;
												 }

												 // Fire physical mouse actions into the client execution stream
												 Sleep(50);
												 SendMessageA(AOWnd, WM_LBUTTONDOWN, 0, lParam);
												 Sleep(50);
												 SendMessageA(AOWnd, WM_LBUTTONUP, 0, lParam);
												 // Decrement counter AFTER the click action finishes
												 g_BuyingAgentCount--;
											 }
											 else
											 {
												 ShowErrorMessage("Anarchy Online is not running.");
												 g_BuyingAgentCount = 0;
												 g_BuyingAgentMissions = 0;
												 return FALSE;
											 }

											 // ONLY queue the next timer cycle if we have remaining attempts!
											 if (g_BuyingAgentCount > 0) {
												 BuyingAgent();
											 }
										 }
										 return 0;
	}


	case MRAM_NEWMISSIONS: {
							   void* pMissionData = g_CurrentPacket;
							   char szBuffer[16] = { 0 };

							   // 1. EXTRACT AUTOMATION ATTEMPTS COUNTER
							   if (!g_BuyingAgentCount && g_bFullscreen) {
								   GetWindowTextA(g_hwndTriesEdit, szBuffer, sizeof(szBuffer)-1);
								   g_BuyingAgentCount = strtoul(szBuffer, NULL, 10);
							   }

							   // 2. SINGLE UNIFIED PARSING PASS (Replaces both previous loops)
							   WaitForSingleObject(g_Mutex, INFINITE);
							   g_FoundMish = 255;
							   for (g_MishNumber = 0; g_MishNumber < 5; g_MishNumber++) {
								   void* pLastData = pMissionData;
								   // FIXED: Removed [g_MishNumber] to pass the singular struct instance address safely
								   pMissionData = (void*)MissionParse(0, &g_MissionSlots, (unsigned char*)pMissionData);
								   if (!pMissionData) {
									   pMissionData = pLastData; // Fallback to last valid pointer on error
									   break;
								   }
							   }
							   ReleaseMutex(g_Mutex);


							   // 3. AUTOMATION STATE MANAGEMENT
							   //if (g_BuyingAgentCount) {
								 //  if (g_BuyingAgentCount) BuyingAgent(); else EndBuyingAgent();
							   //}

							   // 4. RENDERING & UI REFRESH
							   if (g_hwndMishBoard) {
								   RedrawWindow(g_hwndMishBoard, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
								   if (pMissionData && !g_bFullscreen && IsIconic(g_hwndMishBoard)) {
									   ShowWindow(g_hwndMishBoard, SW_RESTORE);
								   }
							   }

							   // 5. AUDIO ALERTS
							   if (g_Settings.bSounds) {
								   PlaySoundA((g_FoundMish == 255) ? "notfound.wav" : "found.wav", NULL, SND_FILENAME | SND_NODEFAULT | SND_ASYNC);
							   }

							   // 6. MACRO AUTOMATION (MOUSE CLICKS & KEISTROKES)
							   if ((g_Settings.bSelectMatch || g_BuyingAgentMissions >= 0) && g_FoundMish != 255 && msg != MRAM_STOPBUYINGAGENT) {
								   HWND AOWnd = FindWindowA("Anarchy client", NULL);
								   if (!AOWnd) {
									   ShowErrorMessage("Project Rubi-Ka is not running.");
									   g_BuyingAgentCount = 0;
								   }
								   else {
									   POINT MousePos;
									   LPARAM clickParam;

									   // Target the matched mission slot
									   MousePos.x = 44 + ((g_FoundMish % 3) * 58);
									   MousePos.y = 57 + ((g_FoundMish / 3) * 57);
									   clickParam = (MousePos.y << 16) | MousePos.x;

									   ClientToScreen(AOWnd, &MousePos);
									   SetCursorPos(MousePos.x, MousePos.y);
									   SendMessageA(AOWnd, WM_LBUTTONDOWN, 0, clickParam); Sleep(50);
									   SendMessageA(AOWnd, WM_LBUTTONUP, 0, clickParam);   Sleep(250);

									   // Click "Accept" button
									   MousePos.x = 76; MousePos.y = 321;
									   clickParam = (MousePos.y << 16) | MousePos.x;
									   ClientToScreen(AOWnd, &MousePos);
									   SetCursorPos(MousePos.x, MousePos.y);
									   // We matched our final goal mission! Clicked it.
									   if (g_BuyingAgentMissions > 0){
										   // Target and click the "Accept" button
										   SendMessageA(AOWnd, WM_LBUTTONDOWN, 0, clickParam); Sleep(50);
										   SendMessageA(AOWnd, WM_LBUTTONUP, 0, clickParam);   Sleep(250);
										   // Hit 'E' to open the item/mission terminal again
										   SendMessageA(AOWnd, WM_KEYDOWN, 0x45, 0); Sleep(50);
										   SendMessageA(AOWnd, WM_KEYUP, 0x45, 0);   Sleep(250);
										   //decrease Missions Important
										   g_BuyingAgentMissions--;
									   }
									   // Decide whether to chain another round or stop cleanly
									   if (g_BuyingAgentMissions > 0) {

										   _setSliders(g_Settings.Sliders[0], g_Settings.Sliders[1], g_Settings.Sliders[2],
											   g_Settings.Sliders[3], g_Settings.Sliders[4], g_Settings.Sliders[5], g_Settings.Sliders[6]);

										   g_bFirstRound = TRUE;
										   GetWindowTextA(g_hwndTriesEdit, szBuffer, sizeof(szBuffer)-1);
										   g_BuyingAgentCount = strtoul(szBuffer, NULL, 10);

										   BuyingAgent();
										   
									   }
									   else {
										   
										   //EndBuyingAgent();
										   g_BuyingAgentCount = 0;
									   }

								   }
							   }
							   WriteLog(NULL);
							   return 0;
	}




	case MRAM_STARTBUYINGAGENT: {
									if (g_Settings.bShowHelp)
									{
										ShowInfoMessage("The buying agent will generate missions from the terminal automatically "
											"until it finds a mission that matches your search criteria. "
											"You have to open the mission terminal window and put it in the upper left corner "
											"before starting the buying agent. "
											"Be sure to set up a reasonable amount of maximum number tries before starting.");
									}

									{
										unsigned long bItemListOk = FALSE, bLocListOk = FALSE, bTypeListOk = FALSE;
										unsigned long bWarnItem, bWarnLoc, bWarnType;
										unsigned long bReadyToGo = FALSE;

										// Make sure that there's something in the relevant watch list
										bWarnItem = g_Settings.bItemBuyAgent;
										bWarnLoc = g_Settings.bLocBuyAgent;
										bWarnType = g_Settings.bMishBuyAgent;

										if (g_WatchLists.itemWatchCount > 0) {
											bItemListOk = TRUE;
										}

										if (g_WatchLists.locWatchCount > 0) {
											bLocListOk = TRUE;
										}

										if (g_Settings.bMatchRepair || g_Settings.bMatchPerson || g_Settings.bMatchItem || g_Settings.bMatchReturn || g_Settings.bMatchKill)
										{
											bTypeListOk = TRUE;
										}

										bReadyToGo = bWarnLoc || bWarnItem || bWarnType;
										if (bWarnItem) bReadyToGo = bReadyToGo && bItemListOk;
										if (bWarnLoc)  bReadyToGo = bReadyToGo && bLocListOk;
										if (bWarnType) bReadyToGo = bReadyToGo && bTypeListOk;

										if (bReadyToGo)
										{
											// FIX: Replaced old UI collection value pulls with native edit fields data extraction
											g_BuyingAgentMissions = g_Settings.iMaxMishes;
											g_BuyingAgentCount = g_Settings.iMaxTries;
											g_bFirstRound = TRUE;
											// Sent Message to start Rolling Agent
											PostMessage(hwnd, MRAM_BUYINGAGENT_DOMISSION, 0, 0);
										}
										else
										{
											ShowInfoMessage("Invalid configuration: Nothing selected to match or empty watchlists.");
										}
									}
									return 0;
	}


	case WM_GETMINMAXINFO: {
							   MINMAXINFO* mmi = (MINMAXINFO*)lParam;
							   mmi->ptMinTrackSize.x = 450;
							   mmi->ptMaxTrackSize.x = 1024;
							   mmi->ptMinTrackSize.y = 685;
							   mmi->ptMaxTrackSize.y = 685;
							   return 0;
	}

	case WM_SIZE: {
					  int wWidth = LOWORD(lParam);
					  int wHeight = HIWORD(lParam);
					  int bottomY1 = wHeight - 68;
					  int bottomY2 = wHeight - 35;

					  InvalidateRect(hwnd, NULL, TRUE);
					  return 0;
	}
	case WM_PAINT: {
					   PAINTSTRUCT ps;
					   HDC hdc = BeginPaint(hwnd, &ps);
					   DrawGuiMissions(hwnd, hdc);
					   EndPaint(hwnd, &ps);
					   return 0;
	}
	case WM_CLOSE: {
					   // 1. Force the working directory back to the MishRoller tool directory
					   // (Requires g_MRDir to be visible via MishRoller.h or an extern declaration)
					   extern char g_MRDir[MAX_PATH];
					   SetCurrentDirectory(g_MRDir);

					   // 2. Export the settings safely right now while the window is still 100% alive
					   ExportSettings("LastSettings.mr");

					   // 3. Manually signal the window to destroy itself and fire the next event
					   DestroyWindow(hwnd);
					   return 0;
	}
	case WM_DESTROY: {
						 int m;
						 // Wipe out active cached bitmap handles to prevent massive system resource leaks
						 for (m = 0; m < MAX_MISSIONS; m++) {
							 if (g_MishBoardData.hMishIconBmp[m] != NULL) {
								 DeleteObject(g_MishBoardData.hMishIconBmp[m]);
								 g_MishBoardData.hMishIconBmp[m] = NULL;
							 }
						 }
//						 FreeOwnerDrawMenuMemory(GetMenu(hwnd));
						 if (g_hFont) DeleteObject(g_hFont);
						 PostQuitMessage(0);
						 return 0;
	}

	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
