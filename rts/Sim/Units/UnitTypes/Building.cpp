#include "StdAfx.h"
// Building.cpp: implementation of the CBuilding class.
//
//////////////////////////////////////////////////////////////////////

#include "Building.h"
#include "Map/ReadMap.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/GroundDecalHandler.h"
#include "Game/GameSetup.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/GroundDecalHandler.h"
#include "Game/GameSetup.h"
#include "Rendering/UnitModels/UnitDrawer.h"

#include "mmgr.h"

CR_BIND_DERIVED(CBuilding, CUnit, );

CR_REG_METADATA(CBuilding, (
				CR_RESERVED(8),
				CR_POSTLOAD(PostLoad)
				));

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
		if(!(losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR)) && (losStatus[gu->myAllyTeam] & (LOS_PREVLOS)) && !gu->spectatingFullView){
			CUnitDrawer::GhostBuilding* gb=SAFE_NEW CUnitDrawer::GhostBuilding;
			gb->pos=pos;
			gb->model=model;
			gb->decal=buildingDecal;
			gb->facing=buildFacing;
			gb->team=team;
			if(model->textureType) //S3O
				unitDrawer->ghostBuildingsS3O.push_back(gb);
			else // 3DO
				unitDrawer->ghostBuildings.push_back(gb);
			mygb=gb;
		}
	}

	if(buildingDecal)
		groundDecals->RemoveBuilding(this,mygb);
}

void CBuilding::Init(const CUnit* builder)
{
	mass=100000;
	physicalState = OnGround;

	if(unitDef->useBuildingGroundDecal){
		groundDecals->AddBuilding(this);
	}
	CUnit::Init(builder);
}

void CBuilding::PostLoad()
{
	if(unitDef->useBuildingGroundDecal){
		groundDecals->AddBuilding(this);
	}
}

void CBuilding::UnitInit (UnitDef* def, int team, const float3& position)
{
	if(def->levelGround)
		blockHeightChanges=true;
	CUnit::UnitInit(def,team,position);
}
