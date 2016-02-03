/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstGame.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
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
#include "System/FileSystem/ArchiveScanner.h"
#include "System/Util.h"


/******************************************************************************/
/******************************************************************************/

static void LuaPushNamedColor(lua_State* L,
                              const string& name, const float3& color)
{
	lua_pushsstring(L, name);
	lua_newtable(L);
	lua_pushnumber(L, color.x); lua_rawseti(L, -2, 1);
	lua_pushnumber(L, color.y); lua_rawseti(L, -2, 2);
	lua_pushnumber(L, color.z); lua_rawseti(L, -2, 3);
	lua_rawset(L, -3);
}


bool LuaConstGame::PushEntries(lua_State* L)
{
	assert(mapInfo);
	assert(gameSetup);

	// FIXME  --  this is getting silly, convert to userdata?
	LuaPushNamedString(L, "version", SpringVersion::GetSync());
	LuaPushNamedString(L, "versionFull", (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetFull(): "");
	LuaPushNamedString(L, "versionPatchSet", (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetPatchSet(): "");
	LuaPushNamedString(L, "buildFlags", (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetAdditional(): "");

	if (unitHandler != NULL) {
		LuaPushNamedNumber(L, "maxUnits",      unitHandler->MaxUnits());
	}
	LuaPushNamedNumber(L, "maxTeams",      MAX_TEAMS);
	LuaPushNamedNumber(L, "maxPlayers",    MAX_PLAYERS);
	LuaPushNamedNumber(L, "gameSpeed",     GAME_SPEED);
	LuaPushNamedNumber(L, "squareSize",    SQUARE_SIZE);

	LuaPushNamedNumber(L, "startPosType",  gameSetup->startPosType);
	LuaPushNamedBool(L,   "ghostedBuildings", gameSetup->ghostedBuildings);

	LuaPushNamedNumber(L, "gravity",             -mapInfo->map.gravity * GAME_SPEED * GAME_SPEED);
	LuaPushNamedNumber(L, "windMin",             wind.GetMinWind());
	LuaPushNamedNumber(L, "windMax",             wind.GetMaxWind());
	LuaPushNamedString(L, "mapName",             mapInfo->map.name);
	LuaPushNamedString(L, "mapHumanName",        mapInfo->map.description); //! deprecated
	LuaPushNamedString(L, "mapDescription",      mapInfo->map.description);
	LuaPushNamedNumber(L, "mapHardness",         mapInfo->map.hardness);

	if (mapDamage) {
		// damage is enabled iff !mapInfo->map.notDeformable
		LuaPushNamedBool(L, "mapDamage", !mapDamage->disabled);
	}


	if (readMap) {
		//FIXME make this available in LoadScreen already!
		LuaPushNamedNumber(L, "mapX",            mapDims.mapx / 64);
		LuaPushNamedNumber(L, "mapY",            mapDims.mapy / 64);
		LuaPushNamedNumber(L, "mapSizeX",        mapDims.mapx * SQUARE_SIZE);
		LuaPushNamedNumber(L, "mapSizeZ",        mapDims.mapy * SQUARE_SIZE);
	}
	LuaPushNamedNumber(L, "extractorRadius",     mapInfo->map.extractorRadius);
	LuaPushNamedNumber(L, "tidal",               mapInfo->map.tidalStrength);

	LuaPushNamedNumber(L, "waterDamage",         mapInfo->water.damage);
	LuaPushNamedString(L, "waterTexture",        mapInfo->water.texture);
	LuaPushNamedNumber(L, "waterRepeatX",        mapInfo->water.repeatX);
	LuaPushNamedNumber(L, "waterRepeatY",        mapInfo->water.repeatY);
	LuaPushNamedString(L, "waterFoamTexture",    mapInfo->water.foamTexture);
	LuaPushNamedString(L, "waterNormalTexture",  mapInfo->water.normalTexture);
	LuaPushNamedNumber(L, "waterNumTiles",       mapInfo->water.numTiles);
	LuaPushNamedBool(L,   "voidWater",           mapInfo->map.voidWater);
	LuaPushNamedBool(L,   "voidGround",          mapInfo->map.voidGround);
	LuaPushNamedBool(L,   "waterHasWaterPlane",  mapInfo->water.hasWaterPlane);
	LuaPushNamedBool(L,   "waterForceRendering", mapInfo->water.forceRendering);
	LuaPushNamedColor(L,  "waterAbsorb",         mapInfo->water.absorb);
	LuaPushNamedColor(L,  "waterBaseColor",      mapInfo->water.baseColor);
	LuaPushNamedColor(L,  "waterMinColor",       mapInfo->water.minColor);
	LuaPushNamedColor(L,  "waterSurfaceColor",   mapInfo->water.surfaceColor);
	LuaPushNamedNumber(L, "waterSurfaceAlpha",   mapInfo->water.surfaceAlpha);
	LuaPushNamedColor(L,  "waterDiffuseColor",   mapInfo->water.diffuseColor);
	LuaPushNamedNumber(L, "waterDiffuseFactor",  mapInfo->water.diffuseFactor);
	LuaPushNamedColor(L,  "waterSpecularColor",  mapInfo->water.specularColor);
	LuaPushNamedNumber(L, "waterSpecularFactor", mapInfo->water.specularFactor);
	LuaPushNamedNumber(L, "waterAmbientFactor",  mapInfo->water.ambientFactor);
	LuaPushNamedColor(L,  "waterPlaneColor",     mapInfo->water.planeColor);
	LuaPushNamedNumber(L, "waterFresnelMin",     mapInfo->water.fresnelMin);
	LuaPushNamedNumber(L, "waterFresnelMax",     mapInfo->water.fresnelMax);
	LuaPushNamedNumber(L, "waterFresnelPower",   mapInfo->water.fresnelPower);
	LuaPushNamedNumber(L, "waterReflectionDistortion", mapInfo->water.reflDistortion);

	const vector<string>& causticTexs = mapInfo->water.causticTextures;
	lua_pushliteral(L, "waterCausticTextures");
	lua_newtable(L);
	for (int i = 0; i < (int)causticTexs.size(); i++) {
		lua_pushsstring(L, causticTexs[i]);
		lua_rawseti(L, -2, i + 1);
	}
	lua_rawset(L, -3);

	LuaPushNamedBool(L,   "allowTeamColors", true);

	LuaPushNamedString(L, "gameName",         modInfo.humanName);
	LuaPushNamedString(L, "gameShortName",    modInfo.shortName);
	LuaPushNamedString(L, "gameVersion",      modInfo.version);
	LuaPushNamedString(L, "gameMutator",      modInfo.mutator);
	LuaPushNamedString(L, "gameDesc",         modInfo.description);

	LuaPushNamedString(L, "modName",         modInfo.humanNameVersioned);
	LuaPushNamedString(L, "modShortName",    modInfo.shortName);
	LuaPushNamedString(L, "modVersion",      modInfo.version);
	LuaPushNamedString(L, "modMutator",      modInfo.mutator);
	LuaPushNamedString(L, "modDesc",         modInfo.description);

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
	         archiveScanner->GetArchiveCompleteChecksum(mapInfo->map.name));
	LuaPushNamedString(L, "mapChecksum", buf);
	SNPRINTF(buf, sizeof(buf), "0x%08X",
	         archiveScanner->GetArchiveCompleteChecksum(modInfo.filename));
	LuaPushNamedString(L, "modChecksum", buf);

	// needed for LuaIntro which also pushes ConstGame entries
	// (but it probably doesn't need to know about categories)
	if (CCategoryHandler::Instance() != NULL) {
		const vector<string>& cats = CCategoryHandler::Instance()->GetCategoryNames(~0);

		lua_pushliteral(L, "springCategories");
		lua_newtable(L);

		for (unsigned int i = 0; i < cats.size(); i++) {
			LuaPushNamedNumber(L, StringToLower(cats[i]), i);
		}

		lua_rawset(L, -3);
	}

	// needed for LuaIntro which also pushes ConstGame entries
	// (but it probably doesn't need to know about armor-types)
	if (damageArrayHandler != NULL) {
		lua_pushliteral(L, "armorTypes");
		lua_newtable(L);

		const std::vector<std::string>& typeList = damageArrayHandler->GetTypeList();
		const int typeCount = (int)typeList.size();
		for (int i = 0; i < typeCount; i++) {
			// bidirectional map
			lua_pushsstring(L, typeList[i]);
			lua_pushnumber(L, i);
			lua_rawset(L, -3);
			lua_pushsstring(L, typeList[i]);
			lua_rawseti(L, -2, i);
		}

		lua_rawset(L, -3);
	}

	return true;
}


/******************************************************************************/
/******************************************************************************/
