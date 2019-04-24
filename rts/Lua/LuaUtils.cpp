/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//#include "System/Platform/Win/win32.h"

#include <cstring>

#include "LuaUtils.h"
#include "LuaConfig.h"

#include "Game/GameVersion.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"
#include "System/StringUtil.h"

#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
	#include "System/TimeProfiler.h"
#else
	#define SCOPED_TIMER(x)
#endif


static const int maxDepth = 16;
int LuaUtils::exportedDataSize = 0;


/******************************************************************************/
/******************************************************************************/


static bool CopyPushData(lua_State* dst, lua_State* src, int index, int depth, spring::unsynced_map<const void*, int>& alreadyCopied);
static bool CopyPushTable(lua_State* dst, lua_State* src, int index, int depth, spring::unsynced_map<const void*, int>& alreadyCopied);


static inline int PosAbsLuaIndex(lua_State* src, int index)
{
	if (index > 0)
		return index;

	return (lua_gettop(src) + index + 1);
}


static bool CopyPushData(lua_State* dst, lua_State* src, int index, int depth, spring::unsynced_map<const void*, int>& alreadyCopied)
{
	switch (lua_type(src, index)) {
		case LUA_TBOOLEAN: {
			lua_pushboolean(dst, lua_toboolean(src, index));
		} break;

		case LUA_TNUMBER: {
			lua_pushnumber(dst, lua_tonumber(src, index));
		} break;

		case LUA_TSTRING: {
			// get string (pointer)
			size_t len;
			const char* data = lua_tolstring(src, index, &len);

			// check cache
			auto it = alreadyCopied.find(data);
			if (it != alreadyCopied.end()) {
				lua_rawgeti(dst, LUA_REGISTRYINDEX, it->second);
				break;
			}

			// copy string
			lua_pushlstring(dst, data, len);

			// cache it
			lua_pushvalue(dst, -1);
			const int dstRef = luaL_ref(dst, LUA_REGISTRYINDEX);
			alreadyCopied[data] = dstRef;
		} break;

		case LUA_TTABLE: {
			CopyPushTable(dst, src, index, depth, alreadyCopied);
		} break;

		default: {
			lua_pushnil(dst); // unhandled type
			return false;
		}
	}

	return true;
}


static bool CopyPushTable(lua_State* dst, lua_State* src, int index, int depth, spring::unsynced_map<const void*, int>& alreadyCopied)
{
	const int table = PosAbsLuaIndex(src, index);

	// check cache
	const void* p = lua_topointer(src, table);
	auto it = alreadyCopied.find(p);
	if (it != alreadyCopied.end()) {
		lua_rawgeti(dst, LUA_REGISTRYINDEX, it->second);
		return true;
	}

	// check table depth
	if (depth++ > maxDepth) {
		LOG("CopyTable: reached max table depth '%i'", depth);
		lua_pushnil(dst); // push something
		return false;
	}

	// create new table
	const auto array_len = lua_objlen(src, table);
	lua_createtable(dst, array_len, 5);

	// cache it
	lua_pushvalue(dst, -1);
	const int dstRef = luaL_ref(dst, LUA_REGISTRYINDEX);
	alreadyCopied[p] = dstRef;

	// copy table entries
	for (lua_pushnil(src); lua_next(src, table) != 0; lua_pop(src, 1)) {
		CopyPushData(dst, src, -2, depth, alreadyCopied); // copy the key
		CopyPushData(dst, src, -1, depth, alreadyCopied); // copy the value
		lua_rawset(dst, -3);
	}

	return true;
}


int LuaUtils::CopyData(lua_State* dst, lua_State* src, int count)
{
	SCOPED_TIMER("Lua::CopyData");

	const int srcTop = lua_gettop(src);
	const int dstTop = lua_gettop(dst);
	if (srcTop < count) {
		LOG_L(L_ERROR, "LuaUtils::CopyData: tried to copy more data than there is");
		return 0;
	}
	lua_checkstack(dst, count + 3); // +3 needed for table copying
	lua_lock(src); // we need to be sure tables aren't changed while we iterate them

	// hold a map of all already copied tables in the lua's registry table
	// needed for recursive tables, i.e. "local t = {}; t[t] = t"
	// the order of traversal doesn't matter so we can use an unsynced map
	spring::unsynced_map<const void*, int> alreadyCopied;

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;
	for (int i = startIndex; i <= endIndex; i++) {
		CopyPushData(dst, src, i, 0, alreadyCopied);
	}

	// clear map
	for (auto& pair: alreadyCopied) {
		luaL_unref(dst, LUA_REGISTRYINDEX, pair.second);
	}

	const int curSrcTop = lua_gettop(src);
	assert(srcTop == curSrcTop);
	lua_settop(dst, dstTop + count);
	lua_unlock(src);
	return count;
}

/******************************************************************************/
/******************************************************************************/

static bool BackupData(LuaUtils::DataDump& d, lua_State* src, int index, int depth);
static bool RestoreData(const LuaUtils::DataDump& d, lua_State* dst, int depth);
static bool BackupTable(LuaUtils::DataDump& d, lua_State* src, int index, int depth);
static bool RestoreTable(const LuaUtils::DataDump& d, lua_State* dst, int depth);


