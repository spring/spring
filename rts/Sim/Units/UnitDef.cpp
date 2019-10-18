/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDef.h"
#include "UnitDefHandler.h"
#include "Game/GameSetup.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Rendering/IconHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"

/******************************************************************************/

UnitDefWeapon::UnitDefWeapon(const WeaponDef* weaponDef) {
	*this = UnitDefWeapon();
	this->def = weaponDef;
}

UnitDefWeapon::UnitDefWeapon(const WeaponDef* weaponDef, const LuaTable& weaponTable) {
	*this = UnitDefWeapon();
	this->def = weaponDef;

	this->slavedTo = weaponTable.GetInt("slaveTo", 0);

	// NOTE:
	//     <maxAngleDif> specifies the full-width arc,
	//     but we want the half-width arc internally
	//     (arcs are always symmetric around mainDir)
	this->maxMainDirAngleDif = math::cos((weaponTable.GetFloat("maxAngleDif", 360.0f) * 0.5f) * math::DEG_TO_RAD);

	const string& btcString = weaponTable.GetString("badTargetCategory", "");
	const string& otcString = weaponTable.GetString("onlyTargetCategory", "");

	this->badTargetCat =                                   CCategoryHandler::Instance()->GetCategories(btcString);
	this->onlyTargetCat = (otcString.empty())? 0xffffffff: CCategoryHandler::Instance()->GetCategories(otcString);

	this->mainDir = weaponTable.GetFloat3("mainDir", FwdVector);
	this->mainDir.SafeNormalize();
}



/******************************************************************************/

