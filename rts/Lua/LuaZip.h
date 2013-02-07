/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LUA_ZIP_H_
#define _LUA_ZIP_H_

#include <string>

class IArchive;
struct lua_State;

#ifndef zipFile
	// might be defined through zip.h already
	typedef void* zipFile;
#endif


class LuaZipFileWriter {
public:
	LuaZipFileWriter();
	~LuaZipFileWriter();

	static bool PushSynced(lua_State* L);
	static bool PushUnsynced(lua_State* L);
	static bool PushNew(lua_State* L, const std::string& filename, zipFile zip);

private: // metatable methods
	static bool CreateMetatable(lua_State* L);
	static int open(lua_State* L);
	static int meta_gc(lua_State* L);
	static int meta_open(lua_State* L);
	static int meta_write(lua_State* L);
};


class LuaZipFileReader {
public:
	LuaZipFileReader();
	~LuaZipFileReader();

	static bool PushSynced(lua_State* L);
	static bool PushUnsynced(lua_State* L);
	static bool PushNew(lua_State* L, const std::string& filename, IArchive* archive);

private: // metatable methods
	static bool CreateMetatable(lua_State* L);
	static int open(lua_State* L);
	static int meta_gc(lua_State* L);
	static int meta_open(lua_State* L);
	static int meta_read(lua_State* L);
};

class LuaZipFolder {
public:
    static int ZipFolder(lua_State* L, const std::string& folderPath, const std::string& zipFilePath, bool includeFolder, const std::string& modes);
};


#endif /* _LUA_ZIP_H_ */