static bool BackupData(LuaUtils::DataDump& d, lua_State* src, int index, int depth) {
	++LuaUtils::exportedDataSize;
	const int type = lua_type(src, index);
	d.type = type;
	switch (type) {
		case LUA_TBOOLEAN: {
			d.bol = lua_toboolean(src, index);
			break;
		}
		case LUA_TNUMBER: {
			d.num = lua_tonumber(src, index);
			break;
		}
		case LUA_TSTRING: {
			size_t len = 0;
			const char* data = lua_tolstring(src, index, &len);
			if (len > 0) {
				d.str.resize(len);
				memcpy(&d.str[0], data, len);
			}
			break;
		}
		case LUA_TTABLE: {
			if (!BackupTable(d, src, index, depth))
				d.type = LUA_TNIL;
			break;
		}
		default: {
			d.type = LUA_TNIL;
			break;
		}
	}
	return true;
}

static bool RestoreData(const LuaUtils::DataDump& d, lua_State* dst, int depth) {
	--LuaUtils::exportedDataSize;

	switch (d.type) {
		case LUA_TBOOLEAN: {
			lua_pushboolean(dst, d.bol);
		} break;

		case LUA_TNUMBER: {
			lua_pushnumber(dst, d.num);
		} break;

		case LUA_TSTRING: {
			lua_pushlstring(dst, d.str.c_str(), d.str.size());
		} break;

		case LUA_TTABLE: {
			RestoreTable(d, dst, depth);
		} break;

		default: {
			lua_pushnil(dst);
		} break;
	}

	return true;
}

static bool BackupTable(LuaUtils::DataDump& d, lua_State* src, int index, int depth) {
	if (depth++ > maxDepth)
		return false;

	const int tableIdx = PosAbsLuaIndex(src, index);
	for (lua_pushnil(src); lua_next(src, tableIdx) != 0; lua_pop(src, 1)) {
		LuaUtils::DataDump dk, dv;
		BackupData(dk, src, -2, depth);
		BackupData(dv, src, -1, depth);
		d.table.emplace_back(dk ,dv);
	}

	return true;
}

static bool RestoreTable(const LuaUtils::DataDump& d, lua_State* dst, int depth) {
	if (depth++ > maxDepth) {
		lua_pushnil(dst);
		return false;
	}

	lua_newtable(dst);
	for (const auto& di: d.table) {
		RestoreData(di.first, dst, depth);
		RestoreData(di.second, dst, depth);
		lua_rawset(dst, -3);
	}

	return true;
}


int LuaUtils::Backup(std::vector<LuaUtils::DataDump>& backup, lua_State* src, int count) {
	const int srcTop = lua_gettop(src);
	if (srcTop < count)
		return 0;

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;
	for (int i = startIndex; i <= endIndex; i++) {
		backup.emplace_back();
		BackupData(backup.back(), src, i, 0);
	}

	return count;
}


int LuaUtils::Restore(const std::vector<LuaUtils::DataDump>& backup, lua_State* dst) {
	const int dstTop = lua_gettop(dst);
	int count = backup.size();
	lua_checkstack(dst, count + 3);

	for (const auto& dd: backup) {
		RestoreData(dd, dst, 0);
	}
	lua_settop(dst, dstTop + count);

	return count;
}


/******************************************************************************/
/******************************************************************************/

static void PushCurrentFunc(lua_State* L, const char* caller)
{
	// get the current function
	lua_Debug ar;
	if (lua_getstack(L, 1, &ar) == 0)
		luaL_error(L, "%s() lua_getstack() error", caller);

	if (lua_getinfo(L, "f", &ar) == 0)
		luaL_error(L, "%s() lua_getinfo() error", caller);

	if (!lua_isfunction(L, -1))
		luaL_error(L, "%s() invalid current function", caller);
}


static void PushFunctionEnv(lua_State* L, const char* caller, int funcIndex)
{
	lua_getfenv(L, funcIndex);
	lua_pushliteral(L, "__fenv");
	lua_rawget(L, -2);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1); // there is no fenv proxy
	} else {
		lua_remove(L, -2); // remove the orig table, leave the proxy
	}

	if (!lua_istable(L, -1)) {
		luaL_error(L, "%s() invalid fenv", caller);
	}
}


void LuaUtils::PushCurrentFuncEnv(lua_State* L, const char* caller)
{
	PushCurrentFunc(L, caller);
	PushFunctionEnv(L, caller, -1);
	lua_remove(L, -2); // remove the function
}

/******************************************************************************/
/******************************************************************************/

static void LowerKeysReal(lua_State* L, spring::unsynced_set<const void*>& checkedSet)
{
	luaL_checkstack(L, 8, __func__);

	const int  sourceTableIdx = lua_gettop(L);
	const int changedTableIdx = sourceTableIdx + 1;

	{
		const void* p = lua_topointer(L, sourceTableIdx);
		if (checkedSet.find(p) != checkedSet.end())
			return;

		checkedSet.insert(p);
	}

	// a new table for changed values
	lua_newtable(L);

	for (lua_pushnil(L); lua_next(L, sourceTableIdx) != 0; lua_pop(L, 1)) {
		if (lua_istable(L, -1))
			LowerKeysReal(L, checkedSet);

		if (!lua_israwstring(L, -2))
			continue;

		const string rawKey = lua_tostring(L, -2);
		const string lowerKey = StringToLower(rawKey);

		if (rawKey == lowerKey)
			continue;

		// removed the mixed case entry
		lua_pushvalue(L, -2); // the key
		lua_pushnil(L);
		lua_rawset(L, sourceTableIdx);
		// does the lower case key alread exist in the table?
		lua_pushsstring(L, lowerKey);
		lua_rawget(L, sourceTableIdx);

		if (lua_isnil(L, -1)) {
			// lower case does not exist, add it to the changed table
			lua_pushsstring(L, lowerKey);
			lua_pushvalue(L, -3); // the value
			lua_rawset(L, changedTableIdx);
		}

		lua_pop(L, 1);
	}

	// copy the changed values into the table
	for (lua_pushnil(L); lua_next(L, changedTableIdx) != 0; lua_pop(L, 1)) {
		lua_pushvalue(L, -2); // copy the key to the top
		lua_pushvalue(L, -2); // copy the value to the top
		lua_rawset(L, sourceTableIdx);
	}

	lua_pop(L, 1); // pop the changed table
}


