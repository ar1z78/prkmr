/* ============================================================================
globals.h - Shared Program Definitions and Configuration Layer
============================================================================ */
#pragma once
#include <windows.h>

// CENTRALIZED CAPACITIES FOR TRACKING POOLS
#define MAX_WATCH_ITEMS       7400
#define MAX_WATCH_LOCATIONS    600

// Centralized Configuration Struct Mapping directly to resource.h IDs
typedef struct {

	// --- Basic Timing Configurations ---
	int dwWaitTime;             // IDC_SET_EDIT_WAIT

	// --- Core Application Checkbox Flags (0 = Unchecked, 1 = Checked) ---
	int bStartMinimized;       // IDC_SET_CHK_MINIMIZED
	int bAlertBox;             // IDC_SET_CHK_ALERT
	int bSelectMatch;          // IDC_SET_CHK_SELECT
	int bSounds;               // IDC_SET_CHK_SOUNDS
	int bLogging;              // IDC_SET_CHK_LOGGING
	int bShowHelp;             // IDC_SET_CHK_HELP
	int bAutoExpand;           // IDC_SET_CHK_EXPAND

	/* --- Mission Sliders Numerical Array (Value Bounds: 0 to 100) ---
	Using an array allows us to map them sequentially in a clean loop:
	Sliders[0] = Easy/Hard      (IDC_SET_EDIT_EASY)
	Sliders[1] = Good/Bad       (IDC_SET_EDIT_GOOD)
	Sliders[2] = Order/Chaos    (IDC_SET_EDIT_ORDER)
	Sliders[3] = Open/Hidden    (IDC_SET_EDIT_OPEN)
	Sliders[4] = Phys/Myst      (IDC_SET_EDIT_PHYS)
	Sliders[5] = Head On/Stealth(IDC_SET_EDIT_HEAD)
	Sliders[6] = Money/XP       (IDC_SET_EDIT_MONEY) */
	int Sliders[7];

	// --- Item Value Properties ---
	int iBuyMod;               // IDC_SET_EDIT_BUYMOD (Default: 7)
	int bMatchSingle;          // IDC_SET_CHK_MATCH_SINGLE
	unsigned int iMatchSingleVal;       // IDC_SET_EDIT_MATCH_SINGLE
	int bMatchTotal;           // IDC_SET_CHK_MATCH_TOTAL
	unsigned int iMatchTotalVal;        // IDC_SET_EDIT_MATCH_TOTAL

	// --- New Search UI Checkbox Configurations ---
	int bItemBuyAgent;         // IDC_SEARCH_ITEM_CHK_BUY
	int bItemHighlight;        // IDC_SEARCH_ITEM_CHK_HIGH
	int bLocBuyAgent;          // IDC_SEARCH_LOC_CHK_BUY
	int bLocHighlight;         // IDC_SEARCH_LOC_CHK_HIGH

	int bMatchRepair;          // IDC_SEARCH_CHK_REPAIR
	int bMatchReturn;          // IDC_SEARCH_CHK_RETURN
	int bMatchPerson;          // IDC_SEARCH_CHK_PERSON
	int bMatchItem;            // IDC_SEARCH_CHK_ITEM
	int bMatchKill;            // IDC_SEARCH_CHK_KILL

	int bMishBuyAgent;       // IDC_SEARCH_MISH_CHK_BUY
	int bMishlHighlight;      // IDC_SEARCH_MISH_CHK_HIGH
	// --- NEW CENTRALIZED AUTOMATION UTILITY TRACKERS ---
	int bGuiOnTop;				// Set Window to stay in top of other windows
	unsigned int iMaxTries;    // Maximum rolling attempts before giving up (replaces old PUL box)
	unsigned int iMaxMishes;   // Target number of successful missions to roll and click
	// --- NEW COLOR & FONT VARIABLES ---
	unsigned long clrBg;        // IDC_SET_EDIT_COLOR_BG (Natively stored COLORREF value)
	unsigned long clrBtn;       // IDC_SET_EDIT_COLOR_BTN (Natively stored COLORREF value)
	unsigned long clrTxt;       // IDC_SET_EDIT_COLOR_TXT (Natively stored COLORREF value)
	unsigned long clrMatch;     // IDC_SET_EDIT_COLOR_MATCH (Natively stored COLORREF value)
	unsigned long clrIconBg;    // IDC_SET_EDIT_COLOR_ICONBG (Natively stored COLORREF value)
	char szFontName[256];       // IDC_SET_EDIT_FONTNAME (Default: "MS Shell Dlg")
	int  iFontSize;            // IDC_SET_EDIT_FONTSIZE (Valid: 8 to 16)
	int  bFontBold;            // IDC_SET_CHK_FONTBOLD (0 = Normal, 1 = Bold)

} PROGRAM_SETTINGS;

