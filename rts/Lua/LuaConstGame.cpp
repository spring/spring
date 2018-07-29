/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaConstGame.h"

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Game/GameSetup.h"
#include "Game/TraceRay.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/UnitHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/StringUtil.h"


/******************************************************************************/
/******************************************************************************/

/*
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
*/


bool LuaConstGame::PushEntries(lua_State* L)
{
	assert(mapInfo != nullptr);
	assert(gameSetup->ScriptLoaded());

	// FIXME  --  this is getting silly, convert to userdata?
	LuaPushNamedNumber(L, "maxUnits",      unitHandler.MaxUnits());

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

	// damage is enabled iff !mapInfo->map.notDeformable
	if (mapDamage != nullptr)
		LuaPushNamedBool(L, "mapDamage", !mapDamage->Disabled());


	if (readMap != nullptr) {
		//FIXME make this available in LoadScreen already!
		LuaPushNamedNumber(L, "mapX",            mapDims.mapx / 64);
		LuaPushNamedNumber(L, "mapY",            mapDims.mapy / 64);
		LuaPushNamedNumber(L, "mapSizeX",        mapDims.mapx * SQUARE_SIZE);
		LuaPushNamedNumber(L, "mapSizeZ",        mapDims.mapy * SQUARE_SIZE);
	}
	LuaPushNamedNumber(L, "extractorRadius",     mapInfo->map.extractorRadius);
	LuaPushNamedNumber(L, "tidal",               mapInfo->map.tidalStrength);

	LuaPushNamedNumber(L, "waterDamage",         mapInfo->water.damage);

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

	LuaPushNamedBool  (L, "paralyzeOnMaxHealth", modInfo.paralyzeOnMaxHealth);
	LuaPushNamedNumber(L, "paralyzeDeclineRate", modInfo.paralyzeDeclineRate);

	sha512::hex_digest mapHexDigest;
	sha512::hex_digest modHexDigest;
	sha512::dump_digest(archiveScanner->GetArchiveCompleteChecksumBytes(mapInfo->map.name), mapHexDigest);
	sha512::dump_digest(archiveScanner->GetArchiveCompleteChecksumBytes(modInfo.filename), modHexDigest);

	LuaPushNamedString(L, "mapChecksum", mapHexDigest.data());
	LuaPushNamedString(L, "modChecksum", modHexDigest.data());

	// needed for LuaIntro which also pushes ConstGame entries
	// (but it probably doesn't need to know about categories)
	if (CCategoryHandler::Instance() != nullptr) {
		const std::vector<string>& cats = CCategoryHandler::Instance()->GetCategoryNames(~0);

		lua_pushliteral(L, "springCategories");
		lua_newtable(L);

		for (unsigned int i = 0; i < cats.size(); i++) {
			LuaPushNamedNumber(L, StringToLower(cats[i]), i);
		}

		lua_rawset(L, -3);
	}

	{
		lua_pushliteral(L, "armorTypes");
		lua_newtable(L);

		// NB: empty for LuaIntro which also pushes ConstGame entries
		const std::vector<std::string>& typeList = damageArrayHandler.GetTypeList();

		for (size_t i = 0; i < typeList.size(); i++) {
			// bidirectional map
			lua_pushsstring(L, typeList[i]);
			lua_pushnumber(L, i);
			lua_rawset(L, -3);
			lua_pushsstring(L, typeList[i]);
			lua_rawseti(L, -2, i);
		}

		lua_rawset(L, -3);
	}

	// environmental damage types
	lua_pushliteral(L, "envDamageTypes");
	lua_newtable(L);
		LuaPushNamedNumber(L, "Debris",          -CSolidObject::DAMAGE_EXPLOSION_DEBRIS );
		LuaPushNamedNumber(L, "GroundCollision", -CSolidObject::DAMAGE_COLLISION_GROUND );
		LuaPushNamedNumber(L, "ObjectCollision", -CSolidObject::DAMAGE_COLLISION_OBJECT );
		LuaPushNamedNumber(L, "Fire",            -CSolidObject::DAMAGE_EXTSOURCE_FIRE   );
		LuaPushNamedNumber(L, "Water",           -CSolidObject::DAMAGE_EXTSOURCE_WATER  );
		LuaPushNamedNumber(L, "Killed",          -CSolidObject::DAMAGE_EXTSOURCE_KILLED );
		LuaPushNamedNumber(L, "Crushed",         -CSolidObject::DAMAGE_EXTSOURCE_CRUSHED);
	lua_rawset(L, -3);

	// weapon avoidance and projectile collision flags
	lua_pushliteral(L, "collisionFlags");
	lua_newtable(L);
		LuaPushNamedNumber(L, "noEnemies",    Collision::NOENEMIES   );
		LuaPushNamedNumber(L, "noFriendlies", Collision::NOFRIENDLIES);
		LuaPushNamedNumber(L, "noFeatures",   Collision::NOFEATURES  );
		LuaPushNamedNumber(L, "noNeutrals",   Collision::NONEUTRALS  );
		LuaPushNamedNumber(L, "noFirebases",  Collision::NOFIREBASES );
		LuaPushNamedNumber(L, "noGround",     Collision::NOGROUND    );
		LuaPushNamedNumber(L, "noCloaked",    Collision::NOCLOAKED   );
		LuaPushNamedNumber(L, "noUnits",      Collision::NOUNITS     );
	lua_rawset(L, -3);

	return true;
}


/******************************************************************************/
/******************************************************************************/
