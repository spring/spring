#include "StdAfx.h"
#include "UnitDefHandler.h"
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <locale>
#include <cctype>

#include "UnitDef.h"
#include "UnitImage.h"

#include "FileSystem/FileHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/Team.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/CollisionVolumeData.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/SensorHandler.h"
#include "Sim/ModInfo.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/LogOutput.h"
#include "System/Sound.h"
#include "System/TdfParser.h"
#include "mmgr.h"

CR_BIND(UnitDef, );

const char YARDMAP_CHAR = 'c';		//Need to be low case.


CUnitDefHandler* unitDefHandler;

CUnitDefHandler::CUnitDefHandler(void) : noCost(false)
{
	weaponDefHandler = SAFE_NEW CWeaponDefHandler();

	PrintLoadMsg("Loading unit definitions");

	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("UnitDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading UnitDefs");
	}

	vector<string> unitDefNames;
	rootTable.GetKeys(unitDefNames);

	numUnitDefs = unitDefNames.size();

	/*
	if (gameSetup) {
		// ?? "restricted" does not automatically mean "cannot be built
		// at all, so we don't need the unitdef for this unit" -- Kloot
		numUnitDefs -= gameSetup->restrictedUnits.size();
	}
	*/

	// This could be wasteful if there is a lot of restricted units, but that is not that likely
	unitDefs = SAFE_NEW UnitDef[numUnitDefs + 1];

	// start at unitdef id 1
	unsigned int id = 1;

	for (unsigned int a = 0; a < unitDefNames.size(); ++a) {
		const string unitName = unitDefNames[a];

		/*
		if (gameSetup) {
			// Restrictions may tell us not to use this unit at all
			// FIXME: causes mod errors when a unit is restricted to
			// 0, since GetUnitByName() will return NULL if its UnitDef
			// has not been loaded -- Kloot
			const std::map<std::string, int>& resUnits = gameSetup->restrictedUnits;

			if ((resUnits.find(unitName) != resUnits.end()) &&
				(resUnits.find(unitName)->second == 0)) {
				continue;
			}
		}
		*/

		// Seems ok, load it
		unitDefs[id].valid = false;
		unitDefs[id].name = unitName;
		unitDefs[id].id = id;
		unitDefs[id].buildangle = 0;
		unitDefs[id].unitImage  = 0;
		unitDefs[id].collisionVolumeData = 0;
		unitDefs[id].techLevel  = -1;
		unitDefs[id].decoyDef   = NULL;
		unitID[unitName] = id;
		for (int ym = 0; ym < 4; ym++) {
			unitDefs[id].yardmaps[ym] = 0;
		}

		// parse the TDF data (but don't load buildpics, etc...)
		LuaTable udTable = rootTable.SubTable(unitName);
		ParseUnit(udTable, unitName, id);

		// Increase index for next unit
		id++;
	}

	// set the real number of unitdefs
	numUnitDefs = (id - 1);

	CleanBuildOptions();
	FindCommanders();
	ProcessDecoys();
	AssignTechLevels();
}


CUnitDefHandler::~CUnitDefHandler(void)
{
	// delete any eventual yardmaps
	for (int i = 1; i <= numUnitDefs; i++) {
		for (int u = 0; u < 4; u++)
			delete[] unitDefs[i].yardmaps[u];

		if (unitDefs[i].unitImage) {
			glDeleteTextures(1, &unitDefs[i].unitImage->textureID);
			delete unitDefs[i].unitImage;
			unitDefs[i].unitImage = 0;
		}

		delete unitDefs[i].collisionVolumeData;
		unitDefs[i].collisionVolumeData = 0;
	}
	delete[] unitDefs;
	delete weaponDefHandler;
}


void CUnitDefHandler::CleanBuildOptions()
{
	// remove invalid build options
	for (int i = 1; i <= numUnitDefs; i++) {
		UnitDef& ud = unitDefs[i];
		map<int, string>& bo = ud.buildOptions;
		map<int, string>::iterator it = bo.begin();
		while (it != bo.end()) {
			bool erase = false;

			const UnitDef* bd = GetUnitByName(it->second);
			if (bd == NULL) {
				logOutput.Print("WARNING: removed the \"" + it->second +
				                "\" entry from the \"" + ud.name + "\" build menu");
				erase = true;
			}
			/*
			else if (bd->maxThisUnit <= 0) {
				// don't remove, just grey out the icon
				erase = true; // silent removal
			}
			*/

			if (erase) {
				map<int, string>::iterator tmp = it;
				it++;
				bo.erase(tmp);
			} else {
				it++;
			}
		}
	}
}


void CUnitDefHandler::ProcessDecoys()
{
	// assign the decoy pointers, and build the decoy map
	map<string, string>::const_iterator mit;
	for (mit = decoyNameMap.begin(); mit != decoyNameMap.end(); ++mit) {
		map<string, int>::iterator fakeIt, realIt;
		fakeIt = unitID.find(mit->first);
		realIt = unitID.find(mit->second);
		if ((fakeIt != unitID.end()) && (realIt != unitID.end())) {
			UnitDef* fake = &unitDefs[fakeIt->second];
			UnitDef* real = &unitDefs[realIt->second];
			fake->decoyDef = real;
			decoyMap[real->id].insert(fake->id);
		}
	}
	decoyNameMap.clear();
}


