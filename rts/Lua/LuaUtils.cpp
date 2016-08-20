/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//#include "System/Platform/Win/win32.h"

#include <boost/cstdint.hpp>

#include <string.h>
#include <map>

#include "LuaUtils.h"
#include "LuaConfig.h"

#include "Rendering/Models/IModelParser.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
	#include "System/TimeProfiler.h"
#else
	#define SCOPED_TIMER(x)
#endif


static const int maxDepth = 16;
int LuaUtils::exportedDataSize = 0;


/******************************************************************************/
/******************************************************************************/


static bool CopyPushData(lua_State* dst, lua_State* src, int index, int depth, std::map<const void*, int>& alreadyCopied);
static bool CopyPushTable(lua_State* dst, lua_State* src, int index, int depth, std::map<const void*, int>& alreadyCopied);


static inline int PosAbsLuaIndex(lua_State* src, int index)
{
	if (index > 0)
		return index;

	return (lua_gettop(src) + index + 1);
}


static bool CopyPushData(lua_State* dst, lua_State* src, int index, int depth, std::map<const void*, int>& alreadyCopied)
{
	const int type = lua_type(src, index);
	switch (type) {
		case LUA_TBOOLEAN: {
			lua_pushboolean(dst, lua_toboolean(src, index));
			break;
		}
		case LUA_TNUMBER: {
			lua_pushnumber(dst, lua_tonumber(src, index));
			break;
		}
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
			break;
		}
		case LUA_TTABLE: {
			CopyPushTable(dst, src, index, depth, alreadyCopied);
			break;
		}
		default: {
			lua_pushnil(dst); // unhandled type
			return false;
		}
	}
	return true;
}


static bool CopyPushTable(lua_State* dst, lua_State* src, int index, int depth, std::map<const void*, int>& alreadyCopied)
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
	SCOPED_TIMER("::CopyData");

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
	std::map<const void*, int> alreadyCopied;

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

static bool BackupData(LuaUtils::DataDump &d, lua_State* src, int index, int depth);
static bool RestoreData(const LuaUtils::DataDump &d, lua_State* dst, int depth);
static bool BackupTable(LuaUtils::DataDump &d, lua_State* src, int index, int depth);
static bool RestoreTable(const LuaUtils::DataDump &d, lua_State* dst, int depth);


