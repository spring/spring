#include "GlobalAI.h"

std::set<IGlobalAI*> ais;


SHARED_EXPORT int GetGlobalAiVersion() {
	return GLOBAL_AI_INTERFACE_VERSION;
}

SHARED_EXPORT void GetAiName(char* name) {
	strcpy(name, AI_VERSION);
}

SHARED_EXPORT IGlobalAI* GetNewAI() {
	if (ais.empty())
		creg::System::InitializeClasses();

	CGlobalAI* ai = new CGlobalAI();
	ais.insert(ai);
	return ai;
}

SHARED_EXPORT void ReleaseAI(IGlobalAI* i) {
	ais.erase(i);
	delete (CGlobalAI*) i;

	if (ais.empty())
		creg::System::FreeClasses();
}

SHARED_EXPORT int IsCInterface(void) {
	return 0;
}

SHARED_EXPORT int IsLoadSupported() {
	return 1;
}
