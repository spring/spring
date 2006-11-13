#include <windows.h>

#include "AbicProxy.h"
#include "dllbuild.h"

/////////////////////////////////////////////////////////////////////////////

std::set<IGlobalAI*> ais;

SPRING_TO_ABIC_API int GetGlobalAiVersion()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

SPRING_TO_ABIC_API void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

SPRING_TO_ABIC_API IGlobalAI* GetNewAI()
{
	AbicProxy* ai=new AbicProxy;
	ais.insert(ai);
	return ai;
}

SPRING_TO_ABIC_API void ReleaseAI(IGlobalAI* i)
{
	delete i;
	ais.erase(i);
}
