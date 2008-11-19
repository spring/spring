#include <string.h>
#include <stdlib.h>
#include "TestGlobalAI.h"
#include "ExternalAI/aibase.h"

/////////////////////////////////////////////////////////////////////////////

std::set<IGlobalAI*> ais;

SHARED_EXPORT int GetGlobalAiVersion()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

SHARED_EXPORT void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

SHARED_EXPORT IGlobalAI* GetNewAI()
{
	TestGlobalAI* ai=new TestGlobalAI;
	ais.insert(ai);
	return ai;
}

SHARED_EXPORT void ReleaseAI(IGlobalAI* i)
{
	delete i;
	ais.erase(i);
}
