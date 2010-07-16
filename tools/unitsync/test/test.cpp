#if 0
################################################################################
file=test.cpp

map="${1:-Castles.smf}"
mod="${2:-ba621.sd7}"

g++ -g -I../../../rts -I../../../rts/System $file ../../../dist/unitsync.so && \
echo ./a.out "$map" "$mod" && \
./a.out "$map" "$mod"

exit

################################################################################
#endif



/******************************************************************************/
/******************************************************************************/
//  Simple file to help test unitsync, compile with:
//
//    g++ -I../../../rts -I../../../rts/System test.cxx ../../../dist/unitsync.so
//


#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
using namespace std;

#include "../unitsync_api.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "System/Option.h"

/******************************************************************************/
/******************************************************************************/

static void DisplayOptions(int optionCount);
static bool TestLuaParser();


/******************************************************************************/
/******************************************************************************/

static void PrintMapInfo(const string& mapName)
{
  printf("  MAP INFO  (for %s)\n", mapName.c_str());
  MapInfo mi;
  char auth[256];
  char desc[256];
  mi.author = auth;
  mi.author[0] = 0;
  mi.description = desc;
  mi.description[0] = 0;
  if (!GetMapInfoEx(mapName.c_str(), &mi, 1)) {
    printf("ERROR getting info for map %s  (%s)\n",
    mapName.c_str(), mi.description);
  }
  else {
    printf("    author:    '%s'\n", mi.author);
    printf("    desc:      '%s'\n", mi.description);
    printf("    gravity:   %i\n",   mi.gravity);
    printf("    tidal:     %i\n",   mi.tidalStrength);
    printf("    maxMetal:  %f\n",   mi.maxMetal);
    printf("    mexRad:    %i\n",   mi.extractorRadius);
    printf("    minWind:   %i\n",   mi.minWind);
    printf("    maxWind:   %i\n",   mi.maxWind);
    printf("    width:     %i\n",   mi.width);
    printf("    height:    %i\n",   mi.height);
    for (int p = 0; p < mi.posCount; p++) {
      const StartPos& sp = mi.positions[p];
      printf("    pos %i:     <%5i, %5i>\n", p, sp.x, sp.z);
    }

    const char* infomaps[] = { "height", "grass", "metal", "type", NULL };
    int width, height;
    for (int i = 0; infomaps[i]; ++i) {
      if (GetInfoMapSize(mapName.c_str(), infomaps[i], &width, &height)) {
        printf("    %smap: %i x %i\n", infomaps[i], width, height);
        /*void* data = malloc(width*height);
        GetInfoMap(mapName.c_str(), infomaps[i], data, 1);
        FILE* f = fopen(infomaps[i], "w");
        if (f == NULL) {
          perror(infomaps[i]);
        } else {
          fwrite(data, 1, width*height, f);
          fclose(f);
        }
        free(data);*/
      }
    }
  }
}