bool LuaUtils::LowerKeys(lua_State* L, int table)
{
	if (!lua_istable(L, table))
		return false;

	// table of processed tables
	spring::unsynced_set<const void*> checkedSet;
	luaL_checkstack(L, 1, __func__);

	lua_pushvalue(L, table); // push the table onto the top of the stack
	LowerKeysReal(L, checkedSet);

	lua_pop(L, 1); // the lowered table, and the check table
	return true;
}


static bool CheckForNaNsReal(lua_State* L, const std::string& path)
{
	luaL_checkstack(L, 3, __func__);
	const int table = lua_gettop(L);
	bool foundNaNs = false;

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_istable(L, -1)) {
			// We can't work on -2 directly cause lua_tostring would replace the value in -2,
			// so we need to make a copy and convert that to a string.
			lua_pushvalue(L, -2);
			const char* key = lua_tostring(L, -1);
			const std::string subpath = path + key + ".";
			lua_pop(L, 1);

			foundNaNs |= CheckForNaNsReal(L, subpath);
			continue;
		}

		if (!lua_isnumber(L, -1))
			continue;

		// Check for NaN
		const float value = lua_tonumber(L, -1);
		if (!math::isinf(value) && !math::isnan(value))
			continue;

		// can't work on -2 directly (lua_tostring would replace the value)
		// so we need to make a copy and convert that to a string
		lua_pushvalue(L, -2);
		const char* key = lua_tostring(L, -1);
		LOG_L(L_WARNING, "%s%s: Got Invalid NaN/Inf!", path.c_str(), key);
		lua_pop(L, 1);

		foundNaNs = true;
	}

	return foundNaNs;
}


bool LuaUtils::CheckTableForNaNs(lua_State* L, int table, const std::string& name)
{
	if (!lua_istable(L, table))
		return false;

	luaL_checkstack(L, 2, __func__);

	// table of processed tables
	lua_newtable(L);
	// push the table onto the top of the stack
	lua_pushvalue(L, table);

	const bool foundNaNs = CheckForNaNsReal(L, name + ": ");

	lua_pop(L, 2); // the lowered table, and the check table

	return foundNaNs;
}


/******************************************************************************/
/******************************************************************************/

// copied from lua/src/lauxlib.cpp:luaL_checkudata()
void* LuaUtils::GetUserData(lua_State* L, int index, const string& type)
{
	const char* tname = type.c_str();
	void *p = lua_touserdata(L, index);
	if (p != nullptr) {                               // value is a userdata?
		if (lua_getmetatable(L, index)) {            // does it have a metatable?
			lua_getfield(L, LUA_REGISTRYINDEX, tname); // get correct metatable
			if (lua_rawequal(L, -1, -2)) {             // the correct mt?
				lua_pop(L, 2);                           // remove both metatables
				return p;
			}
		}
	}
	return nullptr;
}


/******************************************************************************/
/******************************************************************************/

void LuaUtils::PrintStack(lua_State* L)
{
	for (int i = 1, top = lua_gettop(L); i <= top; i++) {
		LOG_L(L_ERROR, "  %i: type = %s (%p)", i, luaL_typename(L, i), lua_topointer(L, i));

		switch (lua_type(L, i)) {
			case LUA_TSTRING:
				LOG_L(L_ERROR, "\t\t%s", lua_tostring(L, i));
				break;
			case LUA_TNUMBER:
				LOG_L(L_ERROR, "\t\t%f", lua_tonumber(L, i));
				break;
			case LUA_TBOOLEAN:
				LOG_L(L_ERROR, "\t\t%s", lua_toboolean(L, i) ? "true" : "false");
				break;
			default: {}
		}
	}
}


int LuaUtils::IsEngineMinVersion(lua_State* L)
{
	const int minMajorVer = luaL_checkint(L, 1);
	const int minMinorVer = luaL_optint(L, 2, 0);
	const int minCommits  = luaL_optint(L, 3, 0);

	if (StringToInt(SpringVersion::GetMajor()) < minMajorVer) {
		lua_pushboolean(L, false);
		return 1;
	}

	if (StringToInt(SpringVersion::GetMajor()) == minMajorVer) {
		if (StringToInt(SpringVersion::GetMinor()) < minMinorVer) {
			lua_pushboolean(L, false);
			return 1;
		}

		if (StringToInt(SpringVersion::GetCommits()) < minCommits) {
			lua_pushboolean(L, false);
			return 1;
		}
	}

	lua_pushboolean(L, true);
	return 1;

}

/******************************************************************************/
/******************************************************************************/

