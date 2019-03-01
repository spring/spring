/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ModInfo.h"

#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"

CModInfo modInfo;

void CModInfo::ResetState()
{
	filename.clear();
	humanName.clear();
	humanNameVersioned.clear();
	shortName.clear();
	version.clear();
	mutator.clear();
	description.clear();

	{
		allowDirectionalPathing    = true;
		allowAircraftToLeaveMap    = true;
		allowAircraftToHitGround   = true;
		allowPushingEnemyUnits     = false;
		allowCrushingAlliedUnits   = false;
		allowUnitCollisionDamage   = false;
		allowUnitCollisionOverlap  = true;
		allowSepAxisCollisionTest  = false;
		allowGroundUnitGravity     = true;
		allowHoverUnitStrafing     = true;
	}
	{
		constructionDecay      = true;
		constructionDecayTime  = 1000;
		constructionDecaySpeed = 1.0f;
	}
	{
		multiReclaim                   = 1;
		reclaimMethod                  = 1;
		reclaimUnitMethod              = 1;
		reclaimUnitEnergyCostFactor    = 0.0f;
		reclaimUnitEfficiency          = 1.0f;
		reclaimFeatureEnergyCostFactor = 0.0f;
		reclaimAllowEnemies            = true;
		reclaimAllowAllies             = true;
	}
	{
		repairEnergyCostFactor    = 0.0f;
		resurrectEnergyCostFactor = 0.5f;
		captureEnergyCostFactor   = 0.0f;
	}
	{
		unitExpMultiplier  = 0.0f;
		unitExpPowerScale  = 0.0f;
		unitExpHealthScale = 0.0f;
		unitExpReloadScale = 0.0f;
	}
	{
		paralyzeDeclineRate = 40.0f;
		paralyzeOnMaxHealth = true;
	}
	{
		transportGround            = 1;
		transportHover             = 0;
		transportShip              = 0;
		transportAir               = 0;
		targetableTransportedUnits = 0;
	}
	{
		fireAtKilled   = 0;
		fireAtCrashing = 0;
	}
	{
		flankingBonusModeDefault = 0;
	}
	{
		losMipLevel = 0;
		airMipLevel = 0;
		radarMipLevel = 0;

		requireSonarUnderWater = true;
		alwaysVisibleOverridesCloaked = false;
		separateJammers = true;
	}
	{
		featureVisibility = FEATURELOS_NONE;
	}
	{
		pathFinderSystem = NOPFS_TYPE;
		pfRawDistMult    = 1.25f;
		pfUpdateRate     = 0.007f;

		allowTake = true;
	}
}

