/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUAZIP_H_
#define LUAZIP_H_


#include <string>


typedef void* zipFile;
class CArchiveBase;
struct lua_State;


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
	static bool PushNew(lua_State* L, const std::string& filename, CArchiveBase* archive);

private: // metatable methods
	static bool CreateMetatable(lua_State* L);
	static int open(lua_State* L);
	static int meta_gc(lua_State* L);
	static int meta_open(lua_State* L);
	static int meta_read(lua_State* L);
};


#endif /* LUAZIP_H_ */
