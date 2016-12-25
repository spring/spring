/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_PARSER_H
#define LUA_PARSER_H

#include "System/FileSystem/VFSModes.h"

#include <string>
#include <vector>
#include <map>
#include <set>

class float3;
struct float4;
class LuaTable;
class LuaParser;
struct lua_State;

using std::string;
using std::vector;
using std::map;
using std::set;


/******************************************************************************/

class LuaTable {

	friend class LuaParser;

	public:
		LuaTable();
		LuaTable(const LuaTable& tbl);
		LuaTable& operator=(const LuaTable& tbl);
		~LuaTable();

		LuaTable SubTable(int key) const;
		LuaTable SubTable(const string& key) const;
		LuaTable SubTableExpr(const string& expr) const;

		bool IsValid() const { return (parser != nullptr); }

		const string& GetPath() const { return path; }

		int GetLength() const;                  // lua '#' operator
		int GetLength(int key) const;           // lua '#' operator
		int GetLength(const string& key) const; // lua '#' operator

		bool GetKeys(vector<int>& data) const;
		bool GetKeys(vector<string>& data) const;

		bool GetMap(map<int, float>& data) const;
		bool GetMap(map<int, string>& data) const;
		bool GetMap(map<string, float>& data) const;
		bool GetMap(map<string, string>& data) const;

		bool KeyExists(int key) const;
		bool KeyExists(const string& key) const;

		enum DataType {
			NIL     = -1,
			NUMBER  = 1,
			STRING  = 2,
			BOOLEAN = 3,
			TABLE   = 4
		};
		DataType GetType(int key) const;
		DataType GetType(const string& key) const;

		// numeric keys
		template<typename T> T Get(int key, T def) const;
		int    Get(int key, int def) const;
		bool   Get(int key, bool def) const;
		float  Get(int key, float def) const;
		float3 Get(int key, const float3& def) const;
		float4 Get(int key, const float4& def) const;
		string Get(int key, const string& def) const;
		unsigned int Get(int key, unsigned int def) const { return (unsigned int)Get(key, (int)def); }

		// string keys  (always lowercase)
		template<typename T> T Get(const string& key, T def) const;
		int    Get(const string& key, int def) const;
		bool   Get(const string& key, bool def) const;
		float  Get(const string& key, float def) const;
		float3 Get(const string& key, const float3& def) const;
		float4 Get(const string& key, const float4& def) const;
		string Get(const string& key, const string& def) const;
		unsigned int Get(const string& key, unsigned int def) const { return (unsigned int)Get(key, (int)def); }

		template<typename T> int    GetInt(T key, int def) const { return Get(key, def); }
		template<typename T> bool   GetBool(T key, bool def) const { return Get(key, def); }
		template<typename T> float  GetFloat(T key, float def) const { return Get(key, def); }
		template<typename T> string GetString(T key, const string& def) const { return Get(key, def); }
		// we cannot use templates for float3/4 cause then we would need to #include "float3.h" in this header
		//template<typename T> float3 GetFloat3(T key, const float3& def) const { return Get(key, def); }
		//template<typename T> float4 GetFloat4(T key, const float4& def) const { return Get(key, def); }
		float3 GetFloat3(int key, const float3& def) const;
		float4 GetFloat4(int key, const float4& def) const;
		float3 GetFloat3(const string& key, const float3& def) const;
		float4 GetFloat4(const string& key, const float4& def) const;

	private:
		LuaTable(LuaParser* parser); // for LuaParser::GetRoot()

		bool PushTable() const;
		bool PushValue(int key) const;
		bool PushValue(const string& key) const;

	private:
		string path;
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
	LuaParser(const string& fileName, const string& fileModes, const string& accessModes, const boolean& synced = {false});
	LuaParser(const string& textChunk, const string& accessModes, const boolean& synced = {false});
	~LuaParser();

	bool Execute();
	bool IsValid() const { return (L != nullptr); }

	LuaTable GetRoot();

	LuaTable SubTableExpr(const string& expr) {
		return GetRoot().SubTableExpr(expr);
	}

	const string& GetErrorLog() const { return errorLog; }

	const set<string>& GetAccessedFiles() const { return accessedFiles; }

	// for setting up the initial params table
	void GetTable(int index,          bool overwrite = false);
	void GetTable(const string& name, bool overwrite = false);
	void EndTable();
	void AddFunc(int key, int (*func)(lua_State*));
	void AddInt(int key, int value);
	void AddBool(int key, bool value);
	void AddFloat(int key, float value);
	void AddString(int key, const string& value);
	void AddFunc(const string& key, int (*func)(lua_State*));
	void AddInt(const string& key, int value);
	void AddBool(const string& key, bool value);
	void AddFloat(const string& key, float value);
	void AddString(const string& key, const string& value);

	void SetLowerKeys(bool state) { lowerKeys = state; }
	void SetLowerCppKeys(bool state) { lowerCppKeys = state; }

public:
	const string fileName;
	const string fileModes;
	const string textChunk;
	const string accessModes;

private:
	void SetupEnv(bool synced);

	void PushParam();

	void AddTable(LuaTable* tbl);
	void RemoveTable(LuaTable* tbl);

private:
	lua_State* L;
	set<LuaTable*> tables;

	int initDepth;
	int rootRef;
	int currentRef;

	bool valid;
	bool lowerKeys; // convert all returned keys to lower case
	bool lowerCppKeys; // convert strings in arguments keys to lower case

	string errorLog;
	set<string> accessedFiles;

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

private:
	static LuaParser* currentParser;
};


/******************************************************************************/

#endif /* LUA_PARSER_H */