UnitDef::UnitDef()
	: SolidObjectDef()
	, cobID(-1)
	, decoyDef(nullptr)
	, metalUpkeep(0.0f)
	, energyUpkeep(0.0f)
	, metalMake(0.0f)
	, makesMetal(0.0f)
	, energyMake(0.0f)
	, buildTime(0.0f)
	, extractsMetal(0.0f)
	, extractRange(0.0f)
	, windGenerator(0.0f)
	, tidalGenerator(0.0f)
	, metalStorage(0.0f)
	, energyStorage(0.0f)
	, harvestMetalStorage(0.0f)
	, harvestEnergyStorage(0.0f)
	, autoHeal(0.0f)
	, idleAutoHeal(0.0f)
	, idleTime(0)
	, power(0.0f)
	, category(-1)
	, speed(0.0f)
	, rSpeed(0.0f)
	, turnRate(0.0f)
	, turnInPlace(false)
	, turnInPlaceSpeedLimit(0.0f)
	, turnInPlaceAngleLimit(0.0f)
	, collide(false)
	, losHeight(0.0f)
	, radarHeight(0.0f)
	, losRadius(0.0f)
	, airLosRadius(0.0f)
	, radarRadius(0.0f)
	, sonarRadius(0.0f)
	, jammerRadius(0.0f)
	, sonarJamRadius(0.0f)
	, seismicRadius(0.0f)
	, seismicSignature(0.0f)
	, stealth(false)
	, sonarStealth(false)
	, buildRange3D(false)
	, buildDistance(16.0f) // 16.0f is the minimum distance between two 1x1 units
	, buildSpeed(0.0f)
	, reclaimSpeed(0.0f)
	, repairSpeed(0.0f)
	, maxRepairSpeed(0.0f)
	, resurrectSpeed(0.0f)
	, captureSpeed(0.0f)
	, terraformSpeed(0.0f)

	, canSubmerge(false)
	, canfly(false)
	, floatOnWater(false)
	, pushResistant(false)
	, strafeToAttack(false)
	, minCollisionSpeed(0.0f)
	, slideTolerance(0.0f)
	, maxHeightDif(0.0f)
	, waterline(0.0f)
	, minWaterDepth(0.0f)
	, maxWaterDepth(0.0f)
	, pathType(-1U)
	, armoredMultiple(0.0f)
	, armorType(0)
	, flankingBonusMode(0)
	, flankingBonusDir(ZeroVector)
	, flankingBonusMax(0.0f)
	, flankingBonusMin(0.0f)
	, flankingBonusMobilityAdd(0.0f)
	, shieldWeaponDef(nullptr)
	, stockpileWeaponDef(nullptr)
	, maxWeaponRange(0.0f)
	, maxCoverage(0.0f)
	, deathExpWeaponDef(nullptr)
	, selfdExpWeaponDef(nullptr)
	, buildPic(nullptr)
	, selfDCountdown(0)
	, builder(false)
	, activateWhenBuilt(false)
	, onoffable(false)
	, fullHealthFactory(false)
	, factoryHeadingTakeoff(false)
	, capturable(false)
	, repairable(false)

	, canmove(false)
	, canAttack(false)
	, canFight(false)
	, canPatrol(false)
	, canGuard(false)
	, canRepeat(false)
	, canResurrect(false)
	, canCapture(false)
	, canCloak(false)
	, canSelfD(true)
	, canKamikaze(false)

	, canRestore(false)
	, canRepair(false)
	, canReclaim(false)
	, canAssist(false)

	, canBeAssisted(false)
	, canSelfRepair(false)

	, canFireControl(false)
	, canManualFire(false)

	, fireState(FIRESTATE_HOLDFIRE)
	, moveState(MOVESTATE_HOLDPOS)
	, wingDrag(0.0f)
	, wingAngle(0.0f)
	, frontToSpeed(0.0f)
	, speedToFront(0.0f)
	, myGravity(0.0f)
	, maxBank(0.0f)
	, maxPitch(0.0f)
	, turnRadius(0.0f)
	, wantedHeight(0.0f)
	, verticalSpeed(0.0f)
	, useSmoothMesh(false)
	, hoverAttack(false)
	, airStrafe(false)
	, dlHoverFactor(0.0f)
	, bankingAllowed(false)
	, maxAcc(0.0f)
	, maxDec(0.0f)
	, maxAileron(0.0f)
	, maxElevator(0.0f)
	, maxRudder(0.0f)
	, crashDrag(0.0f)
	, loadingRadius(0.0f)
	, unloadSpread(0.0f)
	, transportCapacity(0)
	, transportSize(0)
	, minTransportSize(0)
	, isFirePlatform(false)
	, transportMass(0.0f)
	, minTransportMass(0.0f)
	, holdSteady(false)
	, releaseHeld(false)
	, cantBeTransported(false)
	, transportByEnemy(false)
	, transportUnloadMethod(0)
	, fallSpeed(0.0f)
	, unitFallSpeed(0.0f)
	, startCloaked(false)
	, cloakCost(0.0f)
	, cloakCostMoving(0.0f)
	, decloakDistance(0.0f)
	, decloakSpherical(false)
	, decloakOnFire(false)

	, kamikazeDist(0.0f)
	, kamikazeUseLOS(false)
	, targfac(false)
	, needGeo(false)
	, isFeature(false)
	, hideDamage(false)
	, showPlayerName(false)
	, highTrajectoryType(0)
	, noChaseCategory(0)
	, canDropFlare(false)
	, flareReloadTime(0.0f)
	, flareEfficiency(0.0f)
	, flareDelay(0.0f)
	, flareDropVector(ZeroVector)
	, flareTime(0)
	, flareSalvoSize(0)
	, flareSalvoDelay(0)
	, canLoopbackAttack(false)
	, levelGround(false)
	, showNanoFrame(false)
	, showNanoSpray(false)
	, nanoColor(ZeroVector)
	, maxThisUnit(0)
	, realMetalCost(0.0f)
	, realEnergyCost(0.0f)
	, realMetalUpkeep(0.0f)
	, realEnergyUpkeep(0.0f)
	, realBuildTime(0.0f)
{
	memset(&modelCEGTags[0], 0, sizeof(modelCEGTags));
	memset(&pieceCEGTags[0], 0, sizeof(pieceCEGTags));
	memset(&crashCEGTags[0], 0, sizeof(crashCEGTags));

	// filled in later by UnitDrawer
	modelExplGenIDs.fill(-1u); modelExplGenIDs[0] = 0;
	pieceExplGenIDs.fill(-1u); pieceExplGenIDs[0] = 0;
	crashExplGenIDs.fill(-1u); crashExplGenIDs[0] = 0;
}