int main(int argc, char** argv)
{
  if (argc < 3) {
    printf("usage:  %s <map> <mod>\n", argv[0]);
    exit(1);
  }
  const string map = argv[1];
  const string mod = argv[2];
  printf("MAP = %s\n", map.c_str());
  printf("MOD = %s\n", mod.c_str());

  Init(false, 0);

  printf("GetSpringVersion() = %s\n", GetSpringVersion());

  // test the lua parser interface
  TestLuaParser();

  // map names
  printf("  MAPS\n");
  vector<string> mapNames;
  const int mapCount = GetMapCount();
  for (int i = 0; i < mapCount; i++) {
    const string mapName = GetMapName(i);
    mapNames.push_back(mapName);
    printf("    [map %3i]   %s\n", i, mapName.c_str());
  }

  // map archives
  printf("  MAP ARCHIVES  (for %s)\n", map.c_str());
  const int mapArcCount = GetMapArchiveCount(map.c_str());
  for (int a = 0; a < mapArcCount; a++) {
    printf("      arc %i: %s\n", a, GetMapArchiveName(a));
  }

  // map info
  PrintMapInfo(map);

  if (true && false) { // FIXME -- debugging
    for (int i = 0; i < mapNames.size(); i++) {
      PrintMapInfo(mapNames[i]);
    }
  }

  // mod names
  printf("  MODS\n");
  const int modCount = GetPrimaryModCount();
  for (int i = 0; i < modCount; i++) {
    const string modName      = GetPrimaryModName(i);
    const string modShortName = GetPrimaryModShortName(i);
    const string modVersion   = GetPrimaryModVersion(i);
    const string modMutator   = GetPrimaryModMutator(i);
    const string modArchive = GetPrimaryModArchive(i);
    printf("    [mod %3i]   %-32s  <%s> %s %s %s\n", i,
           modName.c_str(), modArchive.c_str(),
           modShortName.c_str(), modVersion.c_str(), modMutator.c_str());
  }

  // load the mod archives
  AddAllArchives(mod.c_str());

  // unit names
  while (true) {
  //const int left = ProcessUnits();
    const int left = ProcessUnitsNoChecksum();
  //printf("unitsLeft = %i\n", left);
    if (left <= 0) {
      break;
    }
  }
  printf("  UNITS\n");
  const int unitCount = GetUnitCount();
  for (int i = 0; i < unitCount; i++) {
    const string unitName     = GetUnitName(i);
    const string unitFullName = GetFullUnitName(i);
    printf("    [unit %3i]   %-16s  <%s>\n", i,
           unitName.c_str(), unitFullName.c_str());
  }
  printf("  SIDES\n");
  const int sideCount = GetSideCount();
  for (int i = 0; i < sideCount; i++) {
    const string sideName  = GetSideName(i);
    const string startUnit = GetSideStartUnit(i);
    printf("    side %i = '%s' <%s>\n",
           i, sideName.c_str(), startUnit.c_str());
  }

  // available Skirmish AIs
  printf("  SkirmishAI\n");
  const int skirmishAICount = GetSkirmishAICount();
  for (int i = 0; i < skirmishAICount; i++) {
    const int skirmishAIInfoCount = GetSkirmishAIInfoCount(i);
    printf("    %i:\n", i);
    for (int j = 0; j < skirmishAIInfoCount; j++) {
      const string key = GetInfoKey(j);
      if ((key == SKIRMISH_AI_PROPERTY_SHORT_NAME) || (key == SKIRMISH_AI_PROPERTY_VERSION)) {
        const string value = GetInfoValue(j);
      	printf("        %s = %s\n", key.c_str(), value.c_str());
      }
    }
  }

  // MapOptions
  printf("  MapOptions\n");
  const int mapOptCount = GetMapOptionCount(map.c_str());
  DisplayOptions(mapOptCount);

  // ModOptions
  printf("  ModOptions\n");
  const int modOptCount = GetModOptionCount();
  DisplayOptions(modOptCount);

  // ModValidMaps
  printf("  ModValidMaps\n");
  const int modValidMapCount = GetModValidMapCount();
  if (modValidMapCount == 0) {
    printf("    * ALL MAPS *\n");
  }
  else {
    for (int i = 0; i < modValidMapCount; i++) {
      printf("    %i: %s\n", i, GetModValidMap(i));
    }
  }

  InitDirListVFS("", NULL, NULL);
  char buf[512];
  for (int i = 0; (i = FindFilesVFS(i, buf, sizeof(buf))); /* noop */) {
    printf("FOUND FILE:  %s\n", buf);
  }

  InitSubDirsVFS("", NULL, NULL);
  for (int i = 0; (i = FindFilesVFS(i, buf, sizeof(buf))); /* noop */) {
    printf("FOUND DIR:  %s\n", buf);
  }

  UnInit();

  return 0;
}


/******************************************************************************/
/******************************************************************************/

