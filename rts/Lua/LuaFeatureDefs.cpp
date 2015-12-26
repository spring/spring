/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <set>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cctype>

#include "LuaFeatureDefs.h"

#include "LuaInclude.h"

#include "Sim/Misc/GlobalSynced.h"
#include "LuaDefs.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
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
static int DrawTypeString(lua_State* L, const void* data);
static int CustomParamsTable(lua_State* L, const void* data);


/******************************************************************************/
/******************************************************************************/

bool LuaFeatureDefs::PushEntries(lua_State* L)
{
	InitParamMap();

	typedef int (*IndxFuncType)(lua_State*);
	typedef int (*IterFuncType)(lua_State*);

	const auto& defsMap = featureHandler->GetFeatureDefs();

	const std::array<const LuaHashString, 3> indxOpers = {{
		LuaHashString("__index"),
		LuaHashString("__newindex"),
		LuaHashString("__metatable")
	}};
	const std::array<const LuaHashString, 2> iterOpers = {{
		LuaHashString("pairs"),
		LuaHashString("next")
	}};

	const std::array<const IndxFuncType, 3> indxFuncs = {FeatureDefIndex, FeatureDefNewIndex, FeatureDefMetatable};
	const std::array<const IterFuncType, 2> iterFuncs = {Pairs, Next};

	for (auto it = defsMap.cbegin(); it != defsMap.cend(); ++it) {
		const auto def = featureHandler->GetFeatureDefByID(it->second); // ObjectDefMapType::mapped_type

		if (def == NULL)
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
			LOG_L(L_ERROR, "[%s] ERROR_TYPE for key \"%s\" in FeatureDefs __index", __FUNCTION__, name);
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
			LOG_L(L_ERROR, "[%s] ERROR_TYPE for key \"%s\" in FeatureDefs __newindex", __FUNCTION__, name);
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

static int DrawTypeString(lua_State* L, const void* data)
{
	const int drawType = *((const int*)data);
	switch (drawType) {
		case DRAWTYPE_MODEL: { HSTR_PUSH(L,   "model"); break; }
		case DRAWTYPE_NONE:  { HSTR_PUSH(L,    "none"); break; }

		default: {
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
		lua_pushsstring(L, it->first);
		lua_pushsstring(L, it->second);
		lua_rawset(L, -3);
	}
	return 1;
}


static int ModelHeight(lua_State* L, const void* data)
{
	const FeatureDef* fd = static_cast<const FeatureDef*>(data);
	const S3DModel* model = NULL;
	float height = 0.0f;

	switch (fd->drawType) {
		case DRAWTYPE_MODEL: {
			model = fd->LoadModel();
			height = model? model->height: 0.0f;
			break;
		}
		case DRAWTYPE_NONE: {
			height = 0.0f;
			break;
		}
		default: {
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
	const FeatureDef* fd = static_cast<const FeatureDef*>(data);
	const S3DModel* model = NULL;
	float radius = 0.0f;

	switch (fd->drawType) {
		case DRAWTYPE_MODEL: {
			model = fd->LoadModel();
			radius = model? model->radius: 0.0f;
			break;
		}
		case DRAWTYPE_NONE: {
			radius = 0.0f;
			break;
		}
		default: {
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


static int ModelName(lua_State* L, const void* data)
{
	const FeatureDef* fd = static_cast<const FeatureDef*>(data);
	lua_pushsstring(L, modelParser->FindModelPath(fd->modelName));
	return 1;
}


static int ColVolTable(lua_State* L, const void* data) {
	auto cv = static_cast<const CollisionVolume*>(data);
	assert(cv != NULL);

	lua_newtable(L);
	switch (cv->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID:
			HSTR_PUSH_STRING(L, "type", "ellipsoid");
			break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER:
			HSTR_PUSH_STRING(L, "type", "cylinder");
			break;
		case CollisionVolume::COLVOL_TYPE_BOX:
			HSTR_PUSH_STRING(L, "type", "box");
			break;
		case CollisionVolume::COLVOL_TYPE_SPHERE:
			HSTR_PUSH_STRING(L, "type", "sphere");
			break;
	}

	LuaPushNamedNumber(L, "scaleX", cv->GetScales().x);
	LuaPushNamedNumber(L, "scaleY", cv->GetScales().y);
	LuaPushNamedNumber(L, "scaleZ", cv->GetScales().z);
	LuaPushNamedNumber(L, "offsetX", cv->GetOffsets().x);
	LuaPushNamedNumber(L, "offsetY", cv->GetOffsets().y);
	LuaPushNamedNumber(L, "offsetZ", cv->GetOffsets().z);
	LuaPushNamedNumber(L, "boundingRadius", cv->GetBoundingRadius());
	LuaPushNamedBool(L, "defaultToSphere",    cv->DefaultToSphere());
	LuaPushNamedBool(L, "defaultToFootPrint", cv->DefaultToFootPrint());
	LuaPushNamedBool(L, "defaultToPieceTree", cv->DefaultToPieceTree());
	return 1;
}


#define TYPE_MODEL_FUNC(name, param)                            \
	static int Model ## name(lua_State* L, const void* data)    \
	{                                                           \
		const FeatureDef* fd = static_cast<const FeatureDef*>(data); \
		if (fd->drawType == DRAWTYPE_MODEL) {                   \
			const S3DModel* model = fd->LoadModel();            \
			lua_pushnumber(L, model? model -> param : 0.0f);    \
			return 1;                                           \
		}                                                       \
		return 0;                                               \
	}

//TYPE_MODEL_FUNC(Height, height); // ::ModelHeight()
//TYPE_MODEL_FUNC(Radius, radius); // ::ModelRadius()
TYPE_MODEL_FUNC(Minx, mins.x)
TYPE_MODEL_FUNC(Midx, relMidPos.x)
TYPE_MODEL_FUNC(Maxx, maxs.x)
TYPE_MODEL_FUNC(Miny, mins.y)
TYPE_MODEL_FUNC(Midy, relMidPos.y)
TYPE_MODEL_FUNC(Maxy, maxs.y)
TYPE_MODEL_FUNC(Minz, mins.z)
TYPE_MODEL_FUNC(Midz, relMidPos.z)
TYPE_MODEL_FUNC(Maxz, maxs.z)


/******************************************************************************/
/******************************************************************************/

static bool InitParamMap()
{
	paramMap.clear();

	paramMap["next"]  = DataElement(READONLY_TYPE);
	paramMap["pairs"] = DataElement(READONLY_TYPE);

	// dummy FeatureDef for address lookups
	const FeatureDef fd;
	const char* start = ADDRESS(fd);

	ADD_FUNCTION("collisionVolume", fd.collisionVolume, ColVolTable);

	ADD_FUNCTION("drawTypeString", fd.drawType,     DrawTypeString);

	ADD_FUNCTION("customParams",   fd.customParams, CustomParamsTable);

	// TODO: share the Model* functions between LuaUnitDefs and LuaFeatureDefs
	// ADD_FUNCTION("model",   fd, ModelTable);
	ADD_FUNCTION("height",    fd, ModelHeight);
	ADD_FUNCTION("radius",    fd, ModelRadius);
	ADD_FUNCTION("minx",      fd, ModelMinx);
	ADD_FUNCTION("midx",      fd, ModelMidx);
	ADD_FUNCTION("maxx",      fd, ModelMaxx);
	ADD_FUNCTION("miny",      fd, ModelMiny);
	ADD_FUNCTION("midy",      fd, ModelMidy);
	ADD_FUNCTION("maxy",      fd, ModelMaxy);
	ADD_FUNCTION("minz",      fd, ModelMinz);
	ADD_FUNCTION("midz",      fd, ModelMidz);
	ADD_FUNCTION("maxz",      fd, ModelMaxz);
	ADD_FUNCTION("modelname", fd, ModelName);

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

	ADD_DEPRECATED_LUADEF_KEY("deathFeature");

	return true;
}


/******************************************************************************/
/******************************************************************************/
