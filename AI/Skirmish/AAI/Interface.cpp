//// -------------------------------------------------------------------------
//// AAI
////
//// A skirmish AI for the TA Spring engine.
//// Copyright Alexander Seizinger
////
//// Released under GPL license: see LICENSE.html for more information.
//// -------------------------------------------------------------------------
//
//#include "aidef.h"
//#include "Platform/Win/win32.h"
//#include "AAI.h"
//
///////////////////////////////////////////////////////////////////////////////
//
//std::set<IGlobalAI*> ais;
//
//
//SHARED_EXPORT int GetGlobalAiVersion()
//{
//	return GLOBAL_AI_INTERFACE_VERSION;
//}
//
//SHARED_EXPORT void GetAiName(char* name)
//{
//	STRCPY(name, AAI_VERSION);
//}
//
//SHARED_EXPORT IGlobalAI* GetNewAI()
//{
//	AAI* ai = new AAI();
//	ais.insert(ai);
//	return ai;
//}
//
//SHARED_EXPORT void ReleaseAI(IGlobalAI* i)
//{
//	ais.erase(i);
//	delete (AAI*)i;
//}
//
//SHARED_EXPORT int IsCInterface(void)
//{
//	return 0;
//}
//
//SHARED_EXPORT int IsLoadSupported()
//{
//	return 0;
//}
