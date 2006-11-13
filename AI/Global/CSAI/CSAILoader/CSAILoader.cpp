#include <windows.h>
#include <_vcclrit.h>

#include "CSAIProxy.h"
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
    __crt_dll_initialize();
    
	CSAIProxy* ai=new CSAIProxy;
	ais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(IGlobalAI* i)
{
	delete i;
	ais.erase(i);
}
