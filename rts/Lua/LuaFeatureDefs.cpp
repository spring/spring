#include "StdAfx.h"
// LuaFeatureDefs.cpp: implementation of the LuaFeatureDefs class.
//
//////////////////////////////////////////////////////////////////////

#include <set>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cctype>

#include "mmgr.h"

#include "LuaFeatureDefs.h"

#include "LuaInclude.h"

#include "Sim/Misc/GlobalSynced.h"
#include "LuaDefs.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "LogOutput.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"

using namespace std;


#define TREE_RADIUS 20  // copied from Feature.cpp


static ParamMap paramMap;

static bool InitParamMap();

//static void PushFeatureDef(lua_State* L, const FeatureDef* featureDef, int index);

// iteration routines
static int Next(lua_State* L);
static int Pairs(lua_State* L);

// FeatureDef
static int FeatureDefIndex(lua_State* L);
static int FeatureDefNewIndex(lua_State* L);
static int FeatureDefMetatable(lua_State* L);

// special access functions
static int DrawTypeString(lua_State* L, const void* data);
static int CustomParamsTable(lua_State* L, const void* data);


/******************************************************************************/
/******************************************************************************/

bool LuaFeatureDefs::PushEntries(lua_State* L)
{
	if (paramMap.empty()) {
	  InitParamMap();
	}

	const map<string, const FeatureDef*>& featureDefs =
		featureHandler->GetFeatureDefs();
	map<string, const FeatureDef*>::const_iterator fdIt;
	for (fdIt = featureDefs.begin(); fdIt != featureDefs.end(); fdIt++) {
	  const FeatureDef* fd = fdIt->second;
		if (fd == NULL) {
	  	continue;
		}
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

		lua_rawset(L, -3); // proxy table into FeatureDefs
	}

	return true;
}


bool LuaFeatureDefs::IsDefaultParam(const string& word)
{
	if (paramMap.empty()) {
	  InitParamMap();
	}
	return (paramMap.find(word) != paramMap.end());
}


/******************************************************************************/

/*static void PushFeatureDef(lua_State* L, const FeatureDef* fd, int index)
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
}*/


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
	if (it == paramMap.end()) {
	  lua_rawget(L, 1);
	  return 1;
	}

	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const FeatureDef* fd = (const FeatureDef*)userData;
	const DataElement& elem = it->second;
	const char* p = ((const char*)fd) + elem.offset;
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
	if (it == paramMap.end()) {
		lua_rawset(L, 1);
		return 0;
	}

	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const FeatureDef* fd = (const FeatureDef*)userData;

	// write-protected
	if (!gs->editDefsEnabled) {
		luaL_error(L, "Attempt to write FeatureDefs[%d].%s", fd->id, name);
		return 0;
	}

	// Definition editing
	const DataElement& elem = it->second;
	const char* p = ((const char*)fd) + elem.offset;

	switch (elem.type) {
		case FUNCTION_TYPE:
		case READONLY_TYPE: {
			luaL_error(L, "Can not write to %s", name);
			return 0;
		}
		case INT_TYPE: {
			*((int*)p) = lua_toint(L, -1);
			return 0;
		}
		case BOOL_TYPE: {
			*((bool*)p) = lua_toboolean(L, -1);
			return 0;
		}
		case FLOAT_TYPE: {
			*((float*)p) = lua_tofloat(L, -1);
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
	//const FeatureDef* fd = (const FeatureDef*)userData;
	return 0;
}


/******************************************************************************/

static int Next(lua_State* L)
{
	return LuaUtils::Next(paramMap, L);
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
		case DRAWTYPE_MODEL: { HSTR_PUSH(L,   "model"); break; }
		case DRAWTYPE_NONE:  { HSTR_PUSH(L,    "none"); break; }
		default:             {
			if (drawType >= DRAWTYPE_TREE) {
				HSTR_PUSH(L,    "tree");
			} else {
				HSTR_PUSH(L, "unknown");
			}
			break;
		}
	}
	return 1;
}


static int CustomParamsTable(lua_State* L, const void* data)
{
	const map<string, string>& params = *((const map<string, string>*)data);
	lua_newtable(L);
	map<string, string>::const_iterator it;
	for (it = params.begin(); it != params.end(); ++it) {
		lua_pushstring(L, it->first.c_str());
		lua_pushstring(L, it->second.c_str());
		lua_rawset(L, -3);
	}
	return 1;
}


static int ModelHeight(lua_State* L, const void* data)
{
	const FeatureDef* fd = ((const FeatureDef*)data);
	float height = 0.0f;
	switch (fd->drawType) {
		case DRAWTYPE_MODEL: { height = LoadModel(fd)->height;  break; }
		case DRAWTYPE_NONE:  { height = 0.0f;                   break; }
		default:             {
			if (fd->drawType >= DRAWTYPE_TREE) {
				height = TREE_RADIUS * 2.0f;
			} else {
				height = 0.0f;
			}
			break;
		}
	}
	lua_pushnumber(L, height);
	return 1;
}


