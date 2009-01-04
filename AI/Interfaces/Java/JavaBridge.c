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

#include "JavaBridge.h"

#include "InterfaceDefines.h"
#include "Util.h"
#include "StreflopBridge.h"
#include "Log.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SAICallback.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SSAILibrary.h"
#include "ExternalAI/Interface/AISEvents.h"
#include "ExternalAI/Interface/SStaticGlobalData.h"

#include <jni.h>

#include <string.h>	// strlen(), strcat(), strcpy()
#include <stdlib.h>	// malloc(), calloc(), free()
#include <inttypes.h> // intptr_t -> a signed int with the same size
                      // as a pointer (whether 32bit or 64bit)




static const struct SStaticGlobalData* staticGlobalData = NULL;
static unsigned int maxTeams = 0;
static unsigned int maxGroups = 0;
static unsigned int maxSkirmishImpls = 0;
static unsigned int sizeImpls = 0;

static const struct SAICallback** teamId_cCallback;
static jobject* teamId_jCallback;
static unsigned int* teamId_aiImplId;

static const char** aiImplId_className;
static jobject* aiImplId_instance;
static jmethodID** aiImplId_methods;
static jobject* aiImplId_classLoader;




static JNIEnv* g_jniEnv = NULL;
static JavaVM* g_jvm = NULL;

static jclass g_cls_url = NULL;
static jmethodID g_m_url_ctor = NULL;

static jclass g_cls_urlClassLoader = NULL;
static jmethodID g_m_urlClassLoader_ctor = NULL;
static jmethodID g_m_urlClassLoader_findClass = NULL;

static jclass g_cls_jnaPointer = NULL;
static jmethodID g_m_jnaPointer_ctor_long = NULL;

static jclass g_cls_props = NULL;
static jmethodID g_m_props_ctor = NULL;
static jmethodID g_m_props_setProperty = NULL;



static bool checkException(JNIEnv* env, const char* errorMsg) {

	if ((*env)->ExceptionCheck(env)) {
		simpleLog_log(errorMsg);
		(*env)->ExceptionDescribe(env);
		return true;
	}

	return false;
}


