#include "StdAfx.h"
// LuaState.cpp: implementation of the CLuaState class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaState.h"

#include <cctype>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "KeyBindings.h"
#include "SimpleParser.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Game/Team.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/ModInfo.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/LogOutput.h"

static string IntToString(int value);
static string BoolToString(bool value);
static string FloatToString(float value);
static string SafeString(const string& text);
static string StringReplace(const string& text,
                            const string& from, const string& to);
static string GetCategoryTableFromBits(unsigned int bits);
static string GetCategoryTableFromString(const string& text);


// FIXME -- use 'em
static const string GameInfoName    = "Game";
static const string UnitDefsName    = "UnitDefs";
static const string WeaponDefsName  = "WeaponDefs";


#ifndef _WIN32
static const string endlStr = "\n";
#else
static const string endlStr = "\r\n";
#endif


/******************************************************************************/

CLuaState& CLuaState::GetSingleton()
{
	static CLuaState singleton;
	if (!singleton.inited) {
		singleton.Init();
	}
	return singleton;
}


CLuaState::CLuaState()
{
	L = NULL;
	inited = false;
}


CLuaState::~CLuaState()
{
	if (L != NULL) {
		lua_close(L);
	}
}


void CLuaState::Init()
{
	L = lua_open();
	if (L != NULL) {
		if (!LoadInfo()) {
			lua_close(L);
			L = NULL;
		}
	}
	inited = true;
}