void CUnitDefHandler::FindCommanders()
{
	TdfParser tdfparser;
	tdfparser.LoadFile("gamedata/SIDEDATA.TDF");

	// get the commander UnitDef IDs
	commanderIDs.clear();
	std::vector<std::string> sides = tdfparser.GetSectionList("");
	for (unsigned int i = 0; i < sides.size(); i++){
		const std::string& section = sides[i];
		if ((section.find("side") == 0) &&
		    (section.find_first_not_of("0123456789", 4) == std::string::npos)) {
			string commUnit = tdfparser.SGetValueDef("", section + "\\COMMANDER");
			StringToLowerInPlace(commUnit);
			if (!commUnit.empty()) {
				std::map<std::string, int>::iterator it = unitID.find(commUnit);
				if (it != unitID.end()) {
					commanderIDs.insert(it->second);
				}
			}
		}
	}
}



void CUnitDefHandler::ParseTAUnit(const LuaTable& udTable, const string& unitName, int id)
{
	UnitDef& ud = unitDefs[id];

	// allocate and fill ud->unitImage
	GetUnitImage(&unitDefs[id], udTable.GetString("buildPic", ""));

	ud.humanName = udTable.GetString("name", "");

	if (ud.humanName.empty()) {
		const string errmsg = "missing 'name' parameter for the" + unitName + " unitdef";
		throw content_error(errmsg);
	}
	ud.filename  = udTable.GetString("filename", "");
	if (ud.filename.empty()) {
		const string errmsg = "missing 'filename' parameter for the" + unitName + " unitdef";
		throw content_error(errmsg);
	}
	ud.tooltip = udTable.GetString("description", ud.name);

	const string decoy = udTable.GetString("decoyFor", "");
	if (!decoy.empty()) {
		decoyNameMap[ud.name] = StringToLower(decoy);
	}

	ud.gaia = udTable.GetString("gaia", "");

	ud.isCommander = udTable.GetBool("commander", false);

	if (ud.isCommander && gameSetup) {
		ud.metalStorage  = udTable.GetFloat("metalStorage",  gameSetup->startMetal);
		ud.energyStorage = udTable.GetFloat("energyStorage", gameSetup->startEnergy);
	} else {
		ud.metalStorage  = udTable.GetFloat("metalStorage",  0.0f);
		ud.energyStorage = udTable.GetFloat("energyStorage", 0.0f);
	}

	ud.extractsMetal  = udTable.GetFloat("extractsMetal",  0.0f);
	ud.windGenerator  = udTable.GetFloat("windGenerator",  0.0f);
	ud.tidalGenerator = udTable.GetFloat("tidalGenerator", 0.0f);

	ud.metalUpkeep  = udTable.GetFloat("metalUse",   0.0f);
	ud.energyUpkeep = udTable.GetFloat("energyUse",  0.0f);
	ud.metalMake    = udTable.GetFloat("metalMake",  0.0f);
	ud.makesMetal   = udTable.GetFloat("makesMetal", 0.0f);
	ud.energyMake   = udTable.GetFloat("energyMake", 0.0f);

	ud.health       = udTable.GetFloat("maxDamage",  0.0f);
	ud.autoHeal     = udTable.GetFloat("autoHeal",      0.0f) * (16.0f / 30.0f);
	ud.idleAutoHeal = udTable.GetFloat("idleAutoHeal", 10.0f) * (16.0f / 30.0f);
	ud.idleTime     = udTable.GetInt("idleTime", 600);

	ud.buildangle = udTable.GetInt("buildAngle", 0);

	ud.isMetalMaker = (ud.makesMetal >= 1 && ud.energyUpkeep > ud.makesMetal * 40);

	ud.controlRadius = 32;
	ud.losHeight = 20;
	ud.metalCost = udTable.GetFloat("buildCostMetal", 0.0f);
	if (ud.metalCost < 1.0f) {
		ud.metalCost = 1.0f; //avoid some nasty divide by 0 etc
	}
	ud.mass = udTable.GetFloat("mass", 0.0f);
	if (ud.mass <= 0.0f) {
		ud.mass=ud.metalCost;
	}
	ud.energyCost = udTable.GetFloat("buildCostEnergy", 0.0f);
	ud.buildTime = udTable.GetFloat("buildTime", 0.0f);
	if (ud.buildTime < 1.0f) {
		ud.buildTime = 1.0f; //avoid some nasty divide by 0 etc
	}

	ud.aihint = id; // FIXME? (as noted in SelectedUnits.cpp, aihint is ignored)
	ud.cobID = udTable.GetInt("cobID", -1);

	ud.losRadius = udTable.GetFloat("sightDistance", 0.0f) * sensorHandler->losMul / (SQUARE_SIZE * (1 << sensorHandler->losMipLevel));
	ud.airLosRadius = udTable.GetFloat("airSightDistance", -1.0f);
	if (ud.airLosRadius == -1.0f) {
		ud.airLosRadius=udTable.GetFloat("sightDistance", 0.0f) * sensorHandler->airLosMul * 1.5f / (SQUARE_SIZE * (1 << sensorHandler->airMipLevel));
	} else {
		ud.airLosRadius = ud.airLosRadius * sensorHandler->airLosMul / (SQUARE_SIZE * (1 << sensorHandler->airMipLevel));
	}

	ud.moveType = 0;

	ud.canSubmerge = udTable.GetBool("canSubmerge", false);
	ud.canfly      = udTable.GetBool("canFly",      false);
	ud.canmove     = udTable.GetBool("canMove",     false);
	ud.reclaimable = udTable.GetBool("reclaimable", true);
	ud.capturable  = udTable.GetBool("capturable",  true);
	ud.canAttack   = udTable.GetBool("canAttack",   true);
	ud.canFight    = udTable.GetBool("canFight",    true);
	ud.canPatrol   = udTable.GetBool("canPatrol",   true);
	ud.canGuard    = udTable.GetBool("canGuard",    true);
	ud.canRepeat   = udTable.GetBool("canRepeat",   true);
	bool noAutoFire  = udTable.GetBool("noAutoFire",  false);
	ud.canFireControl = udTable.GetBool("canFireControl", !noAutoFire);
	ud.fireState = udTable.GetInt("fireState", ud.canFireControl ? -1 : 0);

	ud.builder = udTable.GetBool("builder", true);

	ud.canRestore = udTable.GetBool("canRestore", ud.builder);
	ud.canRepair  = udTable.GetBool("canRepair",  ud.builder);
	ud.canReclaim = udTable.GetBool("canReclaim", ud.builder);
	ud.canBuild   = udTable.GetBool("canBuild",   ud.builder);
	ud.canAssist  = udTable.GetBool("canAssist",  ud.builder);

	ud.canBeAssisted = udTable.GetBool("canBeAssisted", true);
	ud.canSelfRepair = udTable.GetBool("canSelfRepair", false);
	ud.fullHealthFactory = udTable.GetBool("fullHealthFactory", false);
	ud.factoryHeadingTakeoff = udTable.GetBool("factoryHeadingTakeoff", true);

	ud.upright = udTable.GetBool("upright", false);
	ud.collide = udTable.GetBool("collide", true);
	ud.onoffable = udTable.GetBool("onoffable", false);

	ud.maxSlope = udTable.GetFloat("maxSlope", 0.0f);
	ud.maxHeightDif = 40 * tan(ud.maxSlope * (PI / 180));
	ud.maxSlope = cos(ud.maxSlope * (PI / 180));
	ud.minWaterDepth = udTable.GetFloat("minWaterDepth", -10e6f);
	ud.maxWaterDepth = udTable.GetFloat("maxWaterDepth", +10e6f);
	ud.minCollisionSpeed = udTable.GetFloat("minCollisionSpeed", 1.0f);
	ud.slideTolerance = udTable.GetFloat("slideTolerance", 0.0f); // disabled
	ud.pushResistant = udTable.GetBool("pushResistant", false);

	ud.waterline = udTable.GetFloat("waterline", 0.0f);
	if ((ud.waterline > 8.0f) && ud.canmove) {
		// make subs travel at somewhat larger depths
		// to reduce vulnerability to surface weapons
		ud.waterline += 5.0f;
	}

	ud.canSelfD = udTable.GetBool("canSelfDestruct", true);
	ud.selfDCountdown = udTable.GetInt("selfDestructCountdown", 5);

	ud.speed    = udTable.GetFloat("maxVelocity",  0.0f) * 30.0f;
	ud.maxAcc   = udTable.GetFloat("acceleration", 0.5f);
	ud.maxDec   = udTable.GetFloat("brakeRate",    0.5f) * 0.1f;
	ud.turnRate = udTable.GetFloat("turnRate",     0.0f);

	ud.buildRange3D = udTable.GetBool("buildRange3D", false);
	ud.buildDistance = udTable.GetFloat("buildDistance", 128.0f);
	ud.buildDistance = std::max(128.0f, ud.buildDistance);
	ud.buildSpeed = udTable.GetFloat("workerTime", 0.0f);

	ud.repairSpeed    = udTable.GetFloat("repairSpeed",    ud.buildSpeed);
	ud.maxRepairSpeed = udTable.GetFloat("maxRepairSpeed", 1e20f);
	ud.reclaimSpeed   = udTable.GetFloat("reclaimSpeed",   ud.buildSpeed);
	ud.resurrectSpeed = udTable.GetFloat("resurrectSpeed", ud.buildSpeed);
	ud.captureSpeed   = udTable.GetFloat("captureSpeed",   ud.buildSpeed);
	ud.terraformSpeed = udTable.GetFloat("terraformSpeed", ud.buildSpeed);

	ud.flankingBonusMode = udTable.GetInt("flankingBonusMode", modInfo.flankingBonusModeDefault);
	ud.flankingBonusMax  = udTable.GetFloat("flankingBonusMax", 1.9f);
	ud.flankingBonusMin  = udTable.GetFloat("flankingBonusMin", 0.9);
	ud.flankingBonusDir  = udTable.GetFloat3("flankingBonusDir", float3(0.0f, 0.0f, 1.0f));
	ud.flankingBonusMobilityAdd = udTable.GetFloat("flankingBonusMobilityAdd", 0.01f);

	ud.armoredMultiple = udTable.GetFloat("damageModifier", 1.0f);
	ud.armorType = damageArrayHandler->GetTypeFromName(ud.name);

	ud.radarRadius    = udTable.GetInt("radarDistance",    0);
	ud.sonarRadius    = udTable.GetInt("sonarDistance",    0);
	ud.jammerRadius   = udTable.GetInt("radarDistanceJam", 0);
	ud.sonarJamRadius = udTable.GetInt("sonarDistanceJam", 0);

	ud.stealth        = udTable.GetBool("stealth",            false);
	ud.targfac        = udTable.GetBool("isTargetingUpgrade", false);
	ud.isFeature      = udTable.GetBool("isFeature",          false);
	ud.canResurrect   = udTable.GetBool("canResurrect",       false);
	ud.canCapture     = udTable.GetBool("canCapture",         false);
	ud.hideDamage     = udTable.GetBool("hideDamage",         false);
	ud.showPlayerName = udTable.GetBool("showPlayerName",     false);

	ud.cloakCost = udTable.GetFloat("cloakCost", -1.0f);
	ud.cloakCostMoving = udTable.GetFloat("cloakCostMoving", -1.0f);
	if (ud.cloakCostMoving < 0) {
		ud.cloakCostMoving = ud.cloakCost;
	}
	ud.canCloak = (ud.cloakCost >= 0);

	ud.startCloaked     = udTable.GetBool("initCloaked", false);
	ud.decloakDistance  = udTable.GetFloat("minCloakDistance", 0.0f);
	ud.decloakSpherical = udTable.GetBool("decloakSpherical", true);
	ud.decloakOnFire    = udTable.GetBool("decloakOnFire",    true);

	ud.highTrajectoryType = udTable.GetInt("highTrajectory", 0);

	ud.canKamikaze = udTable.GetBool("kamikaze", false);
	ud.kamikazeDist = udTable.GetFloat("kamikazeDistance", -25.0f) + 25.0f; //we count 3d distance while ta count 2d distance so increase slightly

	ud.showNanoFrame = udTable.GetBool("showNanoFrame", true);
	ud.showNanoSpray = udTable.GetBool("showNanoSpray", true);
	ud.nanoColor = udTable.GetFloat3("nanoColor", float3(0.2f,0.7f,0.2f));

	ud.canhover = udTable.GetBool("canHover", false);

	ud.floater = udTable.GetBool("floater", udTable.KeyExists("WaterLine"));

	ud.builder = udTable.GetBool("builder", false);
	if (ud.builder && !ud.buildSpeed) { // core anti is flagged as builder for some reason
		ud.builder = false;
	}

	ud.airStrafe     = udTable.GetBool("airStrafe", true);
	ud.hoverAttack   = udTable.GetBool("hoverAttack", false);
	ud.wantedHeight  = udTable.GetFloat("cruiseAlt", 0.0f);
	ud.dlHoverFactor = udTable.GetFloat("airHoverFactor", -1.0f);

	ud.transportSize     = udTable.GetInt("transportSize",      0);
	ud.minTransportSize  = udTable.GetInt("minTransportSize",   0);
	ud.transportCapacity = udTable.GetInt("transportCapacity",  0);
	ud.isfireplatform    = udTable.GetBool("isFirePlatform",    false);
	ud.isAirBase         = udTable.GetBool("isAirBase",         false);
	ud.loadingRadius     = udTable.GetFloat("loadingRadius",    220.0f);
	ud.unloadSpread      = udTable.GetFloat("unloadSpread",     1.0f);
	ud.transportMass     = udTable.GetFloat("transportMass",    100000.0f);
	ud.minTransportMass  = udTable.GetFloat("minTransportMass", 0.0f);
	ud.holdSteady        = udTable.GetBool("holdSteady",        true);
	ud.releaseHeld       = udTable.GetBool("releaseHeld",       false);
	ud.cantBeTransported = udTable.GetBool("cantBeTransported", false);
	ud.transportByEnemy  = udTable.GetBool("transportByEnemy",  true);
	ud.fallSpeed         = udTable.GetFloat("fallSpeed",    0.2);
	ud.unitFallSpeed     = udTable.GetFloat("unitFallSpeed",  0);
	ud.transportUnloadMethod	= udTable.GetInt("transportUnloadMethod" , 0);

	// modrules transport settings
	if ((!modInfo.transportAir    && ud.canfly)   ||
	    (!modInfo.transportShip   && ud.floater)  ||
	    (!modInfo.transportHover  && ud.canhover) ||
	    (!modInfo.transportGround && !ud.canhover && !ud.floater && !ud.canfly)) {
 		ud.cantBeTransported = true;
	}

	ud.wingDrag     = udTable.GetFloat("wingDrag",     0.07f);  // drag caused by wings
	ud.wingAngle    = udTable.GetFloat("wingAngle",    0.08f);  // angle between front and the wing plane
	ud.frontToSpeed = udTable.GetFloat("frontToSpeed", 0.1f);   // fudge factor for lining up speed and front of plane
	ud.speedToFront = udTable.GetFloat("speedToFront", 0.07f);  // fudge factor for lining up speed and front of plane
	ud.myGravity    = udTable.GetFloat("myGravity",    0.4f);   // planes are slower than real airplanes so lower gravity to compensate

	ud.maxBank = udTable.GetFloat("maxBank", 0.8f);         // max roll
	ud.maxPitch = udTable.GetFloat("maxPitch", 0.45f);      // max pitch this plane tries to keep
	ud.turnRadius = udTable.GetFloat("turnRadius", 500.0f); // hint to the ai about how large turn radius this plane needs
	ud.verticalSpeed = udTable.GetFloat("verticalSpeed", 3.0f); // speed of takeoff and landing, at least for gunships

	ud.maxAileron  = udTable.GetFloat("maxAileron",  0.015f); // turn speed around roll axis
	ud.maxElevator = udTable.GetFloat("maxElevator", 0.01f);  // turn speed around pitch axis
	ud.maxRudder   = udTable.GetFloat("maxRudder",   0.004f); // turn speed around yaw axis

	ud.maxFuel = udTable.GetFloat("maxFuel", 0.0f); //max flight time in seconds before aircraft must return to base
	ud.refuelTime = udTable.GetFloat("refuelTime", 5.0f);
	ud.minAirBasePower = udTable.GetFloat("minAirBasePower", 0.0f);
	ud.maxThisUnit = udTable.GetInt("unitRestricted", MAX_UNITS);

	if (gameSetup) {
		string lname = StringToLower(ud.name);

		if (gameSetup->restrictedUnits.find(lname) != gameSetup->restrictedUnits.end()) {
			ud.maxThisUnit = min(ud.maxThisUnit, gameSetup->restrictedUnits.find(lname)->second);
		}
	}

	ud.categoryString = udTable.GetString("category", "");
	ud.category = CCategoryHandler::Instance()->GetCategories(udTable.GetString("category", ""));
	ud.noChaseCategory = CCategoryHandler::Instance()->GetCategories(udTable.GetString("noChaseCategory", ""));
//	logOutput.Print("Unit %s has cat %i",ud.humanName.c_str(),ud.category);

	ud.iconType = udTable.GetString("iconType", "default");

	ud.shieldWeaponDef    = NULL;
	ud.stockpileWeaponDef = NULL;

	ud.maxWeaponRange = 0.0f;

	const WeaponDef* noWeaponDef = weaponDefHandler->GetWeapon("NOWEAPON");

	LuaTable weaponsTable = udTable.SubTable("weapons");
	for (int w = 0; w < 16; w++) {
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

		while (ud.weapons.size() < w) {
			if (!noWeaponDef) {
				logOutput.Print("Error: Spring requires a NOWEAPON weapon type "
				                "to be present as a placeholder for missing weapons");
				break;
			} else {
				ud.weapons.push_back(UnitDef::UnitDefWeapon("NOWEAPON", noWeaponDef,
				                                            0, float3(0, 0, 1), -1,
				                                            0, 0, 0));
			}
		}

		const string badTarget = wTable.GetString("badTargetCategory", "");
		unsigned int btc = CCategoryHandler::Instance()->GetCategories(badTarget);

		const string onlyTarget = wTable.GetString("onlyTargetCategory", "");
		unsigned int otc;
		if (onlyTarget.empty()) {
			otc = 0xffffffff;
		} else {
			otc = CCategoryHandler::Instance()->GetCategories(onlyTarget);
		}

		const unsigned int slaveTo = wTable.GetInt("slaveTo", 0);

		float3 mainDir = wTable.GetFloat3("mainDir", float3(1.0f, 0.0f, 0.0f));
		mainDir.Normalize();

		const float angleDif = cos(wTable.GetFloat("maxAngleDif", 360.0f) * (PI / 360.0f));

		const float fuelUse = wTable.GetFloat("fuelUsage", 0.0f);

		ud.weapons.push_back(UnitDef::UnitDefWeapon(name, wd, slaveTo, mainDir,
		                                            angleDif, btc, otc, fuelUse));

		if (wd->range > ud.maxWeaponRange) {
			ud.maxWeaponRange = wd->range;
		}
		if (wd->isShield) {
			if (!ud.shieldWeaponDef || // use the biggest shield
			    (ud.shieldWeaponDef->shieldRadius < wd->shieldRadius)) {
				ud.shieldWeaponDef = wd;
			}
		}
		if (wd->stockpile) {
			// interceptors have priority
			if (wd->interceptor        ||
			    !ud.stockpileWeaponDef ||
			    !ud.stockpileWeaponDef->interceptor) {
				ud.stockpileWeaponDef = wd;
			}
		}
	}

	ud.canDGun = udTable.GetBool("canDGun", false);

	string TEDClass = udTable.GetString("TEDClass", "0");
	ud.TEDClassString = TEDClass;
	ud.extractRange = 0;

	if (ud.extractsMetal) {
		ud.extractRange = mapInfo->map.extractorRadius;
		ud.type = "MetalExtractor";
	}
	else if (ud.transportCapacity) {
		ud.type = "Transport";
	}
	else if (ud.builder) {
		if (TEDClass != "PLANT") {
			ud.type = "Builder";
		} else {
			ud.type = "Factory";
		}
	}
	else if (ud.canfly && !ud.hoverAttack) {
		if (!ud.weapons.empty() && (ud.weapons[0].def != 0) &&
		   (ud.weapons[0].def->type=="AircraftBomb" || ud.weapons[0].def->type=="TorpedoLauncher")) {
			ud.type = "Bomber";
			if (ud.turnRadius == 500) { // only reset it if user hasnt set it explicitly
				ud.turnRadius = 1000;     // hint to the ai about how large turn radius this plane needs
			}
		} else {
			ud.type = "Fighter";
		}
		ud.maxAcc = udTable.GetFloat("maxAcc", 0.065f); // engine power
	}
	else if (ud.canmove) {
		ud.type = "GroundUnit";
	}
	else {
		ud.type = "Building";
	}

	ud.movedata = 0;
	if (ud.canmove && !ud.canfly && (ud.type != "Factory")) {
		string moveclass = udTable.GetString("movementClass", "");
		ud.movedata = moveinfo->GetMoveDataFromName(moveclass);
		if ((ud.movedata->moveType == MoveData::Hover_Move) ||
		    (ud.movedata->moveType == MoveData::Ship_Move)) {
			ud.upright=true;
		}
		if (ud.canhover) {
			if (ud.movedata->moveType != MoveData::Hover_Move) {
				logOutput.Print("Inconsistant move data hover %i %s %s",
				                ud.movedata->pathType, ud.humanName.c_str(),
				                moveclass.c_str());
			}
		} else if (ud.floater) {
			if (ud.movedata->moveType != MoveData::Ship_Move) {
				logOutput.Print("Inconsistant move data ship %i %s %s",
				                ud.movedata->pathType, ud.humanName.c_str(),
				                moveclass.c_str());
			}
		} else {
			if (ud.movedata->moveType != MoveData::Ground_Move) {
				logOutput.Print("Inconsistant move data ground %i %s %s",
				                 ud.movedata->pathType, ud.humanName.c_str(),
				                 moveclass.c_str());
			}
		}
//		logOutput.Print("%s uses movetype %i",ud.humanName.c_str(),ud.movedata->pathType);
	}

	if ((ud.maxAcc != 0) && (ud.speed != 0)) {
		//meant to set the drag such that the maxspeed becomes what it should be
		ud.drag = 1.0f / (ud.speed/GAME_SPEED * 1.1f / ud.maxAcc)
		          - (ud.wingAngle * ud.wingAngle * ud.wingDrag);
	} else {
		//shouldn't be needed since drag is only used in CAirMoveType anyway,
		//and aircraft without acceleration or speed aren't common :)
		//initializing it anyway just for safety
		ud.drag = 0.005f;
	}

	std::string objectname = udTable.GetString("objectName", "");
	ud.model.modelpath = "objects3d/" + objectname;
	ud.model.modelname = objectname;

	ud.wreckName = udTable.GetString("corpse", "");
	ud.deathExplosion = udTable.GetString("explodeAs", "");
	ud.selfDExplosion = udTable.GetString("selfDestructAs", "");

	//ud.power = (ud.metalCost + ud.energyCost/60.0f);
	ud.power = udTable.GetFloat("power", (ud.metalCost + (ud.energyCost / 60.0f)));

	// Prevent a division by zero in experience calculations.
	if (ud.power < 1.0e-3f) {
		logOutput.Print("Unit %s is really cheap? %f", ud.humanName.c_str(), ud.power);
		logOutput.Print("This can cause a division by zero in experience calculations.");
		ud.power = 1.0e-3f;
	}

	ud.activateWhenBuilt = udTable.GetBool("activateWhenBuilt", false);

	// TA has only half our res so multiply size with 2
	ud.xsize = udTable.GetInt("footprintX", 1) * 2;
	ud.ysize = udTable.GetInt("footprintZ", 1) * 2;

	ud.needGeo = false;
	if ((ud.type == "Building") || (ud.type == "Factory")) {
		CreateYardMap(&ud, udTable.GetString("yardMap", "c"));
	} else {
		for (int u = 0; u < 4; u++) {
			ud.yardmaps[u] = 0;
		}
	}

	ud.leaveTracks   = udTable.GetBool("leaveTracks", false);
	ud.trackWidth    = udTable.GetFloat("trackWidth",   32.0f);
	ud.trackOffset   = udTable.GetFloat("trackOffset",   0.0f);
	ud.trackStrength = udTable.GetFloat("trackStrength", 0.0f);
	ud.trackStretch  = udTable.GetFloat("trackStretch",  1.0f);
	if (ud.leaveTracks && groundDecals) {
		ud.trackType = groundDecals->GetTrackType(udTable.GetString("trackType", "StdTank"));
	}

	ud.useBuildingGroundDecal = udTable.GetBool("useBuildingGroundDecal", false);
	ud.buildingDecalSizeX = udTable.GetInt("buildingGroundDecalSizeX", 4);
	ud.buildingDecalSizeY = udTable.GetInt("buildingGroundDecalSizeY", 4);
	ud.buildingDecalDecaySpeed = udTable.GetFloat("buildingGroundDecalDecaySpeed", 0.1f);
	if (ud.useBuildingGroundDecal && groundDecals) {
		ud.buildingDecalType = groundDecals->GetBuildingDecalType(udTable.GetString("buildingGroundDecalType", ""));
	}

	ud.canDropFlare    = udTable.GetBool("canDropFlare", false);
	ud.flareReloadTime = udTable.GetFloat("flareReload",     5.0f);
	ud.flareDelay      = udTable.GetFloat("flareDelay",      0.3f);
	ud.flareEfficiency = udTable.GetFloat("flareEfficiency", 0.5f);
	ud.flareDropVector = udTable.GetFloat3("flareDropVector", ZeroVector);
	ud.flareTime       = udTable.GetInt("flareTime", 3) * 30;
	ud.flareSalvoSize  = udTable.GetInt("flareSalvoSize",  4);
	ud.flareSalvoDelay = udTable.GetInt("flareSalvoDelay", 0) * 30;

	ud.smoothAnim = udTable.GetBool("smoothAnim", false);
	ud.canLoopbackAttack = udTable.GetBool("canLoopbackAttack", false);
	ud.canCrash = udTable.GetBool("canCrash", true);
	ud.levelGround = udTable.GetBool("levelGround", true);
	ud.strafeToAttack = udTable.GetBool("strafeToAttack", false);


	// aircraft collision sizes default to half their visual size, to
	// make them less likely to collide or get hit by nontracking weapons
	/*
	const float defScale = ud.canfly ? 0.5f : 1.0f;
	ud.collisionSphereScale = udTable.GetFloat("collisionSphereScale", defScale);
	ud.collisionSphereOffset = udTable.GetFloat3("collisionSphereOffset", ZeroVector);
	ud.useCSOffset = (ud.collisionSphereOffset != ZeroVector);
	*/

	// these take precedence over the old sphere tags and
	// unit->radius (for unit <--> projectile interactions)
	ud.collisionVolumeType = udTable.GetString("collisionVolumeType", "");
	ud.collisionVolumeScales = udTable.GetFloat3("collisionVolumeScales", ZeroVector);
	ud.collisionVolumeOffsets = udTable.GetFloat3("collisionVolumeOffsets", ZeroVector);
	ud.collisionVolumeTest = udTable.GetInt("collisionVolumeTest", COLVOL_TEST_DISC);

	// initialize the (per-unitdef) collision-volume
	// all CUnit instances hold a copy of this object
	ud.collisionVolumeData = SAFE_NEW CollisionVolumeData(ud.collisionVolumeType,
		ud.collisionVolumeScales, ud.collisionVolumeOffsets, ud.collisionVolumeTest);



	ud.seismicRadius    = udTable.GetInt("seismicDistance", 0);
	ud.seismicSignature = udTable.GetFloat("seismicSignature", -1.0f);
	if (ud.seismicSignature == -1.0f) {
		if (!ud.floater && !ud.canhover && !ud.canfly) {
			ud.seismicSignature = sqrt(ud.mass / 100.0f);
		} else {
			ud.seismicSignature = 0.0f;
		}
	}

	LuaTable buildsTable = udTable.SubTable("buildOptions");
	if (buildsTable.IsValid()) {
		for (int bo = 1; true; bo++) {
			const string order = buildsTable.GetString(bo, "");
			if (order.empty()) {
				break;
			}
			ud.buildOptions[bo] = order;
		}
	}

	LuaTable sfxTable = udTable.SubTable("SFXTypes");
	LuaTable expTable = sfxTable.SubTable("explosionGenerators");
	for (int expNum = 1; expNum <= 1024; expNum++) {
		string expsfx = expTable.GetString(expNum, "");
		if (expsfx == "") {
			break;
		} else {
			ud.sfxExplGens.push_back(explGenHandler->LoadGenerator(expsfx));
		}
	}

	// we use range in a modulo operation, so it needs to be >= 1
	ud.pieceTrailCEGTag = udTable.GetString("pieceTrailCEGTag", "");
	ud.pieceTrailCEGRange = udTable.GetInt("pieceTrailCEGRange", 1);
	ud.pieceTrailCEGRange = std::max(ud.pieceTrailCEGRange, 1);

	LuaTable soundsTable = udTable.SubTable("sounds");

	LoadSounds(soundsTable, ud.sounds.ok,          "ok");      // eg. "ok1", "ok2", ...
	LoadSounds(soundsTable, ud.sounds.select,      "select");  // eg. "select1", "select2", ...
	LoadSounds(soundsTable, ud.sounds.arrived,     "arrived"); // eg. "arrived1", "arrived2", ...
	LoadSounds(soundsTable, ud.sounds.build,       "build");
	LoadSounds(soundsTable, ud.sounds.activate,    "activate");
	LoadSounds(soundsTable, ud.sounds.deactivate,  "deactivate");
	LoadSounds(soundsTable, ud.sounds.cant,        "cant");
	LoadSounds(soundsTable, ud.sounds.underattack, "underattack");

	// custom parameters table
	udTable.SubTable("customParams").GetMap(ud.customParams);
}


