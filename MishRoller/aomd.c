#define COBJMACROS      // CRITICAL: Must be at the very top for GCC/MinGW C COM macros to work
#include "aomd.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Globals.h"
#include <math.h>       // For abs() tracking
#include <wincodec.h>   // Windows Imaging Component
#include <shlwapi.h>    // For SHCreateMemStream
#include "RDB.h"        // Exposes GetDataChunk, AODB_TYP_ITEM, AODB_TYP_PF, and AODB_TYP_ICON

// Link against required Windows subsystems (Native MSVC directives)
#pragma comment(lib, "Windowscodecs.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Ole32.lib")

/*******************************
Various parts borrowed from AOMD
(database access, PNG unpacking, playfield names, find item name finder)
********************************/
void GetMissionItem(MissionItem* _pMissionItem, unsigned long _ItemKey1, unsigned long
	_ItemKey2, unsigned long _QL)
{
	MissionItem sItem1, sItem2;

	_pMissionItem->QL = _QL;
	if (!_ItemKey1)
	{
		goto FetchItemName_Err_NotFound;
	}

	/* Get description for item number 1 */
	if (!GetAODBItem(&sItem1, _ItemKey1))
	{
		goto FetchItemName_Err_NotFound;
	}

	/* If no item number 2, then just keep the first description */
	if (!_ItemKey2 || _ItemKey2 == _ItemKey1)
	{
		strcpy(_pMissionItem->pName, sItem1.pName);
		_pMissionItem->IconKey = sItem1.IconKey;
		_pMissionItem->Value = sItem1.Value;
	}
	/* Item number 2 exists, must interpolate */
	else
	{
		if (!GetAODBItem(&sItem2, _ItemKey2))
		{
			goto FetchItemName_Err_NotFound;
		}

		if (abs(_QL - sItem1.QL) < abs(sItem2.QL - _QL))
		{
			strcpy(_pMissionItem->pName, sItem1.pName);
			_pMissionItem->IconKey = sItem1.IconKey;
		}
		else
		{
			strcpy(_pMissionItem->pName, sItem2.pName);
			_pMissionItem->IconKey = sItem2.IconKey;
		}

		/*if ((sItem2.QL - sItem1.QL) == 0)
		{
			_pMissionItem->Value = sItem1.Value;
		}
		else
		{
			_pMissionItem->Value = sItem1.Value + ((sItem2.Value - sItem1.Value) / (sItem2.QL - sItem1.QL) * (_QL - sItem1.QL));
		}*/
		// New value equation
		int qlSpan = sItem2.QL - sItem1.QL;

		if (qlSpan <= 0 || _QL <= sItem1.QL)
		{
			// Fallback if QL span is zero or target QL doesn't exceed the baseline
			_pMissionItem->Value = sItem1.Value;
		}
		else
		{
			// 1. Calculate the Quadratic Growth Constant (k)
			double k = (double)(sItem2.Value - sItem1.Value) / (double)(qlSpan * qlSpan);

			// 2. Solve for the current QL using the square of the distance
			int dist = _QL - sItem1.QL;
			_pMissionItem->Value = (unsigned long)(sItem1.Value + (k * (double)(dist * dist)));

		}

	}

	/* Success */
	return;

FetchItemName_Err_NotFound:
	sprintf(_pMissionItem->pName, "Unknown (%X:%X)", _ItemKey1, _ItemKey2);
	_pMissionItem->IconKey = 0;
	return;
}


/* Get item Data from PRK Database */
unsigned char GetAODBItem(MissionItem* _pMissionItem, unsigned long _ItemKey)
{
	unsigned char *a_xData;
	unsigned long lDataLen = sizeof(MissionItem);
	if (!(a_xData = GetDataChunk(AODB_TYP_ITEM, _ItemKey, &lDataLen)))
	{
		return FALSE;
	}
	if (lDataLen != sizeof(MissionItem))
	{
		return FALSE;
	}
	memcpy(_pMissionItem, a_xData, sizeof(MissionItem));
	return TRUE;
}

