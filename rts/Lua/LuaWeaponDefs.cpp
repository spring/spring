#include "StdAfx.h"
// LuaWeaponDefs.cpp: implementation of the LuaWeaponDefs class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaWeaponDefs.h"

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
#include "Game/UI/SimpleParser.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/LogOutput.h"

using namespace std;


static ParamMap paramMap;

static bool InitParamMap();

// iteration routines
static int Next(lua_State* L);
static int Pairs(lua_State* L);

// meta-table calls
static int WeaponDefIndex(lua_State* L);
static int WeaponDefNewIndex(lua_State* L);
static int WeaponDefMetatable(lua_State* L);

// special access functions
static int NoFeatureCollide(lua_State* L, const void* data);
static int NoFriendlyCollide(lua_State* L, const void* data);
static int VisualsTable(lua_State* L, const void* data);
static int DamagesArray(lua_State* L, const void* data);
static int GuiSoundTable(lua_State* L, const void* data);
static int CategorySetFromBits(lua_State* L, const void* data);
static int CategorySetFromString(lua_State* L, const void* data);


/******************************************************************************/
/******************************************************************************/

bool LuaWeaponDefs::PushEntries(lua_State* L)
{
	if (paramMap.empty()) {
	  InitParamMap();
	}

	const std::map<std::string, int>& weaponMap = weaponDefHandler->weaponID;
	std::map<std::string, int>::const_iterator wit;
	for (wit = weaponMap.begin(); wit != weaponMap.end(); wit++) {
		const WeaponDef* wd = &weaponDefHandler->weaponDefs[wit->second];
		if (wd == NULL) {
	  	continue;
		}
		lua_pushnumber(L, wd->id);
		lua_newtable(L); { // the proxy table

			lua_newtable(L); { // the metatable

				HSTR_PUSH(L, "__index");
				lua_pushlightuserdata(L, (void*)wd);
				lua_pushcclosure(L, WeaponDefIndex, 1);
				lua_rawset(L, -3); // closure 

				HSTR_PUSH(L, "__newindex");
				lua_pushlightuserdata(L, (void*)wd);
				lua_pushcclosure(L, WeaponDefNewIndex, 1);
				lua_rawset(L, -3);

				HSTR_PUSH(L, "__metatable");
				lua_pushlightuserdata(L, (void*)wd);
				lua_pushcclosure(L, WeaponDefMetatable, 1);
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

		lua_rawset(L, -3); // proxy table into WeaponDefs
	}

	return true;
}


/******************************************************************************/

static int WeaponDefIndex(lua_State* L)
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
	const WeaponDef* wd = (const WeaponDef*)userData;
	const DataElement& elem = it->second;
	const char* p = ((const char*)wd) + elem.offset;
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
			luaL_error(L, "ERROR_TYPE in WeaponDefs __index");
		}
	}
	return 0;
}


static int WeaponDefNewIndex(lua_State* L)
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
	const WeaponDef* wd = (const WeaponDef*)userData;

	// write-protected
	if (!gs->editDefsEnabled) {
	 	luaL_error(L, "Attempt to write WeaponDefs[%d].%s", wd->id, name);
	 	return 0;
	}
	
	// Definition editing
	const DataElement& elem = it->second;
	const char* p = ((const char*)wd) + elem.offset;

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
			luaL_error(L, "ERROR_TYPE in WeaponDefs __newindex");
		}
	}
	
	return 0;
}


static int WeaponDefMetatable(lua_State* L)
{
	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const WeaponDef* wd = (const WeaponDef*)userData;
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

static int DamagesArray(lua_State* L, const void* data)
{
	// FIXME -- off by one?
	const DamageArray& d = *((const DamageArray*)data);
	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "impulseFactor",      d.impulseFactor);
	HSTR_PUSH_NUMBER(L, "impulseBoost",       d.impulseBoost);
	HSTR_PUSH_NUMBER(L, "craterMult",         d.craterMult);
	HSTR_PUSH_NUMBER(L, "craterBoost",        d.craterBoost);
	HSTR_PUSH_NUMBER(L, "paralyzeDamageTime", d.paralyzeDamageTime);

	HSTR_PUSH(L, "damages");
	lua_newtable(L);
	const std::vector<std::string>& typeList = damageArrayHandler->typeList;
	const int typeCount = (int)typeList.size();
	for (int i = 0; i < typeCount; i++) {
		LuaPushNamedNumber(L, typeList[i].c_str(), d.damages[i]);
	}
	lua_rawset(L, -3);
	
	return 1;		
}


