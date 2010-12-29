/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDef.h"
#include "UnitDefHandler.h"
#include "UnitDefImage.h"
#include "Game/GameSetup.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Rendering/IconHandler.h"
#include "Rendering/Models/IModelParser.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"
#include "System/myMath.h"
#include "System/Util.h"


/******************************************************************************/

UnitDefWeapon::UnitDefWeapon()
: name("NOWEAPON")
, def(NULL)
, slavedTo(0)
, mainDir(0, 0, 1)
, maxAngleDif(-1)
, fuelUsage(0)
, badTargetCat(0)
, onlyTargetCat(0)
{
}

UnitDefWeapon::UnitDefWeapon(
	std::string name, const WeaponDef* def, int slavedTo, float3 mainDir, float maxAngleDif,
	unsigned int badTargetCat, unsigned int onlyTargetCat, float fuelUse)
: name(name)
, def(def)
, slavedTo(slavedTo)
, mainDir(mainDir)
, maxAngleDif(maxAngleDif)
, fuelUsage(fuelUse)
, badTargetCat(badTargetCat)
, onlyTargetCat(onlyTargetCat)
{
}


/******************************************************************************/

UnitDef::UnitDef()
: id(-1)
, cobID(-1)
, collisionVolume(NULL)
, decoyDef(NULL)
, techLevel(0)
, metalUpkeep(0.0f)
, energyUpkeep(0.0f)
, metalMake(0.0f)
, makesMetal(0.0f)
, energyMake(0.0f)
, metalCost(0.0f)
, energyCost(0.0f)
, buildTime(0.0f)
, extractsMetal(0.0f)
, extractRange(0.0f)
, windGenerator(0.0f)
, tidalGenerator(0.0f)
, metalStorage(0.0f)
, energyStorage(0.0f)
, extractSquare(false)
, autoHeal(0.0f)
, idleAutoHeal(0.0f)
, idleTime(0)
, power(0.0f)
, health(0.0f)
, category(-1)
, speed(0.0f)
, rSpeed(0.0f)
, turnRate(0.0f)
, turnInPlace(false)
, turnInPlaceDistance(0.0f)
, turnInPlaceSpeedLimit(0.0f)
, upright(false)
, collide(false)
, losRadius(0.0f)
, airLosRadius(0.0f)
, losHeight(0.0f)
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
, mass(0.0f)
, pushResistant(false)
, strafeToAttack(false)
, minCollisionSpeed(0.0f)
, slideTolerance(0.0f)
, maxSlope(0.0f)
, maxHeightDif(0.0f)
, minWaterDepth(0.0f)
, waterline(0.0f)
, maxWaterDepth(0.0f)
, armoredMultiple(0.0f)
, armorType(0)
, flankingBonusMode(0)
, flankingBonusDir(ZeroVector)
, flankingBonusMax(0.0f)
, flankingBonusMin(0.0f)
, flankingBonusMobilityAdd(0.0f)
, modelCenterOffset(ZeroVector)
, usePieceCollisionVolumes(false)
, shieldWeaponDef(NULL)
, stockpileWeaponDef(NULL)
, maxWeaponRange(0.0f)
, maxCoverage(0.0f)
, buildPic(NULL)
, canSelfD(true)
, selfDCountdown(0)
, canSubmerge(false)
, canfly(false)
, canmove(false)
, canhover(false)
, floater(false)
, builder(false)
, activateWhenBuilt(false)
, onoffable(false)
, fullHealthFactory(false)
, factoryHeadingTakeoff(false)
, reclaimable(false)
, capturable(false)
, repairable(false)
, canRestore(false)
, canRepair(false)
, canSelfRepair(false)
, canReclaim(false)
, canAttack(false)
, canPatrol(false)
, canFight(false)
, canGuard(false)
, canAssist(false)
, canBeAssisted(false)
, canRepeat(false)
, canFireControl(false)
, fireState(FIRESTATE_HOLDFIRE)
, moveState(MOVESTATE_HOLDPOS)
, wingDrag(0.0f)
, wingAngle(0.0f)
, drag(0.0f)
, frontToSpeed(0.0f)
, speedToFront(0.0f)
, myGravity(0.0f)
, maxBank(0.0f)
, maxPitch(0.0f)
, turnRadius(0.0f)
, wantedHeight(0.0f)
, verticalSpeed(0.0f)
, useSmoothMesh(false)
, canCrash(false)
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
, movedata(NULL)
, xsize(0)
, zsize(0)
, loadingRadius(0.0f)
, unloadSpread(0.0f)
, transportCapacity(0)
, transportSize(0)
, minTransportSize(0)
, isAirBase(false)
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
, canCloak(false)
, startCloaked(false)
, cloakCost(0.0f)
, cloakCostMoving(0.0f)
, decloakDistance(0.0f)
, decloakSpherical(false)
, decloakOnFire(false)
, cloakTimeout(0)
, canKamikaze(false)
, kamikazeDist(0.0f)
, kamikazeUseLOS(false)
, targfac(false)
, canDGun(false)
, needGeo(false)
, isFeature(false)
, hideDamage(false)
, isCommander(false)
, showPlayerName(false)
, canResurrect(false)
, canCapture(false)
, highTrajectoryType(0)
, noChaseCategory(0)
, leaveTracks(false)
, trackWidth(0.0f)
, trackOffset(0.0f)
, trackStrength(0.0f)
, trackStretch(0.0f)
, trackType(0)
, canDropFlare(false)
, flareReloadTime(0.0f)
, flareEfficiency(0.0f)
, flareDelay(0.0f)
, flareDropVector(ZeroVector)
, flareTime(0)
, flareSalvoSize(0)
, flareSalvoDelay(0)
, smoothAnim(false)
, canLoopbackAttack(false)
, levelGround(false)
, useBuildingGroundDecal(false)
, buildingDecalType(0)
, buildingDecalSizeX(0)
, buildingDecalSizeY(0)
, buildingDecalDecaySpeed(0.0f)
, showNanoFrame(false)
, showNanoSpray(false)
, nanoColor(ZeroVector)
, maxFuel(0.0f)
, refuelTime(0.0f)
, minAirBasePower(0.0f)
, pieceTrailCEGRange(-1)
, maxThisUnit(0)
, transportableBuilding(false)
, realMetalCost(0.0f)
, realEnergyCost(0.0f)
, realMetalUpkeep(0.0f)
, realEnergyUpkeep(0.0f)
, realBuildTime(0.0f)
{
}


