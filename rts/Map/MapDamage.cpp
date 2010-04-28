/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "MapDamage.h"
#include "BasicMapDamage.h"
#include "NoMapDamage.h"
#include "ReadMap.h"
#include "MapInfo.h"
#include "Game/GameSetup.h"

IMapDamage* mapDamage;

IMapDamage::IMapDamage()
{
	disabled = true;
}

IMapDamage::~IMapDamage()
{
}

IMapDamage * IMapDamage::GetMapDamage()
{
	bool disable = false;
	
	if (mapInfo->map.notDeformable)
		disable = true;
	else if (gameSetup && gameSetup->disableMapDamage)
		disable = true;
	
	if (disable)
		return new CNoMapDamage;
	else
		return new CBasicMapDamage;
}
