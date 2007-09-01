#include "StdAfx.h"
#include "UnitDefHandler.h"
#include "Rendering/GL/myGL.h"
#include "TdfParser.h"
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <locale>
#include <cctype>
#include "FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "LogOutput.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sound.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "UnitDef.h"
#include "Map/ReadMap.h"
#include "Game/GameSetup.h"
#include "Platform/ConfigHandler.h"
#include "Game/Team.h"
#include "Sim/Misc/SensorHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "mmgr.h"
#include <sys/time.h>//FIXME
#include <time.h>//FIXME

CR_BIND(UnitDef, );

const char YARDMAP_CHAR = 'c';		//Need to be low case.

CUnitDefHandler* unitDefHandler;

CUnitDefHandler::CUnitDefHandler(void) : noCost(false)
{
	struct timeval tv;//FIXME
	gettimeofday(&tv, NULL);//FIXME
	uint64_t startTime = (tv.tv_sec * 1000000) + tv.tv_usec;//FIXME

	PrintLoadMsg("Loading units and weapons");

	weaponDefHandler = SAFE_NEW CWeaponDefHandler();

	LuaParser luaParser("gamedata/unitdefs.lua",
	                    SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!luaParser.Execute()) {
		throw content_error(luaParser.GetErrorLog());
	}

	gettimeofday(&tv, NULL);//FIXME
	uint64_t execTime = (tv.tv_sec * 1000000) + tv.tv_usec;//FIXME
	printf("UNITDEFHANDLER execute time = %.3f\n", double(execTime - startTime) * 1.0e-6);//FIXME

	LuaTable rootTable = luaParser.GetRoot();
	if (!rootTable.IsValid()) {
		throw content_error("Error executing gamedata/unitdefs.lua");
	}

	vector<string> unitDefNames;
	rootTable.GetKeys(unitDefNames);

	numUnitDefs = unitDefNames.size();
	if (gameSetup) {
		numUnitDefs -= gameSetup->restrictedUnits.size();
	}

	// This could be wasteful if there is a lot of restricted units, but that is not that likely
	unitDefs = SAFE_NEW UnitDef[numUnitDefs + 1];

	unsigned int id = 1;  // Start at unit id 1

	for (unsigned int a = 0; a < unitDefNames.size(); ++a) {

		const string unitName = unitDefNames[a];

		// Restrictions may tell us not to use this unit at all
		if (gameSetup) {
			const std::map<std::string, int>& resUnits = gameSetup->restrictedUnits;
		  if ((resUnits.find(unitName) != resUnits.end()) &&
			    (resUnits.find(unitName)->second == 0)) {
				continue;
			}
		}

		// Seems ok, load it
		unitDefs[id].valid = false;
		unitDefs[id].name = unitName;
		unitDefs[id].id = id;
		unitDefs[id].buildangle = 0;
		unitDefs[id].unitimage  = 0;
		unitDefs[id].imageSizeX = -1;
		unitDefs[id].imageSizeY = -1;
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

	FindTABuildOpt();

	ProcessDecoys();

	AssignTechLevels();

	gettimeofday(&tv, NULL);//FIXME
	uint64_t endTime = (tv.tv_sec * 1000000) + tv.tv_usec;//FIXME
	printf("UNITDEFHANDLER load time = %.3f\n", double(endTime - execTime) * 1.0e-6);//FIXME
	printf("UNITDEFHANDLER full time = %.3f\n", double(endTime - startTime) * 1.0e-6);//FIXME
}


CUnitDefHandler::~CUnitDefHandler(void)
{
	//Deleting eventual yeardmaps.
	for(int i = 1; i <= numUnitDefs; i++){
		for (int u = 0; u < 4; u++)
			delete[] unitDefs[i].yardmaps[u];

		if (unitDefs[i].unitimage) {
			glDeleteTextures(1,&unitDefs[i].unitimage);
			unitDefs[i].unitimage = 0;
		}
	}
	delete[] unitDefs;
	delete weaponDefHandler;
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
			decoyMap[real].insert(fake);
		}
	}
	decoyNameMap.clear();
}


