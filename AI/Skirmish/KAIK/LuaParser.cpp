#include <iostream>
#include <cassert>
#include <boost/foreach.hpp>

#ifdef LUA_LIB_EXT
#include <lua5.1/lua.hpp>
#else
#include "lib/lua/include/lua.h"
#include "lib/lua/include/lualib.h"
#include "lib/lua/include/lauxlib.h"
#endif

#include "./Util.h"
#include "./LuaParser.hpp"

void LuaTable::Print(int depth) const {
	std::string tabs = "";
	for (int i = 0; i < depth; i++) {
		tabs += "\t";
	}

	for (std::map<LuaTable*, LuaTable*>::const_iterator it = TblTblPairs.begin(); it != TblTblPairs.end(); it++) {
		std::cout << tabs << "k<tbl>: ";
		std::cout << std::endl;
			it->first->Print(depth + 1);
		std::cout << tabs << "v<tbl>: ";
		std::cout << std::endl;
			it->second->Print(depth + 1);
	}
	for (std::map<LuaTable*, std::string>::const_iterator it = TblStrPairs.begin(); it != TblStrPairs.end(); it++) {
		std::cout << tabs << "k<tbl>: ";
		std::cout << std::endl;
			it->first->Print(depth + 1);
		std::cout << tabs << "v<str>: " << it->second;
		std::cout << std::endl;
	}
	for (std::map<LuaTable*, int>::const_iterator it = TblIntPairs.begin(); it != TblIntPairs.end(); it++) {
		std::cout << tabs << "k<tbl>: ";
		std::cout << std::endl;
			it->first->Print(depth + 1);
		std::cout << tabs << "v<int>: " << it->second;
		std::cout << std::endl;
	}

	for (std::map<std::string, LuaTable*>::const_iterator it = StrTblPairs.begin(); it != StrTblPairs.end(); it++) {
		std::cout << tabs << "k<str>: " << it->first;
		std::cout << ", v<tbl>: ";
		std::cout << std::endl;
			it->second->Print(depth + 1);
	}
	for (std::map<std::string, std::string>::const_iterator it = StrStrPairs.begin(); it != StrStrPairs.end(); it++) {
		std::cout << tabs << "k<str>: " << it->first;
		std::cout << ", v<str>: " << it->second;
		std::cout << std::endl;
	}
	for (std::map<std::string, int>::const_iterator it = StrIntPairs.begin(); it != StrIntPairs.end(); it++) {
		std::cout << tabs << "k<str>: " << it->first;
		std::cout << ", v<int>: " << it->second;
		std::cout << std::endl;
	}

	for (std::map<int, LuaTable*>::const_iterator it = IntTblPairs.begin(); it != IntTblPairs.end(); it++) {
		std::cout << tabs << "k<int>: " << it->first;
		std::cout << ", v<tbl>: ";
		std::cout << std::endl;
			it->second->Print(depth + 1);
	}
	for (std::map<int, std::string>::const_iterator it = IntStrPairs.begin(); it != IntStrPairs.end(); it++) {
		std::cout << tabs << "k<int>: " << it->first;
		std::cout << ", v<str>: " << it->second;
		std::cout << std::endl;
	}
	for (std::map<int, int>::const_iterator it = IntIntPairs.begin(); it != IntIntPairs.end(); it++) {
		std::cout << tabs << "k<int>: " << it->first;
		std::cout << ", v<int>: " << it->second;
		std::cout << std::endl;
	}
}

void LuaTable::Parse(lua_State* ls, int depth) {
	assert(lua_istable(ls, -1));
	lua_pushnil(ls);
	assert(lua_istable(ls, -2));

	while (lua_next(ls, -2) != 0) {
		assert(lua_istable(ls, -3));

		switch (lua_type(ls, -2)) {
			case LUA_TTABLE: {
				LuaTable* key = new LuaTable();

				switch (lua_type(ls, -1)) {
					case LUA_TTABLE: {
						TblTblPairs[key] = new LuaTable();
						TblTblPairs[key]->Parse(ls, depth + 1);

						lua_pop(ls, 1);

						key->Parse(ls, depth + 1);
					} break;
					case LUA_TSTRING: {
						TblStrPairs[key] = AIUtil::StringToLower(lua_tostring(ls, -1));
						lua_pop(ls, 1);

						key->Parse(ls, depth + 1);
					} break;
					case LUA_TNUMBER: {
						TblIntPairs[key] = lua_tointeger(ls, -1);
						lua_pop(ls, 1);

						key->Parse(ls, depth + 1);
					} break;
				}

				continue;
			} break;

			case LUA_TSTRING: {
				const std::string key = AIUtil::StringToLower(lua_tostring(ls, -2));

				switch (lua_type(ls, -1)) {
					case LUA_TTABLE: {
						StrTblPairs[key] = new LuaTable();
						StrTblPairs[key]->Parse(ls, depth + 1);
					} break;
					case LUA_TSTRING: {
						StrStrPairs[key] = AIUtil::StringToLower(lua_tostring(ls, -1));
					} break;
					case LUA_TNUMBER: {
						StrIntPairs[key] = lua_tointeger(ls, -1);
					} break;
				}

				lua_pop(ls, 1);
				continue;
			} break;

			case LUA_TNUMBER: {
				const int key = lua_tointeger(ls, -2);

				switch (lua_type(ls, -1)) {
					case LUA_TTABLE: {
						IntTblPairs[key] = new LuaTable();
						IntTblPairs[key]->Parse(ls, depth + 1);
					} break;
					case LUA_TSTRING: {
						IntStrPairs[key] = AIUtil::StringToLower(lua_tostring(ls, -1));
					} break;
					case LUA_TNUMBER: {
						IntIntPairs[key] = lua_tointeger(ls, -1);
					} break;
				}

				lua_pop(ls, 1);
				continue;
			} break;
		}
	}

	assert(lua_istable(ls, -1));
}

