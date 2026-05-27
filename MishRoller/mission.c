#include "Platform.h"
#include "Globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "aomd.h"
#include "MishRoller.h"
#include "rdb.h"
#include "CoreUtils.h"
#include "missions_UI.h"

// Turn on this define if you want to output the mission packets in Hex..
//#define DEBUG_MISSION_PACKETS 1

MissionClassData g_MissionSlots; // Allocates memory safely for the parsed details

unsigned long MissionParse( unsigned long _Object, MissionClassData* _pData, unsigned char* _pMissionData );
unsigned long ShowItem( MissionClassData* _pData, Item* _pItem, unsigned long _ObjId, unsigned long _ValId, int j );
unsigned long SetAndSearch(unsigned char* _pSrcString, unsigned long _TextEntry, int bIsItem);
unsigned long SetAndSearchType( unsigned long TempVal, unsigned long _TextEntry );
unsigned long ItemMatch( unsigned char* ItemName, unsigned char* ItemSearch );
unsigned long LocationMatch( unsigned char* LocationName, unsigned char* LocationSearch );

void DrawMishIcon(HWND hwnd, int mIdx, int frameTop, unsigned char* pImgData)
{
	HDC hdc;
	BITMAPINFO bmi;
	void* pBits = NULL;
	HBITMAP hNewBmp = NULL;

	// Safety checks
	if (!pImgData || mIdx < 0 || mIdx >= MAX_MISSIONS) return;

	hdc = GetDC(hwnd);
	if (!hdc) return;

	// Metadata header setup
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 48;
	bmi.bmiHeader.biHeight = -48; // Top-down layout mapping
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	// Create an internal hardware-linked device surface image handle
	hNewBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
	ReleaseDC(hwnd, hdc);

	if (hNewBmp && pBits) {
		// 1. Copy the raw game client graphics bytes over once
		CopyMemory(pBits, pImgData, 48 * 48 * 3);

		// 2. Delete any previous old bitmap sitting in this index slot to prevent memory leak chokes
		if (g_MishBoardData.hMishIconBmp[mIdx] != NULL) {
			DeleteObject(g_MishBoardData.hMishIconBmp[mIdx]);
		}

		// 3. Cache the optimized hardware image handle in memory permanently
		g_MishBoardData.hMishIconBmp[mIdx] = hNewBmp;
		g_MishBoardData.bHasIcon[mIdx] = TRUE;
	}
}

