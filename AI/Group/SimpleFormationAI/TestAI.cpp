#include "GroupAI.h"
#include "ExternalAI/aibase.h"
#include <set>

DLL_EXPORT int GetGroupAiVersion()
{
	return AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

DLL_EXPORT int IsUnitSuited(const UnitDef* unitDef)
{
	CGroupAI* aitemp=new CGroupAI();
	return aitemp->IsUnitSuited(unitDef);
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

