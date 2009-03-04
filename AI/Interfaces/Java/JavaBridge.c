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
#include "CUtils/Util.h"
#include "CUtils/SimpleLog.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SSAILibrary.h"
#include "ExternalAI/Interface/AISEvents.h"
#include "ExternalAI/Interface/SAIInterfaceCallback.h"

#include <jni.h>

#include <string.h>	// strlen(), strcat(), strcpy()
#include <stdlib.h>	// malloc(), calloc(), free()
#include <inttypes.h> // intptr_t -> a signed int with the same size
                      // as a pointer (whether 32bit or 64bit)

#define ESTABLISH_SPRING_ENV util_resetEngineEnv();
// The JVM sets the environment it wants automatically
#define ESTABLISH_JAVA_ENV

static int interfaceId = -1;
static const struct SAIInterfaceCallback* callback = NULL;
static unsigned int maxTeams = 0;
static unsigned int maxSkirmishImpls = 0;
static unsigned int sizeImpls = 0;

static unsigned int* teamId_aiImplId;

static char** aiImplId_className;
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

static jclass g_cls_aiCallback = NULL;
static jmethodID g_m_aiCallback_getInstance = NULL;



static bool checkException(JNIEnv* env, const char* const errorMsg) {

	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, errorMsg);
		(*env)->ExceptionDescribe(env);
		return true;
	}

	return false;
}

