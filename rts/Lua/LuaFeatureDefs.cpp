#include "StdAfx.h"
// LuaFeatureDefs.cpp: implementation of the LuaFeatureDefs class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaFeatureDefs.h"

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
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/FileSystem.h"

using namespace std;


static ParamMap paramMap;

static bool InitParamMap();

static void PushFeatureDef(lua_State* L,
                           const FeatureDef* featureDef, int index);

// iteration routines
static int Next(lua_State* L);
static int Pairs(lua_State* L);

// FeatureDefs meta-table calls
static int FeatureDefTableIndex(lua_State* L);
static int FeatureDefTableNewIndex(lua_State* L);
static int FeatureDefTableMetatable(lua_State* L);

// FeatureDef
static int FeatureDefIndex(lua_State* L);
static int FeatureDefNewIndex(lua_State* L);
static int FeatureDefMetatable(lua_State* L);

// special access functions
static int FeatureDefToID(lua_State* L, const void* data);
static int DrawTypeString(lua_State* L, const void* data);


/******************************************************************************/
/******************************************************************************/

bool LuaFeatureDefs::PushEntries(lua_State* L)
{
	if (paramMap.empty()) {
	  InitParamMap();
	}

	lua_newtable(L); { // the metatable
		HSTR_PUSH_CFUNC(L, "__index",     FeatureDefTableIndex);
		HSTR_PUSH_CFUNC(L, "__newindex",  FeatureDefTableNewIndex);
		HSTR_PUSH_CFUNC(L, "__metatable", FeatureDefTableMetatable);
	}
	lua_setmetatable(L, -2);

	return true;
}


bool LuaFeatureDefs::IsDefaultParam(const std::string& word)
{
	if (paramMap.empty()) {
	  InitParamMap();
	}
	return (paramMap.find(word) != paramMap.end());
}


/******************************************************************************/

static void PushFeatureDef(lua_State* L, const FeatureDef* fd, int index)
{
	lua_pushnumber(L, fd->id);
	lua_newtable(L); { // the proxy table

		lua_newtable(L); { // the metatable

			HSTR_PUSH(L, "__index");
			lua_pushlightuserdata(L, (void*)fd);
			lua_pushcclosure(L, FeatureDefIndex, 1);
			lua_rawset(L, -3); // closure 

			HSTR_PUSH(L, "__newindex");
			lua_pushlightuserdata(L, (void*)fd);
			lua_pushcclosure(L, FeatureDefNewIndex, 1);
			lua_rawset(L, -3);

			HSTR_PUSH(L, "__metatable");
			lua_pushlightuserdata(L, (void*)fd);
			lua_pushcclosure(L, FeatureDefMetatable, 1);
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

	lua_rawset(L, index); // proxy table into FeatureDefs
}


static int FeatureDefTableIndex(lua_State* L)
{
	const FeatureDef* fd = NULL;
	if (lua_isnumber(L, 2)) {
		const int id = (int)lua_tonumber(L, 2);
		lua_rawgeti(L, 1, id);
		if (lua_istable(L, -1)) {
			return 1; // already exists
		} else {
			lua_pop(L, 1);
		}
		fd = featureHandler->GetFeatureDefByID(id);
	}
	else if (lua_isstring(L, 2)) {
		const string name = lua_tostring(L, 2);
		fd = featureHandler->GetFeatureDef(name);
		if (fd != NULL) {
			lua_rawgeti(L, 1, fd->id);
			if (lua_istable(L, -1)) {
				return 1;
			} else {
				lua_pop(L, 1);
			}
		}
	}

	if (fd == NULL) {
		return 0; // featureDef does not exist
	}

	// create a new table and return it	
	PushFeatureDef(L, fd, 1);
	lua_rawgeti(L, 1, fd->id);
	return 1;
}


static int FeatureDefTableNewIndex(lua_State* L)
{
	luaL_error(L, "ERROR: can not write to FeatureDefs");
	return 0;
}


static int FeatureDefTableMetatable(lua_State* L)
{
	luaL_error(L, "ERROR: attempt to FeatureDefs metatable");
	return 0;
}


/******************************************************************************/

static int FeatureDefIndex(lua_State* L)
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
	const FeatureDef* ud = (const FeatureDef*)userData;
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
			luaL_error(L, "ERROR_TYPE in FeatureDefs __index");
		}
	}
	return 0;
}


