#/*START
#//##########################################################################//#

file=test.cpp

start=`grep -n '^#END' $file | grep -o '^[^:]*'`
start=`expr $start + 1`
tail -n +$start $file > test.tmp.cxx
echo Clipped $start lines

g++ -I../../../rts/System test.tmp.cxx ../../../game/unitsync.so

echo ./a.out Castles.smf ba621.sd7
./a.out Castles.smf ba621.sd7

exit

#//##########################################################################//#
#END*/


/******************************************************************************/
/******************************************************************************/
//  Simple file to help test unitsync, compile with:
//
//    g++ -I../../../rts/System test.cxx ../../../game/unitsync.so
//


#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
using namespace std;

#include "../unitsync.h"


/******************************************************************************/
/******************************************************************************/
//
//  Generated using:  'grep DLL_EXPORT unitsync.cpp'
//

DLL_EXPORT const char*  __stdcall GetSpringVersion();

DLL_EXPORT void         __stdcall Message(const char* p_szMessage);

DLL_EXPORT void         __stdcall UnInit();
DLL_EXPORT int          __stdcall Init(bool isServer, int id);
DLL_EXPORT int          __stdcall InitArchiveScanner(void);

DLL_EXPORT int          __stdcall ProcessUnits(void);
DLL_EXPORT int          __stdcall ProcessUnitsNoChecksum(void);
DLL_EXPORT const char*  __stdcall GetCurrentList();

DLL_EXPORT void         __stdcall AddClient(int id, const char *unitList);
DLL_EXPORT void         __stdcall RemoveClient(int id);

DLL_EXPORT const char*  __stdcall GetClientDiff(int id);
DLL_EXPORT void         __stdcall InstallClientDiff(const char *diff);

DLL_EXPORT int          __stdcall GetUnitCount();
DLL_EXPORT const char*  __stdcall GetUnitName(int unit);
DLL_EXPORT const char*  __stdcall GetFullUnitName(int unit);
DLL_EXPORT int          __stdcall IsUnitDisabled(int unit);
DLL_EXPORT int          __stdcall IsUnitDisabledByClient(int unit, int clientId);

DLL_EXPORT void         __stdcall AddArchive(const char* name);
DLL_EXPORT void         __stdcall AddAllArchives(const char* root);
DLL_EXPORT unsigned int __stdcall GetArchiveChecksum(const char* arname);

DLL_EXPORT int          __stdcall GetMapCount();
DLL_EXPORT const char*  __stdcall GetMapName(int index);
DLL_EXPORT int          __stdcall GetMapInfoEx(const char* name, MapInfo* outInfo, int version);
DLL_EXPORT int          __stdcall GetMapInfo(const char* name, MapInfo* outInfo);
DLL_EXPORT int          __stdcall GetMapArchiveCount(const char* mapName);
DLL_EXPORT const char*  __stdcall GetMapArchiveName(int index);
DLL_EXPORT unsigned int __stdcall GetMapChecksum(int index);
DLL_EXPORT void*        __stdcall GetMinimap(const char* filename, int miplevel);

DLL_EXPORT int          __stdcall GetPrimaryModCount();
DLL_EXPORT const char*  __stdcall GetPrimaryModName(int index);
DLL_EXPORT const char*  __stdcall GetPrimaryModShortName(int index);
DLL_EXPORT const char*  __stdcall GetPrimaryModVersion(int index);
DLL_EXPORT const char*  __stdcall GetPrimaryModMutator(int index);
DLL_EXPORT const char*  __stdcall GetPrimaryModArchive(int index);
DLL_EXPORT int          __stdcall GetPrimaryModArchiveCount(int index);
DLL_EXPORT const char*  __stdcall GetPrimaryModArchiveList(int arnr);
DLL_EXPORT int          __stdcall GetPrimaryModIndex(const char* name);
DLL_EXPORT unsigned int __stdcall GetPrimaryModChecksum(int index);

DLL_EXPORT int          __stdcall GetSideCount();
DLL_EXPORT const char*  __stdcall GetSideName(int side);

DLL_EXPORT int          __stdcall GetLuaAICount();
DLL_EXPORT const char*  __stdcall GetLuaAIName(int aiIndex);
DLL_EXPORT const char*  __stdcall GetLuaAIDesc(int aiIndex);

DLL_EXPORT int          __stdcall GetMapOptionCount(const char* name);
DLL_EXPORT int          __stdcall GetModOptionCount();
DLL_EXPORT const char*  __stdcall GetOptionKey(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionName(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionDesc(int optIndex);
DLL_EXPORT int          __stdcall GetOptionType(int optIndex);
DLL_EXPORT int          __stdcall GetOptionBoolDef(int optIndex);
DLL_EXPORT float        __stdcall GetOptionNumberDef(int optIndex);
DLL_EXPORT float        __stdcall GetOptionNumberMin(int optIndex);
DLL_EXPORT float        __stdcall GetOptionNumberMax(int optIndex);
DLL_EXPORT float        __stdcall GetOptionNumberStep(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionStringDef(int optIndex);
DLL_EXPORT int          __stdcall GetOptionStringMaxLen(int optIndex);
DLL_EXPORT int          __stdcall GetOptionListCount(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionListDef(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionListItemKey(int optIndex, int itemIndex);
DLL_EXPORT const char*  __stdcall GetOptionListItemName(int optIndex, int itemIndex);
DLL_EXPORT const char*  __stdcall GetOptionListItemDesc(int optIndex, int itemIndex);

DLL_EXPORT int          __stdcall GetModValidMapCount();
DLL_EXPORT const char*  __stdcall GetModValidMap(int index);

DLL_EXPORT int          __stdcall OpenFileVFS(const char* name);
DLL_EXPORT void         __stdcall CloseFileVFS(int handle);
DLL_EXPORT void         __stdcall ReadFileVFS(int handle, void* buf, int length);
DLL_EXPORT int          __stdcall FileSizeVFS(int handle);
DLL_EXPORT int          __stdcall InitFindVFS(const char* pattern);
DLL_EXPORT int          __stdcall FindFilesVFS(int handle, char* nameBuf, int size);

DLL_EXPORT int          __stdcall OpenArchive(const char* name);
DLL_EXPORT void         __stdcall CloseArchive(int archive);
DLL_EXPORT int          __stdcall FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
DLL_EXPORT int          __stdcall OpenArchiveFile(int archive, const char* name);
DLL_EXPORT int          __stdcall ReadArchiveFile(int archive, int handle, void* buffer, int numBytes);
DLL_EXPORT void         __stdcall CloseArchiveFile(int archive, int handle);
DLL_EXPORT int          __stdcall SizeArchiveFile(int archive, int handle);


/******************************************************************************/
/******************************************************************************/

static void DisplayOptions(int optionCount);


/******************************************************************************/
/******************************************************************************/

static bool PrintMapInfo(const string& mapName)
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
  InitArchiveScanner();

  printf("GetSpringVersion() = %s\n", GetSpringVersion());

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

  if (true) { // FIXME -- debugging
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
    const string sideName= GetSideName(i);
    printf("    side %i = '%s'\n", i, sideName.c_str());
  }

  // LuaAI options
  printf("  LuaAI\n");
  const int luaAICount = GetLuaAICount();
  for (int i = 0; i < luaAICount; i++) {
    printf("    %i: name = %s\n", i, GetLuaAIName(i));
    printf("       desc = %s\n",     GetLuaAIDesc(i));
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
