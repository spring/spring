//-------------------------------------------------------------------------
// NTai
// Copyright 2004-2007 AF
// Released under GPL 2 license
//-------------------------------------------------------------------------

#include "CNTai.h"

DLL_EXPORT int GetGlobalAiVersion(){
	return GLOBAL_AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name){
	strcpy(name,ntai::AI_NAME);
}

DLL_EXPORT IGlobalAI* GetNewAI(){
	return new ntai::CNTai();
}

DLL_EXPORT void ReleaseAI(IGlobalAI* i){
	delete i;
}
