#include "GlobalAI.h"
#include "ExternalAI/aibase.h"

/////////////////////////////////////////////////////////////////////////////

std::set<IGlobalAI*> ais;

DLL_EXPORT int GetGlobalAiVersion()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

DLL_EXPORT IGlobalAI* GetNewAI()
{
	CGlobalAI* ai=new CGlobalAI;
	ais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(IGlobalAI* i)
{
	delete (CGlobalAI*)i;
	ais.erase(i);
}