void CUnitDefHandler::FindTABuildOpt()
{
	TdfParser tdfparser;
	tdfparser.LoadFile("gamedata/SIDEDATA.TDF");

	// get the commander IDs while SIDEDATA.TDF is open
	commanderIDs.clear();
	std::vector<std::string> sides = tdfparser.GetSectionList("");
	for (unsigned int i=0; i<sides.size(); i++){
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

	std::vector<std::string> sideunits = tdfparser.GetSectionList("CANBUILD");
	for (unsigned int i=0; i<sideunits.size(); i++) {
		std::map<std::string, std::string>::const_iterator it;

		UnitDef *builder=NULL;
		StringToLowerInPlace(sideunits[i]);
		std::map<std::string, int>::iterator it1 = unitID.find(sideunits[i]);
		if (it1!= unitID.end()) {
			builder = &unitDefs[it1->second];
		}

		if (builder) {
			const std::map<std::string, std::string>& buildoptlist = tdfparser.GetAllValues("CANBUILD\\" + sideunits[i]);
			for (it=buildoptlist.begin(); it!=buildoptlist.end(); it++) {
				std::string opt = StringToLower(it->second);

				if(unitID.find(opt)!= unitID.end()){
					int num = atoi(it->first.substr(8).c_str());
					builder->buildOptions[num] = opt;
				}
			}
		}
	}

	std::vector<std::string> files = CFileHandler::FindFiles("download/", "*.tdf");
	for (unsigned int i=0; i<files.size(); i++) {
		TdfParser dparser(files[i]);

		std::vector<std::string> sectionlist = dparser.GetSectionList("");

		for (unsigned int j = 0; j < sectionlist.size(); j++) {
			const string& build = sectionlist[j];
			UnitDef* builder = NULL;
			std::string un1 = StringToLower(dparser.SGetValueDef("", build + "\\UNITMENU"));
			std::map<std::string, int>::iterator it1 = unitID.find(un1);
			if (it1!= unitID.end()) {
				builder = &unitDefs[it1->second];
			}

			if (builder) {
				string un2 = StringToLower(dparser.SGetValueDef("", build + "\\UNITNAME"));

				if (unitID.find(un2) == unitID.end()) {
					logOutput.Print("couldnt find unit %s", un2.c_str());
				} else {
					int menu = atoi(dparser.SGetValueDef("", build + "\\MENU").c_str());
					int button = atoi(dparser.SGetValueDef("", build + "\\BUTTON").c_str());
					int num = ((menu - 2) * 6) + button + 1;
					builder->buildOptions[num] = un2;
				}
			}
		}

	}
}


void CUnitDefHandler::ParseTAUnit(const LuaTable& udTable, const string& unitName, int id)
{
	UnitDef& ud = unitDefs[id];

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
	ud.tooltip   = udTable.GetString("description", ud.name);

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
	ud.noAutoFire  = udTable.GetBool("noAutoFire",  false);
	ud.reclaimable = udTable.GetBool("reclaimable", true);
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

	ud.upright = udTable.GetBool("upright", false);
	ud.onoffable = udTable.GetBool("onoffable", false);

	ud.maxSlope = udTable.GetFloat("maxSlope", 0.0f);
	ud.maxHeightDif = 40 * tan(ud.maxSlope * (PI / 180));
	ud.maxSlope = cos(ud.maxSlope * (PI / 180));
	ud.minWaterDepth = udTable.GetFloat("minWaterDepth", -10e6);
	ud.maxWaterDepth = udTable.GetFloat("maxWaterDepth", +10e6);
	ud.minCollisionSpeed = udTable.GetFloat("minCollisionSpeed", 1.0f);
	ud.slideTolerance = udTable.GetFloat("slideTolerance", 0.0f); // disabled

	ud.waterline = udTable.GetFloat("waterline", 0.0f);
	if ((ud.waterline > 8.0f) && ud.canmove) {
		ud.waterline += 5.0f; // make subs travel at somewhat larger depths to reduce vulnerability to surface weapons
	}

	ud.selfDCountdown = udTable.GetInt("selfDestructCountdown", 5);

	ud.speed    = udTable.GetFloat("maxVelocity",  0.0f) * 30.0f;
	ud.maxAcc   = udTable.GetFloat("acceleration", 0.5f);
	ud.maxDec   = udTable.GetFloat("brakeRate",    0.5f) * 0.1f;
	ud.turnRate = udTable.GetFloat("turnRate",     0.0f);

	ud.buildDistance = udTable.GetFloat("buildDistance", 128.0f);
	ud.buildDistance = std::max(128.0f, ud.buildDistance);
	ud.buildSpeed = udTable.GetFloat("workerTime", 0.0f);

	ud.repairSpeed    = udTable.GetFloat("repairSpeed",    ud.buildSpeed);
	ud.reclaimSpeed   = udTable.GetFloat("reclaimSpeed",   ud.buildSpeed);
	ud.resurrectSpeed = udTable.GetFloat("resurrectSpeed", ud.buildSpeed);
	ud.captureSpeed   = udTable.GetFloat("captureSpeed",   ud.buildSpeed);
	ud.terraformSpeed = udTable.GetFloat("terraformSpeed", ud.buildSpeed);

	ud.armoredMultiple = udTable.GetFloat("damageModifier", 1.0f);
	ud.armorType=damageArrayHandler->GetTypeFromName(ud.name);
//	logOutput.Print("unit %s has armor %i",ud.name.c_str(),ud.armorType);

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

	ud.startCloaked = udTable.GetBool("init_cloaked", false);
	ud.decloakDistance = udTable.GetFloat("minCloakDistance", -1.0f);
	ud.decloakOnFire = udTable.GetBool("decloakOnFire", true);

	ud.highTrajectoryType = udTable.GetInt("highTrajectory", 0);

	ud.canKamikaze = udTable.GetBool("kamikaze", false);
	ud.kamikazeDist = udTable.GetFloat("kamikazeDistance", -25.0f) + 25.0f; //we count 3d distance while ta count 2d distance so increase slightly

	ud.showNanoFrame = udTable.GetBool("showNanoFrame", true);
	ud.showNanoSpray = udTable.GetBool("showNanoSpray", true);
	ud.nanoColor = udTable.GetFloat3("nanoColor", float3(0.2f,0.7f,0.2f));

	ud.canhover = udTable.GetBool("canHover", false);

	ud.floater = (ud.waterline != 0.0f);
	if (udTable.KeyExists("floater")) {
		ud.floater = udTable.GetBool("floater", false);
	}

	ud.builder = udTable.GetBool("builder", false);
	if (ud.builder && !ud.buildSpeed) { // core anti is flagged as builder for some reason
		ud.builder = false;
	}

	ud.airStrafe     = udTable.GetBool("airStrafe", true);
	ud.hoverAttack   = udTable.GetBool("hoverAttack", false);
	ud.wantedHeight  = udTable.GetFloat("cruiseAlt", 0.0f);
	ud.dlHoverFactor = udTable.GetFloat("airHoverFactor", -1.0f);

	ud.transportSize     = udTable.GetInt("transportSize",     0);
	ud.transportCapacity = udTable.GetInt("transportCapacity", 0);
	ud.isfireplatform    = udTable.GetBool("isFirePlatform",   false);
	ud.isAirBase         = udTable.GetBool("isAirBase",        false);
	ud.loadingRadius     = udTable.GetFloat("loadingRadius",   220.0f);
	ud.transportMass     = udTable.GetFloat("transportMass",   100000.0f);
	ud.holdSteady        = udTable.GetBool("holdSteady",       true);
	ud.releaseHeld       = udTable.GetBool("releaseHeld",      false);
	ud.transportByEnemy  = udTable.GetBool("transportByEnemy", true);

	ud.wingDrag     = udTable.GetFloat("wingDrag",     0.07f);  // drag caused by wings
	ud.wingAngle    = udTable.GetFloat("wingAngle",    0.08f);  // angle between front and the wing plane
	ud.drag         = udTable.GetFloat("drag",         0.005f); // how fast the aircraft loses speed (see also below)
	ud.frontToSpeed = udTable.GetFloat("frontToSpeed", 0.1f);   // fudge factor for lining up speed and front of plane
	ud.speedToFront = udTable.GetFloat("speedToFront", 0.07f);  // fudge factor for lining up speed and front of plane
	ud.myGravity    = udTable.GetFloat("myGravity",    0.4f);   // planes are slower than real airplanes so lower gravity to compensate

	ud.maxBank = udTable.GetFloat("maxBank", 0.8f);         // max roll
	ud.maxPitch = udTable.GetFloat("maxPitch", 0.45f);      // max pitch this plane tries to keep
	ud.turnRadius = udTable.GetFloat("turnRadius", 500.0f); // hint to the ai about how large turn radius this plane needs

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

		const float angleDif = cos(wTable.GetFloat("maxAngleDif", 360.0f) * (PI / 360.0));

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

	if(ud.extractsMetal) {
		ud.extractRange = readmap->extractorRadius;
		ud.type = "MetalExtractor";
	}
	else if (ud.transportCapacity) {
		ud.type = "Transport";
	}
	else if (ud.builder) {
		if (TEDClass!="PLANT") {
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

	ud.movedata=0;
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
	}

	std::string objectname = udTable.GetString("objectName", "");
	ud.model.modelpath = "objects3d/" + objectname;
	ud.model.modelname = objectname;

	ud.wreckName = udTable.GetString("corpse", "");
	ud.deathExplosion = udTable.GetString("explodeAs", "");
	ud.selfDExplosion = udTable.GetString("selfDestructAs", "");

	ud.buildpicname = udTable.GetString("buildPic", "");

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

	ud.canDropFlare = udTable.GetBool("canDropFlare", false);
	ud.flareReloadTime = udTable.GetFloat("flareReload", 5.0f);
	ud.flareEfficiency = udTable.GetFloat("flareEfficiency", 0.5f);
	ud.flareDelay = udTable.GetFloat("flareDelay", 0.3f);
	ud.flareDropVector = udTable.GetFloat3("flareDropVector", ZeroVector);
	ud.flareTime       = udTable.GetInt("flareTime", 3) * 30;
	ud.flareSalvoSize  = udTable.GetInt("flareSalvoSize", 4);
	ud.flareSalvoDelay = udTable.GetInt("flareSalvoDelay", 0) * 30;

	ud.smoothAnim = udTable.GetBool("smoothAnim", false);
	ud.canLoopbackAttack = udTable.GetBool("canLoopbackAttack", false);
	ud.levelGround = udTable.GetBool("levelGround", true);

	// aircraft collision sizes default to half their visual size, to
	// make them less likely to collide or get hit by nontracking weapons
	const float defScale = ud.canfly ? 0.5f : 1.0f;
	ud.collisionSphereScale = udTable.GetFloat("collisionSphereScale", defScale);

	ud.collisionSphereOffset = udTable.GetFloat3("collisionSphereOffset", ZeroVector);
	if (ud.collisionSphereOffset != ZeroVector) {
		ud.useCSOffset = true;
	} else {
		ud.useCSOffset = false;
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
		LoadSound(gsound, fileName);
		return;
	}

	LuaTable sndTable = soundsTable.SubTable(soundName);
	for (int i = 1; true; i++) {
		fileName = sndTable.GetString(i, "");
		if (fileName.empty()) {
			break;
		}
		LoadSound(gsound, fileName);
	}
}


void CUnitDefHandler::LoadSound(GuiSoundSet& gsound, const string& fileName)
{
	const string soundFile = "sounds/" + fileName + ".wav";
	CFileHandler fh(soundFile);

	if (fh.FileExists()) {
		// we have a valid soundfile: store name, ID, and default volume
		PUSH_CODE_MODE;
		ENTER_UNSYNCED;
		const int id = sound->GetWaveId(soundFile);
		POP_CODE_MODE;

		GuiSoundSet::Data soundData(fileName, id, 5.0f);
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
		unitDefs[id].metalCost = 1;
		unitDefs[id].energyCost = 1;
		unitDefs[id].buildTime = 10;
		unitDefs[id].metalUpkeep = 0;
		unitDefs[id].energyUpkeep = 0;
	}
}


const UnitDef *CUnitDefHandler::GetUnitByName(std::string name)
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


const UnitDef *CUnitDefHandler::GetUnitByID(int id)
{
	if ((id <= 0) || (id > numUnitDefs)) {
		return NULL;
	}
	UnitDef* ud = &unitDefs[id];
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
	memset(originalMap, 1, def->xsize * def->ysize / 4);

	if(!yardmapStr.empty()){
		std::string::iterator si = yardmapStr.begin();
		int x, y;
		for(y = 0; y < def->ysize / 2; y++) {
			for(x = 0; x < def->xsize / 2; x++) {
				if(*si == 'g')
					def->needGeo=true;
				if(*si == YARDMAP_CHAR)
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


unsigned int CUnitDefHandler::GetUnitImage(const UnitDef *unitdef)
{
	if (unitdef->unitimage != 0) {
		return unitdef->unitimage;
	}

	CBitmap bitmap;
	if (!unitdef->buildpicname.empty()) {
		bitmap.Load("unitpics/" + unitdef->buildpicname);
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
	ud.unitimage  = texID;
	ud.imageSizeX = bitmap.xsize;
	ud.imageSizeY = bitmap.ysize;
	POP_CODE_MODE;

	return unitdef->unitimage;
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
	if (!useCSOffset) {
		return modelParser->Load3DO(model.modelpath.c_str(),
		                            collisionSphereScale, team);
	} else {
		return modelParser->Load3DO(model.modelpath.c_str(),
		                            collisionSphereScale, team,
		                            collisionSphereOffset);
	};
}
