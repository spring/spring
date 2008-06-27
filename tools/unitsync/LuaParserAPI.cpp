/******************************************************************************/
/******************************************************************************/
//
// file:   LuaParserAPI.cpp
// desc:   LuaParser C interface
// author: Dave Rodgers (aka: trepan)
// 
// LuaParser C interface
//
// Copyright (C) 2008.
// Licensed under the terms of the GNU GPL, v2 or later
//
/******************************************************************************/
/******************************************************************************/

#include "StdAfx.h"
#include "Lua/LuaParser.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "unitsync.h"
#include <string>
#include <vector>
using std::string;
using std::vector;


/******************************************************************************/

static LuaParser* luaParser = NULL;

static LuaTable rootTable;
static LuaTable currTable;
static vector<LuaTable> luaTables;

static vector<int> intKeys;
static vector<string> stringKeys;


/******************************************************************************/
//
//  Primary calls
//

DLL_EXPORT void __stdcall lpClose()
{
	rootTable = LuaTable();
	currTable = LuaTable();

	luaTables.clear();

	intKeys.clear();
	stringKeys.clear();

	delete luaParser;
	luaParser = NULL;

	return;
}


DLL_EXPORT int __stdcall lpOpenFile(const char* filename,
                                    const char* fileModes,
                                    const char* accessModes)
{
	lpClose();
	luaParser = new LuaParser(filename, fileModes, accessModes);
	return 1;
}


DLL_EXPORT int __stdcall lpOpenSource(const char* source,
                                      const char* accessModes)
{
	lpClose();
	luaParser = new LuaParser(source, accessModes);
	return 1;
}


DLL_EXPORT int __stdcall lpExecute()
{
	if (!luaParser) {
		return 0;
	}
	const bool success = luaParser->Execute();
	rootTable = luaParser->GetRoot();
	currTable = rootTable;
	return success ? 1 : 0;
}


DLL_EXPORT const char* __stdcall lpErrorLog()
{
	if (luaParser) {
		return GetStr(luaParser->GetErrorLog());
	}
	return GetStr("no LuaParser is loaded");
}


/******************************************************************************/
//
//  Environment additions
//

DLL_EXPORT void __stdcall lpAddTableInt(int key, int override)
{
	if (luaParser) { luaParser->GetTable(key, override); }
}


DLL_EXPORT void __stdcall lpAddTableStr(const char* key, int override)
{
	if (luaParser) { luaParser->GetTable(key, override); }
}


DLL_EXPORT void __stdcall lpEndTable()
{
	if (luaParser) { luaParser->EndTable(); }
}


DLL_EXPORT void __stdcall lpAddIntKeyIntVal(int key, int val)
{
	if (luaParser) { luaParser->AddInt(key, val); }
}


DLL_EXPORT void __stdcall lpAddStrKeyIntVal(const char* key, int val)
{
	if (luaParser) { luaParser->AddInt(key, val); }
}


DLL_EXPORT void __stdcall lpAddIntKeyBoolVal(int key, int val)
{
	if (luaParser) { luaParser->AddBool(key, val); }
}


DLL_EXPORT void __stdcall lpAddStrKeyBoolVal(const char* key, int val)
{
	if (luaParser) { luaParser->AddBool(key, val); }
}


DLL_EXPORT void __stdcall lpAddIntKeyFloatVal(int key, float val)
{
	if (luaParser) { luaParser->AddFloat(key, val); }
}


DLL_EXPORT void __stdcall lpAddStrKeyFloatVal(const char* key, float val)
{
	if (luaParser) { luaParser->AddFloat(key, val); }
}


DLL_EXPORT void __stdcall lpAddIntKeyStrVal(int key, const char* val)
{
	if (luaParser) { luaParser->AddString(key, val); }
}


DLL_EXPORT void __stdcall lpAddStrKeyStrVal(const char* key, const char* val)
{
	if (luaParser) { luaParser->AddString(key, val); }
}


/******************************************************************************/
//
//  Table manipulation
//