bool CLuaState::LoadCode(const string& code, const string& debug)
{
	lua_pop(L, lua_gettop(L));

	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		logOutput.Print("ERROR: LuaState: Loading: %s\n",
		                lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	error = lua_pcall(L, 0, 0, 0);
	if (error != 0) {
		logOutput.Print("ERROR: LuaState: Running: %s\n",
		                lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	
	return true;
}


static int LuaWriter(lua_State *L, const void* data, size_t size, void* userData)
{
	printf("Dumping %i bytes\n", size);
	fwrite(data, 1, size, (FILE*)userData);
	return 0;
}


bool CLuaState::LoadInfo()
{
	// load some basic lua libs
	luaopen_base(L);
	luaopen_io(L);
	luaopen_math(L);
	luaopen_string(L);
	luaopen_table(L);
	luaopen_debug(L);

	// flush the return values
	lua_pop(L, lua_gettop(L));

	if (!LoadGameInfo()      ||
	    !LoadWeaponDefInfo() ||
	    !LoadUnitDefInfo()) {
		return false;
	}

	/* FIXME	
	string outfile = "lua_dump_" + modInfo->name;
	FILE* f = fopen(outfile.c_str(), "w");
	if (f != NULL) {
		lua_dump(L, LuaWriter, f);
		fclose(f);
	}
	*/

	return true;
}


/******************************************************************************/

bool CLuaState::LoadGameInfo()
{
	string code = "";

	code += "Game = {}" + endlStr;

	code += "Game.version = "   + SafeString(VERSION_STRING) + endlStr;
	code += "Game.commEnds = "  + BoolToString(!!gs->gameMode) + endlStr;
	const float gravity = -(gs->gravity * GAME_SPEED * GAME_SPEED);
	code += "Game.gravity = "   + FloatToString(gravity) + endlStr;
	code += "Game.tidal = "     + FloatToString(readmap->tidalStrength) + endlStr;
	code += "Game.windMin = "   + FloatToString(wind.minWind) + endlStr;
	code += "Game.windMax = "   + FloatToString(wind.maxWind) + endlStr;
	code += "Game.mapX = "      + IntToString(readmap->width / 64) + endlStr;
	code += "Game.mapY = "      + IntToString(readmap->height / 64) + endlStr;
	code += "Game.mapName = \"" + readmap->mapName + "\"" + endlStr;
	code += "Game.modName = \"" + modInfo->name + "\"" + endlStr;
	
	const bool limitDGun      = gameSetup ? gameSetup->limitDgun      : false;
	const bool diminishingMMs = gameSetup ? gameSetup->diminishingMMs : false;
	code += "Game.limitDGun = "        + BoolToString(limitDGun) + endlStr;
	code += "Game.diminishingMetal = " + BoolToString(diminishingMMs) + endlStr;
	
	code += "Game.springCategories = " + GetCategoryTableFromBits(~0) + endlStr;

	if (keyBindings->GetDebug() > 1) {
		logOutput.Print(code);
	}

	if (!LoadCode(code, "Game[]")) {
		return false;
	}
	
	return true;	
}


/******************************************************************************/

bool CLuaState::LoadUnitDefInfo()
{
	bool success = true;
	string code = endlStr;

	code += "UnitDefs = {}" + endlStr;

	LoadCode(code, "UnitDefs[]");
	if (keyBindings->GetDebug() > 1) {
		logOutput.Print(code);
	}
	code.clear();
	
	const std::map<std::string, int>& unitMap = unitDefHandler->unitID;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitMap.begin(); uit != unitMap.end(); uit++) {
		const UnitDef* udPtr = unitDefHandler->GetUnitByID(uit->second);
		if (udPtr != NULL) {
			const string def = "UnitDefs[" + IntToString(udPtr->id) + "]";
			code += def + " = {}" + endlStr;
		}
	}

	for (uit = unitMap.begin(); uit != unitMap.end(); uit++) {
		const UnitDef* udPtr = unitDefHandler->GetUnitByID(uit->second);
		if (udPtr == NULL) {
			continue;
		}
		const UnitDef& ud = *udPtr;

		const string def = "UnitDefs[" + IntToString(uit->second) + "]";

		code += endlStr;

#define ADD_INT(name, value)    \
	unitDefParams.insert(#name);  \
	code += def + "." #name " = " + IntToString(value) + endlStr
#define ADD_BOOL(name, value)   \
	unitDefParams.insert(#name);  \
	code += def + "." #name " = " + BoolToString(value) + endlStr
#define ADD_FLOAT(name, value)  \
	unitDefParams.insert(#name);  \
	code += def + "." #name " = " + FloatToString(value) + endlStr
#define ADD_STRING(name, value) \
	unitDefParams.insert(#name);  \
	code += def + "." #name " = " + SafeString(value) + endlStr

		ADD_STRING(name,      ud.name);
		ADD_STRING(humanName, ud.humanName);
		ADD_STRING(filename,  ud.filename);

		ADD_INT(techLevel, ud.techLevel);
		
		ADD_STRING(TEDClass,       ud.TEDClassString);
		ADD_STRING(iconType,       ud.iconType);

		code += def + ".modCategories = "
		            + GetCategoryTableFromString(ud.categoryString) + endlStr;
		code += def + ".springCategories = "
		            + GetCategoryTableFromBits(ud.category) + endlStr;

		// damage type IDs are offset by 1
		const int damageID = damageArrayHandler->GetTypeFromName(ud.name);
		if (damageID > 0) {
			ADD_STRING(armorType, damageArrayHandler->typeList[damageID - 1]);
		} else {
			ADD_STRING(armorType, "");
		}

		ADD_STRING(type,           ud.type);
		ADD_BOOL(isBomber,         ud.type == "Bomber");         // CUSTOM
		ADD_BOOL(isFighter,        ud.type == "Fighter");        // CUSTOM
		ADD_BOOL(isBuilder,        ud.type == "Builder");        // CUSTOM
		ADD_BOOL(isFactory,        ud.type == "Factory");        // CUSTOM
		ADD_BOOL(isBuilding,       ud.type == "Building");       // CUSTOM
		ADD_BOOL(isTransport,      ud.type == "Transport");      // CUSTOM
		ADD_BOOL(isGroundUnit,     ud.type == "GroundUnit");     // CUSTOM
		ADD_BOOL(isMetalExtractor, ud.type == "MetalExtractor"); // CUSTOM

		ADD_BOOL(isFeature, ud.isFeature);

		// weapons info		
		float maxRange = 0.0f;
		bool hasShield = false;
		bool canParalyze = false;
		bool canStockpile = false;
		bool canAttackWater = false;

		const int weaponCount = (int)ud.weapons.size();
		ADD_INT(weaponCount, weaponCount); // CUSTOM
		code += def + ".weapons = {}" + endlStr;
		int wCount = 0;
		for (int w = 0; w < weaponCount; w++) {
			const UnitDef::UnitDefWeapon& udw = ud.weapons[w];
			const WeaponDef* weapon = udw.def;
			if (weapon) {
				maxRange = max(maxRange, weapon->range);
				hasShield = hasShield || weapon->isShield;
				canParalyze = canParalyze || weapon->paralyzer;
				canStockpile = canStockpile || weapon->stockpile;
				canAttackWater = canAttackWater || weapon->waterweapon;

				const string wdef = def + ".weapons[" + IntToString(wCount) + "]";
				code += wdef + " = {}" + endlStr;
				code += wdef + ".weaponDef = "
				             + "WeaponDefs[" + IntToString(weapon->id) + "]" + endlStr;
				code += wdef + ".fuelUsage = "
				             + FloatToString(udw.fuelUsage) + endlStr;
				code += wdef + ".badTargets = "
				             + GetCategoryTableFromBits(udw.badTargetCat) + endlStr;
				code += wdef + ".onlyTargets = "
				             + GetCategoryTableFromBits(udw.onlyTargetCat) + endlStr;
				wCount++;
			}
		}
		ADD_FLOAT(maxRange,      maxRange);       // CUSTOM
		ADD_BOOL(hasShield,      hasShield);      // CUSTOM
		ADD_BOOL(canParalyze,    canParalyze);    // CUSTOM
		ADD_BOOL(canStockpile,   canStockpile);   // CUSTOM
		ADD_BOOL(canAttackWater, canAttackWater); // CUSTOM

		// FIXME:  canShootAir
		
		// explosion weapons
		const WeaponDef* deathExp = weaponDefHandler->GetWeapon(ud.deathExplosion);
		if (deathExp) {
			code += def + ".deathExplosion = WeaponDefs["
			            + IntToString(deathExp->id) + "]" + endlStr; 
		}
		const WeaponDef* selfdExp = weaponDefHandler->GetWeapon(ud.selfDExplosion);
		if (selfdExp) {
			code += def + ".selfDExplosion = WeaponDefs["
			            + IntToString(selfdExp->id) + "]" + endlStr; 
		}
		
		ADD_BOOL(builder,        ud.builder); 
		ADD_FLOAT(buildSpeed,    ud.buildSpeed);
		ADD_FLOAT(buildDistance, ud.buildDistance);

		ADD_INT(buildOptionsCount, ud.buildOptions.size()); // CUSTOM
		code += def + ".buildOptions = {}" + endlStr;
		std::map<int, std::string>::const_iterator bo_it;
		int bCount = 0;
		for (bo_it = ud.buildOptions.begin(); bo_it != ud.buildOptions.end(); ++bo_it) {
			std::map<std::string, int>::const_iterator um_it =
				unitMap.find(bo_it->second);
			if (um_it != unitMap.end()) {
				code += def + ".buildOptions[" + IntToString(bCount) + "] = "
										+ "UnitDefs[" + IntToString(um_it->second) + "]" + endlStr;
				bCount++;
			}
		}

		ADD_INT(selfDCountdown, ud.selfDCountdown);

		ADD_FLOAT(buildTime,  ud.buildTime); 
		ADD_FLOAT(energyCost, ud.energyCost);
		ADD_FLOAT(metalCost,  ud.metalCost); 

		// resources
		ADD_FLOAT(energyMake,     ud.energyMake);
		ADD_FLOAT(windGenerator,  ud.windGenerator);
		ADD_FLOAT(tidalGenerator, ud.tidalGenerator);
		ADD_FLOAT(extractRange,   ud.extractRange); 
		ADD_FLOAT(extractsMetal,  ud.extractsMetal);
		ADD_FLOAT(metalMake,      ud.metalMake);
		ADD_FLOAT(makesMetal,     ud.makesMetal);
		ADD_FLOAT(energyStorage,  ud.energyStorage); 
		ADD_FLOAT(metalStorage,   ud.metalStorage);  
		ADD_FLOAT(energyUpkeep,   ud.energyUpkeep);
		ADD_FLOAT(metalUpkeep,    ud.metalUpkeep);

		ADD_BOOL(needGeo,         ud.needGeo);
		ADD_BOOL(isMetalMaker,    ud.isMetalMaker);

		// FIXME -- is this right?		
		const float basicEnergy = (ud.energyMake - ud.energyUpkeep);
		const float tidalEnergy = (ud.tidalGenerator * readmap->tidalStrength);
		float windEnergy = 0.0f;
		if (ud.windGenerator > 0.0f) {
			windEnergy = (0.25f * (wind.minWind + wind.maxWind));
		}
		ADD_FLOAT(totalEnergyOut, basicEnergy + tidalEnergy + windEnergy); // CUSTOM

		ADD_FLOAT(autoHeal,     ud.autoHeal);
		ADD_FLOAT(idleAutoHeal, ud.idleAutoHeal);
		ADD_INT(idleTime,       ud.idleTime);

		ADD_FLOAT(power,  ud.power);
		ADD_FLOAT(health, ud.health);

		ADD_FLOAT(speed,    ud.speed);
		ADD_FLOAT(turnRate, ud.turnRate);
		ADD_INT(moveType,   ud.moveType);  
		ADD_BOOL(upright,   ud.upright);  
		
		ADD_BOOL(canAssist,    ud.canAssist);  
		ADD_BOOL(canAttack,    ud.canAttack);  
		ADD_BOOL(canBuild,     ud.canBuild);   
		ADD_BOOL(canCapture,   ud.canCapture);  
		ADD_BOOL(canCloak,     ud.canCloak);
		ADD_BOOL(canDGun,      ud.canDGun);
		ADD_BOOL(canFly,       ud.canfly);
		ADD_BOOL(canGuard,     ud.canGuard);   
		ADD_BOOL(canHover,     ud.canhover);
		ADD_BOOL(canKamikaze,  ud.canKamikaze);
		ADD_BOOL(canMove,      ud.canmove);
		ADD_BOOL(canPatrol,    ud.canPatrol);  
		ADD_BOOL(canReclaim,   ud.canReclaim); 
		ADD_BOOL(canRepair,    ud.canRepair);  
		ADD_BOOL(canRestore,   ud.canRestore); 
		ADD_BOOL(canResurrect, ud.canResurrect);

		ADD_BOOL(noAutoFire,   ud.noAutoFire); 
		ADD_BOOL(reclaimable,  ud.reclaimable);
		ADD_BOOL(onOffable,    ud.onoffable);
		ADD_BOOL(floater,      ud.floater); 

		ADD_BOOL(activateWhenBuilt, ud.activateWhenBuilt);

		ADD_FLOAT(mass, ud.mass);
		ADD_INT(xSize,  ud.xsize); //each size is 8 units
		ADD_INT(ySize,  ud.ysize); //each size is 8 units

		ADD_INT(buildAngle, ud.buildangle);

		// sensors
		ADD_INT(radarRadius,        ud.radarRadius);
		ADD_INT(sonarRadius,        ud.sonarRadius);
		ADD_INT(jammerRadius,       ud.jammerRadius);
		ADD_INT(sonarJamRadius,     ud.sonarJamRadius);
		ADD_INT(seismicRadius,      ud.seismicRadius); 
		ADD_FLOAT(seismicSignature, ud.seismicSignature);

		ADD_BOOL(stealth, ud.stealth);

		ADD_FLOAT(controlRadius, ud.controlRadius);
		ADD_FLOAT(losRadius,     ud.losRadius);
		ADD_FLOAT(airLosRadius,  ud.airLosRadius);
		ADD_FLOAT(losHeight,     ud.losHeight);   

		ADD_FLOAT(maxSlope,      ud.maxSlope);
		ADD_FLOAT(maxHeightDif,  ud.maxHeightDif);
		ADD_FLOAT(minWaterDepth, ud.minWaterDepth);
		ADD_FLOAT(waterline,     ud.waterline);

		ADD_FLOAT(maxWaterDepth,   ud.maxWaterDepth);
		ADD_FLOAT(armoredMultiple, ud.armoredMultiple);

		//aircraft stuff
		ADD_FLOAT(wingDrag,     ud.wingDrag); 
		ADD_FLOAT(wingAngle,    ud.wingAngle);
		ADD_FLOAT(drag,         ud.drag);
		ADD_FLOAT(frontToSpeed, ud.frontToSpeed);
		ADD_FLOAT(speedToFront, ud.speedToFront);
		ADD_FLOAT(myGravity,    ud.myGravity);   

		ADD_FLOAT(maxBank,       ud.maxBank);
		ADD_FLOAT(maxPitch,      ud.maxPitch);
		ADD_FLOAT(turnRadius,    ud.turnRadius);
		ADD_FLOAT(wantedHeight,  ud.wantedHeight);
		ADD_BOOL(hoverAttack,    ud.hoverAttack);  
		ADD_FLOAT(dlHoverFactor, ud.dlHoverFactor); 
		ADD_BOOL(dontLand,       (ud.dlHoverFactor >= 0.0f));

		ADD_FLOAT(maxAcc,      ud.maxAcc);
		ADD_FLOAT(maxDec,      ud.maxDec);
		ADD_FLOAT(maxAileron,  ud.maxAileron);
		ADD_FLOAT(maxElevator, ud.maxElevator);
		ADD_FLOAT(maxRudder,   ud.maxRudder);  

		ADD_FLOAT(maxFuel,         ud.maxFuel);
		ADD_FLOAT(refuelTime,      ud.refuelTime);
		ADD_FLOAT(minAirBasePower, ud.minAirBasePower);

		// transports
		ADD_FLOAT(loadingRadius,   ud.loadingRadius);
		ADD_INT(transportCapacity, ud.transportCapacity);
		ADD_INT(transportSize,     ud.transportSize);
		ADD_BOOL(isAirBase,        ud.isAirBase);   
		ADD_FLOAT(transportMass,   ud.transportMass);

		// cloaking
		ADD_BOOL(startCloaked,     ud.startCloaked);
		ADD_FLOAT(cloakCost,       ud.cloakCost);
		ADD_FLOAT(cloakCostMoving, ud.cloakCostMoving);
		ADD_FLOAT(decloakDistance, ud.decloakDistance);

		// kamikaze
		ADD_FLOAT(kamikazeDist, ud.kamikazeDist);

		ADD_BOOL(targfac,        ud.targfac);
		ADD_BOOL(hideDamage,     ud.hideDamage);
		ADD_BOOL(isCommander,    ud.isCommander);
		ADD_BOOL(showPlayerName, ud.showPlayerName);

		ADD_INT(highTrajectoryType, ud.highTrajectoryType); //0=low, 1=high, 2=choose

		code += def + ".noChaseCategory = "
		            + GetCategoryTableFromBits(ud.noChaseCategory) + endlStr;
		
		ADD_BOOL(canDropFlare, ud.canDropFlare);
		ADD_FLOAT(flareReloadTime, ud.flareReloadTime);
		ADD_FLOAT(flareEfficieny, ud.flareEfficieny); 
		ADD_FLOAT(flareDelay, ud.flareDelay);

		ADD_FLOAT(flareDropVectorX, ud.flareDropVector.x);
		ADD_FLOAT(flareDropVectorY, ud.flareDropVector.y);
		ADD_FLOAT(flareDropVectorZ, ud.flareDropVector.z);
		
		ADD_INT(flareTime, ud.flareTime);
		ADD_INT(flareSalvoSize, ud.flareSalvoSize);
		ADD_INT(flareSalvoDelay, ud.flareSalvoDelay);

		ADD_BOOL(smoothAnim,        ud.smoothAnim);
		ADD_BOOL(canLoopbackAttack, ud.canLoopbackAttack);
		ADD_BOOL(levelGround,       ud.levelGround);

		ADD_BOOL(useBuildingGroundDecal,   ud.useBuildingGroundDecal);
		ADD_INT(buildingDecalType,         ud.buildingDecalType);
		ADD_INT(buildingDecalSizeX,        ud.buildingDecalSizeX);
		ADD_INT(buildingDecalSizeY,        ud.buildingDecalSizeY);
		ADD_FLOAT(buildingDecalDecaySpeed, ud.buildingDecalDecaySpeed);
		ADD_BOOL(isFirePlatform,           ud.isfireplatform);

		// nanospray
		ADD_BOOL(showNanoFrame, ud.showNanoFrame);
		ADD_BOOL(showNanoSpray, ud.showNanoSpray);
		ADD_FLOAT(nanoColorR, ud.nanoColor.x);
		ADD_FLOAT(nanoColorG, ud.nanoColor.y);
		ADD_FLOAT(nanoColorB, ud.nanoColor.z);

		if (keyBindings->GetDebug() > 1) {
			logOutput.Print(code);
		}

		// load the code
		if (!LoadCode(code, def)) {
			success = false;
		}

		code.clear();
	}
	
	return success;
}


/******************************************************************************/

bool CLuaState::LoadWeaponDefInfo()
{
	bool success = true;
	string code = endlStr;

	code += "WeaponDefs = {}" + endlStr;

	LoadCode(code, "WeaponDefs[]");
	if (keyBindings->GetDebug() > 1) {
		logOutput.Print(code);
	}
	code.clear();

	const std::map<std::string, int>& weaponMap = weaponDefHandler->weaponID;
	std::map<std::string, int>::const_iterator wit;
	for (wit = weaponMap.begin(); wit != weaponMap.end(); wit++) {
		const WeaponDef& wd = weaponDefHandler->weaponDefs[wit->second];

		const string def = "WeaponDefs[" + IntToString(wit->second) + "]";

		code += endlStr;
		code += def + " = {}" + endlStr;

#define ADD_W_INT(name, value)    \
	code += def + "." #name " = " + IntToString(value) + endlStr
#define ADD_W_BOOL(name, value)   \
	code += def + "." #name " = " + BoolToString(value) + endlStr
#define ADD_W_FLOAT(name, value)  \
	code += def + "." #name " = " + FloatToString(value) + endlStr
#define ADD_W_STRING(name, value) \
	code += def + "." #name " = " + SafeString(value) + endlStr

		ADD_W_STRING(name, wd.name);

		ADD_W_INT(id, wd.id);

		code += def + ".onlyTargetCategory = "
		            + GetCategoryTableFromBits(wd.onlyTargetCategory) + endlStr;

		ADD_W_STRING(type,              wd.type);
		ADD_W_BOOL(isAircraftBomb,      wd.type == "AircraftBomb");
		ADD_W_BOOL(isStarburstLauncher, wd.type == "StarburstLauncher");
		ADD_W_BOOL(isBeamLaser,         wd.type == "BeamLaser");
		ADD_W_BOOL(isShield,            wd.type == "Shield");
		ADD_W_BOOL(isTorpedoLauncher,   wd.type == "TorpedoLauncher");
		ADD_W_BOOL(isDGun,              wd.type == "DGun");
		ADD_W_BOOL(isLightingCannon,    wd.type == "LightingCannon");
		ADD_W_BOOL(isLaserCannon,       wd.type == "LaserCannon");
		ADD_W_BOOL(isMissileLauncher,   wd.type == "MissileLauncher");
		ADD_W_BOOL(isEmgCannon,         wd.type == "EmgCannon");
		ADD_W_BOOL(isFlame,             wd.type == "flame");
		ADD_W_BOOL(isCannon,            wd.type == "Cannon");
		
		ADD_W_FLOAT(range,           wd.range);
		ADD_W_FLOAT(heightmod,       wd.heightmod);
		ADD_W_FLOAT(accuracy,        wd.accuracy);
		ADD_W_FLOAT(sprayangle,      wd.sprayangle);
		ADD_W_FLOAT(movingAccuracy,  wd.movingAccuracy);
		ADD_W_FLOAT(targetMoveError, wd.targetMoveError);

		code += def + ".damages = {";
		const std::vector<std::string>& typeList = damageArrayHandler->typeList;
		const int damageTypeCount = (int)typeList.size();
		for (int dt = 0; dt < damageTypeCount; dt++) {
			// types IDs are offset by 1
			code += " [" + SafeString(typeList[dt]) + "] = " +
			        FloatToString(wd.damages.damages[dt + 1]);
			if (dt != (damageTypeCount - 1)) {
				code += ",";
			} else {
				code += " ";
			}
		}
		code += "}" + endlStr;
		ADD_W_INT(paralyzeDamageTime, wd.damages.paralyzeDamageTime);
		ADD_W_FLOAT(impulseFactor,    wd.damages.impulseFactor);
		ADD_W_FLOAT(impulseBoost,     wd.damages.impulseBoost);
		ADD_W_FLOAT(craterMult,       wd.damages.craterMult);
		ADD_W_FLOAT(craterBoost,      wd.damages.craterBoost);
		
		ADD_W_BOOL(noSelfDamage,       wd.noSelfDamage);
		ADD_W_FLOAT(areaOfEffect,      wd.areaOfEffect);
		ADD_W_FLOAT(fireStarter,       wd.fireStarter);
		ADD_W_FLOAT(edgeEffectiveness, wd.edgeEffectiveness);
		ADD_W_FLOAT(size,              wd.size);
		ADD_W_FLOAT(sizeGrowth,        wd.sizeGrowth);
		ADD_W_FLOAT(collisionSize,     wd.collisionSize);

		ADD_W_INT(salvoSize,    wd.salvosize);
		ADD_W_FLOAT(salvoDelay, wd.salvodelay);
		ADD_W_FLOAT(reload,     wd.reload);
		ADD_W_FLOAT(beamTime,   wd.beamtime);

		ADD_W_FLOAT(maxAngle, wd.maxAngle);
		ADD_W_FLOAT(restTime, wd.restTime);

		ADD_W_FLOAT(uptime, wd.uptime);

		ADD_W_FLOAT(metalCost,  wd.metalcost);
		ADD_W_FLOAT(energyCost, wd.energycost);
		ADD_W_FLOAT(supplyCost, wd.supplycost);

		ADD_W_BOOL(turret,      wd.turret);
		ADD_W_BOOL(onlyForward, wd.onlyForward);
		ADD_W_BOOL(waterweapon, wd.waterweapon);
		ADD_W_BOOL(tracks,      wd.tracks);
		ADD_W_BOOL(dropped,     wd.dropped);
		ADD_W_BOOL(paralyzer,   wd.paralyzer);

		ADD_W_BOOL(noAutoTarget,   wd.noAutoTarget);  //cant target stuff (for antinuke,dgun)
		ADD_W_BOOL(manualFire,     wd.manualfire);    //use dgun button
		ADD_W_INT(interceptor,     wd.interceptor);   //anti nuke
		ADD_W_INT(targetable,      wd.targetable);    //nuke (can be shot by interceptor)
		ADD_W_BOOL(stockpile,      wd.stockpile);					
		ADD_W_FLOAT(coverageRange, wd.coverageRange); //range of anti nuke

		ADD_W_FLOAT(intensity,      wd.intensity);
		ADD_W_FLOAT(thickness,      wd.thickness);
		ADD_W_FLOAT(laserFlareSize, wd.laserflaresize);
		ADD_W_FLOAT(coreThickness,  wd.corethickness);
		ADD_W_FLOAT(duration,       wd.duration);

		ADD_W_INT(graphicsType, wd.graphicsType);
		ADD_W_BOOL(soundTrigger, wd.soundTrigger);

		ADD_W_BOOL(selfExplode,         wd.selfExplode);
		ADD_W_BOOL(gravityAffected,     wd.gravityAffected);
		ADD_W_BOOL(twoPhase,            wd.twophase);
		ADD_W_BOOL(guided,              wd.guided);
		ADD_W_BOOL(vlaunch,             wd.vlaunch);
		ADD_W_BOOL(selfProp,            wd.selfprop);
		ADD_W_BOOL(noExplode,           wd.noExplode);
		ADD_W_FLOAT(startVelocity,      wd.startvelocity);
		ADD_W_FLOAT(weaponAcceleration, wd.weaponacceleration);
		ADD_W_FLOAT(turnRate,           wd.turnrate);
		ADD_W_FLOAT(maxVelocity,        wd.maxvelocity);

		ADD_W_FLOAT(projectileSpeed, wd.projectilespeed);
		ADD_W_FLOAT(explosionSpeed,  wd.explosionSpeed);

		// missiles
		ADD_W_FLOAT(wobble, wd.wobble);
		ADD_W_FLOAT(trajectoryHeight, wd.trajectoryHeight);

		ADD_W_BOOL(largeBeamLaser, wd.largeBeamLaser);

		// shields
		ADD_W_BOOL(isShield,                wd.isShield);
		ADD_W_BOOL(shieldRepulser,          wd.shieldRepulser);
		ADD_W_BOOL(smartShield,             wd.smartShield);
		ADD_W_BOOL(exteriorShield,          wd.exteriorShield);
		ADD_W_BOOL(visibleShield,           wd.visibleShield);
		ADD_W_BOOL(visibleShieldRepulse,    wd.visibleShieldRepulse);
		ADD_W_FLOAT(shieldEnergyUse,        wd.shieldEnergyUse);
		ADD_W_FLOAT(shieldRadius,           wd.shieldRadius);
		ADD_W_FLOAT(shieldForce,            wd.shieldForce);
		ADD_W_FLOAT(shieldMaxSpeed,         wd.shieldMaxSpeed);
		ADD_W_FLOAT(shieldPower,            wd.shieldPower);
		ADD_W_FLOAT(shieldPowerRegen,       wd.shieldPowerRegen);
		ADD_W_FLOAT(shieldPowerRegenEnergy, wd.shieldPowerRegenEnergy);
		ADD_W_FLOAT(shieldAlpha,            wd.shieldAlpha);

		// bitfields
		ADD_W_INT(shieldInterceptType,     (int)wd.shieldInterceptType);
		ADD_W_INT(interceptedByShieldType, (int)wd.interceptedByShieldType);

		const bool collisionNoFeature =
			(wd.collisionFlags & COLLISION_NOFEATURE) == 0;
		const bool collisionNoFriendly =
			(wd.collisionFlags & COLLISION_NOFRIENDLY) == 0;
		ADD_W_BOOL(collisionNoFeature,  collisionNoFeature);  // CUSTOM
		ADD_W_BOOL(collisionNoFriendly, collisionNoFriendly); // CUSTOM

		ADD_W_BOOL(avoidFriendly, wd.avoidFriendly);

		if (keyBindings->GetDebug() > 1) {
			logOutput.Print(code);
		}

		// load the code
		if (!LoadCode(code, def)) {
			success = false;
		}

		code.clear();
	}
	
	return success;
}


/******************************************************************************/
/******************************************************************************/

static string IntToString(int value)
{
	char buf[32];
	SNPRINTF(buf, sizeof(buf), "%i", value);
	return string(buf);
}


static string BoolToString(bool value)
{
	if (value) {
		return string("true");
	}
	return string("false");
}


static string FloatToString(float value)
{
	char buf[32];
	SNPRINTF(buf, sizeof(buf), "%f", value);
	return string(buf);
}


static string SafeString(const string& text)
{
	const string noSlash =  StringReplace(text,    "\\", "\\\\");
	const string noQuote =  StringReplace(noSlash, "\"", "\\\"");
	const string quote = string("\"");
	return (quote + noQuote + quote);
}


static string StringReplace(const string& text,
                            const string& from, const string& to)
{
	string working = text;
	string::size_type pos = 0;
	while (true) {
		pos = working.find(from, pos);
		if (pos == string::npos) {
			break;
		}
		string tmp = working.substr(0, pos);
		tmp += to;
		tmp += working.substr(pos + from.size(), string::npos);
		pos += to.size();
		working = tmp;
	}
	return working;
}


static string GetCategoryTable(const vector<string> cats)
{
	string table = "{";
	const int catCount = (int)cats.size();
	for (int i = 0; i < catCount; i++) {
		table += " ";
		table += SafeString(StringToLower(cats[i]));
		if (i != (catCount - 1)) {
			table += ",";
		} else {
			table += " ";
		}
	}
	table += "}";
	return table;
}


static string GetCategoryTableFromBits(unsigned int bits)
{
	vector<string> cats = CCategoryHandler::Instance()->GetCategoryNames(bits);
	return GetCategoryTable(cats);
}


static string GetCategoryTableFromString(const string& text)
{
	vector<string> cats = SimpleParser::Tokenize(text, 0);
	return GetCategoryTable(cats);
}


/******************************************************************************/