int LuaUtils::ParseIntArray(lua_State* L, int index, int* array, int size)
{
	if (!lua_istable(L, index))
		return -1;

	for (int i = 0, absIdx = PosAbsLuaIndex(L, index); i < size; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isnumber(L, -1)) {
			array[i] = lua_toint(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}

	return size;
}

int LuaUtils::ParseFloatArray(lua_State* L, int index, float* array, int size)
{
	if (!lua_istable(L, index))
		return -1;

	for (int i = 0, absIdx = PosAbsLuaIndex(L, index); i < size; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isnumber(L, -1)) {
			array[i] = lua_tofloat(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}

	return size;
}

int LuaUtils::ParseStringArray(lua_State* L, int index, string* array, int size)
{
	if (!lua_istable(L, index))
		return -1;

	for (int i = 0, absIdx = PosAbsLuaIndex(L, index); i < size; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isstring(L, -1)) {
			array[i] = lua_tostring(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}

	return size;
}

int LuaUtils::ParseIntVector(lua_State* L, int index, vector<int>& vec)
{
	if (!lua_istable(L, index))
		return -1;

	vec.clear();

	for (int i = 0, absIdx = PosAbsLuaIndex(L, index); ; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isnumber(L, -1)) {
			vec.push_back(lua_toint(L, -1));
			lua_pop(L, 1);
			continue;
		}

		lua_pop(L, 1);
		return i;
	}
}

int LuaUtils::ParseFloatVector(lua_State* L, int index, vector<float>& vec)
{
	if (!lua_istable(L, index))
		return -1;

	vec.clear();

	for (int i = 0, absIdx = PosAbsLuaIndex(L, index); ; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isnumber(L, -1)) {
			vec.push_back(lua_tofloat(L, -1));
			lua_pop(L, 1);
			continue;
		}

		lua_pop(L, 1);
		return i;
	}
}

int LuaUtils::ParseStringVector(lua_State* L, int index, vector<string>& vec)
{
	if (!lua_istable(L, index))
		return -1;

	vec.clear();

	for (int i = 0, absIdx = PosAbsLuaIndex(L, index); ; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isstring(L, -1)) {
			vec.emplace_back(lua_tostring(L, -1));
			lua_pop(L, 1);
			continue;
		}

		lua_pop(L, 1);
		return i;
	}
}


#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI


int LuaUtils::PushModelHeight(lua_State* L, const SolidObjectDef* def, bool isUnitDef)
{
	const S3DModel* model = nullptr;
	float height = 0.0f;

	if (isUnitDef) {
		model = def->LoadModel();
	} else {
		switch (static_cast<const FeatureDef*>(def)->drawType) {
			case DRAWTYPE_NONE: {
			} break;

			case DRAWTYPE_MODEL: {
				model = def->LoadModel();
			} break;

			default: {
				// always >= DRAWTYPE_TREE here
				height = TREE_RADIUS * 2.0f;
			} break;
		}
	}

	if (model != nullptr)
		height = model->height;

	lua_pushnumber(L, height);
	return 1;
}

int LuaUtils::PushModelRadius(lua_State* L, const SolidObjectDef* def, bool isUnitDef)
{
	const S3DModel* model = nullptr;
	float radius = 0.0f;

	if (isUnitDef) {
		model = def->LoadModel();
	} else {
		switch (static_cast<const FeatureDef*>(def)->drawType) {
			case DRAWTYPE_NONE: {
			} break;

			case DRAWTYPE_MODEL: {
				model = def->LoadModel();
			} break;

			default: {
				// always >= DRAWTYPE_TREE here
				radius = TREE_RADIUS;
			} break;
		}
	}

	if (model != nullptr)
		radius = model->radius;

	lua_pushnumber(L, radius);
	return 1;
}

int LuaUtils::PushFeatureModelDrawType(lua_State* L, const FeatureDef* def)
{
	switch (def->drawType) {
		case DRAWTYPE_NONE:  { HSTR_PUSH(L,  "none"); } break;
		case DRAWTYPE_MODEL: { HSTR_PUSH(L, "model"); } break;
		default:             { HSTR_PUSH(L,  "tree"); } break;
	}

	return 1;
}

int LuaUtils::PushModelName(lua_State* L, const SolidObjectDef* def)
{
	lua_pushsstring(L, def->modelName);
	return 1;
}

int LuaUtils::PushModelType(lua_State* L, const SolidObjectDef* def)
{
	const std::string& modelPath = modelLoader.FindModelPath(def->modelName);
	const std::string& modelType = StringToLower(FileSystem::GetExtension(modelPath));
	lua_pushsstring(L, modelType);
	return 1;
}

int LuaUtils::PushModelPath(lua_State* L, const SolidObjectDef* def)
{
	const std::string& modelPath = modelLoader.FindModelPath(def->modelName);
	lua_pushsstring(L, modelPath);
	return 1;
}


int LuaUtils::PushModelTable(lua_State* L, const SolidObjectDef* def) {

	const S3DModel* model = def->LoadModel();

	lua_newtable(L);

	if (model != nullptr) {
		// unit, or non-tree feature
		HSTR_PUSH_NUMBER(L, "minx", model->mins.x);
		HSTR_PUSH_NUMBER(L, "miny", model->mins.y);
		HSTR_PUSH_NUMBER(L, "minz", model->mins.z);
		HSTR_PUSH_NUMBER(L, "maxx", model->maxs.x);
		HSTR_PUSH_NUMBER(L, "maxy", model->maxs.y);
		HSTR_PUSH_NUMBER(L, "maxz", model->maxs.z);

		HSTR_PUSH_NUMBER(L, "midx", model->relMidPos.x);
		HSTR_PUSH_NUMBER(L, "midy", model->relMidPos.y);
		HSTR_PUSH_NUMBER(L, "midz", model->relMidPos.z);
	} else {
		HSTR_PUSH_NUMBER(L, "minx", 0.0f);
		HSTR_PUSH_NUMBER(L, "miny", 0.0f);
		HSTR_PUSH_NUMBER(L, "minz", 0.0f);
		HSTR_PUSH_NUMBER(L, "maxx", 0.0f);
		HSTR_PUSH_NUMBER(L, "maxy", 0.0f);
		HSTR_PUSH_NUMBER(L, "maxz", 0.0f);

		HSTR_PUSH_NUMBER(L, "midx", 0.0f);
		HSTR_PUSH_NUMBER(L, "midy", 0.0f);
		HSTR_PUSH_NUMBER(L, "midz", 0.0f);
	}

	HSTR_PUSH(L, "textures");
	lua_newtable(L);

	if (model != nullptr) {
		LuaPushNamedString(L, "tex1", model->texs[0]);
		LuaPushNamedString(L, "tex2", model->texs[1]);
	} else {
		// just leave these nil
	}

	// model["textures"] = {}
	lua_rawset(L, -3);

	return 1;
}

