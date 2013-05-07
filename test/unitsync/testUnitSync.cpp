/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//TODO rewrite most of this file atm it's more a verbose debug tool than a UnitTest

#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "System/Option.h"
#include "System/Log/ILog.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

namespace us {
	#include "../tools/unitsync/unitsync_api.h"
};

#define BOOST_TEST_MODULE UnitSync
#include <boost/test/unit_test.hpp>


using std::string;


/******************************************************************************/
/******************************************************************************/

static void PrintMapInfo(const string& mapName)
{
	const int map_count = us::GetMapCount();
	int mapidx = -1;
	for (int i = 0; i < map_count; i++){
		if (us::GetMapName(i) == mapName) {
			mapidx = i;
		}
	}
	if (mapidx<0) {
		LOG("Error: Map not found");
		return;
	}
	LOG("    author:    '%s'", us::GetMapAuthor(mapidx));
	LOG("    desc:      '%s'", us::GetMapDescription(mapidx));
	LOG("    gravity:   %i",   us::GetMapGravity(mapidx));
	LOG("    tidal:     %i",   us::GetMapTidalStrength(mapidx));
	const int rescount = us::GetMapResourceCount(mapidx);
	for (int i = 0; i < rescount; i++) {
		const char* resName = us::GetMapResourceName(mapidx, i);
		LOG("    max%s:  %f", resName, us::GetMapResourceMax(mapidx, i));
		LOG("    mex%sRad:    %i", resName, us::GetMapResourceExtractorRadius(mapidx, i));
	}
	LOG("    minWind:   %i",   us::GetMapWindMin(mapidx));
	LOG("    maxWind:   %i",   us::GetMapWindMax(mapidx));
	LOG("    width:     %i",   us::GetMapWidth(mapidx));
	LOG("    height:    %i",   us::GetMapHeight(mapidx));
	const int poscount = us::GetMapPosCount(mapidx);
	for (int p = 0; p < poscount; p++) {
		const int x = us::GetMapPosX(mapidx, p);
		const int z = us::GetMapPosZ(mapidx, p);
		LOG("    pos %i:     <%5i, %5i>", p, x, z);
	}

	const char* infomaps[] = { "height", "grass", "metal", "type", NULL };
	int width, height;
	for (int i = 0; infomaps[i]; ++i) {
		if (us::GetInfoMapSize(mapName.c_str(), infomaps[i], &width, &height)) {
			LOG("    %smap: %i x %i", infomaps[i], width, height);
			/*void* data = malloc(width*height);
			 *	us::GetInfoMap(mapName.c_str(), infomaps[i], data, 1);
			 *	FILE* f = fopen(infomaps[i], "w");
			 *	if (f == NULL) {
			 *	perror(infomaps[i]);
			 } else {
				 fwrite(data, 1, width*height, f);
				 fclose(f);
			 }
			 free(data);*/
		}
	}
}


/******************************************************************************/
/******************************************************************************/

static void DisplayOptions(int optionCount)
{
	for (int i = 0; i < optionCount; i++) {
		LOG("    Option #%i", i);
		LOG("      key  = '%s'", us::GetOptionKey(i));
		LOG("      name = '%s'", us::GetOptionName(i));
		LOG("      desc = '%s'", us::GetOptionDesc(i));
		LOG("      type = %i", us::GetOptionType(i));

		const int type = us::GetOptionType(i);

		if (type == opt_error) {
			LOG("      BAD OPTION");
		}
		else if (type == opt_bool) {
			LOG("      BOOL: def = %s",
			us::GetOptionBoolDef(i) ? "true" : "false");
		}
		else if (type == opt_string) {
			LOG("      STRING: def = '%s', maxlen = %i",
			us::GetOptionStringDef(i),
			us::GetOptionStringMaxLen(i));
		}
		else if (type == opt_number) {
			LOG("      NUMBER: def = %f, min = %f, max = %f, step = %f",
			us::GetOptionNumberDef(i),
			us::GetOptionNumberMin(i),
			us::GetOptionNumberMax(i),
			us::GetOptionNumberStep(i));
		}
		else if (type == opt_list) {
			LOG("      LIST: def = '%s'",
			us::GetOptionListDef(i));

			const int listCount = us::GetOptionListCount(i);
			for (int li = 0; li < listCount; li++) {
				LOG("      %3i: key  = '%s'", li,
				us::GetOptionListItemKey(i, li));
				LOG("           name = '%s'", us::GetOptionListItemName(i, li));
				LOG("           desc = '%s'", us::GetOptionListItemDesc(i, li));
			}
		}
	}
}


/******************************************************************************/
/******************************************************************************/

