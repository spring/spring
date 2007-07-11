#include "StdAfx.h"
// LuaVFS.cpp: implementation of the LuaVFS class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaVFS.h"
#include <set>
#include <list>
#include <cctype>
#include <boost/regex.hpp>
using namespace std;

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Platform/FileSystem.h"


/******************************************************************************/
/******************************************************************************/

bool LuaVFS::PushSynced(lua_State* L)
{
	HSTR_PUSH_CFUNC(L, "Include",    SyncInclude);
	HSTR_PUSH_CFUNC(L, "LoadFile",   SyncLoadFile);
	HSTR_PUSH_CFUNC(L, "FileExists", SyncFileExists);
	HSTR_PUSH_CFUNC(L, "DirList",    SyncDirList);

	HSTR_PUSH_NUMBER(L, "RAW_ONLY",  RAW_ONLY);
	HSTR_PUSH_NUMBER(L, "ZIP_ONLY",  ZIP_ONLY);
	HSTR_PUSH_NUMBER(L, "RAW_FIRST", RAW_FIRST);
	HSTR_PUSH_NUMBER(L, "ZIP_FIRST", ZIP_FIRST);

	HSTR_PUSH_CFUNC(L, "PackU8",    PackU8);
	HSTR_PUSH_CFUNC(L, "PackU16",   PackU16);
	HSTR_PUSH_CFUNC(L, "PackU32",   PackU32);
	HSTR_PUSH_CFUNC(L, "PackS8",    PackS8);
	HSTR_PUSH_CFUNC(L, "PackS16",   PackS16);
	HSTR_PUSH_CFUNC(L, "PackS32",   PackS32);
	HSTR_PUSH_CFUNC(L, "PackF32",   PackF32);
	HSTR_PUSH_CFUNC(L, "UnpackU8",  UnpackU8);
	HSTR_PUSH_CFUNC(L, "UnpackU16", UnpackU16);
	HSTR_PUSH_CFUNC(L, "UnpackU32", UnpackU32);
	HSTR_PUSH_CFUNC(L, "UnpackS8",  UnpackS8);
	HSTR_PUSH_CFUNC(L, "UnpackS16", UnpackS16);
	HSTR_PUSH_CFUNC(L, "UnpackS32", UnpackS32);
	HSTR_PUSH_CFUNC(L, "UnpackF32", UnpackF32);

	return true;
}


bool LuaVFS::PushUnsynced(lua_State* L)
{
	HSTR_PUSH_CFUNC(L, "Include",    UnsyncInclude);
	HSTR_PUSH_CFUNC(L, "LoadFile",   UnsyncLoadFile);
	HSTR_PUSH_CFUNC(L, "FileExists", UnsyncFileExists);
	HSTR_PUSH_CFUNC(L, "DirList",    UnsyncDirList);

	HSTR_PUSH_NUMBER(L, "RAW_ONLY",  RAW_ONLY);
	HSTR_PUSH_NUMBER(L, "ZIP_ONLY",  ZIP_ONLY);
	HSTR_PUSH_NUMBER(L, "RAW_FIRST", RAW_FIRST);
	HSTR_PUSH_NUMBER(L, "ZIP_FIRST", ZIP_FIRST);

	HSTR_PUSH_CFUNC(L, "PackU8",    PackU8);
	HSTR_PUSH_CFUNC(L, "PackU16",   PackU16);
	HSTR_PUSH_CFUNC(L, "PackU32",   PackU32);
	HSTR_PUSH_CFUNC(L, "PackS8",    PackS8);
	HSTR_PUSH_CFUNC(L, "PackS16",   PackS16);
	HSTR_PUSH_CFUNC(L, "PackS32",   PackS32);
	HSTR_PUSH_CFUNC(L, "PackF32",   PackF32);
	HSTR_PUSH_CFUNC(L, "UnpackU8",  UnpackU8);
	HSTR_PUSH_CFUNC(L, "UnpackU16", UnpackU16);
	HSTR_PUSH_CFUNC(L, "UnpackU32", UnpackU32);
	HSTR_PUSH_CFUNC(L, "UnpackS8",  UnpackS8);
	HSTR_PUSH_CFUNC(L, "UnpackS16", UnpackS16);
	HSTR_PUSH_CFUNC(L, "UnpackS32", UnpackS32);
	HSTR_PUSH_CFUNC(L, "UnpackF32", UnpackF32);

	return true;
}


/******************************************************************************/
/******************************************************************************/

