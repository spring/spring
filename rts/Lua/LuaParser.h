/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_PARSER_H
#define LUA_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <set>
using std::string;
using std::vector;
using std::map;
using std::set;

#include "FileSystem/VFSModes.h"

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
		LuaTable SubTable(const string& key) const;
		LuaTable SubTableExpr(const string& expr) const;

		bool IsValid() const { return (parser != NULL); }

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

		int GetType(int key) const;
		int GetType(const string& key) const;

		// numeric keys
		int    GetInt(int key, int def) const;
		bool   GetBool(int key, bool def) const;
		float  GetFloat(int key, float def) const;
		float3 GetFloat3(int key, const float3& def) const;
		float4 GetFloat4(int key, const float4& def) const;
		string GetString(int key, const string& def) const;

		// string keys  (always lowercase)
		int    GetInt(const string& key, int def) const;
		bool   GetBool(const string& key, bool def) const;
		float  GetFloat(const string& key, float def) const;
		float3 GetFloat3(const string& key, const float3& def) const;
		float4 GetFloat4(const string& key, const float4& def) const;
		string GetString(const string& key, const string& def) const;

		/* not having these makes for better code, imo
		LuaTable operator[](int key)           const { return SubTable(key); }
		LuaTable operator[](const string& key) const { return SubTable(key); }
		int    operator()(int key, int def)           const { return GetInt(key, def);    }
		bool   operator()(int key, bool def)          const { return GetBool(key, def);   }
		float  operator()(int key, float def)         const { return GetFloat(key, def);  }
		float3 operator()(int key, const float3& def) const { return GetFloat3(key, def); }
		string operator()(int key, const string& def) const { return GetString(key, def); }
		int    operator()(const string& key, int def)           const { return GetInt(key, def);    }
		bool   operator()(const string& key, bool def)          const { return GetBool(key, def);   }
		float  operator()(const string& key, float def)         const { return GetFloat(key, def);  }
		float3 operator()(const string& key, const float3& def) const { return GetFloat3(key, def); }
		string operator()(const string& key, const string& def) const { return GetString(key, def); }
		*/

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

	friend class LuaTable;

	public:
		LuaParser(const string& fileName,
		          const string& fileModes,
		          const string& accessModes);
		LuaParser(const string& textChunk,
		          const string& accessModes);
		~LuaParser();

		bool Execute();

		bool IsValid() const { return (L != NULL); }

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
		void SetupEnv();

		void PushParam();

		void AddTable(LuaTable* tbl);
		void RemoveTable(LuaTable* tbl);

	private:
		bool valid;
		int initDepth;

		lua_State* L;
		set<LuaTable*> tables;
		int rootRef;
		int currentRef;

		bool lowerKeys; // convert all returned keys to lower case
		bool lowerCppKeys; // convert strings in arguments keys to lower case

		string errorLog;
		set<string> accessedFiles;

	private:
		// Weird call-outs
		static int DontMessWithMyCase(lua_State* L);

		// Spring call-outs
		static int Echo(lua_State* L);
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
