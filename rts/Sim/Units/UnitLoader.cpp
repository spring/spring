/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "UnitLoader.h"
#include "Unit.h"
#include "UnitDef.h"
#include "UnitDefHandler.h"
#include "UnitHandler.h"

#include "CommandAI/AirCAI.h"
#include "CommandAI/BuilderCAI.h"
#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/MobileCAI.h"

#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"

#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/TeamHandler.h"

#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Platform/Watchdog.h"



CUnitLoader* CUnitLoader::GetInstance()
{
	// NOTE: UnitLoader has no internal state, so this is fine wrt. reloading
	static CUnitLoader instance;
	return &instance;
}

CCommandAI* CUnitLoader::NewCommandAI(CUnit* u, const UnitDef* ud)
{
	static_assert(sizeof(CFactoryCAI) <= sizeof(u->caiMemBuffer), "");
	static_assert(sizeof(CBuilderCAI) <= sizeof(u->caiMemBuffer), "");
	static_assert(sizeof(    CAirCAI) <= sizeof(u->caiMemBuffer), "");
	static_assert(sizeof( CMobileCAI) <= sizeof(u->caiMemBuffer), "");
	static_assert(sizeof( CCommandAI) <= sizeof(u->caiMemBuffer), "");

	if (ud->IsFactoryUnit())
		return (new (u->caiMemBuffer) CFactoryCAI(u));

	if (ud->IsMobileBuilderUnit() || ud->IsStaticBuilderUnit())
		return (new (u->caiMemBuffer) CBuilderCAI(u));

	// non-hovering fighter or bomber aircraft; coupled to StrafeAirMoveType
	if (ud->IsStrafingAirUnit())
		return (new (u->caiMemBuffer) CAirCAI(u));
	// all other aircraft; coupled to HoverAirMoveType
	if (ud->IsAirUnit())
		return (new (u->caiMemBuffer) CMobileCAI(u));

	if (ud->IsGroundUnit() || ud->IsTransportUnit())
		return (new (u->caiMemBuffer) CMobileCAI(u));

	return (new (u->caiMemBuffer) CCommandAI(u));
}

CUnit* CUnitLoader::LoadUnit(const std::string& name, const UnitLoadParams& params)
{
	const_cast<UnitLoadParams&>(params).unitDef = unitDefHandler->GetUnitDefByName(name);

	if (params.unitDef == nullptr)
		throw content_error("Couldn't find unittype " +  name);

	return (LoadUnit(params));
}

CUnit* CUnitLoader::LoadUnit(const UnitLoadParams& params)
{
	CUnit* unit = nullptr;

	{
		const UnitDef* ud = params.unitDef;

		if (ud == nullptr)
			return unit;
		// need to check this BEFORE creating the instance
		if (!unitHandler.CanAddUnit(params.unitID))
			return unit;

		if (params.teamID < 0) {
			if (teamHandler.GaiaTeamID() < 0) {
				LOG_L(L_WARNING, "[%s] invalid team %d and no Gaia-team", __func__, params.teamID);
				return unit;
			}

			const_cast<UnitLoadParams&>(params).teamID = teamHandler.GaiaTeamID();
		}

		unit = CUnitHandler::NewUnit(ud);

		unit->PreInit(params);
		unit->PostInit(params.builder);
	}

	if (params.flattenGround)
		FlattenGround(unit);

	return unit;
}



void CUnitLoader::ParseAndExecuteGiveUnitsCommand(const std::vector<std::string>& args, int team)
{
	if (args.size() < 2) {
		LOG_L(L_WARNING, "[%s] not enough arguments (\"/give [amount] <objectName | 'all'> [team] [@x, y, z]\")", __FUNCTION__);
		return;
	}

	float3 pos;
	if (sscanf(args[args.size() - 1].c_str(), "@%f, %f, %f", &pos.x, &pos.y, &pos.z) != 3) {
		LOG_L(L_WARNING, "[%s] invalid position argument (\"/give [amount] <objectName | 'all'> [team] [@x, y, z]\")", __FUNCTION__);
		return;
	}

	int amount = 1;
	int amountArgIdx = -1;
	int teamArgIdx = -1;

	if (args.size() == 4) {
		amountArgIdx = 0;
		teamArgIdx = 2;
	}
	else if (args.size() == 3) {
		if (args[0].find_first_not_of("0123456789") == std::string::npos) {
			amountArgIdx = 0;
		} else {
			teamArgIdx = 1;
		}
	}

	if (amountArgIdx >= 0) {
		amount = atoi(args[amountArgIdx].c_str());

		if ((amount < 0) || (args[amountArgIdx].find_first_not_of("0123456789") != std::string::npos)) {
			LOG_L(L_WARNING, "[%s] invalid amount argument: %s", __FUNCTION__, args[amountArgIdx].c_str());
			return;
		}
	}

	int featureAllyTeam = -1;
	if (teamArgIdx >= 0) {
		team = atoi(args[teamArgIdx].c_str());

		if ((!teamHandler.IsValidTeam(team)) || (args[teamArgIdx].find_first_not_of("0123456789") != std::string::npos)) {
			LOG_L(L_WARNING, "[%s] invalid team argument: %s", __FUNCTION__, args[teamArgIdx].c_str());
			return;
		}

		featureAllyTeam = teamHandler.AllyTeam(team);
	}

	const std::string& objectName = (amountArgIdx >= 0) ? args[1] : args[0];

	if (objectName.empty()) {
		LOG_L(L_WARNING, "[%s] invalid object-name argument", __FUNCTION__);
		return;
	}

	GiveUnits(objectName, pos, amount, team, featureAllyTeam);
}


