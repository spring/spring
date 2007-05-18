#include "StdAfx.h"
// LuaConstGame.cpp: implementation of the LuaConstGame class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaConstGame.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Game/command.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Sim/ModInfo.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/UnitDef.h" // MAX_UNITS
#include "System/FileSystem/ArchiveScanner.h"


/******************************************************************************/
/******************************************************************************/

bool LuaConstGame::PushEntries(lua_State* L)
{
	const float gravity = -(gs->gravity * GAME_SPEED * GAME_SPEED);
	const bool limitDGun      = gameSetup ? gameSetup->limitDgun      : false;
	const bool diminishingMMs = gameSetup ? gameSetup->diminishingMMs : false;
	
	LuaPushNamedString(L, "version",       VERSION_STRING);

	LuaPushNamedNumber(L, "maxUnits",      MAX_UNITS);
	LuaPushNamedNumber(L, "maxTeams",      MAX_TEAMS);
	LuaPushNamedNumber(L, "maxPlayers",    MAX_PLAYERS);
	LuaPushNamedNumber(L, "gameSpeed",     GAME_SPEED);
	LuaPushNamedNumber(L, "squareSize",    SQUARE_SIZE);
	LuaPushNamedNumber(L, "gameMode",      gs->gameMode);

	LuaPushNamedBool(L,   "commEnds",         (gs->gameMode >= 1));
	LuaPushNamedBool(L,   "limitDGun",        limitDGun);
	LuaPushNamedBool(L,   "diminishingMetal", diminishingMMs);

	LuaPushNamedString(L, "mapName",       readmap->mapName);
	LuaPushNamedString(L, "mapHumanName",  readmap->mapHumanName);
	LuaPushNamedNumber(L, "mapX",          readmap->width  / 64);
	LuaPushNamedNumber(L, "mapY",          readmap->height / 64);
	LuaPushNamedNumber(L, "gravity",       gravity);
	LuaPushNamedNumber(L, "tidal",         readmap->tidalStrength);
	LuaPushNamedNumber(L, "windMin",       wind.minWind);
	LuaPushNamedNumber(L, "windMax",       wind.maxWind);
	LuaPushNamedBool(L,   "mapDamage",     !mapDamage->disabled);
	LuaPushNamedBool(L,   "mapWaterVoid",  readmap->voidWater);
	LuaPushNamedBool(L,   "mapWaterPlane", readmap->hasWaterPlane);

	LuaPushNamedString(L, "modName",         modInfo->name);
	LuaPushNamedString(L, "modHumanName",    modInfo->humanName);
	LuaPushNamedBool(L,   "allowTeamColors", modInfo->allowTeamColors);
	LuaPushNamedNumber(L, "multiReclaim",    modInfo->multiReclaim);
	LuaPushNamedNumber(L, "reclaimMethod",   modInfo->reclaimMethod);
	LuaPushNamedNumber(L, "transportAir",    modInfo->transportAir);
	LuaPushNamedNumber(L, "transportShip",   modInfo->transportShip);
	LuaPushNamedNumber(L, "transportHover",  modInfo->transportHover);
	LuaPushNamedNumber(L, "transportGround", modInfo->transportGround);

	char buf[64];
	SNPRINTF(buf, sizeof(buf), "0x%08X",
	         archiveScanner->GetMapChecksum(readmap->mapName));
	LuaPushNamedString(L, "mapChecksum", buf);
	SNPRINTF(buf, sizeof(buf), "0x%08X",
	         archiveScanner->GetModChecksum(modInfo->name));
	LuaPushNamedString(L, "modChecksum", buf);

	const vector<string> cats =
		CCategoryHandler::Instance()->GetCategoryNames(~0);
	lua_pushstring(L, "springCategories");
	lua_newtable(L);
	for (int i = 0; i < (int)cats.size(); i++) {
		LuaPushNamedBool(L, cats[i], true);
	}
	lua_rawset(L, -3);

	lua_pushstring(L, "armorTypes");
	lua_newtable(L);
	const std::vector<std::string>& typeList = damageArrayHandler->typeList;
	const int typeCount = (int)typeList.size();
	for (int i = 0; i < typeCount; i++) {
		LuaPushNamedNumber(L, typeList[i].c_str(), i);
	}
	lua_rawset(L, -3);

	return true;	
}


/******************************************************************************/
/******************************************************************************/