int LuaUtils::PushColVolTable(lua_State* L, const CollisionVolume* vol) {
	assert(vol != nullptr);

	lua_newtable(L);
	switch (vol->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID:
			HSTR_PUSH_CSTRING(L, "type", "ellipsoid");
			break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER:
			HSTR_PUSH_CSTRING(L, "type", "cylinder");
			break;
		case CollisionVolume::COLVOL_TYPE_BOX:
			HSTR_PUSH_CSTRING(L, "type", "box");
			break;
		case CollisionVolume::COLVOL_TYPE_SPHERE:
			HSTR_PUSH_CSTRING(L, "type", "sphere");
			break;
	}

	LuaPushNamedNumber(L, "scaleX", vol->GetScales().x);
	LuaPushNamedNumber(L, "scaleY", vol->GetScales().y);
	LuaPushNamedNumber(L, "scaleZ", vol->GetScales().z);
	LuaPushNamedNumber(L, "offsetX", vol->GetOffsets().x);
	LuaPushNamedNumber(L, "offsetY", vol->GetOffsets().y);
	LuaPushNamedNumber(L, "offsetZ", vol->GetOffsets().z);
	LuaPushNamedNumber(L, "boundingRadius", vol->GetBoundingRadius());
	LuaPushNamedBool(L, "defaultToSphere",    vol->DefaultToSphere());
	LuaPushNamedBool(L, "defaultToFootPrint", vol->DefaultToFootPrint());
	LuaPushNamedBool(L, "defaultToPieceTree", vol->DefaultToPieceTree());
	return 1;
}

int LuaUtils::PushColVolData(lua_State* L, const CollisionVolume* vol) {
	lua_pushnumber(L, vol->GetScales().x);
	lua_pushnumber(L, vol->GetScales().y);
	lua_pushnumber(L, vol->GetScales().z);
	lua_pushnumber(L, vol->GetOffsets().x);
	lua_pushnumber(L, vol->GetOffsets().y);
	lua_pushnumber(L, vol->GetOffsets().z);
	lua_pushnumber(L, vol->GetVolumeType());
	lua_pushnumber(L, int(vol->UseContHitTest()));
	lua_pushnumber(L, vol->GetPrimaryAxis());
	lua_pushboolean(L, vol->IgnoreHits());
	return 10;
}


int LuaUtils::ParseColVolData(lua_State* L, int idx, CollisionVolume* vol)
{
	const float xs = luaL_checkfloat(L, idx++);
	const float ys = luaL_checkfloat(L, idx++);
	const float zs = luaL_checkfloat(L, idx++);
	const float xo = luaL_checkfloat(L, idx++);
	const float yo = luaL_checkfloat(L, idx++);
	const float zo = luaL_checkfloat(L, idx++);
	const int vType = luaL_checkint (L, idx++);
	const int tType = luaL_checkint (L, idx++);
	const int pAxis = luaL_checkint (L, idx++);

	const float3 scales(xs, ys, zs);
	const float3 offsets(xo, yo, zo);

	vol->InitShape(scales, offsets, vType, tType, pAxis);
	return 0;
}


#endif //!defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI


void LuaUtils::PushCommandParamsTable(lua_State* L, const Command& cmd, bool subtable)
{
	if (subtable)
		HSTR_PUSH(L, "params");

	lua_createtable(L, cmd.GetNumParams(), 0);

	for (unsigned int p = 0; p < cmd.GetNumParams(); p++) {
		lua_pushnumber(L, cmd.GetParam(p));
		lua_rawseti(L, -2, p + 1);
	}

	if (subtable)
		lua_rawset(L, -3);
}

void LuaUtils::PushCommandOptionsTable(lua_State* L, const Command& cmd, bool subtable)
{
	if (subtable)
		HSTR_PUSH(L, "options");

	lua_createtable(L, 0, 7);
	HSTR_PUSH_NUMBER(L, "coded", cmd.GetOpts());
	HSTR_PUSH_BOOL(L, "alt",      !!(cmd.GetOpts() & ALT_KEY        ));
	HSTR_PUSH_BOOL(L, "ctrl",     !!(cmd.GetOpts() & CONTROL_KEY    ));
	HSTR_PUSH_BOOL(L, "shift",    !!(cmd.GetOpts() & SHIFT_KEY      ));
	HSTR_PUSH_BOOL(L, "right",    !!(cmd.GetOpts() & RIGHT_MOUSE_KEY));
	HSTR_PUSH_BOOL(L, "meta",     !!(cmd.GetOpts() & META_KEY       ));
	HSTR_PUSH_BOOL(L, "internal", !!(cmd.GetOpts() & INTERNAL_ORDER ));

	if (subtable)
		lua_rawset(L, -3);
}

