#ifndef AOMD_H
#define AOMD_H

#include <windows.h>
#include "Platform.h" // Guarantees basic types like unsigned long, signed long, and unsigned char exist
#include "mission.h"  // Ensures MissionItem layout data definition is pulled in

// Function declarations
unsigned char *GetAOIconDataGrid(unsigned long *pIconKeys, int numTiles);
void GetMissionItem(MissionItem* _pMissionItem, unsigned long _ItemKey1, unsigned long _ItemKey2, unsigned long _QL);
unsigned char GetAODBItem(MissionItem* _pMissionItem, unsigned long _ItemKey);
void MissionPF(signed long _PFNum, unsigned char* _pPFString);

#endif // AOMD_H