static int VisualsTable(lua_State* L, const void* data)
{
	const struct WeaponDef::Visuals& v =
		*((const struct WeaponDef::Visuals*)data);
	lua_newtable(L);
	HSTR_PUSH_STRING(L, "modelName",   v.modelName);
	HSTR_PUSH_NUMBER(L, "renderType",  v.renderType);
	HSTR_PUSH_NUMBER(L, "colorR",      v.color.x);
	HSTR_PUSH_NUMBER(L, "colorG",      v.color.y);
	HSTR_PUSH_NUMBER(L, "colorB",      v.color.z);
	HSTR_PUSH_NUMBER(L, "color2R",     v.color2.x);
	HSTR_PUSH_NUMBER(L, "color2G",     v.color2.y);
	HSTR_PUSH_NUMBER(L, "color2B",     v.color2.z);
	HSTR_PUSH_BOOL  (L, "smokeTrail",  v.smokeTrail);
	HSTR_PUSH_BOOL  (L, "beamWeapon",  v.beamweapon);
	HSTR_PUSH_NUMBER(L, "tileLength",  v.tilelength);
	HSTR_PUSH_NUMBER(L, "scrollSpeed", v.scrollspeed);
	HSTR_PUSH_NUMBER(L, "pulseSpeed",  v.pulseSpeed);
	HSTR_PUSH_NUMBER(L, "noGap",       v.noGap);
	HSTR_PUSH_NUMBER(L, "stages",      v.stages);
	HSTR_PUSH_NUMBER(L, "sizeDecay",   v.sizeDecay);
	HSTR_PUSH_NUMBER(L, "alphaDecay",  v.alphaDecay);
	HSTR_PUSH_NUMBER(L, "separation",  v.separation);
	return 1;
//	CColorMap *colorMap;
//	AtlasedTexture *texture1;
//	AtlasedTexture *texture2;
//	AtlasedTexture *texture3;
//	AtlasedTexture *texture4;
}


static int NoFeatureCollide(lua_State* L, const void* data)
{
	const int bits = *((const int*)data);
	lua_pushboolean(L, (bits & COLLISION_NOFEATURE));
	return 1;
}