static bool TestLuaParser()
{
	const string source =
	"for k, v in pairs(Test) do\n"
	"  print('LUA Test Table:  '..tostring(k)..' = '..tostring(v))\n"
	"end\n"
	"return {\n"
	"  [0] = 'ZERO',\n"
	"  [1] = 'ONE',\n"
	"  [2] = 'TWO',\n"
	"  [3] = 'THREE',\n"
	"  [4] = 4.4,\n"
	"  [5] = 5.5,\n"
	"  [6] = 6.6,\n"
	"  [11] = { 'one', 'two', 'three' },\n"
	"  [12] = { 'one', 'success1', three = { 'success2', 'crap' }, four = { 'success3' }},\n"
	"  string = 'string',\n"
	"  number = 12345678,\n"
	"}";

	if (!us::lpOpenSource(source.c_str(), "r")) {
		LOG("LuaParser API test failed  --  should not happen here");
		return false;
	}

	us::lpAddTableStr("Test", 1);
	us::lpAddIntKeyFloatVal(1, 111.1f);
	us::lpAddIntKeyFloatVal(2, 222.2f);
	us::lpAddIntKeyFloatVal(3, 333.3f);
	us::lpAddTableStr("Sub", 1);
	us::lpAddStrKeyStrVal("test", "value1");
	us::lpEndTable();
	us::lpAddTableInt(4, 1);
	us::lpAddStrKeyStrVal("test", "value2");
	us::lpEndTable();
	us::lpAddTableInt(5, 1);
	us::lpAddStrKeyStrVal("test", "value3");
	us::lpEndTable();
	us::lpAddIntKeyStrVal(6, "hello");
	us::lpAddIntKeyStrVal(7, "world");
	us::lpEndTable();

	if (!us::lpExecute()) {
		LOG("LuaParser API test failed: %s", us::lpErrorLog());
		return false;
	}

	const int intCount = us::lpGetIntKeyListCount();
	for (int i = 0; i < intCount; i++) {
		const char* str = us::lpGetIntKeyStrVal(i, "");
		const float num = us::lpGetIntKeyFloatVal(i, 666.666f);
		LOG("  int-key = %i, val = '%s'  (%f)", i, str, num);
	}

	const int strCount = us::lpGetStrKeyListCount();
	for (int i = 0; i < strCount; i++) {
		const string key = us::lpGetStrKeyListEntry(i);
		const char* str  = us::lpGetStrKeyStrVal(key.c_str(), "");
		const float num  = us::lpGetStrKeyFloatVal(key.c_str(), 666.666f);
		LOG("  str-key = '%s', val = '%s'  (%f)", key.c_str(), str, num);
	}

	us::lpRootTable();
	us::lpSubTableInt(12);
	LOG("SubTable test1: '%s'", us::lpGetIntKeyStrVal(2, "FAILURE"));
	us::lpSubTableStr("three");
	LOG("SubTable test2: '%s'", us::lpGetIntKeyStrVal(1, "FAILURE"));
	us::lpPopTable();
	us::lpSubTableStr("four");
	LOG("SubTable test3: '%s'", us::lpGetIntKeyStrVal(1, "FAILURE"));
	us::lpPopTable();
	us::lpPopTable();
	us::lpPopTable();
	us::lpPopTable();
	us::lpPopTable();
	us::lpPopTable();
	us::lpPopTable();
	us::lpSubTableInt(12);
	LOG("SubTable test1: '%s'", us::lpGetIntKeyStrVal(2, "FAILURE"));
	us::lpPopTable();
	us::lpRootTable();
	us::lpSubTableExpr("[12].four");
	LOG("SubTable sub-expr: '%s'", us::lpGetIntKeyStrVal(1, "FAILURE"));
	us::lpRootTableExpr("[12].four");
	LOG("SubTable root-expr: '%s'", us::lpGetIntKeyStrVal(1, "FAILURE"));
	us::lpRootTableExpr("[12].four");
	LOG("SubTable root-expr: '%s'", us::lpGetIntKeyStrVal(1, "FAILURE"));

	return true;
}


/******************************************************************************/
/******************************************************************************/