int ExtractMissionPacketData(unsigned char* _pMissionData, unsigned char* pEndMissionData, ExtractedMissionData* pOut)
{
	unsigned long TempVal;
	unsigned char* pWorkingData = _pMissionData;
	Item* pTmpItem;

	// Initialize the structure cleanly
	memset(pOut, 0, sizeof(ExtractedMissionData));
	strcpy_s(pOut->CharKey, sizeof(pOut->CharKey), "");

	// 1. Synchronize to the packet magic word header
	do
	{
		if (pWorkingData >= pEndMissionData)
		{
			return 0; // Out of bounds
		}
		pWorkingData++;
		TempVal = *(unsigned long*)pWorkingData;
		TempVal = EndianSwap32(TempVal);
	} while (TempVal != 0xdac3);

	pOut->MishID = *(unsigned long*)pWorkingData + 0x04;
	pOut->MishID = EndianSwap32(pOut->MishID);

	// 2. Skip header and calculate short description safely
	pWorkingData += 24;
	pOut->DescLength = (unsigned long)strnlen((const char*)pWorkingData, (size_t)(pEndMissionData - pWorkingData));

	// 3. Skip short description (+ null terminator), read full description length, and mark pDesc
	pWorkingData += pOut->DescLength + 1;
	TempVal = EndianSwap32(*(unsigned long*)pWorkingData);
	pWorkingData += 4;
	pOut->pDesc = pWorkingData;

	// 4. Skip full description
	pWorkingData += TempVal;
	pOut->DescLength = TempVal; // Hold full description length for later string matching

	// 5. Hard structural safety checks
	if (pWorkingData > pEndMissionData || (pEndMissionData - pWorkingData) < 0xE8)
	{
		return 0; // Truncated or malformed block
	}

	// 6. Extract financial and item layout data relative to post-description position
	pOut->Cash = *(unsigned long*)(pWorkingData + 0xc);
	pOut->Cash = EndianSwap32(pOut->Cash);

	pOut->XP = *(unsigned long*)(pWorkingData + 0x14);
	pOut->XP = EndianSwap32(pOut->XP);

	// Track original location of item array start
	pOut->pItemStart = (Item*)(pWorkingData + 0x24);
	pTmpItem = pOut->pItemStart;

	// 7. Loop item structures exactly like your original routine
	while (pTmpItem->Key1 != 0x2d2d2d2d)
	{
		MissionItem sItem;
		if (!GetAODBItem(&sItem, EndianSwap32(pTmpItem->Key1)))
		{
			strncpy_s(pOut->CharKey, sizeof(pOut->CharKey), (char *)&(pTmpItem->Padding), 4);
			pOut->CharKey[4] = 0;
			break;
		}

		if (pOut->NumItems < 6)
		{
			pOut->CollectedIconKeys[pOut->NumItems] = sItem.IconKey;
		}

		pOut->NumItems++;
		pTmpItem++;

		if (pEndMissionData < (unsigned char*)pTmpItem)
		{
			return 0; // Memory boundary overrun guard
		}
	}

	// 8. Advance working pointer past items to read relative visual properties
	pWorkingData = ((unsigned char*)pTmpItem) + 4;
	if (pWorkingData >= pEndMissionData)
	{
		return 0;
	}

	// 9. Extract Mission details relative to post-items layout pointer
	pOut->MishQL = *(unsigned long*)(pWorkingData + 0xc);
	pOut->MishQL = EndianSwap32(pOut->MishQL);

	pOut->MissionType = *(unsigned long*)(pWorkingData + 0x28);
	pOut->MissionType = EndianSwap32(pOut->MissionType);

	pOut->MishPF = *(unsigned long*)(pWorkingData + 0xA8);
	pOut->MishPF = EndianSwap32(pOut->MishPF);

	TempVal = *(unsigned long*)(pWorkingData + 0xb4);
	TempVal = EndianSwap32(TempVal);
	*(unsigned long*)(&pOut->CoordX) = TempVal;

	TempVal = *(unsigned long*)(pWorkingData + 0xbc);
	TempVal = EndianSwap32(TempVal);
	*(unsigned long*)(&pOut->CoordY) = TempVal;

	// Return exact pointer delta to advance downstream reader engines
	return (int)(pWorkingData - _pMissionData);
}


