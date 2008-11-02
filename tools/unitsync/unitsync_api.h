#ifndef UNITSYNC_API_H
#define UNITSYNC_API_H

#include "unitsync.h"


// from unitsync.cpp:

Export(const char*) GetNextError();
Export(const char*) GetSpringVersion();

Export(void) Message(const char* p_szMessage);

Export(int) Init(bool isServer, int id);
Export(void) UnInit();

Export(const char*) GetWritableDataDirectory();

Export(int) ProcessUnits(void);
Export(int) ProcessUnitsNoChecksum(void);
Export(const char*) GetCurrentList();

//Export(void) AddClient(int id, const char *unitList);
//Export(void) RemoveClient(int id);

//Export(const char*) GetClientDiff(int id);
//Export(void) InstallClientDiff(const char *diff);

Export(int) GetUnitCount();
Export(const char*) GetUnitName(int unit);
Export(const char*) GetFullUnitName(int unit);
//Export(int) IsUnitDisabled(int unit);
//Export(int) IsUnitDisabledByClient(int unit, int clientId);

Export(void) AddArchive(const char* name);
Export(void) AddAllArchives(const char* root);
Export(unsigned int) GetArchiveChecksum(const char* arname);
Export(const char*) GetArchivePath(const char* arname);

Export(int) GetMapCount();
Export(const char*) GetMapName(int index);
Export(int) GetMapInfoEx(const char* name, MapInfo* outInfo, int version);
Export(int) GetMapInfo(const char* name, MapInfo* outInfo);
Export(int) GetMapArchiveCount(const char* mapName);
Export(const char*) GetMapArchiveName(int index);
Export(unsigned int) GetMapChecksum(int index);
Export(unsigned int) GetMapChecksumFromName(const char* mapName);
Export(void*) GetMinimap(const char* filename, int miplevel);
Export(int) GetInfoMapSize(const char* filename, const char* name, int* width, int* height);
Export(int) GetInfoMap(const char* filename, const char* name, void* data, int typeHint);

Export(int) GetSkirmishAICount();
Export(struct SSAISpecifier) GetSkirmishAISpecifier(int index);
Export(int) GetSkirmishAIInfoCount(int index);
Export(const char*) GetInfoKey(int index);
Export(const char*) GetInfoValue(int index);
Export(const char*) GetInfoDescription(int index);

Export(int) GetPrimaryModCount();
Export(const char*) GetPrimaryModName(int index);
Export(const char*) GetPrimaryModShortName(int index);
Export(const char*) GetPrimaryModVersion(int index);
Export(const char*) GetPrimaryModMutator(int index);
Export(const char*) GetPrimaryModGame(int index);
Export(const char*) GetPrimaryModShortGame(int index);
Export(const char*) GetPrimaryModDescription(int index);
Export(const char*) GetPrimaryModArchive(int index);
Export(int) GetPrimaryModArchiveCount(int index);
Export(const char*) GetPrimaryModArchiveList(int arnr);
Export(int) GetPrimaryModIndex(const char* name);
Export(unsigned int) GetPrimaryModChecksum(int index);
Export(unsigned int) GetPrimaryModChecksumFromName(const char* name);

Export(int) GetSideCount();
Export(const char*) GetSideName(int side);
Export(const char*) GetSideStartUnit(int side);

Export(int) GetLuaAICount();
Export(const char*) GetLuaAIName(int aiIndex);
Export(const char*) GetLuaAIDesc(int aiIndex);

Export(int) GetMapOptionCount(const char* name);
Export(int) GetModOptionCount();
Export(const char*) GetOptionKey(int optIndex);
Export(const char*) GetOptionName(int optIndex);
Export(const char*) GetOptionSection(int optIndex);
Export(const char*) GetOptionStyle(int optIndex);
Export(const char*) GetOptionDesc(int optIndex);
Export(int) GetOptionType(int optIndex);
Export(int) GetOptionBoolDef(int optIndex);
Export(float) GetOptionNumberDef(int optIndex);
Export(float) GetOptionNumberMin(int optIndex);
Export(float) GetOptionNumberMax(int optIndex);
Export(float) GetOptionNumberStep(int optIndex);
Export(const char*) GetOptionStringDef(int optIndex);
Export(int) GetOptionStringMaxLen(int optIndex);
Export(int) GetOptionListCount(int optIndex);
Export(const char*) GetOptionListDef(int optIndex);
Export(const char*) GetOptionListItemKey(int optIndex, int itemIndex);
Export(const char*) GetOptionListItemName(int optIndex, int itemIndex);
Export(const char*) GetOptionListItemDesc(int optIndex, int itemIndex);

