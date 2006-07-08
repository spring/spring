//-------------------------------------------------------------------------
// NTai
//
// 
// Copyright 2004-2006 AF
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "float3.h"
#include "GlobalAI.h"
#include "ExternalAI/aibase.h"

DLL_EXPORT int GetGlobalAiVersion(){
	return GLOBAL_AI_INTERFACE_VERSION;
}
DLL_EXPORT void GetAiName(char* name){
	strcpy(name,AI_NAME);
}
DLL_EXPORT IGlobalAI* GetNewAI(){
	return new CNTai();
}
DLL_EXPORT void ReleaseAI(IGlobalAI* i){
	delete i;
}
/*
/////////////////////////////////////////////////////////////////////////////
///* DO NOT EDIT THIS FILE - it is machine generated *#/
#include <jni.h>
///* Header for class jlobby_JSpringAI *#/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// SSSHHHHHHHH!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! You didn't see this!!!!!! /////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _Included_jlobby_JSpringAI
#define _Included_jlobby_JSpringAI
#ifdef __cplusplus
extern "C" {
#endif
	/*
	* Class:     jlobby_JSpringAI
	* Method:    JVersion
	* Signature: ()I
	*#/
	JNIEXPORT jint JNICALL Java_jlobby_JSpringAI_JVersion
		(JNIEnv *env, jobject obj){
			return 1;
		}

	/*
	* Class:     jlobby_JSpringAI
	* Method:    GetName
	* Signature: ()Ljava/lang/String;
	*#/
	JNIEXPORT jstring JNICALL Java_jlobby_JSpringAI_GetName
		(JNIEnv *env, jobject obj){
			return env->NewStringUTF(AI_NAME);
		}

	/*
	* Class:     jlobby_JSpringAI
	* Method:    GetDescription
	* Signature: (Z)Ljava/lang/String;
	*#/
	JNIEXPORT jstring JNICALL Java_jlobby_JSpringAI_GetDescription
		(JNIEnv *env, jobject obj, jboolean b){
			if(b >0){
				return env->NewStringUTF("<b>Description of NTai!!!</b>");
			}else{
				return env->NewStringUTF("Description of NTai!!!");
			}
		}

	/*
	* Class:     jlobby_JSpringAI
	* Method:    GetVersion
	* Signature: ()I
	*#/
	JNIEXPORT jint JNICALL Java_jlobby_JSpringAI_GetVersion
		(JNIEnv *env, jobject obj){
			return 817;// 8.1 RC7
		}

	/*
	* Class:     jlobby_JSpringAI
	* Method:    GetInterfaceVersion
	* Signature: ()I
	*#/
	JNIEXPORT jint JNICALL Java_jlobby_JSpringAI_GetInterfaceVersion
		(JNIEnv *env, jobject obj){
			return GLOBAL_AI_INTERFACE_VERSION;
		}
	/*
	* Class:     jlobby_JSpringAI
	* Method:    IsGroupAI
	* Signature: ()Z
	*#/
	JNIEXPORT jboolean JNICALL Java_jlobby_JSpringAI_IsGroupAI
		(JNIEnv *env, jobject obj){
			return 0;
		}

#ifdef __cplusplus
}
#endif
#endif
*/

