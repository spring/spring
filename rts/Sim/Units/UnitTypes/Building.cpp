#include "StdAfx.h"
// Building.cpp: implementation of the CBuilding class.
//
//////////////////////////////////////////////////////////////////////

#include "Building.h"
#include "Sim/Map/ReadMap.h"

//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuilding::CBuilding(const float3 &pos,int team,UnitDef* unitDef)
: CUnit(pos,team,unitDef)
{
	immobile=true;
	blockHeightChanges=true;
}

CBuilding::~CBuilding()
{
}

void CBuilding::Init(void)
{
	mass=100000;
	CUnit::Update();
	physicalState = OnGround;

	CUnit::Init();
}