LuaVFS::AccessMode LuaVFS::GetMode(lua_State* L, int index, bool synced)
{
	const int args = lua_gettop(L);
	if (index < 0) {
		index = (args - index + 1);
	}
	if ((index < 1) || (index > args)) {
		if (synced && !CLuaHandle::GetDevMode()) {
			return ZIP_ONLY;
		}
		return RAW_FIRST;
	}

	if (!lua_isnumber(L, index)) {
		luaL_error(L, "Bad VFS access mode");
	}
	const AccessMode mode = (AccessMode)(int)lua_tonumber(L, index);
	if ((mode < 0) || (mode > LAST_MODE)) {
		luaL_error(L, "Bad VFS access mode");
	}

	if (synced && !CLuaHandle::GetDevMode()) {
		return ZIP_ONLY;
	}
	return mode;
}


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


static void PushCurrentFuncEnv(lua_State* L, const char* caller)
{
	PushCurrentFunc(L, caller);
	PushFunctionEnv(L, caller, -1);
	lua_remove(L, -2); // remove the function
}


/******************************************************************************/

static bool TestLoadFile(const string& filename, string& data,
                         CFileHandler::VFSmode vfsMode)
{
	CFileHandler fh(filename, vfsMode);
	if (!fh.FileExists()) {
		return false;
	}
	data.clear();
	if (!fh.LoadStringData(data)) {
		return false;
	}
	return true;
}


static bool LoadFileWithMode(const string& filename, string& data,
                             LuaVFS::AccessMode mode)
{
	switch (mode) {
		case LuaVFS::RAW_ONLY: {
			if (TestLoadFile(filename, data, CFileHandler::OnlyRawFS)) {
				return true;
			}
			return false;
		}
		case LuaVFS::ZIP_ONLY: {
			if (TestLoadFile(filename, data, CFileHandler::OnlyArchiveFS)) {
				return true;
			}
			return false;
		}
		case LuaVFS::RAW_FIRST: {
			if (TestLoadFile(filename, data, CFileHandler::OnlyRawFS)) {
				return true;
			}
			if (TestLoadFile(filename, data, CFileHandler::OnlyArchiveFS)) {
				return true;
			}
			return false;
		}
		case LuaVFS::ZIP_FIRST: {
			if (TestLoadFile(filename, data, CFileHandler::OnlyArchiveFS)) {
				return true;
			}
			if (TestLoadFile(filename, data, CFileHandler::OnlyRawFS)) {
				return true;
			}
			return false;
		}
	}
	return false; // bad mode
}


/******************************************************************************/
/******************************************************************************/

int LuaVFS::Include(lua_State* L, bool synced)
{
	const int args = lua_gettop(L);
	if ((args < 1) || !lua_isstring(L, 1) ||
	    ((args >= 2) && !lua_istable(L, 2) && !lua_isnil(L, 2))) {
		luaL_error(L, "Incorrect arguments to Include()");
	}

	const AccessMode mode = GetMode(L, 3, synced);
	
	const string filename = lua_tostring(L, 1);

	string code;
	if (!LoadFileWithMode(filename, code, mode)) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf),
		         "Include() could not load '%s'\n", filename.c_str());
		lua_pushstring(L, buf);
 		lua_error(L);
	}

	int error = luaL_loadbuffer(L, code.c_str(), code.size(), filename.c_str());
	if (error != 0) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "error = %i, %s, %s\n",
		         error, filename.c_str(), lua_tostring(L, -1));
		lua_pushstring(L, buf);
		lua_error(L);
	}

	// set the chunk's fenv to the current fenv, or a user table
	if ((args >= 2) && lua_istable(L, 2)) {
		lua_pushvalue(L, 2); // user fenv
	} else {
		PushCurrentFuncEnv(L, __FUNCTION__);
	}

	// set the include fenv to the current function's fenv
	if (lua_setfenv(L, -2) == 0) {
		luaL_error(L, "Include(): error with setfenv");
	}

	const int paramTop = lua_gettop(L) - 1;	

	error = lua_pcall(L, 0, LUA_MULTRET, 0);


	if (error != 0) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "error = %i, %s, %s\n",
		         error, filename.c_str(), lua_tostring(L, -1));
		lua_pushstring(L, buf);
		lua_error(L);
	}

	return lua_gettop(L) - paramTop;
}


int LuaVFS::SyncInclude(lua_State* L)
{
	return Include(L, true);
}


int LuaVFS::UnsyncInclude(lua_State* L)
{
	return Include(L, false);
}


/******************************************************************************/

int LuaVFS::LoadFile(lua_State* L, bool synced)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to LoadFile()");
	}

	const string filename = lua_tostring(L, 1);

	const AccessMode mode = GetMode(L, 2, synced);

	string data;
	if (LoadFileWithMode(filename, data, mode)) {
		lua_pushlstring(L, data.c_str(), data.size());
		return 1;
	}
	return 0;
}