static int NoFriendlyCollide(lua_State* L, const void* data)
{
	const int bits = *((const int*)data);
	lua_pushboolean(L, (bits & COLLISION_NOFRIENDLY));
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


static int GuiSoundTable(lua_State* L, const void* data)
{
	const GuiSound& sound = *((const GuiSound*)data);
	lua_newtable(L);
	HSTR_PUSH_STRING(L, "name",   sound.name);
	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		HSTR_PUSH_NUMBER(L, "id",   sound.id);
	}
	HSTR_PUSH_NUMBER(L, "volume", sound.volume);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static bool InitParamMap()
{
	paramMap["next"]  = DataElement(READONLY_TYPE); 
	paramMap["pairs"] = DataElement(READONLY_TYPE); 
	
	// dummy WeaponDef for offset generation
	const WeaponDef wd;
	const char* start = ADDRESS(wd);

	ADD_FUNCTION("damages",   wd.damages,   DamagesArray);
	ADD_FUNCTION("visuals",   wd.visuals,   VisualsTable);
	ADD_FUNCTION("hitSound",  wd.soundhit,  GuiSoundTable);
	ADD_FUNCTION("fireSound", wd.firesound, GuiSoundTable);
	ADD_FUNCTION("noFeatureCollide",  wd.collisionFlags, NoFeatureCollide);
	ADD_FUNCTION("noFriendlyCollide", wd.collisionFlags, NoFriendlyCollide);
	ADD_FUNCTION("onlyTargetCategories",
	             wd.onlyTargetCategory, CategorySetFromBits);

	ADD_INT("id", wd.id);

	ADD_INT("tdfId", wd.tdfId);

	ADD_STRING("name", wd.name);
	ADD_STRING("type", wd.type);

	ADD_STRING("description", wd.description);

	ADD_FLOAT("range", wd.range);
	ADD_FLOAT("heightMod", wd.heightmod);
	ADD_FLOAT("accuracy", wd.accuracy);
	ADD_FLOAT("sprayAngle", wd.sprayangle);
	ADD_FLOAT("movingAccuracy", wd.movingAccuracy);
	ADD_FLOAT("targetMoveError", wd.targetMoveError);

	ADD_BOOL("noSelfDamage", wd.noSelfDamage);
	ADD_FLOAT("areaOfEffect", wd.areaOfEffect);
	ADD_FLOAT("fireStarter", wd.fireStarter);
	ADD_FLOAT("edgeEffectiveness", wd.edgeEffectiveness);
	ADD_FLOAT("size", wd.size);
	ADD_FLOAT("sizeGrowth", wd.sizeGrowth);
	ADD_FLOAT("collisionSize", wd.collisionSize);

	ADD_INT("salvoSize",    wd.salvosize);
	ADD_FLOAT("salvoDelay", wd.salvodelay);
	ADD_FLOAT("reload",     wd.reload);
	ADD_FLOAT("beamtime",   wd.beamtime);

	ADD_FLOAT("maxAngle", wd.maxAngle);
	ADD_FLOAT("restTime", wd.restTime);

	ADD_FLOAT("uptime", wd.uptime);

	ADD_FLOAT("metalCost",  wd.metalcost);
	ADD_FLOAT("energyCost", wd.energycost);
	ADD_FLOAT("supplyCost", wd.supplycost);

	ADD_BOOL("turret", wd.turret);
	ADD_BOOL("onlyForward", wd.onlyForward);
	ADD_BOOL("waterWeapon", wd.waterweapon);
	ADD_BOOL("tracks", wd.tracks);
	ADD_BOOL("dropped", wd.dropped);
	ADD_BOOL("paralyzer", wd.paralyzer);

	ADD_BOOL("noAutoTarget",   wd.noAutoTarget);
	ADD_BOOL("manualFire",     wd.manualfire);
	ADD_INT("targetable",      wd.targetable);
	ADD_BOOL("stockpile",      wd.stockpile);					
	ADD_INT("interceptor",     wd.interceptor);
	ADD_FLOAT("coverageRange", wd.coverageRange);

	ADD_FLOAT("intensity", wd.intensity);
	ADD_FLOAT("thickness", wd.thickness);
	ADD_FLOAT("laserFlareSize", wd.laserflaresize);
	ADD_FLOAT("coreThickness", wd.corethickness);
	ADD_FLOAT("duration", wd.duration);

	ADD_INT("graphicsType",  wd.graphicsType);
	ADD_BOOL("soundTrigger", wd.soundTrigger);

	ADD_BOOL("selfExplode", wd.selfExplode);
	ADD_BOOL("gravityAffected", wd.gravityAffected);
	ADD_BOOL("twoPhase", wd.twophase);
	ADD_BOOL("guided", wd.guided);
	ADD_BOOL("vlaunch", wd.vlaunch);
	ADD_BOOL("selfprop", wd.selfprop);
	ADD_BOOL("noExplode", wd.noExplode);
	ADD_FLOAT("startvelocity", wd.startvelocity);
	ADD_FLOAT("weaponAcceleration", wd.weaponacceleration);
	ADD_FLOAT("turnRate", wd.turnrate);
	ADD_FLOAT("maxVelocity", wd.maxvelocity);

	ADD_FLOAT("projectilespeed", wd.projectilespeed);
	ADD_FLOAT("explosionSpeed", wd.explosionSpeed);

	ADD_FLOAT("wobble", wd.wobble);
	ADD_FLOAT("trajectoryHeight", wd.trajectoryHeight);

	ADD_BOOL("largeBeamLaser", wd.largeBeamLaser);

	ADD_BOOL("isShield",                wd.isShield);
	ADD_BOOL("shieldRepulser",          wd.shieldRepulser);
	ADD_BOOL("smartShield",             wd.smartShield);
	ADD_BOOL("exteriorShield",          wd.exteriorShield);
	ADD_BOOL("visibleShield",           wd.visibleShield);
	ADD_BOOL("visibleShieldRepulse",    wd.visibleShieldRepulse);
	ADD_INT( "visibleShieldHitFrames",  wd.visibleShieldHitFrames);
	ADD_FLOAT("shieldEnergyUse",        wd.shieldEnergyUse);
	ADD_FLOAT("shieldRadius",           wd.shieldRadius);
	ADD_FLOAT("shieldForce",            wd.shieldForce);
	ADD_FLOAT("shieldMaxSpeed",         wd.shieldMaxSpeed);
	ADD_FLOAT("shieldPower",            wd.shieldPower);
	ADD_FLOAT("shieldPowerRegen",       wd.shieldPowerRegen);
	ADD_FLOAT("shieldPowerRegenEnergy", wd.shieldPowerRegenEnergy);
	ADD_FLOAT("shieldGoodColorR",       wd.shieldGoodColor.x);
	ADD_FLOAT("shieldGoodColorG",       wd.shieldGoodColor.y);
	ADD_FLOAT("shieldGoodColorB",       wd.shieldGoodColor.z);
	ADD_FLOAT("shieldBadColorR",        wd.shieldBadColor.x);
	ADD_FLOAT("shieldBadColorG",        wd.shieldBadColor.y);
	ADD_FLOAT("shieldBadColorB",        wd.shieldBadColor.z);
	ADD_FLOAT("shieldAlpha",            wd.shieldAlpha);

	// FIXME: these 2 are bit fields
	ADD_INT("shieldInterceptType",  wd.shieldInterceptType);
	ADD_INT("interceptedByShieldType",  wd.interceptedByShieldType);

	ADD_BOOL("avoidFriendly", wd.avoidFriendly);

//	CExplosionGenerator *explosionGenerator;

	ADD_BOOL("sweepFire", wd.sweepFire);

	ADD_BOOL("canAttackGround", wd.canAttackGround);

	return true;
}


/******************************************************************************/
/******************************************************************************/
