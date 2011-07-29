/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InterfaceExport.h"

#include "Interface.h"


static CInterface* myInterface = NULL;

EXPORT(int) initStatic(int interfaceId, const struct SAIInterfaceCallback* callback) {

	if (myInterface == NULL) {
		myInterface = new CInterface(interfaceId, callback);
	}

	return 0; // signal: OK
}

EXPORT(int) releaseStatic() {

	delete myInterface;
	myInterface = NULL;

	return 0;
}

//EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
//		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
//	return myInterface->GetLevelOfSupportFor(engineVersion, engineAIInterfaceGeneratedVersion);
//}



EXPORT(const struct SSkirmishAILibrary*) loadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
) {
	return myInterface->LoadSkirmishAILibrary(shortName, version);
}

EXPORT(int) unloadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
) {
	return myInterface->UnloadSkirmishAILibrary(shortName, version);
}

EXPORT(int) unloadAllSkirmishAILibraries() {
	return myInterface->UnloadAllSkirmishAILibraries();
}
