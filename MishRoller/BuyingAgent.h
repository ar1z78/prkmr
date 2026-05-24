#ifndef BUYING_AGENT_H
#define BUYING_AGENT_H

#define BUYINGAGENT_TIMER 1

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	// 1. Correct functional signatures matching the integer return profiles
	int BuyingAgent(void);
	void EndBuyingAgent(void);

	// 2. FIX: Convert raw definitions to safe global external references
	extern HWND hMainWnd;
	extern UINT_PTR g_TimerID;

	// Slider setup and hardware simulation helper functions
	void _dragMouse(int x0, int y0, int x1, int y1);
	float _linIinterp(float lo, float hi, float ratio);
	void _setSliders(int easy_hard, int good_bad, int order_chaos, int open_hidden, int phys_myst, int headon_stealth, int money_xp);

#ifdef __cplusplus
}
#endif

#endif // BUYING_AGENT_H
