//	Class: -
//	Filename: dummyai.cpp
//	Author:	Stefan Johansson
//	Description:	Implementation file for testai.dll dll file.
//					Implements an AI for spring.
// Dont modify this file
/////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "TestAI.h"
#include "globalai.h"
#include <set>

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif

std::set<IGlobalAI*> ais;

//Returnerar DLLens typ och version
int (WINAPI GetGlobalAiVersion)(){
	return GLOBAL_AI_INTERFACE_VERSION;
}

void (WINAPI GetAiName)(char* name){
	strcpy(name,AI_NAME);
}

IGlobalAI* (WINAPI GetNewAI)(){
	CGlobalAI* ai=new CGlobalAI(ais.size());
	ais.insert(ai);
	return ai;
}

void (WINAPI ReleaseAI)(IGlobalAI* i){
	delete (CGlobalAI*)i;
	ais.erase(i);
}


#ifdef __cplusplus
}
#endif

