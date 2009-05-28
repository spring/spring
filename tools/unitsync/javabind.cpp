#include "StdAfx.h"
#include "unitsync.h"
//#include "aflobby_CUnitSyncJNIBindings.h"
#include "FileSystem/ArchiveFactory.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Map/SMF/mapfile.h"
#include "ConfigHandler.h"
#include "FileSystem/FileSystem.h"
#include "Rendering/Textures/Bitmap.h"
#include "exportdefines.h"

#include <string>
#include <string.h>
#include <vector>
#include <algorithm>

#define NAMEBUF_SIZE 4096

#define JNI_BINDINGS_VERSION 3

// JNIEXPORT doesn't define default visibility
#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif

EXPORT(const char* ) GetSpringVersion();
EXPORT(void        ) UnInit();
EXPORT(int         ) ProcessUnits(void);
EXPORT(int         ) ProcessUnitsNoChecksum(void);
EXPORT(const char* ) GetCurrentList();
EXPORT(void        ) AddClient(int id, const char *unitList);
EXPORT(void        ) RemoveClient(int id);
EXPORT(const char* ) GetClientDiff(int id);
EXPORT(void        ) InstallClientDiff(const char *diff);
EXPORT(int         ) GetUnitCount();
EXPORT(const char* ) GetUnitName(int unit);
EXPORT(const char* ) GetFullUnitName(int unit);
EXPORT(int         ) IsUnitDisabled(int unit);
EXPORT(int         ) IsUnitDisabledByClient(int unit, int clientId);
EXPORT(void        ) AddArchive(const char* name);
EXPORT(void        ) AddAllArchives(const char* root);
EXPORT(unsigned int) GetArchiveChecksum(const char* arname);
EXPORT(int         ) GetMapCount();
EXPORT(const char* ) GetMapName(int index);
EXPORT(int         ) GetMapInfoEx(const char* name, MapInfo* outInfo, int version);
EXPORT(int         ) GetMapInfo(const char* name, MapInfo* outInfo);
EXPORT(void*       ) GetMinimap(const char* filename, int miplevel);
EXPORT(int         ) GetMapArchiveCount(const char* mapName);
EXPORT(const char* ) GetMapArchiveName(int index);
EXPORT(unsigned int) GetMapChecksumFromName(const char* mapName);
EXPORT(unsigned int) GetMapChecksum(int index);
EXPORT(int         ) GetPrimaryModCount();
EXPORT(const char* ) GetPrimaryModName(int index);
EXPORT(const char* ) GetPrimaryModShortName(int index);
EXPORT(const char* ) GetPrimaryModGame(int index);
EXPORT(const char* ) GetPrimaryModShortGame(int index);
EXPORT(const char* ) GetPrimaryModVersion(int index);
EXPORT(const char* ) GetPrimaryModMutator(int index);
EXPORT(const char* ) GetPrimaryModDescription(int index);
EXPORT(const char* ) GetPrimaryModArchive(int index);
EXPORT(int         ) GetPrimaryModArchiveCount(int index);
EXPORT(const char* ) GetPrimaryModArchiveList(int arnr);
EXPORT(int         ) GetPrimaryModIndex(const char* name);
EXPORT(unsigned int) GetPrimaryModChecksum(int index);
EXPORT(unsigned int) GetPrimaryModChecksumFromName(const char* name);
EXPORT(int         ) GetSideCount();
EXPORT(const char* ) GetSideName(int side);
EXPORT(int         ) OpenFileVFS(const char* name);
EXPORT(void        ) CloseFileVFS(int handle);
EXPORT(void        ) ReadFileVFS(int handle, void* buf, int length);
EXPORT(int         ) FileSizeVFS(int handle);
EXPORT(int         ) InitFindVFS(const char* pattern);
EXPORT(int         ) FindFilesVFS(int handle, char* nameBuf, int size);
EXPORT(int         ) OpenArchive(const char* name);
EXPORT(void        ) CloseArchive(int archive);
EXPORT(int         ) FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
EXPORT(int         ) OpenArchiveFile(int archive, const char* name);
EXPORT(int         ) ReadArchiveFile(int archive, int handle, void* buffer, int numBytes);
EXPORT(void        ) CloseArchiveFile(int archive, int handle);
EXPORT(int         ) SizeArchiveFile(int archive, int handle);