int LuaVFS::SyncLoadFile(lua_State* L)
{
	return LoadFile(L, true);
}


int LuaVFS::UnsyncLoadFile(lua_State* L)
{
	return LoadFile(L, false);
}


/******************************************************************************/

int LuaVFS::FileExists(lua_State* L, bool synced)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to FileExists()");
	}

	const string filename = lua_tostring(L, 1);

	const AccessMode mode = GetMode(L, 2, synced);

	switch (mode) {
		case LuaVFS::RAW_ONLY: {
			CFileHandler fh(filename, CFileHandler::OnlyRawFS);
			lua_pushboolean(L, fh.FileExists());
			return 1;
		}
		case LuaVFS::ZIP_ONLY: {
			CFileHandler fh(filename, CFileHandler::OnlyArchiveFS);
			lua_pushboolean(L, fh.FileExists());
			return 1;
		}
		case LuaVFS::RAW_FIRST: {
			CFileHandler fhr(filename, CFileHandler::OnlyRawFS);
			if (fhr.FileExists()) {
				lua_pushboolean(L, true);
				return 1;
			}
			CFileHandler fhz(filename, CFileHandler::OnlyArchiveFS);
			lua_pushboolean(L, fhz.FileExists());
			return 1;
		}
		case LuaVFS::ZIP_FIRST: {
			CFileHandler fhz(filename, CFileHandler::OnlyArchiveFS);
			if (fhz.FileExists()) {
				lua_pushboolean(L, true);
				return 1;
			}
			CFileHandler fhr(filename, CFileHandler::OnlyRawFS);
			lua_pushboolean(L, fhr.FileExists());
			return 1;
		}
	}
	return 0;
}


int LuaVFS::SyncFileExists(lua_State* L)
{
	return FileExists(L, true);
}


int LuaVFS::UnsyncFileExists(lua_State* L)
{
	return FileExists(L, false);
}


/******************************************************************************/

static void AppendRawDirList(const string& dir, const string& pattern,
                             int options, vector<string>& filenames)
{
	// keep searches within the Spring directory
	if ((dir[0] == '/') || (dir[0] == '\\') ||
	    (strstr(dir.c_str(), "..") != NULL) ||
	    ((dir.size() >= 2) && (dir[1] == ':'))) {
		return; // invalid access
	}

	vector<string> files = filesystem.FindFiles(dir, pattern, options);
	for (int f = 0; f < (int)files.size(); f++) {
		filenames.push_back(files[f]);
	}
}


static void AppendZipDirList(const string& dir, const string& pattern,
                             vector<string>& filenames)
{
	string prefix = dir;
	if (dir.find_last_of("\\/") != (dir.size() - 1)) {
		prefix += '/';
	}
	boost::regex regex(filesystem.glob_to_regex(pattern), boost::regex::icase);
	vector<string> files = hpiHandler->GetFilesInDir(dir);
	vector<string>::iterator fi;
	for (fi = files.begin(); fi != files.end(); ++fi) {
		if (boost::regex_match(*fi, regex)) {
			filenames.push_back(prefix + *fi);
		}
	}  
}


static void FillDirList(const string& dir, const string& pattern,
                        int options, vector<string>& filenames,
                        LuaVFS::AccessMode mode)
{
	filenames.clear();
	switch (mode) {
		case LuaVFS::RAW_ONLY: {
			AppendRawDirList(dir, pattern, options, filenames);
			return;
		}
		case LuaVFS::ZIP_ONLY: {
			AppendZipDirList(dir, pattern, filenames);
			return;
		}
		case LuaVFS::RAW_FIRST: {
			AppendRawDirList(dir, pattern, options, filenames);
			AppendZipDirList(dir, pattern,          filenames);
			return;
		}
		case LuaVFS::ZIP_FIRST: {
			AppendZipDirList(dir, pattern,          filenames);
			AppendRawDirList(dir, pattern, options, filenames);
			return;
		}
	}
}


int LuaVFS::DirList(lua_State* L, bool synced)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1) ||
	    ((args >= 2) && !lua_isstring(L, 2)) ||
	    ((args >= 4) && !lua_isstring(L, 4))) {
		luaL_error(L, "Incorrect arguments to DirList()");
	}

	const string dir = lua_tostring(L, 1);

	const AccessMode mode = GetMode(L, 3, synced);

	string pattern = "*";
	if (args >= 2) {
		pattern = lua_tostring(L, 2);
		if (pattern.empty()) {
			pattern = "*"; // FindFiles() croaks on empty strings
		}
	}

	int options = 0;
	if (args >= 4) {
		const string optstr = lua_tostring(L, 4);
		if (strstr(optstr.c_str(), "r") != NULL) {
			options |= FileSystem::RECURSE;
		}
		if (strstr(optstr.c_str(), "d") != NULL) {
			options |= FileSystem::INCLUDE_DIRS;
		}
	}

	vector<string> filenames;
	FillDirList(dir, pattern, options, filenames, mode);

	lua_newtable(L);
	for (int i = 0; i < filenames.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, filenames[i].c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, filenames.size());
	lua_rawset(L, -3);

	return 1;
}