Export(int) GetModValidMapCount();
Export(const char*) GetModValidMap(int index);

Export(int) OpenFileVFS(const char* name);
Export(void) CloseFileVFS(int handle);
Export(void) ReadFileVFS(int handle, void* buf, int length);
Export(int) FileSizeVFS(int handle);

Export(int) InitFindVFS(const char* pattern);
Export(int) InitDirListVFS(const char* path, const char* pattern, const char* modes);
Export(int) InitSubDirsVFS(const char* path, const char* pattern, const char* modes);
Export(int) FindFilesVFS(int handle, char* nameBuf, int size);

Export(int) OpenArchive(const char* name);
Export(int) OpenArchiveType(const char* name, const char* type);
Export(void) CloseArchive(int archive);
Export(int) FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
Export(int) OpenArchiveFile(int archive, const char* name);
Export(int) ReadArchiveFile(int archive, int handle, void* buffer, int numBytes);
Export(void) CloseArchiveFile(int archive, int handle);
Export(int) SizeArchiveFile(int archive, int handle);

Export(const char*) GetSpringConfigString(const char* name, const char* defvalue);
Export(int) GetSpringConfigInt(const char* name, const int defvalue);
Export(float) GetSpringConfigFloat( const char* name, const float defvalue );
Export(void) SetSpringConfigString(const char* name, const char* value);
Export(void) SetSpringConfigInt(const char* name, const int value);
Export(void) SetSpringConfigFloat(const char* name, const float value);


// from LuaParserAPI.cpp:

Export(void) lpClose();
Export(int) lpOpenFile(const char* filename, const char* fileModes, const char* accessModes);
Export(int) lpOpenSource(const char* source, const char* accessModes);
Export(int) lpExecute();
Export(const char*) lpErrorLog();

Export(void) lpAddTableInt(int key, int override);
Export(void) lpAddTableStr(const char* key, int override);
Export(void) lpEndTable();
Export(void) lpAddIntKeyIntVal(int key, int val);
Export(void) lpAddStrKeyIntVal(const char* key, int val);
Export(void) lpAddIntKeyBoolVal(int key, int val);
Export(void) lpAddStrKeyBoolVal(const char* key, int val);
Export(void) lpAddIntKeyFloatVal(int key, float val);
Export(void) lpAddStrKeyFloatVal(const char* key, float val);
Export(void) lpAddIntKeyStrVal(int key, const char* val);
Export(void) lpAddStrKeyStrVal(const char* key, const char* val);

Export(int) lpRootTable();
Export(int) lpRootTableExpr(const char* expr);
Export(int) lpSubTableInt(int key);
Export(int) lpSubTableStr(const char* key);
Export(int) lpSubTableExpr(const char* expr);
Export(void) lpPopTable();

Export(int) lpGetKeyExistsInt(int key);
Export(int) lpGetKeyExistsStr(const char* key);

Export(int) lpGetIntKeyType(int key);
Export(int) lpGetStrKeyType(const char* key);

Export(int) lpGetIntKeyListCount();
Export(int) lpGetIntKeyListEntry(int index);
Export(int) lpGetStrKeyListCount();
Export(const char*) lpGetStrKeyListEntry(int index);

Export(int) lpGetIntKeyIntVal(int key, int defVal);
Export(int) lpGetStrKeyIntVal(const char* key, int defVal);
Export(int) lpGetIntKeyBoolVal(int key, int defVal);
Export(int) lpGetStrKeyBoolVal(const char* key, int defVal);
Export(float) lpGetIntKeyFloatVal(int key, float defVal);
Export(float) lpGetStrKeyFloatVal(const char* key, float defVal);
Export(const char*) lpGetIntKeyStrVal(int key, const char* defVal);
Export(const char*) lpGetStrKeyStrVal(const char* key, const char* defVal);


#endif // UNITSYNC_API_H
