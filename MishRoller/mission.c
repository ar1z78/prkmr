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



unsigned long MissionParse(unsigned long _Object, MissionClassData* _pData, unsigned char* _pMissionData)
{
	char TempStr[256], CharKey[6];
	char PFName[256];
	float CoordX, CoordY;
	unsigned long TempVal, MishPF;
	unsigned long Cash, XP, MishQL, MishID, TotalValue;
	//unsigned long Time;
	//unsigned long ItemKey1, ItemKey2, QL;
	unsigned long bAlertItem, bAlertLoc, bAlertType;
	unsigned long bItemFound = FALSE, bLocFound = FALSE, bAlert = FALSE, bTypeFound = FALSE;
	unsigned long Count = 65536 - 4, DescLength;
	unsigned char* pEndMissionData;
	unsigned char* pDesc;
	Item* pItem;
	Item* pTmpItem;
	unsigned long NumItems = 0, i;
	RECT MWRect;

	pEndMissionData = _pMissionData + 65536 - 4;
	bAlertItem = g_Settings.bItemBuyAgent;
	bAlertLoc = g_Settings.bLocBuyAgent;
	bAlertType = g_Settings.bMishBuyAgent;

	// If one of the watch list is empty, we don't take it into account to decide
	// whether to show an alertbox or not
	if (g_WatchLists.itemWatchCount == 0)
	{
		bAlertItem = FALSE;
	}

	if (g_WatchLists.locWatchCount == 0)
	{
		bAlertLoc = FALSE;
	}

	// Find start of mission packet (look for 0xDAC3 longword)
	do
	{
		if (_pMissionData >= pEndMissionData)
		{
			return 0;
		}
		_pMissionData++;
		TempVal = *(unsigned long*)_pMissionData;
		TempVal = EndianSwap32(TempVal);
	} while (TempVal != 0xdac3);


#ifdef DEBUG_MISSION_PACKETS // Debug Packet
	WriteDebug("\nMission Header:\n");
	DebugPacket(_pMissionData, 6 * 4);
	WriteDebug(0);
#endif

	MishID = *(unsigned long*)_pMissionData + 0x04;
	MishID = EndianSwap32( MishID );

	// Skip header (mission ID, etc.), 6 long words (was 5 pre 16.3)
	_pMissionData += 6 * 4;
	if (_pMissionData >= pEndMissionData)
		return 0;

	// Short Description: this now a null terminated string as of 16.3? just need to parse it byte at a time?
	TempVal = *_pMissionData;
	while (TempVal != 0)
	{
		_pMissionData += 1;
		TempVal = *_pMissionData;
	}
	_pMissionData += 1;

	if (_pMissionData >= pEndMissionData)
	{
		return 0;
	}

	// Get full description length
	TempVal = *(unsigned long*)_pMissionData;
	TempVal = EndianSwap32(TempVal);
	_pMissionData += 4;
	if (_pMissionData >= pEndMissionData)
	{
		return 0;
	}

	pDesc = _pMissionData;
	DescLength = TempVal;

	// Skip full description
	_pMissionData += TempVal;
	if (_pMissionData >= pEndMissionData)
	{
		return 0;
	}

	if ((pEndMissionData - _pMissionData) < 0xe8)
	{
		return 0;
	}

#ifdef DEBUG_MISSION_PACKETS
	WriteDebug("\nAfter Descriptioins:\n");
	DebugPacket(_pMissionData, 0x24);
	WriteDebug(0);
#endif

	Cash = *(unsigned long*)(_pMissionData + 0xc);
	TotalValue = Cash = EndianSwap32(Cash);

	XP = *(unsigned long*)(_pMissionData + 0x14);
	XP = EndianSwap32(XP);

	// 1. Allocate a local tracking array to safely hold up to 6 icon keys
	unsigned long collectedIconKeys[6] = { 0 };

	// Get start of items array, and count the number of items
	pTmpItem = pItem = (Item*)(_pMissionData + 0x24);

	while (pTmpItem->Key1 != 0x2d2d2d2d)
	{
		MissionItem sItem;
		if (!GetAODBItem(&sItem, EndianSwap32(pTmpItem->Key1)))
		{
			strncpy(CharKey, (char *)&(pTmpItem->Padding), 4);
			CharKey[4] = 0;
			break;
		}

		// 2. Harvest the decoded IconKey cleanly before memory pointers advance
		if (NumItems < 6)
		{
			collectedIconKeys[NumItems] = sItem.IconKey;
		}

		NumItems++;
		pTmpItem++;

		if (pEndMissionData < (unsigned char*)pTmpItem)
		{
			return 0;
		}
	}
	// auto expand team mishes
	GetWindowRect(g_hwndMishBoard, &MWRect);
	int current_height = MWRect.bottom - MWRect.top;

	if (NumItems > 2 && g_Settings.bAutoExpand){
		SetWindowPos(g_hwndMishBoard, NULL, 0, 0, 750, current_height, SWP_NOMOVE | SWP_NOZORDER);
	}
	else {
		SetWindowPos(g_hwndMishBoard, NULL, 0, 0, g_MainWndPos.winW, current_height, SWP_NOMOVE | SWP_NOZORDER);
	}
	_pMissionData = ((unsigned char*)pTmpItem) + 4;
	if (_pMissionData >= pEndMissionData)
	{
		return 0;
	}

	/*#ifdef DEBUG_MISSION_PACKETS*/
	WriteDebug("\nAfter Items:\n");
	DebugPacket(_pMissionData, 0x100);
	WriteDebug(0);
	/*#endif*/

	MishQL = *(unsigned long*)(_pMissionData + 0xc);
	MishQL = EndianSwap32(MishQL);


	// 3. GRID GENERATION ROUTINE
	if (NumItems > 0)
	{
		int tilesToProcess = (NumItems > 6) ? 6 : NumItems;

		// Free old active images to prevent dangerous memory leaks
		if (_pData->pImageData)
		{
			free(_pData->pImageData);
			_pData->pImageData = NULL;
		}

		// Generate the complete combined 48x48 pixel multi-tile array block
		_pData->pImageData = GetAOIconDataGrid(collectedIconKeys, tilesToProcess);

		// 3. Compute precise vertical offset passing g_MishNumber cleanly
		int currentFrameTop = 15 + (g_MishNumber * FRAME_H);

		// 4. Execute the shortened immediate draw engine
		//		DrawMishIcon(g_hwndMishBoard, g_MishNumber, currentFrameTop, _pData->pImageData);
		DrawMishIcon(g_hwndMishBoard, g_MishNumber, 0, (unsigned char*)_pData->pImageData);
	}

	// Display cash
	sprintf(_pData->CashStr, "%u", Cash);
	sprintf(_pData->XPStr, "%u", XP);
	g_MishBoardData.iMishCash[g_MishNumber] = Cash;
	g_MishBoardData.iMishXP[g_MishNumber] = XP;

	// Get playfield (was + 0x98 pre 16.3)
	TempVal = *(unsigned long*)(_pMissionData + 0xA8);
	MishPF = TempVal = EndianSwap32(TempVal);
	MissionPF(TempVal, PFName);

	// Get coordinates (was +0xa4, +0xac pre 16.3)
	TempVal = *(unsigned long*)(_pMissionData + 0xb4);
	TempVal = EndianSwap32(TempVal);
	*(unsigned long*)(&CoordX) = TempVal;
	TempVal = *(unsigned long*)(_pMissionData + 0xbc);
	TempVal = EndianSwap32(TempVal);
	*(unsigned long*)(&CoordY) = TempVal;

	sprintf(TempStr, "%s (%.1f, %.1f)", PFName, CoordX, CoordY);

	bLocFound = SetAndSearch((unsigned char*)TempStr, 0, FALSE);
	//fix 	bLocFound = SetAndSearch(TempStr, puGetObjectFromCollection(_pData->pCol, LOCATION), FALSE);
	strcpy_s(g_MishBoardData.szMishLoc[g_MishNumber], sizeof(g_MishBoardData.szMishLoc[g_MishNumber]), (char*)TempStr);


	// Get Mission Type
	TempVal = *(unsigned long*)(_pMissionData + 0x28);
	TempVal = EndianSwap32(TempVal);
	//fix    bTypeFound = SetAndSearchType( TempVal, puGetObjectFromCollection( _pData->pCol, MISHTYPE ) );
	bTypeFound = SetAndSearchType(TempVal, 0);

	// Write Stuff to log:
	WriteLog("mission\t%u\t%u\t%u\t%u\t%s\n", MishID, MishQL, XP, Cash, CharKey);
	WriteLog("loc\t%u\t%.1f\t%.1f\t%s\n", MishPF, CoordX, CoordY, PFName);


	// Display items
	for (i = 0; i < NumItems; i++)
	{
		g_MishBoardData.iMishItemCount[g_MishNumber] = NumItems;
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

	if (NumItems == 0)//&& !g_BuyingAgentCount ) // Special Case.. For Missions with no Item Rewards
	{
	}

	// 1. Create a lowercase copy of the description for searching
	unsigned char* pLowerDesc = malloc(DescLength + 1);
	if (pLowerDesc)
	{
		for (i = 0; i < DescLength; i++) {
			unsigned char c = pDesc[i];
			pLowerDesc[i] = (c >= 'A' && c <= 'Z') ? (c + 0x20) : c;
		}
		pLowerDesc[DescLength] = 0;

		// 2. Search for watchlist items in the description
		unsigned long idx;
		unsigned long bFindItemInDesc = FALSE;

		// Loop directly through global ANSI string array cache memory
		for (idx = 0; idx < (unsigned long)g_WatchLists.itemWatchCount; ++idx)
		{
			unsigned char* pWatchItem = (unsigned char*)g_WatchLists.itemWatchList[idx];
			if (pWatchItem && *pWatchItem)
			{
				// Note: SetAndSearch already lowercases the search term internally, 
				// so we just need to compare it against our lowercased description.
				if (strstr((char*)pLowerDesc, (char*)pWatchItem) != NULL)
				{
					bFindItemInDesc = TRUE;
					break;
				}
			}
		}

		// 3. The "Else" logic: Clear the field if nothing was found
		if (!bFindItemInDesc)
		{
		}
		g_MishBoardData.bMishIsFind[g_MishNumber] = bFindItemInDesc;
		bItemFound |= bFindItemInDesc;
		free(pLowerDesc);
	}


	bAlert = bAlertItem || bAlertLoc || bAlertType;

	if (bAlertItem) bAlert = bAlert && bItemFound;
	if (bAlertLoc) bAlert = bAlert && bLocFound;
	if (bAlertType) bAlert = bAlert && bTypeFound;

	if (bAlert)
	{
		if (g_FoundMish == 255) g_FoundMish = g_MishNumber;
		if (g_BuyingAgentCount)
		{
			// Found mission, stop buying agent
			g_BuyingAgentCount = 0;
		}
		else
		{
			if (g_Settings.bAlertBox && !g_bFullscreen)
			{
				//ShowInfoMessage("Mission(s) matches search criteria!");
			}
		}
	}

	return (unsigned long)_pMissionData;
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
