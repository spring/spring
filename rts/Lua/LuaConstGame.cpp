#include "StdAfx.h"
// LuaConstGame.cpp: implementation of the LuaConstGame class.
//
//////////////////////////////////////////////////////////////////////
#include "mmgr.h"

#include "LuaConstGame.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "FileSystem/ArchiveScanner.h"
#include "Util.h"


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

	// FIXME  --  this is getting silly, convert to userdata?

	const float gravity = -(mapInfo->map.gravity * GAME_SPEED * GAME_SPEED);
	const bool limitDGun        = gameSetup ? gameSetup->limitDgun        : false;
	const bool diminishingMMs   = gameSetup ? gameSetup->diminishingMMs   : false;
	const bool ghostedBuildings = gameSetup ? gameSetup->ghostedBuildings : false;
	const int  startPosType     = gameSetup ? gameSetup->startPosType     : 0;

	// FIXME -- loaded too early - not set yet  (another reason to use userdata)
	lua_pushliteral(L, "gameID");
	lua_pushlstring(L, (const char*)game->gameID, sizeof(game->gameID));
	lua_rawset(L, -3);

	LuaPushNamedString(L, "version",       SpringVersion::GetFull());

	LuaPushNamedNumber(L, "maxUnits",      uh->MaxUnits());
	LuaPushNamedNumber(L, "maxTeams",      MAX_TEAMS);
	LuaPushNamedNumber(L, "maxPlayers",    MAX_PLAYERS);
	LuaPushNamedNumber(L, "gameSpeed",     GAME_SPEED);
	LuaPushNamedNumber(L, "squareSize",    SQUARE_SIZE);

	LuaPushNamedNumber(L, "gameMode",      gameSetup->gameMode);
	LuaPushNamedNumber(L, "startPosType",  startPosType);

	LuaPushNamedBool(L,   "commEnds",         (gameSetup->gameMode >= 1));
	LuaPushNamedBool(L,   "limitDGun",        limitDGun);
	LuaPushNamedBool(L,   "diminishingMetal", diminishingMMs);
	LuaPushNamedBool(L,   "ghostedBuildings", ghostedBuildings);

	const CMapInfo* mi = mapInfo;

	LuaPushNamedBool(L,   "mapDamage",           !mapDamage->disabled);
	LuaPushNamedNumber(L, "gravity",             gravity);
	LuaPushNamedNumber(L, "windMin",             wind.GetMinWind());
	LuaPushNamedNumber(L, "windMax",             wind.GetMaxWind());
	LuaPushNamedString(L, "mapName",             mi->map.name);
	LuaPushNamedString(L, "mapHumanName",        mi->map.humanName);
	LuaPushNamedNumber(L, "mapX",                readmap->width  / 64);
	LuaPushNamedNumber(L, "mapY",                readmap->height / 64);
	LuaPushNamedNumber(L, "mapSizeX",            readmap->width  * SQUARE_SIZE);
	LuaPushNamedNumber(L, "mapSizeZ",            readmap->height * SQUARE_SIZE);
	LuaPushNamedNumber(L, "extractorRadius",     mi->map.extractorRadius);
	LuaPushNamedNumber(L, "tidal",               mi->map.tidalStrength);
	LuaPushNamedNumber(L, "waterDamage",         mi->water.damage);
	LuaPushNamedString(L, "waterTexture",        mi->water.texture);
	LuaPushNamedNumber(L, "waterRepeatX",        mi->water.repeatX);
	LuaPushNamedNumber(L, "waterRepeatY",        mi->water.repeatY);
	LuaPushNamedString(L, "waterFoamTexture",    mi->water.foamTexture);
	LuaPushNamedString(L, "waterNormalTexture",  mi->water.normalTexture);
	LuaPushNamedBool(L,   "waterVoid",           mi->map.voidWater);
	LuaPushNamedBool(L,   "waterPlane",          mi->water.hasWaterPlane);
	LuaPushNamedColor(L,  "waterAbsorb",         mi->water.absorb);
	LuaPushNamedColor(L,  "waterBaseColor",      mi->water.baseColor);
	LuaPushNamedColor(L,  "waterMinColor",       mi->water.minColor);
	LuaPushNamedColor(L,  "waterSurfaceColor",   mi->water.surfaceColor);
	LuaPushNamedNumber(L, "waterSurfaceAlpha",   mi->water.surfaceAlpha);
	LuaPushNamedColor(L,  "waterSpecularColor",  mi->water.specularColor);
	LuaPushNamedNumber(L, "waterSpecularFactor", mi->water.specularFactor);
	LuaPushNamedColor(L,  "waterPlaneColor",     mi->water.planeColor);
	LuaPushNamedNumber(L, "waterFresnelMin",     mi->water.fresnelMin);
	LuaPushNamedNumber(L, "waterFresnelMax",     mi->water.fresnelMax);
	LuaPushNamedNumber(L, "waterFresnelPower",   mi->water.fresnelPower);
	LuaPushNamedColor(L,  "fogColor",            mi->atmosphere.fogColor);
	LuaPushNamedColor(L,  "groundAmbientColor",  mi->light.groundAmbientColor);
	LuaPushNamedColor(L,  "groundSpecularColor", mi->light.groundSpecularColor);
	LuaPushNamedColor(L,  "groundSunColor",      mi->light.groundSunColor);

	const vector<string>& causticTexs = mi->water.causticTextures;
	lua_pushstring(L, "waterCausticTextures");
	lua_newtable(L);
	for (int i = 0; i < (int)causticTexs.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, causticTexs[i].c_str());
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	LuaPushNamedString(L, "modName",         modInfo.humanName);
	LuaPushNamedString(L, "modShortName",    modInfo.shortName);
	LuaPushNamedString(L, "modVersion",      modInfo.version);
	LuaPushNamedString(L, "modMutator",      modInfo.mutator);
	LuaPushNamedString(L, "modDesc",         modInfo.description);

	LuaPushNamedBool(L,   "allowTeamColors", modInfo.allowTeamColors);

	LuaPushNamedBool(L,   "constructionDecay",      modInfo.constructionDecay);
	LuaPushNamedNumber(L, "constructionDecayTime",  modInfo.constructionDecayTime);
	LuaPushNamedNumber(L, "constructionDecaySpeed", modInfo.constructionDecaySpeed);

	LuaPushNamedNumber(L, "multiReclaim",                   modInfo.multiReclaim);
	LuaPushNamedNumber(L, "reclaimMethod",                  modInfo.reclaimMethod);
	LuaPushNamedNumber(L, "reclaimUnitMethod",              modInfo.reclaimUnitMethod);
	LuaPushNamedNumber(L, "reclaimUnitEnergyCostFactor",    modInfo.reclaimUnitEnergyCostFactor);
	LuaPushNamedNumber(L, "reclaimUnitEfficiency",          modInfo.reclaimUnitEfficiency);
	LuaPushNamedNumber(L, "reclaimFeatureEnergyCostFactor", modInfo.reclaimFeatureEnergyCostFactor);
	LuaPushNamedBool(L,   "reclaimAllowEnemies",            modInfo.reclaimAllowEnemies);
	LuaPushNamedBool(L,   "reclaimAllowAllies",             modInfo.reclaimAllowAllies);
	LuaPushNamedNumber(L, "repairEnergyCostFactor",         modInfo.repairEnergyCostFactor);
	LuaPushNamedNumber(L, "resurrectEnergyCostFactor",      modInfo.resurrectEnergyCostFactor);
	LuaPushNamedNumber(L, "captureEnergyCostFactor",        modInfo.captureEnergyCostFactor);

	LuaPushNamedNumber(L, "transportAir",    modInfo.transportAir);
	LuaPushNamedNumber(L, "transportShip",   modInfo.transportShip);
	LuaPushNamedNumber(L, "transportHover",  modInfo.transportHover);
	LuaPushNamedNumber(L, "transportGround", modInfo.transportGround);
	LuaPushNamedNumber(L, "fireAtKilled",    modInfo.fireAtKilled);
	LuaPushNamedNumber(L, "fireAtCrashing",  modInfo.fireAtCrashing);

	LuaPushNamedNumber(L, "requireSonarUnderWater", modInfo.requireSonarUnderWater);

	char buf[64];
	SNPRINTF(buf, sizeof(buf), "0x%08X",
	         archiveScanner->GetMapChecksum(mapInfo->map.name));
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
