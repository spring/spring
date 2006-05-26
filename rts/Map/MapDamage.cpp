#include "StdAfx.h"
#include "MapDamage.h"
#include "BasicMapDamage.h"
#include "NoMapDamage.h"
#include "ReadMap.h"
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
	
	if (readmap->mapDefParser.SGetValueDef("0", "MAP\\NotDeformable") != "0")
		disable = true;
	else if (gameSetup && gameSetup->disableMapDamage)
		disable = true;
	
	if (disable)
		return new CNoMapDamage;
	else
		return new CBasicMapDamage;
}
