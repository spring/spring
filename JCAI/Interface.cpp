//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "BaseAIDef.h"
#include "BaseAIObjects.h"
#include "GlobalAI.h"
#include <windows.h>

/////////////////////////////////////////////////////////////////////////////

std::set<IGlobalAI*> ais;

#define DLL_EXPORT extern "C" __declspec(dllexport)

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
	MainAI* ai=new MainAI(ais.size());
	ais.insert(ai);
	return ai;
}

DLL_EXPORT void WINAPI ReleaseAI(IGlobalAI* i)
{
	delete (MainAI*)i;
	ais.erase(i);

	if (ais.empty ())
		MainAI::FreeCommonData ();
}
