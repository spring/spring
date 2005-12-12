//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
//
// 2005.12.12: Modified by Tobi Vollebregt for use in NTAI.
//   As opposed to TestAI.{cpp,h} this file works on Linux and Windows.
//
#include "float3.h"
#include "GlobalAI.h"
#include "../aibase.h"

/////////////////////////////////////////////////////////////////////////////

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
	CGlobalAI* ai=new CGlobalAI();
	return ai;
}

DLL_EXPORT void WINAPI ReleaseAI(IGlobalAI* i)
{
	delete (CGlobalAI*)i;
}
