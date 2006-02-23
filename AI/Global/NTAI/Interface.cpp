//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "float3.h"
#include "GlobalAI.h"
#include "ExternalAI/aibase.h"

/////////////////////////////////////////////////////////////////////////////

std::set<IGlobalAI*> ais;

DLL_EXPORT int WINAPI GetGlobalAiVersion()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

DLL_EXPORT void WINAPI GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

DLL_EXPORT IGlobalAI* WINAPI GetNewAI()
{
	CGlobalAI* ai=new CGlobalAI(ais.size());
	ais.insert(ai);
	return ai;
}

DLL_EXPORT void WINAPI ReleaseAI(IGlobalAI* i)
{
	delete (CGlobalAI*)i;
	ais.erase(i);
}
