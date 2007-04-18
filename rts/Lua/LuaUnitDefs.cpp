#include "StdAfx.h"
// LuaUnitDefs.cpp: implementation of the LuaUnitDefs class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUnitDefs.h"

#include <set>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cctype>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaDefs.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Game/command.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/Team.h"
#include "Game/UI/SimpleParser.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/FileSystem.h"

using namespace std;


static ParamMap paramMap;

static bool InitParamMap();

// iteration routines
static int Next(lua_State* L);
static int Pairs(lua_State* L);

// meta-table calls
static int UnitDefIndex(lua_State* L);
static int UnitDefNewIndex(lua_State* L);
static int UnitDefMetatable(lua_State* L);

// special access functions
static int UnitDefToID(lua_State* L, const void* data);
static int WeaponDefToID(lua_State* L, const void* data);
static int BuildOptions(lua_State* L, const void* data);
static int SoundsTable(lua_State* L, const void* data);
static int WeaponsTable(lua_State* L, const void* data);
static int ModelDefTable(lua_State* L, const void* data);
static int CategorySetFromBits(lua_State* L, const void* data);
static int CategorySetFromString(lua_State* L, const void* data);


/******************************************************************************/
/******************************************************************************/

bool LuaUnitDefs::PushEntries(lua_State* L)
{
	if (paramMap.empty()) {
	  InitParamMap();
	}

	const std::map <std::string, int>& udMap = unitDefHandler->unitID;
	std::map< std::string, int>::const_iterator udIt;
	for (udIt = udMap.begin(); udIt != udMap.end(); udIt++) {
	  const UnitDef* ud = unitDefHandler->GetUnitByID(udIt->second);
		if (ud == NULL) {
	  	continue;
		}
		lua_pushnumber(L, ud->id);
		lua_newtable(L); { // the proxy table

			lua_newtable(L); { // the metatable

				HSTR_PUSH(L, "__index");
				lua_pushlightuserdata(L, (void*)ud);
				lua_pushcclosure(L, UnitDefIndex, 1);
				lua_rawset(L, -3); // closure 

				HSTR_PUSH(L, "__newindex");
				lua_pushlightuserdata(L, (void*)ud);
				lua_pushcclosure(L, UnitDefNewIndex, 1);
				lua_rawset(L, -3);

				HSTR_PUSH(L, "__metatable");
				lua_pushlightuserdata(L, (void*)ud);
				lua_pushcclosure(L, UnitDefMetatable, 1);
				lua_rawset(L, -3);
			}

			lua_setmetatable(L, -2);
		}

		HSTR_PUSH(L, "pairs");
		lua_pushcfunction(L, Pairs);
		lua_rawset(L, -3);

		HSTR_PUSH(L, "next");
		lua_pushcfunction(L, Next);
		lua_rawset(L, -3);

		lua_rawset(L, -3); // proxy table into UnitDefs
	}

	return true;
}


bool LuaUnitDefs::IsDefaultParam(const std::string& word)
{
	if (paramMap.empty()) {
	  InitParamMap();
	}
	return (paramMap.find(word) != paramMap.end());
}


/******************************************************************************/

static int UnitDefIndex(lua_State* L)
{
	// not a default value	
	if (!lua_isstring(L, 2)) {
		lua_rawget(L, 1);
		return 1;
	}

	const char* name = lua_tostring(L, 2);
	ParamMap::const_iterator it = paramMap.find(name);

	// not a default value	
	if (paramMap.find(name) == paramMap.end()) {
	  lua_rawget(L, 1);
	  return 1;
	}

	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const UnitDef* ud = (const UnitDef*)userData;
	const DataElement& elem = it->second;
	const char* p = ((const char*)ud) + elem.offset;
	switch (elem.type) {
		case READONLY_TYPE: {
			lua_rawget(L, 1);
			return 1;
		}
		case INT_TYPE: {
			lua_pushnumber(L, *((int*)p));
			return 1;
		}
		case BOOL_TYPE: {
			lua_pushboolean(L, *((bool*)p));
			return 1;
		}
		case FLOAT_TYPE: {
			lua_pushnumber(L, *((float*)p));
			return 1;
		}
		case STRING_TYPE: {
			lua_pushstring(L, ((string*)p)->c_str());
			return 1;
		}
		case FUNCTION_TYPE: {
			return elem.func(L, p);
		}
		case ERROR_TYPE:{
			luaL_error(L, "ERROR_TYPE in UnitDefs __index");
		}
	}
	return 0;
}