void CUnitDefHandler::LoadSounds(const LuaTable& soundsTable,
                                 GuiSoundSet& gsound, const string& soundName)
{
	string fileName = soundsTable.GetString(soundName, "");
	if (!fileName.empty()) {
		LoadSound(gsound, fileName, 5.0f);
		return;
	}

	LuaTable sndTable = soundsTable.SubTable(soundName);
	for (int i = 1; true; i++) {
		LuaTable sndFileTable = sndTable.SubTable(i);
		if (sndFileTable.IsValid()) {
			fileName = sndFileTable.GetString("file", "");
			if (!fileName.empty()) {
				const float volume = sndFileTable.GetFloat("volume", 5.0f);
				if (volume > 0.0f) {
					LoadSound(gsound, fileName, volume);
				}
			}
		} else {
			fileName = sndTable.GetString(i, "");
			if (fileName.empty()) {
				break;
			}
			LoadSound(gsound, fileName, 5.0f);
		}
	}
}


void CUnitDefHandler::LoadSound(GuiSoundSet& gsound,
                                const string& fileName, float volume)
{
	const string soundFile = "sounds/" + fileName + ".wav";
	CFileHandler fh(soundFile);

	if (fh.FileExists()) {
		// we have a valid soundfile: store name, ID, and default volume
		PUSH_CODE_MODE;
		ENTER_UNSYNCED;
		const int id = sound->GetWaveId(soundFile);
		POP_CODE_MODE;

		GuiSoundSet::Data soundData(fileName, id, volume);
		gsound.sounds.push_back(soundData);
	}
}


