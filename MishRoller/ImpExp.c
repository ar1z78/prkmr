#include "ImpExp.h"
#include <stdlib.h>
#include <string.h>
#include "MishRoller.h" 
#include "globals.h"

typedef enum ImportSettingsMode
{
	ISM_CONFIG,
	ISM_LOCWATCH,
	ISM_ITEMWATCH,
	ISM_SLIDERS,
	ISM_DONE,
} ImportSettingsMode;

enum
{
	CFG_AODIR,
	CFG_WINDOWX,
	CFG_WINDOWY,
	CFG_WINDOWWIDTH,
	CFG_STARTMINIMIZED,
	CFG_WATCHMSGBOX,
	CFG_ALERTITEM,
	CFG_ALERTLOC,
	CFG_ALERTTYPE,
	CFG_ALWAYSONTOP,
	CFG_BUYINGAGENTMAXTRIES,
	CFG_BUYINGAGENTMAXMISHES,
	CFG_BUYINGAGENTHIDE,
	CFG_BUYINGAGENTSHOWHELP,
	CFG_MISSIONTYPES,
	CFG_HIGHLIGHTOPTS,
	CFG_SOUNDS,
	CFG_LOG,
	CFG_MOUSEMOVE,
	CFG_EXPAND,
	CFG_ITEMVALUE,

	CFG_SLIDER_EASY_HARD,
	CFG_SLIDER_GOOD_BAD,
	CFG_SLIDER_ORDER_CHAOS,
	CFG_SLIDER_OPEN_HIDDEN,
	CFG_SLIDER_PHYS_MYST,
	CFG_SLIDER_HEADON_STEALTH,
	CFG_SLIDER_MONEY_XP,

	CFG_BUYMOD,
	CFG_SHOWXPCR,
	CFG_ROLLWAIT,
	CFG_CUSTOM_COLORS,
	CFG_CUSTOM_FONT,
};


struct
{
	int id;
	char* keyword;
} CfgKeywords[] =
{
	{ CFG_AODIR, "AODIR" },
	{ CFG_SOUNDS, "SOUNDS" },
	{ CFG_MOUSEMOVE, "MOUSEMOVE" },
	{ CFG_LOG, "LOG" },
	{ CFG_WINDOWX, "WINDOWX" },
	{ CFG_WINDOWY, "WINDOWY" },
	{ CFG_WINDOWWIDTH, "WINDOWWIDTH" },
	{ CFG_STARTMINIMIZED, "STARTMINIMIZED" },
	{ CFG_WATCHMSGBOX, "WATCHMSGBOX" },
	{ CFG_BUYINGAGENTSHOWHELP, "BUYINGAGENTSHOWHELP" },
	{ CFG_ALERTLOC, "ALERTLOC" },
	{ CFG_ALERTITEM, "ALERTITEM" },
	{ CFG_ALERTTYPE, "ALERTTYPE" },
	{ CFG_ALWAYSONTOP, "ALWAYSONTOP" },
	{ CFG_BUYINGAGENTMAXTRIES, "BUYINGAGENTMAXTRIES" },
	{ CFG_BUYINGAGENTMAXMISHES, "BUYINGAGENTMAXMISHES" },
	{ CFG_BUYINGAGENTHIDE, "BUYINGAGENTHIDE" },
	{ CFG_MISSIONTYPES, "MISHTYPES" },
	{ CFG_HIGHLIGHTOPTS, "HIGHLIGHTOPTS" },
	{ CFG_EXPAND, "EXPAND" },

	{ CFG_SLIDER_EASY_HARD, "SLIDER_EASY_HARD" },
	{ CFG_SLIDER_GOOD_BAD, "SLIDER_GOOD_BAD" },
	{ CFG_SLIDER_ORDER_CHAOS, "SLIDER_ORDER_CHAOS" },
	{ CFG_SLIDER_OPEN_HIDDEN, "SLIDER_OPEN_HIDDEN" },
	{ CFG_SLIDER_PHYS_MYST, "SLIDER_PHYS_MYST" },
	{ CFG_SLIDER_HEADON_STEALTH, "SLIDER_HEADON_STEALTH" },
	{ CFG_SLIDER_MONEY_XP, "SLIDER_MONEY_XP" },

	{ CFG_ITEMVALUE, "ITEMVALUE" },

	{ CFG_BUYMOD, "BUYMOD" },
	{ CFG_SHOWXPCR, "SHOWXPCR" },
	{ CFG_ROLLWAIT, "ROLLWAIT" },
	{ CFG_CUSTOM_COLORS, "COLORS" },
	{ CFG_CUSTOM_FONT, "FONT" },
	{ 0, NULL }
};