void CUnitLoader::GiveUnits(const std::string& objectName, float3 pos, int amount, int team, int featureAllyTeam)
{
	const CTeam* receivingTeam = teamHandler.Team(team);

	if (objectName == "all") {
		unsigned int numRequestedUnits = unitDefHandler->NumUnitDefs();
		unsigned int currentNumUnits = receivingTeam->GetNumUnits();

		// make sure team unit-limit is not exceeded
		if ((currentNumUnits + numRequestedUnits) > receivingTeam->GetMaxUnits())
			numRequestedUnits = receivingTeam->GetMaxUnits() - currentNumUnits;

		// make sure square is entirely on the map
		const int sqSize = math::ceil(math::sqrt((float) numRequestedUnits));
		const float sqHalfMapSize = sqSize / 2 * 10 * SQUARE_SIZE;

		pos.x = Clamp(pos.x, sqHalfMapSize, float3::maxxpos - sqHalfMapSize - 1);
		pos.z = Clamp(pos.z, sqHalfMapSize, float3::maxzpos - sqHalfMapSize - 1);

		for (int a = 1; a <= numRequestedUnits; ++a) {
			Watchdog::ClearTimers(false, true);

			const float px = pos.x + (a % sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
			const float pz = pos.z + (a / sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
			const UnitDef* ud = unitDefHandler->GetUnitDefByID(a);

			const UnitLoadParams unitParams = {
				ud,
				nullptr,

				float3(px, CGround::GetHeightReal(px, pz), pz),
				ZeroVector,

				-1,
				team,
				FACING_SOUTH,

				false,
				true,
			};

			LoadUnit(unitParams);
		}
	} else {
		unsigned int numRequestedUnits = amount;
		unsigned int currentNumUnits = receivingTeam->GetNumUnits();

		if (receivingTeam->AtUnitLimit()) {
			LOG_L(L_WARNING,
				"[%s] unable to give more units to team %d (current: %u, team limit: %u, global limit: %u)",
				__FUNCTION__, team, currentNumUnits, receivingTeam->GetMaxUnits(), unitHandler.MaxUnits()
			);
			return;
		}

		// make sure team unit-limit is not exceeded
		if ((currentNumUnits + numRequestedUnits) > receivingTeam->GetMaxUnits())
			numRequestedUnits = receivingTeam->GetMaxUnits() - currentNumUnits;

		const UnitDef* unitDef = unitDefHandler->GetUnitDefByName(objectName);
		const FeatureDef* featureDef = featureDefHandler->GetFeatureDef(objectName, false);

		if (unitDef == nullptr && featureDef == nullptr) {
			LOG_L(L_WARNING, "[%s] %s is not a valid object-name", __FUNCTION__, objectName.c_str());
			return;
		}

		if (unitDef != nullptr) {
			const int xsize = unitDef->xsize;
			const int zsize = unitDef->zsize;
			const int squareSize = math::ceil(math::sqrt((float) numRequestedUnits));
			const float3 squarePos = float3(
				pos.x - (((squareSize - 1) * xsize * SQUARE_SIZE) / 2),
				pos.y,
				pos.z - (((squareSize - 1) * zsize * SQUARE_SIZE) / 2)
			);

			int unitsLoaded = numRequestedUnits;

			for (int z = 0; z < squareSize; ++z) {
				for (int x = 0; x < squareSize && (unitsLoaded-- > 0); ++x) {
					const float px = squarePos.x + x * xsize * SQUARE_SIZE;
					const float pz = squarePos.z + z * zsize * SQUARE_SIZE;

					Watchdog::ClearTimers(false, true);

					const UnitLoadParams unitParams = {
						unitDef,
						nullptr,

						float3(px, CGround::GetHeightReal(px, pz), pz),
						ZeroVector,

						-1,
						team,
						FACING_SOUTH,

						false,
						true,
					};

					LoadUnit(unitParams);
				}
			}

			LOG("[%s] spawned %i %s unit(s) for team %i",
					__FUNCTION__, numRequestedUnits, objectName.c_str(), team);
		}

		if (featureDef != nullptr) {
			if (featureAllyTeam < 0)
				team = -1; // default to world features

			const int xsize = featureDef->xsize;
			const int zsize = featureDef->zsize;
			const int squareSize = math::ceil(math::sqrt((float) numRequestedUnits));
			const float3 squarePos = float3(
				pos.x - (((squareSize - 1) * xsize * SQUARE_SIZE) / 2),
				pos.y,
				pos.z - (((squareSize - 1) * zsize * SQUARE_SIZE) / 2)
			);

			int total = amount; // FIXME -- feature count limit?

			for (int z = 0; z < squareSize; ++z) {
				for (int x = 0; x < squareSize && total > 0; ++x) {
					const float px = squarePos.x + x * xsize * SQUARE_SIZE;
					const float pz = squarePos.z + z * zsize * SQUARE_SIZE;
					const float3 featurePos = float3(px, CGround::GetHeightReal(px, pz), pz);

					Watchdog::ClearTimers(false, true);
					FeatureLoadParams params = {
						nullptr,
						nullptr,
						featureDef,

						featurePos,
						ZeroVector,

						team,
						featureAllyTeam,

						0, // rotation
						FACING_SOUTH,

						0, // wreckLevels
						0, // smokeTime
					};

					featureHandler.LoadFeature(params);

					--total;
				}
			}

			LOG("[%s] spawned %i %s feature(s) for team %i",
					__FUNCTION__, numRequestedUnits, objectName.c_str(), team);
		}
	}
}




void CUnitLoader::FlattenGround(const CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;
	// const MoveDef* moveDef = unit->moveDef;

	if (mapDamage->Disabled())
		return;
	if (!unitDef->levelGround)
		return;
	if (unitDef->IsAirUnit())
		return;
	if (!unitDef->IsImmobileUnit())
		return;
	if (unit->FloatOnWater() && unit->IsInWater())
		return;

	// if we are float-capable, only flatten
	// if the terrain here is above sea level
	BuildInfo bi(unitDef, unit->pos, unit->buildFacing);
	bi.pos = CGameHelper::Pos2BuildPos(bi, true);

	const float hss = 0.5f * SQUARE_SIZE;
	const int tx1 = (int) std::max(0.0f ,(bi.pos.x - (bi.GetXSize() * hss)) / SQUARE_SIZE);
	const int tz1 = (int) std::max(0.0f ,(bi.pos.z - (bi.GetZSize() * hss)) / SQUARE_SIZE);
	const int tx2 = std::min(mapDims.mapx, tx1 + bi.GetXSize());
	const int tz2 = std::min(mapDims.mapy, tz1 + bi.GetZSize());

	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			readMap->SetHeight(z * mapDims.mapxp1 + x, bi.pos.y);
		}
	}

	mapDamage->RecalcArea(tx1, tx2, tz1, tz2);
}

