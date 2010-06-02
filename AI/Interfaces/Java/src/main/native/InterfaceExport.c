/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "InterfaceExport.h"

#include "InterfaceDefines.h"
#include "JavaBridge.h"
#include "CUtils/Util.h"
#include "CUtils/SimpleLog.h"

#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SAIInterfaceCallback.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"

#include <stdbool.h>  // bool, true, false
#include <string.h>   // strlen(), strcat(), strcpy()
#include <stdlib.h>   // malloc(), calloc(), free()

#define INTERFACE_PROPERTIES_FILE "interface.properties"

static int interfaceId = -1;
static const struct SAIInterfaceCallback* callback = NULL;

EXPORT(int) initStatic(int _interfaceId,
		const struct SAIInterfaceCallback* _callback) {

	bool success = false;

	// initialize C part of the interface
	interfaceId = _interfaceId;
	callback = _callback;

	const char* const myShortName = callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* const myVersion = callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_VERSION);

	static const int maxProps = 64;
	const char* propKeys[maxProps];
	char* propValues[maxProps];
	int numProps = 0;

	// ### read the interface config file (optional) ###
	static const unsigned int propFilePath_sizeMax = 1024;
	char propFilePath[propFilePath_sizeMax];
	// eg: "~/.spring/AI/Interfaces/Java/${INTERFACE_PROPERTIES_FILE}"
	bool propFileFetched = callback->DataDirs_locatePath(interfaceId,
			propFilePath, propFilePath_sizeMax,
			INTERFACE_PROPERTIES_FILE, false, false, false, false);
	if (!propFileFetched) {
		// if the version specific file does not exist,
		// try to get the common one
		propFileFetched = callback->DataDirs_locatePath(interfaceId,
				propFilePath, propFilePath_sizeMax,
				INTERFACE_PROPERTIES_FILE, false, false, false, true);
	}
	if (propFileFetched) {
		numProps = util_parsePropertiesFile(propFilePath,
				propKeys, (const char**)propValues, maxProps);

		static const unsigned int ddw_sizeMax = 1024;
		char ddw[ddw_sizeMax];
		// eg: "~/.spring/AI/Interfaces/Java/${INTERFACE_PROPERTIES_FILE}"
		bool ddwFetched = callback->DataDirs_locatePath(interfaceId,
				ddw, ddw_sizeMax,
				"", true, true, true, false);
		if (!ddwFetched) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed locating writeable data-dir \"%s\"", ddw);
		}
		int p;
		for (p=0; p < numProps; ++p) {
			char* propValue_tmp = util_allocStrReplaceStr(propValues[p],
					"${home-dir}", ddw);
// 			char* propValue_tmp = util_allocStrReplaceStr(propValues[p],
// 					"${home-dir}/", "");
			free(propValues[p]);
			propValues[p] = propValue_tmp;
		}
	}

	// ### try to fetch the log-level from the properties ###
	int logLevel = SIMPLELOG_LEVEL_NORMAL;
	const char* logLevel_str =
			util_map_getValueByKey(numProps, propKeys, (const char**)propValues,
			"log.level");
	if (logLevel_str != NULL) {
		int logLevel_tmp = atoi(logLevel_str);
		if (logLevel_tmp >= SIMPLELOG_LEVEL_ERROR
				&& logLevel_tmp <= SIMPLELOG_LEVEL_FINEST) {
			logLevel = logLevel_tmp;
		}
	}

	// ### try to fetch whether to use time-stamps from the properties ###
	bool useTimeStamps = true;
	const char* useTimeStamps_str =
			util_map_getValueByKey(numProps, propKeys, (const char**)propValues,
			"log.useTimeStamps");
	if (useTimeStamps_str != NULL) {
		useTimeStamps = util_strToBool(useTimeStamps_str);
	}

	// ### init the log file ###
	char* logFile = util_allocStrCpy(
			util_map_getValueByKey(numProps, propKeys, (const char**)propValues,
			"log.file"));
	if (logFile == NULL) {
		logFile = util_allocStrCatFSPath(2, "log", MY_LOG_FILE);
	}

	static const unsigned int logFilePath_sizeMax = 1024;
	char logFilePath[logFilePath_sizeMax];
	// eg: "~/.spring/AI/Interfaces/Java/${INTERFACE_PROPERTIES_FILE}"
	bool logFileFetched = callback->DataDirs_locatePath(interfaceId,
			logFilePath, logFilePath_sizeMax,
			logFile, true, true, false, false);

	if (logFileFetched) {
		simpleLog_init(logFilePath, useTimeStamps, logLevel, false);
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed initializing log-file \"%s\"", logFileFetched);
	}

	// log settings loaded from interface config file
	if (propFileFetched) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "settings loaded from: %s",
				propFilePath);
		int p;
		for (p=0; p < numProps; ++p) {
			simpleLog_logL(SIMPLELOG_LEVEL_FINE, "\t%i: %s = %s",
					p, propKeys[p], propValues[p]);
		}
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "settings NOT loaded from: %s",
				propFilePath);
	}

	simpleLog_log("This is the log-file of the %s v%s AI Interface",
			myShortName, myVersion);
	simpleLog_log("Using read/write data-directory: %s",
			callback->DataDirs_getWriteableDir(interfaceId));
	simpleLog_log("Using log file: %s", propFilePath);

	FREE(logFile);

	// initialize Java part of the interface
	success = java_initStatic(interfaceId, callback);
	// load the JVM
	success = success && java_preloadJNIEnv();
	if (success) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "Initialization successfull.");
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Initialization failed.");
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
		int teamId,
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion) {

	return LOS_Unknown;
}

int CALLING_CONV proxy_skirmishAI_init(int teamId, const struct SSkirmishAICallback* aiCallback) {

	int ret = -1;

//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 0);
	const char* const shortName = aiCallback->Clb_SkirmishAI_Info_getValueByKey(teamId,
			SKIRMISH_AI_PROPERTY_SHORT_NAME);
	const char* const version = aiCallback->Clb_SkirmishAI_Info_getValueByKey(teamId,
			SKIRMISH_AI_PROPERTY_VERSION);
	const char* const className = aiCallback->Clb_SkirmishAI_Info_getValueByKey(teamId,
			JAVA_SKIRMISH_AI_PROPERTY_CLASS_NAME);
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 1);

	if (className != NULL) {
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 2);
		ret = java_initSkirmishAIClass(shortName, version, className, teamId) ? 0 : 1;
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 3);
	}
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 4);
	bool ok = (ret == 0);
	if (ok) {
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 5);
		ret = java_skirmishAI_init(teamId, aiCallback);
	}
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 6);

	return ret;
}

int CALLING_CONV proxy_skirmishAI_release(int teamId) {
	return java_skirmishAI_release(teamId);
}

int CALLING_CONV proxy_skirmishAI_handleEvent(
		int teamId, int topic, const void* data) {
	return java_skirmishAI_handleEvent(teamId, topic, data);
}


EXPORT(const struct SSkirmishAILibrary*) loadSkirmishAILibrary(
		const char* const shortName,
		const char* const version) {

//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "loadSkirmishAILibrary %u", 0);
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
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "loadSkirmishAILibrary %u", 10);

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
