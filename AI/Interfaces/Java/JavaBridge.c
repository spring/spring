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
#include "StreflopBridge.h"
#include "CUtils/Util.h"
#include "CUtils/SimpleLog.h"

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

/**
 * Creates the Java classpath.
 * It will consist of the following:
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/AIInterface.jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/[*].jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/AIInterface.jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/jlib/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/jlib/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/SkirmishAI.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/SkirmishAI.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/jlib/[*].jar
 */
static bool java_createClassPath(char* classPathStr) {

	classPathStr[0] = '\0';

	unsigned int i, j;

	// the .jar files in the following list will be added to the classpath
	static const unsigned int MAX_ENTRIES = 128;
	char* classPath[MAX_ENTRIES];
	int unsigned classPath_size = 0;
	// the Java AI Interfaces file name
	classPath[classPath_size++] = util_dataDirs_allocFilePath(
			JAVA_AI_INTERFACE_LIBRARY_FILE_NAME, false);


	// the directories in the following list will be searched for .jar files
	// which then will be added to the classpath, plus they will be added
	// to the classpath directly, so you can keep .class files in there
	char* jarDirs[MAX_ENTRIES];
	int unsigned jarDirs_size = 0;
	jarDirs[jarDirs_size++] = util_dataDirs_allocDir(JAVA_LIBS_DIR, false);

// 	// search the individual jar files and add everything to the classpath
// 	for (i = 0; i < sizeJarFiles; ++i) {
// 		char* absoluteFilePath = util_allocStr(MAX_TEXT_LEN);
// 		bool found = util_findFile(
// 				staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
// 				jarFiles[i], absoluteFilePath);
// 		if (found) {
// 			if (i > 0) {
// 				STRCAT(classPath, ENTRY_DELIM);
// 			}
// 			STRCAT(classPath, absoluteFilePath);
// 		}
// 	}
	// add the jar dirs (for .class files) and all contained .jars recursively
	for (i=0; i < jarDirs_size; ++i) {
		if (util_fileExists(jarDirs[i])) {
			// add the dir directly
			classPath[classPath_size++] = util_allocStrCpy(jarDirs[i]);

			// add the contained jars recursively
			char* jarFiles[MAX_ENTRIES];
			unsigned int jarFiles_size = util_listFiles(jarDirs[i], ".jar",
					jarFiles, true, MAX_ENTRIES);
			for (j=0; j < jarFiles_size; ++j) {
				classPath[classPath_size++] = util_allocStrCatFSPath(2,
						jarDirs[i], jarFiles[j]);
			}
		}
		free(jarDirs[i]);
	}

	// concat the classpath entries
	STRCAT(classPathStr, classPath[0]);
	free(classPath[0]);
	for (i=1; i < classPath_size; ++i) {
		if (classPath[i] != NULL) {
			STRCAT(classPathStr, ENTRY_DELIM);
			STRCAT(classPathStr, classPath[i]);
			free(classPath[i]);
		}
	}

	return true;
}
/**
 * Creates the private Java classpath for a single AI.
 * It will consist of the following:
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/SkirmishAI.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/SkirmishAI.jar
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

	if (version != NULL) {
		jarFiles[sizeJarFiles++] = util_allocStrCatFSPath(4,
				SKIRMISH_AI_DATA_DIR, shortName, version, "SkirmishAI.jar");
	}
	jarFiles[sizeJarFiles++] = util_allocStrCatFSPath(3,
			SKIRMISH_AI_DATA_DIR, shortName, "SkirmishAI.jar");


	// the directories in the following list will be searched for .jar files
	// which then will be added to the classpath, plus they will be added
	// to the classpath directly, so you can keep .class files in there
	char* jarDirs[classPathParts_max];
	int unsigned sizeJarDirs = 0;

	if (version != NULL) {
		jarDirs[sizeJarDirs++] = util_allocStrCatFSPath(4,
			SKIRMISH_AI_DATA_DIR, shortName, version, JAVA_LIBS_DIR);
	}
	jarDirs[sizeJarDirs++] = util_allocStrCatFSPath(3,
			SKIRMISH_AI_DATA_DIR, shortName, JAVA_LIBS_DIR);


	// get the absolute paths of the jar files
	int i;
	for (i = 0; i < sizeJarFiles && classPathParts_num < classPathParts_max;
			++i) {
		char* absoluteFilePath = util_allocStr(MAX_TEXT_LEN);
		bool found = util_findFile(
				staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
				jarFiles[i], absoluteFilePath, false);
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
				classPathParts[classPathParts_num++] =
						util_allocStrCatFSPath(2, jarDirs[i], jarFileNames[j]);
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
	if (checkException(env, "Failed creating URL[].")) { return NULL; }
	int u;
	for (u = 0; u < cpp_num; ++u) {
		char* str_fileUrl = util_allocStr(strlen(FILE_URL_PREFIX)
				+ strlen(classPathParts[u]));
		STRCPY(str_fileUrl, FILE_URL_PREFIX);
		STRCAT(str_fileUrl, classPathParts[u]);
		jstring jstr_fileUrl = (*env)->NewStringUTF(env, str_fileUrl);
		if (checkException(env, "Failed creating Java String.")) { return NULL; }
		jobject jurl_fileUrl =
				(*env)->NewObject(env, g_cls_url, g_m_url_ctor, jstr_fileUrl);
		if (checkException(env, "Failed creating Java URL.")) { return NULL; }
		(*env)->SetObjectArrayElement(env, o_cppURLs, u, jurl_fileUrl);
		if (checkException(env, "Failed setting Java URL in array.")) { return NULL; }
	}

	o_jClsLoader = (*env)->NewObject(env, g_cls_urlClassLoader,
			g_m_urlClassLoader_ctor, o_cppURLs);
	if (checkException(env, "Failed creating class-loader.")) { return NULL; }
	o_jClsLoader = (*env)->NewGlobalRef(env, o_jClsLoader);
	if (checkException(env, "Failed to make class-loader a global reference.")) { return NULL; }

	return o_jClsLoader;
}

static bool java_createNativeLibsPath(char* libraryPath) {

	// create the Java library path (where native libs are searched)
	// consists of:

	// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/
	char* dd_r = util_dataDirs_allocDir("", false);
	if (dd_r == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Unable to find read-only data-dir: %s", dd_r);
	} else {
		STRCPY(libraryPath, dd_r);
		free(dd_r);
	}

	// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/lib/
	char* dd_lib_r = util_dataDirs_allocDir(NATIVE_LIBS_DIR, false);
	if (dd_lib_r == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_NORMAL,
				"Unable to find read-only libraries (optional) data-dir: %s",
				NATIVE_LIBS_DIR);
	} else {
		STRCAT(libraryPath, ENTRY_DELIM);
		STRCPY(libraryPath, dd_lib_r);
		free(dd_lib_r);
	}

	return true;
}
static bool java_createJavaVMInitArgs(struct JavaVMInitArgs* vm_args) {

	static const int maxProps = 64;
	const char* propKeys[maxProps];
	const char* propValues[maxProps];

	// ### read JVM options config file ###
	int numProps = 0;
	char* jvmPropFile = util_dataDirs_allocFilePath(JVM_PROPERTIES_FILE, false);
	if (jvmPropFile != NULL) {
		numProps = util_parsePropertiesFile(jvmPropFile,
				propKeys, propValues, maxProps);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"JVM: arguments loaded from: %s", jvmPropFile);
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"JVM: arguments NOT loaded from: %s", jvmPropFile);
	}

	// ### evaluate JNI version to use ###
	//unsigned long int jniVersion = JNI_VERSION_1_1;
	//unsigned long int jniVersion = JNI_VERSION_1_2;
	unsigned long int jniVersion = JNI_VERSION_1_4;
	//unsigned long int jniVersion = JNI_VERSION_1_6;
	if (jvmPropFile != NULL) {
		const char* jniVersionFromCfg =
				util_map_getValueByKey(numProps, propKeys, propValues,
				"jvm.jni.version");
		if (jniVersionFromCfg != NULL) {
			unsigned long int jniVersion_tmp =
					strtoul(jniVersionFromCfg, NULL, 16);
			if (jniVersion_tmp != 0) {
				jniVersion = jniVersion_tmp;
			}
		}
	}
	//else {
	//	//vm_args->version = JNI_VERSION_1_1;
	//	//vm_args->version = JNI_VERSION_1_2;
	//	vm_args->version = JNI_VERSION_1_4;
	//	//vm_args->version = JNI_VERSION_1_6;
	//}
	simpleLog_logL(SIMPLELOG_LEVEL_FINE, "JVM: JNI version: %#x", jniVersion);
	vm_args->version = jniVersion;

	// ### check if unrecognized JVM options should be ignored ###
	// set ignore unrecognized
	// if false, the JVM creation will fail if an
	// unknown or invalid option was specified
	bool ignoreUnrecognized = true;
	if (jvmPropFile != NULL) {
		const char* ignoreUnrecognizedFromCfg =
				util_map_getValueByKey(numProps, propKeys, propValues,
				"jvm.arguments.ignoreUnrecognized");
		if (ignoreUnrecognizedFromCfg != NULL
				&& !util_strToBool(ignoreUnrecognizedFromCfg)) {
			ignoreUnrecognized = false;
		}
	}
	if (ignoreUnrecognized) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"JVM: ignoring unrecognized options");
		vm_args->ignoreUnrecognized = JNI_TRUE;
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"JVM: NOT ignoring unrecognized options");
		vm_args->ignoreUnrecognized = JNI_FALSE;
	}

	// ### create the Java class-path option ###
	// create the java.class.path option ...
	// autogenerate it, and append the part from the jvm options file,
	// if it is specified there
	static const int classPath_max = 8 * 1024;
	char classPath[classPath_max];
	STRCPY(classPath, "-Djava.class.path=");
	char clsPath[classPath_max];
	// ..., autogenerate it ...
	if (!java_createClassPath(clsPath)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed creating Java class-path.");
		return false;
	}
	STRCAT(classPath, clsPath);
	if (jvmPropFile != NULL) {
		// ..., and append the part from the jvm options properties file,
		// if it is specified there
		// TODO: this will not work, as the key is "jvm.option.x",
		// and the value would be "-Djava.class.path=/patha:/pathb:..."
		// Adjust!!
		const char* clsPathFromCfg =
				util_map_getValueByKey(numProps, propKeys, propValues,
				"-Djava.class.path");
		if (clsPathFromCfg != NULL) {
			STRCAT(classPath, ENTRY_DELIM);
			STRCAT(classPath, clsPathFromCfg);
		}
	}

	// ### create the Java library-path option ###
	// create the java.library.path option ...
	// autogenerate it, and append the part from the jvm options file,
	// if it is specified there
	static const int libraryPath_max = 4 * 1024;
	char libraryPath[libraryPath_max];
	STRCPY(libraryPath, "-Djava.library.path=");
	char libPath[libraryPath_max];
	// ..., autogenerate it ...
	if (!java_createNativeLibsPath(libPath)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed creating Java library-path.");
		return false;
	}
	STRCAT(libraryPath, libPath);
	if (jvmPropFile != NULL) {
		// ..., and append the part from the jvm options properties file,
		// if it is specified there
		// TODO: this will not work, as the key is "jvm.option.x",
		// and the value would be "-Djava.class.path=/patha:/pathb:..."
		// Adjust!!
		const char* libPathFromCfg =
				util_map_getValueByKey(numProps, propKeys, propValues,
				"-Djava.library.path");
		if (libPathFromCfg != NULL) {
			STRCAT(libraryPath, ENTRY_DELIM);
			STRCAT(libraryPath, libPathFromCfg);
		}
	}

	// ### create and set all JVM options ###
	const char* strOptions[32];
	unsigned int op = 0;

	strOptions[op++] = classPath;
	strOptions[op++] = libraryPath;

	if (jvmPropFile != NULL) {
		int i;
		for (i=0; i < numProps; ++i) {
			if (strcmp(propKeys[i], "jvm.option.x") == 0) {
				strOptions[op++] = propValues[i];
			}
		}
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "JVM: using default options.");

		strOptions[op++] = "-Xms4M";
		strOptions[op++] = "-Xmx64M";
		strOptions[op++] = "-Xss512K";
		strOptions[op++] = "-Xoss400K";

#if defined JVM_LOGGING
		strOptions[op++] = "-Xcheck:jni";
		strOptions[op++] = "-verbose:jni";
		strOptions[op++] = "-XX:+UnlockDiagnosticVMOptions";
		strOptions[op++] = "-XX:+LogVMOutput";
#endif // defined JVM_LOGGING

#if defined JVM_DEBUGGING
		strOptions[op++] = "-Xdebug";
		strOptions[op++] = "-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address="JVM_DEBUG_PORT;
		// disable JIT (required for debugging under the classical VM)
		strOptions[op++] = "-Djava.compiler=NONE";
		// disable old JDB
		strOptions[op++] = "-Xnoagent";
#endif // defined JVM_DEBUGGING
	}

	const unsigned int numOptions = op;

	struct JavaVMOption* options = (struct JavaVMOption*)
			calloc(numOptions, sizeof(struct JavaVMOption));

	// fill strOptions into the JVM options
	simpleLog_logL(SIMPLELOG_LEVEL_FINE, "JVM: options (%i):", numOptions);
	unsigned int i;
	char* dd_rw = util_dataDirs_allocDir("", true);
	for (i = 0; i < numOptions; ++i) {
		options[i].optionString = util_allocStrReplaceStr(strOptions[i],
				"${home-dir}", dd_rw);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, options[i].optionString);
	}
	free(dd_rw);
	simpleLog_logL(SIMPLELOG_LEVEL_FINE, "");

	vm_args->options = options;
	vm_args->nOptions = numOptions;

	free(jvmPropFile);

	return true;
}

static JNIEnv* java_getJNIEnv() {

	if (g_jniEnv == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "Creating the JVM.");

		JNIEnv* env = NULL;
		JavaVM* jvm = NULL;
		struct JavaVMInitArgs vm_args;
		jint res;

		if (!java_createJavaVMInitArgs(&vm_args)) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed initializing JVM init-arguments.");
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

		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "creating JVM...");
		res = JNI_CreateJavaVM(&jvm, (void**) &env, &vm_args);
		unsigned int i;
		for (i = 0; i < vm_args.nOptions; ++i) {
			free(vm_args.options[i].optionString);
		}
		free(vm_args.options);
		ESTABLISH_SPRING_ENV
		if (res < 0) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Can't create Java VM, error code: %i", res);
			goto end;
		}

		res = (*jvm)->AttachCurrentThreadAsDaemon(jvm, (void**) &env, NULL);
		if (res < 0 || (*env)->ExceptionCheck(env)) {
			if ((*env)->ExceptionCheck(env)) {
				(*env)->ExceptionDescribe(env);
			}
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Can't Attach jvm to current thread,"
					" error code: %i", res);
			goto end;
		}

end:
		if (env == NULL || jvm == NULL || (*env)->ExceptionCheck(env)
				|| res != 0) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "!Failed creating JVM.");
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

	simpleLog_logL(SIMPLELOG_LEVEL_FINER, "Reattaching current thread...");
	jint res = (*g_jvm)->AttachCurrentThreadAsDaemon(g_jvm,
			(void**) &g_jniEnv, NULL);
	if (res < 0 || (*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
		if ((*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
			(*g_jniEnv)->ExceptionDescribe(g_jniEnv);
		}
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Can't Attach jvm to current thread,"
				" error code(2): %i", res);
	}

	return g_jniEnv;
}


bool java_unloadJNIEnv() {

	if (g_jniEnv != NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "Unloading the JVM...");

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
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed destroying the JVM, error code: %i", res);
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
	g_m_url_ctor = (*env)->GetMethodID(env,
			g_cls_url, "<init>", "(Ljava/lang/String;)V");
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
	g_m_urlClassLoader_ctor = (*env)->GetMethodID(env,
			g_cls_urlClassLoader, "<init>", "([Ljava/net/URL;)V");
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
	util_strReplaceChar(classNameP, '.', '/');

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
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Class not found \"%s\"", className);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// get AI constructor
	jmethodID m_ai_ctor = (*env)->GetMethodID(env, cls_ai, "<init>", "()V");
	if (m_ai_ctor == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"No-arg constructor not found for class: %s", className);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// instantiate AI
	jobject o_local_ai = (*env)->NewObject(env, cls_ai, m_ai_ctor);
	if (!o_local_ai || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"No-arg constructor not found for class: %s", className);
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
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed to make AI a global reference.");
		(*env)->ExceptionDescribe(env);
	} else {
		*o_ai = o_global_ai;
	}


	// get the AIs methods

	// init
	methods[MTH_INDEX_SKIRMISH_AI_INIT] = (*env)->GetMethodID(env, cls_ai,
			MTH_SKIRMISH_AI_INIT, SIG_SKIRMISH_AI_INIT);
	if (methods[MTH_INDEX_SKIRMISH_AI_INIT] == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Method not found: %s.%s%s", className,
				MTH_SKIRMISH_AI_INIT, SIG_SKIRMISH_AI_INIT);
		return false;
	}

	// release
	methods[MTH_INDEX_SKIRMISH_AI_RELEASE] = (*env)->GetMethodID(env, cls_ai,
			MTH_SKIRMISH_AI_RELEASE, SIG_SKIRMISH_AI_RELEASE);
	if (methods[MTH_INDEX_SKIRMISH_AI_RELEASE] == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Method not found: %s.%s%s", className,
				MTH_SKIRMISH_AI_RELEASE, SIG_SKIRMISH_AI_RELEASE);
		return false;
	}

	// handleEvent
	methods[MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT] = (*env)->GetMethodID(env,
			cls_ai, MTH_SKIRMISH_AI_HANDLE_EVENT, SIG_SKIRMISH_AI_HANDLE_EVENT);
	if (methods[MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT] == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Method not found: %s.%s%s", className,
				MTH_SKIRMISH_AI_HANDLE_EVENT, SIG_SKIRMISH_AI_HANDLE_EVENT);
		return false;
	}

	return true;
}


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
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"!Class loading failed for class: %s", className);
		}
	}

	return success;
}

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


int java_skirmishAI_init(int teamId,
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		unsigned int optionsSize,
		const char** optionsKeys, const char** optionsValues) {

	int res = -1;

	jmethodID mth = NULL;
	jobject o_ai = NULL;
	ESTABLISH_JAVA_ENV;
	bool success = java_getSkirmishAIAndMethod(teamId, &o_ai,
			MTH_INDEX_SKIRMISH_AI_INIT, &mth);

	JNIEnv* env = NULL;
	if (success) {
		env = java_getJNIEnv();
	}

	// create Java info Properties
	jobject o_infoProps = NULL;
	if (success) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"creating Java info Properties for init() ...");
		o_infoProps = java_createPropertiesFromCMap(env,
				infoSize, infoKeys, infoValues);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
		success = (o_infoProps != NULL);
	}

	// create Java options Properties
	jobject o_optionsProps = NULL;
	if (success) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"creating Java options Properties for init() ...");
		o_optionsProps = java_createPropertiesFromCMap(env,
				optionsSize, optionsKeys, optionsValues);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
		success = (o_optionsProps != NULL);
	}

	if (success) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"calling Java AI method init(teamId)...");
		res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId,
				o_infoProps, o_optionsProps);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
	}
	ESTABLISH_SPRING_ENV;

	return res;
}

int java_skirmishAI_release(int teamId) {

	int res = -1;

	jmethodID mth = NULL;
	jobject o_ai = NULL;
	bool success = java_getSkirmishAIAndMethod(teamId, &o_ai,
			MTH_INDEX_SKIRMISH_AI_RELEASE, &mth);

	if (success) {
		ESTABLISH_JAVA_ENV
		JNIEnv* env = java_getJNIEnv();
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"calling Java AI method release(teamId)...");
		res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
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
		jlong jniPointerToData = (jlong) ((intptr_t)data);
		// instantiate a JNA Pointer
		jobject jnaPointerToData = (*env)->NewObject(env, g_cls_jnaPointer,
				g_m_jnaPointer_ctor_long, jniPointerToData);
		if ((*env)->ExceptionCheck(env)) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"handleEvent: creating JNA pointer to data failed");
			(*env)->ExceptionDescribe(env);
			res = -3;
		}
		res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId, topic,
				jnaPointerToData);
		if ((*env)->ExceptionCheck(env)) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "handleEvent: call failed");
			(*env)->ExceptionDescribe(env);
			res = -2;
		}
		ESTABLISH_SPRING_ENV;
	}

	return res;
}
