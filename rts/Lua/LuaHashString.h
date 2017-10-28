/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HASH_STRING_H
#define LUA_HASH_STRING_H

#include <string>
using std::string;


#include "LuaInclude.h"


template<unsigned int length, unsigned int step = (length >> 5) + 1, unsigned int idx = length, unsigned int stop = length % step>
struct compileTimeHasher {
	static constexpr unsigned int hash(const char *str, unsigned int prev_hash = length) {
		return compileTimeHasher<length, step, idx - step, stop>::hash(str, prev_hash ^ ((prev_hash << 5) + (prev_hash >> 2) + ((unsigned char)str[idx - 1])));
	}
};

// stopping condition
template<unsigned int length, unsigned int step, unsigned int idx>
struct compileTimeHasher<length, step, idx, idx> {
	static constexpr unsigned int hash(const char *str, unsigned int prev_hash = length) {
		return prev_hash;
	}
};


#define COMPILE_TIME_HASH(str) compileTimeHasher<sizeof(str) - 1>::hash(str)


struct LuaHashString {
	public:
		LuaHashString(const string& s)
		: str(s), hash(lua_calchash(s.c_str(), s.size())) {}

		LuaHashString(const LuaHashString& hs)
		: str(hs.str), hash(hs.hash) {}

		LuaHashString& operator=(const LuaHashString& hs) {
			str = hs.str;
			hash = hs.hash;
			return (*this);
		}

		inline unsigned int GetHash() const { return hash; }
		inline const string& GetString() const { return str; }

		void SetString(const string& s) {
			str = s;
			hash = lua_calchash(s.c_str(), s.size());
		}

	public:
		inline void Push(lua_State* L) const {
			lua_pushhstring(L, hash, str.c_str(), str.size());
		}

		inline void GetGlobal(lua_State* L) const {
			Push(L);
			lua_rawget(L, LUA_GLOBALSINDEX);
		}
		inline bool GetGlobalFunc(lua_State* L) const {
			GetGlobal(L);
			if (lua_isfunction(L, -1)) {
				return true;
			}
			lua_pop(L, 1);
			return false;
		}

		inline void GetRegistry(lua_State* L) const {
			Push(L);
			lua_rawget(L, LUA_REGISTRYINDEX);
		}
		inline bool GetRegistryFunc(lua_State* L) const {
			GetRegistry(L);
			if (lua_isfunction(L, -1)) {
				return true;
			}
			lua_pop(L, 1);
			return false;
		}

		inline void PushBool(lua_State* L, bool value) const {
			Push(L);
			lua_pushboolean(L, value);
			lua_rawset(L, -3);
		}
		inline void PushNumber(lua_State* L, lua_Number value) const {
			Push(L);
			lua_pushnumber(L, value);
			lua_rawset(L, -3);
		}
		inline void PushString(lua_State* L, const char* value) const {
			Push(L);
			lua_pushstring(L, value);
			lua_rawset(L, -3);
		}
		inline void PushString(lua_State* L, const string& value) const {
			Push(L);
			lua_pushsstring(L, value);
			lua_rawset(L, -3);
		}

		inline void PushCFunc(lua_State* L, int (*func)(lua_State*)) const {
			Push(L);
			lua_pushcfunction(L, func);
			lua_rawset(L, -3);
		}

	private:
		string str;
		unsigned int hash;
};


// NOTE: scoped to avoid name conflicts
// NOTE: since all the following are static, if name can change (e.g. within a loop)
//       peculiar things will happen. => Only use raw strings (and not variables) in name.

#define HSTR_PUSH(L, name) \
	{ lua_pushhstring(L, COMPILE_TIME_HASH(name), name, sizeof(name) - 1); }

#define HSTR_PUSH_BOOL(L, name, val) \
	{ HSTR_PUSH(L, name); lua_pushboolean(L, val); lua_rawset(L, -3); }

#define HSTR_PUSH_NUMBER(L, name, val) \
	{ HSTR_PUSH(L, name); lua_pushnumber(L, val); lua_rawset(L, -3); }

#define HSTR_PUSH_STRING(L, name, val) \
	{ HSTR_PUSH(L, name); lua_pushsstring(L, val); lua_rawset(L, -3); }

#define HSTR_PUSH_CSTRING(L, name, val) \
	{ HSTR_PUSH(L, name); lua_pushhstring(L, COMPILE_TIME_HASH(val), val, sizeof(val) - 1); lua_rawset(L, -3); }

#define HSTR_PUSH_CFUNC(L, name, val) \
	{ HSTR_PUSH(L, name); lua_pushcfunction(L, val); lua_rawset(L, -3); }


#endif // LUA_HASH_STRING_H