// lua custom lobby settings
EXPORT(int         ) GetMapOptionCount(const char* name);
EXPORT(int         ) GetModOptionCount();

EXPORT(const char* ) GetOptionKey(int optIndex);
EXPORT(const char* ) GetOptionName(int optIndex);
EXPORT(const char* ) GetOptionDesc(int optIndex);
EXPORT(int         ) GetOptionType(int optIndex);

// Bool Options
EXPORT(int         ) GetOptionBoolDef(int optIndex);

// Number Options
EXPORT(float       ) GetOptionNumberDef(int optIndex);
EXPORT(float       ) GetOptionNumberMin(int optIndex);
EXPORT(float       ) GetOptionNumberMax(int optIndex);
EXPORT(float       ) GetOptionNumberStep(int optIndex);

// String Options
EXPORT(const char* ) GetOptionStringDef(int optIndex);
EXPORT(int         ) GetOptionStringMaxLen(int optIndex);

// List Options
EXPORT(int         ) GetOptionListCount(int optIndex);
EXPORT(const char* ) GetOptionListDef(int optIndex);
EXPORT(const char* ) GetOptionListItemKey(int optIndex, int itemIndex);
EXPORT(const char* ) GetOptionListItemName(int optIndex, int itemIndex);
EXPORT(const char* ) GetOptionListItemDesc(int optIndex, int itemIndex);

// Spring settings callback

EXPORT(const char* ) GetSpringConfigString( const char* name, const char* defvalue );
EXPORT(int         ) GetSpringConfigInt( const char* name, const int defvalue );
EXPORT(float       ) GetSpringConfigFloat( const char* name, const float defvalue );
EXPORT(void        ) SetSpringConfigString( const char* name, const char* value );
EXPORT(void        ) SetSpringConfigInt( const char* name, const int value );
EXPORT(void        ) SetSpringConfigFloat( const char* name, const float value );


/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for Sun JNI API, which is used for the Java Unitsync API bindings
inside class aflobby_CUnitSyncJNIBindings */

