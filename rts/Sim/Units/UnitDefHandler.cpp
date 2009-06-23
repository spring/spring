#include "StdAfx.h"
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <locale>
#include <cctype>
#include "mmgr.h"

#include "UnitDefHandler.h"
#include "UnitDef.h"
#include "UnitDefImage.h"

#include "FileSystem/FileHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/Team.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "ConfigHandler.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/IconHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/UnitModels/IModelParser.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/COB/UnitScriptNames.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "LogOutput.h"
#include "Sound/Sound.h"
#include "Exceptions.h"

const char YARDMAP_CHAR = 'c';		//Need to be low case.


CUnitDefHandler* unitDefHandler;


UnitDef::UnitDefWeapon::UnitDefWeapon()
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


UnitDef::UnitDefWeapon::UnitDefWeapon(
	std::string name, const WeaponDef* def, int slavedTo, float3 mainDir, float maxAngleDif,
	unsigned int badTargetCat, unsigned int onlyTargetCat, float fuelUse):
	name(name),
	def(def),
	slavedTo(slavedTo),
	mainDir(mainDir),
	maxAngleDif(maxAngleDif),
	fuelUsage(fuelUse),
	badTargetCat(badTargetCat),
	onlyTargetCat(onlyTargetCat)
{}


CUnitDefHandler::CUnitDefHandler(void) : noCost(false)
{
	weaponDefHandler = new CWeaponDefHandler();

	PrintLoadMsg("Loading unit definitions");

	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("UnitDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading UnitDefs");
	}

	vector<string> unitDefNames;
	rootTable.GetKeys(unitDefNames);

	numUnitDefs = unitDefNames.size();

	/*
	// ?? "restricted" does not automatically mean "cannot be built
	// at all, so we don't need the unitdef for this unit" -- Kloot
	numUnitDefs -= gameSetup->restrictedUnits.size();
	*/

	// This could be wasteful if there is a lot of restricted units, but that is not that likely
	unitDefs = new UnitDef[numUnitDefs + 1];

	// start at unitdef id 1
	unsigned int id = 1;

	for (unsigned int a = 0; a < unitDefNames.size(); ++a) {
		const string unitName = unitDefNames[a];

		/*
		// Restrictions may tell us not to use this unit at all
		// FIXME: causes mod errors when a unit is restricted to
		// 0, since GetUnitByName() will return NULL if its UnitDef
		// has not been loaded -- Kloot
		const std::map<std::string, int>& resUnits = gameSetup->restrictedUnits;

		if ((resUnits.find(unitName) != resUnits.end()) &&
			(resUnits.find(unitName)->second == 0)) {
			continue;
		}
		*/

		// Seems ok, load it
		unitDefs[id].valid = false;
		unitDefs[id].name = unitName;
		unitDefs[id].id = id;
		unitDefs[id].buildangle = 0;
		unitDefs[id].buildPic   = NULL;
		unitDefs[id].decoyDef   = NULL;
		unitDefs[id].techLevel  = -1;
		unitDefs[id].collisionVolume = NULL;
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
	FindStartUnits();
	ProcessDecoys();
	AssignTechLevels();
}


