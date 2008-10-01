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

static vector<int>    intKeys;
static vector<string> strKeys;


/******************************************************************************/
//
//  Primary calls
//

Export(void) lpClose()
{
	rootTable = LuaTable();
	currTable = LuaTable();

	luaTables.clear();

	intKeys.clear();
	strKeys.clear();

	delete luaParser;
	luaParser = NULL;

	return;
}


Export(int) lpOpenFile(const char* filename,
                                    const char* fileModes,
                                    const char* accessModes)
{
	lpClose();
	luaParser = new LuaParser(filename, fileModes, accessModes);
	return 1;
}


Export(int) lpOpenSource(const char* source,
                                      const char* accessModes)
{
	lpClose();
	luaParser = new LuaParser(source, accessModes);
	return 1;
}


Export(int) lpExecute()
{
	if (!luaParser) {
		return 0;
	}
	const bool success = luaParser->Execute();
	rootTable = luaParser->GetRoot();
	currTable = rootTable;
	return success ? 1 : 0;
}


Export(const char*) lpErrorLog()
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

Export(void) lpAddTableInt(int key, int override)
{
	if (luaParser) { luaParser->GetTable(key, override); }
}


Export(void) lpAddTableStr(const char* key, int override)
{
	if (luaParser) { luaParser->GetTable(key, override); }
}


Export(void) lpEndTable()
{
	if (luaParser) { luaParser->EndTable(); }
}


Export(void) lpAddIntKeyIntVal(int key, int val)
{
	if (luaParser) { luaParser->AddInt(key, val); }
}


Export(void) lpAddStrKeyIntVal(const char* key, int val)
{
	if (luaParser) { luaParser->AddInt(key, val); }
}


Export(void) lpAddIntKeyBoolVal(int key, int val)
{
	if (luaParser) { luaParser->AddBool(key, val); }
}


Export(void) lpAddStrKeyBoolVal(const char* key, int val)
{
	if (luaParser) { luaParser->AddBool(key, val); }
}


Export(void) lpAddIntKeyFloatVal(int key, float val)
{
	if (luaParser) { luaParser->AddFloat(key, val); }
}


Export(void) lpAddStrKeyFloatVal(const char* key, float val)
{
	if (luaParser) { luaParser->AddFloat(key, val); }
}


Export(void) lpAddIntKeyStrVal(int key, const char* val)
{
	if (luaParser) { luaParser->AddString(key, val); }
}


Export(void) lpAddStrKeyStrVal(const char* key, const char* val)
{
	if (luaParser) { luaParser->AddString(key, val); }
}


/******************************************************************************/
//
//  Table manipulation
//

Export(int) lpRootTable()
{
	currTable = rootTable;
	luaTables.clear();
	return currTable.IsValid() ? 1 : 0;
}


Export(int) lpRootTableExpr(const char* expr)
{
	currTable = rootTable.SubTableExpr(expr);
	luaTables.clear();
	return currTable.IsValid() ? 1 : 0;
}


Export(int) lpSubTableInt(int key)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTable(key);
	return currTable.IsValid() ? 1 : 0;
}


Export(int) lpSubTableStr(const char* key)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTable(key);
	return currTable.IsValid() ? 1 : 0;
}


Export(int) lpSubTableExpr(const char* expr)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTableExpr(expr);
	return currTable.IsValid() ? 1 : 0;
}


Export(void) lpPopTable()
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

Export(int) lpGetKeyExistsInt(int key)
{
	return currTable.KeyExists(key) ? 1 : 0;
}


Export(int) lpGetKeyExistsStr(const char* key)
{
	return currTable.KeyExists(key) ? 1 : 0;
}


/******************************************************************************/
//
//  Type
//

Export(int) lpGetIntKeyType(int key)
{
	return currTable.GetType(key);
}


Export(int) lpGetStrKeyType(const char* key)
{
	return currTable.GetType(key);
}


/******************************************************************************/
//
// Key lists
//

Export(int) lpGetIntKeyListCount()
{
	if (!currTable.IsValid()) {
		intKeys.clear();
		return 0;
	}
	intKeys.clear();
	currTable.GetKeys(intKeys);
	return (int)intKeys.size();
}


Export(int) lpGetIntKeyListEntry(int index)
{
	if ((index < 0) || (index >= (int)intKeys.size())) {
		return 0;
	}
	return intKeys[index];
}


Export(int) lpGetStrKeyListCount()
{
	if (!currTable.IsValid()) {
		strKeys.clear();
		return 0;
	}
	strKeys.clear();
	currTable.GetKeys(strKeys);
	return (int)strKeys.size();
}


Export(const char*) lpGetStrKeyListEntry(int index)
{
	if ((index < 0) || (index >= (int)strKeys.size())) {
		return GetStr("");
	}
	return GetStr(strKeys[index]);
}


/******************************************************************************/
//
//  Value queries
//

Export(int) lpGetIntKeyIntVal(int key, int defVal)
{
	return currTable.GetInt(key, defVal);
}


Export(int) lpGetStrKeyIntVal(const char* key, int defVal)
{
	return currTable.GetInt(key, defVal);
}


Export(int) lpGetIntKeyBoolVal(int key, int defVal)
{
	return currTable.GetBool(key, defVal) ? 1 : 0;
}


Export(int) lpGetStrKeyBoolVal(const char* key, int defVal)
{
	return currTable.GetBool(key, defVal) ? 1 : 0;
}


Export(float) lpGetIntKeyFloatVal(int key, float defVal)
{
	return currTable.GetFloat(key, defVal);
}


Export(float) lpGetStrKeyFloatVal(const char* key, float defVal)
{
	return currTable.GetFloat(key, defVal);
}


Export(const char*) lpGetIntKeyStrVal(int key,
                                                   const char* defVal)
{
	return GetStr(currTable.GetString(key, defVal));
}


Export(const char*) lpGetStrKeyStrVal(const char* key,
                                                   const char* defVal)
{
	return GetStr(currTable.GetString(key, defVal));
}


/******************************************************************************/
/******************************************************************************/
