#include "RAI.h"
#include "ExternalAI/aibase.h"
//#include <set>

/////////////////////////////////////////////////////////////////////////////

std::set<IGlobalAI*> oldais;

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
	cRAI* ai=new cRAI;
	oldais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(IGlobalAI* i)
{
	delete (cRAI*)i;
	oldais.erase(i);
}
