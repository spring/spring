#include "StdAfx.h"
// KeyAutoBinder.cpp: implementation of the CKeyAutoBinder class.
//
//////////////////////////////////////////////////////////////////////

#include "KeyAutoBinder.h"

#include <cctype>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "KeyBindings.h"
#include "Game/GameSetup.h"
#include "Game/Team.h"
#include "Map/ReadMap.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/ModInfo.h"


static string IntToString(int value);
static string BoolToString(bool value);
static string FloatToString(float value);
static string SafeString(const string& text);
static string StringReplace(const string& text,
                            const string& from, const string& to);


// FIXME -- use 'em
static const string ReqFuncName = "hasReqs";
static const string SortFuncName = "isBetter";


/******************************************************************************/

CKeyAutoBinder::CKeyAutoBinder()
{
	L = lua_open();
	if (L != NULL) {
		if (!LoadInfo()) {
			lua_close(L);
			L = NULL;
		}
	}
}


CKeyAutoBinder::~CKeyAutoBinder()
{
	if (L != NULL) {
		lua_close(L);
	}
}


bool CKeyAutoBinder::LoadCode(const string& code, const string& debug)
{
	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		printf("error = %i, %s, %s\n", error, debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	error = lua_pcall(L, 0, 0, 0);
	if (error != 0) {
		printf("error = %i, %s, %s\n", error, debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	
	return true;
}


bool CKeyAutoBinder::LoadInfo()
{
	// load some basic lua libs
	luaopen_base(L);
	luaopen_io(L);
	luaopen_math(L);
	luaopen_string(L);
	luaopen_table(L);
	
	const string endlStr = "\n";
	string code = MakeGameInfo() + MakeUnitDefInfo();

	// handy sorting comparison routine
	code += "function compare(a, b)"                     + endlStr;
	code += "  if (type(a) == \"number\") then"          + endlStr;
	code += "    if (a > b) then return  1.0; end"       + endlStr;
	code += "    if (a < b) then return -1.0; end"       + endlStr;
	code += "  elseif (type(a) == \"boolean\") then"     + endlStr;
	code += "    if (a and not b) then return  1.0; end" + endlStr;
	code += "    if (not a and b) then return -1.0; end" + endlStr;
	code += "  elseif (type(a) == \"string\") then"      + endlStr;
	code += "    if (a > b) then return  1.0; end"       + endlStr;
	code += "    if (a < b) then return -1.0; end"       + endlStr;
	code += "  end"                                      + endlStr;
	code += "  return 0.0"                               + endlStr;
	code += "end"                                        + endlStr;

	if (keyBindings->GetDebug() > 0) {
		printf("%s", code.c_str());
	}

	// load and run the init code
	if (!LoadCode(code, "AutoBindInfo")) {
		return false;
	}
	
	return true;
}


string CKeyAutoBinder::MakeGameInfo()
{
	const string endlStr = "\n";
	string code = "";

	code += "game = {}" + endlStr;

	code += "game.commEnds = "  + BoolToString(gs->gameMode) + endlStr;
	const float gravity = -(gs->gravity * GAME_SPEED * GAME_SPEED);
	code += "game.gravity = "   + FloatToString(gravity) + endlStr;
	code += "game.tidal = "     + FloatToString(readmap->tidalStrength) + endlStr;
	code += "game.windMin = "   + FloatToString(wind.minWind) + endlStr;
	code += "game.windMax = "   + FloatToString(wind.maxWind) + endlStr;
	code += "game.mapX = "      + IntToString(readmap->width / 64) + endlStr;
	code += "game.mapY = "      + IntToString(readmap->height / 64) + endlStr;
	code += "game.mapName = \"" + readmap->mapName + "\"" + endlStr;
	code += "game.modName = \"" + modInfo->name + "\"" + endlStr;
	
	if (gameSetup) {
		code += "game.limitDGun = "        + BoolToString(gameSetup->limitDgun) + endlStr;
		code += "game.diminishingMetal = " + BoolToString(gameSetup->diminishingMMs) + endlStr;
	} else {
		code += "game.limitDGun = false"        + endlStr;
		code += "game.diminishingMetal = false" + endlStr;
	}

	return code;	
}


string CKeyAutoBinder::MakeUnitDefInfo()
{
	const string endlStr = "\n";
	string code = "";

	code += endlStr;
	code += "unitDefs = {}" + endlStr;
	
	const std::map<std::string, int>& unitMap = unitDefHandler->unitID;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitMap.begin(); uit != unitMap.end(); uit++) {
		const UnitDef& ud = *(unitDefHandler->GetUnitByID(uit->second));

		const string def = "unitDefs[" + IntToString(uit->second) + "]";

		code += endlStr;
		code += def + " = {}" + endlStr;

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
		
		ADD_BOOL(builder,        ud.builder); 
		ADD_FLOAT(buildSpeed,    ud.buildSpeed);
		ADD_FLOAT(buildDistance, ud.buildDistance);
		ADD_INT(builderCount,    ud.buildOptions.size()); // CUSTOM

		ADD_INT(weaponCount,  ud.weapons.size()); // CUSTOM
		
		// NEEDED: maxRange, canShootAir, canShootWater, canParalyze
		
		
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

		// FIXME -- is this right?		
		const float basicEnergy = (ud.energyMake - ud.energyUpkeep);
		const float tidalEnergy = (ud.tidalGenerator * readmap->tidalStrength);
		float windEnergy = 0.0f;
		if (ud.windGenerator > 0.0f) {
			windEnergy = (0.25f * (wind.minWind + wind.maxWind));
		}
		ADD_FLOAT(totalEnergyOut, basicEnergy + tidalEnergy + windEnergy); // CUSTOM

		ADD_BOOL(needGeo,         ud.needGeo);
		ADD_BOOL(isMetalMaker,    ud.isMetalMaker);

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
		ADD_INT(armorType,         ud.armorType);

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

//  unsigned int noChaseCategory;
		ADD_INT(noChaseCategory, ud.noChaseCategory);
		
		ADD_BOOL(canDropFlare, ud.canDropFlare);
		ADD_FLOAT(flareReloadTime, ud.flareReloadTime);
		ADD_FLOAT(flareEfficieny, ud.flareEfficieny); 
		ADD_FLOAT(flareDelay, ud.flareDelay);

//  float3 flareDropVector;
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
	}
	
	return code;
}


string CKeyAutoBinder::MakeWeaponDefInfo()
{
	const string endlStr = "\n";
	string code = "";

	code += endlStr;
	code += "weaponDefs = {}" + endlStr;
	
	// FIXME -- kinda empty right now...
}


/******************************************************************************/

static CKeyAutoBinder* thisBinder = NULL;


bool CKeyAutoBinder::UnitDefHolder::operator<(const UnitDefHolder& m) const
{
	return thisBinder->IsBetter(udID, m.udID);
}


/******************************************************************************/

bool CKeyAutoBinder::BindBuildType(const string& keystr,
                                   const vector<string>& requirements,
                                   const vector<string>& sortCriteria,
                                   const vector<string>& chords)
{
	if (L == NULL) {
		return false;
	}

	const string reqCall = MakeRequirementCall(requirements);
	const string sortCall = MakeSortCriteriaCall(sortCriteria);

	if (keyBindings->GetDebug() > 0) {  
		printf("reqCall(%s):\n%s\n", keystr.c_str(), reqCall.c_str());
		printf("sortCall(%s):\n%s\n", keystr.c_str(), sortCall.c_str());
	}
	
	if (!LoadCode(reqCall,  "Load_hasReqs") ||
	    !LoadCode(sortCall, "Load_isBetter")) {
		return false;
	}
	
	vector<UnitDefHolder> defs;

	// find the unit definitions that meet the requirements	
	const std::map<std::string, int>& unitMap = unitDefHandler->unitID;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitMap.begin(); uit != unitMap.end(); uit++) {
		const UnitDef& ud = unitDefHandler->unitDefs[uit->second];
		if (HasRequirements(uit->second)) {
			UnitDefHolder udh;
			udh.ud = &ud;
			udh.udID = uit->second;
			defs.push_back(udh);
		}
	}
	
	// sort the results
	thisBinder = this;
	sort(defs.begin(), defs.end());
	
	// add the bindings
	for (unsigned int i = 0; i < defs.size(); i++) {
		const string bindStr = "bind";
		const string lowerName = StringToLower(defs[i].ud->name);
		const string action = string("buildunit_") + lowerName;
		keyBindings->Command(bindStr + " " + keystr + " " + action);
		if ((i < chords.size()) && (StringToLower(chords[i]) != "none")) {
			keyBindings->Command(bindStr + " " + chords[i] + " " + action);
		}
		if (keyBindings->GetDebug() > 0) {  
			printf("%s\n", string(bindStr + " " + keystr + " " + action).c_str());
		}
	}
	
	return true;
}


string CKeyAutoBinder::ConvertBooleanSymbols(const string& text) const
{
	string newText = text;
	newText = StringReplace(newText, "&&", " and ");
	newText = StringReplace(newText, "||", " or ");
	newText = StringReplace(newText, "!",  " not ");
	return newText;
}


string CKeyAutoBinder::AddUnitDefPrefix(const string& text,
                                        const string& prefix) const
{
	string result;
	const char* end = text.c_str();
	
	while (end[0] != 0) {
		const char* c = end;
		while ((c[0] != 0) && !isalpha(c[0]) && (c[0] != '_')) { c++; }
		result += string(end, c - end);
		if (c[0] == 0) {
			break;
		}
		const char* start = c;
		while ((c[0] != 0) && (isalnum(c[0]) || (c[0] == '_'))) { c++; }
		string word(start, c - start);
		if (unitDefParams.find(word) != unitDefParams.end()) {
			result += prefix;
		}
		result += word;
		end = c;
	}
		
	return result;
}


string CKeyAutoBinder::MakeRequirementCall(const vector<string>& requirements)
{
	const string endlStr = "\n";
	string code = "";

	code += "function hasReqs(thisID)" + endlStr;
	
	const int count = (int)requirements.size();
	
	if (count <= 0) {
		code += "return false" + endlStr;
		code += "end" + endlStr;
		return code;
	}

	code += "local this = unitDefs[thisID]" + endlStr;
	
	if (StringToLower(requirements[0]) == "rawlua") {
		for (int i = 1; i < count; i++) {
			code += requirements[i] + endlStr;
		}
		code += "end" + endlStr;
		return code;
	}
	
	code += "return ";
	const int lastReq = (count - 1);
	for (int i = 0; i < count; i++) {
		code += "(";
		code += requirements[i];
		code += ")";
		if (i != lastReq) {
			code += " and ";
		}
	}
	code += endlStr + "end" + endlStr;
	
	return ConvertBooleanSymbols(AddUnitDefPrefix(code, "this."));
}


string CKeyAutoBinder::MakeSortCriteriaCall(const vector<string>& sortCriteria)
{
	const string endlStr = "\n";
	string code = "";

	code += "function isBetter(thisID, thatID)" + endlStr;
	
	const int count = (int)sortCriteria.size();

	if (count <= 0) {
		code += "return false" + endlStr;
		code += "end" + endlStr;
		return code;
	}

	code += "local this = unitDefs[thisID]" + endlStr;
	code += "local that = unitDefs[thatID]" + endlStr;

	if (StringToLower(sortCriteria[0]) == "rawlua") {
		for (int i = 1; i < count; i++) {
			code += sortCriteria[i] + endlStr;
		}
		code += "end" + endlStr;
		return code;
	}
	
	for (int i = 0; i < count; i++) {
		const string natural = ConvertBooleanSymbols(sortCriteria[i]);
		const string thisStr = AddUnitDefPrefix(natural, "this.");
		const string thatStr = AddUnitDefPrefix(natural, "that.");
		code += "local test = compare(" + thisStr + ", " + thatStr + ")" + endlStr;
		code += "if     (test ==  1.0) then return true" + endlStr;
		code += "elseif (test == -1.0) then return false; end" + endlStr;
	}
	code += "return false" + endlStr;
	code += "end" + endlStr;
	
	return ConvertBooleanSymbols(code);
}


bool CKeyAutoBinder::HasRequirements(int unitDefID)
{
	lua_getglobal(L, "hasReqs");
	lua_pushnumber(L, unitDefID);
	const int error = lua_pcall(L, 1, 1, 0);
	if (error != 0) {
		printf("error = %i, %s, %s\n", error, "Call_hasReqs", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	const bool value = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return value;
}


bool CKeyAutoBinder::IsBetter(int thisDefID, int thatDefID)
{
	lua_getglobal(L, "isBetter");
	lua_pushnumber(L, thisDefID);
	lua_pushnumber(L, thatDefID);
	const int error = lua_pcall(L, 2, 1, 0);
	if (error != 0) {
		printf("error = %i, %s, %s\n", error, "Call_isBetter", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	const bool value = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return value;
}


/******************************************************************************/

static string IntToString(int value)
{
	char buf[16];
	SNPRINTF(buf, 16, "%i", value);
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
	char buf[16];
	SNPRINTF(buf, 16, "%f", value);
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


/******************************************************************************/