UnitDef::UnitDef(const LuaTable& udTable, const std::string& unitName, int id)
: name(unitName)
, id(id)
, collisionVolume(NULL)
, decoyDef(NULL)
, techLevel(-1)
, buildPic(NULL)
, movedata(NULL)
{
	humanName = udTable.GetString("name", "");
	if (humanName.empty()) {
		const string errmsg = "missing 'name' parameter for the " + unitName + " unitdef";
		throw content_error(errmsg);
	}
	filename  = udTable.GetString("filename", "");
	if (filename.empty()) {
		const string errmsg = "missing 'filename' parameter for the" + unitName + " unitdef";
		throw content_error(errmsg);
	}
	tooltip = udTable.GetString("description", name);
	buildPicName = udTable.GetString("buildPic", "");
	decoyName = udTable.GetString("decoyFor", "");

	isCommander = udTable.GetBool("commander", false);

	metalStorage  = udTable.GetFloat("metalStorage",  0.0f);
	energyStorage = udTable.GetFloat("energyStorage", 0.0f);

	extractsMetal  = udTable.GetFloat("extractsMetal",  0.0f);
	windGenerator  = udTable.GetFloat("windGenerator",  0.0f);
	tidalGenerator = udTable.GetFloat("tidalGenerator", 0.0f);

	metalUpkeep  = udTable.GetFloat("metalUse",   0.0f);
	energyUpkeep = udTable.GetFloat("energyUse",  0.0f);
	metalMake    = udTable.GetFloat("metalMake",  0.0f);
	makesMetal   = udTable.GetFloat("makesMetal", 0.0f);
	energyMake   = udTable.GetFloat("energyMake", 0.0f);

	health       = udTable.GetFloat("maxDamage",  0.0f);
	autoHeal     = udTable.GetFloat("autoHeal",      0.0f) * (16.0f / GAME_SPEED);
	idleAutoHeal = udTable.GetFloat("idleAutoHeal", 10.0f) * (16.0f / GAME_SPEED);
	idleTime     = udTable.GetInt("idleTime", 600);

	losHeight = 20;
	metalCost = udTable.GetFloat("buildCostMetal", 0.0f);
	if (metalCost < 1.0f) {
		metalCost = 1.0f; //avoid some nasty divide by 0 etc
	}
	mass = udTable.GetFloat("mass", 0.0f);
	if (mass <= 0.0f) {
		mass = metalCost;
	}
	energyCost = udTable.GetFloat("buildCostEnergy", 0.0f);
	buildTime = udTable.GetFloat("buildTime", 0.0f);
	if (buildTime < 1.0f) {
		buildTime = 1.0f; //avoid some nasty divide by 0 etc
	}

	cobID = udTable.GetInt("cobID", -1);

	losRadius = udTable.GetFloat("sightDistance", 0.0f) * modInfo.losMul / (SQUARE_SIZE * (1 << modInfo.losMipLevel));
	airLosRadius = udTable.GetFloat("airSightDistance", -1.0f);
	if (airLosRadius == -1.0f) {
		airLosRadius = udTable.GetFloat("sightDistance", 0.0f) * modInfo.airLosMul * 1.5f / (SQUARE_SIZE * (1 << modInfo.airMipLevel));
	} else {
		airLosRadius = airLosRadius * modInfo.airLosMul / (SQUARE_SIZE * (1 << modInfo.airMipLevel));
	}

	canSubmerge = udTable.GetBool("canSubmerge", false);
	canfly      = udTable.GetBool("canFly",      false);
	canmove     = udTable.GetBool("canMove",     false);
	reclaimable = udTable.GetBool("reclaimable", true);
	capturable  = udTable.GetBool("capturable",  true);
	repairable  = udTable.GetBool("repairable",  true);
	canAttack   = udTable.GetBool("canAttack",   true);
	canFight    = udTable.GetBool("canFight",    true);
	canPatrol   = udTable.GetBool("canPatrol",   true);
	canGuard    = udTable.GetBool("canGuard",    true);
	canRepeat   = udTable.GetBool("canRepeat",   true);

	builder = udTable.GetBool("builder", false);

	canRestore = udTable.GetBool("canRestore", builder);
	canRepair  = udTable.GetBool("canRepair",  builder);
	canReclaim = udTable.GetBool("canReclaim", builder);
	canAssist  = udTable.GetBool("canAssist",  builder);

	canBeAssisted = udTable.GetBool("canBeAssisted", true);
	canSelfRepair = udTable.GetBool("canSelfRepair", false);
	fullHealthFactory = udTable.GetBool("fullHealthFactory", false);
	factoryHeadingTakeoff = udTable.GetBool("factoryHeadingTakeoff", true);

	upright = udTable.GetBool("upright", false);
	collide = udTable.GetBool("collide", true);
	onoffable = udTable.GetBool("onoffable", false);

	maxSlope = Clamp(udTable.GetFloat("maxSlope", 0.0f), 0.0f, 89.0f);
	maxHeightDif = 40 * tan(maxSlope * (PI / 180));
	maxSlope = cos(maxSlope * (PI / 180));
	minWaterDepth = udTable.GetFloat("minWaterDepth", -10e6f);
	maxWaterDepth = udTable.GetFloat("maxWaterDepth", +10e6f);
	waterline = udTable.GetFloat("waterline", 0.0f);
	minCollisionSpeed = udTable.GetFloat("minCollisionSpeed", 1.0f);
	slideTolerance = udTable.GetFloat("slideTolerance", 0.0f); // disabled
	pushResistant = udTable.GetBool("pushResistant", false);

	canSelfD = udTable.GetBool("canSelfDestruct", true);
	selfDCountdown = udTable.GetInt("selfDestructCountdown", 5);

	speed  = udTable.GetFloat("maxVelocity", 0.0f) * GAME_SPEED;
	rSpeed = udTable.GetFloat("maxReverseVelocity", 0.0f) * GAME_SPEED;
	speed  = fabs(speed);
	rSpeed = fabs(rSpeed);

	maxAcc = fabs(udTable.GetFloat("acceleration", 0.5f)); // no negative values
	maxDec = fabs(udTable.GetFloat("brakeRate", 3.0f * maxAcc)) * (canfly? 0.1f: 1.0f); // no negative values

	turnRate    = udTable.GetFloat("turnRate", 0.0f);
	turnInPlace = udTable.GetBool("turnInPlace", true);
	turnInPlaceDistance = udTable.GetFloat("turnInPlaceDistance", 350.f);
	turnInPlaceSpeedLimit = ((turnRate / SPRING_CIRCLE_DIVS) * ((PI + PI) * SQUARE_SIZE)) * (speed / GAME_SPEED);
	turnInPlaceSpeedLimit = udTable.GetFloat("turnInPlaceSpeedLimit", std::min(speed, turnInPlaceSpeedLimit));

	canFireControl = !udTable.GetBool("noAutoFire", false);
	fireState = udTable.GetInt("fireState", canFireControl? FIRESTATE_NONE: FIRESTATE_FIREATWILL);
	fireState = std::min(fireState, int(FIRESTATE_FIREATWILL));
	moveState = udTable.GetInt("moveState", (canmove && speed > 0.0f)? MOVESTATE_NONE: MOVESTATE_MANEUVER);
	moveState = std::min(moveState, int(MOVESTATE_ROAM));

	buildRange3D = udTable.GetBool("buildRange3D", false);
	// 128.0f is the ancient default
	buildDistance = udTable.GetFloat("buildDistance", 128.0f);
	// 38.0f was evaluated by bobthedinosaur and FLOZi to be the bare minimum
	// to not overlap for a 1x1 constructor building a 1x1 structure
	buildDistance = std::max(38.0f, buildDistance);
	buildSpeed = udTable.GetFloat("workerTime", 0.0f);

	repairSpeed    = udTable.GetFloat("repairSpeed",    buildSpeed);
	maxRepairSpeed = udTable.GetFloat("maxRepairSpeed", 1e20f);
	reclaimSpeed   = udTable.GetFloat("reclaimSpeed",   buildSpeed);
	resurrectSpeed = udTable.GetFloat("resurrectSpeed", buildSpeed);
	captureSpeed   = udTable.GetFloat("captureSpeed",   buildSpeed);
	terraformSpeed = udTable.GetFloat("terraformSpeed", buildSpeed);

	flankingBonusMode = udTable.GetInt("flankingBonusMode", modInfo.flankingBonusModeDefault);
	flankingBonusMax  = udTable.GetFloat("flankingBonusMax", 1.9f);
	flankingBonusMin  = udTable.GetFloat("flankingBonusMin", 0.9);
	flankingBonusDir  = udTable.GetFloat3("flankingBonusDir", float3(0.0f, 0.0f, 1.0f));
	flankingBonusMobilityAdd = udTable.GetFloat("flankingBonusMobilityAdd", 0.01f);

	armoredMultiple = udTable.GetFloat("damageModifier", 1.0f);
	armorType = damageArrayHandler->GetTypeFromName(name);

	radarRadius    = udTable.GetInt("radarDistance",    0);
	sonarRadius    = udTable.GetInt("sonarDistance",    0);
	jammerRadius   = udTable.GetInt("radarDistanceJam", 0);
	sonarJamRadius = udTable.GetInt("sonarDistanceJam", 0);

	stealth        = udTable.GetBool("stealth",            false);
	sonarStealth   = udTable.GetBool("sonarStealth",       false);
	targfac        = udTable.GetBool("isTargetingUpgrade", false);
	isFeature      = udTable.GetBool("isFeature",          false);
	canResurrect   = udTable.GetBool("canResurrect",       false);
	canCapture     = udTable.GetBool("canCapture",         false);
	hideDamage     = udTable.GetBool("hideDamage",         false);
	showPlayerName = udTable.GetBool("showPlayerName",     false);

	cloakCost = udTable.GetFloat("cloakCost", -1.0f);
	cloakCostMoving = udTable.GetFloat("cloakCostMoving", -1.0f);
	if (cloakCostMoving < 0) {
		cloakCostMoving = cloakCost;
	}
	canCloak = (cloakCost >= 0);

	startCloaked     = udTable.GetBool("initCloaked", false);
	decloakDistance  = udTable.GetFloat("minCloakDistance", 0.0f);
	decloakSpherical = udTable.GetBool("decloakSpherical", true);
	decloakOnFire    = udTable.GetBool("decloakOnFire",    true);
	cloakTimeout     = udTable.GetInt("cloakTimeout", 128);

	highTrajectoryType = udTable.GetInt("highTrajectory", 0);

	canKamikaze = udTable.GetBool("kamikaze", false);
	kamikazeDist = udTable.GetFloat("kamikazeDistance", -25.0f) + 25.0f; //we count 3d distance while ta count 2d distance so increase slightly
	kamikazeUseLOS = udTable.GetBool("kamikazeUseLOS", false);

	showNanoFrame = udTable.GetBool("showNanoFrame", true);
	showNanoSpray = udTable.GetBool("showNanoSpray", true);
	nanoColor = udTable.GetFloat3("nanoColor", float3(0.2f,0.7f,0.2f));

	canhover = udTable.GetBool("canHover", false);
	floater = udTable.GetBool("floater", udTable.KeyExists("WaterLine"));

	if (builder && !buildSpeed) { // core anti is flagged as builder for some reason
		builder = false;
	}

	airStrafe      = udTable.GetBool("airStrafe", true);
	hoverAttack    = udTable.GetBool("hoverAttack", false);
	wantedHeight   = udTable.GetFloat("cruiseAlt", 0.0f);
	dlHoverFactor  = udTable.GetFloat("airHoverFactor", -1.0f);
	bankingAllowed = udTable.GetBool("bankingAllowed", true);
	useSmoothMesh  = udTable.GetBool("useSmoothMesh", true);

	transportSize     = udTable.GetInt("transportSize",      0);
	minTransportSize  = udTable.GetInt("minTransportSize",   0);
	transportCapacity = udTable.GetInt("transportCapacity",  0);
	isFirePlatform    = udTable.GetBool("isFirePlatform",    false);
	isAirBase         = udTable.GetBool("isAirBase",         false);
	loadingRadius     = udTable.GetFloat("loadingRadius",    220.0f);
	unloadSpread      = udTable.GetFloat("unloadSpread",     1.0f);
	transportMass     = udTable.GetFloat("transportMass",    100000.0f);
	minTransportMass  = udTable.GetFloat("minTransportMass", 0.0f);
	holdSteady        = udTable.GetBool("holdSteady",        false);
	releaseHeld       = udTable.GetBool("releaseHeld",       false);
	cantBeTransported = udTable.GetBool("cantBeTransported", false);
	transportByEnemy  = udTable.GetBool("transportByEnemy",  true);
	fallSpeed         = udTable.GetFloat("fallSpeed",    0.2);
	unitFallSpeed     = udTable.GetFloat("unitFallSpeed",  0);
	transportUnloadMethod	= udTable.GetInt("transportUnloadMethod" , 0);

	// modrules transport settings
	if ((!modInfo.transportAir    && canfly)   ||
	    (!modInfo.transportShip   && floater)  ||
	    (!modInfo.transportHover  && canhover) ||
	    (!modInfo.transportGround && !canhover && !floater && !canfly)) {
 		cantBeTransported = true;
	}

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
	turnRadius = udTable.GetFloat("turnRadius", 500.0f); // hint to CAirMoveType about required turn-radius
	verticalSpeed = udTable.GetFloat("verticalSpeed", 3.0f); // speed of takeoff and landing, at least for gunships

	maxAileron  = udTable.GetFloat("maxAileron",  0.015f); // turn speed around roll axis
	maxElevator = udTable.GetFloat("maxElevator", 0.01f);  // turn speed around pitch axis
	maxRudder   = udTable.GetFloat("maxRudder",   0.004f); // turn speed around yaw axis

	maxFuel = udTable.GetFloat("maxFuel", 0.0f); //max flight time in seconds before aircraft must return to base
	refuelTime = udTable.GetFloat("refuelTime", 5.0f);
	minAirBasePower = udTable.GetFloat("minAirBasePower", 0.0f);
	maxThisUnit = udTable.GetInt("unitRestricted", MAX_UNITS);
	transportableBuilding = udTable.GetBool("transportableBuilding", false);

	const string lname = StringToLower(name);

	if (gameSetup->restrictedUnits.find(lname) != gameSetup->restrictedUnits.end()) {
		maxThisUnit = std::min(maxThisUnit, gameSetup->restrictedUnits.find(lname)->second);
	}

	categoryString = udTable.GetString("category", "");

	category = CCategoryHandler::Instance()->GetCategories(udTable.GetString("category", ""));
	noChaseCategory = CCategoryHandler::Instance()->GetCategories(udTable.GetString("noChaseCategory", ""));

	const string iconName = udTable.GetString("iconType", "default");
	iconType = iconHandler->GetIcon(iconName);

	shieldWeaponDef    = NULL;
	stockpileWeaponDef = NULL;

	maxWeaponRange = 0.0f;
	maxCoverage = 0.0f;

	LuaTable weaponsTable = udTable.SubTable("weapons");
	ParseWeaponsTable(weaponsTable);

	canDGun = udTable.GetBool("canDGun", false);
	needGeo = false;

	extractRange = mapInfo->map.extractorRadius * int(extractsMetal > 0.0f);
	extractSquare = udTable.GetBool("extractSquare", false);

	// aircraft have MoveTypes but no MoveData;
	// static structures have no use for either
	// (but get StaticMoveType instances)
	if (WantsMoveType() && !canfly) {
		const std::string& moveClass = StringToLower(udTable.GetString("movementClass", ""));
		const std::string errMsg = "WARNING: Couldn't find a MoveClass named " + moveClass + " (used in UnitDef: " + unitName + ")";

		if ((movedata = moveinfo->GetMoveDataFromName(moveClass)) == NULL) {
			throw content_error(errMsg); //! invalidate unitDef (this gets catched in ParseUnitDef!)
		}

		if (canhover) {
			if (movedata->moveType != MoveData::Hover_Move) {
				logOutput.Print("Inconsistent movedata %i for %s (moveclass %s): canhover, but not a hovercraft movetype",
				     movedata->pathType, name.c_str(), moveClass.c_str());
			}
		} else if (floater) {
			if (movedata->moveType != MoveData::Ship_Move) {
				logOutput.Print("Inconsistent movedata %i for %s (moveclass %s): floater, but not a ship movetype",
				     movedata->pathType, name.c_str(), moveClass.c_str());
			}
		} else {
			if (movedata->moveType != MoveData::Ground_Move) {
				logOutput.Print("Inconsistent movedata %i for %s (moveclass %s): neither canhover nor floater, but not a ground movetype",
				     movedata->pathType, name.c_str(), moveClass.c_str());
			}
		}
	}

	if (movedata == NULL) {
		upright = upright || !canfly;
	} else {
		upright = upright ||
			(movedata->moveType == MoveData::Hover_Move) ||
			(movedata->moveType == MoveData::Ship_Move);
	}

	if (IsAirUnit()) {
		if (IsFighterUnit() || IsBomberUnit()) {
			// double turn-radius for bombers if not set explicitly
			if (IsBomberUnit() && turnRadius == 500.0f) {
				turnRadius *= 2.0f;
			}

			maxAcc = udTable.GetFloat("maxAcc", 0.065f); // engine power
		}

		if ((maxAcc != 0) && (speed != 0)) {
			//meant to set the drag such that the maxspeed becomes what it should be
			drag = (1.0f / (speed / GAME_SPEED * 1.1f / maxAcc)) - (wingAngle * wingAngle * wingDrag);
			drag = Clamp(drag, 0.0f, 1.0f);
		} else {
			//shouldn't be needed since drag is only used in CAirMoveType anyway,
			//and aircraft without acceleration or speed aren't common :)
			//initializing it anyway just for safety
			drag = 0.005f;
		}
	}


	objectName = udTable.GetString("objectName", "");
	if (objectName.find(".") == std::string::npos) {
		objectName += ".3do"; // NOTE: get rid of this?
	}
	modelDef.modelPath = "objects3d/" + objectName;
	modelDef.modelName = objectName;

	scriptName = udTable.GetString("script", unitName + ".cob");
	scriptPath = "scripts/" + scriptName;

	wreckName = udTable.GetString("corpse", "");
	deathExplosion = udTable.GetString("explodeAs", "");
	selfDExplosion = udTable.GetString("selfDestructAs", "");

	power = udTable.GetFloat("power", (metalCost + (energyCost / 60.0f)));

	// Prevent a division by zero in experience calculations.
	if (power < 1.0e-3f) {
		logOutput.Print("Unit %s is really cheap? %f", humanName.c_str(), power);
		logOutput.Print("This can cause a division by zero in experience calculations.");
		power = 1.0e-3f;
	}

	activateWhenBuilt = udTable.GetBool("activateWhenBuilt", false);

	// footprint sizes are assumed to be expressed in TA-engine units;
	// Spring's heightmap resolution is double the footprint (yardmap)
	// resolution, so we scale the values
	// NOTE that this is done for the MoveData footprints as well
	xsize = std::max(1 * 2, (udTable.GetInt("footprintX", 1) * 2));
	zsize = std::max(1 * 2, (udTable.GetInt("footprintZ", 1) * 2));

	if (IsImmobileUnit()) {
		CreateYardMap(udTable.GetString("yardMap", ""));
	}

	leaveTracks   = udTable.GetBool("leaveTracks", false);
	trackTypeName = udTable.GetString("trackType", "StdTank");
	trackWidth    = udTable.GetFloat("trackWidth",   32.0f);
	trackOffset   = udTable.GetFloat("trackOffset",   0.0f);
	trackStrength = udTable.GetFloat("trackStrength", 0.0f);
	trackStretch  = udTable.GetFloat("trackStretch",  1.0f);

	useBuildingGroundDecal = udTable.GetBool("useBuildingGroundDecal", false);
	buildingDecalTypeName = udTable.GetString("buildingGroundDecalType", "");
	buildingDecalSizeX = udTable.GetInt("buildingGroundDecalSizeX", 4);
	buildingDecalSizeY = udTable.GetInt("buildingGroundDecalSizeY", 4);
	buildingDecalDecaySpeed = udTable.GetFloat("buildingGroundDecalDecaySpeed", 0.1f);

	canDropFlare    = udTable.GetBool("canDropFlare", false);
	flareReloadTime = udTable.GetFloat("flareReload",     5.0f);
	flareDelay      = udTable.GetFloat("flareDelay",      0.3f);
	flareEfficiency = udTable.GetFloat("flareEfficiency", 0.5f);
	flareDropVector = udTable.GetFloat3("flareDropVector", ZeroVector);
	flareTime       = udTable.GetInt("flareTime", 3) * GAME_SPEED;
	flareSalvoSize  = udTable.GetInt("flareSalvoSize",  4);
	flareSalvoDelay = udTable.GetInt("flareSalvoDelay", 0) * GAME_SPEED;

	smoothAnim = udTable.GetBool("smoothAnim", false);
	canLoopbackAttack = udTable.GetBool("canLoopbackAttack", false);
	canCrash = udTable.GetBool("canCrash", true);
	levelGround = udTable.GetBool("levelGround", true);
	strafeToAttack = udTable.GetBool("strafeToAttack", false);


	modelCenterOffset = udTable.GetFloat3("modelCenterOffset", ZeroVector);
	usePieceCollisionVolumes = udTable.GetBool("usePieceCollisionVolumes", false);

	// initialize the (per-unitdef) collision-volume
	// all CUnit instances hold a copy of this object
	collisionVolume = new CollisionVolume(
		udTable.GetString("collisionVolumeType", ""),
		udTable.GetFloat3("collisionVolumeScales", ZeroVector),
		udTable.GetFloat3("collisionVolumeOffsets", ZeroVector),
		udTable.GetInt("collisionVolumeTest", CollisionVolume::COLVOL_HITTEST_DISC)
	);

	if (usePieceCollisionVolumes) {
		collisionVolume->Disable();
	}


	seismicRadius    = udTable.GetInt("seismicDistance", 0);
	seismicSignature = udTable.GetFloat("seismicSignature", -1.0f);
	if (seismicSignature == -1.0f) {
		if (!floater && !canhover && !canfly) {
			seismicSignature = sqrt(mass / 100.0f);
		} else {
			seismicSignature = 0.0f;
		}
	}

	LuaTable buildsTable = udTable.SubTable("buildOptions");
	if (buildsTable.IsValid()) {
		for (int bo = 1; true; bo++) {
			const string order = buildsTable.GetString(bo, "");
			if (order.empty()) {
				break;
			}
			buildOptions[bo] = order;
		}
	}

	LuaTable sfxTable = udTable.SubTable("SFXTypes");
	LuaTable expTable = sfxTable.SubTable("explosionGenerators");

	for (int expNum = 1; expNum <= 1024; expNum++) {
		std::string expsfx = expTable.GetString(expNum, "");

		if (expsfx == "") {
			break;
		} else {
			sfxExplGenNames.push_back(expsfx);
		}
	}

	// we use range in a modulo operation, so it needs to be >= 1
	pieceTrailCEGTag = udTable.GetString("pieceTrailCEGTag", "");
	pieceTrailCEGRange = udTable.GetInt("pieceTrailCEGRange", 1);
	pieceTrailCEGRange = std::max(pieceTrailCEGRange, 1);

	// custom parameters table
	udTable.SubTable("customParams").GetMap(customParams);
}


