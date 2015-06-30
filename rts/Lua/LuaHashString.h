/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HASH_STRING_H
#define LUA_HASH_STRING_H

#include <string>
using std::string;


#include "LuaInclude.h"


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
		inline void PushHashString(lua_State* L, const LuaHashString& hs) const {
			Push(L);
			hs.Push(L);
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
	{ static const LuaHashString hsPriv(name); hsPriv.Push(L); }

#define HSTR_PUSH_BOOL(L, name, val) \
	{ static const LuaHashString hsPriv(name); hsPriv.PushBool(L, val); }

#define HSTR_PUSH_NUMBER(L, name, val) \
	{ static const LuaHashString hsPriv(name); hsPriv.PushNumber(L, val); }

#define HSTR_PUSH_STRING(L, name, val) \
	{ static const LuaHashString hsPriv(name); hsPriv.PushString(L, val); }

#define HSTR_PUSH_HSTR(L, name, val) \
	{ static const LuaHashString hsPriv(name); hsPriv.PushHashString(L, val); }

#define HSTR_PUSH_CFUNC(L, name, val) \
	{ static const LuaHashString hsPriv(name); hsPriv.PushCFunc(L, val); }


#endif // LUA_HASH_STRING_H