UnitDef::UnitDef(const LuaTable& udTable, const std::string& unitName, int id)
{
	// rely on default-ctor to initialize all members
	*this = UnitDef();
	this->id = id;

	name = unitName;
	humanName = udTable.GetString("name", "");
	tooltip = udTable.GetString("description", name);
	wreckName = udTable.GetString("corpse", "");
	buildPicName = udTable.GetString("buildPic", "");
	decoyName = udTable.GetString("decoyFor", "");

	metalStorage  = udTable.GetFloat("metalStorage",  0.0f);
	energyStorage = udTable.GetFloat("energyStorage", 0.0f);
	harvestMetalStorage  = udTable.GetFloat("harvestMetalStorage", udTable.GetFloat("harvestStorage", 0.0f));
	harvestEnergyStorage = udTable.GetFloat("harvestEnergyStorage", 0.0f);

	extractsMetal  = udTable.GetFloat("extractsMetal",  0.0f);
	windGenerator  = udTable.GetFloat("windGenerator",  0.0f);
	tidalGenerator = udTable.GetFloat("tidalGenerator", 0.0f);

	metalUpkeep  = udTable.GetFloat("metalUse",   0.0f);
	energyUpkeep = udTable.GetFloat("energyUse",  0.0f);
	metalMake    = udTable.GetFloat("metalMake",  0.0f);
	makesMetal   = udTable.GetFloat("makesMetal", 0.0f);
	energyMake   = udTable.GetFloat("energyMake", 0.0f);

	health       = std::max(0.1f, udTable.GetFloat("maxDamage",  0.0f)); //avoid some nasty divide by 0
	autoHeal     = udTable.GetFloat("autoHeal",      0.0f) * (UNIT_SLOWUPDATE_RATE / float(GAME_SPEED));
	idleAutoHeal = udTable.GetFloat("idleAutoHeal", 10.0f) * (UNIT_SLOWUPDATE_RATE / float(GAME_SPEED));
	idleTime     = udTable.GetInt("idleTime", 600);

	// iff a mass value is not defined, default to metalCost
	// (do not allow it to be zero or negative in either case)
	metal = std::max(1.0f, udTable.GetFloat("buildCostMetal", 0.0f));
	mass = Clamp(udTable.GetFloat("mass", metal), CSolidObject::MINIMUM_MASS, CSolidObject::MAXIMUM_MASS);
	crushResistance = udTable.GetFloat("crushResistance", mass);

	energy = udTable.GetFloat("buildCostEnergy", 0.0f);
	buildTime = std::max(0.1f, udTable.GetFloat("buildTime", 0.0f)); //avoid some nasty divide by 0

	cobID = udTable.GetInt("cobID", -1);

	buildRange3D = udTable.GetBool("buildRange3D", false);
	// 128.0f is the ancient default
	buildDistance = udTable.GetFloat("buildDistance", 128.0f);
	// 38.0f was evaluated by bobthedinosaur and FLOZi to be the bare minimum
	// to not overlap for a 1x1 constructor building a 1x1 structure
	buildDistance = std::max(38.0f, buildDistance);
	buildSpeed = udTable.GetFloat("workerTime", 0.0f);
	builder = udTable.GetBool("builder", false);
	builder &= IsBuilderUnit();

	repairSpeed    = udTable.GetFloat("repairSpeed",    buildSpeed);
	maxRepairSpeed = udTable.GetFloat("maxRepairSpeed",      1e20f);
	reclaimSpeed   = udTable.GetFloat("reclaimSpeed",   buildSpeed);
	resurrectSpeed = udTable.GetFloat("resurrectSpeed", buildSpeed);
	captureSpeed   = udTable.GetFloat("captureSpeed",   buildSpeed);
	terraformSpeed = udTable.GetFloat("terraformSpeed", buildSpeed);

	reclaimable  = udTable.GetBool("reclaimable",  true);
	capturable   = udTable.GetBool("capturable",   true);
	repairable   = udTable.GetBool("repairable",   true);

	canmove      = udTable.GetBool("canMove",         false);
	canAttack    = udTable.GetBool("canAttack",       true);
	canFight     = udTable.GetBool("canFight",        true);
	canPatrol    = udTable.GetBool("canPatrol",       true);
	canGuard     = udTable.GetBool("canGuard",        true);
	canRepeat    = udTable.GetBool("canRepeat",       true);
	canCloak     = udTable.GetBool("canCloak",        (udTable.GetFloat("cloakCost", 0.0f) != 0.0f));
	canSelfD     = udTable.GetBool("canSelfDestruct", true);
	canKamikaze  = udTable.GetBool("kamikaze",        false);

	// capture and resurrect count as special abilities
	// (because captureSpeed and resurrectSpeed default
	// to buildSpeed, canCapture and canResurrect would
	// otherwise become true for all regular builders)
	canRestore   = udTable.GetBool("canRestore",   builder) && (terraformSpeed > 0.0f);
	canRepair    = udTable.GetBool("canRepair",    builder) && (   repairSpeed > 0.0f);
	canReclaim   = udTable.GetBool("canReclaim",   builder) && (  reclaimSpeed > 0.0f);
	canCapture   = udTable.GetBool("canCapture",     false) && (  captureSpeed > 0.0f);
	canResurrect = udTable.GetBool("canResurrect",   false) && (resurrectSpeed > 0.0f);
	canAssist    = udTable.GetBool("canAssist",    builder);

	canBeAssisted = udTable.GetBool("canBeAssisted", true);
	canSelfRepair = udTable.GetBool("canSelfRepair", false);

	canFireControl = !udTable.GetBool("noAutoFire", false);
	canManualFire = udTable.GetBool("canManualFire", udTable.GetBool("canDGun", false));

	fullHealthFactory = udTable.GetBool("fullHealthFactory", false);
	factoryHeadingTakeoff = udTable.GetBool("factoryHeadingTakeoff", true);

	upright = udTable.GetBool("upright", false);
	collidable = udTable.GetBool("blocking", true);
	collide = udTable.GetBool("collide", true);

	const float maxSlopeDeg = Clamp(udTable.GetFloat("maxSlope", 0.0f), 0.0f, 89.0f);
	const float maxSlopeRad = maxSlopeDeg * math::DEG_TO_RAD;

	// FIXME: kill the magic constant
	maxHeightDif = 40.0f * math::tanf(maxSlopeRad);

	minWaterDepth = udTable.GetFloat("minWaterDepth", -10e6f);
	maxWaterDepth = udTable.GetFloat("maxWaterDepth", +10e6f);
	waterline = udTable.GetFloat("waterline", 0.0f);
	minCollisionSpeed = udTable.GetFloat("minCollisionSpeed", 1.0f);
	slideTolerance = udTable.GetFloat("slideTolerance", 0.0f); // disabled
	pushResistant = udTable.GetBool("pushResistant", false);
	selfDCountdown = udTable.GetInt("selfDestructCountdown", 5);

	speed  = udTable.GetFloat("maxVelocity", 0.0f) * GAME_SPEED;
	speed  = math::fabs(speed);
	rSpeed = udTable.GetFloat("maxReverseVelocity", 0.0f) * GAME_SPEED;
	rSpeed = math::fabs(rSpeed);

	fireState = udTable.GetInt("fireState", canFireControl? FIRESTATE_NONE: FIRESTATE_FIREATWILL);
	fireState = std::min(fireState, int(FIRESTATE_FIREATNEUTRAL));
	moveState = udTable.GetInt("moveState", (canmove && speed > 0.0f)? MOVESTATE_NONE: MOVESTATE_MANEUVER);
	moveState = std::min(moveState, int(MOVESTATE_ROAM));

	flankingBonusMode = udTable.GetInt("flankingBonusMode", modInfo.flankingBonusModeDefault);
	flankingBonusMax  = udTable.GetFloat("flankingBonusMax", 1.9f);
	flankingBonusMin  = udTable.GetFloat("flankingBonusMin", 0.9f);
	flankingBonusDir  = udTable.GetFloat3("flankingBonusDir", FwdVector);
	flankingBonusMobilityAdd = udTable.GetFloat("flankingBonusMobilityAdd", 0.01f);

	armoredMultiple = udTable.GetFloat("damageModifier", 1.0f);
	armorType = damageArrayHandler.GetTypeFromName(name);

	losHeight = udTable.GetFloat("losEmitHeight", 20.0f);
	radarHeight = udTable.GetFloat("radarEmitHeight", losHeight);

	losRadius = udTable.GetFloat("sightDistance", 0.0f);
	airLosRadius = udTable.GetFloat("airSightDistance", 1.5f * losRadius);
	radarRadius    = udTable.GetInt("radarDistance",    0);
	sonarRadius    = udTable.GetInt("sonarDistance",    0);
	jammerRadius   = udTable.GetInt("radarDistanceJam", 0);
	sonarJamRadius = udTable.GetInt("sonarDistanceJam", 0);

	seismicRadius    = udTable.GetInt("seismicDistance", 0);
	seismicSignature = udTable.GetFloat("seismicSignature", -1.0f);

	stealth        = udTable.GetBool("stealth",            false);
	sonarStealth   = udTable.GetBool("sonarStealth",       false);
	targfac        = udTable.GetBool("isTargetingUpgrade", false);
	isFeature      = udTable.GetBool("isFeature",          false);
	hideDamage     = udTable.GetBool("hideDamage",         false);
	showPlayerName = udTable.GetBool("showPlayerName",     false);

	cloakCost = udTable.GetFloat("cloakCost", 0.0f);
	cloakCostMoving = udTable.GetFloat("cloakCostMoving", cloakCost);

	startCloaked     = udTable.GetBool("initCloaked", false);
	decloakDistance  = udTable.GetFloat("minCloakDistance", 0.0f);
	decloakSpherical = udTable.GetBool("decloakSpherical", true);
	decloakOnFire    = udTable.GetBool("decloakOnFire",    true);

	highTrajectoryType = udTable.GetInt("highTrajectory", 0);

	// we count 3d distance while ta count 2d distance so increase slightly
	kamikazeDist = udTable.GetFloat("kamikazeDistance", -25.0f) + 25.0f;
	kamikazeUseLOS = udTable.GetBool("kamikazeUseLOS", false);

	showNanoFrame = udTable.GetBool("showNanoFrame", true);
	showNanoSpray = udTable.GetBool("showNanoSpray", true);
	nanoColor = udTable.GetFloat3("nanoColor", float3(0.2f,0.7f,0.2f));

	canfly      = udTable.GetBool("canFly",      false);
	canSubmerge = udTable.GetBool("canSubmerge", false) && canfly;

	airStrafe      = udTable.GetBool("airStrafe", true);
	hoverAttack    = udTable.GetBool("hoverAttack", false);
	wantedHeight   = udTable.GetFloat("cruiseAlt", 0.0f);
	dlHoverFactor  = udTable.GetFloat("airHoverFactor", -1.0f);
	bankingAllowed = udTable.GetBool("bankingAllowed", true);
	useSmoothMesh  = udTable.GetBool("useSmoothMesh", true);


	maxAcc = math::fabs(udTable.GetFloat("acceleration", 0.5f)); // no negative values
	maxDec = math::fabs(udTable.GetFloat("brakeRate", maxAcc)); // no negative values

	turnRate    = udTable.GetFloat("turnRate", 0.0f);
	turnInPlace = udTable.GetBool("turnInPlace", true);
	turnInPlaceSpeedLimit = turnRate / SPRING_CIRCLE_DIVS;
	turnInPlaceSpeedLimit *= (math::TWOPI * SQUARE_SIZE);
	turnInPlaceSpeedLimit /= std::max(speed / GAME_SPEED, 1.0f);
	turnInPlaceSpeedLimit = udTable.GetFloat("turnInPlaceSpeedLimit", std::min(speed, turnInPlaceSpeedLimit));
	turnInPlaceAngleLimit = udTable.GetFloat("turnInPlaceAngleLimit", 0.0f);


	transportSize     = udTable.GetInt("transportSize",      0);
	minTransportSize  = udTable.GetInt("minTransportSize",   0);
	transportCapacity = udTable.GetInt("transportCapacity",  0);
	isFirePlatform    = udTable.GetBool("isFirePlatform",    false);
	loadingRadius     = udTable.GetFloat("loadingRadius",    220.0f);
	unloadSpread      = udTable.GetFloat("unloadSpread",     5.0f);
	transportMass     = udTable.GetFloat("transportMass",    100000.0f);
	minTransportMass  = udTable.GetFloat("minTransportMass", 0.0f);
	holdSteady        = udTable.GetBool("holdSteady",        false);
	releaseHeld       = udTable.GetBool("releaseHeld",       false);
	cantBeTransported = udTable.GetBool("cantBeTransported", !RequireMoveDef());
	transportByEnemy  = udTable.GetBool("transportByEnemy",  true);
	fallSpeed         = udTable.GetFloat("fallSpeed",    0.2f);
	unitFallSpeed     = udTable.GetFloat("unitFallSpeed",  0);
	transportUnloadMethod = udTable.GetInt("transportUnloadMethod" , 0);

	wingDrag     = udTable.GetFloat("wingDrag",     0.07f);  // drag caused by wings
	wingDrag     = Clamp(wingDrag, 0.0f, 1.0f);
	wingAngle    = udTable.GetFloat("wingAngle",    0.08f);  // angle between front and the wing plane
	frontToSpeed = udTable.GetFloat("frontToSpeed", 0.1f);   // fudge factor for lining up speed and front of plane
	speedToFront = udTable.GetFloat("speedToFront", 0.07f);  // fudge factor for lining up speed and front of plane
	myGravity    = udTable.GetFloat("myGravity",    0.4f);   // planes are slower than real airplanes so lower gravity to compensate
	crashDrag    = udTable.GetFloat("crashDrag",    0.005f); // drag used when crashing
	crashDrag    = Clamp(crashDrag, 0.0f, 1.0f);

	maxBank = udTable.GetFloat("maxBank", 0.8f);         // max roll
	maxPitch = udTable.GetFloat("maxPitch", 0.45f);      // max pitch this plane tries to keep
	turnRadius = udTable.GetFloat("turnRadius", 500.0f); // hint to CStrafeAirMoveType about required turn-radius
	verticalSpeed = udTable.GetFloat("verticalSpeed", 3.0f); // speed of takeoff and landing, at least for gunships

	maxAileron  = udTable.GetFloat("maxAileron",  0.015f); // turn speed around roll axis
	maxElevator = udTable.GetFloat("maxElevator", 0.01f);  // turn speed around pitch axis
	maxRudder   = udTable.GetFloat("maxRudder",   0.004f); // turn speed around yaw axis

	maxThisUnit = udTable.GetInt("unitRestricted", MAX_UNITS);
	maxThisUnit = std::min(maxThisUnit, gameSetup->GetRestrictedUnitLimit(name, MAX_UNITS));

	categoryString = udTable.GetString("category", "");

	category = CCategoryHandler::Instance()->GetCategories(udTable.GetString("category", ""));
	noChaseCategory = CCategoryHandler::Instance()->GetCategories(udTable.GetString("noChaseCategory", ""));

	iconType = icon::iconHandler.GetIcon(udTable.GetString("iconType", "default"));

	shieldWeaponDef    = nullptr;
	stockpileWeaponDef = nullptr;

	maxWeaponRange = 0.0f;
	maxCoverage = 0.0f;

	LuaTable weaponsTable = udTable.SubTable("weapons");
	ParseWeaponsTable(weaponsTable);

	needGeo = false;
	extractRange = mapInfo->map.extractorRadius * int(extractsMetal > 0.0f);

	{
		MoveDef* moveDef = nullptr;

		// aircraft have MoveTypes but no MoveDef;
		// static structures have no use for either
		// (but get StaticMoveType instances)
		if (RequireMoveDef()) {
			const std::string& moveClass = StringToLower(udTable.GetString("movementClass", ""));
			const std::string errMsg = "WARNING: Couldn't find a MoveClass named " + moveClass + " (used in UnitDef: " + unitName + ")";

			// invalidate this unitDef; caught in ParseUnitDef
			if ((moveDef = moveDefHandler.GetMoveDefByName(moveClass)) == nullptr)
				throw content_error(errMsg);

			this->pathType = moveDef->pathType;
		}

		if (moveDef == nullptr) {
			upright           |= !canfly;
			floatOnWater      |= udTable.GetBool("floater", udTable.KeyExists("WaterLine"));

			// we have no MoveDef, so pathType == -1 and IsAirUnit() MIGHT be true
			cantBeTransported |= (!modInfo.transportAir && canfly);
		} else {
			upright           |= (moveDef->speedModClass == MoveDef::Hover);
			upright           |= (moveDef->speedModClass == MoveDef::Ship );

			// we have a MoveDef, so pathType != -1 and IsGroundUnit() MUST be true
			cantBeTransported |= (!modInfo.transportGround && moveDef->speedModClass == MoveDef::Tank );
			cantBeTransported |= (!modInfo.transportGround && moveDef->speedModClass == MoveDef::KBot );
			cantBeTransported |= (!modInfo.transportShip   && moveDef->speedModClass == MoveDef::Ship );
			cantBeTransported |= (!modInfo.transportHover  && moveDef->speedModClass == MoveDef::Hover);
		}

		if (seismicSignature == -1.0f) {
			const bool isTank = (moveDef != nullptr && moveDef->speedModClass == MoveDef::Tank);
			const bool isKBot = (moveDef != nullptr && moveDef->speedModClass == MoveDef::KBot);

			// seismic signatures only make sense for certain mobile ground units
			if (isTank || isKBot) {
				seismicSignature = math::sqrt(mass / 100.0f);
			} else {
				seismicSignature = 0.0f;
			}
		}
	}

	if (IsAirUnit()) {
		if (IsFighterAirUnit() || IsBomberAirUnit()) {
			// double turn-radius for bombers if not set explicitly
			turnRadius *= (1.0f + (IsBomberAirUnit() && turnRadius == 500.0f));
			maxAcc = udTable.GetFloat("maxAcc", 0.065f); // engine power
		}
	}


	modelName = udTable.GetString("objectName", "");
	scriptName = "scripts/" + udTable.GetString("script", unitName + ".cob");

	deathExpWeaponDef = weaponDefHandler->GetWeaponDef(udTable.GetString("explodeAs", ""));
	selfdExpWeaponDef = weaponDefHandler->GetWeaponDef(udTable.GetString("selfDestructAs", udTable.GetString("explodeAs", "")));

	if (deathExpWeaponDef == nullptr && (deathExpWeaponDef = weaponDefHandler->GetWeaponDef("NOWEAPON")) == nullptr) {
		LOG_L(L_ERROR, "Couldn't find WeaponDef NOWEAPON and explodeAs for %s is missing!", unitName.c_str());
	}
	if (selfdExpWeaponDef == nullptr && (selfdExpWeaponDef = weaponDefHandler->GetWeaponDef("NOWEAPON")) == nullptr) {
		LOG_L(L_ERROR, "Couldn't find WeaponDef NOWEAPON and selfDestructAs for %s is missing!", unitName.c_str());
	}

	power = udTable.GetFloat("power", (metal + (energy / 60.0f)));

	// Prevent a division by zero in experience calculations.
	if (power < 1.0e-3f) {
		LOG_L(L_WARNING, "Unit %s is really cheap? %f", humanName.c_str(), power);
		LOG_L(L_WARNING, "This can cause a division by zero in experience calculations.");
		power = 1.0e-3f;
	}

	activateWhenBuilt = udTable.GetBool("activateWhenBuilt", false);
	onoffable = udTable.GetBool("onoffable", false);

	xsize = std::max(1 * SPRING_FOOTPRINT_SCALE, (udTable.GetInt("footprintX", 1) * SPRING_FOOTPRINT_SCALE));
	zsize = std::max(1 * SPRING_FOOTPRINT_SCALE, (udTable.GetInt("footprintZ", 1) * SPRING_FOOTPRINT_SCALE));

	buildingMask = (std::uint16_t)udTable.GetInt("buildingMask", 1); //1st bit set to 1 constitutes for "normal building"
	if (IsImmobileUnit())
		CreateYardMap(udTable.GetString("yardMap", ""));

	decalDef.Parse(udTable);

	canDropFlare    = udTable.GetBool("canDropFlare", false);
	flareReloadTime = udTable.GetFloat("flareReload",     5.0f);
	flareDelay      = udTable.GetFloat("flareDelay",      0.3f);
	flareEfficiency = udTable.GetFloat("flareEfficiency", 0.5f);
	flareDropVector = udTable.GetFloat3("flareDropVector", ZeroVector);
	flareTime       = udTable.GetInt("flareTime", 3) * GAME_SPEED;
	flareSalvoSize  = udTable.GetInt("flareSalvoSize",  4);
	flareSalvoDelay = udTable.GetInt("flareSalvoDelay", 0) * GAME_SPEED;

	canLoopbackAttack = udTable.GetBool("canLoopbackAttack", false);
	levelGround = udTable.GetBool("levelGround", true);
	strafeToAttack = udTable.GetBool("strafeToAttack", false);


	// initialize the (per-unitdef) collision-volume
	// all CUnit instances hold a copy of this object
	ParseCollisionVolume(udTable);
	ParseSelectionVolume(udTable);

	{
		const LuaTable& buildsTable = udTable.SubTable("buildOptions");
		const LuaTable& paramsTable = udTable.SubTable("customParams");

		if (buildsTable.IsValid())
			buildsTable.GetMap(buildOptions);

		// custom parameters table
		paramsTable.GetMap(customParams);
	}
	{
		const LuaTable&      sfxTable =  udTable.SubTable("SFXTypes");
		const LuaTable& modelCEGTable = sfxTable.SubTable(     "explosionGenerators");
		const LuaTable& pieceCEGTable = sfxTable.SubTable("pieceExplosionGenerators");
		const LuaTable& crashCEGTable = sfxTable.SubTable("crashExplosionGenerators");

		std::vector<int> cegKeys;
		std::array<const LuaTable*, 3> cegTbls = {&modelCEGTable, &pieceCEGTable, &crashCEGTable};
		std::array<char[64], MAX_UNITDEF_EXPGEN_IDS>* cegTags[3] = {&modelCEGTags, &pieceCEGTags, &crashCEGTags};

		for (int i = 0; i < 3; i++) {
			auto& tagStrs = *cegTags[i];

			cegKeys.clear();
			cegKeys.reserve(tagStrs.size());

			cegTbls[i]->GetKeys(cegKeys);

			// get at most N tags, discard the rest
			for (unsigned int j = 0, k = 0; j < cegKeys.size() && k < tagStrs.size(); j++) {
				const std::string& tag = cegTbls[i]->GetString(cegKeys[j], "");

				if (tag.empty())
					continue;

				strncpy(tagStrs[k++], tag.c_str(), sizeof(tagStrs[0]));
			}
		}
	}
}


