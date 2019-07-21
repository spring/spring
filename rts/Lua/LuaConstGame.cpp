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
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/StringUtil.h"


bool LuaConstGame::PushEntries(lua_State* L)
{
	{
		// game, should perhaps be moved over to ConstEngine
		LuaPushNamedNumber(L, "maxTeams"  , MAX_TEAMS  );
		LuaPushNamedNumber(L, "maxPlayers", MAX_PLAYERS);
		LuaPushNamedNumber(L, "gameSpeed" , GAME_SPEED );
		LuaPushNamedNumber(L, "squareSize", SQUARE_SIZE);
	}

	if (CGameSetup::ScriptLoaded()) {
		// game-setup
		LuaPushNamedNumber(L, "startPosType"    , gameSetup->startPosType);
		LuaPushNamedBool(L,   "ghostedBuildings", gameSetup->ghostedBuildings);
	}

	if (unitHandler.MaxUnits() > 0) {
		// simulation; values are meaningless prior to Game::Load
		LuaPushNamedNumber(L, "maxUnits", unitHandler.MaxUnits());

		// NB: not constants
		LuaPushNamedNumber(L, "windMin" , envResHandler.GetMinWindStrength());
		LuaPushNamedNumber(L, "windMax" , envResHandler.GetMaxWindStrength());

		// map-damage; enabled iff !mapInfo->map.notDeformable
		LuaPushNamedBool(L, "mapDamage", !mapDamage->Disabled());
	}

	if (readMap != nullptr) {
		// FIXME: make this available in LoadScreen (LuaIntro) already
		// requires pre-parsing the map header and filling in mapDims
		LuaPushNamedNumber(L, "mapX"    , mapDims.mapx / 64);
		LuaPushNamedNumber(L, "mapY"    , mapDims.mapy / 64);
		LuaPushNamedNumber(L, "mapSizeX", mapDims.mapx * SQUARE_SIZE);
		LuaPushNamedNumber(L, "mapSizeZ", mapDims.mapy * SQUARE_SIZE);
	}

	if (mapInfo != nullptr) {
		// map-info
		LuaPushNamedString(L, "mapName"        ,  mapInfo->map.name);
		LuaPushNamedString(L, "mapDescription" ,  mapInfo->map.description);
		LuaPushNamedNumber(L, "mapHardness"    ,  mapInfo->map.hardness);
		LuaPushNamedNumber(L, "extractorRadius",  mapInfo->map.extractorRadius);
		LuaPushNamedNumber(L, "tidal"          ,  mapInfo->map.tidalStrength); // NB: not constant
		LuaPushNamedNumber(L, "waterDamage"    ,  mapInfo->water.damage);
		LuaPushNamedNumber(L, "gravity"        , -mapInfo->map.gravity * GAME_SPEED * GAME_SPEED);
	}

	if (!modInfo.filename.empty()) {
		// mod-info; values are meaningless prior to Game::Load
		LuaPushNamedString(L, "gameName"     , modInfo.humanName);
		LuaPushNamedString(L, "gameShortName", modInfo.shortName);
		LuaPushNamedString(L, "gameVersion"  , modInfo.version);
		LuaPushNamedString(L, "gameMutator"  , modInfo.mutator);
		LuaPushNamedString(L, "gameDesc"     , modInfo.description);

		LuaPushNamedString(L, "modName"      , modInfo.humanNameVersioned);
		LuaPushNamedString(L, "modShortName" , modInfo.shortName);
		LuaPushNamedString(L, "modVersion"   , modInfo.version);
		LuaPushNamedString(L, "modMutator"   , modInfo.mutator);
		LuaPushNamedString(L, "modDesc"      , modInfo.description);

		LuaPushNamedBool  (L, "constructionDecay"     , modInfo.constructionDecay);
		LuaPushNamedNumber(L, "constructionDecayTime" , modInfo.constructionDecayTime);
		LuaPushNamedNumber(L, "constructionDecaySpeed", modInfo.constructionDecaySpeed);

		LuaPushNamedNumber(L, "multiReclaim"                  , modInfo.multiReclaim);
		LuaPushNamedNumber(L, "reclaimMethod"                 , modInfo.reclaimMethod);
		LuaPushNamedNumber(L, "reclaimUnitMethod"             , modInfo.reclaimUnitMethod);
		LuaPushNamedNumber(L, "reclaimUnitEnergyCostFactor"   , modInfo.reclaimUnitEnergyCostFactor);
		LuaPushNamedNumber(L, "reclaimUnitEfficiency"         , modInfo.reclaimUnitEfficiency);
		LuaPushNamedNumber(L, "reclaimFeatureEnergyCostFactor", modInfo.reclaimFeatureEnergyCostFactor);
		LuaPushNamedBool  (L, "reclaimAllowEnemies"           , modInfo.reclaimAllowEnemies);
		LuaPushNamedBool  (L, "reclaimAllowAllies"            , modInfo.reclaimAllowAllies);
		LuaPushNamedNumber(L, "repairEnergyCostFactor"        , modInfo.repairEnergyCostFactor);
		LuaPushNamedNumber(L, "resurrectEnergyCostFactor"     , modInfo.resurrectEnergyCostFactor);
		LuaPushNamedNumber(L, "captureEnergyCostFactor"       , modInfo.captureEnergyCostFactor);

		LuaPushNamedNumber(L, "transportAir"   , modInfo.transportAir);
		LuaPushNamedNumber(L, "transportShip"  , modInfo.transportShip);
		LuaPushNamedNumber(L, "transportHover" , modInfo.transportHover);
		LuaPushNamedNumber(L, "transportGround", modInfo.transportGround);
		LuaPushNamedNumber(L, "fireAtKilled"   , modInfo.fireAtKilled);
		LuaPushNamedNumber(L, "fireAtCrashing" , modInfo.fireAtCrashing);

		LuaPushNamedNumber(L, "requireSonarUnderWater", modInfo.requireSonarUnderWater);

		LuaPushNamedBool  (L, "paralyzeOnMaxHealth", modInfo.paralyzeOnMaxHealth);
		LuaPushNamedNumber(L, "paralyzeDeclineRate", modInfo.paralyzeDeclineRate);
	}

	if (archiveScanner != nullptr && mapInfo != nullptr) {
		// archive checksums
		sha512::hex_digest mapHexDigest;
		sha512::hex_digest modHexDigest;
		sha512::dump_digest(archiveScanner->GetArchiveCompleteChecksumBytes(mapInfo->map.name), mapHexDigest);
		sha512::dump_digest(archiveScanner->GetArchiveCompleteChecksumBytes(modInfo.filename), modHexDigest);

		LuaPushNamedString(L, "mapChecksum", mapHexDigest.data());
		LuaPushNamedString(L, "modChecksum", modHexDigest.data());
	}

	{
		// deprecated nonsense
		LuaPushNamedBool(L, "allowTeamColors", true);
	}

	{
		// NB: instance is never null, but might not contain any data yet (e.g. in LuaIntro)
		const std::vector<string>& cats = CCategoryHandler::Instance()->GetCategoryNames(~0);

		lua_pushliteral(L, "springCategories");
		lua_createtable(L, 0, cats.size());

		for (unsigned int i = 0; i < cats.size(); i++) {
			LuaPushNamedNumber(L, StringToLower(cats[i]), i);
		}

		lua_rawset(L, -3);
	}

	{
		// NB: empty for LuaIntro and LuaParser which also push ConstGame entries
		const std::vector<std::string>& typeList = damageArrayHandler.GetTypeList();

		lua_pushliteral(L, "armorTypes");
		lua_createtable(L, typeList.size(), typeList.size());

		for (size_t i = 0; i < typeList.size(); i++) {
			// bidirectional map (k,v) and (v,k)
			lua_pushsstring(L, typeList[i]);
			lua_pushnumber(L, i);
			lua_rawset(L, -3);
			lua_pushsstring(L, typeList[i]);
			lua_rawseti(L, -2, i);
		}

		lua_rawset(L, -3);
	}

	{
		// environmental damage types
		lua_pushliteral(L, "envDamageTypes");
		lua_createtable(L, 0, 7);
			LuaPushNamedNumber(L, "Debris"         , -CSolidObject::DAMAGE_EXPLOSION_DEBRIS );
			LuaPushNamedNumber(L, "GroundCollision", -CSolidObject::DAMAGE_COLLISION_GROUND );
			LuaPushNamedNumber(L, "ObjectCollision", -CSolidObject::DAMAGE_COLLISION_OBJECT );
			LuaPushNamedNumber(L, "Fire"           , -CSolidObject::DAMAGE_EXTSOURCE_FIRE   );
			LuaPushNamedNumber(L, "Water"          , -CSolidObject::DAMAGE_EXTSOURCE_WATER  );
			LuaPushNamedNumber(L, "Killed"         , -CSolidObject::DAMAGE_EXTSOURCE_KILLED );
			LuaPushNamedNumber(L, "Crushed"        , -CSolidObject::DAMAGE_EXTSOURCE_CRUSHED);
		lua_rawset(L, -3);
	}
	{
		// weapon avoidance and projectile collision flags
		lua_pushliteral(L, "collisionFlags");
		lua_createtable(L, 0, 9);
			LuaPushNamedNumber(L, "noEnemies"   , Collision::NOENEMIES   );
			LuaPushNamedNumber(L, "noFriendlies", Collision::NOFRIENDLIES);
			LuaPushNamedNumber(L, "noFeatures"  , Collision::NOFEATURES  );
			LuaPushNamedNumber(L, "noNeutrals"  , Collision::NONEUTRALS  );
			LuaPushNamedNumber(L, "noFireBases" , Collision::NOFIREBASES );
			LuaPushNamedNumber(L, "noNonTargets", Collision::NONONTARGETS);
			LuaPushNamedNumber(L, "noGround"    , Collision::NOGROUND    );
			LuaPushNamedNumber(L, "noCloaked"   , Collision::NOCLOAKED   );
			LuaPushNamedNumber(L, "noUnits"     , Collision::NOUNITS     );
		lua_rawset(L, -3);
	}
	{
		// MoveDef speed-modifier types
		lua_pushliteral(L, "speedModClasses");
		lua_createtable(L, 0, 5);
			LuaPushNamedNumber(L, "Tank" , MoveDef::SpeedModClass::Tank );
			LuaPushNamedNumber(L, "KBot" , MoveDef::SpeedModClass::KBot );
			LuaPushNamedNumber(L, "Hover", MoveDef::SpeedModClass::Hover);
			LuaPushNamedNumber(L, "Boat" , MoveDef::SpeedModClass::Ship );
			LuaPushNamedNumber(L, "Ship" , MoveDef::SpeedModClass::Ship );
		lua_rawset(L, -3);
	}

	return true;
}

