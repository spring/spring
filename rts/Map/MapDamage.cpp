/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "MapDamage.h"
#include "BasicMapDamage.h"
#include "NoMapDamage.h"
#include "MapInfo.h"
#include "Game/GameSetup.h"

static CDummyMapDamage dummyMapDamage;
static CBasicMapDamage basicMapDamage;

// never null
IMapDamage* mapDamage = &dummyMapDamage;

IMapDamage* IMapDamage::InitMapDamage()
{
	if (mapInfo->map.notDeformable) {
		dummyMapDamage.Init();
		return &dummyMapDamage;
	}
	if (gameSetup->disableMapDamage) {
		dummyMapDamage.Init();
		return &dummyMapDamage;
	}

	basicMapDamage.Init();
	return &basicMapDamage;
}

void IMapDamage::FreeMapDamage(IMapDamage* p)
{
	assert(p == mapDamage);
	mapDamage = &dummyMapDamage;
}

