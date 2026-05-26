#include "rdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Instantiation of the Global Variables ---
sqlite3*      g_pSQLite  = NULL;
sqlite3_stmt* g_stmtItem = NULL;
sqlite3_stmt* g_stmtIcon = NULL;
sqlite3_stmt* g_stmtPF   = NULL;


int OpenLocalDB(void)
{
	char DBPath[MAX_PATH];
	sprintf_s(DBPath, sizeof(DBPath), "%s\\cd_image\\rdb.db", g_AODir);

	// 1. Open the database (Lightweight flag check)
	if (sqlite3_open_v2(DBPath, &g_pSQLite, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
		if (g_pSQLite) {
			sqlite3_close(g_pSQLite);
			g_pSQLite = NULL;
		}
		return FALSE;
	}

	// 2. Prepare statements (Fast compilation step, no heavy disk scans)
	if (sqlite3_prepare_v2(g_pSQLite, "SELECT data FROM rdb_1000020 WHERE id = ?;", -1, &g_stmtItem, NULL) != SQLITE_OK ||
		sqlite3_prepare_v2(g_pSQLite, "SELECT data FROM rdb_1010008 WHERE id = ?;", -1, &g_stmtIcon, NULL) != SQLITE_OK ||
		sqlite3_prepare_v2(g_pSQLite, "SELECT data FROM rdb_1000001 WHERE id = ?;", -1, &g_stmtPF, NULL) != SQLITE_OK)
	{
		// Clean up any successfully prepared statements before closing
		if (g_stmtItem) { sqlite3_finalize(g_stmtItem); g_stmtItem = NULL; }
		if (g_stmtIcon) { sqlite3_finalize(g_stmtIcon); g_stmtIcon = NULL; }
		if (g_stmtPF)   { sqlite3_finalize(g_stmtPF);   g_stmtPF = NULL; }

		sqlite3_close(g_pSQLite);
		g_pSQLite = NULL;
		return FALSE;
	}

	return TRUE;
}


void* GetDataChunk(unsigned long _KeyHi, unsigned long _KeyLo, unsigned long* _pSize)
{
    sqlite3_stmt* pStmt = NULL;
    void* pReturnData = NULL;

	// Tables: Items (1000020), Icons (1010008), Playfields (1000001)
    switch (_KeyHi) {
    case AODB_TYP_ITEM: pStmt = g_stmtItem; break;
    case AODB_TYP_ICON: pStmt = g_stmtIcon; break;
    case AODB_TYP_PF:   pStmt = g_stmtPF;   break;
    default: return NULL;
    }

    if (!pStmt) return NULL;

	// 2. Reset and Bind Key
	sqlite3_reset(pStmt);
	// Use _KeyLo directly as an integer. SQLite handles the byte order.
    sqlite3_bind_int(pStmt, 1, (int)_KeyLo);
	
	// 3. Execute
    if (sqlite3_step(pStmt) == SQLITE_ROW) {
        const unsigned char* blob = (const unsigned char*)sqlite3_column_blob(pStmt, 0);
        int blobSize = sqlite3_column_bytes(pStmt, 0);

        if (!blob || blobSize <= 0) return NULL;
		
		// --- TYPE 1: ITEMS (Parse into MissionItem struct) ---
        if (_KeyHi == AODB_TYP_ITEM) {
            MissionItem* pItem = (MissionItem*)malloc(sizeof(MissionItem));
            if (!pItem) return NULL;
            memset(pItem, 0, sizeof(MissionItem));
			
			// Scan tags for QL, Icon, and Value
            for (int i = 4; i + 8 <= blobSize; i += 8) {
                unsigned long tag = *(unsigned long*)(blob + i);
                unsigned long val = *(unsigned long*)(blob + i + 4);
                switch (tag) {
                case 0x36: pItem->QL = val; break;
                case 0x4F: pItem->IconKey = val; break;
                case 0x4A: pItem->Value = val; break;
                }
            }
			
			// Find Name Signature: 15 00 00 00 21 00 00 00
            for (int i = 0; i + 12 <= blobSize; i++) {
                if (*(unsigned long*)(blob + i) == 0x15 && *(unsigned long*)(blob + i + 4) == 0x21) {
                    unsigned short nameLen = *(unsigned short*)(blob + i + 8);
                    if (nameLen > AODB_MAX_NAME_LEN) nameLen = AODB_MAX_NAME_LEN;
                    memcpy(pItem->pName, blob + i + 12, nameLen);
                    pItem->pName[nameLen] = 0;
                    break;
                }
            }

            pReturnData = pItem;
            if (_pSize) *_pSize = sizeof(MissionItem);
        }
		
		// --- TYPE 2: PLAYFIELDS (Skip 8-byte header) ---
        else if (_KeyHi == AODB_TYP_PF) {
            if (blobSize > 8) {
                const char* strData = (const char*)(blob + 8);
                int finalSize = (int)(strlen(strData) + 1);
                pReturnData = malloc(finalSize);
                if (pReturnData) {
                    memcpy(pReturnData, strData, finalSize);
                    if (_pSize) *_pSize = (unsigned long)finalSize;
                }
            }
        }
		
		// --- TYPE 2: PLAYFIELDS (Skip 8-byte header) ---
        else {
            pReturnData = malloc(blobSize);
            if (pReturnData) {
                memcpy(pReturnData, blob, blobSize);
                if (_pSize) *_pSize = (unsigned long)blobSize;
            }
        }
    }

    return pReturnData;
}

void ReleaseAODatabase(void)
{
    if (g_stmtItem) { sqlite3_finalize(g_stmtItem); g_stmtItem = NULL; }
    if (g_stmtIcon) { sqlite3_finalize(g_stmtIcon); g_stmtIcon = NULL; }
    if (g_stmtPF)   { sqlite3_finalize(g_stmtPF);   g_stmtPF   = NULL; }
    if (g_pSQLite)  { sqlite3_close(g_pSQLite);     g_pSQLite  = NULL; }
}