unsigned long MissionParse(unsigned long _Object, MissionClassData* _pData, unsigned char* _pMissionData)
{
	char TempStr[256];
	char PFName[256];
	unsigned long TotalValue;
	unsigned long bAlertItem, bAlertLoc, bAlertType;
	unsigned long bItemFound = FALSE, bLocFound = FALSE, bAlert = FALSE, bTypeFound = FALSE;
	unsigned long i;
	RECT MWRect;
	Item* pItem;

	unsigned char* pEndMissionData = _pMissionData + 65536 - 4;
	ExtractedMissionData missionData;

	bAlertItem = g_Settings.bItemBuyAgent;
	bAlertLoc = g_Settings.bLocBuyAgent;
	bAlertType = g_Settings.bMishBuyAgent;

	if (g_WatchLists.itemWatchCount == 0) bAlertItem = FALSE;
	if (g_WatchLists.locWatchCount == 0)  bAlertLoc = FALSE;

	// Execute the separate packet processing step safely
	int bytesParsed = ExtractMissionPacketData(_pMissionData, pEndMissionData, &missionData);
	if (bytesParsed == 0)
	{
		return 0; // Failed extraction or check boundaries breached
	}

	// Keep pointer alignment synchronized for subsequent engine calls
	unsigned char* pFinalMissionData = _pMissionData + bytesParsed;

	// Handle automated board panel size expansion
	GetWindowRect(g_hwndMishBoard, &MWRect);
	int current_height = MWRect.bottom - MWRect.top;
	int Wwidth = g_MainWndPos.winW;

	if (missionData.NumItems > 2 && g_Settings.bAutoExpand) Wwidth = 750;
	SetWindowPos(g_hwndMishBoard, NULL, 0, 0, Wwidth, current_height, SWP_NOMOVE | SWP_NOZORDER);

	// Composite graphic multi-tile array creation block
	if (missionData.NumItems > 0)
	{
		int tilesToProcess = (missionData.NumItems > 6) ? 6 : missionData.NumItems;

		if (_pData->pImageData)
		{
			free(_pData->pImageData);
			_pData->pImageData = NULL;
		}

		_pData->pImageData = GetAOIconDataGrid(missionData.CollectedIconKeys, tilesToProcess);
		DrawMishIcon(g_hwndMishBoard, g_MishNumber, 0, (unsigned char*)_pData->pImageData);
	}

	// Assign text display attributes
	sprintf_s(_pData->CashStr, sizeof(_pData->CashStr), "%u", missionData.Cash);
	sprintf_s(_pData->XPStr, sizeof(_pData->XPStr), "%u", missionData.XP);
	g_MishBoardData.iMishCash[g_MishNumber] = missionData.Cash;
	g_MishBoardData.iMishXP[g_MishNumber] = missionData.XP;
	TotalValue = missionData.Cash;

	// Evaluate Playfields and Positioning Coordinates
	MissionPF(missionData.MishPF, PFName);
	sprintf_s(TempStr, sizeof(TempStr), "%s (%.1f, %.1f)", PFName, missionData.CoordX, missionData.CoordY);

	bLocFound = SetAndSearch((unsigned char*)TempStr, 0, FALSE);
	strcpy_s(g_MishBoardData.szMishLoc[g_MishNumber], sizeof(g_MishBoardData.szMishLoc[g_MishNumber]), (char*)TempStr);

	// Evaluate core mission types safely using verified value
	bTypeFound = SetAndSearchType(missionData.MissionType, 0);

	// Record data trace to debug logs
	WriteLog("mission\t%u\t%u\t%u\t%u\t%s\n", missionData.MishID, missionData.MishQL, missionData.XP, missionData.Cash, missionData.CharKey);
	WriteLog("loc\t%u\t%.1f\t%.1f\t%s\n", missionData.MishPF, missionData.CoordX, missionData.CoordY, PFName);

	// Process reward item tables sequentially
	pItem = missionData.pItemStart;
	for (i = 0; i < missionData.NumItems; i++)
	{
		g_MishBoardData.iMishItemCount[g_MishNumber] = missionData.NumItems;
		bItemFound |= ShowItem(_pData, pItem++, i + ITEM1, i + ITEMVAL1, i);
		TotalValue += _pData->Reward.Value * g_Settings.iBuyMod / 100;
	}

	g_MishBoardData.iMishTotal[g_MishNumber] = TotalValue;
	if (TotalValue > g_Settings.iMatchTotalVal)
	{
		g_MishBoardData.bHilightTotal[g_MishNumber] = TRUE;
		if (g_Settings.bMatchTotal) bItemFound = TRUE;
	}
	else
	{
		g_MishBoardData.bHilightTotal[g_MishNumber] = FALSE;
	}

	// Safe multi-watchlist lowercase scan through full descriptions
	unsigned char* pLowerDesc = malloc(missionData.DescLength + 1);
	if (pLowerDesc)
	{
		for (i = 0; i < missionData.DescLength; i++) {
			unsigned char c = missionData.pDesc[i];
			pLowerDesc[i] = (c >= 'A' && c <= 'Z') ? (c + 0x20) : c;
		}
		pLowerDesc[missionData.DescLength] = 0;

		unsigned long idx;
		unsigned long bFindItemInDesc = FALSE;

		for (idx = 0; idx < (unsigned long)g_WatchLists.itemWatchCount; ++idx)
		{
			unsigned char* pWatchItem = (unsigned char*)g_WatchLists.itemWatchList[idx];
			if (pWatchItem && *pWatchItem)
			{
				if (strstr((char*)pLowerDesc, (char*)pWatchItem) != NULL)
				{
					bFindItemInDesc = TRUE;
					break;
				}
			}
		}

		g_MishBoardData.bMishIsFind[g_MishNumber] = bFindItemInDesc;
		bItemFound |= bFindItemInDesc;
		free(pLowerDesc);
	}

	// Consolidated filter evaluation engine
	bAlert = bAlertItem || bAlertLoc || bAlertType;

	if (bAlertItem) bAlert = bAlert && bItemFound;
	if (bAlertLoc)  bAlert = bAlert && bLocFound;
	if (bAlertType) bAlert = bAlert && bTypeFound;

	if (bAlert)
	{
		if (g_FoundMish == 255) g_FoundMish = g_MishNumber;
		if (g_BuyingAgentCount)
		{
			g_BuyingAgentCount = 0;
		}
	}

	return (unsigned long)pFinalMissionData;
}