///**
// * Creates the Java classpath.
// * It will consist of the following:
// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/interface.jar
// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/
// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/[*].jar
// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/interface.jar
// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/jlib/
// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/jlib/[*].jar
// * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/ai.jar
// * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/
// * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/[*].jar
// * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/ai.jar
// * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/jlib/
// * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/jlib/[*].jar
// */
//static bool java_createClassPath(char* classPath) {
//
//	classPath[0] = '\0';
//
//	// Check which of the skirmish AIs to be used in the current game will
//	// possibly be using this interface
//	unsigned int i;
//	unsigned int j;
//	unsigned int applSkirmishAIs[staticGlobalData->maxTeams];
//	unsigned int sizeApplSkimrishAIs = 0;
//	const char* myShortName = util_getMyInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
//	for (i = 0; i < staticGlobalData->numSkirmishAIs; ++i) {
//		// find the interface shortName
//		const char* ai_intShortName = util_map_getValueByKey(
//				staticGlobalData->skirmishAIInfosSizes[i],
//				staticGlobalData->skirmishAIInfosKeys[i],
//				staticGlobalData->skirmishAIInfosValues[i],
//				SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME);
///*
//		const char* shortName_ai = util_map_getValueByKey(
//				staticGlobalData->skirmishAIInfosSizes[i],
//				staticGlobalData->skirmishAIInfosKeys[i],
//				staticGlobalData->skirmishAIInfosValues[i],
//				SKIRMISH_AI_PROPERTY_SHORT_NAME);
//*/
////simpleLog_log("shortName_ai: %s", shortName_ai);
////simpleLog_log("intShortName_ai: %s", intShortName);
//
//		// if the interface shortName was found, check for appliance
//		if (ai_intShortName != NULL && strcmp(ai_intShortName, myShortName) == 0) {
////simpleLog_log("applSkirmishAIs: %i", i);
//			applSkirmishAIs[sizeApplSkimrishAIs++] = i;
//		}
//	}
//
//
//	// the .jar files in the following list will be added to the classpath
//	static const unsigned int MAX_ENTRIES = 128;
//	static const unsigned int MAX_TEXT_LEN = 256;
//	char* jarFiles[MAX_ENTRIES];
//	int unsigned sizeJarFiles = 0;
//	// the Java AI Interfaces file name
//	{
//		//jarFiles[sizeJarFiles++] = AI_INTERFACES_DATA_DIR""sPS""MY_SHORT_NAME""sPS""MY_VERSION""sPS"interface.jar";
//		//jarFiles[sizeJarFiles++] = AI_INTERFACES_DATA_DIR""sPS""MY_SHORT_NAME""sPS"interface.jar";
//
//		jarFiles[sizeJarFiles] =
//				util_allocStr(strlen(util_getDataDirVersioned()) + strlen(sPS)
//						+ strlen(JAVA_AI_INTERFACE_LIBRARY_FILE_NAME));
//		STRCPY(jarFiles[sizeJarFiles], util_getDataDirVersioned());
//		STRCAT(jarFiles[sizeJarFiles], sPS);
//		STRCAT(jarFiles[sizeJarFiles], JAVA_AI_INTERFACE_LIBRARY_FILE_NAME);
////simpleLog_log("jarFiles[%i]: %s", sizeJarFiles, jarFiles[sizeJarFiles]);
//		sizeJarFiles++;
//
//		jarFiles[sizeJarFiles] =
//				util_allocStr(strlen(util_getDataDirUnversioned()) + strlen(sPS)
//						+ strlen(JAVA_AI_INTERFACE_LIBRARY_FILE_NAME));
//		STRCPY(jarFiles[sizeJarFiles], util_getDataDirUnversioned());
//		STRCAT(jarFiles[sizeJarFiles], sPS);
//		STRCAT(jarFiles[sizeJarFiles], JAVA_AI_INTERFACE_LIBRARY_FILE_NAME);
////simpleLog_log("jarFiles[%i]: %s", sizeJarFiles, jarFiles[sizeJarFiles]);
//		sizeJarFiles++;
//	}
//	// the file names of the Java AIs used during the current game
////simpleLog_log("sizeApplSkimrishAIs: %u", sizeApplSkimrishAIs);
//	for (i = 0; i < sizeApplSkimrishAIs; ++i) {
//		const char* shortName_ai = util_map_getValueByKey(
//				staticGlobalData->skirmishAIInfosSizes[applSkirmishAIs[i]],
//				staticGlobalData->skirmishAIInfosKeys[applSkirmishAIs[i]],
//				staticGlobalData->skirmishAIInfosValues[applSkirmishAIs[i]],
//				SKIRMISH_AI_PROPERTY_SHORT_NAME);
//		const char* version_ai = util_map_getValueByKey(
//				staticGlobalData->skirmishAIInfosSizes[applSkirmishAIs[i]],
//				staticGlobalData->skirmishAIInfosKeys[applSkirmishAIs[i]],
//				staticGlobalData->skirmishAIInfosValues[applSkirmishAIs[i]],
//				SKIRMISH_AI_PROPERTY_VERSION);
//
//		if (shortName_ai != NULL) {
////simpleLog_log("shortName_ai: %s", shortName_ai);
//			char* jarPath = util_allocStr(MAX_TEXT_LEN);
//			SNPRINTF(jarPath, MAX_TEXT_LEN, "%s"sPS"%s"sPS"ai.jar",
//					SKIRMISH_AI_DATA_DIR, shortName_ai);
//			jarFiles[sizeJarFiles++] = jarPath;
//
//			if (version_ai != NULL) {
////simpleLog_log("version_ai: %s", version_ai);
//				jarPath = util_allocStr(MAX_TEXT_LEN);
//				SNPRINTF(jarPath, MAX_TEXT_LEN, "%s"sPS"%s"sPS"%s"sPS"ai.jar",
//						SKIRMISH_AI_DATA_DIR, shortName_ai, version_ai);
//				jarFiles[sizeJarFiles++] = jarPath;
//			}
//		}
//	}
//
//
//	// the directories in the following list will be searched for .jar files
//	// which then will be added to the classpath, plus they will be added
//	// to the classpath directly, so you can keep .class files in there
//	char* jarDirs[MAX_ENTRIES];
//	int unsigned sizeJarDirs = 0;
//	jarDirs[sizeJarDirs++] = util_allocStrCpyCat(util_getDataDirVersioned(),
//			sPS"jlib");
//	jarDirs[sizeJarDirs++] = util_allocStrCpyCat(util_getDataDirUnversioned(),
//			sPS"jlib");
//	// the jlib dirs of the Java AIs used during the current game
//	for (i = 0; i < sizeApplSkimrishAIs; ++i) {
//		const char* shortName_ai = util_map_getValueByKey(
//				staticGlobalData->skirmishAIInfosSizes[applSkirmishAIs[i]],
//				staticGlobalData->skirmishAIInfosKeys[applSkirmishAIs[i]],
//				staticGlobalData->skirmishAIInfosValues[applSkirmishAIs[i]],
//				SKIRMISH_AI_PROPERTY_SHORT_NAME);
//		const char* version_ai = util_map_getValueByKey(
//				staticGlobalData->skirmishAIInfosSizes[applSkirmishAIs[i]],
//				staticGlobalData->skirmishAIInfosKeys[applSkirmishAIs[i]],
//				staticGlobalData->skirmishAIInfosValues[applSkirmishAIs[i]],
//				SKIRMISH_AI_PROPERTY_VERSION);
//
//		if (shortName_ai != NULL) {
//			char* jarDir = util_allocStr(MAX_TEXT_LEN);
//			SNPRINTF(jarDir, MAX_TEXT_LEN, "%s"sPS"%s"sPS"jlib",
//					SKIRMISH_AI_DATA_DIR, shortName_ai);
//			jarDirs[sizeJarDirs++] = jarDir;
//
//			if (version_ai != NULL) {
//				jarDir = util_allocStr(MAX_TEXT_LEN);
//				SNPRINTF(jarDir, MAX_TEXT_LEN, "%s"sPS"%s"sPS"%s"sPS"jlib",
//						SKIRMISH_AI_DATA_DIR, shortName_ai, version_ai);
//				jarDirs[sizeJarDirs++] = jarDir;
//			}
//		}
//	}
//
//
//	// searching the individual jar files and adding everything to the classpath
//	STRCAT(classPath, "-Djava.class.path=");
///*
//	// add the first jar file
//	if (sizeJarFiles > 0) {
//		if (util_fileExists(jarFiles[0])) {
//			char* absoluteFilePath = util_allocStr(MAX_TEXT_LEN);
//			bool found = util_findFile(
//					staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
//					jarFiles[0], absoluteFilePath);
//			if (found) {
//				STRCAT(classPath, absoluteFilePath);
//			}
//		}
//	}
//*/
//	// add the rest of the jar files
//	for (i = 0; i < sizeJarFiles; ++i) {
////simpleLog_log("jarFiles[%i]: %s", i, jarFiles[i]);
//		//if (util_fileExists(jarFiles[i])) {
//			char* absoluteFilePath = util_allocStr(MAX_TEXT_LEN);
//			bool found = util_findFile(
//					staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
//					jarFiles[i], absoluteFilePath);
////simpleLog_log("jarFiles[%i]: %i", i, found);
//			if (found) {
//				if (i > 0) {
//					STRCAT(classPath, ENTRY_DELIM);
//				}
//				STRCAT(classPath, absoluteFilePath);
//			}
//		//}
//	}
//	// add the jar dirs (for .class files)
//	for (i = 0; i < sizeJarDirs; ++i) {
//		char* absoluteDirPath = util_allocStr(MAX_TEXT_LEN);
//		bool found = util_findDir(
//				staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
//				jarDirs[i], absoluteDirPath, false, false);
//		free(jarDirs[i]);
//		if (found) {
//			STRCAT(classPath, ENTRY_DELIM);
//			STRCAT(classPath, absoluteDirPath);
//			jarDirs[i] = absoluteDirPath;
//		} else {
//			jarDirs[i] = NULL;
//		}
//	}
//	// add the jars in the dirs
//	for (i = 0; i < sizeJarDirs; ++i) {
//		if (jarDirs[i] != NULL) {
//			char* jarFileNames[MAX_ENTRIES];
//			unsigned int sizeJarFileNames = util_listFiles(jarDirs[i], ".jar",
//					jarFileNames, true, MAX_ENTRIES);
//			for (j = 0; j < sizeJarFileNames; ++j) {
//				STRCAT(classPath, ENTRY_DELIM);
//				STRCAT(classPath, jarDirs[i]);
//				STRCAT(classPath, sPS);
//				STRCAT(classPath, jarFileNames[j]);
//			}
//		}
//	}
//
//	return true;
//}
/**
 * Creates the Java classpath.
 * It will consist of the following:
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/interface.jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/[*].jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/interface.jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/jlib/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/jlib/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/ai.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/ai.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/jlib/[*].jar
 */