static void DisplayOptions(int optionCount)
{
  for (int i = 0; i < optionCount; i++) {
    printf("    Option #%i\n", i);
    printf("      key  = '%s'\n", GetOptionKey(i));
    printf("      name = '%s'\n", GetOptionName(i));
    printf("      desc = '%s'\n", GetOptionDesc(i));
    printf("      type = %i\n", GetOptionType(i));

    const int type = GetOptionType(i);

    if (type == opt_error) {
      printf("      BAD OPTION\n");
    }
    else if (type == opt_bool) {
      printf("      BOOL: def = %s\n",
             GetOptionBoolDef(i) ? "true" : "false");
    }
    else if (type == opt_string) {
      printf("      STRING: def = '%s', maxlen = %i\n",
             GetOptionStringDef(i),
             GetOptionStringMaxLen(i));
    }
    else if (type == opt_number) {
      printf("      NUMBER: def = %f, min = %f, max = %f, step = %f\n",
             GetOptionNumberDef(i),
             GetOptionNumberMin(i),
             GetOptionNumberMax(i),
             GetOptionNumberStep(i));
    }
    else if (type == opt_list) {
      printf("      LIST: def = '%s'\n",
             GetOptionListDef(i));

      const int listCount = GetOptionListCount(i);
      for (int li = 0; li < listCount; li++) {
        printf("      %3i: key  = '%s'\n", li,
                                         GetOptionListItemKey(i, li));
        printf("           name = '%s'\n", GetOptionListItemName(i, li));
        printf("           desc = '%s'\n", GetOptionListItemDesc(i, li));
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
    "}\n";

  if (!lpOpenSource(source.c_str(), "r")) {
    printf("LuaParser API test failed  --  should not happen here\n");
    return false;
  }

  lpAddTableStr("Test", 1);
    lpAddIntKeyFloatVal(1, 111.1f);
    lpAddIntKeyFloatVal(2, 222.2f);
    lpAddIntKeyFloatVal(3, 333.3f);
    lpAddTableStr("Sub", 1);
      lpAddStrKeyStrVal("test", "value1");
    lpEndTable();
    lpAddTableInt(4, 1);
      lpAddStrKeyStrVal("test", "value2");
    lpEndTable();
    lpAddTableInt(5, 1);
      lpAddStrKeyStrVal("test", "value3");
    lpEndTable();
    lpAddIntKeyStrVal(6, "hello");
    lpAddIntKeyStrVal(7, "world");
  lpEndTable();

  if (!lpExecute()) {
    printf("LuaParser API test failed: %s\n", lpErrorLog());
    return false;
  }

  const int intCount = lpGetIntKeyListCount();
  for (int i = 0; i < intCount; i++) {
    const char* str = lpGetIntKeyStrVal(i, "");
    const float num = lpGetIntKeyFloatVal(i, 666.666f);
    printf("  int-key = %i, val = '%s'  (%f)\n", i, str, num);
  }

  const int strCount = lpGetStrKeyListCount();
  for (int i = 0; i < strCount; i++) {
    const string key = lpGetStrKeyListEntry(i);
    const char* str  = lpGetStrKeyStrVal(key.c_str(), "");
    const float num  = lpGetStrKeyFloatVal(key.c_str(), 666.666f);
    printf("  str-key = '%s', val = '%s'  (%f)\n", key.c_str(), str, num);
  }

  lpRootTable();
    lpSubTableInt(12);
      printf("SubTable test1: '%s'\n", lpGetIntKeyStrVal(2, "FAILURE"));
      lpSubTableStr("three");
        printf("SubTable test2: '%s'\n", lpGetIntKeyStrVal(1, "FAILURE"));
      lpPopTable();
      lpSubTableStr("four");
        printf("SubTable test3: '%s'\n", lpGetIntKeyStrVal(1, "FAILURE"));
      lpPopTable();
    lpPopTable();
    lpPopTable();
    lpPopTable();
    lpPopTable();
    lpPopTable();
    lpPopTable();
    lpSubTableInt(12);
      printf("SubTable test1: '%s'\n", lpGetIntKeyStrVal(2, "FAILURE"));
    lpPopTable();
  lpRootTable();
  lpSubTableExpr("[12].four");
      printf("SubTable sub-expr: '%s'\n", lpGetIntKeyStrVal(1, "FAILURE"));
  lpRootTableExpr("[12].four");
      printf("SubTable root-expr: '%s'\n", lpGetIntKeyStrVal(1, "FAILURE"));
  lpRootTableExpr("[12].four");
      printf("SubTable root-expr: '%s'\n", lpGetIntKeyStrVal(1, "FAILURE"));

  return true;
}


/******************************************************************************/
/******************************************************************************/