unsigned long ShowItem(MissionClassData* _pData, Item* _pItem, unsigned long _ObjId, unsigned long _ValID, int j)
{
	unsigned long ItemKey1, ItemKey2, QL;
	unsigned long bItemFound = FALSE;
	char TempStr[256];

	ItemKey1 = _pItem->Key1;
	ItemKey1 = EndianSwap32(ItemKey1);
	ItemKey2 = _pItem->Key2;
	ItemKey2 = EndianSwap32(ItemKey2);
	QL = _pItem->QL;
	QL = EndianSwap32(QL);

	if (ItemKey1 == 0x6af2 && ItemKey2 == 0x6af3)
	{
		// Mission with no item reward, clear item records
		g_MishBoardData.iMishItemQL[g_MishNumber][j] = 0;
		g_MishBoardData.iMishItemVal[g_MishNumber][j] = 0;
		g_MishBoardData.szMishItemName[g_MishNumber][j][0] = '\0';
		g_MishBoardData.bHilightItemVal[g_MishNumber][j] = FALSE;
		g_MishBoardData.bHilightItemName[g_MishNumber][j] = FALSE;
	}
	else
	{
		GetMissionItem(&_pData->Reward, ItemKey1, ItemKey2, QL);
		WriteLog("reward\t%u\t%u\t%u\t%s\n", ItemKey1, ItemKey2, QL, _pData->Reward.pName);

		// Display item name and ql
		sprintf_s(TempStr, sizeof(TempStr), "QL%u %s", QL, _pData->Reward.pName);

		// FIX: Pass the item index 'j' into SetAndSearch to track the specific highlight slot
		bItemFound = SetAndSearch((unsigned char*)TempStr, j, TRUE);

		g_MishBoardData.iMishItemQL[g_MishNumber][j] = QL;
		g_MishBoardData.iMishItemVal[g_MishNumber][j] = _pData->Reward.Value * g_Settings.iBuyMod / 100;

		strcpy_s(g_MishBoardData.szMishItemName[g_MishNumber][j], sizeof(g_MishBoardData.szMishItemName[g_MishNumber][j]), (char*)_pData->Reward.pName);

		if (_pData->Reward.Value * g_Settings.iBuyMod / 100 > g_Settings.iMatchSingleVal)
		{
			g_MishBoardData.bHilightItemVal[g_MishNumber][j] = TRUE;
			if (g_Settings.bMatchSingle) bItemFound = TRUE;
		}
		else
		{
			g_MishBoardData.bHilightItemVal[g_MishNumber][j] = FALSE;
		}
	}

	return bItemFound;
}



