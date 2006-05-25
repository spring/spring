#include "StdAfx.h"
// Building.cpp: implementation of the CBuilding class.
//
//////////////////////////////////////////////////////////////////////

#include "Building.h"
#include "Sim/Map/ReadMap.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/GroundDecalHandler.h"
#include "Game/GameSetup.h"
#include "Rendering/UnitModels/UnitDrawer.h"

#include "mmgr.h"

CR_BIND_DERIVED(CBuilding, CUnit);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuilding::CBuilding()
: buildingDecal(0)
{
	immobile=true;
}

CBuilding::~CBuilding()
{
	CUnitDrawer::GhostBuilding* mygb=0;
	if(!gameSetup || gameSetup->ghostedBuildings) {
		if(!(losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR)) && (losStatus[gu->myAllyTeam] & (LOS_PREVLOS)) && !gu->spectating){
			CUnitDrawer::GhostBuilding* gb=new CUnitDrawer::GhostBuilding;
			gb->pos=pos;
			gb->model=model;
			gb->decal=buildingDecal;
			unitDrawer->ghostBuildings.push_back(gb);
			mygb=gb;
		}
	}

	if(buildingDecal)
		groundDecals->RemoveBuilding(this,mygb);
}

void CBuilding::Init(void)
{
	mass=100000;
	physicalState = OnGround;

	if(unitDef->useBuildingGroundDecal){
		groundDecals->AddBuilding(this);
	}
	CUnit::Init();
}

void CBuilding::UnitInit (UnitDef* def, int team, const float3& position)
{
	if(def->levelGround)
		blockHeightChanges=true;
	CUnit::UnitInit(def,team,position);
}