static jclass java_findClass(JNIEnv* env, const char* const className) {

	jclass res = NULL;

	res = (*env)->FindClass(env, className);
	if (res == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Class not found: \"%s\"", className);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		res = NULL;
	}

	return res;
}
static jobject java_makeGlobalRef(JNIEnv* env, jobject localObject) {

	jobject res = NULL;

	// make the local class a global reference,
	// so it will not be garbage collected,
	// even after this method returned
	// only if expricitly deleted with DeleteGlobalRef
	res = (*env)->NewGlobalRef(env, localObject);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Failed to make a global reference.");
		(*env)->ExceptionDescribe(env);
		res = NULL;
	}

	return res;
}
static void java_deleteGlobalRef(JNIEnv* env, jobject globalObject) {

	// delete the AI class-loader global reference,
	// so it will be garbage collected
	(*env)->DeleteGlobalRef(env, globalObject);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed to delete global reference.");
		(*env)->ExceptionDescribe(env);
	}
}
static jmethodID java_getMethodID(JNIEnv* env, jclass cls,
		const char* const name, const char* const signature) {

	jmethodID res = NULL;

	res = (*env)->GetMethodID(env, cls, name, signature);
	if (res == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Method not found: %s(%s)",
				name, signature);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		res = NULL;
	}

	return res;
}
static jmethodID java_getStaticMethodID(JNIEnv* env, jclass cls,
		const char* const name, const char* const signature) {

	jmethodID res = NULL;

	res = (*env)->GetStaticMethodID(env, cls, name, signature);
	if (res == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Method not found: %s(%s)",
				name, signature);
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		res = NULL;
	}

	return res;
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

	// the .jar files in the following list will be added to the classpath
	static const unsigned int classPath_sizeMax = 128;
	char* classPath[classPath_sizeMax];
	int unsigned classPath_size = 0;

	//static const unsigned int path_sizeMax = 2048;

	// the Java AI Interfaces file name
	classPath[classPath_size++] = callback->DataDirs_allocatePath(interfaceId,
			JAVA_AI_INTERFACE_LIBRARY_FILE_NAME,
			false, false, false);

	// the directories in the following list will be searched for .jar files
	// which then will be added to the classpath, plus they will be added
	// to the classpath directly, so you can keep .class files in there
	static const unsigned int jarDirs_sizeMax = 128;
	char* jarDirs[jarDirs_sizeMax];
	int unsigned jarDirs_size = 0;
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2,
			callback->DataDirs_getConfigDir(interfaceId), JAVA_LIBS_DIR);

	// add the jar dirs (for .class files) and all contained .jars recursively
	unsigned int jd, jf;
	for (jd=0; jd < jarDirs_size && classPath_size < classPath_sizeMax; ++jd) {
		if (util_fileExists(jarDirs[jd])) {
			// add the dir directly
			classPath[classPath_size++] = util_allocStrCpy(jarDirs[jd]);

			// add the contained jars recursively
			static const unsigned int jarFiles_sizeMax = 128;
			char* jarFiles[jarFiles_sizeMax];
			unsigned int jarFiles_size = util_listFiles(jarDirs[jd], ".jar",
					jarFiles, true, jarFiles_sizeMax);
			for (jf=0; jf < jarFiles_size && classPath_size < classPath_sizeMax;
					++jf) {
				classPath[classPath_size++] = util_allocStrCatFSPath(2,
						jarDirs[jd], jarFiles[jf]);
			}
		}
		free(jarDirs[jd]);
		jarDirs[jd] = NULL;
	}

	// concat the classpath entries
	if (classPath[0] != NULL) {
		STRCAT(classPathStr, classPath[0]);
		free(classPath[0]);
		classPath[0] = NULL;
	}
	unsigned int cp;
	for (cp=1; cp < classPath_size; ++cp) {
		if (classPath[cp] != NULL) {
			STRCAT(classPathStr, ENTRY_DELIM);
			STRCAT(classPathStr, classPath[cp]);
			free(classPath[cp]);
			classPath[cp] = NULL;
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

//	static const unsigned int MAX_TEXT_LEN = 256;

	int classPathParts_num = 0;

	// the .jar files in the following list will be added to the classpath
	char* jarFiles[classPathParts_max];
	int unsigned sizeJarFiles = 0;

	const char* const skirmDD =
			callback->SkirmishAIs_Info_getValueByKey(interfaceId,
			shortName, version,
			SKIRMISH_AI_PROPERTY_DATA_DIR);
	if (skirmDD == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Retrieving the data-dir of Skirmish AI %s-%s failed.",
				shortName, version);
	}
	jarFiles[sizeJarFiles++] = util_allocStrCatFSPath(2,
			skirmDD, "SkirmishAI.jar");


	// the directories in the following list will be searched for .jar files
	// which then will be added to the classpath, plus they will be added
	// to the classpath directly, so you can keep .class files in there
	char* jarDirs[classPathParts_max];
	int unsigned sizeJarDirs = 0;

	jarDirs[sizeJarDirs++] = util_allocStrCatFSPath(2,
			skirmDD, JAVA_LIBS_DIR);


	// get the absolute paths of the jar files
	int i;
	for (i = 0; i < sizeJarFiles && classPathParts_num < classPathParts_max;
			++i) {
//		char* absoluteFilePath = util_allocStr(MAX_TEXT_LEN);
//		bool found = util_findFile(
//				staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
//				jarFiles[i], absoluteFilePath, false);
// //simpleLog_log("jarFiles[%i]: %i", i, found);
//		if (found) {
//			classPathParts[classPathParts_num++] = absoluteFilePath;
			classPathParts[classPathParts_num++] = jarFiles[i];
//		}
	}
// 	// add the jar dirs (for .class files)
// 	for (i = 0; i < sizeJarDirs && classPathParts_num < classPathParts_max; ++i)
// 	{
// 		char* absoluteDirPath = util_allocStr(MAX_TEXT_LEN);
// 		bool found = util_findDir(
// 				staticGlobalData->dataDirs, staticGlobalData->numDataDirs,
// 				jarDirs[i], absoluteDirPath, false, false);
//		if (found) {
//			classPathParts[classPathParts_num++] = absoluteDirPath;
//		classPathParts[classPathParts_num++] = jarDirs[i];
// 		} else {
// 			free(jarDirs[i]);
// 			jarDirs[i] = NULL;
// 		}
//	}
//	// add the jars in the dirs
	int j;
	for (i = 0; i < sizeJarDirs && classPathParts_num < classPathParts_max; ++i)
	{
		if (jarDirs[i] != NULL) {
			// add the jar dir (for .class files)
			classPathParts[classPathParts_num++] = jarDirs[i];

			// add the jars in the dir
			char* jarFileNames[classPathParts_max-classPathParts_num];
			unsigned int sizeJarFileNames = util_listFiles(jarDirs[i], ".jar",
					jarFileNames, true, classPathParts_max-classPathParts_num);
			for (j = 0; j < sizeJarFileNames && classPathParts_num < classPathParts_max; ++j) {
				classPathParts[classPathParts_num++] =
						util_allocStrCatFSPath(2, jarDirs[i], jarFileNames[j]);
			}
		}
// 		free(jarDirs[i]);
// 		jarDirs[i] = NULL;
	}

	return true;
}

