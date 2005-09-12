//	Class: -
//	Filename: dummyai.cpp
//	Author:	Stefan Johansson
//	Description:	Implementation file for testai.dll dll file.
//					Implements an AI for spring.
// Dont modify this file
/////////////////////////////////////////////////////////////////////

#include "TestAI.h"
#include "GlobalAI.h"
#include <set>

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif

std::set<IGlobalAI*> ais;

//Returnerar DLLens typ och version
int (APIENTRY GetGlobalAiVersion)()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

void (APIENTRY GetAiName)(char* name)
{
	strcpy(name,AI_NAME);
}

IGlobalAI* (APIENTRY GetNewAI)()
{
	CGlobalAI* ai=new CGlobalAI();
	ais.insert(ai);
	return ai;
}

void (APIENTRY ReleaseAI)(IGlobalAI* i)
{
	delete (CGlobalAI*)i;
	ais.erase(i);
}


#ifdef __cplusplus
}
#endif

