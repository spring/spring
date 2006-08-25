#include "StdAfx.h"
#include "GroupAI.h"
#include "ExternalAI/aibase.h"
#include "Sim/Units/UnitDef.h"
#include <set>

DLL_EXPORT int GetGroupAiVersion()
{
	return AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

DLL_EXPORT bool IsUnitSuited(const UnitDef* unitDef)
{
	if(unitDef->energyUpkeep>0.0f && (unitDef->makesMetal>0.0f || unitDef->extractsMetal>0.0f))
		return true;
	else
		return false;
}

DLL_EXPORT IGroupAI* GetNewAI()
{
	CGroupAI* ai=new CGroupAI();
//	ais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(IGroupAI* i)
{
	delete i;
//	ais.erase(i);
}

