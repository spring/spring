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
#include "ExternalAI/Interface/SStaticGlobalData.h"
#include "ExternalAI/Interface/SSAILibrary.h"

#include <stdbool.h>	// bool, true, false
#include <string.h>	// strlen(), strcat(), strcpy()
#include <stdlib.h>	// malloc(), calloc(), free()

#define INTERFACE_PROPERTIES_FILE "interface.properties"

static const struct SStaticGlobalData* staticGlobalData = NULL;

EXPORT(int) initStatic(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		const struct SStaticGlobalData* _staticGlobalData) {

	bool success = false;


	// initialize C part of the interface
	staticGlobalData = _staticGlobalData;

	util_setMyInfo(infoSize, infoKeys, infoValues);

	const char* myShortName = util_getMyInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* myVersion = util_getMyInfo(AI_INTERFACE_PROPERTY_VERSION);

	static const int maxProps = 64;
	const char* propKeys[maxProps];
	const char* propValues[maxProps];
	int numProps = 0;

	// ### read the interface config file (optional) ###
	char* interfacePropFile_rel = util_allocStrCatFSPath(2,
				util_getDataDirVersioned(), INTERFACE_PROPERTIES_FILE);
	char interfacePropFile_abs[2048];
	bool interfacePropFile_found = util_findFile(
			staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
			interfacePropFile_rel, interfacePropFile_abs);
	free(interfacePropFile_rel);

	// ### read JVM options config file ###
	if (interfacePropFile_found) {
		numProps = util_parsePropertiesFile(interfacePropFile_abs,
				propKeys, propValues, maxProps);
		int p;
		for (p=0; p < numProps; ++p) {
			char* propValue_tmp = util_allocStrReplaceStr(propValues[p],
					"${home-dir}", util_getDataDirVersioned());
			free(propValues[p]);
			propValues[p] = propValue_tmp;
		}
	}

	// ### try to fetch the log-level from the properties ###
	int logLevel = SIMPLELOG_LEVEL_NORMAL;
	const char* logLevel_str =
			util_map_getValueByKey(numProps, propKeys, propValues,
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
			util_map_getValueByKey(numProps, propKeys, propValues,
			"log.useTimeStamps");
	if (useTimeStamps_str != NULL) {
		useTimeStamps = util_strToBool(useTimeStamps_str);
	}

	// ### init the log file ###
	char* logFile = util_allocStrCpy(
			util_map_getValueByKey(numProps, propKeys, propValues,
			"log.file"));
	if (logFile == NULL) {
		logFile = util_allocStrCatFSPath(3,
				util_getDataDirVersioned(), "log", MY_LOG_FILE);
	}
	char logDir[strlen(logFile) + 1];
	if (util_getParentDir(logFile, logDir)
			&& (util_fileExists(logDir) || util_makeDirRecursive(logDir))) {
		simpleLog_init(logFile, useTimeStamps, logLevel);
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed initializing log-file \"%s\"", logFile);
	}

	// log settings loaded from interface config file
	if (interfacePropFile_found) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "settings loaded from: %s",
				interfacePropFile_abs);
		int p;
		for (p=0; p < numProps; ++p) {
			simpleLog_logL(SIMPLELOG_LEVEL_FINE, "\t%i: %s = %s",
					p, propKeys[p], propValues[p]);
		}
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "settings NOT loaded from: %s",
				interfacePropFile_abs);
	}

	simpleLog_log("This is the log-file of the %s v%s AI Interface", myShortName, myVersion);
	simpleLog_log("Using data-directory (version specific): %s",
			util_getDataDirVersioned());
	simpleLog_log("Using data-directory (version-less): %s",
			util_getDataDirUnversioned());
	simpleLog_log("Using log file: %s", logFile);
	free(logFile);

	// initialize Java part of the interface
	success = java_initStatic(staticGlobalData);
	// load the JVM
	success = success && java_preloadJNIEnv();
	simpleLog_logL(SIMPLELOG_LEVEL_FINE, "initStatic java_preloadJNIEnv done.");

	return success ? 0 : -1;
}

EXPORT(int) releaseStatic() {

	bool success = false;

	// release Java part of the interface
	success = java_releaseStatic();
	success = success && java_unloadJNIEnv();

	// release C part of the interface
	free(util_getDataDirVersioned());
	free(util_getDataDirUnversioned());

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
static struct SSAILibrary* mySSAILibrary = NULL;



enum LevelOfSupport CALLING_CONV proxy_skirmishAI_getLevelOfSupportFor(
		int teamId,
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion) {

	return LOS_Unknown;
}

int CALLING_CONV proxy_skirmishAI_init(int teamId,
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		unsigned int optionsSize,
		const char** optionsKeys, const char** optionsValues) {

	int ret = -1;

//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 0);
	const char* className = util_map_getValueByKey(
			infoSize, infoKeys, infoValues,
			JAVA_SKIRMISH_AI_PROPERTY_CLASS_NAME);
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 1);

	if (className != NULL) {
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 2);
		ret = java_initSkirmishAIClass(infoSize, infoKeys, infoValues) ? 0 : 1;
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 3);
	}
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 4);
	bool ok = (ret == 0);
	if (ok) {
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "proxy_skirmishAI_init %u", 5);
		ret = java_skirmishAI_init(teamId,
				infoSize, infoKeys, infoValues,
				optionsSize, optionsKeys, optionsValues);
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



EXPORT(const struct SSAILibrary*) loadSkirmishAILibrary(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues) {

//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "loadSkirmishAILibrary %u", 0);
	if (mySSAILibrary == NULL) {
		mySSAILibrary =
				(struct SSAILibrary*) malloc(sizeof(struct SSAILibrary));

		mySSAILibrary->getLevelOfSupportFor =
				proxy_skirmishAI_getLevelOfSupportFor;
/*
		mySSAILibrary->getInfo = proxy_skirmishAI_getInfo;
		mySSAILibrary->getOptions = proxy_skirmishAI_getOptions;
*/
		mySSAILibrary->init = proxy_skirmishAI_init;
		mySSAILibrary->release = proxy_skirmishAI_release;
		mySSAILibrary->handleEvent = proxy_skirmishAI_handleEvent;
	}
//simpleLog_logL(SIMPLELOG_LEVEL_FINE, "loadSkirmishAILibrary %u", 10);

	return mySSAILibrary;
}
EXPORT(int) unloadSkirmishAILibrary(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues) {

	const char* className = util_map_getValueByKey(
			infoSize, infoKeys, infoValues,
			JAVA_SKIRMISH_AI_PROPERTY_CLASS_NAME);

	return java_releaseSkirmishAIClass(className) ? 0 : -1;
}
EXPORT(int) unloadAllSkirmishAILibraries() {
	return java_releaseAllSkirmishAIClasses() ? 0 : -1;
}