BOOST_AUTO_TEST_CASE( UnitSync )
{
	us::Init(false, 0);

	int i = us::GetMapCount() - 1, j = us::GetPrimaryModCount() - 1;
	while (i > 0 && !us::GetMapName(i)) --i;
	while (j > 0 && !us::GetPrimaryModName(j)) --j;

	const string map = us::GetMapName(i);
	const string mod = us::GetPrimaryModName(j);
	LOG("MAP = %s", map.c_str());
	LOG("MOD = %s", mod.c_str());

	LOG("GetSpringVersion() = %s", us::GetSpringVersion());

	// test the lua parser interface
	BOOST_CHECK(TestLuaParser());

	// map names
	LOG("  MAPS");
	std::vector<string> mapNames;
	const int mapCount = us::GetMapCount();
	for (int i = 0; i < mapCount; i++) {
		const string mapName = us::GetMapName(i);
		mapNames.push_back(mapName);
		LOG("    [map %3i]   %s", i, mapName.c_str());
	}

	// map archives
	LOG("  MAP ARCHIVES  (for %s)", map.c_str());
	const int mapArcCount = us::GetMapArchiveCount(map.c_str());
	for (int a = 0; a < mapArcCount; a++) {
		LOG("      arc %i: %s", a, us::GetMapArchiveName(a));
	}

	// map info
	PrintMapInfo(map);

	if (true && false) { // FIXME -- debugging
		for (int i = 0; i < mapNames.size(); i++) {
			PrintMapInfo(mapNames[i]);
		}
	}

	// mod names
	LOG("  GAMES");
	const int modCount = us::GetPrimaryModCount();
	for (int i = 0; i < modCount; i++) {
		const string modArchive = us::GetPrimaryModArchive(i);
		const int infoCount = us::GetPrimaryModInfoCount(i);
		for (int j=0; j < infoCount; j++) {
			const char* key = us::GetInfoKey(j);
			string skey="";
			string svalue="";
			if (key!=NULL)
				skey=key;
			const char* value = us::GetInfoValueString(j);
			if (value!=NULL)
				svalue=value;
			LOG("    [%s]: %s = %s", modArchive.c_str(), skey.c_str(), svalue.c_str());
		}
	}

	// load the mod archives
	us::AddAllArchives(mod.c_str());

	// unit names
	while (true) {
		const int left = us::ProcessUnits();
		//LOG("unitsLeft = %i", left);
		if (left <= 0) {
			break;
		}
	}
	LOG("  UNITS");
	const int unitCount = us::GetUnitCount();
	for (int i = 0; i < unitCount; i++) {
		const string unitName     = us::GetUnitName(i);
		const string unitFullName = us::GetFullUnitName(i);
		LOG("    [unit %3i]   %-16s  <%s>", i,
		unitName.c_str(), unitFullName.c_str());
	}
	LOG("  SIDES");
	const int sideCount = us::GetSideCount();
	for (int i = 0; i < sideCount; i++) {
		const string sideName  = us::GetSideName(i);
		const string startUnit = us::GetSideStartUnit(i);
		LOG("    side %i = '%s' <%s>",
		i, sideName.c_str(), startUnit.c_str());
	}

	// available Skirmish AIs
	LOG("  SkirmishAI");
	const int skirmishAICount = us::GetSkirmishAICount();
	for (int i = 0; i < skirmishAICount; i++) {
		const int skirmishAIInfoCount = us::GetSkirmishAIInfoCount(i);
		LOG("    %i:", i);
		for (int j = 0; j < skirmishAIInfoCount; j++) {
			const string key = us::GetInfoKey(j);
			if ((key == SKIRMISH_AI_PROPERTY_SHORT_NAME) || (key == SKIRMISH_AI_PROPERTY_VERSION)) {
				const string value = us::GetInfoValueString(j);
				LOG("        %s = %s", key.c_str(), value.c_str());
			}
		}
	}

	// MapOptions
	LOG("  MapOptions");
	const int mapOptCount = us::GetMapOptionCount(map.c_str());
	DisplayOptions(mapOptCount);

	// ModOptions
	LOG("  ModOptions");
	const int modOptCount = us::GetModOptionCount();
	DisplayOptions(modOptCount);

	// ModValidMaps
	LOG("  ModValidMaps");
	const int modValidMapCount = us::GetModValidMapCount();
	if (modValidMapCount == 0) {
		LOG("    * ALL MAPS *");
	}
	else {
		for (int i = 0; i < modValidMapCount; i++) {
			LOG("    %i: %s", i, us::GetModValidMap(i));
		}
	}

	us::InitDirListVFS(NULL, NULL, NULL);
	char buf[512];
	for (int i = 0; (i = us::FindFilesVFS(i, buf, sizeof(buf))); /* noop */) {
		LOG("FOUND FILE:  %s", buf);
	}

	us::InitSubDirsVFS(NULL, NULL, NULL);
	for (int i = 0; (i = us::FindFilesVFS(i, buf, sizeof(buf))); /* noop */) {
		LOG("FOUND DIR:  %s", buf);
	}

	us::UnInit();
}