UnitDef::~UnitDef()
{
	for (int u = 0; u < 4; u++) {
		yardmaps[u].clear();
	}

	if (buildPic) {
		buildPic->Free();
		delete buildPic;
		buildPic = NULL;
	}

	delete collisionVolume;
	collisionVolume = NULL;

	for (std::vector<CExplosionGenerator*>::iterator it = sfxExplGens.begin(); it != sfxExplGens.end(); ++it) {
		delete *it;
	}
}


S3DModel* UnitDef::LoadModel() const
{
	// not exactly kosher, but...
	UnitDef* udef = const_cast<UnitDef*>(this);

	if (udef->modelDef.model == NULL) {
		udef->modelDef.model = modelParser->Load3DModel(udef->modelDef.modelPath, udef->modelCenterOffset);
		udef->modelDef.modelTextures["tex1"] = udef->modelDef.model->tex1;
		udef->modelDef.modelTextures["tex2"] = udef->modelDef.model->tex2;
	}

	return (udef->modelDef.model);
}


void UnitDef::ParseWeaponsTable(const LuaTable& weaponsTable)
{
	const WeaponDef* noWeaponDef = weaponDefHandler->GetWeapon("NOWEAPON");

	for (int w = 0; w < MAX_WEAPONS_PER_UNIT; w++) {
		LuaTable wTable;
		string name = weaponsTable.GetString(w + 1, "");
		if (name.empty()) {
			wTable = weaponsTable.SubTable(w + 1);
			name = wTable.GetString("name", "");
		}
		const WeaponDef* wd = NULL;
		if (!name.empty()) {
			wd = weaponDefHandler->GetWeapon(name);
		}
		if (wd == NULL) {
			if (w <= 3) {
				continue; // allow empty weapons among the first 3
			} else {
				break;
			}
		}

		while (weapons.size() < w) {
			if (!noWeaponDef) {
				logOutput.Print("Error: Spring requires a NOWEAPON weapon type "
				                "to be present as a placeholder for missing weapons");
				break;
			} else {
				weapons.push_back(UnitDefWeapon());
				weapons.back().def = noWeaponDef;
			}
		}

		const string& badTarget = wTable.GetString("badTargetCategory", "");
		const string& onlyTarget = wTable.GetString("onlyTargetCategory", "");

		unsigned int btc = CCategoryHandler::Instance()->GetCategories(badTarget);
		unsigned int otc = 0xffffffff;

		if (!onlyTarget.empty()) {
			otc = CCategoryHandler::Instance()->GetCategories(onlyTarget);
		}

		const unsigned int slaveTo = wTable.GetInt("slaveTo", 0);

		const float3 mainDir = wTable.GetFloat3("mainDir", float3(1.0f, 0.0f, 0.0f)).SafeNormalize();
		const float angleDif = cos(wTable.GetFloat("maxAngleDif", 360.0f) * (PI / 360.0f));

		const float fuelUse = wTable.GetFloat("fuelUsage", 0.0f);

		UnitDefWeapon weapon(name, wd, slaveTo, mainDir, angleDif, btc, otc, fuelUse);
		weapons.push_back(weapon);

		maxWeaponRange = std::max(maxWeaponRange, wd->range);

		if (wd->interceptor && wd->coverageRange > maxCoverage)
			maxCoverage = wd->coverageRange;

		if (wd->isShield) {
			if (!shieldWeaponDef || // use the biggest shield
			    (shieldWeaponDef->shieldRadius < wd->shieldRadius)) {
				shieldWeaponDef = wd;
			}
		}

		if (wd->stockpile) {
			// interceptors have priority
			if (wd->interceptor || !stockpileWeaponDef || !stockpileWeaponDef->interceptor) {
				stockpileWeaponDef = wd;
			}
		}
	}
}



