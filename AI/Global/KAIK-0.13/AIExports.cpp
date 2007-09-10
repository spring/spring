#include "GlobalAI.h"

std::set<IGlobalAI*> ais;


DLL_EXPORT int GetGlobalAiVersion() {
	return GLOBAL_AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name) {
	strcpy(name, AI_VERSION);
}

DLL_EXPORT IGlobalAI* GetNewAI() {
	#ifndef USE_CREG
	if (ais.empty())
		creg::System::InitializeClasses();
	#endif

	CGlobalAI* ai = new CGlobalAI();
	ais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(IGlobalAI* i) {
	ais.erase(i);
	delete (CGlobalAI*) i;

	#ifndef USE_CREG
	if (ais.empty())
		creg::System::FreeClasses();
	#endif
}

DLL_EXPORT int IsCInterface(void) {
	return 0;
}

#ifndef USE_CREG
DLL_EXPORT int IsLoadSupported() {
	return 1;
}
#endif