static bool java_createClassPath(char* classPath) {

	classPath[0] = '\0';

	// Check which of the skirmish AIs to be used in the current game will
	// possibly be using this interface
	unsigned int i;
	unsigned int j;
/*
	unsigned int applSkirmishAIs[staticGlobalData->maxTeams];
	unsigned int sizeApplSkimrishAIs = 0;
	const char* myShortName = util_getMyInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
	for (i = 0; i < staticGlobalData->numSkirmishAIs; ++i) {
		// find the interface shortName
		const char* ai_intShortName = util_map_getValueByKey(
				staticGlobalData->skirmishAIInfosSizes[i],
				staticGlobalData->skirmishAIInfosKeys[i],
				staticGlobalData->skirmishAIInfosValues[i],
				SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME);
//simpleLog_log("shortName_ai: %s", shortName_ai);
//simpleLog_log("intShortName_ai: %s", intShortName);

		// if the interface shortName was found, check for appliance
		if (ai_intShortName != NULL && strcmp(ai_intShortName, myShortName) == 0) {
//simpleLog_log("applSkirmishAIs: %i", i);
			applSkirmishAIs[sizeApplSkimrishAIs++] = i;
		}
	}
*/


	// the .jar files in the following list will be added to the classpath
	static const unsigned int MAX_ENTRIES = 128;
	static const unsigned int MAX_TEXT_LEN = 256;
	char* jarFiles[MAX_ENTRIES];
	int unsigned sizeJarFiles = 0;
	// the Java AI Interfaces file name
	{
		//jarFiles[sizeJarFiles++] = AI_INTERFACES_DATA_DIR""sPS""MY_SHORT_NAME""sPS""MY_VERSION""sPS"interface.jar";
		//jarFiles[sizeJarFiles++] = AI_INTERFACES_DATA_DIR""sPS""MY_SHORT_NAME""sPS"interface.jar";

		jarFiles[sizeJarFiles] =
				util_allocStr(strlen(util_getDataDirVersioned()) + strlen(sPS)
						+ strlen(JAVA_AI_INTERFACE_LIBRARY_FILE_NAME));
		STRCPY(jarFiles[sizeJarFiles], util_getDataDirVersioned());
		STRCAT(jarFiles[sizeJarFiles], sPS);
		STRCAT(jarFiles[sizeJarFiles], JAVA_AI_INTERFACE_LIBRARY_FILE_NAME);
//simpleLog_log("jarFiles[%i]: %s", sizeJarFiles, jarFiles[sizeJarFiles]);
		sizeJarFiles++;

		jarFiles[sizeJarFiles] =
				util_allocStr(strlen(util_getDataDirUnversioned()) + strlen(sPS)
						+ strlen(JAVA_AI_INTERFACE_LIBRARY_FILE_NAME));
		STRCPY(jarFiles[sizeJarFiles], util_getDataDirUnversioned());
		STRCAT(jarFiles[sizeJarFiles], sPS);
		STRCAT(jarFiles[sizeJarFiles], JAVA_AI_INTERFACE_LIBRARY_FILE_NAME);
//simpleLog_log("jarFiles[%i]: %s", sizeJarFiles, jarFiles[sizeJarFiles]);
		sizeJarFiles++;
	}
	// the file names of the Java AIs used during the current game
//simpleLog_log("sizeApplSkimrishAIs: %u", sizeApplSkimrishAIs);
/*
	for (i = 0; i < sizeApplSkimrishAIs; ++i) {
		const char* shortName_ai = util_map_getValueByKey(
				staticGlobalData->skirmishAIInfosSizes[applSkirmishAIs[i]],
				staticGlobalData->skirmishAIInfosKeys[applSkirmishAIs[i]],
				staticGlobalData->skirmishAIInfosValues[applSkirmishAIs[i]],
				SKIRMISH_AI_PROPERTY_SHORT_NAME);
		const char* version_ai = util_map_getValueByKey(
				staticGlobalData->skirmishAIInfosSizes[applSkirmishAIs[i]],
				staticGlobalData->skirmishAIInfosKeys[applSkirmishAIs[i]],
				staticGlobalData->skirmishAIInfosValues[applSkirmishAIs[i]],
				SKIRMISH_AI_PROPERTY_VERSION);

		if (shortName_ai != NULL) {
//simpleLog_log("shortName_ai: %s", shortName_ai);
			char* jarPath = util_allocStr(MAX_TEXT_LEN);
			SNPRINTF(jarPath, MAX_TEXT_LEN, "%s"sPS"%s"sPS"ai.jar",
					SKIRMISH_AI_DATA_DIR, shortName_ai);
			jarFiles[sizeJarFiles++] = jarPath;

			if (version_ai != NULL) {
//simpleLog_log("version_ai: %s", version_ai);
				jarPath = util_allocStr(MAX_TEXT_LEN);
				SNPRINTF(jarPath, MAX_TEXT_LEN, "%s"sPS"%s"sPS"%s"sPS"ai.jar",
						SKIRMISH_AI_DATA_DIR, shortName_ai, version_ai);
				jarFiles[sizeJarFiles++] = jarPath;
			}
		}
	}
*/


	// the directories in the following list will be searched for .jar files
	// which then will be added to the classpath, plus they will be added
	// to the classpath directly, so you can keep .class files in there
	char* jarDirs[MAX_ENTRIES];
	int unsigned sizeJarDirs = 0;
	jarDirs[sizeJarDirs++] = util_allocStrCpyCat(util_getDataDirVersioned(),
			sPS"jlib");
	jarDirs[sizeJarDirs++] = util_allocStrCpyCat(util_getDataDirUnversioned(),
			sPS"jlib");
	// the jlib dirs of the Java AIs used during the current game
/*
	for (i = 0; i < sizeApplSkimrishAIs; ++i) {
		const char* shortName_ai = util_map_getValueByKey(
				staticGlobalData->skirmishAIInfosSizes[applSkirmishAIs[i]],
				staticGlobalData->skirmishAIInfosKeys[applSkirmishAIs[i]],
				staticGlobalData->skirmishAIInfosValues[applSkirmishAIs[i]],
				SKIRMISH_AI_PROPERTY_SHORT_NAME);
		const char* version_ai = util_map_getValueByKey(
				staticGlobalData->skirmishAIInfosSizes[applSkirmishAIs[i]],
				staticGlobalData->skirmishAIInfosKeys[applSkirmishAIs[i]],
				staticGlobalData->skirmishAIInfosValues[applSkirmishAIs[i]],
				SKIRMISH_AI_PROPERTY_VERSION);

		if (shortName_ai != NULL) {
			char* jarDir = util_allocStr(MAX_TEXT_LEN);
			SNPRINTF(jarDir, MAX_TEXT_LEN, "%s"sPS"%s"sPS"jlib",
					SKIRMISH_AI_DATA_DIR, shortName_ai);
			jarDirs[sizeJarDirs++] = jarDir;

			if (version_ai != NULL) {
				jarDir = util_allocStr(MAX_TEXT_LEN);
				SNPRINTF(jarDir, MAX_TEXT_LEN, "%s"sPS"%s"sPS"%s"sPS"jlib",
						SKIRMISH_AI_DATA_DIR, shortName_ai, version_ai);
				jarDirs[sizeJarDirs++] = jarDir;
			}
		}
	}
*/


	// searching the individual jar files and adding everything to the classpath
	STRCAT(classPath, "-Djava.class.path=");
/*
	// add the first jar file
	if (sizeJarFiles > 0) {
		if (util_fileExists(jarFiles[0])) {
			char* absoluteFilePath = util_allocStr(MAX_TEXT_LEN);
			bool found = util_findFile(
					staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
					jarFiles[0], absoluteFilePath);
			if (found) {
				STRCAT(classPath, absoluteFilePath);
			}
		}
	}
*/
	// add the rest of the jar files
	for (i = 0; i < sizeJarFiles; ++i) {
//simpleLog_log("jarFiles[%i]: %s", i, jarFiles[i]);
		//if (util_fileExists(jarFiles[i])) {
			char* absoluteFilePath = util_allocStr(MAX_TEXT_LEN);
			bool found = util_findFile(
					staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
					jarFiles[i], absoluteFilePath);
//simpleLog_log("jarFiles[%i]: %i", i, found);
			if (found) {
				if (i > 0) {
					STRCAT(classPath, ENTRY_DELIM);
				}
				STRCAT(classPath, absoluteFilePath);
			}
		//}
	}
	// add the jar dirs (for .class files)
	for (i = 0; i < sizeJarDirs; ++i) {
		char* absoluteDirPath = util_allocStr(MAX_TEXT_LEN);
		bool found = util_findDir(
				staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
				jarDirs[i], absoluteDirPath, false, false);
		free(jarDirs[i]);
		if (found) {
			STRCAT(classPath, ENTRY_DELIM);
			STRCAT(classPath, absoluteDirPath);
			jarDirs[i] = absoluteDirPath;
		} else {
			jarDirs[i] = NULL;
		}
	}
	// add the jars in the dirs
	for (i = 0; i < sizeJarDirs; ++i) {
		if (jarDirs[i] != NULL) {
			char* jarFileNames[MAX_ENTRIES];
			unsigned int sizeJarFileNames = util_listFiles(jarDirs[i], ".jar",
					jarFileNames, true, MAX_ENTRIES);
			for (j = 0; j < sizeJarFileNames; ++j) {
				STRCAT(classPath, ENTRY_DELIM);
				STRCAT(classPath, jarDirs[i]);
				STRCAT(classPath, sPS);
				STRCAT(classPath, jarFileNames[j]);
			}
		}
	}

	return true;
}
/**
 * Creates the private Java classpath for a single AI.
 * It will consist of the following:
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/ai.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/ai.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/jlib/[*].jar
 */
