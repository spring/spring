/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cmath>
#include <zlib.h>
#include <boost/regex.hpp>

#include "LuaVFS.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaIO.h"
#include "LuaUtils.h"
#include "LuaZip.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"

using std::min;


/******************************************************************************/
/******************************************************************************/

bool LuaVFS::PushCommon(lua_State* L)
{
	HSTR_PUSH_STRING(L, "RAW",       SPRING_VFS_RAW);
	HSTR_PUSH_STRING(L, "MOD",       SPRING_VFS_MOD);
	HSTR_PUSH_STRING(L, "MAP",       SPRING_VFS_MAP);
	HSTR_PUSH_STRING(L, "BASE",      SPRING_VFS_BASE);
	HSTR_PUSH_STRING(L, "ZIP",       SPRING_VFS_ZIP);
	HSTR_PUSH_STRING(L, "RAW_FIRST", SPRING_VFS_RAW_FIRST);
	HSTR_PUSH_STRING(L, "ZIP_FIRST", SPRING_VFS_ZIP_FIRST);
	HSTR_PUSH_STRING(L, "RAW_ONLY",  SPRING_VFS_RAW); // backwards compatibility
	HSTR_PUSH_STRING(L, "ZIP_ONLY",  SPRING_VFS_ZIP); // backwards compatibility

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

	HSTR_PUSH_CFUNC(L, "ZlibDecompress", ZlibDecompress);

	return true;
}


bool LuaVFS::PushSynced(lua_State* L)
{
	PushCommon(L);

	HSTR_PUSH_CFUNC(L, "Include",    SyncInclude);
	HSTR_PUSH_CFUNC(L, "LoadFile",   SyncLoadFile);
	HSTR_PUSH_CFUNC(L, "FileExists", SyncFileExists);
	HSTR_PUSH_CFUNC(L, "DirList",    SyncDirList);
	HSTR_PUSH_CFUNC(L, "SubDirs",    SyncSubDirs);

	return true;
}


bool LuaVFS::PushUnsynced(lua_State* L)
{
	PushCommon(L);

	HSTR_PUSH_CFUNC(L, "Include",		UnsyncInclude);
	HSTR_PUSH_CFUNC(L, "LoadFile",		UnsyncLoadFile);
	HSTR_PUSH_CFUNC(L, "FileExists",	UnsyncFileExists);
	HSTR_PUSH_CFUNC(L, "DirList",		UnsyncDirList);
	HSTR_PUSH_CFUNC(L, "SubDirs",		UnsyncSubDirs);
	HSTR_PUSH_CFUNC(L, "UseArchive",	UseArchive);
	HSTR_PUSH_CFUNC(L, "CompressFolder",	CompressFolder);
	HSTR_PUSH_CFUNC(L, "MapArchive",	MapArchive);
	HSTR_PUSH_CFUNC(L, "UnmapArchive",	UnmapArchive);

	HSTR_PUSH_CFUNC(L, "ZlibCompress", ZlibCompress);

	return true;
}


/******************************************************************************/
/******************************************************************************/

const string LuaVFS::GetModes(lua_State* L, int index, bool synced)
{
	const char* defMode = SPRING_VFS_RAW_FIRST;
	if (synced && !CLuaHandle::GetDevMode()) {
		defMode = SPRING_VFS_ZIP;
	}

	string modes = luaL_optstring(L, index, defMode);
	if (synced && !CLuaHandle::GetDevMode()) {
		modes = CFileHandler::ForbidModes(modes, SPRING_VFS_RAW);
	}

	return modes;
}


/******************************************************************************/