void LuaTable::GetTblTblKeys(std::list<LuaTable*>*   keys) const { BOOST_FOREACH(TblTblPair p, TblTblPairs) { keys->push_back(p.first); } }
void LuaTable::GetTblStrKeys(std::list<LuaTable*>*   keys) const { BOOST_FOREACH(TblStrPair p, TblStrPairs) { keys->push_back(p.first); } }
void LuaTable::GetTblIntKeys(std::list<LuaTable*>*   keys) const { BOOST_FOREACH(TblIntPair p, TblIntPairs) { keys->push_back(p.first); } }
void LuaTable::GetStrTblKeys(std::list<std::string>* keys) const { BOOST_FOREACH(StrTblPair p, StrTblPairs) { keys->push_back(p.first); } }
void LuaTable::GetStrStrKeys(std::list<std::string>* keys) const { BOOST_FOREACH(StrStrPair p, StrStrPairs) { keys->push_back(p.first); } }
void LuaTable::GetStrIntKeys(std::list<std::string>* keys) const { BOOST_FOREACH(StrIntPair p, StrIntPairs) { keys->push_back(p.first); } }
void LuaTable::GetIntTblKeys(std::list<int>*         keys) const { BOOST_FOREACH(IntTblPair p, IntTblPairs) { keys->push_back(p.first); } }
void LuaTable::GetIntStrKeys(std::list<int>*         keys) const { BOOST_FOREACH(IntStrPair p, IntStrPairs) { keys->push_back(p.first); } }
void LuaTable::GetIntIntKeys(std::list<int>*         keys) const { BOOST_FOREACH(IntIntPair p, IntIntPairs) { keys->push_back(p.first); } }

const LuaTable* LuaTable::GetTblVal(LuaTable* key, LuaTable* defVal) const {
	const std::map<LuaTable*, LuaTable*>::const_iterator it = TblTblPairs.find(key);
	return ((it != TblTblPairs.end())? it->second: defVal);
}
const LuaTable* LuaTable::GetTblVal(const std::string& key, LuaTable* defVal) const {
	const std::map<std::string, LuaTable*>::const_iterator it = StrTblPairs.find(key);
	return ((it != StrTblPairs.end())? it->second: defVal);
}
const LuaTable* LuaTable::GetTblVal(int key, LuaTable* defVal) const {
	const std::map<int, LuaTable*>::const_iterator it = IntTblPairs.find(key);
	return ((it != IntTblPairs.end())? it->second: defVal);
}

const std::string& LuaTable::GetStrVal(LuaTable* key, const std::string& defVal) const {
	const std::map<LuaTable*, std::string>::const_iterator it = TblStrPairs.find(key);
	return ((it != TblStrPairs.end())? it->second: defVal);
}
const std::string& LuaTable::GetStrVal(const std::string& key, const std::string& defVal) const {
	const std::map<std::string, std::string>::const_iterator it = StrStrPairs.find(key);
	return ((it != StrStrPairs.end())? it->second: defVal);
}
const std::string& LuaTable::GetStrVal(int key, const std::string& defVal) const {
	const std::map<int, std::string>::const_iterator it = IntStrPairs.find(key);
	return ((it != IntStrPairs.end())? it->second: defVal);
}

int LuaTable::GetIntVal(LuaTable* key, int defVal) const {
	const std::map<LuaTable*, int>::const_iterator it = TblIntPairs.find(key);
	return ((it != TblIntPairs.end())? it->second: defVal);
}
int LuaTable::GetIntVal(const std::string& key, int defVal) const {
	const std::map<std::string, int>::const_iterator it = StrIntPairs.find(key);
	return ((it != StrIntPairs.end())? it->second: defVal);
}
int LuaTable::GetIntVal(int key, int defVal) const {
	const std::map<int, int>::const_iterator it = IntIntPairs.find(key);
	return ((it != IntIntPairs.end())? it->second: defVal);
}



LuaParser::LuaParser(): luaState(lua_open()), root(NULL) {
	luaL_openlibs(luaState);
}
LuaParser::~LuaParser() {
	for (std::map<std::string, LuaTable*>::iterator it = tables.begin(); it != tables.end(); it++) {
		delete it->second;
	}

	root = NULL;
	lua_close(luaState);
}

bool LuaParser::Execute(const std::string& file, const std::string& table) {
	bool ret = false;

	int loadErr = 0; // 0 | LUA_ERRFILE | LUA_ERRSYNTAX | LUA_ERRMEM
	int callErr = 0; // 0 | LUA_ERRRUN | LUA_ERRMEM | LUA_ERRERR

	if ((loadErr = luaL_loadfile(luaState, file.c_str())) != 0 || (callErr = lua_pcall(luaState, 0, 0, 0)) != 0) {
		error = std::string(lua_tostring(luaState, -1));
		lua_pop(luaState, 1);
		return false;
	}

	if (tables.find(file) == tables.end()) {
		tables[file] = new LuaTable();
		root = tables[file];

		assert(lua_gettop(luaState) == 0);
		lua_getglobal(luaState, table.c_str());

		if (lua_isnil(luaState, -1) == 0) {
			assert(lua_istable(luaState, -1));
			root->Parse(luaState, 0);
			ret = true;
		} else {
			error = "no global table variable \'" + table + "\' declared in chunk \'" + file + "\'";
		}

		lua_pop(luaState, 1);
		assert(lua_gettop(luaState) == 0);
	} else {
		root = tables[file];
		ret = true;
	}

	return ret;
}
