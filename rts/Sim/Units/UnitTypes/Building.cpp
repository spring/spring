#include "StdAfx.h"
// Building.cpp: implementation of the CBuilding class.
//
//////////////////////////////////////////////////////////////////////

#include "Building.h"
#include "Sim/Map/ReadMap.h"
#include "Sim/Units/UnitDef.h"

#include "mmgr.h"

CR_BIND_DERIVED(CBuilding, CUnit);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuilding::CBuilding()
{
	immobile=true;
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

void CBuilding::UnitInit (UnitDef* def, int team, const float3& position)
{
	if(def->levelGround)
		blockHeightChanges=true;
	CUnit::UnitInit(def,team,position);
}
