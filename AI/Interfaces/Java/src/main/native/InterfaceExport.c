/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InterfaceExport.h"

#include "InterfaceDefines.h"
#include "JavaBridge.h"

// generated at build time
#include "CallbackFunctionPointerBridge.h"

#include "CUtils/Util.h"
#include "CUtils/SimpleLog.h"

#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SAIInterfaceCallback.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"

#include <stdbool.h>  // bool, true, false
#include <string.h>   // strlen(), strcat(), strcpy()
#include <stdlib.h>   // malloc(), calloc(), free()

static int interfaceId = -1;
static const struct SAIInterfaceCallback* callback = NULL;

EXPORT(int) initStatic(int _interfaceId,
		const struct SAIInterfaceCallback* _callback) {

	simpleLog_initcallback(_interfaceId, "Java Interface", _callback->Log_logsl, LOG_LEVEL_INFO);

	bool success = false;

	// initialize C part of the interface
	interfaceId = _interfaceId;
	callback = _callback;

	const char* const myShortName = callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* const myVersion = callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_VERSION);
	if (myShortName == NULL || myVersion == NULL) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Couldn't fetch AI Name / Version \"%d\"", _interfaceId);
		return -1;
	}

	simpleLog_log("Initialized %s v%s AI Interface",
			myShortName, myVersion);

	// initialize Java part of the interface
	success = java_initStatic(interfaceId, callback);
	// load the JVM
	success = success && java_preloadJNIEnv();
	if (success) {
		simpleLog_logL(LOG_LEVEL_NOTICE, "Initialization successfull.");
	} else {
		simpleLog_logL(LOG_LEVEL_ERROR, "Initialization failed.");
	}

	return success ? 0 : -1;
}

EXPORT(int) releaseStatic() {

	bool success = false;

	// release Java part of the interface
	success = java_releaseStatic();
	success = success && java_unloadJNIEnv();

	// release C part of the interface
	util_finalize();

	return success ? 0 : -1;
}

EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {

/*
	if (strcmp(engineVersionString, ENGINE_VERSION_STRING) == 0 &&
			engineVersionNumber <= ENGINE_VERSION_NUMBER) {
		return LOS_Working;
	}

	return LOS_None;
*/
	return LOS_Unknown;
}

// skirmish AI methods
static struct SSkirmishAILibrary* mySSkirmishAILibrary = NULL;



enum LevelOfSupport CALLING_CONV proxy_skirmishAI_getLevelOfSupportFor(
		const char* aiShortName, const char* aiVersion,
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion) {

	return LOS_Unknown;
}

int CALLING_CONV proxy_skirmishAI_init(int skirmishAIId, const struct SSkirmishAICallback* aiCallback) {

	int ret = -1;

	const char* const shortName = aiCallback->SkirmishAI_Info_getValueByKey(
			skirmishAIId, SKIRMISH_AI_PROPERTY_SHORT_NAME);
	const char* const version = aiCallback->SkirmishAI_Info_getValueByKey(
			skirmishAIId, SKIRMISH_AI_PROPERTY_VERSION);
	const char* const className = aiCallback->SkirmishAI_Info_getValueByKey(
			skirmishAIId, JAVA_SKIRMISH_AI_PROPERTY_CLASS_NAME);

	if (className != NULL) {
		ret = java_initSkirmishAIClass(shortName, version, className, skirmishAIId) ? 0 : 1;
	}
	bool ok = (ret == 0);
	if (ok) {
		funcPntBrdg_addCallback(skirmishAIId, aiCallback);
		ret = java_skirmishAI_init(skirmishAIId, aiCallback);
	}

	return ret;
}

int CALLING_CONV proxy_skirmishAI_release(int skirmishAIId) {

	int failure = java_skirmishAI_release(skirmishAIId);
	funcPntBrdg_removeCallback(skirmishAIId);
	return failure;
}

int CALLING_CONV proxy_skirmishAI_handleEvent(
		int skirmishAIId, int topicId, const void* data) {
	return java_skirmishAI_handleEvent(skirmishAIId, topicId, data);
}


EXPORT(const struct SSkirmishAILibrary*) loadSkirmishAILibrary(
		const char* const shortName,
		const char* const version) {

	if (mySSkirmishAILibrary == NULL) {
		mySSkirmishAILibrary =
				(struct SSkirmishAILibrary*) malloc(sizeof(struct SSkirmishAILibrary));

		mySSkirmishAILibrary->getLevelOfSupportFor =
				&proxy_skirmishAI_getLevelOfSupportFor;
/*
		mySSkirmishAILibrary->getInfo = proxy_skirmishAI_getInfo;
		mySSkirmishAILibrary->getOptions = proxy_skirmishAI_getOptions;
*/
		mySSkirmishAILibrary->init = &proxy_skirmishAI_init;
		mySSkirmishAILibrary->release = &proxy_skirmishAI_release;
		mySSkirmishAILibrary->handleEvent = &proxy_skirmishAI_handleEvent;
	}

	return mySSkirmishAILibrary;
}
EXPORT(int) unloadSkirmishAILibrary(
		const char* const shortName,
		const char* const version) {

	const char* const className = callback->SkirmishAIs_Info_getValueByKey(
			interfaceId, shortName, version,
			JAVA_SKIRMISH_AI_PROPERTY_CLASS_NAME);

	return java_releaseSkirmishAIClass(className) ? 0 : -1;
}
EXPORT(int) unloadAllSkirmishAILibraries() {
	return java_releaseAllSkirmishAIClasses() ? 0 : -1;
}