void LuaUtils::PushUnitAndCommand(lua_State* L, const CUnit* unit, const Command& cmd)
{
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	lua_pushnumber(L, cmd.GetID());

	PushCommandParamsTable(L, cmd, false);
	PushCommandOptionsTable(L, cmd, false);

	lua_pushnumber(L, cmd.GetTag());
}


static bool ParseCommandOptions(
	lua_State* L,
	Command& cmd,
	const char* caller,
	const int idx
) {
	if (lua_isnumber(L, idx)) {
		cmd.SetOpts(lua_tonumber(L, idx));
		return true;
	}

	if (lua_istable(L, idx)) {
		for (lua_pushnil(L); lua_next(L, idx) != 0; lua_pop(L, 1)) {
			// "key" = value (table format of CommandNotify)
			// ignore the "coded" key; not a boolean value
			if (lua_israwstring(L, -2)) {
				if (!lua_isboolean(L, -1))
					continue;

				const bool value = lua_toboolean(L, -1);

				switch (hashString(lua_tostring(L, -2))) {
					case hashString("right"): {
						cmd.SetOpts(cmd.GetOpts() | (RIGHT_MOUSE_KEY * value));
					} break;
					case hashString("alt"): {
						cmd.SetOpts(cmd.GetOpts() | (ALT_KEY * value));
					} break;
					case hashString("ctrl"): {
						cmd.SetOpts(cmd.GetOpts() | (CONTROL_KEY * value));
					} break;
					case hashString("shift"): {
						cmd.SetOpts(cmd.GetOpts() | (SHIFT_KEY * value));
					} break;
					case hashString("meta"): {
						cmd.SetOpts(cmd.GetOpts() | (META_KEY * value));
					} break;
				}

				continue;
			}

			// [idx] = "value", avoid 'n'
			if (lua_israwnumber(L, -2)) {
				if (!lua_isstring(L, -1))
					continue;

				switch (hashString(lua_tostring(L, -1))) {
					case hashString("right"): {
						cmd.SetOpts(cmd.GetOpts() | RIGHT_MOUSE_KEY);
					} break;
					case hashString("alt"): {
						cmd.SetOpts(cmd.GetOpts() | ALT_KEY);
					} break;
					case hashString("ctrl"): {
						cmd.SetOpts(cmd.GetOpts() | CONTROL_KEY);
					} break;
					case hashString("shift"): {
						cmd.SetOpts(cmd.GetOpts() | SHIFT_KEY);
					} break;
					case hashString("meta"): {
						cmd.SetOpts(cmd.GetOpts() | META_KEY);
					} break;
				}
			}
		}

		return true;
	}

	luaL_error(L, "%s(): bad options-argument type", caller);
	return false;
}

static bool ParseCommandTimeOut(
	lua_State* L,
	Command& cmd,
	const char* caller,
	const int idx
) {
	if (!lua_isnumber(L, idx))
		return false;

	cmd.SetTimeOut(lua_tonumber(L, idx));
	return true;
}

Command LuaUtils::ParseCommand(lua_State* L, const char* caller, int idIndex)
{
	// cmdID
	if (!lua_isnumber(L, idIndex))
		luaL_error(L, "%s(): bad command ID", caller);

	Command cmd(lua_toint(L, idIndex));

	{
		// params
		const int paramTableIdx = idIndex + 1;

		if (lua_isnumber(L, paramTableIdx)) {
			cmd.PushParam(lua_tofloat(L, paramTableIdx));
		} else if (lua_istable(L, paramTableIdx)) {
			for (lua_pushnil(L); lua_next(L, paramTableIdx) != 0; lua_pop(L, 1)) {
				if (!lua_israwnumber(L, -2))
					continue; // avoid 'n'

				if (!lua_isnumber(L, -1))
					luaL_error(L, "%s(): expected <number idx=%d, number value> in params-table", caller, lua_tonumber(L, -2));

				cmd.PushParam(lua_tofloat(L, -1));
			}
		} else {
			luaL_error(L, "%s(): bad param (expected table or number)", caller);
		}
	}

	// options
	ParseCommandOptions(L, cmd, caller, idIndex + 2);
	// timeout
	ParseCommandTimeOut(L, cmd, caller, idIndex + 3);

	// XXX should do some sanity checking?
	return cmd;
}


Command LuaUtils::ParseCommandTable(lua_State* L, const char* caller, int tableIdx)
{
	// cmdID
	lua_rawgeti(L, tableIdx, 1);

	if (!lua_isnumber(L, -1))
		luaL_error(L, "%s(): bad command ID", caller);

	Command cmd(lua_toint(L, -1));
	lua_pop(L, 1);

	{
		// params
		lua_rawgeti(L, tableIdx, 2);

		if (lua_isnumber(L, -1)) {
			cmd.PushParam(lua_tofloat(L, -1));
		} else if (lua_istable(L, -1)) {
			const int paramTableIdx = lua_gettop(L);

			for (lua_pushnil(L); lua_next(L, paramTableIdx) != 0; lua_pop(L, 1)) {
				if (!lua_israwnumber(L, -2))
					continue; // avoid 'n'

				if (!lua_isnumber(L, -1))
					luaL_error(L, "%s(): bad param table entry", caller);

				cmd.PushParam(lua_tofloat(L, -1));
			}
		} else {
			luaL_error(L, "%s(): bad param (expected table or number)", caller);
		}

		lua_pop(L, 1);
	}

	{
		// options
		lua_rawgeti(L, tableIdx, 3);
		ParseCommandOptions(L, cmd, caller, lua_gettop(L));
		lua_pop(L, 1);
	}
	{
		// timeout
		lua_rawgeti(L, tableIdx, 4);
		ParseCommandTimeOut(L, cmd, caller, lua_gettop(L));
		lua_pop(L, 1);
	}

	// XXX should do some sanity checking?
	return cmd;
}


