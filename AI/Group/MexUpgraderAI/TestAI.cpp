#include "StdAfx.h"
#include "GroupAI.h"
#include "ExternalAI/aibase.h"
#include "Sim/Units/UnitDef.h"

DLL_EXPORT int GetGroupAiVersion()
{
	return AI_INTERFACE_VERSION;
}

static const char* aiNameList[] = { AI_NAME, NULL };

DLL_EXPORT const char** GetAiNameList()
{
	return aiNameList;
}

DLL_EXPORT bool IsLoadSupported(unsigned aiNumber)
{
	return true;
}

DLL_EXPORT bool IsUnitSuited(unsigned aiNumber, const UnitDef* unitDef)
{
	if(unitDef->buildSpeed==0 || !unitDef->canmove)
		return false;
	else
		return true;
}

DLL_EXPORT IGroupAI* GetNewAI(unsigned aiNumber)
{
	CGroupAI* ai=new CGroupAI();
//	ais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(unsigned aiNumber, IGroupAI* i)
{
	delete i;
//	ais.erase(i);
}

