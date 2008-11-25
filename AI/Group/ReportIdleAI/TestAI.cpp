#include "StdAfx.h"
#include "GroupAI.h"
#include "ExternalAI/aibase.h"
#include "Sim/Units/UnitDef.h"

SHARED_EXPORT int GetGroupAiVersion()
{
	return AI_INTERFACE_VERSION;
}

static const char* aiNameList[] = { AI_NAME, NULL };

SHARED_EXPORT const char** GetAiNameList()
{
	return aiNameList;
}

SHARED_EXPORT bool IsUnitSuited(unsigned aiNumber, const UnitDef* unitDef)
{
	if(unitDef->buildSpeed==0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

SHARED_EXPORT IGroupAI* GetNewAI(unsigned aiNumber)
{
	CGroupAI* ai=new CGroupAI();
//	ais.insert(ai);
	return ai;
}

SHARED_EXPORT void ReleaseAI(unsigned aiNumber, IGroupAI* i)
{
	delete i;
//	ais.erase(i);
}