#ifndef _Included_aflobby_CUnitSyncJNIBindings
#define _Included_aflobby_CUnitSyncJNIBindings
#ifdef __cplusplus
extern "C" {
#endif

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetSpringVersion
	* Signature: ()Ljava/lang/String;
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIVersion_GetVersion
		(JNIEnv *env, jclass myobject){
			return JNI_BINDINGS_VERSION;
		}
	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetSpringVersion
	* Signature: ()Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetSpringVersion
		(JNIEnv *env, jclass myobject){
			return env->NewStringUTF(GetSpringVersion());
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    Init
	* Signature: (ZI)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_Init
		(JNIEnv *env, jclass myobject, jboolean isServer, jint id){
			return 1;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    UnInit
	* Signature: ()V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_UnInit
		(JNIEnv *env, jclass myobject){
			UnInit();
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    ProcessUnits
	* Signature: ()I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_ProcessUnits
		(JNIEnv *env, jclass myobject){
			while (true) {
				const int left = ProcessUnits();
				if (left <= 0){
					break;
				}
			}
			return 0;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    ProcessUnitsNoChecksum
	* Signature: ()I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_ProcessUnitsNoChecksum
		(JNIEnv *env, jclass myobject){
			while (true) {
				const int left = ProcessUnitsNoChecksum();
				if (left <= 0){
					break;
				}
			}
			return 0;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetCurrentList
	* Signature: ()Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetCurrentList
		(JNIEnv *env, jclass myobject){
			return env->NewStringUTF(GetCurrentList());
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    AddClient
	* Signature: (ILjava/lang/String;)V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_AddClient
		(JNIEnv *env, jclass myobject, jint id, jstring unitList){
			const char* c = env->GetStringUTFChars(unitList,0);
			AddClient(id,c);
			env->ReleaseStringUTFChars(unitList,c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    RemoveClient
	* Signature: (I)V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_RemoveClient
		(JNIEnv *env, jclass myobject, jint id){
			RemoveClient(id);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetClientDiff
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetClientDiff
		(JNIEnv *env, jclass myobject, jint id){
			return env->NewStringUTF(GetClientDiff(id));
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    InstallClientDiff
	* Signature: (Ljava/lang/String;)V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_InstallClientDiff
		(JNIEnv *env, jclass myobject, jstring diff){
			const char* c = env->GetStringUTFChars(diff,0);
			InstallClientDiff(c);
			env->ReleaseStringUTFChars(diff,c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetUnitCount
	* Signature: ()I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetUnitCount
		(JNIEnv *env, jclass myobject){
			return GetUnitCount();
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetUnitName
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetUnitName
		(JNIEnv *env, jclass myobject, jint unit){
			return env->NewStringUTF(GetUnitName(unit));
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetFullUnitName
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetFullUnitName
		(JNIEnv *env, jclass myobject, jint unit){
			return env->NewStringUTF(GetFullUnitName(unit));
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    IsUnitDisabled
	* Signature: (I)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_IsUnitDisabled
		(JNIEnv *env, jclass myobject, jint unit){
			return IsUnitDisabled(unit);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    IsUnitDisabledByClient
	* Signature: (II)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_IsUnitDisabledByClient
		(JNIEnv *env, jclass myobject, jint unit, jint clientId){
			return IsUnitDisabledByClient(unit,clientId);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    AddArchive
	* Signature: (Ljava/lang/String;)V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_AddArchive
		(JNIEnv *env, jclass myobject, jstring name){
			const char* c = env->GetStringUTFChars(name,0);
			AddArchive(c);
			env->ReleaseStringUTFChars(name,c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    AddAllArchives
	* Signature: (Ljava/lang/String;)V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_AddAllArchives
		(JNIEnv *env, jclass myobject, jstring root){
			const char* c = env->GetStringUTFChars(root,0);
			AddAllArchives(c);
			env->ReleaseStringUTFChars(root,c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetArchiveChecksum
	* Signature: (Ljava/lang/String;)I
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetArchiveChecksum
		(JNIEnv *env, jclass myobject, jstring arname){
			const char* a = env->GetStringUTFChars(arname,0);
			char* c = new char[15];
			unsigned int i = GetArchiveChecksum(a);
			sprintf(c,"%u",i);
			env->ReleaseStringUTFChars(arname,a);
			return env->NewStringUTF((const char*)c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetMapCount
	* Signature: ()I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMapCount
		(JNIEnv *env, jclass myobject){
			signed int i = GetMapCount();
			return i;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetMapName
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMapName
		(JNIEnv *env, jclass myobject, jint index){
			return env->NewStringUTF(GetMapName((signed)index));
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetMapArchiveCount
	* Signature: (Ljava/lang/String;)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMapArchiveCount
		(JNIEnv *env, jclass myobject, jstring mapName){
			const char* c = env->GetStringUTFChars(mapName,0);
			int i = GetMapArchiveCount(c);
			env->ReleaseStringUTFChars(mapName,c);
			return i;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetMapArchiveName
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMapArchiveName
		(JNIEnv *env, jclass myobject, jint index){
			return env->NewStringUTF(GetMapArchiveName(index));
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetMapChecksum
	* Signature: (I)I
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMapChecksum
		(JNIEnv *env, jclass myobject, jint index){
			char* c = new char[15];
			int i = GetMapChecksum(index);
			sprintf(c,"%u",i);
			return env->NewStringUTF((const char*)c);
		}

	
	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetMapChecksumFromName
	* Signature: (I)I
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMapChecksumFromName
		(JNIEnv *env, jclass myobject, jstring name){
			char* c = new char[15];
			const char* cname = env->GetStringUTFChars(name,0);
			int i = GetMapChecksumFromName(cname);
			sprintf(c,"%u",i);
			return env->NewStringUTF((const char*)c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetPrimaryModCount
	* Signature: ()I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModCount
		(JNIEnv *env, jclass myobject){
			return GetPrimaryModCount();
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetPrimaryModName
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModName
		(JNIEnv *env, jclass myobject, jint index){
			const char* c = GetPrimaryModName(index);
			return env->NewStringUTF(c);
		}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModShortName
		(JNIEnv *env, jclass myobject, jint index){
			const char* c = GetPrimaryModShortName(index);
			return env->NewStringUTF(c);
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModGame
		(JNIEnv *env, jclass myobject, jint index){
			const char* c = GetPrimaryModGame(index);
			return env->NewStringUTF(c);
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModShortGame
		(JNIEnv *env, jclass myobject, jint index){
			const char* c = GetPrimaryModShortGame(index);
			return env->NewStringUTF(c);
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModVersion
		(JNIEnv *env, jclass myobject, jint index){
			const char* c = GetPrimaryModVersion(index);
			return env->NewStringUTF(c);
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModMutator
		(JNIEnv *env, jclass myobject, jint index){
			const char* c = GetPrimaryModMutator(index);
			return env->NewStringUTF(c);
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModDescription
		(JNIEnv *env, jclass myobject, jint index){
			const char* c = GetPrimaryModDescription(index);
			return env->NewStringUTF(c);
	}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetPrimaryModArchive
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModArchive
		(JNIEnv *env, jclass myobject, jint index){
			const char* c = GetPrimaryModArchive(index);
			return env->NewStringUTF(c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetPrimaryModArchiveCount
	* Signature: (I)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModArchiveCount
		(JNIEnv *env, jclass myobject, jint index){
			return GetPrimaryModArchiveCount(index);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetPrimaryModArchiveList
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModArchiveList
		(JNIEnv *env, jclass myobject, jint arnr){
			const char* c = GetPrimaryModArchiveList(arnr);
			return env->NewStringUTF(c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetPrimaryModIndex
	* Signature: (Ljava/lang/String;)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModIndex
		(JNIEnv *env, jclass myobject, jstring name){
			const char* cname = env->GetStringUTFChars(name,0);
			int i = GetPrimaryModIndex(cname);
			env->ReleaseStringUTFChars(name,cname);
			return i;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetPrimaryModChecksum
	* Signature: (I)I
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModChecksum
		(JNIEnv *env, jclass myobject, jint index){
			char* c = new char[15];
			int i = GetPrimaryModChecksum(index);
			sprintf(c,"%i",i);
			return env->NewStringUTF((const char*)c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetPrimaryModChecksumFromName
	* Signature: (I)I
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetPrimaryModChecksumFromName
		(JNIEnv *env, jclass myobject, jstring name){
			char* c = new char[15];
			const char* cname = env->GetStringUTFChars(name,0);
			int i = GetPrimaryModChecksumFromName(cname);
			sprintf(c,"%u",i);
			return env->NewStringUTF((const char*)c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetSideCount
	* Signature: ()I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetSideCount
		(JNIEnv *env, jclass myobject){
			return GetSideCount();
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetSideName
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetSideName
		(JNIEnv *env, jclass myobject, jint side){
			return env->NewStringUTF(GetSideName(side));
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    penFileVFS
	* Signature: (Ljava/lang/String;)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_OpenFileVFS
		(JNIEnv *env, jclass myobject, jstring name){
			const char* cname = env->GetStringUTFChars(name,0);
			int i = OpenFileVFS(cname);
			env->ReleaseStringUTFChars(name,cname);
			return i;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    CloseFileVFS
	* Signature: (I)V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_CloseFileVFS
		(JNIEnv *env, jclass myobject, jint handle){
			CloseFileVFS(handle);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    ReadFileVFS
	* Signature: (I)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_ReadFileVFS
		(JNIEnv *env, jclass myobject, jint handle){
			int length = FileSizeVFS(handle);
			unsigned char* buf = new unsigned char[length];
			ReadFileVFS(handle,buf,length);
			return env->NewStringUTF((const char*)buf);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    FileSizeVFS
	* Signature: (I)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_FileSizeVFS
		(JNIEnv *env, jclass myobject, jint handle){
			return FileSizeVFS(handle);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    SearchVFS
	* Signature: (ILjava/lang/String;I)I
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_SearchVFS
		(JNIEnv *env, jclass myobject, jstring pattern){
			const char* cpattern = env->GetStringUTFChars(pattern,0);

			std::string path = filesystem.GetDirectory(cpattern);
			std::string patt = filesystem.GetFilename(cpattern);
			std::vector<string> f = CFileHandler::FindFiles(path, patt);
			string s = "";
			if(f.empty()==false){
				for(std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i){
					string q = *i;
					if(s != string("")){
						s += ",";
					}
					s += q;
					
				}
			}

			return env->NewStringUTF(s.c_str());

		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    OpenArchive
	* Signature: (Ljava/lang/String;)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_OpenArchive
		(JNIEnv *env, jclass myobject, jstring name){
			const char* cname = env->GetStringUTFChars(name,0);
			int i =  OpenArchive(cname);
			env->ReleaseStringUTFChars(name,cname);
			return i;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    CloseArchive
	* Signature: (I)V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_CloseArchive
		(JNIEnv *env, jclass myobject, jint archive){
			CloseArchive(archive);
		}
	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    ListFilesArchive
	* Signature: (IILjava/lang/String;I)I
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_ListFilesArchive
		(JNIEnv *env, jclass myobject, jint archive){

			string rstring;
			bool loop = true;
			int cur = 0;

			// loop to get each entry untill FindFilesArchive returns zero
			// signifying there are no more entries...
			while(loop){
				string name;
				int s;
				char* c = new char[128];
				memset(c,0,128);
				cur = FindFilesArchive(archive, cur, c, &s);

				if(rstring != string("")){
					rstring += ",";
				}
				rstring += c;
				rstring += "#";
				rstring += s;

				delete[] c;

				if(cur == 0){
					break;
				}
			}

			return env->NewStringUTF(rstring.c_str());
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    OpenArchiveFile
	* Signature: (ILjava/lang/String;)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_OpenArchiveFile
		(JNIEnv *env, jclass myobject, jint archive, jstring name){
			const char* file = env->GetStringUTFChars(name,0);
			int i = OpenArchiveFile(archive,file);
			env->ReleaseStringUTFChars(name,file);
			return i;
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    ReadArchiveFile
	* Signature: (II)Ljava/lang/String;
	public static native String ReadArchiveFile(int archive, int handle);
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_ReadArchiveFile
		(JNIEnv *env, jclass myobject, jint archive, jint handle){
			int size = SizeArchiveFile(archive,handle);
			unsigned char* buffer = new unsigned char[size];
			ReadArchiveFile(archive,handle,buffer,size);
			return env->NewStringUTF((const char*)buffer);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    CloseArchiveFile
	* Signature: (II)V
	public static native void CloseArchiveFile(int archive, int handle);
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_CloseArchiveFile
		(JNIEnv *env, jclass myobject, jint archive, jint handle){
			CloseArchiveFile(archive,handle);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    SizeArchiveFile
	* Signature: (II)I
	* public static native int  SizeArchiveFile(int archive, int handle);    
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_SizeArchiveFile
		(JNIEnv *env, jclass myobject, jint archive, jint handle){
			return SizeArchiveFile(archive,handle);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    GetDataDirs
	* Signature: 
	public static native String GetDataDirs(boolean write);
	*/
	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetDataDirs
		(JNIEnv *env, jclass myobject, jboolean write){
			std::vector<std::string> f;
			std::string s;
			if (write) 
				s = FileSystemHandler::GetInstance().GetWriteDir()+",";
			else {
				f = FileSystemHandler::GetInstance().GetDataDirectories();
				if(f.empty() == false){
					for(std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i){
						s += *i+",";
					}
				}
			}
			/*int size = SizeArchiveFile(archive,handle);
			unsigned char* buffer = new unsigned char[size];
			ReadArchiveFile(archive,handle,buffer,size);*/
			return env->NewStringUTF((const char*)s.c_str());
		}

	

#define RM	0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    WriteMiniMap
	* Signature: (II)I
	*/
	JNIEXPORT jboolean JNICALL Java_aflobby_CUnitSyncJNIBindings_WriteMiniMap
		(JNIEnv *env, jclass myobject, jstring mapfile, jstring imagename, jint miplevel){
			const char *filename = env->GetStringUTFChars(mapfile, 0);
			const char *bitmap_filename = env->GetStringUTFChars(imagename, 0);
			void* minimap = GetMinimap(filename, miplevel);
			if (!minimap){
				env->ReleaseStringUTFChars(mapfile, filename);
				env->ReleaseStringUTFChars(mapfile, bitmap_filename);
				return false;
			}
			int size = 1024 >> miplevel;
			CBitmap bm;
			bm.Alloc(size, size);
			unsigned short *src = (unsigned short*)minimap;
			unsigned char *dst = bm.mem;
			for (int y = 0; y < size; y++) {
				for (int x = 0; x < size; x++){
					dst[0] = RED_RGB565   ((*src)) << 3;
					dst[1] = GREEN_RGB565 ((*src)) << 2;
					dst[2] = BLUE_RGB565  ((*src)) << 3;
					dst[3] = 255;
					++src;
					dst += 4;
				}
			}
			remove(bitmap_filename); //somehow overwriting doesn't work??
			bm.Save(bitmap_filename);
			// check whether the bm.Save succeeded?
			
			
			FILE* f = fopen(bitmap_filename, "rb");
			bool success = !!f;
			if (success) {
				fclose(f);
			}
			env->ReleaseStringUTFChars(mapfile, filename);
			env->ReleaseStringUTFChars(mapfile, bitmap_filename);
			return success;
		}

		
	JNIEXPORT jcharArray JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMiniMapArray
		(JNIEnv *env, jclass myobject, jstring mapName){
			jcharArray a = env->NewCharArray(1024*1024*2);
			const char* c = env->GetStringUTFChars(mapName,0);
			env->SetCharArrayRegion(a,0,1024*1024*2,(jchar*)GetMinimap(c,0));
			env->ReleaseStringUTFChars(mapName,c);
			return a;
	}

	// lua custom lobby settings
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMapOptionCount
		(JNIEnv *env, jclass myobject, jstring mapName){
			const char* c = env->GetStringUTFChars(mapName,0);
			int i = GetMapOptionCount(c);
			env->ReleaseStringUTFChars(mapName,c);
			return i;
	}

	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetModOptionCount
		(JNIEnv *env, jclass myobject){
			return GetModOptionCount();
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionKey
		(JNIEnv *env, jclass myobject, jint optIndex){
			return env->NewStringUTF(GetOptionKey(optIndex));
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionName
		(JNIEnv *env, jclass myobject, jint optIndex){
			return env->NewStringUTF(GetOptionName(optIndex));
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionDesc
		(JNIEnv *env, jclass myobject, jint optIndex){
			return env->NewStringUTF(GetOptionDesc(optIndex));
	}

	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionType
		(JNIEnv *env, jclass myobject, jint optIndex){
			return GetOptionType(optIndex);
	}

	// Bool Options
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionBoolDef
		(JNIEnv *env, jclass myobject, jint optIndex){
			return GetOptionBoolDef(optIndex);
	}

	// Number Options
	JNIEXPORT jfloat JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionNumberDef
		(JNIEnv *env, jclass myobject, jint optIndex){
			return GetOptionNumberDef(optIndex);
	}

	JNIEXPORT jfloat JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionNumberMin
		(JNIEnv *env, jclass myobject, jint optIndex){
			return GetOptionNumberMin(optIndex);
	}

	JNIEXPORT jfloat JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionNumberMax
		(JNIEnv *env, jclass myobject, jint optIndex){
			return GetOptionNumberMax(optIndex);
	}

	JNIEXPORT jfloat JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionNumberStep
		(JNIEnv *env, jclass myobject, jint optIndex){
			return GetOptionNumberStep(optIndex);
	}

	// String Options

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionStringDef
		(JNIEnv *env, jclass myobject, jint optIndex){
			return env->NewStringUTF(GetOptionStringDef(optIndex));
	}

	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionStringMaxLen
		(JNIEnv *env, jclass myobject, jint optIndex){
			return GetOptionStringMaxLen(optIndex);
	}

	// List Options

	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionListCount
		(JNIEnv *env, jclass myobject, jint optIndex){
			return GetOptionListCount(optIndex);
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionListDef
		(JNIEnv *env, jclass myobject, jint optIndex){
			return env->NewStringUTF(GetOptionListDef(optIndex));
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionListItemKey
		(JNIEnv *env, jclass myobject, jint optIndex, jint itemIndex){
			return env->NewStringUTF(GetOptionListItemKey(optIndex, itemIndex));
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionListItemName
		(JNIEnv *env, jclass myobject, jint optIndex, jint itemIndex){
			return env->NewStringUTF(GetOptionListItemName(optIndex, itemIndex));
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionListItemDesc
		(JNIEnv *env, jclass myobject, jint optIndex, jint itemIndex){
			return env->NewStringUTF(GetOptionListItemDesc(optIndex, itemIndex));
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetSpringConfigString
		( JNIEnv *env, jclass myobject, jstring name, jstring defvalue ){
			const char* cn = env->GetStringUTFChars(name,0);
			const char* cd = env->GetStringUTFChars(defvalue,0);

			const char* r = GetSpringConfigString(cn,cd);

			env->ReleaseStringUTFChars(name,cn);
			env->ReleaseStringUTFChars(defvalue,cd);

			return env->NewStringUTF(r);
	}

	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_GetSpringConfigInt
		( JNIEnv *env, jclass myobject, jstring name, jint defvalue ){
			const char* cn = env->GetStringUTFChars(name,0);

			int r = GetSpringConfigInt(cn,defvalue);

			env->ReleaseStringUTFChars(name,cn);

			return r;
	}

	JNIEXPORT jfloat JNICALL Java_aflobby_CUnitSyncJNIBindings_GetSpringConfigFloat
		( JNIEnv *env, jclass myobject, jstring name, jfloat defvalue ){

			const char* cn = env->GetStringUTFChars(name,0);

			float r = GetSpringConfigFloat(cn,defvalue);

			env->ReleaseStringUTFChars(name,cn);

			return r;
	}

	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_SetSpringConfigString
		( JNIEnv *env, jclass myobject, jstring name, jstring value ){
			const char* c = env->GetStringUTFChars(name,0);
			const char* v = env->GetStringUTFChars(value,0);
			SetSpringConfigString(c,v);
			env->ReleaseStringUTFChars(name,c);
			env->ReleaseStringUTFChars(name,v);
	}

	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_SetSpringConfigInt
		( JNIEnv *env, jclass myobject, jstring name, jint value ){
			const char* c = env->GetStringUTFChars(name,0);
			SetSpringConfigInt(c,value);
			env->ReleaseStringUTFChars(name,c);
	}

	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_SetSpringConfigFloat
		( JNIEnv *env, jclass myobject, jstring name, jfloat value ){
			const char* c = env->GetStringUTFChars(name,0);
			SetSpringConfigFloat(c,value);
			env->ReleaseStringUTFChars(name,c);
	}


	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetMapInfo
		( JNIEnv *env, jclass myobject, jstring name ){
			const char* c = env->GetStringUTFChars(name,0);
			MapInfo* i = new MapInfo();
			GetMapInfo(c,i);
			std::string s;

			s = i->author;
			s += "\n";

			s += i->description;
			s += "\n";

			s += i->extractorRadius;
			s += "\n";

			s+= i->gravity;
			s += "\n";

			s += i->height;
			s += "\n";

			s += i->maxMetal;
			s += "\n";

			s += i->maxWind;
			s += "\n";

			s += i->minWind;
			s += "\n";

			s += i->posCount;
			s += "\n";

			s += i->tidalStrength;
			s += "\n";

			s += i->width;

			for(int n = 0; n < i->posCount; n++){
				s += "\n";
				StartPos p = i->positions[n];
				s += p.x;
				s += ",";
				s += p.z;
			}

			delete i;
			
			env->ReleaseStringUTFChars(name,c);
			
			return env->NewStringUTF(s.c_str());
			
	}

#ifdef __cplusplus
}
#endif
#endif
