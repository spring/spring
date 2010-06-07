#ifndef UNITSYNC_API_H
#define UNITSYNC_API_H

#include "unitsync.h"
#include "System/exportdefines.h"


// from unitsync.cpp:

EXPORT(const char* ) GetNextError();
EXPORT(const char* ) GetSpringVersion();

EXPORT(int         ) Init(bool isServer, int id);
EXPORT(void        ) UnInit();

EXPORT(const char* ) GetWritableDataDirectory();

EXPORT(int         ) ProcessUnits();
EXPORT(int         ) ProcessUnitsNoChecksum();

EXPORT(int         ) GetUnitCount();
EXPORT(const char* ) GetUnitName(int unit);
EXPORT(const char* ) GetFullUnitName(int unit);

EXPORT(void        ) AddArchive(const char* name);
EXPORT(void        ) AddAllArchives(const char* root);
EXPORT(void        ) RemoveAllArchives();
EXPORT(unsigned int) GetArchiveChecksum(const char* arname);
EXPORT(const char* ) GetArchivePath(const char* arname);

/** @deprecated */
EXPORT(int         ) GetMapInfoEx(const char* name, MapInfo* outInfo, int version);
/** @deprecated */
EXPORT(int         ) GetMapInfo(const char* name, MapInfo* outInfo);

EXPORT(int         ) GetMapCount();
EXPORT(const char* ) GetMapName(int index);
EXPORT(const char* ) GetMapDescription(int index);
EXPORT(const char* ) GetMapAuthor(int index);
EXPORT(int         ) GetMapWidth(int index);
EXPORT(int         ) GetMapHeight(int index);
EXPORT(int         ) GetMapTidalStrength(int index);
EXPORT(int         ) GetMapWindMin(int index);
EXPORT(int         ) GetMapWindMax(int index);
EXPORT(int         ) GetMapGravity(int index);
EXPORT(int         ) GetMapResourceCount(int index);
EXPORT(const char* ) GetMapResourceName(int index, int resourceIndex);
EXPORT(float       ) GetMapResourceMax(int index, int resourceIndex);
EXPORT(int         ) GetMapResourceExtractorRadius(int index, int resourceIndex);
EXPORT(int         ) GetMapPosCount(int index);
EXPORT(float       ) GetMapPosX(int index, int posIndex);
EXPORT(float       ) GetMapPosZ(int index, int posIndex);

EXPORT(float       ) GetMapMinHeight(const char* name);
EXPORT(float       ) GetMapMaxHeight(const char* name); 
EXPORT(int         ) GetMapArchiveCount(const char* mapName);
EXPORT(const char* ) GetMapArchiveName(int index);
EXPORT(unsigned int) GetMapChecksum(int index);
EXPORT(unsigned int) GetMapChecksumFromName(const char* mapName);
EXPORT(void*       ) GetMinimap(const char* filename, int miplevel);
EXPORT(int         ) GetInfoMapSize(const char* filename, const char* name, int* width, int* height);
EXPORT(int         ) GetInfoMap(const char* filename, const char* name, void* data, int typeHint);

EXPORT(int         ) GetSkirmishAICount();
EXPORT(int         ) GetSkirmishAIInfoCount(int index);
EXPORT(const char* ) GetInfoKey(int index);
EXPORT(const char* ) GetInfoValue(int index);
EXPORT(const char* ) GetInfoDescription(int index);
EXPORT(int         ) GetSkirmishAIOptionCount(int index);

EXPORT(int         ) GetPrimaryModCount();
EXPORT(const char* ) GetPrimaryModName(int index);
EXPORT(const char* ) GetPrimaryModShortName(int index);
EXPORT(const char* ) GetPrimaryModVersion(int index);
EXPORT(const char* ) GetPrimaryModMutator(int index);
EXPORT(const char* ) GetPrimaryModGame(int index);
EXPORT(const char* ) GetPrimaryModShortGame(int index);
EXPORT(const char* ) GetPrimaryModDescription(int index);
EXPORT(const char* ) GetPrimaryModArchive(int index);
EXPORT(int         ) GetPrimaryModArchiveCount(int index);
EXPORT(const char* ) GetPrimaryModArchiveList(int arnr);
EXPORT(int         ) GetPrimaryModIndex(const char* name);
EXPORT(unsigned int) GetPrimaryModChecksum(int index);
EXPORT(unsigned int) GetPrimaryModChecksumFromName(const char* name);

EXPORT(int         ) GetSideCount();
EXPORT(const char* ) GetSideName(int side);
EXPORT(const char* ) GetSideStartUnit(int side);

EXPORT(int         ) GetMapOptionCount(const char* name);
EXPORT(int         ) GetModOptionCount();
EXPORT(int         ) GetCustomOptionCount(const char* filename);
EXPORT(const char* ) GetOptionKey(int optIndex);
EXPORT(const char* ) GetOptionScope(int optIndex);
EXPORT(const char* ) GetOptionName(int optIndex);
EXPORT(const char* ) GetOptionSection(int optIndex);
EXPORT(const char* ) GetOptionStyle(int optIndex);
EXPORT(const char* ) GetOptionDesc(int optIndex);
EXPORT(int         ) GetOptionType(int optIndex);
EXPORT(int         ) GetOptionBoolDef(int optIndex);
EXPORT(float       ) GetOptionNumberDef(int optIndex);
EXPORT(float       ) GetOptionNumberMin(int optIndex);
EXPORT(float       ) GetOptionNumberMax(int optIndex);
EXPORT(float       ) GetOptionNumberStep(int optIndex);
EXPORT(const char* ) GetOptionStringDef(int optIndex);
EXPORT(int         ) GetOptionStringMaxLen(int optIndex);
EXPORT(int         ) GetOptionListCount(int optIndex);
EXPORT(const char* ) GetOptionListDef(int optIndex);
EXPORT(const char* ) GetOptionListItemKey(int optIndex, int itemIndex);
EXPORT(const char* ) GetOptionListItemName(int optIndex, int itemIndex);
EXPORT(const char* ) GetOptionListItemDesc(int optIndex, int itemIndex);

