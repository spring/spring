/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//TODO rewrite most of this file atm it's more a verbose debug tool than a UnitTest

#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "System/Option.h"
#include "System/Log/ILog.h"
#include "System/VersionGenerated.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
//#include <future>
//#include <chrono>


namespace us {
	#include "../tools/unitsync/unitsync_api.h"
};

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"



/******************************************************************************/
/******************************************************************************/

static void PrintMapInfo(const std::string& mapName)
{
	/*const int map_count = us::GetMapCount();
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
		}
	}*/
}


/******************************************************************************/
/******************************************************************************/
/*
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
*/

/******************************************************************************/
/******************************************************************************/

static bool TestLuaParser()
{
	const std::string source =
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
		const std::string key = us::lpGetStrKeyListEntry(i);
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

#define CHECK_ERROR_MESSAGE(s) if ((s = us::GetNextError()) != NULL) { FAIL_CHECK(s); }

static std::string GetGameName(int gameidx)
{
	const int infocount = us::GetPrimaryModInfoCount(gameidx);
	const char* errmsg = us::GetNextError();
	CHECK_ERROR_MESSAGE(errmsg);
	for (int i=0; i<infocount; i++) {
		const std::string key = us::GetInfoKey(i);
		if (key == "name") {
			return us::GetInfoValueString(i);
		}
	}
	return "";
}

TEST_CASE("UnitSync")
{

	const char* errmsg;
	// PreInit tests
	us::SetSpringConfigFile("/tmp/foo.cfg");
	CHECK(us::GetWritableDataDirectory() == nullptr);
	CHECK_ERROR_MESSAGE(errmsg); // there's an error cause we called GetWritableDataDirectory() before Init()!

	// Check if unitsync function IsSpringReleaseVersion matches VersionGenerated.h
	CHECK(us::IsSpringReleaseVersion() == (SPRING_VERSION_ENGINE_RELEASE == 1));

	// Init Unitsync
	/*LOG("ArchiveScanner: ");
	auto fut = std::async(std::launch::async, []() -> int { return us::Init(false, 0); } );
	while (fut.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
		LOG("still scanning");
	}
	INFO("us::Init(false, 0) != 0");
	CHECK(fut.get() != 0);*/
	us::SetSpringConfigFile(""); //reset to default config file
	CHECK(us::Init(false, 0) != 0);
	CHECK_ERROR_MESSAGE(errmsg);

	// Lua
	CHECK(TestLuaParser());

	// Select random Game & Map
	const int mapcount = us::GetMapCount() - 1;
	const int gamecount = us::GetPrimaryModCount() - 1;
	const int mapidx = 1;
	const int gameidx = 1;
//	CHECK(gamecount>0);
//	CHECK(mapcount>0);

	for (int i=0; i<gamecount; i++) {
		const std::string name = GetGameName(i);
		INFO(i);
		CHECK(!name.empty());
		const int primaryModIndex = us::GetPrimaryModIndex(name.c_str());
		const size_t hash = us::GetPrimaryModChecksum(i);
		CHECK(hash != 0);
		INFO(name << ": " << primaryModIndex << "!=" << i);
		CHECK(primaryModIndex == i);
		INFO(name);
		CHECK(hash == us::GetPrimaryModChecksumFromName(name.c_str()));
		CHECK_ERROR_MESSAGE(errmsg);
	}
	for (int i=0; i<mapcount; i++) {
		const std::string name = us::GetMapName(i);
		INFO(i);
		CHECK(!name.empty());
		const size_t hash = us::GetMapChecksum(i);
		CHECK(hash != 0);
		INFO(name);
		CHECK(hash == us::GetMapChecksumFromName(name.c_str()));
		CHECK_ERROR_MESSAGE(errmsg);
	}
//	while (i > 0 && !us::GetMapName(i)) --i;
//	while (j > 0 && !us::GetPrimaryModName(j)) --j;

	// Test them
	if (gamecount > 0 && mapcount > 0) {
		const std::string map = us::GetMapName(mapidx);
		const std::string game = GetGameName(gameidx);
		CHECK_ERROR_MESSAGE(errmsg);
		CHECK(!map.empty());
		CHECK(!game.empty());

	//	CHECK(us::GetMapArchiveCount(map.c_str()) >= 1); // expects map name
	//	CHECK(us::GetPrimaryModArchiveCount(j) >= 1); // expects game index!

		// map info
		PrintMapInfo(map);

		// load the mod archives
		us::AddAllArchives(game.c_str());
		CHECK_ERROR_MESSAGE(errmsg);

		// unit names
		CHECK(us::ProcessUnits() == 0);
		CHECK_ERROR_MESSAGE(errmsg);
		CHECK(us::GetUnitCount() >= 1);
		CHECK(us::GetSideCount() >= 1);
	}

	// VFS
	CHECK(us::InitDirListVFS(NULL, NULL, NULL) == 0);
	char buf[512];
	for (int i = 0; (i = us::FindFilesVFS(i, buf, sizeof(buf))); ) {
		LOG("FOUND FILE:  %s", buf);
	}
	CHECK(us::InitSubDirsVFS(NULL, NULL, NULL) == 0);
	for (int i = 0; (i = us::FindFilesVFS(i, buf, sizeof(buf))); ) {
		LOG("FOUND DIR:  %s", buf);
	}

	// Check Config Interface
	CHECK(us::GetSpringConfigFile() != NULL);
	us::SetSpringConfigInt("_unitsync_test", 1);
	CHECK(us::GetSpringConfigInt("_unitsync_test", 1e9) == 1);
	us::DeleteSpringConfigKey("_unitsync_test");
	CHECK(us::GetSpringConfigInt("_unitsync_test", 1e9) == 1e9);

	// DeInit Unitsync
	CHECK_ERROR_MESSAGE(errmsg);
	us::UnInit();
	CHECK_ERROR_MESSAGE(errmsg);

	// Check if VFS is deinit'ed
	CHECK(us::GetWritableDataDirectory() == nullptr);
	CHECK_ERROR_MESSAGE(errmsg);
}