static bool java_createAIClassPath(const char* shortName, const char* version,
		char** classPathParts, int classPathParts_max) {

	static const unsigned int MAX_TEXT_LEN = 256;

	int classPathParts_num = 0;

	// the .jar files in the following list will be added to the classpath
	char* jarFiles[classPathParts_max];
	int unsigned sizeJarFiles = 0;

	char* jarPath;
	if (version != NULL) {
		jarPath = util_allocStr(MAX_TEXT_LEN);
		SNPRINTF(jarPath, MAX_TEXT_LEN, "%s"sPS"%s"sPS"%s"sPS"ai.jar",
				SKIRMISH_AI_DATA_DIR, shortName, version);
		jarFiles[sizeJarFiles++] = jarPath;
	}

	jarPath = util_allocStr(MAX_TEXT_LEN);
	SNPRINTF(jarPath, MAX_TEXT_LEN, "%s"sPS"%s"sPS"ai.jar",
			SKIRMISH_AI_DATA_DIR, shortName);
	jarFiles[sizeJarFiles++] = jarPath;


	// the directories in the following list will be searched for .jar files
	// which then will be added to the classpath, plus they will be added
	// to the classpath directly, so you can keep .class files in there
	char* jarDirs[classPathParts_max];
	int unsigned sizeJarDirs = 0;

	char* jarDir;
	if (version != NULL) {
		jarDir = util_allocStr(MAX_TEXT_LEN);
		SNPRINTF(jarDir, MAX_TEXT_LEN, "%s"sPS"%s"sPS"%s"sPS"jlib",
				SKIRMISH_AI_DATA_DIR, shortName, version);
		jarDirs[sizeJarDirs++] = jarDir;
	}

	jarDir = util_allocStr(MAX_TEXT_LEN);
	SNPRINTF(jarDir, MAX_TEXT_LEN, "%s"sPS"%s"sPS"jlib",
			SKIRMISH_AI_DATA_DIR, shortName);
	jarDirs[sizeJarDirs++] = jarDir;


	// get the absolute paths of the jar files
	int i;
	for (i = 0; i < sizeJarFiles && classPathParts_num < classPathParts_max;
			++i) {
		char* absoluteFilePath = util_allocStr(MAX_TEXT_LEN);
		bool found = util_findFile(
				staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
				jarFiles[i], absoluteFilePath);
//simpleLog_log("jarFiles[%i]: %i", i, found);
		if (found) {
			classPathParts[classPathParts_num++] = absoluteFilePath;
		}
	}
	// add the jar dirs (for .class files)
	for (i = 0; i < sizeJarDirs && classPathParts_num < classPathParts_max; ++i)
	{
		char* absoluteDirPath = util_allocStr(MAX_TEXT_LEN);
		bool found = util_findDir(
				staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
				jarDirs[i], absoluteDirPath, false, false);
		if (found) {
			classPathParts[classPathParts_num++] = absoluteDirPath;
		} else {
			free(jarDirs[i]);
			jarDirs[i] = NULL;
		}
	}
	// add the jars in the dirs
	int j;
	for (i = 0; i < sizeJarDirs && classPathParts_num < classPathParts_max; ++i)
	{
		if (jarDirs[i] != NULL) {
			char* jarFileNames[classPathParts_max-classPathParts_num];
			unsigned int sizeJarFileNames = util_listFiles(jarDirs[i], ".jar",
					jarFileNames, true, classPathParts_max-classPathParts_num);
			for (j = 0; j < sizeJarFileNames; ++j) {
				classPathParts[classPathParts_num] = util_allocStr(
						strlen(jarDirs[i]) + 1 + strlen(jarFileNames[j]));
				STRCAT(classPathParts[classPathParts_num], jarDirs[i]);
				STRCAT(classPathParts[classPathParts_num], sPS);
				STRCAT(classPathParts[classPathParts_num], jarFileNames[j]);
			}
		}
		free(jarDirs[i]);
		jarDirs[i] = NULL;
	}

	return true;
}

static jobject java_createAIClassLoader(JNIEnv* env,
		const char* shortName, const char* version) {

	static const char* FILE_URL_PREFIX = "file://";

	jobject o_jClsLoader = NULL;

	int cpp_max = 512;
	char* classPathParts[cpp_max];
	int cpp_num =
			java_createAIClassPath(shortName, version, classPathParts, cpp_max);

	jobjectArray o_cppURLs = (*env)->NewObjectArray(env, cpp_num, g_cls_url,
			NULL);
	if (checkException(env, "!Failed creating URL[].")) { return NULL; }
	int u;
	for (u = 0; u < cpp_num; ++u) {
		char* str_fileUrl = util_allocStr(strlen(FILE_URL_PREFIX)
				+ strlen(classPathParts[u]));
		STRCPY(str_fileUrl, FILE_URL_PREFIX);
		STRCAT(str_fileUrl, classPathParts[u]);
		jstring jstr_fileUrl = (*env)->NewStringUTF(env, str_fileUrl);
		if (checkException(env, "!Failed creating Java String.")) { return NULL; }
		jobject jurl_fileUrl = (*env)->NewObject(env, g_cls_url, g_m_url_ctor, jstr_fileUrl);
		if (checkException(env, "!Failed creating Java URL.")) { return NULL; }
		(*env)->SetObjectArrayElement(env, o_cppURLs, u, jurl_fileUrl);
		if (checkException(env, "!Failed setting Java URL in array.")) { return NULL; }
	}

	o_jClsLoader = (*env)->NewObject(env, g_cls_urlClassLoader, g_m_urlClassLoader_ctor, o_cppURLs);
	if (checkException(env, "!Failed creating class-loader.")) { return NULL; }
	o_jClsLoader = (*env)->NewGlobalRef(env, o_jClsLoader);
	if (checkException(env, "!Failed to make class-loader a global reference.")) { return NULL; }

	return o_jClsLoader;
}