static int UnitDefNewIndex(lua_State* L)
{
	// not a default value, set it
	if (!lua_isstring(L, 2)) {
		lua_rawset(L, 1);
		return 0;
	}

	const char* name = lua_tostring(L, 2);
	ParamMap::const_iterator it = paramMap.find(name);
	
	// not a default value, set it
	if (paramMap.find(name) == paramMap.end()) {
		lua_rawset(L, 1);
		return 0;
	}

	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const UnitDef* ud = (const UnitDef*)userData;

	// write-protected
	if (!gs->editDefsEnabled) {
		luaL_error(L, "Attempt to write UnitDefs[%d].%s", ud->id, name);
		return 0;
	}

	// Definition editing
	const DataElement& elem = it->second;
	const char* p = ((const char*)ud) + elem.offset;

	switch (elem.type) {
		case FUNCTION_TYPE:
		case READONLY_TYPE: {
			luaL_error(L, "Can not write to %s", name);
			return 0;
		}
		case INT_TYPE: {
			*((int*)p) = (int)lua_tonumber(L, -1);
			return 0;
		}
		case BOOL_TYPE: {
			*((bool*)p) = lua_toboolean(L, -1);
			return 0;
		}
		case FLOAT_TYPE: {
			*((float*)p) = (float)lua_tonumber(L, -1);
			return 0;
		}
		case STRING_TYPE: {
			*((string*)p) = lua_tostring(L, -1);
			return 0;
		}
		case ERROR_TYPE:{
			luaL_error(L, "ERROR_TYPE in UnitDefs __newindex");
		}
	}
	
	return 0;
}


static int UnitDefMetatable(lua_State* L)
{
	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const UnitDef* ud = (const UnitDef*)userData;
	return 0;
}


/******************************************************************************/

static int Next(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 2); // create a 2nd argument if there isn't one

	// internal parameters first
	if (lua_isnil(L, 2)) {
		const string& nextKey = paramMap.begin()->first;
		lua_pushstring(L, nextKey.c_str()); // push the key
		lua_pushvalue(L, 3);                // copy the key
		lua_gettable(L, 1);                 // get the value
		return 2;
	}

	// all internal parameters use strings as keys
	if (lua_isstring(L, 2)) {
		const char* key = lua_tostring(L, 2);
		ParamMap::const_iterator it = paramMap.find(key);
		if ((it != paramMap.end()) && (it->second.type != READONLY_TYPE)) {
			// last key was an internal parameter
			it++;
			while ((it != paramMap.end()) && (it->second.type == READONLY_TYPE)) {
				it++; // skip read-only parameters
			}
			if ((it != paramMap.end()) && (it->second.type != READONLY_TYPE)) {
				// next key is an internal parameter
				const string& nextKey = it->first;
				lua_pushstring(L, nextKey.c_str()); // push the key
				lua_pushvalue(L, 3);                // copy the key
				lua_gettable(L, 1);                 // get the value (proxied)
				return 2;
			}
			// start the user parameters,
			// remove the internal key and push a nil
			lua_settop(L, 1); 
			lua_pushnil(L);
		}
	}

	// user parameter
	if (lua_next(L, 1)) {
		return 2;
	}

	// end of the line
	lua_pushnil(L);
	return 1;
}


static int Pairs(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushcfunction(L, Next);	// iterator
	lua_pushvalue(L, 1);        // state (table)
	lua_pushnil(L);             // initial value
	return 3;
}