void CUnitDefHandler::ParseUnit(const LuaTable& udTable, const string& unitName, int id)
{
  try {
    ParseTAUnit(udTable, unitName, id);
  } catch (content_error const& e) {
    std::cout << e.what() << std::endl;
    return;
  }

	unitDefs[id].valid = true;

	if (noCost) {
		unitDefs[id].metalCost    = 1;
		unitDefs[id].energyCost   = 1;
		unitDefs[id].buildTime    = 10;
		unitDefs[id].metalUpkeep  = 0;
		unitDefs[id].energyUpkeep = 0;
	}
}


const UnitDef* CUnitDefHandler::GetUnitByName(std::string name)
{
	StringToLowerInPlace(name);

	std::map<std::string, int>::iterator it = unitID.find(name);
	if (it == unitID.end()) {
		return NULL;
	}

	const int id = it->second;
	if (!unitDefs[id].valid) {
		return NULL; // should not happen
	}

	return &unitDefs[id];
}


const UnitDef* CUnitDefHandler::GetUnitByID(int id)
{
	if ((id <= 0) || (id > numUnitDefs)) {
		return NULL;
	}
	const UnitDef* ud = &unitDefs[id];
	if (!ud->valid) {
		return NULL;
	}
	return ud;
}


/*
Creates a open ground blocking map, called yardmap.
When sat != 0, is used instead of normal all-over-blocking.
*/
void CUnitDefHandler::CreateYardMap(UnitDef *def, std::string yardmapStr) {
	//Force string to lower case.
	StringToLowerInPlace(yardmapStr);

	//Creates the map.
	for (int u=0;u<4;u++)
		def->yardmaps[u] = SAFE_NEW unsigned char[def->xsize * def->ysize];

	unsigned char *originalMap = SAFE_NEW unsigned char[def->xsize * def->ysize / 4];		//TAS resolution is double of TA resolution.
	memset(originalMap, 255, def->xsize * def->ysize / 4);

	if(!yardmapStr.empty()){
		std::string::iterator si = yardmapStr.begin();
		int x, y;
		for(y = 0; y < def->ysize / 2; y++) {
			for(x = 0; x < def->xsize / 2; x++) {
				if(*si == 'g')
					def->needGeo=true;
				else if(*si == YARDMAP_CHAR)
					originalMap[x + y*def->xsize/2] = 1;
				else if(*si == 'y')
					originalMap[x + y*def->xsize/2] = 0;
				do {
					si++;
				} while(si != yardmapStr.end() && *si == ' ');
				if(si == yardmapStr.end())
					break;
			}
			if(si == yardmapStr.end())
				break;
		}
	}
	for(int y = 0; y < def->ysize; y++)
		for(int x = 0; x < def->xsize; x++){
			def->yardmaps[0][x + y*def->xsize] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[1][(def->ysize*def->xsize)-(def->ysize*(x+1)-(y+1)+1)] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[2][(def->ysize*def->xsize)-(x + y*def->xsize+1)] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[3][def->ysize*(x+1)-(y+1)] = originalMap[x/2 + y/2*def->xsize/2];
		}
	delete[] originalMap;
}