static bool java_createJavaVMInitArgs(struct JavaVMInitArgs* vm_args) {

	char classPath[16 * 1024];
	if (!java_createClassPath(classPath)) {
		simpleLog_error(-1, "!Failed creating Java classpath.");
		return false;
	}

	// create the Java library path (where native libs are searched)
	// consists of:
	// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/lib/
	// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/lib/
	char libraryPathPart1[strlen(util_getDataDirVersioned()) + strlen(sPS)
						+ strlen(JAVA_AI_INTERFACE_NATIVE_LIBS_DIR) + 1];
	STRCPY(libraryPathPart1, util_getDataDirVersioned());
	STRCAT(libraryPathPart1, sPS);
	STRCAT(libraryPathPart1, JAVA_AI_INTERFACE_NATIVE_LIBS_DIR);
	bool libraryPathPart1_exists = util_fileExists(libraryPathPart1);

	char libraryPathPart2[strlen(util_getDataDirUnversioned()) + strlen(sPS)
						+ strlen(JAVA_AI_INTERFACE_NATIVE_LIBS_DIR) + 1];
	STRCPY(libraryPathPart2, util_getDataDirUnversioned());
	STRCAT(libraryPathPart2, sPS);
	STRCAT(libraryPathPart2, JAVA_AI_INTERFACE_NATIVE_LIBS_DIR);
	bool libraryPathPart2_exists = util_fileExists(libraryPathPart2);

	char libraryPath[1024];
	STRCPY(libraryPath, "-Djava.library.path=");
	STRCAT(libraryPath, util_getDataDirVersioned());
	STRCAT(libraryPath, ENTRY_DELIM);
	STRCAT(libraryPath, util_getDataDirUnversioned());
	STRCAT(libraryPath, ENTRY_DELIM);
	if (libraryPathPart1_exists) {
		STRCAT(libraryPath, libraryPathPart1);
	}
	if (libraryPathPart1_exists && libraryPathPart2_exists) {
		STRCAT(libraryPath, ENTRY_DELIM);
	}
	if (libraryPathPart2_exists) {
		STRCAT(libraryPath, libraryPathPart2);
	}

	const char* strOptions[32];
	unsigned int op = 0;

	strOptions[op++] = classPath;
	strOptions[op++] = libraryPath;

	//if (GetOptionsFromConfigFile(&strOptions, CONFIG_FILE) < 0) {
		//simpleLog_fine("No JVM arguments loaded from config file: ___");
		simpleLog_fine(" -> using default options.");

		strOptions[op++] = "-Xms4M";
		strOptions[op++] = "-Xmx64M";
		strOptions[op++] = "-Xss512K";
		strOptions[op++] = "-Xoss400K";
		//strOptions[op++] = "-XX:+AlwaysRestoreFPU";
		//strOptions[op++] = "-Djava.util.logging.config.file="JAI_DIR"/logging.properties";

#if defined JVM_LOGGING
		simpleLog_fine("JVM logging enabled.");
		strOptions[op++] = "-Xcheck:jni";
		strOptions[op++] = "-verbose:jni";
		strOptions[op++] = "-XX:+UnlockDiagnosticVMOptions";
		strOptions[op++] = "-XX:+LogVMOutput";
		//strOptions[op++] = "-XX:LogFile=C:/javaLog.txt";
		//strOptions[op++] = "-XX:LogFile="JAI_DIR"/log/jvm-log.txt";
#endif // defined JVM_LOGGING

#if defined JVM_DEBUGGING
		simpleLog_fine("JVM debugging enabled.");
		strOptions[op++] = "-Xdebug";
		strOptions[op++] = "-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address="JVM_DEBUG_PORT;
		// disable JIT (required for debugging under the classical VM)
		strOptions[op++] = "-Djava.compiler=NONE";
		strOptions[op++] = "-Xnoagent"; // disables old JDB
#endif // defined JVM_DEBUGGING
	//}

	const unsigned int numOptions = op;

	struct JavaVMOption* options = (struct JavaVMOption*)
			calloc(numOptions, sizeof(struct JavaVMOption));

	// fill strOptions into the JVM options
	simpleLog_fine("JVM init options (size: %i):", numOptions);
	unsigned int i;
	for (i = 0; i < numOptions; ++i) {
		options[i].optionString = util_allocStrCpy(strOptions[i]);
		simpleLog_fine(strOptions[i]);
	}
	simpleLog_fine("");

	vm_args->version = JNI_VERSION_1_4;
	//vm_args->version = JNI_VERSION_1_1;
	//vm_args->version = JNI_VERSION_1_6;
	vm_args->options = options;
	vm_args->nOptions = numOptions;
	vm_args->ignoreUnrecognized = JNI_FALSE;

	return true;
}