unsigned long SetAndSearchType(unsigned long TempVal, unsigned long _TextEntry)
{
	unsigned char match = 0;
	unsigned char TempStr[50];
	switch (TempVal)
	{
	case 0x2c4e:
		sprintf_s((char*)TempStr, sizeof(TempStr), "Repair");
		if (g_Settings.bMatchRepair) match = 1;
		break;

	case 0x26add: // PRK new id
		sprintf_s((char*)TempStr, sizeof(TempStr), "Return Item");
		if (g_Settings.bMatchReturn) match = 1;
		break;

	case 0x2c47:
		sprintf_s((char*)TempStr, sizeof(TempStr), "Find Person");
		if (g_Settings.bMatchPerson) match = 1;
		break;

	case 0x2c49:
		sprintf_s((char*)TempStr, sizeof(TempStr), "Find Item");
		if (g_Settings.bMatchItem) match = 1;
		break;

	case 0x2c42:
		sprintf_s((char*)TempStr, sizeof(TempStr), "Kill Person");
		if (g_Settings.bMatchKill) match = 1;
		break;

	default:
		sprintf_s((char*)TempStr, sizeof(TempStr), "Unknown: 0x%08X - Please report", TempVal);
		break;
	}

	strcpy_s(g_MishBoardData.szMishType[g_MishNumber], sizeof(g_MishBoardData.szMishType[g_MishNumber]), (char*)TempStr);
	g_MishBoardData.bHilightType[g_MishNumber] = match && g_Settings.bMishlHighlight;

	return match;
}



/* Set string to a textentry control, search for it in a list,
and hilight the textentry if the string was found.
*/
/* Set string, evaluate matching criteria against active watchlist pools,
and highlight the active layer if a search term matches.
*/
unsigned long SetAndSearch(unsigned char* _pSrcString, unsigned long itemIndex, int bIsItem)
{
	unsigned long idx;
	unsigned char TmpItemName[256];
	unsigned char c;
	unsigned char* pChar;

	// Convert item name to lower case
	pChar = TmpItemName;
	do
	{
		c = *_pSrcString++;
		if (c >= 'A' && c <= 'Z')
		{
			*pChar++ = c + 0x20;
		}
		else
		{
			*pChar++ = c;
		}
	} while (c);

	// --- ITEM WATCHLIST LOOKUP ---
	if (bIsItem)
	{
		// First reset the highlight status for this item slot
		g_MishBoardData.bHilightItemName[g_MishNumber][itemIndex] = FALSE;

		for (idx = 0; idx < (unsigned long)g_WatchLists.itemWatchCount; ++idx)
		{
			if (ItemMatch(TmpItemName, (unsigned char*)g_WatchLists.itemWatchList[idx]))
			{
				g_MishBoardData.bHilightItemName[g_MishNumber][itemIndex] = g_Settings.bItemHighlight;
				return TRUE;
			}
		}
	}
	// --- LOCATION WATCHLIST LOOKUP ---
	else
	{
		g_MishBoardData.bHilightLoc[g_MishNumber] = FALSE;

		for (idx = 0; idx < (unsigned long)g_WatchLists.locWatchCount; ++idx)
		{
			if (LocationMatch(TmpItemName, (unsigned char*)g_WatchLists.locWatchList[idx]))
			{
				g_MishBoardData.bHilightLoc[g_MishNumber] = g_Settings.bLocHighlight;
				return TRUE;
			}
		}
	}

	return FALSE;
}



/*******************************
Item Search, to allow web-search-like constructors "<text>", -<text> and
word-match
Examples:
Searching for 'decus -gloves' will match all decus items except gloves;
Searching for 'decus armor' will match on 'decus body armor', and 'decus
armor boots'
Searching for '"decus armor"' will match on 'decus armor boots' but not on
'decus body armor'
Searching for '"primus decus" -gloves -boots -body' will match on all primus
decus armor except for gloves, boots and body
********************************/
unsigned long ItemMatch( unsigned char* ItemName, unsigned char* ItemSearch )
{
    unsigned char TmpString[ 256 ];
    unsigned char* pChar;
    unsigned char c, OpenQuoteFlag, ExcludeFlag, HadValidString = FALSE;

    do
    {
        pChar = TmpString;
        OpenQuoteFlag = FALSE;
        ExcludeFlag = FALSE;

        do
        {
            c = *ItemSearch++;

            if( c >= 'A' && c <= 'Z' )
            {
                *pChar++ = c + 0x20;
            }
            else if( c == '"' )
            {
                if( OpenQuoteFlag )
                {
                    *pChar++ = 0;
                    OpenQuoteFlag = FALSE;
                }
                else
                {
                    OpenQuoteFlag = TRUE;
                }
            }
            else if( c == '-' && pChar == TmpString )
            {
                ExcludeFlag = TRUE;
            }
            else if( c != ' ' || OpenQuoteFlag )
            {
                *pChar++ = c;
            }
            else
            {
                *pChar++ = 0;
            }

        }
        while( c && !( c == ' ' && !OpenQuoteFlag ) );

        if( strlen( TmpString ) )
        {
            HadValidString = TRUE;

            if( ExcludeFlag )
            {
                if( strstr( ItemName, TmpString ) )
                {
                    return FALSE;
                }
            }
            else
            {
                if( !strstr( ItemName, TmpString ) )
                {
                    return FALSE;
                }
            }
        }
    }
    while( c );

    return HadValidString;
}


