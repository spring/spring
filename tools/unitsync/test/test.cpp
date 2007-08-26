#/*START
#//##########################################################################//#

file=test.cpp

start=`grep -n '^#END' $file | grep -o '[^:]*'`
start=`expr $start + 1`

tail -n +$start $file > test.tmp.cpp

g++ -I../../../rts/System test.tmp.cpp ../../../game/unitsync.so

./a.out Castles.smf BA561.sd7

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
DLL_EXPORT const char*  __stdcall GetPrimaryModArchive(int index);
DLL_EXPORT int          __stdcall GetPrimaryModArchiveCount(int index);
DLL_EXPORT const char*  __stdcall GetPrimaryModArchiveList(int arnr);
DLL_EXPORT int          __stdcall GetPrimaryModIndex(const char* name);
DLL_EXPORT unsigned int __stdcall GetPrimaryModChecksum(int index);
DLL_EXPORT int          __stdcall GetSideCount();
DLL_EXPORT const char*  __stdcall GetSideName(int side);
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

  // map names
  const int mapCount = GetMapCount();
  for (int i = 0; i < mapCount; i++) {
    printf("  map name (%i) = %s\n", i, GetMapName(i));
  }

  // mod names
  const int modCount = GetPrimaryModCount();
  for (int i = 0; i < modCount; i++) {
    printf("  mod name (%i) = %s  <%s>\n", i,
           GetPrimaryModName(i), GetPrimaryModArchive(i));
  }

  // load the mod archives
  AddAllArchives(mod.c_str());

  // unit names
  ProcessUnits();
  const int unitCount = GetUnitCount();
  for (int i = 0; i < unitCount; i++) {
    printf("Unit(%i) = %s  <%s>\n", i, GetUnitName(i), GetFullUnitName(i));
  }

  UnInit();

  return 0;
}


/******************************************************************************/
/******************************************************************************/