/* ============================================================================
Safely relocates and resizes the main MishRoller window layout canvas
============================================================================ */

void ExportSettings(char* filename)
{
	FILE* fp;
	RECT rc = { 0 };
	//unsigned long Record;
	//unsigned char* pString;
	unsigned int Val = 0;
	char* myfilename;

	myfilename = malloc(strlen(filename) + 5);
	strcpy(myfilename, filename);

	if (!strstr(myfilename, ".mr"))
	{
		strcat(myfilename, ".mr");
	}

	if (!(fp = fopen(myfilename, "w")))
	{
		free(myfilename);
		return;
	}
	free(myfilename);
	fprintf(fp, "::Config::\n");
	fprintf(fp, "AODIR::%s\n", g_AODir);

	// Ask for Window rectacle
	if (g_hwndMishBoard != NULL && IsWindow(g_hwndMishBoard))
	{
		GetWindowRect(g_hwndMishBoard, &rc);
	}
	// Win32 RECT stores Left, Top, Right, Bottom. 
	// Width is calculated by subtracting Left from Right.
	g_MainWndPos.winX = rc.left;
	g_MainWndPos.winY = rc.top;
	g_MainWndPos.winW = rc.right - rc.left;

	fprintf(fp, "WINDOWX::%d\nWINDOWY::%d\nWINDOWWIDTH::%d\n", g_MainWndPos.winX, g_MainWndPos.winY, g_MainWndPos.winW);

	fprintf(fp, "STARTMINIMIZED::%u\n", g_Settings.bStartMinimized);

	fprintf(fp, "WATCHMSGBOX::%u\n", g_Settings.bAlertBox);

	fprintf(fp, "BUYINGAGENTSHOWHELP::%u\n", g_Settings.bShowHelp);

	fprintf(fp, "SOUNDS::%u\n", g_Settings.bSounds);

	fprintf(fp, "EXPAND::%u\n", g_Settings.bAutoExpand);

	fprintf(fp, "MOUSEMOVE::%u\n", g_Settings.bSelectMatch);

	fprintf(fp, "LOG::%u\n", g_Settings.bLogging);

	fprintf(fp, "ALERTITEM::%u\n", g_Settings.bItemBuyAgent);

	fprintf(fp, "ALERTLOC::%u\n", g_Settings.bLocBuyAgent);

	fprintf(fp, "ALERTTYPE::%u\n", g_Settings.bMishBuyAgent);

	fprintf(fp, "ALWAYSONTOP::%u\n", g_Settings.bGuiOnTop);

	fprintf(fp, "BUYINGAGENTMAXTRIES::%u\n", g_Settings.iMaxTries);

	fprintf(fp, "BUYINGAGENTMAXMISHES::%u\n", g_Settings.iMaxMishes);

	if (g_Settings.bMatchRepair) Val |= 0x01;
	if (g_Settings.bMatchReturn) Val |= 0x02;
	if (g_Settings.bMatchPerson) Val |= 0x04;
	if (g_Settings.bMatchItem) Val |= 0x08;
	if (g_Settings.bMatchKill) Val |= 0x10;

	fprintf(fp, "MISHTYPES::%u\n", Val);

	Val = 0;
	if (g_Settings.bItemHighlight) Val |= 0x01;
	if (g_Settings.bLocHighlight) Val |= 0x02;
	if (g_Settings.bMishlHighlight) Val |= 0x04;

	fprintf(fp, "HIGHLIGHTOPTS::%u\n", Val);

	fprintf(fp, "SLIDER_EASY_HARD::%u\n", g_Settings.Sliders[0]);

	fprintf(fp, "SLIDER_GOOD_BAD::%u\n", g_Settings.Sliders[1]);

	fprintf(fp, "SLIDER_ORDER_CHAOS::%u\n", g_Settings.Sliders[2]);

	fprintf(fp, "SLIDER_OPEN_HIDDEN::%u\n", g_Settings.Sliders[3]);

	fprintf(fp, "SLIDER_PHYS_MYST::%u\n", g_Settings.Sliders[4]);

	fprintf(fp, "SLIDER_HEADON_STEALTH::%u\n", g_Settings.Sliders[5]);

	fprintf(fp, "SLIDER_MONEY_XP::%u\n", g_Settings.Sliders[6]);

	fprintf(fp, "BUYMOD::%u\n", g_Settings.iBuyMod);

	fprintf(fp, "SHOWXPCR::%u\n", g_Settings.bShowXPCR);

	fprintf(fp, "ITEMVALUE::%u::%u::%u::%u\n", g_Settings.bMatchTotal, g_Settings.iMatchTotalVal, g_Settings.bMatchSingle, g_Settings.iMatchSingleVal);

	fprintf(fp, "ROLLWAIT::%u\n", g_Settings.dwWaitTime);

	// --- NEW COLORS & FONT EXPORTS ---
	fprintf(fp, "COLORS::%lu::%lu::%lu::%lu::%lu\n", g_Settings.clrBg, g_Settings.clrBtn, g_Settings.clrTxt, g_Settings.clrMatch, g_Settings.clrIconBg);
	fprintf(fp, "FONT::%s::%u::%u\n", g_Settings.szFontName, g_Settings.iFontSize, g_Settings.bFontBold);

	// --- EXPORT ITEM WATCHLIST ARRAYS ---
	fprintf(fp, "::ItemWatch::\n");
	{
		int idx;
		for (idx = 0; idx < g_WatchLists.itemWatchCount; ++idx)
		{
			if (g_WatchLists.itemWatchList[idx][0] != '\0')
			{
				fprintf(fp, "%s\n", g_WatchLists.itemWatchList[idx]);
			}
		}
	}

	// --- EXPORT LOCATION WATCHLIST ARRAYS ---
	fprintf(fp, "::LocWatch::\n");
	{
		int idx;
		for (idx = 0; idx < g_WatchLists.locWatchCount; ++idx)
		{
			if (g_WatchLists.locWatchList[idx][0] != '\0')
			{
				fprintf(fp, "%s\n", g_WatchLists.locWatchList[idx]);
			}
		}
	}
	fprintf(fp, "::END::\n");


	fclose(fp);
}

