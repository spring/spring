//-------------------------------------------------------------------------
// NTai
// Copyright 2004-2007 AF
// Released under GPL 2 license
//-------------------------------------------------------------------------

#include "CNTai.h"

SHARED_EXPORT int GetGlobalAiVersion(){
	return GLOBAL_AI_INTERFACE_VERSION;
}

SHARED_EXPORT void GetAiName(char* name){
	strcpy(name,ntai::AI_NAME);
}

SHARED_EXPORT IGlobalAI* GetNewAI(){
	return new ntai::CNTai();
}

SHARED_EXPORT void ReleaseAI(IGlobalAI* i){
	delete i;
}

