#include "StdAfx.h"
// LuaConstGame.cpp: implementation of the LuaConstGame class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaConstGame.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"
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


extern GLfloat FogLand[];


/******************************************************************************/
/******************************************************************************/

static void LuaPushNamedColor(lua_State* L,
                              const string& name, const float3& color)
{
	lua_pushlstring(L, name.c_str(), name.size());
	lua_newtable(L);
	lua_pushnumber(L, color.x); lua_rawseti(L, -2, 1);
	lua_pushnumber(L, color.y); lua_rawseti(L, -2, 2);
	lua_pushnumber(L, color.z); lua_rawseti(L, -2, 3);
	lua_rawset(L, -3);
}


bool LuaConstGame::PushEntries(lua_State* L)
{
	const float gravity = -(gs->gravity * GAME_SPEED * GAME_SPEED);
	const bool limitDGun        = gameSetup ? gameSetup->limitDgun        : false;
	const bool diminishingMMs   = gameSetup ? gameSetup->diminishingMMs   : false;
	const bool ghostedBuildings = gameSetup ? gameSetup->ghostedBuildings : false;
	const int  startPosType     = gameSetup ? gameSetup->startPosType     : 0;
	const float3 fogColor(FogLand[0], FogLand[1], FogLand[2]);

	LuaPushNamedString(L, "version",       VERSION_STRING);

	LuaPushNamedNumber(L, "maxUnits",      MAX_UNITS);
	LuaPushNamedNumber(L, "maxTeams",      MAX_TEAMS);
	LuaPushNamedNumber(L, "maxPlayers",    MAX_PLAYERS);
	LuaPushNamedNumber(L, "gameSpeed",     GAME_SPEED);
	LuaPushNamedNumber(L, "squareSize",    SQUARE_SIZE);

	LuaPushNamedNumber(L, "gameMode",      gs->gameMode);
	LuaPushNamedNumber(L, "startPosType",  startPosType);

	LuaPushNamedBool(L,   "commEnds",         (gs->gameMode >= 1));
	LuaPushNamedBool(L,   "limitDGun",        limitDGun);
	LuaPushNamedBool(L,   "diminishingMetal", diminishingMMs);
	LuaPushNamedBool(L,   "ghostedBuildings", ghostedBuildings);

	LuaPushNamedBool(L,   "mapDamage",         !mapDamage->disabled);
	LuaPushNamedNumber(L, "gravity",           gravity);
	LuaPushNamedNumber(L, "windMin",           wind.GetMinWind());
	LuaPushNamedNumber(L, "windMax",           wind.GetMaxWind());
	LuaPushNamedString(L, "mapName",           readmap->mapName);
	LuaPushNamedString(L, "mapHumanName",      readmap->mapHumanName);
	LuaPushNamedNumber(L, "mapX",              readmap->width  / 64);
	LuaPushNamedNumber(L, "mapY",              readmap->height / 64);
	LuaPushNamedNumber(L, "mapSizeX",          readmap->width  * SQUARE_SIZE);
	LuaPushNamedNumber(L, "mapSizeZ",          readmap->height * SQUARE_SIZE);
	LuaPushNamedNumber(L, "extractorRadius",   readmap->extractorRadius);
	LuaPushNamedNumber(L, "tidal",             readmap->tidalStrength);
	LuaPushNamedString(L, "waterTexture",      readmap->waterTexture);
	LuaPushNamedBool(L,   "waterVoid",         readmap->voidWater);
	LuaPushNamedBool(L,   "waterPlane",        readmap->hasWaterPlane);
	LuaPushNamedColor(L,  "waterAbsorb",       readmap->waterAbsorb);
	LuaPushNamedColor(L,  "waterBaseColor",    readmap->waterBaseColor);
	LuaPushNamedColor(L,  "waterMinColor",     readmap->waterMinColor);
	LuaPushNamedColor(L,  "waterSurfaceColor", readmap->waterSurfaceColor);
	LuaPushNamedColor(L,  "waterPlaneColor",   readmap->waterPlaneColor);
	LuaPushNamedColor(L,  "fogColor",          fogColor);

	LuaPushNamedString(L, "modName",         modInfo.humanName);
	LuaPushNamedString(L, "modShortName",    modInfo.shortName);
	LuaPushNamedString(L, "modVersion",      modInfo.version);
	LuaPushNamedString(L, "modMutator",      modInfo.mutator);
	LuaPushNamedString(L, "modDesc",         modInfo.description);

	LuaPushNamedBool(L,   "allowTeamColors", modInfo.allowTeamColors);
	LuaPushNamedNumber(L, "multiReclaim",    modInfo.multiReclaim);
	LuaPushNamedNumber(L, "reclaimMethod",   modInfo.reclaimMethod);
	LuaPushNamedNumber(L, "transportAir",    modInfo.transportAir);
	LuaPushNamedNumber(L, "transportShip",   modInfo.transportShip);
	LuaPushNamedNumber(L, "transportHover",  modInfo.transportHover);
	LuaPushNamedNumber(L, "transportGround", modInfo.transportGround);
	LuaPushNamedNumber(L, "fireAtKilled",    modInfo.fireAtKilled);
	LuaPushNamedNumber(L, "fireAtCrashing",  modInfo.fireAtCrashing);

	char buf[64];
	SNPRINTF(buf, sizeof(buf), "0x%08X",
	         archiveScanner->GetMapChecksum(readmap->mapName));
	LuaPushNamedString(L, "mapChecksum", buf);
	SNPRINTF(buf, sizeof(buf), "0x%08X",
	         archiveScanner->GetModChecksum(modInfo.filename));
	LuaPushNamedString(L, "modChecksum", buf);

	const vector<string> cats =
		CCategoryHandler::Instance()->GetCategoryNames(~0);
	lua_pushstring(L, "springCategories");
	lua_newtable(L);
	for (int i = 0; i < (int)cats.size(); i++) {
		LuaPushNamedNumber(L, StringToLower(cats[i]), i);
	}
	lua_rawset(L, -3);

	lua_pushstring(L, "armorTypes");
	lua_newtable(L);
	const std::vector<std::string>& typeList = damageArrayHandler->GetTypeList();
	const int typeCount = (int)typeList.size();
	for (int i = 0; i < typeCount; i++) {
		// bidirectional map
		lua_pushstring(L, typeList[i].c_str());
		lua_pushnumber(L, i);
		lua_rawset(L, -3);
		lua_pushnumber(L, i);
		lua_pushstring(L, typeList[i].c_str());
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	return true;
}


/******************************************************************************/
/******************************************************************************/