void UnitDef::CreateYardMap(std::string yardMapStr)
{
	// create the yardmaps for each build-facing
	// (xsize and zsize are in heightmap units at
	// this point)
	for (int u = 0; u < NUM_FACINGS; u++) {
		yardmaps[u].resize(xsize * zsize);
	}

	const unsigned int hxsize = xsize >> 1;
	const unsigned int hzsize = zsize >> 1;

	std::vector<unsigned char> yardMap(hxsize * hzsize, 255);

	if (!yardMapStr.empty()) {
		StringToLowerInPlace(yardMapStr);

		unsigned int x = 0;
		unsigned int z = 0;

		for (unsigned int n = 0; n < yardMapStr.size(); n++) {
			const unsigned char c = yardMapStr[n];

			if (c == ' ') { continue; }
			if (c == 'g') { needGeo = true; continue; }
			if (c == 'c') { yardMap[x + z * hxsize] = 1; }   // blocking (not walkable, not buildable)
			if (c == 'y') { yardMap[x + z * hxsize] = 0; }   // non-blocking (walkable, buildable)
		//	if (c == 'o') { yardMap[x + z * hxsize] = 2; }   // walkable, not buildable?
		//	if (c == 'w') { yardMap[x + z * hxsize] = 3; }   // not walkable, buildable?

			x += 1;
			z += ((x == hxsize)? 1: 0);
			x %= hxsize;

			if (z >= hzsize) {
				break;
			}
		}
	}

	for (unsigned int z = 0; z < zsize; z++) {
		for (unsigned int x = 0; x < xsize; x++) {
			const unsigned int yardMapIdx = (x >> 1) + ((z >> 1) * hxsize);
			const unsigned char yardMapChar = yardMap[yardMapIdx];

			yardmaps[FACING_SOUTH][                  (x + z * xsize)                ] = yardMapChar;
			yardmaps[FACING_EAST ][(xsize * zsize) - (zsize * (x + 1) - (z + 1) + 1)] = yardMapChar;
			yardmaps[FACING_NORTH][(xsize * zsize) - (x + z * xsize + 1)            ] = yardMapChar;
			yardmaps[FACING_WEST ][                   zsize * (x + 1) - (z + 1)     ] = yardMapChar;
		}
	}
}