static JNIEnv* java_getJNIEnv() {

	if (g_jniEnv == NULL) {
		simpleLog_fine("Creating the JVM.");

		JNIEnv* env = NULL;
		JavaVM* jvm = NULL;
		struct JavaVMInitArgs vm_args;
		jint res;

		if (!java_createJavaVMInitArgs(&vm_args)) {
			simpleLog_error(-1, "!Failed initializing JVM init-arguments.");
			goto end;
		}

		/*
				// looking for existing JVMs is problematic,
				// cause they could be initialized with other
				// JVM-arguments then we need
				simpleLog_log("looking for existing JVMs ...");
				jsize numJVMsFound = 0;

				// jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);
				// Returns all Java VMs that have been created.
				// Pointers to VMs are written in the buffer vmBuf,
				// in the order they are created.
				// At most bufLen number of entries will be written.
				// The total number of created VMs is returned in *nVMs.
				// Returns “0” on success; returns a negative number on failure.
				res = JNI_GetCreatedJavaVMs(&jvm, 1, &numJVMsFound);
				if (res < 0) {
					simpleLog_log("!Can't looking for Java VMs, error code: %i", res);
					goto end;
				}
				simpleLog_log("number of existing JVMs: %i", numJVMsFound);
		 */

		simpleLog_fine("creating JVM...");
		res = JNI_CreateJavaVM(&jvm, (void**) &env, &vm_args);
		unsigned int i;
		for (i = 0; i < vm_args.nOptions; ++i) {
			free(vm_args.options[i].optionString);
		}
		free(vm_args.options);
		ESTABLISH_SPRING_ENV
		if (res < 0) {
			simpleLog_error(res, "!Can't create Java VM, error code: %i", res);
			goto end;
		}

		res = (*jvm)->AttachCurrentThreadAsDaemon(jvm, (void**) &env, NULL);
		//res = AttachCurrentThread(jvm, (void**) &env, NULL);
		if (res < 0 || (*env)->ExceptionCheck(env)) {
			if ((*env)->ExceptionCheck(env)) {
				(*env)->ExceptionDescribe(env);
			}
			simpleLog_error(res, "!Can't Attach jvm to current thread,"
					" error code: %i", res);
			goto end;
		}

end:
		if (env == NULL || jvm == NULL || (*env)->ExceptionCheck(env)
				|| res != 0) {
			simpleLog_fine("!Failed creating JVM.");
			if (env != NULL && (*env)->ExceptionCheck(env)) {
				(*env)->ExceptionDescribe(env);
			}
			if (jvm != NULL) {
				res = (*jvm)->DestroyJavaVM(jvm);
			}
			g_jniEnv = NULL;
			g_jvm = NULL;
		} else {
			g_jniEnv = env;
			g_jvm = jvm;
		}
	}

	//simpleLog_fine("Reattaching current thread...");
	jint res = (*g_jvm)->AttachCurrentThreadAsDaemon(g_jvm,
			(void**) &g_jniEnv, NULL);
	//res = jvm->AttachCurrentThread((void**) & jniEnv, NULL);
	if (res < 0 || (*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
		if ((*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
			(*g_jniEnv)->ExceptionDescribe(g_jniEnv);
		}
		simpleLog_error(res, "!Can't Attach jvm to current thread,"
				" error code(2): %i", res);
	}

	return g_jniEnv;
}


bool java_unloadJNIEnv() {

	if (g_jniEnv != NULL) {
		simpleLog_fine("Unloading the JVM...");

		// We have to be the ONLY running thread (native and Java)
		// this may not help, but cant be bad
		jint res = (*g_jvm)->AttachCurrentThreadAsDaemon(g_jvm,
				(void**) &g_jniEnv, NULL);
		//res = jvm->AttachCurrentThread((void**) & jniEnv, NULL);
		if (res < 0 || (*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
			if ((*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
				(*g_jniEnv)->ExceptionDescribe(g_jniEnv);
			}
			simpleLog_log("!Can't Attach jvm to current thread,"
					" error code: %i", res);
			return false;
		}

		res = (*g_jvm)->DestroyJavaVM(g_jvm);
		if (res != 0) {
			simpleLog_log("!Failed destroying the JVM, error code: %i", res);
			return false;
		} else {
			g_jniEnv = NULL;
			g_jvm = NULL;
		}
	}


	return true;
}


bool java_preloadJNIEnv() {

	ESTABLISH_JAVA_ENV;
	JNIEnv* env = java_getJNIEnv();
	ESTABLISH_SPRING_ENV;

	return env != NULL;
}


bool java_initStatic(const struct SStaticGlobalData* _staticGlobalData) {

	staticGlobalData = _staticGlobalData;

	maxTeams = staticGlobalData->maxTeams;
	maxTeams = staticGlobalData->maxTeams;
	maxGroups = staticGlobalData->maxGroups;
	maxSkirmishImpls = maxTeams;
	sizeImpls = 0;

	teamId_aiImplId = (unsigned int*) calloc(maxTeams, sizeof(unsigned int));
	teamId_cCallback =(const struct SAICallback**)
			calloc(maxTeams, sizeof(struct SAICallback*));
	teamId_jCallback = (jobject*) calloc(maxTeams, sizeof(jobject));
	unsigned int t;
	for (t = 0; t < maxTeams; ++t) {
		teamId_aiImplId[t] = 0;
		teamId_cCallback[t] = NULL;
		teamId_jCallback[t] = NULL;
	}

	aiImplId_className = (const char**) calloc(maxSkirmishImpls, sizeof(char*));
	aiImplId_instance = (jobject*) calloc(maxSkirmishImpls, sizeof(jobject));
	aiImplId_methods = (jmethodID**)
			calloc(maxSkirmishImpls, sizeof(jmethodID*));
	aiImplId_classLoader = (jobject*) calloc(maxSkirmishImpls, sizeof(jobject));
	unsigned int impl;
	for (impl = 0; impl < maxSkirmishImpls; ++impl) {
		aiImplId_className[impl] = NULL;
		aiImplId_instance[impl] = NULL;
		aiImplId_methods[impl] = NULL;
		aiImplId_classLoader[impl] = NULL;
	}

/*
	ESTABLISH_JAVA_ENV;
	JNIEnv* env = java_getJNIEnv();
	callback_init(env);
	wrappSAICallback_init(env);
	ESTABLISH_SPRING_ENV;
*/

	return true;
}


static bool java_initURLClass(JNIEnv* env) {

	// get the URL class
	char fcCls[] = "java/net/URL";
	g_cls_url = (*env)->FindClass(env, fcCls);
	if (g_cls_url == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!Class not found \"%s\"", fcCls);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// make the URL class a global reference,
	// so it will not be garbage collected,
	// even after this method returned
	g_cls_url = (*env)->NewGlobalRef(env, g_cls_url);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_log("!Failed to make \"%s\" a global reference.", fcCls);
		(*env)->ExceptionDescribe(env);
		return false;
	}

	// get (String)	constructor
	g_m_url_ctor = (*env)->GetMethodID(env, g_cls_url, "<init>", "(Ljava/lang/String;)V");
	if (g_m_url_ctor == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!(String) constructor not found for class: %s", fcCls);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	return true;
}

static bool java_initURLClassLoaderClass(JNIEnv* env) {

	// get the URLClassLoader class
	char fcCls[] = "java/net/URLClassLoader";
	g_cls_urlClassLoader = (*env)->FindClass(env, fcCls);
	if (g_cls_urlClassLoader == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!Class not found \"%s\"", fcCls);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// make the URLClassLoader class a global reference,
	// so it will not be garbage collected,
	// even after this method returned
	g_cls_urlClassLoader = (*env)->NewGlobalRef(env, g_cls_urlClassLoader);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_log("!Failed to make \"%s\" a global reference.", fcCls);
		(*env)->ExceptionDescribe(env);
		return false;
	}

	// get (URL[])	constructor
	g_m_urlClassLoader_ctor = (*env)->GetMethodID(env, g_cls_urlClassLoader, "<init>",
			"([Ljava/net/URL;)V");
	if (g_m_urlClassLoader_ctor == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!(URL[]) constructor not found for class: %s", fcCls);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// get the findClass(String) method
	g_m_urlClassLoader_findClass = (*env)->GetMethodID(env,
			g_cls_urlClassLoader, "findClass",
			"(Ljava/lang/String;)Ljava/lang/Class;");
	if (g_m_urlClassLoader_findClass == NULL || (*env)->ExceptionCheck(env)) {
		g_m_urlClassLoader_findClass = NULL;
		simpleLog_log("!Method not found: %s.%s%s", fcCls,
				"findClass",
				"(Ljava/lang/String;)Ljava/lang/Class;");
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	return true;
}

static bool java_initPropertiesClass(JNIEnv* env) {

	// get the Properties class
	g_cls_props = (*env)->FindClass(env, "java/util/Properties");
	if (g_cls_props == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!Class not found \"%s\"", "java/util/Properties");
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// make the Properties class a global reference,
	// so it will not be garbage collected,
	// even after this method returned
	g_cls_props = (*env)->NewGlobalRef(env, g_cls_props);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_log("!Failed to make AI a global reference.");
		(*env)->ExceptionDescribe(env);
		return false;
	}

	// get no-arg constructor
	g_m_props_ctor = (*env)->GetMethodID(env, g_cls_props, "<init>", "()V");
	if (g_m_props_ctor == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!No-arg constructor not found for class: %s",
				"java/util/Properties");
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// get the setProperty() method
	g_m_props_setProperty = (*env)->GetMethodID(env, g_cls_props, "setProperty",
			"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
	if (g_m_props_setProperty == NULL || (*env)->ExceptionCheck(env)) {
		g_m_props_setProperty = NULL;
		simpleLog_log("!Method not found: %s.%s%s", "java/util/Properties",
				"setProperty",
				"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	return true;
}


static bool java_initPointerClass(JNIEnv* env) {

	if (g_cls_jnaPointer == NULL) {
		// get the Pointer class
		g_cls_jnaPointer = (*env)->FindClass(env, "com/sun/jna/Pointer");
		if (g_cls_jnaPointer == NULL || (*env)->ExceptionCheck(env)) {
			simpleLog_log("!Class not found \"%s\"", "com/sun/jna/Pointer");
			if ((*env)->ExceptionCheck(env)) {
				(*env)->ExceptionDescribe(env);
				return false;
			}
		}

		// make the Pointer class a global reference,
		// so it will not be garbage collected,
		// even after this method returned
		g_cls_jnaPointer = (*env)->NewGlobalRef(env, g_cls_jnaPointer);
		if ((*env)->ExceptionCheck(env)) {
			simpleLog_log("!Failed to make Pointer a global reference.");
			(*env)->ExceptionDescribe(env);
			return false;
		}

		// get native pointer constructor
		g_m_jnaPointer_ctor_long =
				(*env)->GetMethodID(env, g_cls_jnaPointer, "<init>", "(J)V");
		if (g_m_jnaPointer_ctor_long == NULL || (*env)->ExceptionCheck(env)) {
			simpleLog_log("!long constructor not found for class: %s",
					"com/sun/jna/Pointer");
			if ((*env)->ExceptionCheck(env)) {
				(*env)->ExceptionDescribe(env);
				return false;
			}
		}
	}

	return true;
}

static jobject java_createPropertiesInstance(JNIEnv* env) {

	jobject o_props = NULL;

	// initialize the properties class if not yet done
	if (g_m_props_setProperty == NULL) {
		if (!java_initPropertiesClass(env)) {
			return NULL;
		}
	}

	// create a Properties instance
	o_props = (*env)->NewObject(env, g_cls_props, g_m_props_ctor);
	if (o_props == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!Failed creating Properties instance");
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
	}

	return o_props;
}

static jobject java_createPropertiesFromCMap(JNIEnv* env,
		unsigned int size,
		const char** keys, const char** values) {

	jobject o_props = java_createPropertiesInstance(env);
	if (o_props == NULL) {
		return o_props;
	}

	// fill the Java Properties instance with the info keys and values
	unsigned int op;
	for (op=0; op < size; op++) {
		jstring jstr_key = (*env)->NewStringUTF(env, keys[op]);
		jstring jstr_value = (*env)->NewStringUTF(env, values[op]);
		(*env)->CallObjectMethod(env, o_props, g_m_props_setProperty, jstr_key,
				jstr_value);
		if ((*env)->ExceptionCheck(env)) {
			simpleLog_log("!Failed adding property");
			if ((*env)->ExceptionCheck(env)) {
				(*env)->ExceptionDescribe(env);
			}
			return NULL;
		}
	}

	return o_props;
}


bool java_releaseStatic() {

	unsigned int impl;
	for (impl = 0; impl < maxSkirmishImpls; ++impl) {
		if (aiImplId_methods[impl] != NULL) {
			free(aiImplId_methods[impl]);
			aiImplId_methods[impl] = NULL;
		}
	}

	free(teamId_aiImplId);

	free(aiImplId_className);
	free(aiImplId_instance);
	free(aiImplId_methods);
	free(aiImplId_classLoader);

	return true;
}

bool java_getSkirmishAIAndMethod(unsigned int teamId, jobject* o_ai,
		unsigned int methodIndex, jmethodID* mth) {

	bool success = false;

	unsigned int implId = teamId_aiImplId[teamId];
	*o_ai = aiImplId_instance[implId];
	*mth = aiImplId_methods[implId][methodIndex];
	success = (*mth) != NULL;

	return success;
}


/**
 * Instantiates an instance of the class specified className.
 *
 * @param	className	fully qualified name of a Java clas that implements
 *						interface com.clan_sy.spring.ai.AI, eg:
 *						"com.myai.AI"
 * @param	aiInstance	where the AI instance will be stored
 * @param	methods		where the method IDs of the AI will be stored
 */
static bool java_loadSkirmishAI(JNIEnv* env,
		const char* shortName, const char* version, const char* className,
		jobject* o_ai, jmethodID methods[MTHS_SIZE_SKIRMISH_AI],
		jobject* o_aiClassLoader) {

	// convert className from "com.myai.AI" to "com/myai/AI"
	char classNameP[strlen(className)+1];
	STRCPY(classNameP, className);
	util_strReplace(classNameP, '.', '/');

	// get the AIs private class-loader
	jobject o_global_aiClassLoader =
			java_createAIClassLoader(env, shortName, version);
	if (o_global_aiClassLoader == NULL) { return false; }
	*o_aiClassLoader = o_global_aiClassLoader;

	// get the AI class
	//jclass cls_ai = (*env)->FindClass(env, classNameP);
	jstring jstr_className = (*env)->NewStringUTF(env, className);
	jclass cls_ai = (*env)->CallObjectMethod(env, o_global_aiClassLoader,
			g_m_urlClassLoader_findClass, jstr_className);
	if (cls_ai == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!Class not found \"%s\"", className);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// get AI constructor
	jmethodID m_ai_ctor = (*env)->GetMethodID(env, cls_ai, "<init>", "()V");
	if (m_ai_ctor == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!No-arg constructor not found for class: %s", className);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// instantiate AI
	//jobject o_local_ai = (*env)->CallStaticObjectMethod(env, cls_ai, m_ai_ctor);
	jobject o_local_ai = (*env)->NewObject(env, cls_ai, m_ai_ctor);
	if (!o_local_ai || (*env)->ExceptionCheck(env)) {
		simpleLog_log("!No-arg constructor not found for class: %s", className);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// make the AI a global reference,
	// so it will not be garbage collected,
	// even after this method returned
	jobject o_global_ai = (*env)->NewGlobalRef(env, o_local_ai);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_log("!Failed to make AI a global reference.");
		(*env)->ExceptionDescribe(env);
	} else {
		*o_ai = o_global_ai;
	}


	// get the AIs methods

	// init
	methods[MTH_INDEX_SKIRMISH_AI_INIT] = (*env)->GetMethodID(env, cls_ai,
			MTH_SKIRMISH_AI_INIT, SIG_SKIRMISH_AI_INIT);
	if (methods[MTH_INDEX_SKIRMISH_AI_INIT] == NULL) {
		simpleLog_log("!Method not found: %s.%s%s", className,
				MTH_SKIRMISH_AI_INIT, SIG_SKIRMISH_AI_INIT);
		return false;
	}

	// release
	methods[MTH_INDEX_SKIRMISH_AI_RELEASE] = (*env)->GetMethodID(env, cls_ai,
			MTH_SKIRMISH_AI_RELEASE, SIG_SKIRMISH_AI_RELEASE);
	if (methods[MTH_INDEX_SKIRMISH_AI_RELEASE] == NULL) {
		simpleLog_log("!Method not found: %s.%s%s", className,
				MTH_SKIRMISH_AI_RELEASE, SIG_SKIRMISH_AI_RELEASE);
		return false;
	}

	// handleEvent
	methods[MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT] = (*env)->GetMethodID(env,
			cls_ai, MTH_SKIRMISH_AI_HANDLE_EVENT, SIG_SKIRMISH_AI_HANDLE_EVENT);
	if (methods[MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT] == NULL) {
		simpleLog_log("!Method not found: %s.%s%s", className,
				MTH_SKIRMISH_AI_HANDLE_EVENT, SIG_SKIRMISH_AI_HANDLE_EVENT);
		return false;
	}

	return true;
}


/**
 * Instantiates an instance of the specified className.
 *
 * @param	className	fully qualified name of a Java clas that implements
 *						interface com.clan_sy.spring.ai.AI, eg:
 *						"com.myai.AI"
 * @param	aiInstance	where the AI instance will be stored
 * @param	methods		where the method IDs of the AI will be stored
 */
bool java_initSkirmishAIClass(unsigned int infoSize,
		const char** infoKeys, const char** infoValues) {

	bool success = false;

	const char* shortName =
			util_map_getValueByKey(infoSize, infoKeys, infoValues,
			SKIRMISH_AI_PROPERTY_SHORT_NAME);
	const char* version =
			util_map_getValueByKey(infoSize, infoKeys, infoValues,
			SKIRMISH_AI_PROPERTY_VERSION);
	const char* className =
			util_map_getValueByKey(infoSize, infoKeys, infoValues,
			JAVA_SKIRMISH_AI_PROPERTY_CLASS_NAME);

	// see if an AI for className is instantiated already
	unsigned int implId;
	for (implId = 0; implId < sizeImpls; ++implId) {
		if (strcmp(aiImplId_className[implId], className) == 0) {
			break;
		}
	}

	// instantiate AI (if needed)
	if (aiImplId_className[implId] == NULL) {
		ESTABLISH_JAVA_ENV;
		JNIEnv* env = java_getJNIEnv();

		if (g_cls_url == NULL) {
			java_initURLClass(env);
		}
		if (g_cls_urlClassLoader == NULL) {
			java_initURLClassLoaderClass(env);
		}
		if (g_cls_jnaPointer == NULL) {
			java_initPointerClass(env);
		}

		aiImplId_methods[implId] = (jmethodID*) calloc(MTHS_SIZE_SKIRMISH_AI,
				sizeof(jmethodID));
		success = java_loadSkirmishAI(env, shortName, version, className,
				&(aiImplId_instance[implId]), aiImplId_methods[implId],
				&(aiImplId_classLoader[implId]));
		ESTABLISH_SPRING_ENV;
		if (success) {
			aiImplId_className[implId] = util_allocStrCpy(className);
		} else {
			simpleLog_error(-1, "!Class loading failed for class: %s",
					className);
		}
	}

	return success;
}
/**
 * Release an instance of the specified className.
 *
 * @param	className	fully qualified name of a Java clas that implements
 *						interface com.clan_sy.spring.ai.AI, eg:
 *						"com.myai.AI"
 * @param	aiInstance	where the AI instance will be stored
 * @param	methods		where the method IDs of the AI will be stored
 */
bool java_releaseSkirmishAIClass(const char* className) {

	bool success = false;

	// see if an AI for className is instantiated
	unsigned int implId;
	for (implId = 0; implId < sizeImpls; ++implId) {
		if (strcmp(aiImplId_className[implId], className) == 0) {
			break;
		}
	}

	// release AI (if needed)
	if (aiImplId_className[implId] != NULL) {
		ESTABLISH_JAVA_ENV;
		JNIEnv* env = java_getJNIEnv();

		// delete the AI class-loader global reference,
		// so it will be garbage collected
		(*env)->DeleteGlobalRef(env, aiImplId_classLoader[implId]);
		success = !((*env)->ExceptionCheck(env));
		if (!success) {
			simpleLog_log(
					"!Failed to delete AI class-loader global reference.");
			(*env)->ExceptionDescribe(env);
		}

		// delete the AI global reference,
		// so it will be garbage collected
		(*env)->DeleteGlobalRef(env, aiImplId_instance[implId]);
		success = !((*env)->ExceptionCheck(env));
		if (!success) {
			simpleLog_log("!Failed to delete AI global reference.");
			(*env)->ExceptionDescribe(env);
		}
		ESTABLISH_SPRING_ENV;

		if (success) {
			free(aiImplId_classLoader[implId]);
			aiImplId_classLoader[implId] = NULL;

			free(aiImplId_instance[implId]);
			aiImplId_instance[implId] = NULL;

			free(aiImplId_methods[implId]);
			aiImplId_methods[implId] = NULL;

			free(aiImplId_className[implId]);
			aiImplId_className[implId] = NULL;
		}
	}

	return success;
}
bool java_releaseAllSkirmishAIClasses() {

	bool success = true;

	const char* className;
	unsigned int implId;
	for (implId = 0; implId < sizeImpls; ++implId) {
		className = aiImplId_className[implId];
		if (className != NULL) {
			success = success && java_releaseSkirmishAIClass(className);
		}
	}

	return success;
}


const struct SAICallback* java_getSkirmishAICCallback(int teamId) {
	return teamId_cCallback[teamId];
}

/*
static jobject java_toJavaAICallback(JNIEnv* env, int teamId,
		const struct SAICallback* cCallback) {

	jobject jCallback = NULL;

	jclass cls_jClb = (*env)->FindClass(env, CLS_AI_CALLBACK);
	jCallback = (*env)->AllocObject(env, cls_jClb);
	jCallback = (*env)->NewGlobalRef(env, jCallback);

	teamId_cCallback[teamId] = cCallback;
	teamId_jCallback[teamId] = jCallback;

	return jCallback;
}
*/


/**
 * Instantiates an instance of the class specified className.
 *
 * @param	className	fully qualified name of a Java clas that implements
 *						interface com.clan_sy.spring.ai.AI, eg:
 *						"com.myai.AI"
 * @param	aiInstance	where the AI instance will be stored
 * @param	methods		where the method IDs of the AI will be stored
 */
int java_skirmishAI_init(int teamId,
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		unsigned int optionsSize,
		const char** optionsKeys, const char** optionsValues) {

	int res = -1;

	jmethodID mth = NULL;
	jobject o_ai = NULL;
	ESTABLISH_JAVA_ENV;
	bool success = java_getSkirmishAIAndMethod(teamId, &o_ai, MTH_INDEX_SKIRMISH_AI_INIT, &mth);

	JNIEnv* env = NULL;
	if (success) {
		env = java_getJNIEnv();
	}

//simpleLog_fine("java_skirmishAI_init 1");
	// create Java info Properties
	jobject o_infoProps = NULL;
	if (success) {
		simpleLog_fine("creating Java info Properties for init() ...");
		o_infoProps = java_createPropertiesFromCMap(env, infoSize, infoKeys, infoValues);
		simpleLog_fine("done.");
		success = (o_infoProps != NULL);
	}

	// create Java options Properties
	jobject o_optionsProps = NULL;
	if (success) {
		simpleLog_fine("creating Java options Properties for init() ...");
		o_optionsProps = java_createPropertiesFromCMap(env, optionsSize, optionsKeys, optionsValues);
		simpleLog_fine("done.");
		success = (o_optionsProps != NULL);
	}

	if (success) {
		simpleLog_fine("calling Java AI method init(teamId)...");
		res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId,
				o_infoProps, o_optionsProps);
		simpleLog_fine("done.");
	}
	ESTABLISH_SPRING_ENV;

	return res;
}

int java_skirmishAI_release(int teamId) {

	int res = -1;

	jmethodID mth = NULL;
	jobject o_ai = NULL;
	bool success = java_getSkirmishAIAndMethod(teamId, &o_ai, MTH_INDEX_SKIRMISH_AI_RELEASE, &mth);

	if (success) {
		ESTABLISH_JAVA_ENV
		JNIEnv* env = java_getJNIEnv();
		simpleLog_fine("calling Java AI method release(teamId)...");
		res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId);
		simpleLog_fine("done.");
		ESTABLISH_SPRING_ENV
	}

	return res;
}

int java_skirmishAI_handleEvent(int teamId, int topic, const void* data) {

	int res = -1;

	jmethodID mth = NULL;
	jobject o_ai = NULL;
	bool success = java_getSkirmishAIAndMethod(teamId, &o_ai,
			MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT, &mth);

	if (success) {
		ESTABLISH_JAVA_ENV;
		JNIEnv* env = java_getJNIEnv();
		//simpleLog_fine("calling Java AI method handleEvent(teamId, evt)...");
/*
if (topic == EVENT_INIT) {
	simpleLog_fine("EVENT_INIT...");
	const struct SInitEvent* initEvt = (const struct SInitEvent*) data;
	simpleLog_fine("check: is Clb_Game_getCurrentFrame initialized...");
	if (initEvt->callback->Clb_Game_getCurrentFrame == NULL) {
		simpleLog_fine("Game_getCurrentFrame is NULL");
	} else {
		simpleLog_fine("Game_getCurrentFrame is NOT NULL");
	}
}
*/
		//jobject evt = java_toJavaAIEvent(env, teamId, topic, data);
		//res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId, evt);
		//res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId, topic, data);
		//jlong jniPointerToData = (jlong) data;
		//jlong jniPointerToData = (jlong) ((uintptr_t)data & PAGEOFFSET);
		jlong jniPointerToData = (jlong) ((intptr_t)data);
		// instantiate a JNA Pointer
		jobject jnaPointerToData = (*env)->NewObject(env, g_cls_jnaPointer,
				g_m_jnaPointer_ctor_long, jniPointerToData);
		if ((*env)->ExceptionCheck(env)) {
			simpleLog_log("!handleEvent: creating JNA pointer to data failed");
			(*env)->ExceptionDescribe(env);
			res = -3;
		}
		res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId, topic,
				jnaPointerToData);
		if ((*env)->ExceptionCheck(env)) {
			simpleLog_log("!handleEvent() call failed");
			(*env)->ExceptionDescribe(env);
			res = -2;
		}
		//simpleLog_fine("done.");
		ESTABLISH_SPRING_ENV;
	}

	return res;
}