void UnitDef::ParseWeaponsTable(const LuaTable& weaponsTable)
{
	const WeaponDef* noWeaponDef = weaponDefHandler->GetWeaponDef("NOWEAPON");

	for (int k = 0, w = 0; w < MAX_WEAPONS_PER_UNIT; w++) {
		LuaTable wTable;
		std::string wdName = weaponsTable.GetString(w + 1, "");

		if (wdName.empty()) {
			wTable = weaponsTable.SubTable(w + 1);
			wdName = wTable.GetString("name", "");
		}

		const WeaponDef* wd = nullptr;

		if (!wdName.empty())
			wd = weaponDefHandler->GetWeaponDef(wdName);

		if (wd == nullptr) {
			// allow any of the first three weapons to be null; these will be
			// replaced by NoWeapon's if there is a valid WeaponDef among this
			// set
			if (w <= 3)
				continue;

			// otherwise stop trying
			break;
		}

		while (k < w) {
			if (noWeaponDef == nullptr) {
				LOG_L(L_ERROR, "[%s] missing NOWEAPON for WeaponDef %s (#%d) of UnitDef %s", __func__, wdName.c_str(), w, humanName.c_str());
				break;
			}

			weapons[k++] = {noWeaponDef};
		}

		weapons[k++] = {wd, wTable};

		maxWeaponRange = std::max(maxWeaponRange, wd->range);

		if (wd->interceptor && wd->coverageRange > maxCoverage)
			maxCoverage = wd->coverageRange;

		if (wd->isShield) {
			// use the biggest shield
			if (shieldWeaponDef == nullptr || (shieldWeaponDef->shieldRadius < wd->shieldRadius))
				shieldWeaponDef = wd;
		}

		if (wd->stockpile) {
			// interceptors have priority
			if (wd->interceptor || stockpileWeaponDef == nullptr || !stockpileWeaponDef->interceptor)
				stockpileWeaponDef = wd;
		}
	}
}