typedef struct {
	char szMishLoc[5][256];      // Live location text for all 5 slots
	char szMishType[5][256];     // Live mission type text
	int iMishXP[5];              // Live XP reward values
	int iMishCash[5];            // Live Cash reward values
	int iMishTotal[5];           // Live Total calculated credit value
	BOOL bMishIsFind[5];         // True if target item was found

	int iMishItemCount[5];       // Active count of items drawn per slot
	int iMishItemQL[5][6];       // 5 slots x 6 max items QL trackers
	int iMishItemVal[5][6];      // 5 slots x 6 max items Value trackers
	char szMishItemName[5][6][256]; // 5 slots x 6 max items Names

	HBITMAP hMishIconBmp[5]; // Pre-compiled image handles
	BOOL bHasIcon[5];

	// --- CENTRALIZED HIGH-LIGHT COLOR STATE FLAGS ---
	BOOL bHilightLoc[5];           // Controls color of szMishLoc
	BOOL bHilightType[5];          // Controls color of szMishType
	BOOL bHilightTotal[5];         // Controls color of iMishTotal

	BOOL bHilightItemVal[5][6];    // Controls color of iMishItemVal
	BOOL bHilightItemName[5][6];   // Controls color of szMishItemName
} MISSION_BOARD_DATA;


// Dedicated memory buffer structures for runtime arrays
typedef struct {
	int itemWatchCount;
	char itemWatchList[MAX_WATCH_ITEMS][256];

	int locWatchCount;
	char locWatchList[MAX_WATCH_LOCATIONS][256];
} SEARCH_WATCHLISTS;

typedef struct {
	char text[128]; // Persistent buffer for tracking owner-drawn strings safely
} MenuStringData;

typedef struct {
	int winX;
	int winY;
	int winW;
} winpos;

// Expose the settings instance as a visible external global across files
#ifdef __cplusplus
extern "C" {
#endif
	extern HWND g_hwndMishBoard; // external reference for the main window
	extern HWND g_hwndSearch; // external reference for the Search window
	extern winpos g_MainWndPos;
	extern PROGRAM_SETTINGS g_Settings;
	extern SEARCH_WATCHLISTS g_WatchLists; // Separate instance block
	extern MISSION_BOARD_DATA g_MishBoardData;

	// Expose static menu strings to the compiler linker stage
	extern MenuStringData mnuFile;
	extern MenuStringData mnuWindows;
	extern MenuStringData mnuExit;
	extern MenuStringData mnuSearch;
	extern MenuStringData mnuSettings;

	void CreateMainMenu(HWND hwnd); // Export function prototype safely
	// UI 
	INT_PTR HandleSharedStaticColor(WPARAM wp);
	INT_PTR HandleSharedEditColor(WPARAM wp);
	BOOL    DrawSharedFlatButton(LPDRAWITEMSTRUCT pDrawItem);
	void    ApplySharedClassBackground(HWND hwnd);
	INT_PTR HandleSharedDlgColor(void);
	BOOL    DrawSharedFlatCheckbox(LPDRAWITEMSTRUCT pDrawItem);
#ifdef __cplusplus
}
#endif