CUnitDefHandler::~CUnitDefHandler(void)
{
	// delete any eventual yardmaps
	for (int i = 1; i <= numUnitDefs; i++) {
		UnitDef& ud = unitDefs[i];
		for (int u = 0; u < 4; u++) {
			delete[] ud.yardmaps[u];
		}

		if (ud.buildPic) {
			if (ud.buildPic->textureOwner) {
				glDeleteTextures(1, &unitDefs[i].buildPic->textureID);
			}
			delete ud.buildPic;
			ud.buildPic = NULL;
		}

		delete ud.collisionVolume;
		ud.collisionVolume = NULL;
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


void CUnitDefHandler::FindStartUnits()
{
	for (unsigned int i = 0; i < sideParser.GetCount(); i++) {
		const std::string& startUnit = sideParser.GetStartUnit(i);
		if (!startUnit.empty()) {
			std::map<std::string, int>::iterator it = unitID.find(startUnit);
			if (it != unitID.end()) {
				startUnitIDs.insert(it->second);
			}
		}
	}
}


void CUnitDefHandler::ParseTAUnit(const LuaTable& udTable, const string& unitName, int id)
{
	UnitDef& ud = unitDefs[id];

	// allocate and fill ud->unitImage
	ud.buildPicName = udTable.GetString("buildPic", "");

	ud.humanName = udTable.GetString("name", "");

	if (ud.humanName.empty()) {
		const string errmsg = "missing 'name' parameter for the " + unitName + " unitdef";
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

	ud.losRadius = udTable.GetFloat("sightDistance", 0.0f) * modInfo.losMul / (SQUARE_SIZE * (1 << modInfo.losMipLevel));
	ud.airLosRadius = udTable.GetFloat("airSightDistance", -1.0f);
	if (ud.airLosRadius == -1.0f) {
		ud.airLosRadius=udTable.GetFloat("sightDistance", 0.0f) * modInfo.airLosMul * 1.5f / (SQUARE_SIZE * (1 << modInfo.airMipLevel));
	} else {
		ud.airLosRadius = ud.airLosRadius * modInfo.airLosMul / (SQUARE_SIZE * (1 << modInfo.airMipLevel));
	}

	ud.canSubmerge = udTable.GetBool("canSubmerge", false);
	ud.canfly      = udTable.GetBool("canFly",      false);
	ud.canmove     = udTable.GetBool("canMove",     false);
	ud.reclaimable = udTable.GetBool("reclaimable", true);
	ud.capturable  = udTable.GetBool("capturable",  true);
	ud.repairable  = udTable.GetBool("repairable",  true);
	ud.canAttack   = udTable.GetBool("canAttack",   true);
	ud.canFight    = udTable.GetBool("canFight",    true);
	ud.canPatrol   = udTable.GetBool("canPatrol",   true);
	ud.canGuard    = udTable.GetBool("canGuard",    true);
	ud.canRepeat   = udTable.GetBool("canRepeat",   true);

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
	if ((ud.waterline >= 5.0f) && ud.canmove) {
		// make subs travel at somewhat larger depths
		// to reduce vulnerability to surface weapons
		ud.waterline += 10.0f;
	}

	ud.canSelfD = udTable.GetBool("canSelfDestruct", true);
	ud.selfDCountdown = udTable.GetInt("selfDestructCountdown", 5);

	ud.speed    = udTable.GetFloat("maxVelocity",  0.0f) * 30.0f;
	ud.maxAcc   = fabs(udTable.GetFloat("acceleration", 0.5f)); // no negative values
	ud.maxDec   = fabs(udTable.GetFloat("brakeRate",    3.0f * ud.maxAcc)) * (ud.canfly? 0.1f: 1.0f); // no negative values

	ud.turnRate    = udTable.GetFloat("turnRate",     0.0f);
	ud.turnInPlace = udTable.GetBool( "turnInPlace",  true);
	ud.turnInPlaceDistance = udTable.GetFloat("turnInPlaceDistance", 350.f);
	ud.turnInPlaceSpeedLimit = udTable.GetFloat("turnInPlaceSpeedLimit", 15.f);

	bool noAutoFire  = udTable.GetBool("noAutoFire",  false);
	ud.canFireControl = udTable.GetBool("canFireControl", !noAutoFire);
	ud.fireState = udTable.GetInt("fireState", ud.canFireControl ? -1 : 2);
	ud.fireState = std::min(ud.fireState,2);
	ud.moveState = udTable.GetInt("moveState", (ud.canmove && ud.speed>0.0f)  ? -1 : 1);
	ud.moveState = std::min(ud.moveState,2);

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
	ud.sonarStealth   = udTable.GetBool("sonarStealth",       false);
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
	ud.bankingAllowed = udTable.GetBool("bankingAllowed", true);

	ud.transportSize     = udTable.GetInt("transportSize",      0);
	ud.minTransportSize  = udTable.GetInt("minTransportSize",   0);
	ud.transportCapacity = udTable.GetInt("transportCapacity",  0);
	ud.isFirePlatform    = udTable.GetBool("isFirePlatform",    false);
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
	ud.wingDrag = std::min(1.0f, std::max(0.0f, ud.wingDrag));
	ud.wingAngle    = udTable.GetFloat("wingAngle",    0.08f);  // angle between front and the wing plane
	ud.frontToSpeed = udTable.GetFloat("frontToSpeed", 0.1f);   // fudge factor for lining up speed and front of plane
	ud.speedToFront = udTable.GetFloat("speedToFront", 0.07f);  // fudge factor for lining up speed and front of plane
	ud.myGravity    = udTable.GetFloat("myGravity",    0.4f);   // planes are slower than real airplanes so lower gravity to compensate
	ud.crashDrag    = udTable.GetFloat("crashDrag",0.005f);     // drag used when crashing
	ud.crashDrag = std::min(1.0f, std::max(0.0f, ud.crashDrag));

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

	string lname = StringToLower(ud.name);

	if (gameSetup->restrictedUnits.find(lname) != gameSetup->restrictedUnits.end()) {
		ud.maxThisUnit = std::min(ud.maxThisUnit, gameSetup->restrictedUnits.find(lname)->second);
	}

	ud.categoryString = udTable.GetString("category", "");
	ud.category = CCategoryHandler::Instance()->GetCategories(udTable.GetString("category", ""));
	ud.noChaseCategory = CCategoryHandler::Instance()->GetCategories(udTable.GetString("noChaseCategory", ""));
//	logOutput.Print("Unit %s has cat %i",ud.humanName.c_str(),ud.category);

	const string iconName = udTable.GetString("iconType", "default");
	ud.iconType = iconHandler->GetIcon(iconName);

	ud.shieldWeaponDef    = NULL;
	ud.stockpileWeaponDef = NULL;

	ud.maxWeaponRange = 0.0f;
	ud.maxCoverage = 0.0f;

	const WeaponDef* noWeaponDef = weaponDefHandler->GetWeapon("NOWEAPON");

	LuaTable weaponsTable = udTable.SubTable("weapons");
	for (int w = 0; w < COB_MaxWeapons; w++) {
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
				ud.weapons.push_back(UnitDef::UnitDefWeapon());
				ud.weapons.back().def = noWeaponDef;
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
		if (wd->interceptor && wd->coverageRange > ud.maxCoverage) {
			ud.maxCoverage = wd->coverageRange;
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

	ud.extractRange = 0.0f;
	ud.extractSquare = udTable.GetBool("extractSquare", false);

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
			ud.upright = true;
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
		ud.drag = std::min(1.0f, std::max(0.0f, ud.drag));
	} else {
		//shouldn't be needed since drag is only used in CAirMoveType anyway,
		//and aircraft without acceleration or speed aren't common :)
		//initializing it anyway just for safety
		ud.drag = 0.005f;
	}

	std::string objectname = udTable.GetString("objectName", "");
	if (objectname.find(".") == std::string::npos) {
		objectname += ".3do";
	}
	ud.modelDef.modelpath = "objects3d/" + objectname;
	ud.modelDef.modelname = objectname;

	ud.scriptName = udTable.GetString("script", unitName + ".cob");
	ud.scriptPath = "scripts/" + ud.scriptName;

	ud.wreckName = udTable.GetString("corpse", "");
	ud.deathExplosion = udTable.GetString("explodeAs", "");
	ud.selfDExplosion = udTable.GetString("selfDestructAs", "");

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
	ud.zsize = udTable.GetInt("footprintZ", 1) * 2;

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


	ud.modelCenterOffset = udTable.GetFloat3("modelCenterOffset", ZeroVector);

	ud.collisionVolumeTypeStr   = udTable.GetString("collisionVolumeType", "");
	ud.collisionVolumeScales    = udTable.GetFloat3("collisionVolumeScales", ZeroVector);
	ud.collisionVolumeOffsets   = udTable.GetFloat3("collisionVolumeOffsets", ZeroVector);
	ud.collisionVolumeTest      = udTable.GetInt("collisionVolumeTest", COLVOL_TEST_DISC);
	ud.usePieceCollisionVolumes = udTable.GetBool("usePieceCollisionVolumes", false);

	// initialize the (per-unitdef) collision-volume
	// all CUnit instances hold a copy of this object
	ud.collisionVolume = new CollisionVolume(
		ud.collisionVolumeTypeStr,
		ud.collisionVolumeScales,
		ud.collisionVolumeOffsets,
		ud.collisionVolumeTest
	);

	if (ud.usePieceCollisionVolumes) {
		ud.collisionVolume->Disable();
	}


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
		LoadSound(gsound, fileName, 1.0f);
		return;
	}

	LuaTable sndTable = soundsTable.SubTable(soundName);
	for (int i = 1; true; i++) {
		LuaTable sndFileTable = sndTable.SubTable(i);
		if (sndFileTable.IsValid()) {
			fileName = sndFileTable.GetString("file", "");
			if (!fileName.empty()) {
				const float volume = sndFileTable.GetFloat("volume", 1.0f);
				if (volume > 0.0f) {
					LoadSound(gsound, fileName, volume);
				}
			}
		} else {
			fileName = sndTable.GetString(i, "");
			if (fileName.empty()) {
				break;
			}
			LoadSound(gsound, fileName, 1.0f);
		}
	}
}


void CUnitDefHandler::LoadSound(GuiSoundSet& gsound,
                                const string& fileName, const float volume)
{
	if (!sound->HasSoundItem(fileName))
	{
		string soundFile = "sounds/" + fileName;

		if (soundFile.find(".wav") == string::npos) {
			// .wav extension missing, add it
			soundFile += ".wav";
		}
		CFileHandler fh(soundFile);
		if (fh.FileExists()) {
			// we have a valid soundfile: store name, ID, and default volume
			const int id = sound->GetSoundId(soundFile);
			GuiSoundSet::Data soundData(fileName, id, volume);
			gsound.sounds.push_back(soundData);
		}
	}
	else
	{
		const int id = sound->GetSoundId(fileName);
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
		def->yardmaps[u] = new unsigned char[def->xsize * def->zsize];

	unsigned char *originalMap = new unsigned char[def->xsize * def->zsize / 4];		//TAS resolution is double of TA resolution.
	memset(originalMap, 255, def->xsize * def->zsize / 4);

	if(!yardmapStr.empty()){
		std::string::iterator si = yardmapStr.begin();
		int x, y;
		for(y = 0; y < def->zsize / 2; y++) {
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
	for(int y = 0; y < def->zsize; y++)
		for(int x = 0; x < def->xsize; x++){
			def->yardmaps[0][x + y*def->xsize] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[1][(def->zsize*def->xsize)-(def->zsize*(x+1)-(y+1)+1)] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[2][(def->zsize*def->xsize)-(x + y*def->xsize+1)] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[3][def->zsize*(x+1)-(y+1)] = originalMap[x/2 + y/2*def->xsize/2];
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


unsigned int CUnitDefHandler::GetUnitDefImage(const UnitDef* unitDef)
{
	if (unitDef->buildPic != NULL) {
		return (unitDef->buildPic->textureID);
	}
	SetUnitDefImage(unitDef, unitDef->buildPicName);
	return unitDef->buildPic->textureID;
}


void CUnitDefHandler::SetUnitDefImage(const UnitDef* unitDef,
                                      const std::string& texName)
{
	if (unitDef->buildPic == NULL) {
		unitDef->buildPic = new UnitDefImage;
	} else if (unitDef->buildPic->textureOwner) {
		glDeleteTextures(1, &unitDef->buildPic->textureID);
	}

	CBitmap bitmap;

	if (!texName.empty()) {
		bitmap.Load("unitpics/" + texName);
	}
	else {
		if (!LoadBuildPic("unitpics/" + unitDef->name + ".dds", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitDef->name + ".png", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitDef->name + ".pcx", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitDef->name + ".bmp", bitmap)) {
			bitmap.Alloc(1, 1); // last resort
		}
	}

	const unsigned int texID = bitmap.CreateTexture(false);

	UnitDefImage* unitImage = unitDef->buildPic;
	unitImage->textureID = texID;
	unitImage->textureOwner = true;
	unitImage->imageSizeX = bitmap.xsize;
	unitImage->imageSizeY = bitmap.ysize;
}


void CUnitDefHandler::SetUnitDefImage(const UnitDef* unitDef,
                                      unsigned int texID, int xsize, int ysize)
{
	if (unitDef->buildPic == NULL) {
		unitDef->buildPic = new UnitDefImage;
	} else if (unitDef->buildPic->textureOwner) {
		glDeleteTextures(1, &unitDef->buildPic->textureID);
	}

	UnitDefImage* unitImage = unitDef->buildPic;
	unitImage->textureID = texID;
	unitImage->textureOwner = false;
	unitImage->imageSizeX = xsize;
	unitImage->imageSizeY = ysize;
}


void CUnitDefHandler::AssignTechLevels()
{
	set<int>::iterator it;
	for (it = startUnitIDs.begin(); it != startUnitIDs.end(); ++it) {
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
	std::multimap<int, std::string> entries;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitID.begin(); uit != unitID.end(); uit++) {
		const string& unitName = uit->first;
		const UnitDef* ud = GetUnitByName(unitName);
		if (ud) {
			char buf[256];
			SNPRINTF(buf, sizeof(buf), " %3i:  %-15s  // %s :: %s\n",
							 ud->techLevel, unitName.c_str(),
							 ud->humanName.c_str(), ud->tooltip.c_str());
			entries.insert(std::pair<int, string>(ud->techLevel, buf));
		}
	}
	int prevLevel = -2;
	std::multimap<int, std::string>::iterator eit;
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






UnitDef::~UnitDef() {
	for (std::vector<CExplosionGenerator*>::iterator it = sfxExplGens.begin(); it != sfxExplGens.end(); ++it) {
		delete *it;
	}
}

S3DModel* UnitDef::LoadModel() const {
	// not exactly kosher, but...
	UnitDef* udef = const_cast<UnitDef*>(this);

	if (udef->modelDef.model == NULL) {
		udef->modelDef.model = modelParser->Load3DModel(udef->modelDef.modelpath, udef->modelCenterOffset);
	}

	return (udef->modelDef.model);
}