static int FeatureDefNewIndex(lua_State* L)
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
	const FeatureDef* ud = (const FeatureDef*)userData;

	// write-protected
	if (!gs->editDefsEnabled) {
		luaL_error(L, "Attempt to write FeatureDefs[%d].%s", ud->id, name);
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
			luaL_error(L, "ERROR_TYPE in FeatureDefs __newindex");
		}
	}
	
	return 0;
}


static int FeatureDefMetatable(lua_State* L)
{
	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const FeatureDef* ud = (const FeatureDef*)userData;
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

static int DrawTypeString(lua_State* L, const void* data)
{
	const int drawType = *((const int*)data);
	switch (drawType) {
		case DRAWTYPE_3DO:  { HSTR_PUSH(L,     "3do"); break; }
		case DRAWTYPE_TREE: { HSTR_PUSH(L,    "tree"); break; }
		case DRAWTYPE_NONE: { HSTR_PUSH(L,    "none"); break; }
		default:            { HSTR_PUSH(L, "unknown"); break; }
	}
	return 1;
}


static int FeatureDefToID(lua_State* L, const void* data)
{
	const FeatureDef* ud = *((const FeatureDef**)data);
	if (ud == NULL) {
		return 0;
	}
	lua_pushnumber(L, ud->id);
	return 1;
}

#define DRAWTYPE_3DO 0
#define DRAWTYPE_TREE 1
#define DRAWTYPE_NONE -1



/******************************************************************************/
/******************************************************************************/

static bool InitParamMap()
{
	paramMap["next"]  = DataElement(READONLY_TYPE); 
	paramMap["pairs"] = DataElement(READONLY_TYPE); 

	// dummy FeatureDef for address lookups
	const FeatureDef fd;
	const char* start = ADDRESS(fd);
	
	ADD_FUNCTION("drawTypeString",  fd.drawType, DrawTypeString);
  
	ADD_INT("id", fd.id);

	ADD_STRING("name",    fd.myName);
	ADD_STRING("tooltip", fd.description);

	ADD_FLOAT("metal",     fd.metal);
	ADD_FLOAT("energy",    fd.energy);
	ADD_FLOAT("maxHealth", fd.maxHealth);

	ADD_FLOAT("radius", fd.radius);
	ADD_FLOAT("mass", fd.mass);

	ADD_INT("xsize", fd.xsize);
	ADD_INT("ysize", fd.ysize);
	
	ADD_FLOAT("hitSphereScale",    fd.collisionSphereScale);
	ADD_FLOAT("hitSphereOffsetX",  fd.collisionSphereOffset.x);
	ADD_FLOAT("hitSphereOffsetY",  fd.collisionSphereOffset.y);
	ADD_FLOAT("hitSphereOffsetZ",  fd.collisionSphereOffset.z);
	ADD_BOOL("useHitSphereOffset", fd.useCSOffset);

	ADD_INT("drawType",     fd.drawType);
	ADD_INT("modelType",    fd.modelType);
	ADD_STRING("modelname", fd.modelname);

//FIXME	S3DOModel* model;      //used by 3do obects

	ADD_BOOL("upright",      fd.upright);
	ADD_BOOL("destructable", fd.destructable);
	ADD_BOOL("reclaimable",  fd.reclaimable);
	ADD_BOOL("blocking",     fd.blocking);
	ADD_BOOL("burnable",     fd.burnable);
	ADD_BOOL("floating",     fd.floating);
	ADD_BOOL("geoThermal",   fd.geoThermal);

	// name of feature that this turn into when killed (not reclaimed)
	ADD_STRING("deathFeature", fd.deathFeature);

	return true;
}


/******************************************************************************/
/******************************************************************************/