void CUnitLoader::RestoreGround(const CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;

	if (mapDamage->Disabled())
		return;
	if (!unitDef->levelGround)
		return;
	if (unitDef->IsAirUnit())
		return;
	if (!unitDef->IsImmobileUnit())
		return;
	if (unit->FloatOnWater() && unit->IsInWater())
		return;

	BuildInfo bi(unitDef, unit->pos, unit->buildFacing);
	bi.pos = CGameHelper::Pos2BuildPos(bi, true);
	const float hss = 0.5f * SQUARE_SIZE;
	const int tx1 = (int) std::max(0.0f ,(bi.pos.x - (bi.GetXSize() * hss)) / SQUARE_SIZE);
	const int tz1 = (int) std::max(0.0f ,(bi.pos.z - (bi.GetZSize() * hss)) / SQUARE_SIZE);
	const int tx2 = std::min(mapDims.mapx, tx1 + bi.GetXSize());
	const int tz2 = std::min(mapDims.mapy, tz1 + bi.GetZSize());


	const float* heightmap = readMap->GetCornerHeightMapSynced();
	int num = 0;
	float heightdiff = 0.0f;
	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			int index = z * mapDims.mapxp1 + x;
			heightdiff += heightmap[index] - readMap->GetOriginalHeightMapSynced()[index];
			++num;
		}
	}
	// adjust the terrain profile to match orgheightmap
	heightdiff /= (float)num;
	heightdiff += unit->pos.y - bi.pos.y;
	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			int index = z * mapDims.mapxp1 + x;
			readMap->SetHeight(index, heightdiff + readMap->GetOriginalHeightMapSynced()[index]);
		}
	}
	// but without affecting the build height
	heightdiff = bi.pos.y - CGameHelper::Pos2BuildPos(bi, true).y;
	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			int index = z * mapDims.mapxp1 + x;
			readMap->SetHeight(index, heightdiff + heightmap[index]);
		}
	}

	mapDamage->RecalcArea(tx1, tx2, tz1, tz2);
}