static bool BackupData(LuaUtils::DataDump &d, lua_State* src, int index, int depth) {
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
			if(!BackupTable(d, src, index, depth))
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

static bool RestoreData(const LuaUtils::DataDump &d, lua_State* dst, int depth) {
	--LuaUtils::exportedDataSize;
	const int type = d.type;
	switch (type) {
		case LUA_TBOOLEAN: {
			lua_pushboolean(dst, d.bol);
			break;
		}
		case LUA_TNUMBER: {
			lua_pushnumber(dst, d.num);
			break;
		}
		case LUA_TSTRING: {
			lua_pushlstring(dst, d.str.c_str(), d.str.size());
			break;
		}
		case LUA_TTABLE: {
			RestoreTable(d, dst, depth);
			break;
		}
		default: {
			lua_pushnil(dst);
			break;
		}
	}
	return true;
}

static bool BackupTable(LuaUtils::DataDump &d, lua_State* src, int index, int depth) {
	if (depth++ > maxDepth)
		return false;

	const int table = PosAbsLuaIndex(src, index);
	for (lua_pushnil(src); lua_next(src, table) != 0; lua_pop(src, 1)) {
		LuaUtils::DataDump dk, dv;
		BackupData(dk, src, -2, depth);
		BackupData(dv, src, -1, depth);
		d.table.push_back(std::pair<LuaUtils::DataDump, LuaUtils::DataDump>(dk ,dv));
	}

	return true;
}

static bool RestoreTable(const LuaUtils::DataDump &d, lua_State* dst, int depth) {
	if (depth++ > maxDepth) {
		lua_pushnil(dst);
		return false;
	}

	lua_newtable(dst);
	for (std::vector<std::pair<LuaUtils::DataDump, LuaUtils::DataDump> >::const_iterator i = d.table.begin(); i != d.table.end(); ++i) {
		RestoreData((*i).first, dst, depth);
		RestoreData((*i).second, dst, depth);
		lua_rawset(dst, -3);
	}

	return true;
}


int LuaUtils::Backup(std::vector<LuaUtils::DataDump> &backup, lua_State* src, int count) {
	const int srcTop = lua_gettop(src);
	if (srcTop < count)
		return 0;

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;
	for (int i = startIndex; i <= endIndex; i++) {
		backup.push_back(DataDump());
		BackupData(backup.back(), src, i, 0);
	}

	return count;
}


int LuaUtils::Restore(const std::vector<LuaUtils::DataDump> &backup, lua_State* dst) {
	const int dstTop = lua_gettop(dst);
	int count = backup.size();
	lua_checkstack(dst, count + 3);

	for (std::vector<DataDump>::const_iterator i = backup.begin(); i != backup.end(); ++i) {
		RestoreData(*i, dst, 0);
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
	if (lua_getstack(L, 1, &ar) == 0) {
		luaL_error(L, "%s() lua_getstack() error", caller);
	}
	if (lua_getinfo(L, "f", &ar) == 0) {
		luaL_error(L, "%s() lua_getinfo() error", caller);
	}
	if (!lua_isfunction(L, -1)) {
		luaL_error(L, "%s() invalid current function", caller);
	}
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


static bool LowerKeysCheck(lua_State* L, int table, int alreadyCheckTable)
{
	bool checked = true;
	lua_pushvalue(L, table);
	lua_rawget(L, alreadyCheckTable);
	if (lua_isnil(L, -1)) {
		checked = false;
		lua_pushvalue(L, table);
		lua_pushboolean(L, true);
		lua_rawset(L, alreadyCheckTable);
	}
	lua_pop(L, 1);
	return checked;
}


static bool LowerKeysReal(lua_State* L, int alreadyCheckTable)
{
	luaL_checkstack(L, 8, __FUNCTION__);
	const int table = lua_gettop(L);
	if (LowerKeysCheck(L, table, alreadyCheckTable)) {
		return true;
	}

	// a new table for changed values
	const int changed = table + 1;
	lua_newtable(L);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_istable(L, -1)) {
			LowerKeysReal(L, alreadyCheckTable);
		}
		if (lua_israwstring(L, -2)) {
			const string rawKey = lua_tostring(L, -2);
			const string lowerKey = StringToLower(rawKey);
			if (rawKey != lowerKey) {
				// removed the mixed case entry
				lua_pushvalue(L, -2); // the key
				lua_pushnil(L);
				lua_rawset(L, table);
				// does the lower case key alread exist in the table?
				lua_pushsstring(L, lowerKey);
				lua_rawget(L, table);
				if (lua_isnil(L, -1)) {
					// lower case does not exist, add it to the changed table
					lua_pushsstring(L, lowerKey);
					lua_pushvalue(L, -3); // the value
					lua_rawset(L, changed);
				}
				lua_pop(L, 1);
			}
		}
	}

	// copy the changed values into the table
	for (lua_pushnil(L); lua_next(L, changed) != 0; lua_pop(L, 1)) {
		lua_pushvalue(L, -2); // copy the key to the top
		lua_pushvalue(L, -2); // copy the value to the top
		lua_rawset(L, table);
	}

	lua_pop(L, 1); // pop the changed table
	return true;
}


bool LuaUtils::LowerKeys(lua_State* L, int table)
{
	if (!lua_istable(L, table)) {
		return false;
	}

	// table of processed tables
	luaL_checkstack(L, 2, __FUNCTION__);
	lua_newtable(L);
	const int checkedTableIdx = lua_gettop(L);

	lua_pushvalue(L, table); // push the table onto the top of the stack
	LowerKeysReal(L, checkedTableIdx);

	lua_pop(L, 2); // the lowered table, and the check table
	return true;
}


static bool CheckForNaNsReal(lua_State* L, const std::string& path)
{
	luaL_checkstack(L, 3, __FUNCTION__);
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
		} else
		if (lua_isnumber(L, -1)) {
			// Check for NaN
			const float value = lua_tonumber(L, -1);
			if (math::isinf(value) || math::isnan(value)) {
				// We can't work on -2 directly cause lua_tostring would replace the value in -2,
				// so we need to make a copy and convert that to a string.
				lua_pushvalue(L, -2);
				const char* key = lua_tostring(L, -1);
				LOG_L(L_WARNING, "%s%s: Got Invalid NaN/Inf!", path.c_str(), key);
				lua_pop(L, 1);
				foundNaNs = true;
			}
		}
	}

	return foundNaNs;
}


