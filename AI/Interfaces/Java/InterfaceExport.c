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
#include "Util.h"
#include "JavaBridge.h"
#include "Log.h"

#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SStaticGlobalData.h"

#include <stdbool.h>	// bool, true, false
#include <string.h>	// strlen(), strcat(), strcpy()
#include <stdlib.h>	// malloc(), calloc(), free()

static const struct SStaticGlobalData* staticGlobalData = NULL;

EXPORT(int) initStatic(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		const struct SStaticGlobalData* _staticGlobalData) {

	bool success = false;


	// initialize C part fo the interface
	staticGlobalData = _staticGlobalData;

	util_setMyInfo(infoSize, infoKeys, infoValues);

	const char* myShortName = util_getMyInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* myVersion = util_getMyInfo(AI_INTERFACE_PROPERTY_VERSION);
/*
	// example: "AI/Interfaces/Java"
	//const char* myDataDirRelative = AI_INTERFACES_DATA_DIR""sPS""MY_SHORT_NAME;
	char myDataDirRelative[128];
	strcpy(myDataDirRelative, AI_INTERFACES_DATA_DIR);
	strcat(myDataDirRelative, sPS""MY_SHORT_NAME);
	// example: "AI/Interfaces/Java/0.1"
	//const char* myDataDirVersRelative =
	//		AI_INTERFACES_DATA_DIR""sPS""MY_SHORT_NAME""sPS""MY_VERSION;
	char myDataDirVersRelative[128];
	strcpy(myDataDirVersRelative, AI_INTERFACES_DATA_DIR);
	strcat(myDataDirVersRelative, sPS""MY_SHORT_NAME""sPS""MY_VERSION);

	// "C:/Games/spring/AI/Interfaces/C"
	char* myDataDir = util_allocStr(1024);
	bool exists = util_findDir(staticGlobalData->dataDirs,
			staticGlobalData->numDataDirs, myDataDirRelative, myDataDir, true,
			true);
	if (!exists) {
		simpleLog_error(-1, "Failed to create "MY_NAME" data directory.");
	}

	// "C:/Games/spring/AI/Interfaces/C/0.1"
	char* myDataDirVers = util_allocStr(1024);
	exists = util_findDir(staticGlobalData->dataDirs,
			staticGlobalData->numDataDirs, myDataDirVersRelative, myDataDirVers,
			true, true);
	if (!exists) {
		simpleLog_error(-2, "Failed to create "MY_NAME" versioned data directory.");
	}
*/

	char* logFileName = util_allocStr(strlen(util_getDataDirVersioned())
			+ 1 + strlen(MY_LOG_FILE));
	logFileName[0]= '\0';
	logFileName = strcat(logFileName, util_getDataDirVersioned());
	logFileName = strcat(logFileName, sPS);
	logFileName = strcat(logFileName, MY_LOG_FILE);
	simpleLog_init(logFileName, true, true);
/*
	util_setDataDirs(myDataDirUnversioned, myDataDir);
*/

	simpleLog_log("This is the log-file of the %s v%s", myShortName, myVersion);
	simpleLog_log("Using data-directory (version specific): %s",
			util_getDataDirVersioned());
	simpleLog_log("Using data-directory (version-less): %s",
			util_getDataDirUnversioned());
	simpleLog_log("Using log file: %s", logFileName);
	free(logFileName);


	// initialize Java part of the interface
	success = java_initStatic(staticGlobalData);
	// load the JVM
	success = success && java_preloadJNIEnv();
	simpleLog_fine("initStatic java_preloadJNIEnv done.");

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

//simpleLog_fine("proxy_skirmishAI_init %u", 0);
	const char* className = util_map_getValueByKey(
			infoSize, infoKeys, infoValues,
			JAVA_SKIRMISH_AI_PROPERTY_CLASS_NAME);
//simpleLog_fine("proxy_skirmishAI_init %u", 1);

	if (className != NULL) {
//simpleLog_fine("proxy_skirmishAI_init %u", 2);
		ret = java_initSkirmishAIClass(className) ? 0 : 1;
//simpleLog_fine("proxy_skirmishAI_init %u", 3);
	}
//simpleLog_fine("proxy_skirmishAI_init %u", 4);
	bool ok = (ret == 0);
	if (ok) {
//simpleLog_fine("proxy_skirmishAI_init %u", 5);
		ret = java_skirmishAI_init(teamId,
				infoSize, infoKeys, infoValues,
				optionsSize, optionsKeys, optionsValues);
	}
//simpleLog_fine("proxy_skirmishAI_init %u", 6);

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

//simpleLog_fine("loadSkirmishAILibrary %u", 0);
	if (mySSAILibrary == NULL) {
		mySSAILibrary = (struct SSAILibrary*) malloc(sizeof(struct SSAILibrary));

		mySSAILibrary->getLevelOfSupportFor = proxy_skirmishAI_getLevelOfSupportFor;
/*
		mySSAILibrary->getInfo = proxy_skirmishAI_getInfo;
		mySSAILibrary->getOptions = proxy_skirmishAI_getOptions;
*/
		mySSAILibrary->init = proxy_skirmishAI_init;
		mySSAILibrary->release = proxy_skirmishAI_release;
		mySSAILibrary->handleEvent = proxy_skirmishAI_handleEvent;
	}
//simpleLog_fine("loadSkirmishAILibrary %u", 10);

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
