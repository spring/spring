/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "unitsync.h"

#include "Lua/LuaParser.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/ExportDefines.h"

#include <string>
#include <vector>


/******************************************************************************/

static LuaParser* luaParser = NULL;

static LuaTable rootTable;
static LuaTable currTable;
static std::vector<LuaTable> luaTables;

static std::vector<int>         intKeys;
static std::vector<std::string> strKeys;


/******************************************************************************/
//
//  Primary calls
//

EXPORT(void) lpClose()
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


EXPORT(int) lpOpenFile(const char* filename, const char* fileModes,
		const char* accessModes)
{
	lpClose();
	luaParser = new LuaParser(filename, fileModes, accessModes);
	return 1;
}


EXPORT(int) lpOpenSource(const char* source, const char* accessModes)
{
	lpClose();
	luaParser = new LuaParser(source, accessModes);
	return 1;
}


EXPORT(int) lpExecute()
{
	if (!luaParser) {
		return 0;
	}
	const bool success = luaParser->Execute();
	rootTable = luaParser->GetRoot();
	currTable = rootTable;
	return success ? 1 : 0;
}


EXPORT(const char*) lpErrorLog()
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

EXPORT(void) lpAddTableInt(int key, int override)
{
	if (luaParser) { luaParser->GetTable(key, override); }
}


EXPORT(void) lpAddTableStr(const char* key, int override)
{
	if (luaParser) { luaParser->GetTable(key, override); }
}


EXPORT(void) lpEndTable()
{
	if (luaParser) { luaParser->EndTable(); }
}


EXPORT(void) lpAddIntKeyIntVal(int key, int val)
{
	if (luaParser) { luaParser->AddInt(key, val); }
}


EXPORT(void) lpAddStrKeyIntVal(const char* key, int val)
{
	if (luaParser) { luaParser->AddInt(key, val); }
}


EXPORT(void) lpAddIntKeyBoolVal(int key, int val)
{
	if (luaParser) { luaParser->AddBool(key, val); }
}


EXPORT(void) lpAddStrKeyBoolVal(const char* key, int val)
{
	if (luaParser) { luaParser->AddBool(key, val); }
}


EXPORT(void) lpAddIntKeyFloatVal(int key, float val)
{
	if (luaParser) { luaParser->AddFloat(key, val); }
}


EXPORT(void) lpAddStrKeyFloatVal(const char* key, float val)
{
	if (luaParser) { luaParser->AddFloat(key, val); }
}


EXPORT(void) lpAddIntKeyStrVal(int key, const char* val)
{
	if (luaParser) { luaParser->AddString(key, val); }
}


EXPORT(void) lpAddStrKeyStrVal(const char* key, const char* val)
{
	if (luaParser) { luaParser->AddString(key, val); }
}


/******************************************************************************/
//
//  Table manipulation
//

EXPORT(int) lpRootTable()
{
	currTable = rootTable;
	luaTables.clear();
	return currTable.IsValid() ? 1 : 0;
}


EXPORT(int) lpRootTableExpr(const char* expr)
{
	currTable = rootTable.SubTableExpr(expr);
	luaTables.clear();
	return currTable.IsValid() ? 1 : 0;
}


EXPORT(int) lpSubTableInt(int key)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTable(key);
	return currTable.IsValid() ? 1 : 0;
}


EXPORT(int) lpSubTableStr(const char* key)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTable(key);
	return currTable.IsValid() ? 1 : 0;
}


EXPORT(int) lpSubTableExpr(const char* expr)
{
	luaTables.push_back(currTable);
	currTable = currTable.SubTableExpr(expr);
	return currTable.IsValid() ? 1 : 0;
}


EXPORT(void) lpPopTable()
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

EXPORT(int) lpGetKeyExistsInt(int key)
{
	return currTable.KeyExists(key) ? 1 : 0;
}


EXPORT(int) lpGetKeyExistsStr(const char* key)
{
	return currTable.KeyExists(key) ? 1 : 0;
}


/******************************************************************************/
//
//  Type
//

EXPORT(int) lpGetIntKeyType(int key)
{
	return currTable.GetType(key);
}


EXPORT(int) lpGetStrKeyType(const char* key)
{
	return currTable.GetType(key);
}


/******************************************************************************/
//
// Key lists
//

EXPORT(int) lpGetIntKeyListCount()
{
	if (!currTable.IsValid()) {
		intKeys.clear();
		return 0;
	}
	intKeys.clear();
	currTable.GetKeys(intKeys);
	return (int)intKeys.size();
}


EXPORT(int) lpGetIntKeyListEntry(int index)
{
	if ((index < 0) || (index >= (int)intKeys.size())) {
		return 0;
	}
	return intKeys[index];
}


EXPORT(int) lpGetStrKeyListCount()
{
	if (!currTable.IsValid()) {
		strKeys.clear();
		return 0;
	}
	strKeys.clear();
	currTable.GetKeys(strKeys);
	return (int)strKeys.size();
}


EXPORT(const char*) lpGetStrKeyListEntry(int index)
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

EXPORT(int) lpGetIntKeyIntVal(int key, int defVal)
{
	return currTable.GetInt(key, defVal);
}


EXPORT(int) lpGetStrKeyIntVal(const char* key, int defVal)
{
	return currTable.GetInt(key, defVal);
}


EXPORT(int) lpGetIntKeyBoolVal(int key, int defVal)
{
	return currTable.GetBool(key, defVal) ? 1 : 0;
}


EXPORT(int) lpGetStrKeyBoolVal(const char* key, int defVal)
{
	return currTable.GetBool(key, defVal) ? 1 : 0;
}


EXPORT(float) lpGetIntKeyFloatVal(int key, float defVal)
{
	return currTable.GetFloat(key, defVal);
}


EXPORT(float) lpGetStrKeyFloatVal(const char* key, float defVal)
{
	return currTable.GetFloat(key, defVal);
}


EXPORT(const char*) lpGetIntKeyStrVal(int key, const char* defVal)
{
	return GetStr(currTable.GetString(key, defVal));
}


EXPORT(const char*) lpGetStrKeyStrVal(const char* key, const char* defVal)
{
	return GetStr(currTable.GetString(key, defVal));
}


/******************************************************************************/
/******************************************************************************/
