#ifndef __MISHROLLER_H__
#define __MISHROLLER_H__

#define MR_VERSION "1.0.0"

#include <windows.h>
#include "mission.h"
#include "sqlite3.h"

// Active App Messages
//#define MRAM_QUIT                    (WM_USER + 101)
// #define MRAM_SKIP                 (WM_USER + 102)
//#define MRAM_OK                      (WM_USER + 103)
//#define MRAM_CANCEL                  (WM_USER + 104)
#define MRAM_NEWMISSIONS              (WM_USER + 105)
#define MRAM_BUYINGAGENT_DOMISSION   (WM_USER + 106)
// #define MRAM_PRESTARTBUYINGAGENT  (WM_USER + 107)
#define MRAM_STARTBUYINGAGENT        (WM_USER + 108)
#define MRAM_STOPBUYINGAGENT         (WM_USER + 109)
//#define MRAM_STARTFULLSCREEN         (WM_USER + 110)
//#define MRAM_STOPFULLSCREEN          (WM_USER + 111)
// #define MRAM_EXPORTSETTINGS       (WM_USER + 112)
// #define MRAM_IMPORTSETTINGS       (WM_USER + 113)
// #define MRAM_SET_SLIDERS          (WM_USER + 114)

#ifdef __cplusplus
extern "C" {
#endif

	extern unsigned long g_ItemWatchList;
	//extern unsigned long g_LocWatchList;
	//extern unsigned long g_TypeWatchList;

	// REMOVED: Old 'g_pCol' context array descriptor line 

	extern unsigned long g_BuyingAgentCount;

	// REMOVED: Legacy PUL Window identity integer marker. 
	// Use 'g_hwndMishBoard' from globals.h instead.

	extern unsigned char g_MishNumber, g_FoundMish;
	extern unsigned char g_bFullscreen;
	extern char g_AODir[MAX_PATH];
	extern unsigned long g_BuyingAgentMissions;
	extern unsigned long g_bFirstRound;



	unsigned long MissionParse(unsigned long _Object, MissionClassData* _pData, unsigned char* _pMissionData);
	void EndBuyingAgent(void);
	void WriteLog(const char* format, ...);
	void _setSliders(int easy_hard, int good_bad, int order_chaos, int open_hidden, int phys_myst, int headon_stealth, int money_xp);

#ifdef __cplusplus
}
#endif

// Endianness macros
#define EndianSwap16(x) ( ( x ) >> 8 | ( x ) << 8 )
#define EndianSwap32(x) ( ( x ) << 24 | ( ( x ) & 0xff00 ) << 8 | ( ( x ) >> 8 ) & 0xff00 | ( x ) >> 24 )

// REMOVED OLD PUL MACROS (They will break compilation since g_pCol is gone)
// #define PUL_GET_CB(x) ...
// #define PUL_SET_CB(x,y) ...

// Database functions
int OpenLocalDB();
void ReleaseAODatabase();
void* GetDataChunk(unsigned long _KeyHi, unsigned long _KeyLo, unsigned long* _pSize);

#endif
