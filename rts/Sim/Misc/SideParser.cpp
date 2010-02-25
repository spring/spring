/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <string>
#include <set>
using std::string;
using std::set;

#include "mmgr.h"

#include "SideParser.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "LogOutput.h"
#include "Util.h"


SideParser sideParser;


/******************************************************************************/

SideParser::SideParser()
{
}


SideParser::~SideParser()
{
}


bool SideParser::Load()
{
	dataVec.clear();
	errorLog.clear();

	LuaParser parser("gamedata/sidedata.lua",
	                 SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
#if !defined UNITSYNC && !defined DEDICATED
	// this should not be included with unitsync:
	// 1. avoids linkage with LuaSyncedRead
	// 2. ModOptions are not valid during unitsync mod parsing
	parser.GetTable("Spring");
	parser.AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
	parser.EndTable();
#endif
	if (!parser.Execute()) {
		errorLog = parser.GetErrorLog();
		return false;
	}

	set<string> sideSet;

	const LuaTable root = parser.GetRoot();
	for (int i = 1; /* no-op */; i++) {
		const LuaTable sideTable = root.SubTable(i);
		if (!sideTable.IsValid()) {
			break;
		}

		Data data;
		data.caseName  = sideTable.GetString("name", "");
		data.sideName  = StringToLower(data.caseName);
		data.startUnit = sideTable.GetString("startUnit", "");
		data.startUnit = StringToLower(data.startUnit);

		if (data.sideName.empty()) {
			logOutput.Print("Missing side name: %i", i);
		}
		else if (data.startUnit.empty()) {
			logOutput.Print("Missing side start unit: " + data.sideName);
		}
		else {
			if (sideSet.find(data.sideName) != sideSet.end()) {
				logOutput.Print("Duplicate side name: " + data.sideName);
			}
			else {
				sideSet.insert(data.sideName);
				dataVec.push_back(data);
			}
		}
	}
	return true;
}

 
/******************************************************************************/

const SideParser::Data* SideParser::FindSide(const string& sideName) const
{
	const string name = StringToLower(sideName);
	for (unsigned int i = 0; i < dataVec.size(); i++) {
		const Data& data = dataVec[i];
		if (name == data.sideName) {
			return &data;
		}
	}
	return NULL;
}


const string& SideParser::GetSideName(unsigned int index,
                                      const string& def) const
{
	if (!ValidSide(index)) {
		return def;
	}
	return dataVec[index].sideName;
}


const string& SideParser::GetCaseName(unsigned int index,
                                      const string& def) const
{
	if (!ValidSide(index)) {
		return def;
	}
	return dataVec[index].caseName;
}


const string& SideParser::GetCaseName(const string& name,
                                      const string& def) const
{
	const Data* data = FindSide(name);
	if (data == NULL) {
		return def;
	}
	return data->caseName;
}


const string& SideParser::GetStartUnit(unsigned int index,
                                       const string& def) const
{
	if (!ValidSide(index)) {
		return def;
	}
	return dataVec[index].startUnit;
}


const string& SideParser::GetStartUnit(const string& name,
                                       const string& def) const
{
	const Data* data = FindSide(name);
	if (data == NULL) {
		return def;
	}
	return data->startUnit;
}


/******************************************************************************/