void CModInfo::Init(const std::string& modFileName)
{
	{
		filename = modFileName;
		humanNameVersioned = archiveScanner->GameHumanNameFromArchive(modFileName);

		const CArchiveScanner::ArchiveData& md = archiveScanner->GetArchiveData(humanNameVersioned);

		humanName   = md.GetName();
		shortName   = md.GetShortName();
		version     = md.GetVersion();
		mutator     = md.GetMutator();
		description = md.GetDescription();
	}

	LuaParser parser("gamedata/modrules.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	// customize the defs environment
	parser.GetTable("Spring");
	parser.AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
	parser.EndTable();
	parser.Execute();

	if (!parser.IsValid())
		LOG_L(L_ERROR, "[ModInfo::%s] error \"%s\" loading mod-rules, using defaults", __func__, parser.GetErrorLog().c_str());

	const LuaTable& root = parser.GetRoot();

	{
		// system
		const LuaTable& system = root.SubTable("system");

		pathFinderSystem = Clamp(system.GetInt("pathFinderSystem", HAPFS_TYPE), int(NOPFS_TYPE), int(QTPFS_TYPE));
		pfRawDistMult = system.GetFloat("pathFinderRawDistMult", pfRawDistMult);
		pfUpdateRate = system.GetFloat("pathFinderUpdateRate", pfUpdateRate);

		allowTake = system.GetBool("allowTake", allowTake);
	}

	{
		// movement
		const LuaTable& movementTbl = root.SubTable("movement");

		allowDirectionalPathing = movementTbl.GetBool("allowDirectionalPathing", allowDirectionalPathing);
		allowAircraftToLeaveMap = movementTbl.GetBool("allowAirPlanesToLeaveMap", allowAircraftToLeaveMap);
		allowAircraftToHitGround = movementTbl.GetBool("allowAircraftToHitGround", allowAircraftToHitGround);
		allowPushingEnemyUnits = movementTbl.GetBool("allowPushingEnemyUnits", allowPushingEnemyUnits);
		allowCrushingAlliedUnits = movementTbl.GetBool("allowCrushingAlliedUnits", allowCrushingAlliedUnits);
		allowUnitCollisionDamage = movementTbl.GetBool("allowUnitCollisionDamage", allowUnitCollisionDamage);
		allowUnitCollisionOverlap = movementTbl.GetBool("allowUnitCollisionOverlap", allowUnitCollisionOverlap);
		allowSepAxisCollisionTest = movementTbl.GetBool("allowSepAxisCollisionTest", allowSepAxisCollisionTest);
		allowGroundUnitGravity = movementTbl.GetBool("allowGroundUnitGravity", allowGroundUnitGravity);
		allowHoverUnitStrafing = movementTbl.GetBool("allowHoverUnitStrafing", (pathFinderSystem == QTPFS_TYPE));
	}

	{
		// construction
		const LuaTable& constructionTbl = root.SubTable("construction");

		constructionDecay = constructionTbl.GetBool("constructionDecay", constructionDecay);
		constructionDecayTime = (int)(constructionTbl.GetFloat("constructionDecayTime", 6.66) * GAME_SPEED);
		constructionDecaySpeed = std::max(constructionTbl.GetFloat("constructionDecaySpeed", 0.03), 0.01f);
	}

	{
		// reclaim
		const LuaTable& reclaimTbl = root.SubTable("reclaim");

		multiReclaim  = reclaimTbl.GetInt("multiReclaim",  0);
		reclaimMethod = reclaimTbl.GetInt("reclaimMethod", reclaimMethod);
		reclaimUnitMethod = reclaimTbl.GetInt("unitMethod", reclaimUnitMethod);
		reclaimUnitEnergyCostFactor = reclaimTbl.GetFloat("unitEnergyCostFactor", reclaimUnitEnergyCostFactor);
		reclaimUnitEfficiency = reclaimTbl.GetFloat("unitEfficiency", reclaimUnitEfficiency);
		reclaimFeatureEnergyCostFactor = reclaimTbl.GetFloat("featureEnergyCostFactor", reclaimFeatureEnergyCostFactor);
		reclaimAllowEnemies = reclaimTbl.GetBool("allowEnemies", reclaimAllowEnemies);
		reclaimAllowAllies = reclaimTbl.GetBool("allowAllies", reclaimAllowAllies);
	}

	{
		// repair
		const LuaTable& repairTbl = root.SubTable("repair");
		repairEnergyCostFactor = repairTbl.GetFloat("energyCostFactor", repairEnergyCostFactor);
	}

	{
		// resurrect
		const LuaTable& resurrectTbl = root.SubTable("resurrect");
		resurrectEnergyCostFactor  = resurrectTbl.GetFloat("energyCostFactor", resurrectEnergyCostFactor);
	}

	{
		// capture
		const LuaTable& captureTbl = root.SubTable("capture");
		captureEnergyCostFactor = captureTbl.GetFloat("energyCostFactor", captureEnergyCostFactor);
	}

	{
		// paralyze
		const LuaTable& paralyzeTbl = root.SubTable("paralyze");
		paralyzeDeclineRate = paralyzeTbl.GetFloat("paralyzeDeclineRate", paralyzeDeclineRate);
		paralyzeOnMaxHealth = paralyzeTbl.GetBool("paralyzeOnMaxHealth", paralyzeOnMaxHealth);
	}

	{
		// fire-at-dead-units
		const LuaTable& fireAtDeadTbl = root.SubTable("fireAtDead");

		fireAtKilled   = fireAtDeadTbl.GetBool("fireAtKilled", bool(fireAtKilled));
		fireAtCrashing = fireAtDeadTbl.GetBool("fireAtCrashing", bool(fireAtCrashing));
	}

	{
		// transportability
		const LuaTable& transportTbl = root.SubTable("transportability");

		transportAir    = transportTbl.GetBool("transportAir",    bool(transportAir   ));
		transportShip   = transportTbl.GetBool("transportShip",   bool(transportShip  ));
		transportHover  = transportTbl.GetBool("transportHover",  bool(transportHover ));
		transportGround = transportTbl.GetBool("transportGround", bool(transportGround));

		targetableTransportedUnits = transportTbl.GetBool("targetableTransportedUnits", bool(targetableTransportedUnits));
	}

	{
		// experience
		const LuaTable& experienceTbl = root.SubTable("experience");

		unitExpMultiplier  = experienceTbl.GetFloat("experienceMult", 1.0f);
		unitExpPowerScale  = experienceTbl.GetFloat(    "powerScale", 1.0f);
		unitExpHealthScale = experienceTbl.GetFloat(   "healthScale", 0.7f);
		unitExpReloadScale = experienceTbl.GetFloat(   "reloadScale", 0.4f);
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

		requireSonarUnderWater = sensors.GetBool("requireSonarUnderWater", requireSonarUnderWater);
		alwaysVisibleOverridesCloaked = sensors.GetBool("alwaysVisibleOverridesCloaked", alwaysVisibleOverridesCloaked);
		separateJammers = sensors.GetBool("separateJammers", separateJammers);

		// losMipLevel is used as index to readMap->mipHeightmaps,
		// so the maximum value is CReadMap::numHeightMipMaps - 1
		losMipLevel = los.GetInt("losMipLevel", 1);
		// airLosMipLevel doesn't have such restrictions, it's just
		// used in various bitshifts with signed integers
		airMipLevel = los.GetInt("airMipLevel", 1);
		radarMipLevel = los.GetInt("radarMipLevel", 2);

		if ((losMipLevel < 0) || (losMipLevel > 6))
			throw content_error("Sensors\\Los\\LosMipLevel out of bounds. The minimum value is 0. The maximum value is 6.");

		if ((radarMipLevel < 0) || (radarMipLevel > 6))
			throw content_error("Sensors\\Los\\RadarMipLevel out of bounds. The minimum value is 0. The maximum value is 6.");

		if ((airMipLevel < 0) || (airMipLevel > 30))
			throw content_error("Sensors\\Los\\AirLosMipLevel out of bounds. The minimum value is 0. The maximum value is 30.");
	}
}