static jobject java_createAIClassLoader(JNIEnv* env,
		const char* shortName, const char* version) {

#ifdef _WIN32
	static const char* FILE_URL_PREFIX = "file:///";
#else // _WIN32
	static const char* FILE_URL_PREFIX = "file://";
#endif // _WIN32

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
		#ifdef _WIN32
		// we can not use windows path separators in file URLs
		util_strReplaceChar(classPathParts[u], '\\', '/');
		#endif

		char* str_fileUrl = util_allocStrCat(2, FILE_URL_PREFIX, classPathParts[u]);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"Skirmish AI %s %s class-path part %i: %s",
				shortName, version, u, str_fileUrl);
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
	const char* const dd_r =
			callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_DATA_DIR);
	if (dd_r == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Unable to find read-only data-dir.");
		return false;
	} else {
		STRCPY(libraryPath, dd_r);
	}

	// * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/lib/
	char* dd_lib_r = callback->DataDirs_allocatePath(interfaceId,
			NATIVE_LIBS_DIR, false, false, true);
	if (dd_lib_r == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_NORMAL,
				"Unable to find read-only native libs data-dir (optional): %s",
				NATIVE_LIBS_DIR);
	} else {
		STRCAT(libraryPath, ENTRY_DELIM);
		STRCPY(libraryPath, dd_lib_r);
		free(dd_lib_r);
		dd_lib_r = NULL;
	}

	return true;
}
static bool java_createJavaVMInitArgs(struct JavaVMInitArgs* vm_args) {

	static const int maxProps = 64;
	const char* propKeys[maxProps];
	const char* propValues[maxProps];

	// ### read JVM options config file ###
	int numProps = 0;
	char* jvmPropFile = callback->DataDirs_allocatePath(interfaceId,
			JVM_PROPERTIES_FILE, false, false, false);
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
		// TODO: FIXME: this will not work, as the key is "jvm.option.x",
		// and the value would be "-Djava.class.path=/patha:/pathb:..."
		// NOTE: Possibly already fixed, see below!
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
		// TODO: FIXME: this will not work, as the key is "jvm.option.x",
		// and the value would be "-Djava.class.path=/patha:/pathb:..."
		// NOTE: Possibly already fixed, see below!
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
	char* dd_rw = callback->DataDirs_allocatePath(interfaceId,
			"", true, true, true);
	for (i = 0; i < numOptions; ++i) {
		options[i].optionString = util_allocStrReplaceStr(strOptions[i],
				"${home-dir}", dd_rw);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, options[i].optionString);
	}
	free(dd_rw);
	dd_rw = NULL;
	simpleLog_logL(SIMPLELOG_LEVEL_FINE, "");

	vm_args->options = options;
	vm_args->nOptions = numOptions;

	free(jvmPropFile);
	jvmPropFile = NULL;

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
					simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
							"Can not look for Java VMs, error code: %i", res);
					goto end;
				}
				simpleLog_log("number of existing JVMs: %i", numJVMsFound);
		 */

		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "creating JVM...");
		res = JNI_CreateJavaVM(&jvm, (void**) &env, &vm_args);
		size_t i;
		for (i = 0; i < vm_args.nOptions; ++i) {
			free(vm_args.options[i].optionString);
			vm_args.options[i].optionString = NULL;
		}
		free(vm_args.options);
		vm_args.options = NULL;
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
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "JVM: Failed creating.");
			if (env != NULL && (*env)->ExceptionCheck(env)) {
				(*env)->ExceptionDescribe(env);
			}
			if (jvm != NULL) {
				res = (*jvm)->DestroyJavaVM(jvm);
			}
			g_jniEnv = NULL;
			g_jvm = NULL;
			return g_jniEnv;
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
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "JVM: Unloading ...");

		// We have to be the ONLY running thread (native and Java)
		// this may not help, but cant be bad
		jint res = (*g_jvm)->AttachCurrentThreadAsDaemon(g_jvm,
				(void**) &g_jniEnv, NULL);
		//res = jvm->AttachCurrentThread((void**) & jniEnv, NULL);
		if (res < 0 || (*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
			if ((*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
				(*g_jniEnv)->ExceptionDescribe(g_jniEnv);
			}
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"JVM: Can not Attach to the current thread,"
					" error code: %i", res);
			return false;
		}

		res = (*g_jvm)->DestroyJavaVM(g_jvm);
		if (res != 0) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"JVM: Failed destroying, error code: %i", res);
			return false;
		} else {
			g_jniEnv = NULL;
			g_jvm = NULL;
		}
	}


	return true;
}


bool java_preloadJNIEnv() {

	ESTABLISH_JAVA_ENV
	JNIEnv* env = java_getJNIEnv();
	ESTABLISH_SPRING_ENV

	return env != NULL;
}


