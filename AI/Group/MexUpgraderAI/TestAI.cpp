#include "GroupAI.h"
#include "ExternalAI/aibase.h"

DLL_EXPORT int GetGroupAiVersion()
{
	return AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
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