void ImportSettings(char* filename)
{
	FILE* fp;
	unsigned char* pString;
	char buffer[1000];
	unsigned char Keyword[256], Value[256];
	int Id, i;
	unsigned long Val;
	int mode = ISM_DONE;
	char c;



	g_MainWndPos.winX = CW_USEDEFAULT;
	g_MainWndPos.winY = CW_USEDEFAULT;
	g_MainWndPos.winW = 320;
	// Wipe out old counts so the file reloads data cleanly from row index 0
	g_WatchLists.itemWatchCount = 0;
	g_WatchLists.locWatchCount = 0;
	// Pre-set defaults in case a partial line is read
	g_Settings.clrBg = (unsigned long)RGB(240, 240, 240);
	g_Settings.clrBtn = (unsigned long)RGB(224, 224, 224);
	g_Settings.clrTxt = (unsigned long)RGB(0, 0, 0);
	g_Settings.clrMatch = (unsigned long)RGB(255, 0, 0);
	g_Settings.clrIconBg = (unsigned long)RGB(0, 0, 0);
	strcpy(g_Settings.szFontName, "Tahoma");
	g_Settings.iFontSize = 9;
	g_Settings.bFontBold = 0;

	if (!(fp = fopen(filename, "r")))
	{
		return;
	}

	while (fgets(buffer, 1000, fp))
	{
		if (sscanf(buffer, "::%s", &buffer) == 1)
		{
			strtok(buffer, ":");
			if (!_stricmp(buffer, "Config")) mode = ISM_CONFIG;
			if (!_stricmp(buffer, "LocWatch")) mode = ISM_LOCWATCH;
			if (!_stricmp(buffer, "ItemWatch")) mode = ISM_ITEMWATCH;

			if (!_stricmp(buffer, "Sliders")) mode = ISM_SLIDERS;
			if (!_stricmp(buffer, "Done")) mode = ISM_DONE;
			continue;
		}
		switch (mode)
		{
		case ISM_DONE:
			break;

		case ISM_CONFIG:
			if (sscanf(buffer, "%[^:]::%[^\n]\n", Keyword, Value) != EOF)
			{
				i = 0, Id = -1;
				while (CfgKeywords[i].keyword)
				{
					if (!strcmp(Keyword, CfgKeywords[i].keyword))
					{
						Id = CfgKeywords[i].id;
						break;
					}

					i++;
				}

				switch (Id)
				{

				case CFG_AODIR:
					strcpy(g_AODir, Value);
					break;

				case CFG_WINDOWX:
					sscanf(Value, "%u", &Val);
					if (Val < 16384)
					{
						g_MainWndPos.winX = (int)Val;
					}
					break;

				case CFG_WINDOWY:
					sscanf(Value, "%u", &Val);
					if (Val < 16384)
					{
						g_MainWndPos.winY = (int)Val;
					}
					break;

				case CFG_WINDOWWIDTH:
					sscanf(Value, "%u", &Val);
					g_MainWndPos.winW = (int)Val;
					break;


				case CFG_STARTMINIMIZED:
					sscanf(Value, "%u", &Val);
					g_Settings.bStartMinimized = Val;
					break;

				case CFG_WATCHMSGBOX:
					sscanf(Value, "%u", &Val);
					g_Settings.bAlertBox = Val;
					break;

				case CFG_BUYINGAGENTSHOWHELP:
					sscanf(Value, "%u", &Val);
					g_Settings.bShowHelp = Val;
					break;

				case CFG_SOUNDS:
					sscanf(Value, "%u", &Val);
					g_Settings.bSounds = Val;
					break;

				case CFG_MOUSEMOVE:
					sscanf(Value, "%u", &Val);
					g_Settings.bSelectMatch = Val;
					break;

				case CFG_EXPAND:
					sscanf(Value, "%u", &Val);
					g_Settings.bAutoExpand = Val;
					break;

				case CFG_LOG:
					sscanf(Value, "%u", &Val);
					g_Settings.bLogging = Val;
					break;

				case CFG_ALERTITEM:
					sscanf(Value, "%u", &Val);
					g_Settings.bItemBuyAgent = Val;
					break;

				case CFG_ALERTLOC:
					sscanf(Value, "%u", &Val);
					g_Settings.bLocBuyAgent = Val;
					break;

				case CFG_ALERTTYPE:
					sscanf(Value, "%u", &Val);
					g_Settings.bMishBuyAgent = Val;
					break;

				case CFG_ALWAYSONTOP:
					sscanf(Value, "%u", &Val);
					g_Settings.bGuiOnTop = Val;
					break;

				case CFG_BUYINGAGENTMAXTRIES:
					sscanf(Value, "%u", &Val);
					g_Settings.iMaxTries = Val;
					break;

				case CFG_BUYINGAGENTMAXMISHES:
					sscanf(Value, "%u", &Val);
					g_Settings.iMaxMishes = Val;
					break;

				case CFG_MISSIONTYPES:
					sscanf(Value, "%u", &Val);
					g_Settings.bMatchRepair = Val & 0x01;
					g_Settings.bMatchReturn = Val & 0x02;
					g_Settings.bMatchPerson = Val & 0x04;
					g_Settings.bMatchItem = Val & 0x08;
					g_Settings.bMatchKill = Val & 0x10;
					break;

				case CFG_HIGHLIGHTOPTS:
					sscanf(Value, "%u", &Val);
					g_Settings.bItemHighlight = Val & 0x01;
					g_Settings.bLocHighlight = Val & 0x02;
					g_Settings.bMishlHighlight = Val & 0x04;
					break;

				case CFG_SLIDER_EASY_HARD:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[0] = Val;
				case CFG_SLIDER_GOOD_BAD:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[1] = Val;
				case CFG_SLIDER_ORDER_CHAOS:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[2] = Val;
				case CFG_SLIDER_OPEN_HIDDEN:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[3] = Val;
				case CFG_SLIDER_PHYS_MYST:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[4] = Val;
				case CFG_SLIDER_HEADON_STEALTH:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[5] = Val;
				case CFG_SLIDER_MONEY_XP:
					sscanf(Value, "%u", &Val);
					g_Settings.Sliders[6] = Val;
					break;

				case CFG_BUYMOD:
					sscanf(Value, "%u", &Val);
					g_Settings.iBuyMod = Val;
					break;

				case CFG_SHOWXPCR:
					sscanf(Value, "%u", &Val);
					g_Settings.bShowXPCR = Val;
					break;

				case CFG_ROLLWAIT:
					sscanf(Value, "%u", &Val);
					g_Settings.dwWaitTime = Val;
					break;

				case CFG_CUSTOM_COLORS:
					sscanf((char*)Value, "%lu::%lu::%lu::%lu::%lu",
						&g_Settings.clrBg, &g_Settings.clrBtn, &g_Settings.clrTxt, &g_Settings.clrMatch, &g_Settings.clrIconBg);
					break;

				case CFG_CUSTOM_FONT:
					sscanf((char*)Value, "%[^::]::%u::%u",
						g_Settings.szFontName, &g_Settings.iFontSize, &g_Settings.bFontBold);
					break;

				case CFG_ITEMVALUE:
				{
					unsigned long a, b, c, d;
					sscanf(Value, "%u::%u::%u::%u", &a, &b, &c, &d);
					g_Settings.bMatchTotal = a;
					g_Settings.iMatchTotalVal = b;
					g_Settings.bMatchSingle = c;
					g_Settings.iMatchSingleVal = d;

				}
					break;
				}

			}
			break;

		case ISM_ITEMWATCH:
		case ISM_LOCWATCH:

			pString = buffer + strlen(buffer);

			while (pString > buffer)
			{
				c = *--pString;
				if (c != ' ' && c != '\t' && c != '\n')
				{
					break;
				}
			}

			*(pString + 1) = 0;

			// Strip leading spaces/tab
			pString = buffer;

			while (c = *pString++)
			{
				if (c != ' ' && c != '\t')
				{
					break;
				}
			}

			pString--;

			// If the resulting string isn't empty, add it to the list
			if (*pString)
			{
				// --- NATIVE ARRAY REPLACEMENT ROUTINE ---
				if (mode == ISM_ITEMWATCH)
				{
					// Safely guard against buffer overflows
					if (g_WatchLists.itemWatchCount < MAX_WATCH_ITEMS)
					{
						// Copy clean string directly into the next empty array row slot
						strcpy(g_WatchLists.itemWatchList[g_WatchLists.itemWatchCount], (char*)pString);
						g_WatchLists.itemWatchCount++;
					}
				}
				else // ISM_LOCWATCH
				{
					if (g_WatchLists.locWatchCount < MAX_WATCH_LOCATIONS)
					{
						strcpy(g_WatchLists.locWatchList[g_WatchLists.locWatchCount], (char*)pString);
						g_WatchLists.locWatchCount++;
					}
				}
			}
			break;

		}
	}

	fclose(fp);
}
