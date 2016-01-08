/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <memory>

#include "InterfaceExport.h"
#include "Interface.h"

// win32 namespace pollution
#ifdef interface
#undef interface
#endif

static std::unique_ptr<CInterface> interface;

EXPORT(int) initStatic(int interfaceId, const SAIInterfaceCallback* callback) {
	if (interface.get() == nullptr)
		interface.reset(new CInterface(interfaceId, callback));

	return 0; // signal: OK
}

EXPORT(int) releaseStatic() {
	interface.reset();
	return 0;
}

//EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
//		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
//	return interface->GetLevelOfSupportFor(engineVersion, engineAIInterfaceGeneratedVersion);
//}



EXPORT(const SSkirmishAILibrary*) loadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
) {
	return interface->LoadSkirmishAILibrary(shortName, version);
}

EXPORT(int) unloadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
) {
	return interface->UnloadSkirmishAILibrary(shortName, version);
}

EXPORT(int) unloadAllSkirmishAILibraries() {
	return interface->UnloadAllSkirmishAILibraries();
}