static bool LoadBuildPic(const string& filename, CBitmap& bitmap)
{
	CFileHandler bfile(filename);
	if (bfile.FileExists()) {
		bitmap.Load(filename);
		return true;
	}
	return false;
}


unsigned int CUnitDefHandler::GetUnitImage(const UnitDef* unitdef, std::string buildPicName)
{
	if (unitdef->unitImage != 0) {
		return (unitdef->unitImage->textureID);
	}

	CBitmap bitmap;

	if (!buildPicName.empty()) {
		bitmap.Load("unitpics/" + buildPicName);
	}
	else {
		if (!LoadBuildPic("unitpics/" + unitdef->name + ".dds", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitdef->name + ".png", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitdef->name + ".pcx", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitdef->name + ".bmp", bitmap)) {
			bitmap.Alloc(1, 1); // last resort
		}
	}

	const unsigned int texID = bitmap.CreateTexture(false);

	PUSH_CODE_MODE;
	ENTER_SYNCED;
	UnitDef& ud = unitDefs[unitdef->id]; // get away with the const
	ud.unitImage = SAFE_NEW UnitImage;
	ud.unitImage->buildPicName = buildPicName;
	ud.unitImage->textureID = texID;
	ud.unitImage->imageSizeX = bitmap.xsize;
	ud.unitImage->imageSizeY = bitmap.ysize;
	POP_CODE_MODE;

	return texID;
}