bool java_initStatic(int _interfaceId,
		const struct SAIInterfaceCallback* _callback) {

	interfaceId = _interfaceId;
	callback = _callback;

	maxTeams = callback->Teams_getSize(interfaceId);
	maxSkirmishImpls = callback->Teams_getSize(interfaceId);
	sizeImpls = 0;

	teamId_aiImplId = (unsigned int*) calloc(maxTeams, sizeof(unsigned int));
	unsigned int t;
	for (t = 0; t < maxTeams; ++t) {
		teamId_aiImplId[t] = 0;
	}

	aiImplId_className = (char**) calloc(maxSkirmishImpls, sizeof(char*));
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

	if (g_m_url_ctor == NULL) {
		// get the URL class
		static const char* const fcCls = "java/net/URL";

		g_cls_url = java_findClass(env, fcCls);
		if (g_cls_url == NULL) return false;

		g_cls_url = java_makeGlobalRef(env, g_cls_url);
		if (g_cls_url == NULL) return false;

		// get (String)	constructor
		g_m_url_ctor = java_getMethodID(env, g_cls_url,
				"<init>", "(Ljava/lang/String;)V");
		if (g_m_url_ctor == NULL) return false;
	}

	return true;
}

static bool java_initURLClassLoaderClass(JNIEnv* env) {

	if (g_m_urlClassLoader_findClass == NULL) {
		// get the URLClassLoader class
		static const char* const fcCls = "java/net/URLClassLoader";

		g_cls_urlClassLoader = java_findClass(env, fcCls);
		if (g_cls_urlClassLoader == NULL) return false;

		g_cls_urlClassLoader = java_makeGlobalRef(env, g_cls_urlClassLoader);
		if (g_cls_urlClassLoader == NULL) return false;

		// get (URL[])	constructor
		g_m_urlClassLoader_ctor = java_getMethodID(env,
				g_cls_urlClassLoader, "<init>", "([Ljava/net/URL;)V");
		if (g_m_urlClassLoader_ctor == NULL) return false;

		// get the findClass(String) method
		g_m_urlClassLoader_findClass = java_getMethodID(env,
				g_cls_urlClassLoader, "findClass",
				"(Ljava/lang/String;)Ljava/lang/Class;");
		if (g_m_urlClassLoader_findClass == NULL) return false;
	}

	return true;
}


static bool java_initPointerClass(JNIEnv* env) {

	if (g_m_jnaPointer_ctor_long == NULL) {
		// get the Pointer class
		static const char* const fcCls = "com/sun/jna/Pointer";

		g_cls_jnaPointer = java_findClass(env, fcCls);
		if (g_cls_jnaPointer == NULL) return false;

		g_cls_jnaPointer = java_makeGlobalRef(env, g_cls_jnaPointer);
		if (g_cls_jnaPointer == NULL) return false;

		// get native pointer constructor
		g_m_jnaPointer_ctor_long =
				java_getMethodID(env, g_cls_jnaPointer, "<init>", "(J)V");
		if (g_m_jnaPointer_ctor_long == NULL) return false;
	}

	return true;
}


static jobject java_createAICallback(JNIEnv* env, const struct SSkirmishAICallback* aiCallback) {

	jobject o_clb = NULL;

	// initialize the AI Callback class, if not yet done
	if (g_cls_aiCallback == NULL) {
		// get the AI Callback class
#ifdef _WIN32
		static const char* const aiCallbackClassName = "com/clan_sy/spring/ai/Win32AICallback";
#else
		static const char* const aiCallbackClassName = "com/clan_sy/spring/ai/DefaultAICallback";
#endif

		g_cls_aiCallback = java_findClass(env, aiCallbackClassName);
		g_cls_aiCallback = java_makeGlobalRef(env, g_cls_aiCallback);

		// get no-arg constructor
		static const size_t signature_sizeMax = 512;
		char signature[signature_sizeMax];
		SNPRINTF(signature, signature_sizeMax, "(Lcom/sun/jna/Pointer;)L%s;", aiCallbackClassName);
		g_m_aiCallback_getInstance = java_getStaticMethodID(env, g_cls_aiCallback, "getInstance", signature);
	}

	// create a Java JNA Pointer to the AI Callback
	jlong jniPointerToClb = (jlong) ((intptr_t)aiCallback);
	jobject jnaPointerToClb = (*env)->NewObject(env, g_cls_jnaPointer,
			g_m_jnaPointer_ctor_long, jniPointerToClb);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed creating JNA pointer to the AI Callback");
		(*env)->ExceptionDescribe(env);
		jnaPointerToClb = NULL;
	}

	// create a Java AI Callback instance