void LuaUtils::ParseCommandArray(
	lua_State* L,
	const char* caller,
	int tableIdx,
	std::vector<Command>& commands
) {
	if (!lua_istable(L, tableIdx))
		luaL_error(L, "%s(): error parsing command array", caller);

	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (!lua_istable(L, -1))
			continue;

		commands.emplace_back(ParseCommandTable(L, caller, lua_gettop(L)));
	}
}


int LuaUtils::ParseFacing(lua_State* L, const char* caller, int index)
{
	if (lua_israwnumber(L, index))
		return std::max(0, std::min(3, lua_toint(L, index)));

	if (lua_israwstring(L, index)) {
		const char* dir = lua_tostring(L, index);

		switch (dir[0]) {
			case 'S': case 's': { return 0; } break;
			case 'E': case 'e': { return 1; } break;
			case 'N': case 'n': { return 2; } break;
			case 'W': case 'w': { return 3; } break;
			default           : {           } break;
		}

		luaL_error(L, "%s(): bad facing string \"%s\"", caller, dir);
	}

	luaL_error(L, "%s(): bad facing parameter", caller);
	return 0;
}


/******************************************************************************/
/******************************************************************************/


int LuaUtils::Next(const ParamMap& paramMap, lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 2); // create a 2nd argument if there isn't one

	// internal parameters first
	if (lua_isnoneornil(L, 2)) {
		const string& nextKey = paramMap.begin()->first;
		lua_pushsstring(L, nextKey); // push the key
		lua_pushvalue(L, 3);         // copy the key
		lua_gettable(L, 1);          // get the value
		return 2;
	}

	// all internal parameters use strings as keys
	if (lua_isstring(L, 2)) {
		const char* key = lua_tostring(L, 2);
		ParamMap::const_iterator it = paramMap.find(key);
		if ((it != paramMap.end()) && (it->second.type != READONLY_TYPE)) {
			// last key was an internal parameter
			++it;
			while ((it != paramMap.end()) && (it->second.type == READONLY_TYPE || it->second.deprecated)) {
				++it; // skip read-only and deprecated/error parameters
			}
			if ((it != paramMap.end()) && (it->second.type != READONLY_TYPE)) {
				// next key is an internal parameter
				const string& nextKey = it->first;
				lua_pushsstring(L, nextKey); // push the key
				lua_pushvalue(L, 3);         // copy the key
				lua_gettable(L, 1);          // get the value (proxied)
				return 2;
			}
			// start the user parameters,
			// remove the internal key and push a nil
			lua_settop(L, 1);
			lua_pushnil(L);
		}
	}

	// user parameter
	if (lua_next(L, 1))
		return 2;

	// end of the line
	lua_pushnil(L);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static void LogMsg(lua_State* L, const char* logSection, int logLevel, int argIndex)
{
	// mostly copied from lua/src/lbaselib.cpp
	std::string msg;

	const int numArgs = lua_gettop(L);

	// rely on Lua's own number formatting
	lua_getglobal(L, "tostring");

	if (numArgs != argIndex || !lua_istable(L, argIndex)) {
		// print individual args
		for (int i = argIndex; i <= numArgs; i++) {
			lua_pushvalue(L, -1);     // function to be called
			lua_pushvalue(L, i);      // value to print
			lua_pcall(L, 1, 1, 0);

			const char* s = lua_tostring(L, -1);  // get result

			if (i > argIndex)
				msg += ", ";
			if (s != nullptr)
				msg += s;

			lua_pop(L, 1);            // pop result
		}
	} else {
		// print table values (array style)
		msg = "TABLE: ";

		for (lua_pushnil(L); lua_next(L, argIndex) != 0; lua_pop(L, 1)) {
			if (!lua_israwnumber(L, -2)) // only numeric keys
				continue;

			lua_pushvalue(L, -3);    // function to be called
			lua_pushvalue(L, -2);    // value to print
			lua_pcall(L, 1, 1, 0);

			const char* s = lua_tostring(L, -1);  // get result

			if ((msg.size() + 1) > sizeof("TABLE: "))
				msg += ", ";
			if (s != nullptr)
				msg += s;

			lua_pop(L, 1);            // pop result
		}
	}

	if (logSection == nullptr) {
		LOG("%s", msg.c_str());
	} else {
		LOG_SI(logSection, logLevel, "%s", msg.c_str());
	}
}


int LuaUtils::Echo(lua_State* L)
{
	LogMsg(L, nullptr, -1, 1);
	return 0;
}


bool LuaUtils::PushLogEntries(lua_State* L)
{
#define PUSH_LOG_LEVEL(cmd) LuaPushNamedNumber(L, #cmd, LOG_LEVEL_ ## cmd)
	PUSH_LOG_LEVEL(DEBUG);
	PUSH_LOG_LEVEL(INFO);
	PUSH_LOG_LEVEL(NOTICE);
	PUSH_LOG_LEVEL(WARNING);
	PUSH_LOG_LEVEL(ERROR);
	PUSH_LOG_LEVEL(FATAL);
	return true;
}