bool LuaUtils::CheckTableForNaNs(lua_State* L, int table, const std::string& name)
{
	if (!lua_istable(L, table)) {
		return false;
	}

	luaL_checkstack(L, 2, __FUNCTION__);

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
	if (p != NULL) {                               // value is a userdata?
		if (lua_getmetatable(L, index)) {            // does it have a metatable?
			lua_getfield(L, LUA_REGISTRYINDEX, tname); // get correct metatable
			if (lua_rawequal(L, -1, -2)) {             // the correct mt?
				lua_pop(L, 2);                           // remove both metatables
				return p;
			}
		}
	}
	return NULL;
}


/******************************************************************************/
/******************************************************************************/

void LuaUtils::PrintStack(lua_State* L)
{
	const int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) {
		LOG_L(L_ERROR, "  %i: type = %s (%p)", i, luaL_typename(L, i), lua_topointer(L, i));
		const int type = lua_type(L, i);
		switch(type) {
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


/******************************************************************************/
/******************************************************************************/

int LuaUtils::ParseIntArray(lua_State* L, int index, int* array, int size)
{
	if (!lua_istable(L, index))
		return -1;

	const int absIdx = PosAbsLuaIndex(L, index);

	for (int i = 0; i < size; i++) {
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

	const int absIdx = PosAbsLuaIndex(L, index);

	for (int i = 0; i < size; i++) {
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

	const int absIdx = PosAbsLuaIndex(L, index);

	for (int i = 0; i < size; i++) {
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
	const int absIdx = PosAbsLuaIndex(L, index);

	for (int i = 0; ; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isnumber(L, -1)) {
			vec.push_back(lua_toint(L, -1));
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
}

int LuaUtils::ParseFloatVector(lua_State* L, int index, vector<float>& vec)
{
	if (!lua_istable(L, index))
		return -1;

	vec.clear();
	const int absIdx = PosAbsLuaIndex(L, index);

	for (int i = 0; ; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isnumber(L, -1)) {
			vec.push_back(lua_tofloat(L, -1));
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
}

int LuaUtils::ParseStringVector(lua_State* L, int index, vector<string>& vec)
{
	if (!lua_istable(L, index))
		return -1;

	vec.clear();
	const int absIdx = PosAbsLuaIndex(L, index);

	for (int i = 0; ; i++) {
		lua_rawgeti(L, absIdx, (i + 1));

		if (lua_isstring(L, -1)) {
			vec.push_back(lua_tostring(L, -1));
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
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


#endif //!defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI


void LuaUtils::PushCommandParamsTable(lua_State* L, const Command& cmd, bool subtable)
{
	if (subtable) {
		HSTR_PUSH(L, "params");
	}

	lua_createtable(L, cmd.params.size(), 0);

	for (unsigned int p = 0; p < cmd.params.size(); p++) {
		lua_pushnumber(L, cmd.params[p]);
		lua_rawseti(L, -2, p + 1);
	}

	if (subtable) {
		lua_rawset(L, -3);
	}
}

void LuaUtils::PushCommandOptionsTable(lua_State* L, const Command& cmd, bool subtable)
{
	if (subtable) {
		HSTR_PUSH(L, "options");
	}

	lua_createtable(L, 0, 7);
	HSTR_PUSH_NUMBER(L, "coded", cmd.options);
	HSTR_PUSH_BOOL(L, "alt",      !!(cmd.options & ALT_KEY        ));
	HSTR_PUSH_BOOL(L, "ctrl",     !!(cmd.options & CONTROL_KEY    ));
	HSTR_PUSH_BOOL(L, "shift",    !!(cmd.options & SHIFT_KEY      ));
	HSTR_PUSH_BOOL(L, "right",    !!(cmd.options & RIGHT_MOUSE_KEY));
	HSTR_PUSH_BOOL(L, "meta",     !!(cmd.options & META_KEY       ));
	HSTR_PUSH_BOOL(L, "internal", !!(cmd.options & INTERNAL_ORDER ));

	if (subtable) {
		lua_rawset(L, -3);
	}
}

void LuaUtils::PushUnitAndCommand(lua_State* L, const CUnit* unit, const Command& cmd)
{
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	lua_pushnumber(L, cmd.GetID());

	PushCommandParamsTable(L, cmd, false);
	PushCommandOptionsTable(L, cmd, false);

	lua_pushnumber(L, cmd.tag);
}

void LuaUtils::ParseCommandOptions(
	lua_State* L,
	Command& cmd,
	const char* caller,
	const int idx
) {
	if (lua_isnumber(L, idx)) {
		cmd.options = (unsigned char)lua_tonumber(L, idx);
	} else if (lua_istable(L, idx)) {
		for (lua_pushnil(L); lua_next(L, idx) != 0; lua_pop(L, 1)) {
			// "key" = value (table format of CommandNotify)
			if (lua_israwstring(L, -2)) {
				const std::string key = lua_tostring(L, -2);

				// we do not care about the "coded" key (not a boolean value)
				if (!lua_isboolean(L, -1))
					continue;

				const bool value = lua_toboolean(L, -1);

				if (key == "right") {
					cmd.options |= (RIGHT_MOUSE_KEY * value);
				} else if (key == "alt") {
					cmd.options |= (ALT_KEY * value);
				} else if (key == "ctrl") {
					cmd.options |= (CONTROL_KEY * value);
				} else if (key == "shift") {
					cmd.options |= (SHIFT_KEY * value);
				} else if (key == "meta") {
					cmd.options |= (META_KEY * value);
				}

				continue;
			}

			// [idx] = "value", avoid 'n'
			if (lua_israwnumber(L, -2)) {
				//const int idx = lua_tonumber(L, -2);

				if (!lua_isstring(L, -1))
					continue;

				const std::string value = lua_tostring(L, -1);

				if (value == "right") {
					cmd.options |= RIGHT_MOUSE_KEY;
				} else if (value == "alt") {
					cmd.options |= ALT_KEY;
				} else if (value == "ctrl") {
					cmd.options |= CONTROL_KEY;
				} else if (value == "shift") {
					cmd.options |= SHIFT_KEY;
				} else if (value == "meta") {
					cmd.options |= META_KEY;
				}
			}
		}
	} else {
		luaL_error(L, "%s(): bad options-argument type", caller);
	}
}


Command LuaUtils::ParseCommand(lua_State* L, const char* caller, int idIndex)
{
	// cmdID
	if (!lua_isnumber(L, idIndex)) {
		luaL_error(L, "%s(): bad command ID", caller);
	}

	Command cmd(lua_toint(L, idIndex));

	// params
	const int paramTableIdx = (idIndex + 1);

	if (!lua_istable(L, paramTableIdx)) {
		luaL_error(L, "%s(): bad param table", caller);
	}

	for (lua_pushnil(L); lua_next(L, paramTableIdx) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) { // avoid 'n'
			if (!lua_isnumber(L, -1)) {
				luaL_error(L, "%s(): expected <number idx=%d, number value> in params-table", caller, lua_tonumber(L, -2));
			}

			cmd.PushParam(lua_tofloat(L, -1));
		}
	}

	// options
	ParseCommandOptions(L, cmd, caller, (idIndex + 2));

	// XXX should do some sanity checking?
	return cmd;
}


Command LuaUtils::ParseCommandTable(lua_State* L, const char* caller, int table)
{
	// cmdID
	lua_rawgeti(L, table, 1);
	if (!lua_isnumber(L, -1)) {
		luaL_error(L, "%s(): bad command ID", caller);
	}
	const int id = lua_toint(L, -1);
	Command cmd(id);
	lua_pop(L, 1);

	// params
	lua_rawgeti(L, table, 2);
	if (!lua_istable(L, -1)) {
		luaL_error(L, "%s(): bad param table", caller);
	}
	const int paramTable = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, paramTable) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) { // avoid 'n'
			if (!lua_isnumber(L, -1)) {
				luaL_error(L, "%s(): bad param table entry", caller);
			}
			const float value = lua_tofloat(L, -1);
			cmd.PushParam(value);
		}
	}
	lua_pop(L, 1);

	// options
	lua_rawgeti(L, table, 3);
	ParseCommandOptions(L, cmd, caller, lua_gettop(L));
	lua_pop(L, 1);

	// XXX should do some sanity checking?

	return cmd;
}


void LuaUtils::ParseCommandArray(lua_State* L, const char* caller,
                                 int table, vector<Command>& commands)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing command array", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_istable(L, -1)) {
			continue;
		}
		Command cmd = ParseCommandTable(L, caller, lua_gettop(L));
		commands.push_back(cmd);
	}
}


int LuaUtils::ParseFacing(lua_State* L, const char* caller, int index)
{
	if (lua_israwnumber(L, index)) {
		return std::max(0, std::min(3, lua_toint(L, index)));
	}
	else if (lua_israwstring(L, index)) {
		const string dir = StringToLower(lua_tostring(L, index));
		if (dir == "s") { return 0; }
		if (dir == "e") { return 1; }
		if (dir == "n") { return 2; }
		if (dir == "w") { return 3; }
		if (dir == "south") { return 0; }
		if (dir == "east")  { return 1; }
		if (dir == "north") { return 2; }
		if (dir == "west")  { return 3; }
		luaL_error(L, "%s(): bad facing string", caller);
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
	if (lua_next(L, 1)) {
		return 2;
	}

	// end of the line
	lua_pushnil(L);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static std::string getprintf_msg(lua_State* L, int index)
{
	// copied from lua/src/lib/lbaselib.c
	string msg = "";
	const int args = lua_gettop(L); // number of arguments

	lua_getglobal(L, "tostring");

	for (int i = index; i <= args; i++) {
		const char* s;
		lua_pushvalue(L, -1);     // function to be called
		lua_pushvalue(L, i);      // value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  // get result
		if (i > index) {
			msg += ", ";
		}
		msg += s;
		lua_pop(L, 1);            // pop result
	}

	if ((args != index) || !lua_istable(L, index)) {
		return msg;
	}

	// print solo tables (array style)
	msg = "TABLE: ";
	bool first = true;
	for (lua_pushnil(L); lua_next(L, index) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {  // only numeric keys
			const char *s;
			lua_pushvalue(L, -3);    // function to be called
			lua_pushvalue(L, -2);    // value to print
			lua_call(L, 1, 1);
			s = lua_tostring(L, -1);  // get result
			if (!first) {
				msg += ", ";
			}
			msg += s;
			first = false;
			lua_pop(L, 1);            // pop result
		}
	}

	return msg;
}


int LuaUtils::Echo(lua_State* L)
{
	const std::string msg = getprintf_msg(L, 1);
	LOG("%s", msg.c_str());
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

	int loglevel = 0;
	if (lua_israwnumber(L, 2)) {
		loglevel = lua_tonumber(L, 2);
	}
	else if (lua_israwstring(L, 2)) {
		std::string loglvlstr = lua_tostring(L, 2);
		StringToLowerInPlace(loglvlstr);
		if (loglvlstr == "debug") {
			loglevel = LOG_LEVEL_DEBUG;
		}
		else if (loglvlstr == "info") {
			loglevel = LOG_LEVEL_INFO;
		}
		else if (loglvlstr == "notice") {
			loglevel = LOG_LEVEL_INFO;
		}
		else if (loglvlstr == "warning") {
			loglevel = LOG_LEVEL_WARNING;
		}
		else if (loglvlstr == "error") {
			loglevel = LOG_LEVEL_ERROR;
		}
		else if (loglvlstr == "fatal") {
			loglevel = LOG_LEVEL_FATAL;
		}
		else {
			return luaL_error(L, "Incorrect arguments to Spring.Log(logsection, loglevel, ...)");
		}
	}
	else {
		return luaL_error(L, "Incorrect arguments to Spring.Log(logsection, loglevel, ...)");
	}

	const std::string msg = getprintf_msg(L, 3);
	LOG_SI(section, loglevel, "%s", msg.c_str());
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