/******************************************************************************/
/******************************************************************************/

static int UnitDefToID(lua_State* L, const void* data)
{
	const UnitDef* ud = *((const UnitDef**)data);
	if (ud == NULL) {
		return 0;
	}
	lua_pushnumber(L, ud->id);
	return 1;
}


static int WeaponDefToID(lua_State* L, const void* data)
{
	const WeaponDef* wd = *((const WeaponDef**)data);
	if (wd == NULL) {
		return 0;
	}
	lua_pushnumber(L, wd->id);
	return 1;
}


static int SafeIconType(lua_State* L, const void* data)
{
	// the iconType is unsynced because LuaUI has SetUnitDefIcon()
	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		const string& iconType = *((const string*)data);
		lua_pushstring(L, iconType.c_str());
		return 1;
	}
	return 0;
}


static int BuildOptions(lua_State* L, const void* data)
{
	const map<int, string>& buildOptions = *((const map<int, string>*)data);
	const map<string, int>& unitMap = unitDefHandler->unitID;

	lua_newtable(L);
	int count = 0;
	map<int, string>::const_iterator it;
	for (it = buildOptions.begin(); it != buildOptions.end(); ++it) {
		map<string, int>::const_iterator fit = unitMap.find(it->second);
		if (fit != unitMap.end()) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, fit->second); // UnitDef id
			lua_rawset(L, -3);
		}
	}
	HSTR_PUSH_NUMBER(L, "n", count);
	return 1;
}


static inline int BuildCategorySet(lua_State* L, const vector<string>& cats)
{
	lua_newtable(L);
	const int count = (int)cats.size();
	for (int i = 0; i < count; i++) {
		lua_pushstring(L, cats[i].c_str());
		lua_pushboolean(L, true);
		lua_rawset(L, -3);
	}
	return 1;
}


static int CategorySetFromBits(lua_State* L, const void* data)
{
	const int bits = *((const int*)data);
	const vector<string> cats =
		CCategoryHandler::Instance()->GetCategoryNames(bits);
	return BuildCategorySet(L, cats);
}


static int CategorySetFromString(lua_State* L, const void* data)
{
	const string& str = *((const string*)data);
	const vector<string> cats = SimpleParser::Tokenize(str, 0);
	return BuildCategorySet(L, cats);
}


