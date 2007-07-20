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