static int ModelRadius(lua_State* L, const void* data)
{
	const FeatureDef* fd = ((const FeatureDef*)data);
	float radius = 0.0f;
	switch (fd->drawType) {
		case DRAWTYPE_MODEL: { radius = LoadModel(fd)->radius;  break; }
		case DRAWTYPE_NONE:  { radius = 0.0f;                   break; }
		default:             {
			if (fd->drawType >= DRAWTYPE_TREE) {
				radius = TREE_RADIUS;
			} else {
				radius = 0.0f;
			}
			break;
		}
	}
	lua_pushnumber(L, radius);
	return 1;
}


#define TYPE_MODEL_FUNC(name, param)                       \
	static int Model ## name(lua_State* L, const void* data) \
	{                                                        \
		const FeatureDef* fd = ((const FeatureDef*)data);     \
		if (fd->drawType == DRAWTYPE_MODEL) {                     \
			const S3DModel* model = LoadModel(fd);            \
			lua_pushnumber(L, model -> param);                   \
			return 1;                                            \
		}                                                      \
		return 0;                                              \
	}

//TYPE_MODEL_FUNC(Height, height);
//TYPE_MODEL_FUNC(Radius, radius);
TYPE_MODEL_FUNC(Minx,   minx);
TYPE_MODEL_FUNC(Midx,   relMidPos.x);
TYPE_MODEL_FUNC(Maxx,   maxx);
TYPE_MODEL_FUNC(Miny,   miny);
TYPE_MODEL_FUNC(Midy,   relMidPos.y);
TYPE_MODEL_FUNC(Maxy,   maxy);
TYPE_MODEL_FUNC(Minz,   minz);
TYPE_MODEL_FUNC(Midz,   relMidPos.z);
TYPE_MODEL_FUNC(Maxz,   maxz);


/******************************************************************************/
/******************************************************************************/

static bool InitParamMap()
{
	paramMap["next"]  = DataElement(READONLY_TYPE);
	paramMap["pairs"] = DataElement(READONLY_TYPE);

	// dummy FeatureDef for address lookups
	const FeatureDef fd;
	const char* start = ADDRESS(fd);

	ADD_FUNCTION("drawTypeString", fd.drawType,     DrawTypeString);

	ADD_FUNCTION("customParams",   fd.customParams, CustomParamsTable);

	ADD_FUNCTION("height",  fd, ModelHeight);
	ADD_FUNCTION("radius",  fd, ModelRadius);
	ADD_FUNCTION("minx",    fd, ModelMinx);
	ADD_FUNCTION("midx",    fd, ModelMidx);
	ADD_FUNCTION("maxx",    fd, ModelMaxx);
	ADD_FUNCTION("miny",    fd, ModelMiny);
	ADD_FUNCTION("midy",    fd, ModelMidy);
	ADD_FUNCTION("maxy",    fd, ModelMaxy);
	ADD_FUNCTION("minz",    fd, ModelMinz);
	ADD_FUNCTION("midz",    fd, ModelMidz);
	ADD_FUNCTION("maxz",    fd, ModelMaxz);

	ADD_INT("id", fd.id);

	ADD_STRING("name",     fd.myName);
	ADD_STRING("tooltip",  fd.description);
	ADD_STRING("filename", fd.filename);

	ADD_FLOAT("metal",       fd.metal);
	ADD_FLOAT("energy",      fd.energy);
	ADD_FLOAT("maxHealth",   fd.maxHealth);
	ADD_FLOAT("reclaimTime", fd.reclaimTime);

	ADD_FLOAT("mass", fd.mass);

	ADD_INT("xsize", fd.xsize);
	ADD_INT("zsize", fd.zsize);

	/*
	ADD_FLOAT("hitSphereScale",    fd.collisionSphereScale);
	ADD_FLOAT("hitSphereOffsetX",  fd.collisionSphereOffset.x);
	ADD_FLOAT("hitSphereOffsetY",  fd.collisionSphereOffset.y);
	ADD_FLOAT("hitSphereOffsetZ",  fd.collisionSphereOffset.z);
	ADD_BOOL("useHitSphereOffset", fd.useCSOffset);
	*/

	ADD_INT("drawType",     fd.drawType);
	ADD_STRING("modelname", fd.modelname);

	ADD_BOOL("upright",      fd.upright);
	ADD_BOOL("destructable", fd.destructable);
	ADD_BOOL("reclaimable",  fd.reclaimable);
	ADD_BOOL("blocking",     fd.blocking);
	ADD_BOOL("burnable",     fd.burnable);
	ADD_BOOL("floating",     fd.floating);
	ADD_BOOL("geoThermal",   fd.geoThermal);
	ADD_BOOL("noSelect",     fd.noSelect);
	ADD_INT("resurrectable", fd.resurrectable);

	ADD_INT("smokeTime",    fd.smokeTime);

	// name of feature that this turn into when killed (not reclaimed)
	ADD_STRING("deathFeature", fd.deathFeature);

	return true;
}


/******************************************************************************/
/******************************************************************************/
