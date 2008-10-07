//	Description:	Implementation file for testai.dll dll file.
//					Implements an AI for spring.
// Dont modify this file
/////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "TestAI.h"
#include "groupai.h"
#include <set>

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif

std::set<IGroupAI*> ais;

//Returnerar DLLens typ och version
int (WINAPI GetGroupAiVersion)()
{
	return AI_INTERFACE_VERSION;
}

void (WINAPI GetAiName)(char* name)
{
	strcpy(name,AI_NAME);
}

IGroupAI* (WINAPI GetNewAI)()
{
	CGroupAI* ai=new CGroupAI();
	ais.insert(ai);
	return ai;
}

void (WINAPI ReleaseAI)(IGroupAI* i)
{
	delete i;
	ais.erase(i);
}


#ifdef __cplusplus
}
#endif