void UnitDef::CreateYardMap(std::string&& yardMapStr)
{
	// if a unit is immobile but does *not* have a yardmap
	// defined, assume it is not supposed to be a building
	// (so do not assign a default per facing)
	if (yardMapStr.empty())
		return;

	const bool highResMap = (tolower(yardMapStr[0]) == 'h');

	// determine number of characters to parse from str
	const unsigned int hxSize = xsize >> (1 - highResMap);
	const unsigned int hzSize = zsize >> (1 - highResMap);
	const unsigned int ymSize = hxSize * hzSize;

	// if high-res yardmap, start reading at second character
	unsigned int ymReadIdx = highResMap;
	unsigned int ymCopyIdx = 0;

	std::array<YardMapStatus, 256 * 256> defYardMap;

	if (ymSize > defYardMap.size()) {
		LOG_L(L_WARNING, "[%s] %s: footprint{x=%u,z=%u} too large to create %s-res yardmap", __func__, name.c_str(), xsize, zsize, highResMap? "high": "low");
		return;
	}

	yardmap.resize(xsize * zsize);
	defYardMap.fill(YARDMAP_BLOCKED);

	// read the yardmap from the LuaDef string
	while (ymReadIdx < yardMapStr.size()) {
		const char c = tolower(yardMapStr[ymReadIdx++]);

		if (isspace(c))
			continue;
		// continue rather than break s.t. the excess-count can be shown
		if ((ymCopyIdx++) >= ymSize)
			continue;

		switch (c) {
			case 'g': { defYardMap[ymCopyIdx - 1] = YARDMAP_GEO; needGeo = true; } break;
			case 'y': { defYardMap[ymCopyIdx - 1] = YARDMAP_OPEN;                } break;
			case 'c': { defYardMap[ymCopyIdx - 1] = YARDMAP_YARD;                } break;
			case 'i': { defYardMap[ymCopyIdx - 1] = YARDMAP_YARDINV;             } break;
		//	case 'w': { defYardMap[ymCopyIdx - 1] = YARDMAP_WALKABLE;            } break; // TODO?
			case 'w':
			case 'x':
			case 'f':
			case 'o': { defYardMap[ymCopyIdx - 1] = YARDMAP_BLOCKED;             } break;
			default : {                                                          } break;
		}
	}

	// print warnings
	if (ymCopyIdx > ymSize)
		LOG_L(L_WARNING, "[%s] %s: given yardmap contains %u excess char(s)!", __func__, name.c_str(), ymCopyIdx - ymSize);

	if (ymCopyIdx > 0 && ymCopyIdx < ymSize)
		LOG_L(L_WARNING, "[%s] %s: given yardmap requires %u extra char(s)!", __func__, name.c_str(), ymSize - ymCopyIdx);

	// write the final yardmap at blocking-map resolution
	// (in case of a high-res map this becomes a 1:1 copy,
	// otherwise the given yardmap will be upsampled 2:1)
	for (unsigned int bmz = 0; bmz < zsize; bmz++) {
		for (unsigned int bmx = 0; bmx < xsize; bmx++) {
			const unsigned int ymx = bmx >> (1 - highResMap);
			const unsigned int ymz = bmz >> (1 - highResMap);

			yardmap[bmx + bmz * xsize] = defYardMap[ymx + ymz * hxSize];
		}
	}
}



void UnitDef::SetNoCost(bool noCost)
{
	if (noCost) {
		// initialized from UnitDefHandler::PushNewUnitDef
		realMetalCost    = metal;
		realEnergyCost   = energy;
		realMetalUpkeep  = metalUpkeep;
		realEnergyUpkeep = energyUpkeep;
		realBuildTime    = buildTime;

		metal        =  1.0f;
		energy       =  1.0f;
		buildTime    = 10.0f;
		metalUpkeep  =  0.0f;
		energyUpkeep =  0.0f;
	} else {
		metal        = realMetalCost;
		energy       = realEnergyCost;
		buildTime    = realBuildTime;
		metalUpkeep  = realMetalUpkeep;
		energyUpkeep = realEnergyUpkeep;
	}
}

bool UnitDef::HasBomberWeapon(unsigned int idx) const {
	// checked by Is*AirUnit
	assert(HasWeapon(idx));
	return (weapons[idx].def->IsAircraftWeapon());
}