int LuaVFS::SyncDirList(lua_State* L)
{
	return DirList(L, true);
}


int LuaVFS::UnsyncDirList(lua_State* L)
{
	return DirList(L, false);
}


/******************************************************************************/
/******************************************************************************/
//
//  NOTE: Endianess should be handled
//

template <typename T>
int PackType(lua_State* L)
{
	vector<T> vals;

	if (lua_istable(L, 1)) {
		const int table = 1;
		for (int i = 1;
		     lua_rawgeti(L, 1, i), lua_isnumber(L, -1);
		     lua_pop(L, 1), i++) {
			vals.push_back((T)lua_tonumber(L, i));
		}
		return 1;
	}
	else {
		const int args = lua_gettop(L);
		for (int i = 1; i <= args; i++) {
			if (!lua_isnumber(L, i)) {
				break;
			}
			vals.push_back((T)lua_tonumber(L, i));
		}
	}

	if (vals.empty()) {
		return 0;
	}

	const int bufSize = sizeof(T) * vals.size();
	char* buf = SAFE_NEW char[bufSize];
	for (int i = 0; i < (int)vals.size(); i++) {
		memcpy(buf + (i * sizeof(T)), &vals[i], sizeof(T));
	}
	lua_pushlstring(L, buf, bufSize);
	delete[] buf;
	
	return 1;
}


int LuaVFS::PackU8(lua_State* L)  { return PackType<boost::uint8_t>(L); }
int LuaVFS::PackU16(lua_State* L) { return PackType<boost::uint16_t>(L); }
int LuaVFS::PackU32(lua_State* L) { return PackType<boost::uint32_t>(L); }
int LuaVFS::PackS8(lua_State* L)  { return PackType<boost::int8_t>(L); }
int LuaVFS::PackS16(lua_State* L) { return PackType<boost::int16_t>(L); }
int LuaVFS::PackS32(lua_State* L) { return PackType<boost::int32_t>(L); }
int LuaVFS::PackF32(lua_State* L) { return PackType<float>(L); }


/******************************************************************************/

template <typename T>
int UnpackType(lua_State* L)
{
	if (!lua_isstring(L, 1)) {
		return 0;
	}
	size_t len;
	const char* str = lua_tolstring(L, 1, &len);

	if (lua_isnumber(L, 2)) {
		const int pos = (int)lua_tonumber(L, 2);
		if ((pos < 1) || (pos >= len)) {
			return 0;
		}
		const int offset = (pos - 1);
		str += offset;
		len -= offset;
	}
	
	const int eSize = sizeof(T);
	if (len < eSize) {
		return 0;
	}

	if (!lua_isnumber(L, 3)) {
		const T value = *((T*)str);
		lua_pushnumber(L, value);
		return 1;
	}
	else {
		const int maxCount = (len / eSize);
		int tableCount = (int)lua_tonumber(L, 3);
		if (tableCount < 0) {
			tableCount = maxCount;
		}
		tableCount = min(maxCount, tableCount);
		lua_newtable(L);
		for (int i = 0; i < tableCount; i++) {
			const T value = *(((T*)str) + i);
			lua_pushnumber(L, value);
			lua_rawseti(L, -2, (i + 1));
		}
		lua_pushstring(L, "n");
		lua_pushnumber(L, tableCount);
		lua_rawset(L, -3);
		return 1;
	}			

	return 0;
}


int LuaVFS::UnpackU8(lua_State* L)  { return UnpackType<boost::uint8_t>(L); }
int LuaVFS::UnpackU16(lua_State* L) { return UnpackType<boost::uint16_t>(L); }
int LuaVFS::UnpackU32(lua_State* L) { return UnpackType<boost::uint32_t>(L); }
int LuaVFS::UnpackS8(lua_State* L)  { return UnpackType<boost::int8_t>(L); }
int LuaVFS::UnpackS16(lua_State* L) { return UnpackType<boost::int16_t>(L); }
int LuaVFS::UnpackS32(lua_State* L) { return UnpackType<boost::int32_t>(L); }
int LuaVFS::UnpackF32(lua_State* L) { return UnpackType<float>(L); }


/******************************************************************************/
/******************************************************************************/

