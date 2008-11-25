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
#include "ExternalAI/aibase.h"

/////////////////////////////////////////////////////////////////////////////

std::set<IGlobalAI*> ais;

SHARED_EXPORT int GetGlobalAiVersion()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

SHARED_EXPORT void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

SHARED_EXPORT IGlobalAI* GetNewAI()
{
	MainAI* ai=new MainAI(ais.size());
	ais.insert(ai);
	return ai;
}

SHARED_EXPORT void ReleaseAI(IGlobalAI* i)
{
	delete i;
	ais.erase(i);

	if (ais.empty ())
		MainAI::FreeCommonData ();
}