static bool LoadFileWithModes(const string& filename, string& data,
                             const string& modes)
{
	CFileHandler fh(filename, modes);
	if (!fh.FileExists()) {
		return false;
	}
	data.clear();
	if (!fh.LoadStringData(data)) {
		return false;
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/

int LuaVFS::Include(lua_State* L, bool synced)
{
	const string filename = luaL_checkstring(L, 1);
	if (!LuaIO::IsSimplePath(filename)) {
		// the path may point to a file or dir outside of any data-dir
//FIXME		return 0;
	}

	bool hasCustomEnv = false;
	if (!lua_isnoneornil(L, 2)) {
		//note this check must happen before luaL_loadbuffer gets called
		// it pushes new values on the stack and if only index 1 was given
		// to Include those by luaL_loadbuffer are pushed to index 2,3,...
		luaL_checktype(L, 2, LUA_TTABLE);
		hasCustomEnv = true;
	}

	const string modes = GetModes(L, 3, synced);

	string code;
	if (!LoadFileWithModes(filename, code, modes)) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf),
			 "Include() could not load '%s'", filename.c_str());
		lua_pushstring(L, buf);
 		lua_error(L);
	}

	int error = luaL_loadbuffer(L, code.c_str(), code.size(), filename.c_str());
	if (error != 0) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "error = %i, %s, %s",
		         error, filename.c_str(), lua_tostring(L, -1));
		lua_pushstring(L, buf);
		lua_error(L);
	}

	// set the chunk's fenv to the current fenv, or a user table
	if (hasCustomEnv) {
		lua_pushvalue(L, 2); // user fenv
	} else {
		LuaUtils::PushCurrentFuncEnv(L, __FUNCTION__);
	}

	// set the include fenv to the current function's fenv
	if (lua_setfenv(L, -2) == 0) {
		luaL_error(L, "Include(): error with setfenv");
	}

	const int paramTop = lua_gettop(L) - 1;

	error = lua_pcall(L, 0, LUA_MULTRET, 0);

	if (error != 0) {
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "error = %i, %s, %s",
		         error, filename.c_str(), lua_tostring(L, -1));
		lua_pushstring(L, buf);
		lua_error(L);
	}

	// FIXME -- adjust stack?

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
	const string filename = luaL_checkstring(L, 1);
	if (!LuaIO::IsSimplePath(filename)) {
		// the path may point to a file or dir outside of any data-dir
//FIXME		return 0;
	}

	const string modes = GetModes(L, 2, synced);

	string data;
	if (LoadFileWithModes(filename, data, modes)) {
		lua_pushsstring(L, data);
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
	const string filename = luaL_checkstring(L, 1);
	if (!LuaIO::IsSimplePath(filename)) {
		// the path may point to a file or dir outside of any data-dir
//FIXME		return 0;
	}

	const string modes = GetModes(L, 2, synced);

	//CFileHandler fh(filename, modes);
	//lua_pushboolean(L, fh.FileExists());

	lua_pushboolean(L, CFileHandler::FileExists(filename, modes));
	return 1;
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

int LuaVFS::DirList(lua_State* L, bool synced)
{
	const string dir = luaL_checkstring(L, 1);
	// keep searches within the Spring directory
	if (!LuaIO::IsSimplePath(dir)) {
		// the path may point to a file or dir outside of any data-dir
//FIXME		return 0;
	}
	const string pattern = luaL_optstring(L, 2, "*");
	const string modes = GetModes(L, 3, synced);

	LuaUtils::PushStringVector(L, CFileHandler::DirList(dir, pattern, modes));
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

int LuaVFS::SubDirs(lua_State* L, bool synced)
{
	const string dir = luaL_checkstring(L, 1);
	// keep searches within the Spring directory
	if (!LuaIO::IsSimplePath(dir)) {
		// the path may point to a file or dir outside of any data-dir
//FIXME		return 0;
	}
	const string pattern = luaL_optstring(L, 2, "*");
	const string modes = GetModes(L, 3, synced);

	LuaUtils::PushStringVector(L, CFileHandler::SubDirs(dir, pattern, modes));
	return 1;
}


int LuaVFS::SyncSubDirs(lua_State* L)
{
	return SubDirs(L, true);
}


int LuaVFS::UnsyncSubDirs(lua_State* L)
{
	return SubDirs(L, false);
}


/******************************************************************************/
/******************************************************************************/

int LuaVFS::UseArchive(lua_State* L)
{
	const string filename = luaL_checkstring(L, 1);
	if (!LuaIO::IsSimplePath(filename)) {
		// the path may point to a file or dir outside of any data-dir
		//FIXME		return 0;
	}

	int funcIndex = 2;
	if (CLuaHandle::GetHandleSynced(L)) {
		return 0;
	}

	if (!lua_isfunction(L, funcIndex)) {
		return 0;
	}

	string fileData;
	CFileHandler f(filename, SPRING_VFS_RAW);
	if (!f.FileExists()) {
		return 0;
	}

	CVFSHandler* oldHandler = vfsHandler;
	vfsHandler = new CVFSHandler;
	vfsHandler->AddArchive(filename, false);

	const int error = lua_pcall(L, lua_gettop(L) - funcIndex, LUA_MULTRET, 0);

	delete vfsHandler;
	vfsHandler = oldHandler;

	if (error != 0) {
		lua_error(L);
	}

	return lua_gettop(L) - funcIndex + 1;
}

int LuaVFS::MapArchive(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L)) {
		// only from unsynced
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	const std::string filename = archiveScanner->ArchiveFromName(luaL_checkstring(L, 1));

	if (!LuaIO::IsSimplePath(filename)) {
		// the path may point to a file or dir outside of any data-dir
		return 0;
	}

	CFileHandler f(filename, SPRING_VFS_RAW);

	if (!f.FileExists()) {
		std::ostringstream buf;
		buf << "Achive not found: " << filename;

		lua_pushboolean(L, false);
		lua_pushsstring(L, buf.str());
		return 2;
	}

	if (args >= 2) {
		// parse checksum as a STRING to convert it to a number because
		// lua numbers are float and so limited to 2^24, while the checksum is an int32.
		// const unsigned int argChecksum = lua_tonumber(L, 2);
		const unsigned int argChecksum = StringToInt(lua_tostring(L, 2));
		const unsigned int realChecksum = archiveScanner->GetSingleArchiveChecksum(filename);

		if (argChecksum != realChecksum) {
			std::ostringstream buf;

			buf << "[" << __FUNCTION__ << "] incorrect archive checksum ";
			buf << "(got: " << argChecksum << ", expected: " << realChecksum << ")";

			lua_pushboolean(L, false);
			lua_pushsstring(L, buf.str());
			return 2;
		}
	}

	if (!vfsHandler->AddArchive(filename, false)) {
		std::ostringstream buf;
		buf << "[" << __FUNCTION__ << "] failed to load archive: " << filename;

		lua_pushboolean(L, false);
		lua_pushsstring(L, buf.str());
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

int LuaVFS::UnmapArchive(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L)) {
		// only from unsynced
		return 0;
	}

	const std::string filename = archiveScanner->ArchiveFromName(luaL_checkstring(L, 1));

	if (!LuaIO::IsSimplePath(filename)) {
		// the path may point to a file or dir outside of any data-dir
		return 0;
	}

	if (!vfsHandler->RemoveArchive(filename)) {
		std::ostringstream buf;
		buf << "[" << __FUNCTION__ << "] failed to remove archive: " << filename;

		lua_pushboolean(L, false);
		lua_pushsstring(L, buf.str());
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}



/******************************************************************************/

int LuaVFS::CompressFolder(lua_State* L)
{
	const string folderPath = luaL_checkstring(L, 1);

	const string archiveType = luaL_optstring(L, 2, "zip");
	if (archiveType != "zip" && archiveType != "7z") { //TODO: add 7z support
		luaL_error(L, ("Unsupported archive type " + archiveType).c_str());
	}

	 // "sdz" is the default type if not specified
	const string compressedFilePath = luaL_optstring(L, 3, (folderPath + ".sdz").c_str());
	const bool includeFolder = luaL_optboolean(L, 4, false);
	const string modes = GetModes(L, 5, false);

	if (CFileHandler::FileExists(compressedFilePath, modes)) {
		luaL_error(L, ("File already exists " + compressedFilePath).c_str());
	}
	if (archiveType == "zip") {
		LuaZipFolder::ZipFolder(L, folderPath, compressedFilePath, includeFolder, modes);
	} else if (archiveType == "7z") {
		SevenZipFolder(L, folderPath, compressedFilePath, includeFolder, modes);
	}
	return 0;
}

/******************************************************************************/

int LuaVFS::SevenZipFolder(lua_State* L, const string& folderPath, const string& zipFilePath, bool includeFolder, const string& modes)
{
	luaL_error(L, "Seven zip compression is not implemented yet.");
	return 0;
}


int LuaVFS::ZlibCompress(lua_State* L)
{
	size_t inLen;
	const char* inData = luaL_checklstring(L, 1, &inLen);

	long unsigned bufsize = compressBound(inLen);
	std::vector<boost::uint8_t> compressed(bufsize, 0);
	const int error = compress(&compressed[0], &bufsize, (const boost::uint8_t*)inData, inLen);
	if (error == Z_OK)
	{
		lua_pushlstring(L, (const char*)&compressed[0], bufsize);
		return 1;
	}
	else
	{
		return luaL_error(L, "Error while compressing");
	}
}


int LuaVFS::ZlibDecompress(lua_State* L)
{
	size_t inLen;
	const char* inData = luaL_checklstring(L, 1, &inLen);

	long unsigned bufsize = std::max(luaL_optint(L, 2, 65000), 0);

	std::vector<boost::uint8_t> uncompressed(bufsize, 0);
	const int error = uncompress(&uncompressed[0], &bufsize, (const boost::uint8_t*)inData, inLen);
	if (error == Z_OK)
	{
		lua_pushlstring(L, (const char*)&uncompressed[0], bufsize);
		return 1;
	}
	else
	{
		return luaL_error(L, "Error while decompressing");
	}
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
		for (int i = 1;
		     lua_rawgeti(L, 1, i), lua_isnumber(L, -1);
		     lua_pop(L, 1), i++) {
			vals.push_back((T)lua_tonumber(L, -1));
		}
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
	char* buf = new char[bufSize];
	for (int i = 0; i < (int)vals.size(); i++) {
		memcpy(buf + (i * sizeof(T)), &vals[i], sizeof(T));
	}
	lua_pushlstring(L, buf, bufSize);
	delete[] buf;

	return 1;
}


int LuaVFS::PackU8(lua_State*  L) { return PackType<boost::uint8_t>(L);  }
int LuaVFS::PackU16(lua_State* L) { return PackType<boost::uint16_t>(L); }
int LuaVFS::PackU32(lua_State* L) { return PackType<boost::uint32_t>(L); }
int LuaVFS::PackS8(lua_State*  L) { return PackType<boost::int8_t>(L);   }
int LuaVFS::PackS16(lua_State* L) { return PackType<boost::int16_t>(L);  }
int LuaVFS::PackS32(lua_State* L) { return PackType<boost::int32_t>(L);  }
int LuaVFS::PackF32(lua_State* L) { return PackType<float>(L);           }


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
		const int pos = lua_toint(L, 2);
		if ((pos < 1) || ((size_t)pos >= len)) {
			return 0;
		}
		const int offset = (pos - 1);
		str += offset;
		len -= offset;
	}

	const size_t eSize = sizeof(T);
	if (len < eSize) {
		return 0;
	}

	if (!lua_isnumber(L, 3)) {
		const T value = *((T*)str);
		lua_pushnumber(L, value);
		return 1;
	}
	else {
		const size_t maxCount = (len / eSize);
		int tableCount = lua_toint(L, 3);
		if (tableCount < 0) {
			tableCount = maxCount;
		}
		tableCount = min((int)maxCount, tableCount);
		lua_newtable(L);
		for (int i = 0; i < tableCount; i++) {
			const T value = *(((T*)str) + i);
			lua_pushnumber(L, value);
			lua_rawseti(L, -2, (i + 1));
		}
		return 1;
	}

	return 0;
}


int LuaVFS::UnpackU8(lua_State*  L) { return UnpackType<boost::uint8_t>(L);  }
int LuaVFS::UnpackU16(lua_State* L) { return UnpackType<boost::uint16_t>(L); }
int LuaVFS::UnpackU32(lua_State* L) { return UnpackType<boost::uint32_t>(L); }
int LuaVFS::UnpackS8(lua_State*  L) { return UnpackType<boost::int8_t>(L);   }
int LuaVFS::UnpackS16(lua_State* L) { return UnpackType<boost::int16_t>(L);  }
int LuaVFS::UnpackS32(lua_State* L) { return UnpackType<boost::int32_t>(L);  }
int LuaVFS::UnpackF32(lua_State* L) { return UnpackType<float>(L);           }


/******************************************************************************/
/******************************************************************************/

