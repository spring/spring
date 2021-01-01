/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_PARSER_H
#define LUA_PARSER_H

#include <string>
#include <vector>

#include "LuaContextData.h"

#include "System/FileSystem/VFSModes.h"
#include "System/UnorderedMap.hpp"

class float3;
struct float4;
class LuaTable;
class LuaParser;
struct lua_State;


/******************************************************************************/

class LuaTable {
friend class LuaParser;

public:
	LuaTable();
	LuaTable(const LuaTable& tbl);
	LuaTable& operator=(const LuaTable& tbl);
	~LuaTable();

	LuaTable SubTable(int key) const;
	LuaTable SubTable(const std::string& key) const;
	LuaTable SubTableExpr(const std::string& expr) const;

	bool IsValid() const { return (parser != nullptr); }

	const std::string& GetPath() const { return path; }

	int GetLength() const;                  // lua '#' operator
	int GetLength(int key) const;           // lua '#' operator
	int GetLength(const std::string& key) const; // lua '#' operator

	bool GetKeys(std::vector<int>& data) const;
	bool GetKeys(std::vector<std::string>& data) const;

	bool GetPairs(std::vector<std::pair<int, float>>& data) const { return false; } // TODO
	bool GetPairs(std::vector<std::pair<int, std::string>>& data) const;
	bool GetPairs(std::vector<std::pair<std::string, float>>& data) const;
	bool GetPairs(std::vector<std::pair<std::string, std::string>>& data) const;

	bool GetMap(spring::unordered_map<int, float>& data) const;
	bool GetMap(spring::unordered_map<int, std::string>& data) const;
	bool GetMap(spring::unordered_map<std::string, float>& data) const;
	bool GetMap(spring::unordered_map<std::string, std::string>& data) const;

	bool KeyExists(int key) const;
	bool KeyExists(const std::string& key) const;

	enum DataType {
		NIL     = -1,
		NUMBER  = 1,
		STRING  = 2,
		BOOLEAN = 3,
		TABLE   = 4
	};
	DataType GetType(int key) const;
	DataType GetType(const std::string& key) const;

	// numeric keys
	template<typename T> T Get(int key, T def) const;
	int    Get(int key, int def) const;
	bool   Get(int key, bool def) const;
	float  Get(int key, float def) const;
	float3 Get(int key, const float3& def) const;
	float4 Get(int key, const float4& def) const;
	std::string Get(int key, const std::string& def) const;
	unsigned int Get(int key, unsigned int def) const { return (unsigned int)Get(key, (int)def); }

	// string keys  (always lowercase)
	template<typename T> T Get(const std::string& key, T def) const;
	int    Get(const std::string& key, int def) const;
	bool   Get(const std::string& key, bool def) const;
	float  Get(const std::string& key, float def) const;
	float3 Get(const std::string& key, const float3& def) const;
	float4 Get(const std::string& key, const float4& def) const;
	std::string Get(const std::string& key, const std::string& def) const;
	unsigned int Get(const std::string& key, unsigned int def) const { return (unsigned int)Get(key, (int)def); }

	template<typename T> int    GetInt(T key, int def) const { return Get(key, def); }
	template<typename T> bool   GetBool(T key, bool def) const { return Get(key, def); }
	template<typename T> float  GetFloat(T key, float def) const { return Get(key, def); }
	template<typename T> std::string GetString(T key, const std::string& def) const { return Get(key, def); }
	// we cannot use templates for float3/4 cause then we would need to #include "float3.h" in this header
	//template<typename T> float3 GetFloat3(T key, const float3& def) const { return Get(key, def); }
	//template<typename T> float4 GetFloat4(T key, const float4& def) const { return Get(key, def); }
	float3 GetFloat3(int key, const float3& def) const;
	float4 GetFloat4(int key, const float4& def) const;
	float3 GetFloat3(const std::string& key, const float3& def) const;
	float4 GetFloat4(const std::string& key, const float4& def) const;

private:
	LuaTable(LuaParser* parser); // for LuaParser::GetRoot()

	bool PushTable() const;
	bool PushValue(int key) const;
	bool PushValue(const std::string& key) const;

private:
	std::string path;
	mutable bool isValid;
	LuaParser* parser;
	lua_State* L;
	int refnum;
};


/******************************************************************************/

class LuaParser {
private:
	friend class LuaTable;
	// prevent implicit bool-to-string conversion
	struct boolean { bool b; };

public:
	LuaParser() = default;
	LuaParser(const std::string& fileName, const std::string& fileModes, const std::string& accessModes, const boolean& synced = {false}, const boolean& setup = {true});
	LuaParser(const std::string& textChunk, const std::string& accessModes, int = 0, const boolean& synced = {false}, const boolean& setup = {true});
	~LuaParser();

	void SetupLua(bool isSyncedCtxt, bool isDefsParser);

	bool Execute();
	bool IsValid() const { return (L != nullptr); } // true if nothing failed during Execute
	bool NoTable() const { return (errorLog.find("no return table") == 0); } // parser is still valid if true

	LuaTable GetRoot();
	LuaTable SubTableExpr(const std::string& expr) {
		return GetRoot().SubTableExpr(expr);
	}

	const std::string& GetErrorLog() const { return errorLog; }

	// for setting up the initial params table
	void GetTable(int index,               bool overwrite = false);
	void GetTable(const std::string& name, bool overwrite = false);
	void EndTable();
	void AddFunc(int key, int (*func)(lua_State*));
	void AddInt(int key, int value);
	void AddBool(int key, bool value);
	void AddFloat(int key, float value);
	void AddString(int key, const std::string& value);
	void AddFunc(const std::string& key, int (*func)(lua_State*));
	void AddInt(const std::string& key, int value);
	void AddBool(const std::string& key, bool value);
	void AddFloat(const std::string& key, float value);
	void AddString(const std::string& key, const std::string& value);

	void SetLowerKeys(bool state) { lowerKeys = state; }
	void SetLowerCppKeys(bool state) { lowerCppKeys = state; }

public:
	const std::string fileName;
	const std::string fileModes;
	const std::string textChunk;
	const std::string accessModes;

private:
	void SetupEnv(bool isSyncedCtxt, bool isDefsParser);
	void PushParam();

	void AddTable(LuaTable* tbl);
	void RemoveTable(LuaTable* tbl);

private:
	lua_State* L = nullptr;
	luaContextData D;

	// NOTE: holds *stack* pointers
	std::vector<LuaTable*> tables;
	// unused
	std::vector<std::string> accessedFiles;

	std::string errorLog;

	int initDepth = -1;
	int rootRef = -1;
	int currentRef = -1;

	bool valid = false;
	bool lowerKeys = false; // convert all returned keys to lower case
	bool lowerCppKeys = false; // convert strings in arguments keys to lower case

private:
	// Weird call-outs
	static int DontMessWithMyCase(lua_State* L);

	// Spring call-outs
	static int RandomSeed(lua_State* L);
	static int Random(lua_State* L);
	static int DummyRandomSeed(lua_State* L);
	static int DummyRandom(lua_State* L);
	static int TimeCheck(lua_State* L);

	// VFS call-outs
	static int DirList(lua_State* L);
	static int SubDirs(lua_State* L);
	static int Include(lua_State* L);
	static int LoadFile(lua_State* L);
	static int FileExists(lua_State* L);
};


/******************************************************************************/

#endif /* LUA_PARSER_H */