static int WeaponsTable(lua_State* L, const void* data)
{
	const vector<UnitDef::UnitDefWeapon>& weapons =
		*((const vector<UnitDef::UnitDefWeapon>*)data);
	const int weaponCount = (int)weapons.size();

	lua_newtable(L);

	for (int i = 0; i < weaponCount; i++) {
		const UnitDef::UnitDefWeapon& udw = weapons[i];
		const WeaponDef* weapon = udw.def;
		lua_pushnumber(L, i + 1);
		lua_newtable(L); {
			HSTR_PUSH_NUMBER(L, "weaponDef",   weapon->id);
			HSTR_PUSH_NUMBER(L, "slavedTo",    udw.slavedTo);
			HSTR_PUSH_NUMBER(L, "maxAngleDif", udw.maxAngleDif);
			HSTR_PUSH_NUMBER(L, "fuelUsage",   udw.fuelUsage);

			HSTR_PUSH(L, "badTargets");
			CategorySetFromBits(L, &udw.badTargetCat);
			lua_rawset(L, -3);

			HSTR_PUSH(L, "onlyTargets");
			CategorySetFromBits(L, &udw.onlyTargetCat);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", weaponCount);
	return 1;
}


static void PushGuiSound(lua_State* L,
                         const string& name, const GuiSound& sound)
{
	lua_pushstring(L, name.c_str());
	lua_newtable(L);
	HSTR_PUSH_STRING(L, "name",   sound.name);
	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		HSTR_PUSH_NUMBER(L, "id",   sound.id); 
	}
	HSTR_PUSH_NUMBER(L, "volume", sound.volume);
	lua_rawset(L, -3);  
}


static int SoundsTable(lua_State* L, const void* data)
{
	const UnitDef::SoundStruct& sounds = *((const UnitDef::SoundStruct*)data);

	lua_newtable(L);
	PushGuiSound(L, "select",      sounds.select);
	PushGuiSound(L, "ok",          sounds.ok);
	PushGuiSound(L, "arrived",     sounds.arrived);
	PushGuiSound(L, "build",       sounds.build);
	PushGuiSound(L, "repair",      sounds.repair);
	PushGuiSound(L, "working",     sounds.working);
	PushGuiSound(L, "underattack", sounds.underattack);
	PushGuiSound(L, "cant",        sounds.cant);
	PushGuiSound(L, "activate",    sounds.activate);
	PushGuiSound(L, "deactivate",  sounds.deactivate);

	return 1;
}


static int ModelDefTable(lua_State* L, const void* data)
{
	const UnitModelDef& md = *((const UnitModelDef*)data);
	lua_newtable(L);
	HSTR_PUSH_STRING(L, "path", md.modelpath);
	HSTR_PUSH_STRING(L, "name", md.modelname);
	HSTR_PUSH(L, "textures");
	lua_newtable(L);
	std::map<std::string, std::string>::const_iterator it;
	for (it = md.textures.begin(); it != md.textures.end(); ++it) {
		LuaPushNamedString(L, it->first, it->second);
	}
	lua_rawset(L, -3);
	return 1;
}


static int MoveDataTable(lua_State* L, const void* data)
{
	const MoveData* md = *((const MoveData**)data);
	lua_newtable(L);
	if (md == NULL) {
		return 1;
	}

	HSTR_PUSH_NUMBER(L, "id", md->pathType);
	
	const int Ship_Move   = MoveData::Ship_Move;
	const int Hover_Move  = MoveData::Hover_Move;
	const int Ground_Move = MoveData::Ground_Move;

	switch (md->moveType) {
		case Ship_Move:   { HSTR_PUSH_STRING(L, "type", "ship");   break; }
		case Hover_Move:  { HSTR_PUSH_STRING(L, "type", "hover");  break; }
		case Ground_Move: { HSTR_PUSH_STRING(L, "type", "ground"); break; }
		default:          { HSTR_PUSH_STRING(L, "type", "error");  break; } 
	}

	switch (md->moveFamily) {
		case 0:  { HSTR_PUSH_STRING(L, "family", "tank");  break; }
		case 1:  { HSTR_PUSH_STRING(L, "family", "kbot");  break; }
		case 2:  { HSTR_PUSH_STRING(L, "family", "hover"); break; }
		case 3:  { HSTR_PUSH_STRING(L, "family", "ship");  break; }
		default: { HSTR_PUSH_STRING(L, "family", "error"); break; }
	}

	HSTR_PUSH_NUMBER(L, "size",          md->size);
	HSTR_PUSH_NUMBER(L, "depth",         md->depth);
	HSTR_PUSH_NUMBER(L, "maxSlope",      md->maxSlope);
	HSTR_PUSH_NUMBER(L, "slopeMod",      md->slopeMod);
	HSTR_PUSH_NUMBER(L, "depthMod",      md->depthMod);
	HSTR_PUSH_NUMBER(L, "crushStrength", md->crushStrength);

	return 1;
}


static int TotalEnergyOut(lua_State* L, const void* data)
{
	const UnitDef& ud = *((const UnitDef*)data);
	const float basicEnergy = (ud.energyMake - ud.energyUpkeep);
	const float tidalEnergy = (ud.tidalGenerator * readmap->tidalStrength);
	float windEnergy = 0.0f;
	if (ud.windGenerator > 0.0f) {
		windEnergy = (0.25f * (wind.minWind + wind.maxWind));
	}
	lua_pushnumber(L, basicEnergy + tidalEnergy + windEnergy); // CUSTOM
	return 1;
}


#define TYPE_STRING_FUNC(name)                          \
	static int Is ## name(lua_State* L, const void* data) \
	{                                                     \
		const string& type = *((const string*)data);        \
		lua_pushboolean(L, type == #name);                  \
		return 1;                                           \
	}
		
TYPE_STRING_FUNC(Bomber);
TYPE_STRING_FUNC(Builder);
TYPE_STRING_FUNC(Building);
TYPE_STRING_FUNC(Factory);
TYPE_STRING_FUNC(Fighter);
TYPE_STRING_FUNC(Transport);
TYPE_STRING_FUNC(GroundUnit);
TYPE_STRING_FUNC(MetalExtractor);



/******************************************************************************/
/******************************************************************************/

static bool InitParamMap()
{
	paramMap["next"]  = DataElement(READONLY_TYPE); 
	paramMap["pairs"] = DataElement(READONLY_TYPE); 

	// dummy UnitDef for address lookups
	const UnitDef ud;
	const char* start = ADDRESS(ud);
	
//	ADD_BOOL(valid, ud.valid);

// ADD_INT("weaponCount", weaponCount); // CUSTOM
/*
ADD_FLOAT("maxRange",       maxRange);       // CUSTOM
ADD_BOOL("hasShield",       hasShield);      // CUSTOM
ADD_BOOL("canParalyze",     canParalyze);    // CUSTOM
ADD_BOOL("canStockpile",    canStockpile);   // CUSTOM
ADD_BOOL("canAttackWater",  canAttackWater); // CUSTOM 
*/
// ADD_INT("buildOptionsCount", ud.buildOptions.size(")); // CUSTOM
  
	ADD_FUNCTION("totalEnergyOut", ud, TotalEnergyOut);
	
	ADD_FUNCTION("modCategories",      ud.categoryString,  CategorySetFromString);
	ADD_FUNCTION("springCategories",   ud.category,        CategorySetFromBits);
	ADD_FUNCTION("noChaseCategories",  ud.noChaseCategory, CategorySetFromBits);

	ADD_FUNCTION("buildOptions",       ud.buildOptions,       BuildOptions);
	ADD_FUNCTION("decoyDef",           ud.decoyDef,           UnitDefToID);
	ADD_FUNCTION("weapons",            ud.weapons,            WeaponsTable);
	ADD_FUNCTION("sounds",             ud.sounds,             SoundsTable);
	ADD_FUNCTION("model",              ud.model,              ModelDefTable);
	ADD_FUNCTION("moveData",           ud.movedata,           MoveDataTable);
	ADD_FUNCTION("shieldWeaponDef",    ud.shieldWeaponDef,    WeaponDefToID);
	ADD_FUNCTION("stockpileWeaponDef", ud.stockpileWeaponDef, WeaponDefToID);
	ADD_FUNCTION("iconType",           ud.iconType,           SafeIconType);

	ADD_FUNCTION("isBomber",         ud.type, IsBomber);
	ADD_FUNCTION("isBuilder",        ud.type, IsBuilder);
	ADD_FUNCTION("isBuilding",       ud.type, IsBuilding);
	ADD_FUNCTION("isFactory",        ud.type, IsFactory);
	ADD_FUNCTION("isFighter",        ud.type, IsFighter);
	ADD_FUNCTION("isTransport",      ud.type, IsTransport);
	ADD_FUNCTION("isGroundUnit",     ud.type, IsGroundUnit);
	ADD_FUNCTION("isMetalExtractor", ud.type, IsMetalExtractor);


	ADD_INT("id", ud.id);

	ADD_STRING("name",      ud.name);
	ADD_STRING("humanName", ud.humanName);
	ADD_STRING("filename",  ud.filename);

	ADD_STRING("buildpicname", ud.buildpicname);
	ADD_INT("imageSizeX", ud.imageSizeX);
	ADD_INT("imageSizeY", ud.imageSizeY);

	ADD_STRING("gaia", ud.gaia);

	ADD_INT("aihint",      ud.aihint);
	ADD_INT("techLevel",   ud.techLevel);
	ADD_INT("maxThisUnit", ud.maxThisUnit);

	ADD_STRING("TEDClass", ud.TEDClassString);

	ADD_STRING("type", ud.type);
	ADD_STRING("tooltip", ud.tooltip);

	ADD_STRING("wreckName", ud.wreckName);
	ADD_STRING("deathExplosion", ud.deathExplosion);
	ADD_STRING("selfDExplosion", ud.selfDExplosion);

	ADD_FLOAT("metalUpkeep",    ud.metalUpkeep);
	ADD_FLOAT("energyUpkeep",   ud.energyUpkeep);
	ADD_FLOAT("metalMake",      ud.metalMake);
	ADD_FLOAT("makesMetal",     ud.makesMetal);
	ADD_FLOAT("energyMake",     ud.energyMake);
	ADD_FLOAT("metalCost",      ud.metalCost);
	ADD_FLOAT("energyCost",     ud.energyCost);
	ADD_FLOAT("buildTime",      ud.buildTime);
	ADD_FLOAT("extractsMetal",  ud.extractsMetal);
	ADD_FLOAT("extractRange",   ud.extractRange);
	ADD_FLOAT("windGenerator",  ud.windGenerator);
	ADD_FLOAT("tidalGenerator", ud.tidalGenerator);
	ADD_FLOAT("metalStorage",   ud.metalStorage);
	ADD_FLOAT("energyStorage",  ud.energyStorage);

	ADD_BOOL("isMetalMaker", ud.isMetalMaker);

	ADD_FLOAT("power", ud.power);

	ADD_FLOAT("health", ud.health);
	ADD_FLOAT("autoHeal",     ud.autoHeal);
	ADD_FLOAT("idleAutoHeal", ud.idleAutoHeal);

	ADD_INT("idleTime", ud.idleTime);

	ADD_INT("selfDCountdown", ud.selfDCountdown);

	ADD_INT("moveType",   ud.moveType);
	ADD_FLOAT("speed",    ud.speed);
	ADD_FLOAT("turnRate", ud.turnRate);
	ADD_BOOL("upright",   ud.upright);

	ADD_FLOAT("losHeight",     ud.losHeight);
	ADD_FLOAT("losRadius",     ud.losRadius);
	ADD_FLOAT("airLosRadius",  ud.airLosRadius);
	ADD_FLOAT("controlRadius", ud.controlRadius);

	ADD_INT("radarRadius",        ud.radarRadius);
	ADD_INT("sonarRadius",        ud.sonarRadius);
	ADD_INT("jammerRadius",       ud.jammerRadius);
	ADD_INT("sonarJamRadius",     ud.sonarJamRadius);
	ADD_INT("seismicRadius",      ud.seismicRadius);

	ADD_FLOAT("seismicSignature", ud.seismicSignature);

	ADD_BOOL("stealth",           ud.stealth);

	ADD_FLOAT("mass", ud.mass);

	ADD_FLOAT("maxSlope",      ud.maxSlope);
	ADD_FLOAT("maxHeightDif",  ud.maxHeightDif);
	ADD_FLOAT("minWaterDepth", ud.minWaterDepth);
	ADD_FLOAT("waterline",     ud.waterline);
	ADD_FLOAT("maxWaterDepth", ud.maxWaterDepth);

	ADD_INT("armorType",         ud.armorType);
	ADD_FLOAT("armoredMultiple", ud.armoredMultiple);

	ADD_FLOAT("hitSphereScale",   ud.collisionSphereScale);
	ADD_FLOAT("hitSphereOffsetX", ud.collisionSphereOffset.x);
	ADD_FLOAT("hitSphereOffsetY", ud.collisionSphereOffset.y);
	ADD_FLOAT("hitSphereOffsetZ", ud.collisionSphereOffset.z);
	ADD_BOOL("useHitSphereOffset", ud.useCSOffset);

	ADD_FLOAT("minCollisionSpeed", ud.minCollisionSpeed);
	ADD_FLOAT("slideTolerance",    ud.slideTolerance);

	ADD_FLOAT("maxWeaponRange", ud.maxWeaponRange);

	ADD_FLOAT("buildSpeed", ud.buildSpeed);
	ADD_FLOAT("buildDistance", ud.buildDistance);

	ADD_BOOL("canSubmerge", ud.canSubmerge);
	ADD_BOOL("canFly",      ud.canfly);
	ADD_BOOL("canMove",     ud.canmove);
	ADD_BOOL("canHover",    ud.canhover);
	ADD_BOOL("floater",     ud.floater);
	ADD_BOOL("builder",     ud.builder);
	ADD_BOOL("onOffable",   ud.onoffable);
	ADD_BOOL("activateWhenBuilt", ud.activateWhenBuilt);

	ADD_BOOL("reclaimable",       ud.reclaimable);

	ADD_BOOL("canDGun",           ud.canDGun);
	ADD_BOOL("canCloak",          ud.canCloak);
	ADD_BOOL("canRestore",        ud.canRestore);
	ADD_BOOL("canRepair",         ud.canRepair);
	ADD_BOOL("canReclaim",        ud.canReclaim);
	ADD_BOOL("noAutoFire",        ud.noAutoFire);
	ADD_BOOL("canAttack",         ud.canAttack);
	ADD_BOOL("canPatrol",         ud.canPatrol);
	ADD_BOOL("canFight",          ud.canFight);
	ADD_BOOL("canGuard",          ud.canGuard);
	ADD_BOOL("canBuild",          ud.canBuild);
	ADD_BOOL("canAssist",         ud.canAssist);
	ADD_BOOL("canRepeat",         ud.canRepeat);
	ADD_BOOL("canCapture",        ud.canCapture);
	ADD_BOOL("canResurrect",      ud.canResurrect);
	ADD_BOOL("canLoopbackAttack", ud.canLoopbackAttack);

	//aircraft stuff
	ADD_FLOAT("wingDrag",     ud.wingDrag);
	ADD_FLOAT("wingAngle",    ud.wingAngle);
	ADD_FLOAT("drag",         ud.drag);
	ADD_FLOAT("frontToSpeed", ud.frontToSpeed);
	ADD_FLOAT("speedToFront", ud.speedToFront);
	ADD_FLOAT("myGravity",    ud.myGravity);

	ADD_FLOAT("maxBank",      ud.maxBank);
	ADD_FLOAT("maxPitch",     ud.maxPitch);
	ADD_FLOAT("turnRadius",   ud.turnRadius);
	ADD_FLOAT("wantedHeight", ud.wantedHeight);
	ADD_BOOL("hoverAttack",   ud.hoverAttack);
	ADD_BOOL("airStrafe",     ud.airStrafe);

	// < 0 means it can land,
	// >= 0 indicates how much the unit will move during hovering on the spot
	ADD_FLOAT("dlHoverFactor", ud.dlHoverFactor);

//	bool DontLand (") { return dlHoverFactor >= 0.0f; }

	ADD_FLOAT("maxAcc",      ud.maxAcc);
	ADD_FLOAT("maxDec",      ud.maxDec);
	ADD_FLOAT("maxAileron",  ud.maxAileron);
	ADD_FLOAT("maxElevator", ud.maxElevator);
	ADD_FLOAT("maxRudder",   ud.maxRudder);

	ADD_FLOAT("maxFuel", ud.maxFuel);
	ADD_FLOAT("refuelTime", ud.refuelTime);
	ADD_FLOAT("minAirBasePower", ud.minAirBasePower);

//	MoveData* movedata;
//	unsigned char* yardmapLevels[6];
//	unsigned char* yardmaps[4];			//Iterations of the Ymap for building rotation
	
	ADD_INT("xsize", ud.xsize);
	ADD_INT("ysize", ud.ysize);

	ADD_INT("buildangle", ud.buildangle);

	// transport stuff
	ADD_INT("transportCapacity", ud.transportCapacity);
	ADD_INT("transportSize",     ud.transportSize);
	ADD_FLOAT("transportMass",   ud.transportMass);
	ADD_FLOAT("loadingRadius",   ud.loadingRadius);
	ADD_BOOL("isAirBase",        ud.isAirBase);
	ADD_BOOL("isFirePlatform",   ud.isfireplatform);
	ADD_BOOL("holdSteady",       ud.holdSteady);
	ADD_BOOL("releaseHeld",      ud.releaseHeld);
	ADD_BOOL("transportByEnemy", ud.transportByEnemy);
	
	ADD_BOOL("startCloaked",     ud.startCloaked);
	ADD_FLOAT("cloakCost",       ud.cloakCost);
	ADD_FLOAT("cloakCostMoving", ud.cloakCostMoving);
	ADD_FLOAT("decloakDistance", ud.decloakDistance);

	ADD_BOOL("canKamikaze",   ud.canKamikaze);
	ADD_FLOAT("kamikazeDist", ud.kamikazeDist);

	ADD_BOOL("targfac",        ud.targfac);

	ADD_BOOL("needGeo",        ud.needGeo);
	ADD_BOOL("isFeature",      ud.isFeature);

	ADD_BOOL("isCommander",    ud.isCommander);

	ADD_BOOL("hideDamage",     ud.hideDamage);
	ADD_BOOL("showPlayerName", ud.showPlayerName);

	ADD_INT("highTrajectoryType", ud.highTrajectoryType);

	ADD_INT("trackType",       ud.trackType);
	ADD_BOOL("leaveTracks",    ud.leaveTracks);
	ADD_FLOAT("trackWidth",    ud.trackWidth);
	ADD_FLOAT("trackOffset",   ud.trackOffset);
	ADD_FLOAT("trackStrength", ud.trackStrength);
	ADD_FLOAT("trackStretch",  ud.trackStretch);

	ADD_BOOL("canDropFlare",      ud.canDropFlare);
	ADD_FLOAT("flareReloadTime",  ud.flareReloadTime);
	ADD_FLOAT("flareEfficieny",   ud.flareEfficieny);
	ADD_FLOAT("flareDelay",       ud.flareDelay);
	ADD_FLOAT("flareDropVectorX", ud.flareDropVector.x);
	ADD_FLOAT("flareDropVectorY", ud.flareDropVector.y);
	ADD_FLOAT("flareDropVectorZ", ud.flareDropVector.z);
	ADD_INT("flareTime",          ud.flareTime);
	ADD_INT("flareSalvoSize",     ud.flareSalvoSize);
	ADD_INT("flareSalvoDelay",    ud.flareSalvoDelay);

	ADD_BOOL("smoothAnim", ud.smoothAnim);
	ADD_BOOL("levelGround", ud.levelGround);

	ADD_BOOL("useBuildingGroundDecal",   ud.useBuildingGroundDecal);
	ADD_INT("buildingDecalType",         ud.buildingDecalType);
	ADD_INT("buildingDecalSizeX",        ud.buildingDecalSizeX);
	ADD_INT("buildingDecalSizeY",        ud.buildingDecalSizeY);
	ADD_FLOAT("buildingDecalDecaySpeed", ud.buildingDecalDecaySpeed);

	ADD_BOOL("showNanoFrame", ud.showNanoFrame);
	ADD_BOOL("showNanoSpray", ud.showNanoSpray);
	ADD_FLOAT("nanoColorR",   ud.nanoColor.x);
	ADD_FLOAT("nanoColorG",   ud.nanoColor.y);
	ADD_FLOAT("nanoColorB",   ud.nanoColor.z);

//	std::vector<CExplosionGenerator*>  sfxExplGens;  //list of explosiongenerators for use in scripts

	return true;
}


/******************************************************************************/
/******************************************************************************/