// Now takes an array of icon keys and the total number of items
unsigned char *GetAOIconDataGrid(unsigned long *pIconKeys, int numTiles)
{
	// Background layout colors (Red, Green, Blue)
	BYTE repR = GetRValue(g_Settings.clrIconBg);
	BYTE repG = GetGValue(g_Settings.clrIconBg);
	BYTE repB = GetBValue(g_Settings.clrIconBg);


	UINT smallWidth = 24;
	UINT smallHeight = 16;
	UINT gridCols = 2;

	unsigned long lDataLen = 0;
	unsigned char *a_xData = NULL;
	unsigned char *pOriginalIconData = NULL;
	unsigned char *pSmallIconData = NULL;
	unsigned char *pFinalCanvasData = NULL;

	UINT origWidth = 0;
	UINT origHeight = 0;
	unsigned long lOrigBytesPerRow = 0;
	unsigned long lSmallBytesPerRow = 0;

	const UINT finalWidth = 48;
	const UINT finalHeight = 48;
	unsigned long lFinalBytesPerRow = 0;

	HRESULT hr = S_OK;

	UINT y = 0;
	UINT x = 0;
	int i = 0;
	UINT itemRow = 0;
	UINT itemCol = 0;
	UINT startX = 0;
	UINT startY = 0;
	unsigned char *pSrcRow = NULL;
	unsigned char *pDestRow = NULL;

	IStream *pStream = NULL;
	IWICImagingFactory *pFactory = NULL;
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pFrame = NULL;
	IWICBitmapScaler *pScaler = NULL;
	IWICFormatConverter *pConverter = NULL;
	IWICBitmap* pBitmap = NULL;

	// Adjust layouts based on item counts
	if (numTiles == 1) {
		smallWidth = 48; smallHeight = 48; gridCols = 1;
	}
	else if (numTiles == 2) {
		smallWidth = 48; smallHeight = 24; gridCols = 1;
	}
	else if (numTiles == 3 || numTiles == 4) {
		smallWidth = 24; smallHeight = 24; gridCols = 2;
	}
	else {
		smallWidth = 24; smallHeight = 16; gridCols = 2;
	}

	// 1. Allocate the clean master 48x48 background canvas
	lFinalBytesPerRow = (((finalWidth * 24) + 31) / 32) * 4;
	pFinalCanvasData = (unsigned char *)malloc(finalHeight * lFinalBytesPerRow);
	if (!pFinalCanvasData) return NULL;

	// Fill background with chosen RGB color
	for (y = 0; y < finalHeight; y++) {
		unsigned char *pCanvasPixel = pFinalCanvasData + (y * lFinalBytesPerRow);
		for (x = 0; x < finalWidth; x++) {
			pCanvasPixel[0] = repB; pCanvasPixel[1] = repG; pCanvasPixel[2] = repR;
			pCanvasPixel += 3;
		}
	}

	// Initialize COM once for this grid assembly
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return pFinalCanvasData;

	hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&pFactory);
	if (FAILED(hr)) { CoUninitialize(); return pFinalCanvasData; }

	// 2. Loop through each item up to numTiles limit and tile them into the canvas
	for (i = 0; i < numTiles; i++)
	{
		// Extract raw bytes for this specific icon key from SQLite
		a_xData = (unsigned char*)GetDataChunk(AODB_TYP_ICON, pIconKeys[i], &lDataLen);
		if (!a_xData) continue; // Skip if database entry is missing

		// Set up stream loader pipelines
		pStream = SHCreateMemStream((const BYTE*)a_xData, lDataLen);
		free(a_xData);
		if (!pStream) continue;

		if (SUCCEEDED(IWICImagingFactory_CreateDecoderFromStream(pFactory, pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder)) &&
			SUCCEEDED(IWICBitmapDecoder_GetFrame(pDecoder, 0, &pFrame)) &&
			SUCCEEDED(IWICImagingFactory_CreateFormatConverter(pFactory, &pConverter)) &&
			SUCCEEDED(IWICFormatConverter_Initialize(pConverter, (IWICBitmapSource*)pFrame, &GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom)))
		{
			IWICBitmapFrameDecode_GetSize(pFrame, &origWidth, &origHeight);
			lOrigBytesPerRow = (((origWidth * 24) + 31) / 32) * 4;
			pOriginalIconData = (unsigned char *)malloc(origHeight * lOrigBytesPerRow);

			if (pOriginalIconData && SUCCEEDED(IWICFormatConverter_CopyPixels(pConverter, NULL, lOrigBytesPerRow, origHeight * lOrigBytesPerRow, (BYTE*)pOriginalIconData)))
			{
				// Green to RGB color loop
				for (y = 0; y < origHeight; y++) {
					unsigned char *pPixel = pOriginalIconData + (y * lOrigBytesPerRow);
					for (x = 0; x < origWidth; x++) {
						if (pPixel[1] == 255 && pPixel[2] == 0 && pPixel[0] == 0) {
							pPixel[0] = repB; pPixel[1] = repG; pPixel[2] = repR;
						}
						pPixel += 3;
					}
				}

				// Scaler Pipeline
				if (SUCCEEDED(IWICImagingFactory_CreateBitmapFromMemory(pFactory, origWidth, origHeight, &GUID_WICPixelFormat24bppBGR, lOrigBytesPerRow, origHeight * lOrigBytesPerRow, (BYTE*)pOriginalIconData, &pBitmap)) &&
					SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(pFactory, &pScaler)) &&
					SUCCEEDED(IWICBitmapScaler_Initialize(pScaler, (IWICBitmapSource*)pBitmap, smallWidth, smallHeight, WICBitmapInterpolationModeFant)))
				{
					lSmallBytesPerRow = (((smallWidth * 24) + 31) / 32) * 4;
					pSmallIconData = (unsigned char *)malloc(smallHeight * lSmallBytesPerRow);

					if (pSmallIconData && SUCCEEDED(IWICBitmapScaler_CopyPixels(pScaler, NULL, lSmallBytesPerRow, smallHeight * lSmallBytesPerRow, (BYTE*)pSmallIconData)))
					{
						// Map grid positions
						itemRow = i / gridCols;
						itemCol = i % gridCols;
						startX = itemCol * smallWidth;
						startY = itemRow * smallHeight;

						// Blit this item's specific rows into the shared master canvas
						for (y = 0; y < smallHeight; y++) {
							pSrcRow = pSmallIconData + (y * lSmallBytesPerRow);
							pDestRow = pFinalCanvasData + ((startY + y) * lFinalBytesPerRow) + (startX * 3);
							memcpy(pDestRow, pSrcRow, smallWidth * 3);
						}
					}
					if (pSmallIconData) free(pSmallIconData);
				}
				if (pBitmap) { IWICBitmap_Release(pBitmap); pBitmap = NULL; }
				if (pScaler) { IWICBitmapScaler_Release(pScaler); pScaler = NULL; }
			}
			if (pOriginalIconData) { free(pOriginalIconData); pOriginalIconData = NULL; }
		}

		// Clean up this item's loop handles before moving to the next item
		if (pConverter) { IWICFormatConverter_Release(pConverter); pConverter = NULL; }
		if (pFrame) { IWICBitmapFrameDecode_Release(pFrame); pFrame = NULL; }
		if (pDecoder) { IWICBitmapDecoder_Release(pDecoder); pDecoder = NULL; }
		if (pStream) { IStream_Release(pStream); pStream = NULL; }
	}

	if (pFactory) IWICImagingFactory_Release(pFactory);
	CoUninitialize();

	return pFinalCanvasData;
}



// Return mission PlayField descriptive string database layout mapping
void MissionPF(signed long _PFNum, unsigned char* _pPFString)
{
	unsigned char *pData;

	// Read data for this playfield
	if (!(pData = (unsigned char*)GetDataChunk(AODB_TYP_PF, _PFNum, NULL)))
	{
		return;
	}

	strcpy((char*)_pPFString, (char*)pData);

	free(pData);
}