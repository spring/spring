// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "aidef.h"
#include "Platform/Win/win32.h"
#include "AAI.h"

/////////////////////////////////////////////////////////////////////////////

std::set<IGlobalAI*> ais;


DLL_EXPORT int GetGlobalAiVersion()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name, AAI_VERSION);
}

DLL_EXPORT IGlobalAI* GetNewAI()
{
	AAI* ai = new AAI();
	ais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(IGlobalAI* i)
{
	ais.erase(i);
	delete (AAI*)i;
}

DLL_EXPORT int IsCInterface(void) 
{
	return 0;
} 

DLL_EXPORT int IsLoadSupported() 
{
	return 0;
}
