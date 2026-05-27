#ifndef __MISSION_H__
#define __MISSION_H__

//#include <pul/pul.h>

#define MR_MISSIONCLASS_ATT PU_TAG_USER + 0x10000
#define MR_MISSIONCLASS_MTD PU_TAG_USER + 0x11000

#define MRA_MISSION_TITLE           MR_MISSIONCLASS_ATT + 1
#define MRM_MISSION_PARSEMISSION    MR_MISSIONCLASS_MTD + 1

// As of version 15.3.3 according to Jayde @ aodb.info, MAX(LENGTH(name)) is 104, adding some padding here...
#define AODB_MAX_NAME_LEN 127

typedef struct
{
    unsigned char    pName[ AODB_MAX_NAME_LEN + 1 ];
    unsigned long   Value;
    unsigned long   QL;
    unsigned long   IconKey;
} MissionItem;

typedef struct
{
    //pusObjectCollection*    pCol;
	void* pcol;
    //pusObjectCollection*    pSingleCol;
    //pusObjectCollection*    pTeamCol;
    unsigned char                    CashStr[ 16 ];
    unsigned char                    XPStr[ 16 ];
    unsigned char                    TimeStr[ 16 ];
    unsigned char                    TypeStr[ 16 ];
    MissionItem             Reward;
    unsigned char*                   pImageData;
} MissionClassData;

typedef struct
{
    unsigned long   Key1;
    unsigned long   Key2;
    unsigned long   QL;
    unsigned long   Padding;
} Item;

typedef struct {
	unsigned long MishID;
	unsigned long Cash;
	unsigned long XP;
	unsigned long MishQL;
	unsigned long MissionType;
	unsigned long MishPF;
	float CoordX;
	float CoordY;
	unsigned long NumItems;
	unsigned long CollectedIconKeys[6];
	unsigned char* pDesc;
	unsigned long DescLength;
	Item* pItemStart;
	char CharKey[6];
} ExtractedMissionData;

enum
{
    ROOTOBJ,
    LOCATION,
    CASH,
    MISHXP,
    FINDITEM,
    IMAGE,
    FOLD,
    MISHTYPE,
    TOTALVAL,
    ITEM1,
    ITEM2,
    ITEM3,
    ITEM4,
    ITEM5,
    ITEM6,
    ITEMVAL1,
    ITEMVAL2,
    ITEMVAL3,
    ITEMVAL4,
    ITEMVAL5,
    ITEMVAL6,
    ITEMROW1,
    ITEMROW2,
    ITEMROW3,
    ITEMROW4,
    ITEMROW5,
    ITEMROW6,
};

#endif