int LuaUtils::ParseLogLevel(lua_State* L, int index)
{
	if (lua_israwnumber(L, index))
		return (lua_tonumber(L, index));

	if (lua_israwstring(L, index)) {
		switch (lua_tostring(L, index)[0]) {
			case 'D': case 'd': { return LOG_LEVEL_DEBUG  ; } break;
			case 'I': case 'i': { return LOG_LEVEL_INFO   ; } break;
			case 'N': case 'n': { return LOG_LEVEL_NOTICE ; } break;
			case 'W': case 'w': { return LOG_LEVEL_WARNING; } break;
			case 'E': case 'e': { return LOG_LEVEL_ERROR  ; } break;
			case 'F': case 'f': { return LOG_LEVEL_FATAL  ; } break;
			default           : {                           } break;
		}
	}

	return -1;
}

/*-
	Logs a msg to the logfile / console
	@param loglevel loglevel that will be used for the message
	@param msg string to be logged
	@fn Spring.Log(string logsection, int loglevel, ...)
	@fn Spring.Log(string logsection, string loglevel, ...)
*/
int LuaUtils::Log(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args < 3)
		return luaL_error(L, "Incorrect arguments to Spring.Log(logsection, loglevel, ...)");

	const char* section = luaL_checkstring(L, 1);

	const int loglevel = LuaUtils::ParseLogLevel(L, 2);
	if (loglevel < 0)
		return luaL_error(L, "Incorrect arguments to Spring.Log(logsection, loglevel, ...)");

	LogMsg(L, section, loglevel, 3);
	return 0;
}

/******************************************************************************/
/******************************************************************************/

LuaUtils::ScopedStackChecker::ScopedStackChecker(lua_State* L, int _returnVars)
	: luaState(L)
	, prevTop(lua_gettop(luaState))
	, returnVars(_returnVars)
{
}

LuaUtils::ScopedStackChecker::~ScopedStackChecker() {
	const int curTop = lua_gettop(luaState); // use var so you can print it in gdb
	assert(curTop == prevTop + returnVars);
}

/******************************************************************************/
/******************************************************************************/

#define DEBUG_TABLE "debug"
#define DEBUG_FUNC "traceback"

/// this function always leaves one item on the stack
/// and returns its index if valid and zero otherwise
int LuaUtils::PushDebugTraceback(lua_State* L)
{
	lua_getglobal(L, DEBUG_TABLE);

	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, DEBUG_FUNC);
		lua_remove(L, -2); // remove DEBUG_TABLE from stack

		if (!lua_isfunction(L, -1)) {
			return 0; // leave a stub on stack
		}
	} else {
		lua_pop(L, 1);
		static const LuaHashString traceback("traceback");
		if (!traceback.GetRegistryFunc(L)) {
			lua_pushnil(L); // leave a stub on stack
			return 0;
		}
	}

	return lua_gettop(L);
}



LuaUtils::ScopedDebugTraceBack::ScopedDebugTraceBack(lua_State* _L)
	: L(_L)
	, errFuncIdx(PushDebugTraceback(_L))
{
	assert(errFuncIdx >= 0);
}

LuaUtils::ScopedDebugTraceBack::~ScopedDebugTraceBack() {
	// make sure we are at same position on the stack
	const int curTop = lua_gettop(L);
	assert(errFuncIdx == 0 || curTop == errFuncIdx);

	lua_pop(L, 1);
}

/******************************************************************************/
/******************************************************************************/

void LuaUtils::PushStringVector(lua_State* L, const vector<string>& vec)
{
	lua_createtable(L, vec.size(), 0);
	for (size_t i = 0; i < vec.size(); i++) {
		lua_pushsstring(L, vec[i]);
		lua_rawseti(L, -2, (int)(i + 1));
	}
}

/******************************************************************************/
/******************************************************************************/

void LuaUtils::PushCommandDesc(lua_State* L, const SCommandDescription& cd)
{
	const int numParams = cd.params.size();
	const int numTblKeys = 12;

	lua_checkstack(L, 1 + 1 + 1 + 1);
	lua_createtable(L, 0, numTblKeys);

	HSTR_PUSH_NUMBER(L, "id",          cd.id);
	HSTR_PUSH_NUMBER(L, "type",        cd.type);
	HSTR_PUSH_STRING(L, "name",        cd.name);
	HSTR_PUSH_STRING(L, "action",      cd.action);
	HSTR_PUSH_STRING(L, "tooltip",     cd.tooltip);
	HSTR_PUSH_STRING(L, "texture",     cd.iconname);
	HSTR_PUSH_STRING(L, "cursor",      cd.mouseicon);
	HSTR_PUSH_BOOL(L,   "queueing",    cd.queueing);
	HSTR_PUSH_BOOL(L,   "hidden",      cd.hidden);
	HSTR_PUSH_BOOL(L,   "disabled",    cd.disabled);
	HSTR_PUSH_BOOL(L,   "showUnique",  cd.showUnique);
	HSTR_PUSH_BOOL(L,   "onlyTexture", cd.onlyTexture);

	HSTR_PUSH(L, "params");

	lua_createtable(L, 0, numParams);

	for (int p = 0; p < numParams; p++) {
		lua_pushsstring(L, cd.params[p]);
		lua_rawseti(L, -2, p + 1);
	}

	// CmdDesc["params"] = {[1] = "string1", [2] = "string2", ...}
	lua_settable(L, -3);
}
