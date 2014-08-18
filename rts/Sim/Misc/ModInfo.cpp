/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ModInfo.h"

#include "Game/GameSetup.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/Exceptions.h"
#include "System/GlobalConfig.h"
#include "System/myMath.h"

CModInfo modInfo;


void CModInfo::Init(const char* modArchive)
{
	filename = modArchive;
	humanNameVersioned = archiveScanner->NameFromArchive(modArchive);

	const CArchiveScanner::ArchiveData md = archiveScanner->GetArchiveData(humanNameVersioned);

	humanName   = md.GetName();
	shortName   = md.GetShortName();
	version     = md.GetVersion();
	mutator     = md.GetMutator();
	description = md.GetDescription();

	// initialize the parser
	LuaParser parser("gamedata/modrules.lua",
	                 SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	// customize the defs environment
	parser.GetTable("Spring");
	parser.AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
	parser.EndTable();
	parser.Execute();

	if (!parser.IsValid()) {
		LOG_L(L_ERROR, "Failed loading mod-rules, using defaults; error: %s", parser.GetErrorLog().c_str());
	}

	const LuaTable& root = parser.GetRoot();

	{
		// system
		const LuaTable& system = root.SubTable("system");
		pathFinderSystem = system.GetInt("pathFinderSystem", PFS_TYPE_DEFAULT) % PFS_NUM_TYPES;

		pfUpdateRate = system.GetFloat("pathFinderUpdateRate", 1.0f);
	}

	{
		// movement
		const LuaTable& movementTbl = root.SubTable("movement");

		allowAircraftToLeaveMap = movementTbl.GetBool("allowAirPlanesToLeaveMap", true);
		allowAircraftToHitGround = movementTbl.GetBool("allowAircraftToHitGround", true);
		allowPushingEnemyUnits = movementTbl.GetBool("allowPushingEnemyUnits", false);
		allowCrushingAlliedUnits = movementTbl.GetBool("allowCrushingAlliedUnits", false);
		allowUnitCollisionDamage = movementTbl.GetBool("allowUnitCollisionDamage", false);
		allowUnitCollisionOverlap = movementTbl.GetBool("allowUnitCollisionOverlap", true);
		allowGroundUnitGravity = movementTbl.GetBool("allowGroundUnitGravity", true);
		allowHoverUnitStrafing = movementTbl.GetBool("allowHoverUnitStrafing", (pathFinderSystem == PFS_TYPE_QTPFS));
		useClassicGroundMoveType = movementTbl.GetBool("useClassicGroundMoveType", false);
	}

	{
		// construction
		const LuaTable& constructionTbl = root.SubTable("construction");

		constructionDecay = constructionTbl.GetBool("constructionDecay", true);
		constructionDecayTime = (int)(constructionTbl.GetFloat("constructionDecayTime", 6.66) * GAME_SPEED);
		constructionDecaySpeed = std::max(constructionTbl.GetFloat("constructionDecaySpeed", 0.03), 0.01f);
	}

	{
		// reclaim
		const LuaTable& reclaimTbl = root.SubTable("reclaim");

		multiReclaim  = reclaimTbl.GetInt("multiReclaim",  0);
		reclaimMethod = reclaimTbl.GetInt("reclaimMethod", 1);
		reclaimUnitMethod = reclaimTbl.GetInt("unitMethod", 1);
		reclaimUnitEnergyCostFactor = reclaimTbl.GetFloat("unitEnergyCostFactor", 0.0);
		reclaimUnitEfficiency = reclaimTbl.GetFloat("unitEfficiency", 1.0);
		reclaimFeatureEnergyCostFactor = reclaimTbl.GetFloat("featureEnergyCostFactor", 0.0);
		reclaimAllowEnemies = reclaimTbl.GetBool("allowEnemies", true);
		reclaimAllowAllies = reclaimTbl.GetBool("allowAllies", true);
	}

	{
		// repair
		const LuaTable& repairTbl = root.SubTable("repair");
		repairEnergyCostFactor = repairTbl.GetFloat("energyCostFactor", 0.0);
	}

	{
		// resurrect
		const LuaTable& resurrectTbl = root.SubTable("resurrect");
		resurrectEnergyCostFactor  = resurrectTbl.GetFloat("energyCostFactor",  0.5);
	}

	{
		// capture
		const LuaTable& captureTbl = root.SubTable("capture");
		captureEnergyCostFactor = captureTbl.GetFloat("energyCostFactor", 0.0);
	}

	{
		// paralyze
		const LuaTable& paralyzeTbl = root.SubTable("paralyze");
		paralyzeOnMaxHealth = paralyzeTbl.GetBool("paralyzeOnMaxHealth", true);
	}

	{
		// fire-at-dead-units
		const LuaTable& fireAtDeadTbl = root.SubTable("fireAtDead");

		fireAtKilled   = fireAtDeadTbl.GetBool("fireAtKilled", false);
		fireAtCrashing = fireAtDeadTbl.GetBool("fireAtCrashing", false);
	}

	{
		// transportability
		const LuaTable& transportTbl = root.SubTable("transportability");

		transportAir    = transportTbl.GetBool("transportAir",   false);
		transportShip   = transportTbl.GetBool("transportShip",  false);
		transportHover  = transportTbl.GetBool("transportHover", false);
		transportGround = transportTbl.GetBool("transportGround", true);

		targetableTransportedUnits = transportTbl.GetBool("targetableTransportedUnits", false);
	}

	{
		// experience
		const LuaTable& experienceTbl = root.SubTable("experience");

		CUnit::SetExpMultiplier (experienceTbl.GetFloat("experienceMult", 1.0f));
		CUnit::SetExpPowerScale (experienceTbl.GetFloat("powerScale",  1.0f));
		CUnit::SetExpHealthScale(experienceTbl.GetFloat("healthScale", 0.7f));
		CUnit::SetExpReloadScale(experienceTbl.GetFloat("reloadScale", 0.4f));
	}

	{
		// flanking bonus
		const LuaTable& flankingBonusTbl = root.SubTable("flankingBonus");
		flankingBonusModeDefault = flankingBonusTbl.GetInt("defaultMode", 1);
	}

	{
		// feature visibility
		const LuaTable& featureLOS = root.SubTable("featureLOS");

		featureVisibility = featureLOS.GetInt("featureVisibility", FEATURELOS_ALL);
		featureVisibility = Clamp(featureVisibility, int(FEATURELOS_NONE), int(FEATURELOS_ALL));
	}

	{
		// sensors, line-of-sight
		const LuaTable& sensors = root.SubTable("sensors");
		const LuaTable& los = sensors.SubTable("los");

		requireSonarUnderWater = sensors.GetBool("requireSonarUnderWater", true);

		// losMipLevel is used as index to readMap->mipHeightmaps,
		// so the max value is CReadMap::numHeightMipMaps - 1
		losMipLevel = los.GetInt("losMipLevel", 1);
		losMul = los.GetFloat("losMul", 1.0f);
		// airLosMipLevel doesn't have such restrictions, it's just used in various
		// bitshifts with signed integers
		airMipLevel = los.GetInt("airMipLevel", 2);
		airLosMul = los.GetFloat("airLosMul", 1.0f);

		if ((losMipLevel < 0) || (losMipLevel > 6)) {
			throw content_error("Sensors\\Los\\LosMipLevel out of bounds. "
				                "The minimum value is 0. The maximum value is 6.");
		}

		if ((airMipLevel < 0) || (airMipLevel > 30)) {
			throw content_error("Sensors\\Los\\AirLosMipLevel out of bounds. "
				                "The minimum value is 0. The maximum value is 30.");
		}
	}
}