void UnitDef::SetNoCost(bool noCost)
{
	if (noCost) {
		realMetalCost    = metalCost;
		realEnergyCost   = energyCost;
		realMetalUpkeep  = metalUpkeep;
		realEnergyUpkeep = energyUpkeep;
		realBuildTime    = buildTime;

		metalCost    =  1.0f;
		energyCost   =  1.0f;
		buildTime    = 10.0f;
		metalUpkeep  =  0.0f;
		energyUpkeep =  0.0f;
	} else {
		metalCost    = realMetalCost;
		energyCost   = realEnergyCost;
		buildTime    = realBuildTime;
		metalUpkeep  = realMetalUpkeep;
		energyUpkeep = realEnergyUpkeep;
	}
}



bool UnitDef::IsAllowedTerrainHeight(float rawHeight, float* clampedHeight) const {
	float maxDepth = this->maxWaterDepth;
	float minDepth = this->minWaterDepth;

	if (movedata != NULL) {
		// we are a mobile ground-unit
		if (movedata->moveType == MoveData::Ship_Move) {
			minDepth = movedata->depth;
		} else {
			maxDepth = movedata->depth;
		}
	}

	if (floater || canhover) {
		// if we are a surface unit, <maxDepth> is irrelevant
		// (eg. hovercraft may be dropped anywhere over water)
		maxDepth = +10e6f;
	}

	if (clampedHeight != NULL) {
		*clampedHeight = rawHeight;
		*clampedHeight = std::max(*clampedHeight, -maxDepth);
		*clampedHeight = std::min(*clampedHeight, -minDepth);
	}

	// <rawHeight> must lie in the range [-maxDepth, -minDepth]
	return (rawHeight >= -maxDepth && rawHeight <= -minDepth);
}



bool UnitDef::HasBomberWeapon() const {
	if (weapons.empty()) { return false; }
	if (weapons[0].def == NULL) { return false; }
	return
		weapons[0].def->type == "AircraftBomb" ||
		weapons[0].def->type == "TorpedoLauncher";
}

/******************************************************************************/