/*******************************
Location Search, to allow as above, plus location range search
Examples:
Searching for 'athen -shire' will match on 'west athens' and 'old athens'
Searching for 'athen (100-200,500-600)' will match on any athem mission with
coords x from 100 to 200, y from 500 to 600.
Searching for 'athen (100.2,200.3)' will match on any athen mission with
coords x and y exacly 100.2 and 200.3 respectively
Searching for 'athen (0-500,3000-999999)' will match on any athen mission
with coords x <=500, y>=3000 (but less than 999999)
********************************/
unsigned long LocationMatch( unsigned char* LocationName, unsigned char* LocationSearch )
{
    unsigned char Name[ 256 ] = { 0 }, Search[ 256 ] = { 0 };
    unsigned char CoordX[ 20 ] = { 0 }, CoordY[ 20 ] = { 0 };
    unsigned char SearchCoordXFrom[ 20 ] = { 0 }, SearchCoordXTo[ 20 ] = { 0 };
    unsigned char SearchCoordYFrom[ 20 ] = { 0 }, SearchCoordYTo[ 20 ] = { 0 };
    unsigned char *pChar;
    unsigned char c, OpenBracketFlag = FALSE, YCoordFlag = FALSE;
    double x, y, xfrom, xto, yfrom, yto;

    // pull Name, CoordX, CoordY
    pChar = Name;
    do
    {
        c = *LocationName++;

        if( c == '(' )
        {
            *pChar = 0;
            OpenBracketFlag = TRUE;
            pChar = CoordX;
        }
        else if( c == ',' && OpenBracketFlag )
        {
            *pChar = 0;
            pChar = CoordY;
        }
        else
        {
            *pChar++ = c;
        }
    }
    while( c );

    // pull Search, SearchCoordXFrom, SearchCoordXTo, SearchCoordYFrom, SearchCoordYTo
    OpenBracketFlag = FALSE;
    pChar = Search;
    do
    {
        c = *LocationSearch++;

        if( c == '(' )
        {
            *pChar = 0;
            OpenBracketFlag = TRUE;
            pChar = SearchCoordXFrom;
        }
        else if( c == ',' && OpenBracketFlag )
        {
            *pChar = 0;
            YCoordFlag = TRUE;
            pChar = SearchCoordYFrom;
        }
        else if( c == '-' && OpenBracketFlag )
        {
            if( YCoordFlag )
            {
                *pChar = 0;
                pChar = SearchCoordYTo;
            }
            else
            {
                *pChar = 0;
                pChar = SearchCoordXTo;
            }
        }
        else
        {
            *pChar++ = c;
        }
    }
    while( c );

    // compare LocationName to LocationSearch
    if( ItemMatch( Name, Search ) )
    {

        // if matched, compare coordtinates
        x = atof( CoordX );
        y = atof( CoordY );
        xfrom = atof( SearchCoordXFrom );
        xto = atof( SearchCoordXTo );
        yfrom = atof( SearchCoordYFrom );
        yto = atof( SearchCoordYTo );

        if( x > 0 && y > 0 && xfrom > 0 && yfrom > 0 )
        { // carry on only if we have mission location
            if( ( x >= xfrom && ( x <= xto || !xto ) ) && ( y >= yfrom && ( y <= yto || !yto ) ) )
            {
                return TRUE; // loc name matched, coords matched
            }
            else
            {
                return FALSE; // loc name matched; coonds wrong
            }
        }
        else
        {
            return TRUE; // loc name matched; won't compare coords
        }
    }

    return FALSE; // loc name didn't match
}