void CUnitDefHandler::AssignTechLevels()
{
	set<int>::iterator it;
	for (it = commanderIDs.begin(); it != commanderIDs.end(); ++it) {
		AssignTechLevel(unitDefs[*it], 0);
	}
}


void CUnitDefHandler::AssignTechLevel(UnitDef& ud, int level)
{
	if ((ud.techLevel >= 0) && (ud.techLevel <= level)) {
		return;
	}

	ud.techLevel = level;

	level++;

	map<int, std::string>::const_iterator bo_it;
	for (bo_it = ud.buildOptions.begin(); bo_it != ud.buildOptions.end(); ++bo_it) {
		std::map<std::string, int>::const_iterator ud_it = unitID.find(bo_it->second);
		if (ud_it != unitID.end()) {
			AssignTechLevel(unitDefs[ud_it->second], level);
		}
	}
}


bool CUnitDefHandler::SaveTechLevels(const std::string& filename,
                                     const std::string& modname)
{
	FILE* f;
	if (filename.empty()) {
		f = stdout;
	} else {
		f = fopen(filename.c_str(), "wt");
		if (f == NULL) {
			return false;
		}
	}

	fprintf(f, "\nTech Levels for \"%s\"\n", modname.c_str());
	multimap<int, string> entries;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitID.begin(); uit != unitID.end(); uit++) {
		const string& unitName = uit->first;
		const UnitDef* ud = GetUnitByName(unitName);
		if (ud) {
			char buf[256];
			SNPRINTF(buf, sizeof(buf), " %3i:  %-15s  // %s :: %s\n",
							 ud->techLevel, unitName.c_str(),
							 ud->humanName.c_str(), ud->tooltip.c_str());
			entries.insert(pair<int, string>(ud->techLevel, buf));
		}
	}
	int prevLevel = -2;
	multimap<int, string>::iterator eit;
	for (eit = entries.begin(); eit != entries.end(); ++eit) {
		if (eit->first != prevLevel) {
			fprintf(f, "\n");
			prevLevel = eit->first;
		}
		fprintf(f, "%s", eit->second.c_str());
	}

	if (f != stdout) {
		fclose(f);
	}
	return true;
}


UnitDef::~UnitDef()
{
}


S3DOModel* UnitDef::LoadModel(int team) const
{
	return modelParser->Load3DModel(model.modelpath.c_str(), 1.0f, team);

	/*
	if (!useCSOffset) {
		return modelParser->Load3DO(model.modelpath.c_str(),
		                            collisionSphereScale, team);
	} else {
		return modelParser->Load3DO(model.modelpath.c_str(),
		                            collisionSphereScale, team,
		                            collisionSphereOffset);
	};
	*/
}
