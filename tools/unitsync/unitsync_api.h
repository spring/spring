#ifndef UNITSYNC_API_H
#define UNITSYNC_API_H

#include "unitsync.h"


/******************************************************************************/
/******************************************************************************/

DLL_EXPORT const char*  __stdcall GetSpringVersion();

DLL_EXPORT void         __stdcall Message(const char* p_szMessage);

DLL_EXPORT int          __stdcall Init(bool isServer, int id);
DLL_EXPORT void         __stdcall UnInit();

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
DLL_EXPORT const char*  __stdcall GetSideStartUnit(int side);

DLL_EXPORT int          __stdcall GetLuaAICount();
DLL_EXPORT const char*  __stdcall GetLuaAIName(int aiIndex);
DLL_EXPORT const char*  __stdcall GetLuaAIDesc(int aiIndex);

DLL_EXPORT int          __stdcall GetMapOptionCount(const char* name);
DLL_EXPORT int          __stdcall GetModOptionCount();
DLL_EXPORT const char*  __stdcall GetOptionKey(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionName(int optIndex);
DLL_EXPORT const char* __stdcall GetOptionSection(int optIndex);
DLL_EXPORT const char* __stdcall GetOptionStyle(int optIndex);
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
DLL_EXPORT int          __stdcall InitDirListVFS(const char* path,
                                                 const char* pattern,
                                                 const char* modes);
DLL_EXPORT int          __stdcall InitSubDirsVFS(const char* path,
                                                 const char* pattern,
                                                 const char* modes);
DLL_EXPORT int          __stdcall FindFilesVFS(int handle, char* nameBuf, int size);

DLL_EXPORT int          __stdcall OpenArchive(const char* name);
DLL_EXPORT int          __stdcall OpenArchiveType(const char* name, const char* type);
DLL_EXPORT void         __stdcall CloseArchive(int archive);
DLL_EXPORT int          __stdcall FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
DLL_EXPORT int          __stdcall OpenArchiveFile(int archive, const char* name);
DLL_EXPORT int          __stdcall ReadArchiveFile(int archive, int handle, void* buffer, int numBytes);
DLL_EXPORT void         __stdcall CloseArchiveFile(int archive, int handle);
DLL_EXPORT int          __stdcall SizeArchiveFile(int archive, int handle);

DLL_EXPORT void        __stdcall lpClose();
DLL_EXPORT int         __stdcall lpOpenFile(const char* filename,
                                            const char* fileModes,
                                            const char* accessModes);
DLL_EXPORT int         __stdcall lpOpenSource(const char* source,
                                              const char* accessModes);
DLL_EXPORT int         __stdcall lpExecute();
DLL_EXPORT const char* __stdcall lpErrorLog();

DLL_EXPORT void        __stdcall lpAddTableInt(int key, int override);
DLL_EXPORT void        __stdcall lpAddTableStr(const char* key, int override);
DLL_EXPORT void        __stdcall lpEndTable();
DLL_EXPORT void        __stdcall lpAddIntKeyIntVal(int key, int val);
DLL_EXPORT void        __stdcall lpAddStrKeyIntVal(const char* key, int val);
DLL_EXPORT void        __stdcall lpAddIntKeyBoolVal(int key, int val);
DLL_EXPORT void        __stdcall lpAddStrKeyBoolVal(const char* key, int val);
DLL_EXPORT void        __stdcall lpAddIntKeyFloatVal(int key, float val);
DLL_EXPORT void        __stdcall lpAddStrKeyFloatVal(const char* key, float val);
DLL_EXPORT void        __stdcall lpAddIntKeyStrVal(int key, const char* val);
DLL_EXPORT void        __stdcall lpAddStrKeyStrVal(const char* key, const char* val);

DLL_EXPORT int         __stdcall lpRootTable();
DLL_EXPORT int         __stdcall lpRootTableExpr(const char* expr);
DLL_EXPORT int         __stdcall lpSubTableInt(int key);
DLL_EXPORT int         __stdcall lpSubTableStr(const char* key);
DLL_EXPORT int         __stdcall lpSubTableExpr(const char* expr);
DLL_EXPORT void        __stdcall lpPopTable();

DLL_EXPORT int         __stdcall lpGetKeyExistsInt(int key);
DLL_EXPORT int         __stdcall lpGetKeyExistsStr(const char* key);

DLL_EXPORT int         __stdcall lpGetIntKeyType(int key);
DLL_EXPORT int         __stdcall lpGetStrKeyType(const char* key);

DLL_EXPORT int         __stdcall lpGetIntKeyListCount();
DLL_EXPORT int         __stdcall lpGetIntKeyListEntry(int index);
DLL_EXPORT int         __stdcall lpGetStrKeyListCount();
DLL_EXPORT const char* __stdcall lpGetStrKeyListEntry(int index);

DLL_EXPORT int         __stdcall lpGetIntKeyIntVal(int key, int defVal);
DLL_EXPORT int         __stdcall lpGetStrKeyIntVal(const char* key, int defVal);
DLL_EXPORT int         __stdcall lpGetIntKeyBoolVal(int key, int defVal);
DLL_EXPORT int         __stdcall lpGetStrKeyBoolVal(const char* key, int defVal);
DLL_EXPORT float       __stdcall lpGetIntKeyFloatVal(int key, float defVal);
DLL_EXPORT float       __stdcall lpGetStrKeyFloatVal(const char* key, float defVal);
DLL_EXPORT const char* __stdcall lpGetIntKeyStrVal(int key, const char* defVal);
DLL_EXPORT const char* __stdcall lpGetStrKeyStrVal(const char* key, const char* defVal);


#endif // UNITSYNC_API_H
