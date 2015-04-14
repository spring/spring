/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_IO_H
#define LUA_IO_H

#include <stdio.h>
#include <string>

struct lua_State;


class LuaIO {
	public:
		static FILE* fopen(lua_State* L, const char* path, const char* mode);
		static FILE* popen(lua_State* L, const char* command, const char* type);
		static int   pclose(lua_State* L, FILE* stream);
		static int   system(lua_State* L, const char* command);
		static int   remove(lua_State* L, const char* pathname);
		static int   rename(lua_State* L, const char* oldpath, const char* newpath);

	public:
		/// relative path, with no ..'s
		static bool IsSimplePath(const std::string& path);

		static bool SafeExecPath(const std::string& path);
		static bool SafeReadPath(const std::string& path);
		static bool SafeWritePath(const std::string& path);
};

#endif /* LUA_IO_H */
