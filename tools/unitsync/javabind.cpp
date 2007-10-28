#include "StdAfx.h"
#include "unitsync.h"
//#include "aflobby_CUnitSyncJNIBindings.h"
#include "FileSystem/ArchiveFactory.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Map/SMF/mapfile.h"
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/Textures/Bitmap.h"
#include "TdfParser.h"

#include "Syncer.h"
#include "SyncServer.h"

#include <string>
#include <vector>
#include <algorithm>

#define NAMEBUF_SIZE 4096

#define JNI_BINDINGS_VERSION 3

// JNIEXPORT doesn't define default visibility
#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif

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
DLL_EXPORT void*        __stdcall GetMinimap(const char* filename, int miplevel);
DLL_EXPORT int          __stdcall GetMapArchiveCount(const char* mapName);
DLL_EXPORT const char*  __stdcall GetMapArchiveName(int index);
DLL_EXPORT unsigned int __stdcall GetMapChecksum(int index);
DLL_EXPORT int          __stdcall GetPrimaryModCount();
DLL_EXPORT const char*  __stdcall GetPrimaryModName(int index);
DLL_EXPORT const char*	__stdcall GetPrimaryModShortName(int index);
DLL_EXPORT const char*	__stdcall GetPrimaryModGame(int index);
DLL_EXPORT const char*	__stdcall GetPrimaryModShortGame(int index);
DLL_EXPORT const char*	__stdcall GetPrimaryModMutator(int index);
DLL_EXPORT const char*	__stdcall GetPrimaryModDescription(int index);
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

// lua custom lobby settings
DLL_EXPORT int          __stdcall GetMapOptionCount(const char* name);
DLL_EXPORT int          __stdcall GetModOptionCount();

DLL_EXPORT const char*  __stdcall GetOptionName(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionDesc(int optIndex);
DLL_EXPORT int          __stdcall GetOptionType(int optIndex);

// Bool Options
DLL_EXPORT int          __stdcall GetOptionBoolDef(int optIndex);

// Number Options
DLL_EXPORT float        __stdcall GetOptionNumberDef(int optIndex);
DLL_EXPORT float        __stdcall GetOptionNumberMin(int optIndex);
DLL_EXPORT float        __stdcall GetOptionNumberMax(int optIndex);
DLL_EXPORT float        __stdcall GetOptionNumberStep(int optIndex);

// String Options
DLL_EXPORT const char*  __stdcall GetOptionStringDef(int optIndex);
DLL_EXPORT int          __stdcall GetOptionStringMaxLen(int optIndex);

// List Options
DLL_EXPORT int          __stdcall GetOptionListCount(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionListDef(int optIndex);
DLL_EXPORT const char*  __stdcall GetOptionListItemName(int optIndex, int itemIndex);
DLL_EXPORT const char*  __stdcall GetOptionListItemDesc(int optIndex, int itemIndex);

/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class aflobby_CUnitSyncJNIBindings */

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
	* Method:    Message
	* Signature: (Ljava/lang/String;)V
	*/
	JNIEXPORT void JNICALL Java_aflobby_CUnitSyncJNIBindings_Message
		(JNIEnv *env, jclass myobject, jstring  p_szMessage){
			const char* c = env->GetStringUTFChars( p_szMessage,0);
			Message(c);
			env->ReleaseStringUTFChars( p_szMessage,c);
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    Init
	* Signature: (ZI)I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_Init
		(JNIEnv *env, jclass myobject, jboolean isServer, jint id){
			return Init(isServer,id);
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
			return ProcessUnits();
		}

	/*
	* Class:     aflobby_CUnitSyncJNIBindings
	* Method:    ProcessUnitsNoChecksum
	* Signature: ()I
	*/
	JNIEXPORT jint JNICALL Java_aflobby_CUnitSyncJNIBindings_ProcessUnitsNoChecksum
		(JNIEnv *env, jclass myobject){
			return ProcessUnitsNoChecksum();
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
			unsigned int i = GetMapChecksum(index);
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
				for(vector<string>::iterator i = f.begin(); i != f.end(); ++i){
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
			env->SetCharArrayRegion(a,0,1024*1024*2,(const jchar*)GetMinimap(c,0));
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

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionListItemName
		(JNIEnv *env, jclass myobject, jint optIndex, jint itemIndex){
			return env->NewStringUTF(GetOptionListItemName(optIndex, itemIndex));
	}

	JNIEXPORT jstring JNICALL Java_aflobby_CUnitSyncJNIBindings_GetOptionListItemDesc
		(JNIEnv *env, jclass myobject, jint optIndex, jint itemIndex){
			return env->NewStringUTF(GetOptionListItemDesc(optIndex, itemIndex));
	}

#ifdef __cplusplus
}
#endif
#endif