EXPORT(int         ) GetModValidMapCount();
EXPORT(const char* ) GetModValidMap(int index);

EXPORT(int         ) OpenFileVFS(const char* name);
EXPORT(void        ) CloseFileVFS(int handle);
EXPORT(int        ) ReadFileVFS(int handle, void* buf, int length);
EXPORT(int         ) FileSizeVFS(int handle);

EXPORT(int         ) InitFindVFS(const char* pattern);
EXPORT(int         ) InitDirListVFS(const char* path, const char* pattern, const char* modes);
EXPORT(int         ) InitSubDirsVFS(const char* path, const char* pattern, const char* modes);
EXPORT(int         ) FindFilesVFS(int handle, char* nameBuf, int size);

EXPORT(int         ) OpenArchive(const char* name);
EXPORT(int         ) OpenArchiveType(const char* name, const char* type);
EXPORT(void        ) CloseArchive(int archive);
EXPORT(int         ) FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
EXPORT(int         ) OpenArchiveFile(int archive, const char* name);
EXPORT(int         ) ReadArchiveFile(int archive, int handle, void* buffer, int numBytes);
EXPORT(void        ) CloseArchiveFile(int archive, int handle);
EXPORT(int         ) SizeArchiveFile(int archive, int handle);

EXPORT(void        ) SetSpringConfigFile(const char* filenameAsAbsolutePath);
EXPORT(const char* ) GetSpringConfigFile();
EXPORT(const char* ) GetSpringConfigString(const char* name, const char* defvalue);
EXPORT(int         ) GetSpringConfigInt(const char* name, const int defvalue);
EXPORT(float       ) GetSpringConfigFloat( const char* name, const float defvalue );
EXPORT(void        ) SetSpringConfigString(const char* name, const char* value);
EXPORT(void        ) SetSpringConfigInt(const char* name, const int value);
EXPORT(void        ) SetSpringConfigFloat(const char* name, const float value);


// from LuaParserAPI.cpp:

EXPORT(void       ) lpClose();
EXPORT(int        ) lpOpenFile(const char* filename, const char* fileModes, const char* accessModes);
EXPORT(int        ) lpOpenSource(const char* source, const char* accessModes);
EXPORT(int        ) lpExecute();
EXPORT(const char*) lpErrorLog();

EXPORT(void       ) lpAddTableInt(int key, int override);
EXPORT(void       ) lpAddTableStr(const char* key, int override);
EXPORT(void       ) lpEndTable();
EXPORT(void       ) lpAddIntKeyIntVal(int key, int val);
EXPORT(void       ) lpAddStrKeyIntVal(const char* key, int val);
EXPORT(void       ) lpAddIntKeyBoolVal(int key, int val);
EXPORT(void       ) lpAddStrKeyBoolVal(const char* key, int val);
EXPORT(void       ) lpAddIntKeyFloatVal(int key, float val);
EXPORT(void       ) lpAddStrKeyFloatVal(const char* key, float val);
EXPORT(void       ) lpAddIntKeyStrVal(int key, const char* val);
EXPORT(void       ) lpAddStrKeyStrVal(const char* key, const char* val);

EXPORT(int        ) lpRootTable();
EXPORT(int        ) lpRootTableExpr(const char* expr);
EXPORT(int        ) lpSubTableInt(int key);
EXPORT(int        ) lpSubTableStr(const char* key);
EXPORT(int        ) lpSubTableExpr(const char* expr);
EXPORT(void       ) lpPopTable();

EXPORT(int        ) lpGetKeyExistsInt(int key);
EXPORT(int        ) lpGetKeyExistsStr(const char* key);

EXPORT(int        ) lpGetIntKeyType(int key);
EXPORT(int        ) lpGetStrKeyType(const char* key);

EXPORT(int        ) lpGetIntKeyListCount();
EXPORT(int        ) lpGetIntKeyListEntry(int index);
EXPORT(int        ) lpGetStrKeyListCount();
EXPORT(const char*) lpGetStrKeyListEntry(int index);

EXPORT(int        ) lpGetIntKeyIntVal(int key, int defVal);
EXPORT(int        ) lpGetStrKeyIntVal(const char* key, int defVal);
EXPORT(int        ) lpGetIntKeyBoolVal(int key, int defVal);
EXPORT(int        ) lpGetStrKeyBoolVal(const char* key, int defVal);
EXPORT(float      ) lpGetIntKeyFloatVal(int key, float defVal);
EXPORT(float      ) lpGetStrKeyFloatVal(const char* key, float defVal);
EXPORT(const char*) lpGetIntKeyStrVal(int key, const char* defVal);
EXPORT(const char*) lpGetStrKeyStrVal(const char* key, const char* defVal);


#endif // UNITSYNC_API_H