DLL_EXPORT int __stdcall lpRootTable()
{
	currTable = rootTable;
	luaTables.clear();
	return currTable.IsValid() ? 1 : 0;
}


DLL_EXPORT int __stdcall lpRootTableExpr(const char* expr)
{
	currTable = rootTable.SubTableExpr(expr);
	luaTables.clear();
	return currTable.IsValid() ? 1 : 0;
}


DLL_EXPORT int __stdcall lpSubTableInt(int key)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTable(key);
	return currTable.IsValid() ? 1 : 0;
}


DLL_EXPORT int __stdcall lpSubTableStr(const char* key)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTable(key);
	return currTable.IsValid() ? 1 : 0;
}


DLL_EXPORT int __stdcall lpSubTableExpr(const char* expr)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTableExpr(expr);
	return currTable.IsValid() ? 1 : 0;
}


DLL_EXPORT void __stdcall lpPopTable()
{
	if (luaTables.empty()) {
		currTable = rootTable;
		return;
	}
	const unsigned popSize = luaTables.size() - 1;
	currTable = luaTables[popSize];
	luaTables.resize(popSize);
}


/******************************************************************************/
//
//  Key existance
//

DLL_EXPORT int __stdcall lpGetKeyExistsInt(int key)
{
	return currTable.KeyExists(key) ? 1 : 0;
}


DLL_EXPORT int __stdcall lpGetKeyExistsStr(const char* key)
{
	return currTable.KeyExists(key) ? 1 : 0;
}


/******************************************************************************/
//
// Key lists
//

DLL_EXPORT int __stdcall lpGetIntKeyListCount()
{
	if (!currTable.IsValid()) {
		intKeys.clear();
		return 0;
	}
	currTable.GetKeys(intKeys);
	return (int)intKeys.size();
}


DLL_EXPORT int __stdcall lpGetIntKeyListEntry(int index)
{
	if ((index < 0) || (index >= intKeys.size())) {
		return 0;
	}
	return intKeys[index];
}


DLL_EXPORT int __stdcall lpGetStrKeyListCount()
{
	if (!currTable.IsValid()) {
		stringKeys.clear();
		return 0;
	}
	currTable.GetKeys(stringKeys);
	return (int)stringKeys.size();
}


DLL_EXPORT const char* __stdcall lpGetStrKeyListEntry(int index)
{
	if ((index < 0) || (index >= stringKeys.size())) {
		return GetStr("");
	}
	return GetStr(stringKeys[index]);
}


/******************************************************************************/
//
//  Value queries
//

DLL_EXPORT int __stdcall lpGetIntKeyIntVal(int key, int defVal)
{
	return currTable.GetInt(key, defVal);
}


DLL_EXPORT int __stdcall lpGetStrKeyIntVal(const char* key, int defVal)
{
	return currTable.GetInt(key, defVal);
}


DLL_EXPORT int __stdcall lpGetIntKeyBoolVal(int key, int defVal)
{
	return currTable.GetBool(key, defVal) ? 1 : 0;
}


DLL_EXPORT int __stdcall lpGetStrKeyBoolVal(const char* key, int defVal)
{
	return currTable.GetBool(key, defVal) ? 1 : 0;
}


DLL_EXPORT float __stdcall lpGetIntKeyFloatVal(int key, float defVal)
{
	return currTable.GetFloat(key, defVal);
}


DLL_EXPORT float __stdcall lpGetStrKeyFloatVal(const char* key, float defVal)
{
	return currTable.GetFloat(key, defVal);
}


DLL_EXPORT const char* __stdcall lpGetIntKeyStrVal(int key,
                                                   const char* defVal)
{
	return GetStr(currTable.GetString(key, defVal));
}


DLL_EXPORT const char* __stdcall lpGetStrKeyStrVal(const char* key,
                                                   const char* defVal)
{
	return GetStr(currTable.GetString(key, defVal));
}


/******************************************************************************/
/******************************************************************************/
