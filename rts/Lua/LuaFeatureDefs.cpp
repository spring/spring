/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <vector>
#include <cctype>

#include "LuaFeatureDefs.h"

#include "LuaInclude.h"
#include "LuaDefs.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Log/ILog.h"


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
static int CustomParamsTable(lua_State* L, const void* data);


/******************************************************************************/
/******************************************************************************/

bool LuaFeatureDefs::PushEntries(lua_State* L)
{
	InitParamMap();

	typedef int (*IndxFuncType)(lua_State*);
	typedef int (*IterFuncType)(lua_State*);

	const auto& defsVec = featureDefHandler->GetFeatureDefsVec();

	const std::array<const LuaHashString, 3> indxOpers = {{
		LuaHashString("__index"),
		LuaHashString("__newindex"),
		LuaHashString("__metatable")
	}};
	const std::array<const LuaHashString, 2> iterOpers = {{
		LuaHashString("pairs"),
		LuaHashString("next")
	}};

	const std::array<const IndxFuncType, 3> indxFuncs = {{FeatureDefIndex, FeatureDefNewIndex, FeatureDefMetatable}};
	const std::array<const IterFuncType, 2> iterFuncs = {{Pairs, Next}};

	for (const auto& element : defsVec) {
		const auto def = featureDefHandler->GetFeatureDefByID(element.id); // ObjectDefMapType::mapped_type

		if (def == nullptr)
			continue;

		PushObjectDefProxyTable(L, indxOpers, iterOpers, indxFuncs, iterFuncs, def);
	}

	return true;
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
	const FeatureDef* fd = static_cast<const FeatureDef*>(userData);
	const DataElement& elem = it->second;
	const void* p = ((const char*)fd) + elem.offset;
	switch (elem.type) {
		case READONLY_TYPE: {
			lua_rawget(L, 1);
			return 1;
		}
		case INT_TYPE: {
			lua_pushnumber(L, *reinterpret_cast<const int*>(p));
			return 1;
		}
		case BOOL_TYPE: {
			lua_pushboolean(L, *reinterpret_cast<const bool*>(p));
			return 1;
		}
		case FLOAT_TYPE: {
			lua_pushnumber(L, *reinterpret_cast<const float*>(p));
			return 1;
		}
		case STRING_TYPE: {
			lua_pushsstring(L, *reinterpret_cast<const std::string*>(p));
			return 1;
		}
		case FUNCTION_TYPE: {
			return elem.func(L, p);
		}
		case ERROR_TYPE: {
			LOG_L(L_ERROR, "[%s] ERROR_TYPE for key \"%s\" in FeatureDefs __index", __func__, name);
			lua_pushnil(L);
			return 1;
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
	const FeatureDef* fd = static_cast<const FeatureDef*>(userData);

	// write-protected
	if (!gs->editDefsEnabled) {
		luaL_error(L, "Attempt to write FeatureDefs[%d].%s", fd->id, name);
		return 0;
	}

	// Definition editing
	const DataElement& elem = it->second;
	const void* p = ((const char*)fd) + elem.offset;

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
		case ERROR_TYPE: {
			LOG_L(L_ERROR, "[%s] ERROR_TYPE for key \"%s\" in FeatureDefs __newindex", __func__, name);
			lua_pushnil(L);
			return 1;
		}
	}

	return 0;
}


static int FeatureDefMetatable(lua_State* L)
{
	/*const void* userData =*/ lua_touserdata(L, lua_upvalueindex(1));
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



static int CustomParamsTable(lua_State* L, const void* data)
{
	const spring::unordered_map<std::string, std::string>& params = *((const spring::unordered_map<std::string, std::string>*)data);
	lua_newtable(L);

	for (const auto& param: params) {
		lua_pushsstring(L, param.first);
		lua_pushsstring(L, param.second);
		lua_rawset(L, -3);
	}

	return 1;
}



static int ModelTable(lua_State* L, const void* data) {
	return (LuaUtils::PushModelTable(L, static_cast<const SolidObjectDef*>(data)));
}

static int ModelName(lua_State* L, const void* data) {
	return (LuaUtils::PushModelName(L, static_cast<const SolidObjectDef*>(data)));
}

static int ModelType(lua_State* L, const void* data) {
	return (LuaUtils::PushModelType(L, static_cast<const SolidObjectDef*>(data)));
}

static int ModelPath(lua_State* L, const void* data) {
	return (LuaUtils::PushModelPath(L, static_cast<const SolidObjectDef*>(data)));
}

static int ModelHeight(lua_State* L, const void* data) {
	return (LuaUtils::PushModelHeight(L, static_cast<const SolidObjectDef*>(data), false));
}

static int ModelRadius(lua_State* L, const void* data) {
	return (LuaUtils::PushModelRadius(L, static_cast<const SolidObjectDef*>(data), false));
}

static int ModelDrawType(lua_State* L, const void* data) {
	return (LuaUtils::PushFeatureModelDrawType(L, static_cast<const FeatureDef*>(data)));
}

static int ColVolTable(lua_State* L, const void* data) {
	return (LuaUtils::PushColVolTable(L, static_cast<const CollisionVolume*>(data)));
}



/******************************************************************************/
/******************************************************************************/

static bool InitParamMap()
{
	spring::clear_unordered_map(paramMap);

	paramMap["next"]  = DataElement(READONLY_TYPE);
	paramMap["pairs"] = DataElement(READONLY_TYPE);

	// dummy FeatureDef for address lookups
	const FeatureDef fd;
	const char* start = ADDRESS(fd);

	ADD_FUNCTION("model", fd, ModelTable);
	ADD_FUNCTION("collisionVolume", fd.collisionVolume, ColVolTable);
	ADD_FUNCTION("selectionVolume", fd.selectionVolume, ColVolTable);

	ADD_FUNCTION("modelname", fd, ModelName);
	ADD_FUNCTION("modeltype", fd, ModelType);
	ADD_FUNCTION("modelpath", fd, ModelPath);
	ADD_FUNCTION("height", fd, ModelHeight);
	ADD_FUNCTION("radius", fd, ModelRadius);
	ADD_FUNCTION("drawTypeString", fd, ModelDrawType);

	ADD_FUNCTION("customParams", fd.customParams, CustomParamsTable);

	ADD_INT("id", fd.id);
	ADD_INT("deathFeatureID", fd.deathFeatureDefID);

	ADD_STRING("name",     fd.name);
	ADD_STRING("tooltip",  fd.description);

	ADD_FLOAT("metal",       fd.metal);
	ADD_FLOAT("energy",      fd.energy);
	ADD_FLOAT("maxHealth",   fd.health);
	ADD_FLOAT("reclaimTime", fd.reclaimTime);

	ADD_FLOAT("mass", fd.mass);

	ADD_INT("xsize", fd.xsize);
	ADD_INT("zsize", fd.zsize);

	ADD_INT("drawType",     fd.drawType);

	ADD_BOOL("upright",      fd.upright);
	ADD_BOOL("destructable", fd.destructable);
	ADD_BOOL("reclaimable",  fd.reclaimable);
	ADD_BOOL("autoreclaim",  fd.autoreclaim);
	ADD_BOOL("blocking",     fd.collidable);
	ADD_BOOL("burnable",     fd.burnable);
	ADD_BOOL("floating",     fd.floating);
	ADD_BOOL("geoThermal",   fd.geoThermal);
	ADD_BOOL("noSelect",     fd.selectable);
	ADD_INT("resurrectable", fd.resurrectable);

	ADD_INT("smokeTime",    fd.smokeTime);

	ADD_DEPRECATED_LUADEF_KEY("minx");
	ADD_DEPRECATED_LUADEF_KEY("miny");
	ADD_DEPRECATED_LUADEF_KEY("minz");
	ADD_DEPRECATED_LUADEF_KEY("midx");
	ADD_DEPRECATED_LUADEF_KEY("midy");
	ADD_DEPRECATED_LUADEF_KEY("midz");
	ADD_DEPRECATED_LUADEF_KEY("maxx");
	ADD_DEPRECATED_LUADEF_KEY("maxy");
	ADD_DEPRECATED_LUADEF_KEY("maxz");
	ADD_DEPRECATED_LUADEF_KEY("deathFeature");

	return true;
}


/******************************************************************************/
/******************************************************************************/