// 	o_clb = (*env)->NewObject(env, g_cls_aiCallback, g_m_aiCallback_ctor_P, jnaPointerToClb);
// 	if (o_clb == NULL || (*env)->ExceptionCheck(env)) {
// 		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Failed creating Java AI Callback instance");
// 		if ((*env)->ExceptionCheck(env)) {
// 			(*env)->ExceptionDescribe(env);
// 		}
// 		o_clb = NULL;
// 	}
	o_clb = (*env)->CallStaticObjectMethod(env, g_cls_aiCallback, g_m_aiCallback_getInstance, jnaPointerToClb);
	if (o_clb == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Failed creating Java AI Callback instance");
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		o_clb = NULL;
	}

	o_clb = java_makeGlobalRef(env, o_clb);

	return o_clb;
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
	teamId_aiImplId = NULL;

	free(aiImplId_className);
	free(aiImplId_instance);
	free(aiImplId_methods);
	free(aiImplId_classLoader);
	aiImplId_className = NULL;
	aiImplId_instance = NULL;
	aiImplId_methods = NULL;
	aiImplId_classLoader = NULL;

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


bool java_initSkirmishAIClass(
		const char* const shortName,
		const char* const version,
		const char* const className) {

	bool success = false;

	// see if an AI for className is instantiated already
	unsigned int implId;
	for (implId = 0; implId < sizeImpls; ++implId) {
		if (strcmp(aiImplId_className[implId], className) == 0) {
			break;
		}
	}

	// instantiate AI (if needed)
	if (aiImplId_className[implId] == NULL) {
		ESTABLISH_JAVA_ENV
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
		ESTABLISH_SPRING_ENV
		if (success) {
			aiImplId_className[implId] = util_allocStrCpy(className);
		} else {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Class loading failed for class: %s", className);
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
		ESTABLISH_JAVA_ENV
		JNIEnv* env = java_getJNIEnv();

		// delete the AI class-loader global reference,
		// so it will be garbage collected
		(*env)->DeleteGlobalRef(env, aiImplId_classLoader[implId]);
		success = !((*env)->ExceptionCheck(env));
		if (!success) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed to delete AI class-loader global reference.");
			(*env)->ExceptionDescribe(env);
		}

		// delete the AI global reference,
		// so it will be garbage collected
		(*env)->DeleteGlobalRef(env, aiImplId_instance[implId]);
		success = !((*env)->ExceptionCheck(env));
		if (!success) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed to delete AI global reference.");
			(*env)->ExceptionDescribe(env);
		}
		ESTABLISH_SPRING_ENV

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


// const struct SSkirmishAICallback* java_getSkirmishAICCallback(int teamId) {
// 	return teamId_cCallback[teamId];
// }


int java_skirmishAI_init(int teamId, const struct SSkirmishAICallback* aiCallback) {

	int res = -1;

	jmethodID mth = NULL;
	jobject o_ai = NULL;
	ESTABLISH_JAVA_ENV
	bool success = java_getSkirmishAIAndMethod(teamId, &o_ai,
			MTH_INDEX_SKIRMISH_AI_INIT, &mth);

	JNIEnv* env = NULL;
	if (success) {
		env = java_getJNIEnv();
	}

// 	// create Java info Properties
// 	jobject o_infoProps = NULL;
// 	if (success) {
// 		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
// 				"creating Java info Properties for init() ...");
// 		o_infoProps = java_createPropertiesFromCMap(env,
// 				infoSize, infoKeys, infoValues);
// 		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
// 		success = (o_infoProps != NULL);
// 	}

// 	// create Java options Properties
// 	jobject o_optionsProps = NULL;
// 	if (success) {
// 		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
// 				"creating Java options Properties for init() ...");
// 		o_optionsProps = java_createPropertiesFromCMap(env,
// 				optionsSize, optionsKeys, optionsValues);
// 		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
// 		success = (o_optionsProps != NULL);
// 	}

	// create Java AI Callback
	jobject o_aiCallback = NULL;
	if (success) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"creating Java AI Callback for init() ...");
		o_aiCallback = java_createAICallback(env, aiCallback);
		success = (o_aiCallback != NULL);
		if (success) {
			simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
		} else {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "failed!");
		}
	}

	if (success) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"calling Java AI method init(teamId, callback)...");
		res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId,
				o_aiCallback);
		success = (res == 0);
		if (success) {
			simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
		} else {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "failed!");
		}
	}
	java_deleteGlobalRef(env, o_aiCallback);
	ESTABLISH_SPRING_ENV

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
		ESTABLISH_JAVA_ENV
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
		ESTABLISH_SPRING_ENV
	}

	return res;
}
