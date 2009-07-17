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
#include "JvmLocater.h"
#include "CUtils/Util.h"
#include "CUtils/SimpleLog.h"
#include "CUtils/SharedLibrary.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SAIInterfaceCallback.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"

#include <jni.h>

#include <string.h>	// strlen(), strcat(), strcpy()
#include <stdlib.h>	// malloc(), calloc(), free()

// These defines are taken from the OpenJDK jlong_md.h files
// (one for solaris and one for windows)
#ifdef __arch64__
	#define jlong_to_ptr(a) ((void*)(a))
	#define ptr_to_jlong(a) ((jlong)(a))
#else
	/* Double casting to avoid warning messages looking for casting of */
	/* smaller sizes into pointers */
	#define jlong_to_ptr(a) ((void*)(int)(a))
	#define ptr_to_jlong(a) ((jlong)(int)(a))
#endif

static int interfaceId = -1;
static const struct SAIInterfaceCallback* callback = NULL;

static size_t  team_sizeMax = 0;
static size_t* team_skirmishAiImpl;

static size_t       skirmishAiImpl_sizeMax = 0;
static size_t       skirmishAiImpl_size = 0;
static char**       skirmishAiImpl_className;
static jobject*     skirmishAiImpl_instance;
static jmethodID**  skirmishAiImpl_methods;
static jobject*     skirmishAiImpl_classLoader;

// vars used to integrate the JVM
// it is loaded at runtime, not at loadtime
static sharedLib_t jvmSharedLib = NULL;
typedef jint (JNICALL JNI_GetDefaultJavaVMInitArgs_t)(void* vmArgs);
typedef jint (JNICALL JNI_CreateJavaVM_t)(JavaVM** vm, void** jniEnv, void* vmArgs);
typedef jint (JNICALL JNI_GetCreatedJavaVMs_t)(JavaVM** vms, jsize vms_sizeMax, jsize* vms_size);
static JNI_GetDefaultJavaVMInitArgs_t* JNI_GetDefaultJavaVMInitArgs_f;
static JNI_CreateJavaVM_t*             JNI_CreateJavaVM_f;
static JNI_GetCreatedJavaVMs_t*        JNI_GetCreatedJavaVMs_f;

// JNI global vars
static JavaVM* g_jvm = NULL;

static jclass g_cls_url = NULL;
static jmethodID g_m_url_ctor = NULL;

static jclass g_cls_urlClassLoader = NULL;
static jmethodID g_m_urlClassLoader_ctor = NULL;
static jmethodID g_m_urlClassLoader_findClass = NULL;

static jclass g_cls_jnaPointer = NULL;
static jmethodID g_m_jnaPointer_ctor_long = NULL;
//static jmethodID g_m_jnaPointer_getPointer_L = NULL;
//static jclass g_cls_intByRef = NULL;
//static jmethodID g_m_intByRef_ctor_I = NULL;
//static jmethodID g_m_intByRef_getPointer = NULL;

static jclass g_cls_aiCallback = NULL;
static jmethodID g_m_aiCallback_getInstance = NULL;


// ### General helper functions following ###

/// Sets the FPU state to how spring likes it
static inline void establishSpringEnv() {

	//(*g_jvm)->DetachCurrentThread(g_jvm);
	util_resetEngineEnv();
}
/// The JVM sets the environment it wants automatically, so this is a no-op
static inline void establishJavaEnv() {}


static inline size_t minSize(size_t size1, size_t size2) {
	return (size1 < size2) ? size1 : size2;
}


// /**
//  * Called when the JVM loads this native library.
//  */
// JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
// 	return JNI_VERSION_1_6;
// }
//
// /**
//  * Called when the JVM unloads this native library.
//  */
// JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {}




// ### JNI helper functions following ###

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
	const bool hasException = (*env)->ExceptionCheck(env);
	if (res == NULL || hasException) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Class not found: \"%s\"", className);
		if (hasException) {
			(*env)->ExceptionDescribe(env);
		}
		res = NULL;
	}

	return res;
}
static jobject java_makeGlobalRef(JNIEnv* env, jobject localObject,
		const char* objDesc) {

	jobject res = NULL;

	// Make the local class a global reference,
	// so it will not be garbage collected,
	// even after this method returned,
	// but only if explicitly deleted with DeleteGlobalRef
	res = (*env)->NewGlobalRef(env, localObject);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed to make %s a global reference.",
				((objDesc == NULL) ? "" : objDesc));
		(*env)->ExceptionDescribe(env);
		res = NULL;
	}

	return res;
}
static bool java_deleteGlobalRef(JNIEnv* env, jobject globalObject,
		const char* objDesc) {

	// delete the AI class-loader global reference,
	// so it will be garbage collected
	(*env)->DeleteGlobalRef(env, globalObject);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed to delete global reference %s.",
				((objDesc == NULL) ? "" : objDesc));
		(*env)->ExceptionDescribe(env);
		return false;
	}

	return true;
}
static jmethodID java_getMethodID(JNIEnv* env, jclass cls,
		const char* const name, const char* const signature) {

	jmethodID res = NULL;

	res = (*env)->GetMethodID(env, cls, name, signature);
	const bool hasException = (*env)->ExceptionCheck(env);
	if (res == NULL || hasException) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Method not found: %s(%s)",
				name, signature);
		if (hasException) {
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
	const bool hasException = (*env)->ExceptionCheck(env);
	if (res == NULL || hasException) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Method not found: %s(%s)",
				name, signature);
		if (hasException) {
			(*env)->ExceptionDescribe(env);
		}
		res = NULL;
	}

	return res;
}

/**
 * Creates the AI Interface global Java class path.
 *
 * It will consist of the following:
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/AIInterface.jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/[*].jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/jlib/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/jlib/[*].jar
 */
static size_t java_createClassPath(char* classPathStr, const size_t classPathStr_sizeMax) {

	// the dirs and .jar files in the following array
	// will be concatenated with intermediate path separators
	// to form the classPathStr
	static const size_t classPath_sizeMax = 128;
	char**              classPath = (char**) calloc(classPath_sizeMax, sizeof(char*));
	size_t              classPath_size = 0;

	// the Java AI Interfaces java library file path (.../AIInterface.jar)
	classPath[classPath_size++] = callback->DataDirs_allocatePath(interfaceId,
			JAVA_AI_INTERFACE_LIBRARY_FILE_NAME,
			false, false, false, false);

	// the directories in the following list will be searched for .jar files
	// which will then be added to the classPathStr, plus the dirs will be added
	// to the classPathStr directly, so you can keep .class files in there
	static const size_t jarDirs_sizeMax = 128;
	char**              jarDirs = (char**) calloc(jarDirs_sizeMax, sizeof(char*));
	size_t              jarDirs_size = 0;

	// the main java libs dir (.../jlibs/)
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2,
			callback->DataDirs_getConfigDir(interfaceId), JAVA_LIBS_DIR);

	// add the jar dirs (for .class files) and all contained .jars recursively
	size_t jd, jf;
	for (jd=0; (jd < jarDirs_size) && (classPath_size < classPath_sizeMax); ++jd) {
		if (util_fileExists(jarDirs[jd])) {
			// add the dir directly
			classPath[classPath_size++] = util_allocStrCpy(jarDirs[jd]);

			// add the contained jars recursively
			static const size_t jarFiles_sizeMax = 128;
			char**              jarFiles = (char**) calloc(jarFiles_sizeMax, sizeof(char*));
			const size_t        jarFiles_size = util_listFiles(jarDirs[jd], ".jar",
					jarFiles, true, jarFiles_sizeMax);
			for (jf=0; (jf < jarFiles_size) && (classPath_size < classPath_sizeMax); ++jf) {
				classPath[classPath_size++] = util_allocStrCatFSPath(2,
						jarDirs[jd], jarFiles[jf]);
				FREE(jarFiles[jf]);
			}
			FREE(jarFiles);
		}
		FREE(jarDirs[jd]);
	}
	FREE(jarDirs);

	// concat the classpath entries
	classPathStr[0] = '\0';
	if (classPath[0] != NULL) {
		STRCATS(classPathStr, classPathStr_sizeMax, classPath[0]);
		FREE(classPath[0]);
	}
	size_t cp;
	for (cp=1; cp < classPath_size; ++cp) {
		if (classPath[cp] != NULL) {
			STRCATS(classPathStr, classPathStr_sizeMax, ENTRY_DELIM);
			STRCATS(classPathStr, classPathStr_sizeMax, classPath[cp]);
			FREE(classPath[cp]);
		}
	}
	FREE(classPath);

	return classPath_size;
}

/**
 * Creates a Skirmish AI local Java class path.
 *
 * It will consist of the following:
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/SkirmishAI.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/jlib/[*].jar
 */
static size_t java_createAIClassPath(const char* shortName, const char* version,
		char** classPathParts, const size_t classPathParts_sizeMax) {

	size_t classPathParts_size = 0;

	// the .jar files in the following list will be added to the classpath
	const size_t jarFiles_sizeMax = classPathParts_sizeMax;
	char**       jarFiles = (char**) calloc(jarFiles_sizeMax, sizeof(char*));
	size_t       jarFiles_size = 0;

	const char* const skirmDD =
			callback->SkirmishAIs_Info_getValueByKey(interfaceId,
			shortName, version,
			SKIRMISH_AI_PROPERTY_DATA_DIR);
	if (skirmDD == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Retrieving the data-dir of Skirmish AI %s-%s failed.",
				shortName, version);
	}
	// {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/SkirmishAI.jar
	jarFiles[jarFiles_size++] = util_allocStrCatFSPath(2,
			skirmDD, "SkirmishAI.jar");

	// the directories in the following list will be searched for .jar files
	// which then will be added to the classpath, plus they will be added
	// to the classpath directly, so you can keep .class files in there
	const size_t jarDirs_sizeMax = classPathParts_sizeMax;
	char**       jarDirs = (char**) calloc(jarDirs_sizeMax, sizeof(char*));
	size_t       jarDirs_size = 0;

	// {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/SkirmishAI/
	// this can be usefull for AI devs while testing,
	// if they do not want to put everything into a jar all the time
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "SkirmishAI");

	// {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, JAVA_LIBS_DIR);

	// add the common/jlib dir, if it is specified and exists
	const char* const skirmDDCommon =
			callback->SkirmishAIs_Info_getValueByKey(interfaceId,
			shortName, version,
			SKIRMISH_AI_PROPERTY_DATA_DIR_COMMON);
	if (skirmDDCommon != NULL) {
		char* commonJLibsDir = util_allocStrCatFSPath(2, skirmDDCommon, JAVA_LIBS_DIR);
		if (commonJLibsDir != NULL && util_fileExists(commonJLibsDir)) {
			// {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/jlib/
			jarDirs[jarDirs_size++] = commonJLibsDir;
		} else {
			FREE(commonJLibsDir);
		}
	}

	// add the directly specified .jar files
	size_t jf;
	for (jf = 0; (jf < jarFiles_size) && (classPathParts_size < classPathParts_sizeMax); ++jf) {
		classPathParts[classPathParts_size++] = util_allocStrCpy(jarFiles[jf]);
		FREE(jarFiles[jf]);
	}

	// add the dirs and the contained .jar files
	size_t jd, sjf;
	for (jd = 0; (jd < jarDirs_size) && (classPathParts_size < classPathParts_sizeMax); ++jd)
	{
		if (jarDirs[jd] != NULL && util_fileExists(jarDirs[jd])) {
			// add the jar dir (for .class files)
			classPathParts[classPathParts_size++] = util_allocStrCpy(jarDirs[jd]);

			// add the jars in the dir
			const size_t subJarFiles_sizeMax = classPathParts_sizeMax - classPathParts_size;
			char**       subJarFiles = (char**) calloc(subJarFiles_sizeMax, sizeof(char*));
			const size_t subJarFiles_size = util_listFiles(jarDirs[jd], ".jar",
					subJarFiles, true, subJarFiles_sizeMax);
			for (sjf = 0; (sjf < subJarFiles_size) && (classPathParts_size < classPathParts_sizeMax); ++sjf) {
				// .../[*].jar
				classPathParts[classPathParts_size++] =
						util_allocStrCatFSPath(2, jarDirs[jd], subJarFiles[sjf]);
				FREE(subJarFiles[sjf]);
			}
			FREE(subJarFiles);
		}
		FREE(jarDirs[jd]);
	}

	FREE(jarDirs);
	FREE(jarFiles);

	return classPathParts_size;
}

static jobject java_createAIClassLoader(JNIEnv* env,
		const char* shortName, const char* version) {

#ifdef _WIN32
	static const char* FILE_URL_PREFIX = "file:///";
#else // _WIN32
	static const char* FILE_URL_PREFIX = "file://";
#endif // _WIN32

	jobject o_jClsLoader = NULL;

	static const size_t classPathParts_sizeMax = 512;
	char**              classPathParts = (char**) calloc(classPathParts_sizeMax, sizeof(char*));
	const size_t        classPathParts_size =
			java_createAIClassPath(shortName, version, classPathParts, classPathParts_sizeMax);

	jobjectArray o_cppURLs = (*env)->NewObjectArray(env, classPathParts_size,
			g_cls_url, NULL);
	if (checkException(env, "Failed creating URL[].")) { return NULL; }
	size_t cpp;
	for (cpp = 0; cpp < classPathParts_size; ++cpp) {
		#ifdef _WIN32
		// we can not use windows path separators in file URLs
		util_strReplaceChar(classPathParts[cpp], '\\', '/');
		#endif

		char* str_fileUrl = util_allocStrCat(2, FILE_URL_PREFIX, classPathParts[cpp]);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"Skirmish AI %s %s class-path part %i: %s",
				shortName, version, cpp, str_fileUrl);
		jstring jstr_fileUrl = (*env)->NewStringUTF(env, str_fileUrl);
		if (checkException(env, "Failed creating Java String.")) { return NULL; }
		jobject jurl_fileUrl =
				(*env)->NewObject(env, g_cls_url, g_m_url_ctor, jstr_fileUrl);
		if (checkException(env, "Failed creating Java URL.")) { return NULL; }
		(*env)->SetObjectArrayElement(env, o_cppURLs, cpp, jurl_fileUrl);
		if (checkException(env, "Failed setting Java URL in array.")) { return NULL; }

		FREE(classPathParts[cpp]);
	}

	o_jClsLoader = (*env)->NewObject(env, g_cls_urlClassLoader,
			g_m_urlClassLoader_ctor, o_cppURLs);
	if (checkException(env, "Failed creating class-loader.")) { return NULL; }
	o_jClsLoader = java_makeGlobalRef(env, o_jClsLoader, "Skirmish AI class-loader");

	FREE(classPathParts);

	return o_jClsLoader;
}

/**
 * Creates the Java library path.
 * -> where native shared libraries are searched
 *
 * It will consist of the following:
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/lib/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/lib/
 */
static bool java_createNativeLibsPath(char* libraryPath, const size_t libraryPath_sizeMax) {

	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/
	const char* const dd_r =
			callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_DATA_DIR);
	if (dd_r == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Unable to find read-only data-dir.");
		return false;
	} else {
		STRCPYS(libraryPath, libraryPath_sizeMax, dd_r);
	}

	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/lib/
	char* dd_lib_r = callback->DataDirs_allocatePath(interfaceId,
			NATIVE_LIBS_DIR, false, false, true, false);
	if (dd_lib_r == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_NORMAL,
				"Unable to find read-only native libs data-dir (optional): %s",
				NATIVE_LIBS_DIR);
	} else {
		STRCATS(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
		STRCPYS(libraryPath, libraryPath_sizeMax, dd_lib_r);
		FREE(dd_lib_r);
	}

	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/
	const char* const dd_r_common =
			callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_DATA_DIR_COMMON);
	if (dd_r_common == NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_NORMAL,
				"Unable to find common read-only data-dir (optional).");
	} else {
		STRCATS(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
		STRCPYS(libraryPath, libraryPath_sizeMax, dd_r_common);
	}

	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/lib/
	if (dd_r_common != NULL) {
		char* dd_lib_r_common = callback->DataDirs_allocatePath(interfaceId,
			NATIVE_LIBS_DIR, false, false, true, true);
		if (dd_lib_r_common == NULL || !util_fileExists(dd_lib_r_common)) {
			simpleLog_logL(SIMPLELOG_LEVEL_NORMAL,
					"Unable to find common read-only native libs data-dir (optional).");
		} else {
			STRCATS(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
			STRCPYS(libraryPath, libraryPath_sizeMax, dd_lib_r_common);
			FREE(dd_lib_r_common);
		}
	}

	return true;
}
static bool java_createJavaVMInitArgs(struct JavaVMInitArgs* vm_args) {

	static const size_t cfgProps_sizeMax = 64;
	const char**        cfgProps_keys = (const char**) calloc(cfgProps_sizeMax, sizeof(char*));
	const char**        cfgProps_values = (const char**) calloc(cfgProps_sizeMax, sizeof(char*));
	size_t              cfgProps_size = 0;

	// ### read JVM options config file ###
	char* jvmPropFile = callback->DataDirs_allocatePath(interfaceId,
			JVM_PROPERTIES_FILE, false, false, false, false);
	if (jvmPropFile == NULL) {
		// if the version specific file does not exist,
		// try to get the common one
		jvmPropFile = callback->DataDirs_allocatePath(interfaceId,
			JVM_PROPERTIES_FILE, false, false, false, true);
	}
	if (jvmPropFile != NULL) {
		cfgProps_size = util_parsePropertiesFile(jvmPropFile,
				cfgProps_keys, cfgProps_values, cfgProps_sizeMax);
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"JVM: arguments loaded from: %s", jvmPropFile);
	} else {
		cfgProps_size = 0;
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"JVM: arguments NOT loaded from: %s", jvmPropFile);
	}

	// ### evaluate JNI version to use ###
	//jint jniVersion = JNI_VERSION_1_1;
	//jint jniVersion = JNI_VERSION_1_2;
	jint jniVersion = JNI_VERSION_1_4;
	//jint jniVersion = JNI_VERSION_1_6;
	if (jvmPropFile != NULL) {
		const char* jniVersionFromCfg =
				util_map_getValueByKey(
				cfgProps_size, cfgProps_keys, cfgProps_values,
				"jvm.jni.version");
		if (jniVersionFromCfg != NULL) {
			unsigned long int jniVersion_tmp =
					strtoul(jniVersionFromCfg, NULL, 16);
			if (jniVersion_tmp != 0/* && jniVersion_tmp != ULONG_MAX*/) {
				jniVersion = (jint) jniVersion_tmp;
			}
		}
	}
	simpleLog_logL(SIMPLELOG_LEVEL_FINE, "JVM: JNI version: %#x", jniVersion);
	vm_args->version = jniVersion;

	// ### check if unrecognized JVM options should be ignored ###
	// set ignore unrecognized
	// if false, the JVM creation will fail if an
	// unknown or invalid option was specified
	bool ignoreUnrecognized = true;
	if (jvmPropFile != NULL) {
		const char* ignoreUnrecognizedFromCfg =
				util_map_getValueByKey(
				cfgProps_size, cfgProps_keys, cfgProps_values,
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
	// autogenerate the class path
	static const size_t classPath_sizeMax = 8 * 1024;
	char* classPath = util_allocStr(classPath_sizeMax);
	// ..., autogenerate it ...
	if (!java_createClassPath(classPath, classPath_sizeMax)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed creating Java class-path.");
		return false;
	}
	if (jvmPropFile != NULL) {
		// ..., and append the part from the jvm options properties file,
		// if it is specified there
		const char* clsPathFromCfg =
				util_map_getValueByKey(
				cfgProps_size, cfgProps_keys, cfgProps_values,
				"jvm.option.java.class.path");
		if (clsPathFromCfg != NULL) {
			STRCATS(classPath, classPath_sizeMax, ENTRY_DELIM);
			STRCATS(classPath, classPath_sizeMax, clsPathFromCfg);
		}
	}
	// create the java.class.path option
	static const size_t classPathOpt_sizeMax = 8 * 1024;
	char* classPathOpt = util_allocStr(classPathOpt_sizeMax);
	STRCPYS(classPathOpt, classPathOpt_sizeMax, "-Djava.class.path=");
	STRCATS(classPathOpt, classPathOpt_sizeMax, classPath);
	FREE(classPath);

	// ### create the Java library-path option ###
	// autogenerate the java library path
	static const size_t libraryPath_sizeMax = 4 * 1024;
	char* libraryPath = util_allocStr(libraryPath_sizeMax);
	// ..., autogenerate it ...
	if (!java_createNativeLibsPath(libraryPath, libraryPath_sizeMax)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed creating Java library-path.");
		return false;
	}
	if (jvmPropFile != NULL) {
		// ..., and append the part from the jvm options properties file,
		// if it is specified there
		const char* libPathFromCfg =
				util_map_getValueByKey(
				cfgProps_size, cfgProps_keys, cfgProps_values,
				"jvm.option.java.library.path");
		if (libPathFromCfg != NULL) {
			STRCATS(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
			STRCATS(libraryPath, libraryPath_sizeMax, libPathFromCfg);
		}
	}
	// create the java.library.path option ...
	// autogenerate it, and append the part from the jvm options file,
	// if it is specified there
	static const size_t libraryPathOpt_sizeMax = 4 * 1024;
	char* libraryPathOpt = util_allocStr(libraryPathOpt_sizeMax);
	STRCPYS(libraryPathOpt, libraryPathOpt_sizeMax, "-Djava.library.path=");
	STRCATS(libraryPathOpt, libraryPathOpt_sizeMax, libraryPath);
	FREE(libraryPath);

	// ### create and set all JVM options ###
	static const size_t strOptions_sizeMax = 64;
	const char* strOptions[strOptions_sizeMax];
	size_t op = 0;

	strOptions[op++] = classPathOpt;
	strOptions[op++] = libraryPathOpt;

	static const char* const JCPVAL = "-Djava.class.path=";
	const size_t JCPVAL_size = strlen(JCPVAL);
	static const char* const JLPVAL = "-Djava.library.path=";
	const size_t JLPVAL_size = strlen(JCPVAL);
	if (jvmPropFile != NULL) {
		// ### add string options from the JVM config file with property name "jvm.option.x" ###
		int i;
		for (i=0; i < cfgProps_size; ++i) {
			if (strcmp(cfgProps_keys[i], "jvm.option.x") == 0) {
				const char* const val = cfgProps_values[i];
				const size_t val_size = strlen(val);
				// ignore "-Djava.class.path=..."
				// and "-Djava.library.path=..." options
				if (strncmp(val, JCPVAL, minSize(val_size, JCPVAL_size)) != 0 &&
					strncmp(val, JLPVAL, minSize(val_size, JLPVAL_size)) != 0) {
					strOptions[op++] = cfgProps_values[i];
				}
			}
		}
	} else {
		// ### ... or set default ones, if the JVM config file was not found ###
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

	const size_t options_size = op;

	vm_args->options = (struct JavaVMOption*) calloc(options_size, sizeof(struct JavaVMOption));

	// fill strOptions into the JVM options
	simpleLog_logL(SIMPLELOG_LEVEL_FINE, "JVM: options:", options_size);
	char* dd_rw = callback->DataDirs_allocatePath(interfaceId,
			"", true, true, true, false);
	size_t i;
	jint nOptions = 0;
	for (i = 0; i < options_size; ++i) {
		char* tmpOptionString = util_allocStrReplaceStr(strOptions[i],
				"${home-dir}", dd_rw);
		// do not add empty options
		if (tmpOptionString != NULL) {
			if (strlen(tmpOptionString) > 0) {
				vm_args->options[nOptions].optionString = tmpOptionString;
				vm_args->options[nOptions].extraInfo = NULL;
				simpleLog_logL(SIMPLELOG_LEVEL_FINE, "JVM option %ul: %s", nOptions, tmpOptionString);
				nOptions++;
			} else {
				free(tmpOptionString);
				tmpOptionString = NULL;
			}
		}
	}
	vm_args->nOptions = nOptions;

	FREE(dd_rw);
	FREE(classPathOpt);
	FREE(libraryPathOpt);
	simpleLog_logL(SIMPLELOG_LEVEL_FINE, "");

	FREE(jvmPropFile);

	FREE(cfgProps_keys);
	FREE(cfgProps_values);

	return true;
}



static JNIEnv* java_reattachCurrentThread() {

	JNIEnv* env = NULL;

	simpleLog_logL(SIMPLELOG_LEVEL_FINEST, "Reattaching current thread...");
	//jint res = (*g_jvm)->AttachCurrentThreadAsDaemon(g_jvm, (void**) &env, NULL);
	jint res = (*g_jvm)->AttachCurrentThread(g_jvm, (void**) &env, NULL);
	bool failure = (res < 0);
	if (failure) {
		env = NULL;
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed attaching jvm to current thread,"
				" error code(2): %i", (int) res);
	}

	return env;
}

static JNIEnv* java_getJNIEnv() {

	JNIEnv* ret = NULL;

	if (g_jvm == NULL) {
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
				// Returns NULL on success; a negative number on failure.
				res = JNI_GetCreatedJavaVMs_f(&jvm, 1, &numJVMsFound);
				if (res < 0) {
					simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
							"Can not look for Java VMs, error code: %i", res);
					goto end;
				}
				simpleLog_log("number of existing JVMs: %i", numJVMsFound);
		 */

		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "creating JVM...");
		res = JNI_CreateJavaVM_f(&jvm, (void**) &env, &vm_args);
		if (res != 0 || (*env)->ExceptionCheck(env)) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Can not create Java VM, error code: %i", res);
			goto end;
		}

		// free the JavaVMInitArgs content
		jint i;
		for (i = 0; i < vm_args.nOptions; ++i) {
			FREE(vm_args.options[i].optionString);
		}
		FREE(vm_args.options);

		//res = (*jvm)->AttachCurrentThreadAsDaemon(jvm, (void**) &env, NULL);
		res = (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);
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
			g_jvm = NULL;
			ret = NULL;
		} else {
			g_jvm = jvm;
			ret = env;
		}
	} else {
		ret = java_reattachCurrentThread();
	}

	return ret;
}


bool java_unloadJNIEnv() {

	if (g_jvm != NULL) {
		simpleLog_logL(SIMPLELOG_LEVEL_FINE, "JVM: Unloading ...");

		//JNIEnv* env = java_getJNIEnv();

		// We have to be the ONLY running thread (native and Java)
		// this may not help, but will not hurt either
		//jint res = (*g_jvm)->AttachCurrentThreadAsDaemon(g_jvm,
		//		(void**) &g_jniEnv, NULL);
		//res = jvm->AttachCurrentThread((void**) & jniEnv, NULL);
		/*if (res < 0 || (*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
			if ((*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
				(*g_jniEnv)->ExceptionDescribe(g_jniEnv);
			}
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"JVM: Can not Attach to the current thread,"
					" error code: %i", res);
			return false;
		}*/

		jint res = (*g_jvm)->DetachCurrentThread(g_jvm);
		if (res != 0) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"JVM: Failed detaching current thread, error code: %i", res);
			return false;
		}

		res = (*g_jvm)->DestroyJavaVM(g_jvm);
		if (res != 0) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"JVM: Failed destroying, error code: %i", res);
			return false;
		} else {
			g_jvm = NULL;
		}
	}

	return true;
}


bool java_preloadJNIEnv() {

	establishJavaEnv();
	JNIEnv* env = java_getJNIEnv();
	establishSpringEnv();

	return env != NULL;
}


bool java_initStatic(int _interfaceId,
		const struct SAIInterfaceCallback* _callback) {

	interfaceId = _interfaceId;
	callback = _callback;

	team_sizeMax = callback->Teams_getSize(interfaceId);
	skirmishAiImpl_sizeMax = team_sizeMax;
	skirmishAiImpl_size = 0;

	team_skirmishAiImpl = (size_t*) calloc(team_sizeMax, sizeof(size_t));
	size_t t;
	for (t = 0; t < team_sizeMax; ++t) {
		team_skirmishAiImpl[t] = 999999;
	}

	skirmishAiImpl_className = (char**) calloc(skirmishAiImpl_sizeMax, sizeof(char*));
	skirmishAiImpl_instance = (jobject*) calloc(skirmishAiImpl_sizeMax, sizeof(jobject));
	skirmishAiImpl_methods = (jmethodID**) calloc(skirmishAiImpl_sizeMax, sizeof(jmethodID*));
	skirmishAiImpl_classLoader = (jobject*) calloc(skirmishAiImpl_sizeMax, sizeof(jobject));
	size_t sai;
	for (sai = 0; sai < skirmishAiImpl_sizeMax; ++sai) {
		skirmishAiImpl_className[sai] = NULL;
		skirmishAiImpl_instance[sai] = NULL;
		skirmishAiImpl_methods[sai] = NULL;
		skirmishAiImpl_classLoader[sai] = NULL;
	}

	// dynamically load the JVM
	char* jreLocationFile = callback->DataDirs_allocatePath(interfaceId,
			JRE_LOCATION_FILE, false, false, false, false);

	static const size_t jrePath_sizeMax = 1024;
	char jrePath[jrePath_sizeMax];
	bool jreFound = GetJREPath(jrePath, jrePath_sizeMax, jreLocationFile, NULL);
	if (!jreFound) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed locating a JRE installation"
				", you may specify the JAVA_HOME env var.");
		return false;
	}
	FREE(jreLocationFile);

#if defined __arch64__
	static const char* jvmType = "server";
#else
	static const char* jvmType = "client";
#endif
	static const size_t jvmLibPath_sizeMax = 1024;
	char jvmLibPath[jvmLibPath_sizeMax];
	bool jvmLibFound = GetJVMPath(jrePath, jvmType, jvmLibPath,
			jvmLibPath_sizeMax, NULL);
	if (!jvmLibFound) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed locating the %s version of the JVM, please contact spring devs.", jvmType);
		return false;
	}

	jvmSharedLib = sharedLib_load(jvmLibPath);
	if (!sharedLib_isLoaded(jvmSharedLib)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed to load the JVM at \"%s\".", jvmLibPath);
		return false;
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_NORMAL,
				"Successfully loaded the JVM at \"%s\".", jvmLibPath);

		JNI_GetDefaultJavaVMInitArgs_f = (JNI_GetDefaultJavaVMInitArgs_t*)
				sharedLib_findAddress(jvmSharedLib, "JNI_GetDefaultJavaVMInitArgs");
		if (JNI_GetDefaultJavaVMInitArgs_f == NULL) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed to load the JVM, function \"%s\" not exported.", "JNI_GetDefaultJavaVMInitArgs");
		}

		JNI_CreateJavaVM_f = (JNI_CreateJavaVM_t*)
				sharedLib_findAddress(jvmSharedLib, "JNI_CreateJavaVM");
		if (JNI_CreateJavaVM_f == NULL) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed to load the JVM, function \"%s\" not exported.", "JNI_CreateJavaVM");
		}

		JNI_GetCreatedJavaVMs_f = (JNI_GetCreatedJavaVMs_t*)
				sharedLib_findAddress(jvmSharedLib, "JNI_GetCreatedJavaVMs");
		if (JNI_GetCreatedJavaVMs_f == NULL) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed to load the JVM, function \"%s\" not exported.", "JNI_GetCreatedJavaVMs");
		}
	}

	return true;
}


static bool java_initURLClass(JNIEnv* env) {

	if (g_m_url_ctor == NULL) {
		// get the URL class
		static const char* const fcCls = "java/net/URL";

		g_cls_url = java_findClass(env, fcCls);
		if (g_cls_url == NULL) return false;

		g_cls_url = java_makeGlobalRef(env, g_cls_url, fcCls);
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

		g_cls_urlClassLoader =
				java_makeGlobalRef(env, g_cls_urlClassLoader, fcCls);
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

	// This method is 64bit save, as it does not cast to jint,
	// which is 32bit on all architectures, even 64bit ones.
	if (g_m_jnaPointer_ctor_long == NULL) {
		// get the Pointer class
		static const char* const fcCls = "com/sun/jna/Pointer";

		g_cls_jnaPointer = java_findClass(env, fcCls);
		if (g_cls_jnaPointer == NULL) return false;

		g_cls_jnaPointer = java_makeGlobalRef(env, g_cls_jnaPointer, fcCls);
		if (g_cls_jnaPointer == NULL) return false;

		// get native pointer constructor
		g_m_jnaPointer_ctor_long =
				java_getMethodID(env, g_cls_jnaPointer, "<init>", "(J)V");
		if (g_m_jnaPointer_ctor_long == NULL) return false;
	}
	/*if (g_m_intByRef_getPointer == NULL) {
		// create the JNA pointer as described here:
		// https://jna.dev.java.net/#ptr_values
		// This way:
		// new IntByReference(value).getPointer().getPointer(0)
		static const char* const ptrCls = "com/sun/jna/Pointer";
		static const char* const ibrCls = "com/sun/jna/ptr/IntByReference";

		g_cls_jnaPointer = java_findClass(env, ptrCls);
		if (g_cls_jnaPointer == NULL) return false;
		g_cls_jnaPointer = java_makeGlobalRef(env, g_cls_jnaPointer, ptrCls);
		if (g_cls_jnaPointer == NULL) return false;
		static const size_t sig_jnaPointer_getPointer_L_sizeMax = 128;
		char sig_jnaPointer_getPointer_L[sig_jnaPointer_getPointer_L_sizeMax];
		SNPRINTF(sig_jnaPointer_getPointer_L,
				sig_jnaPointer_getPointer_L_sizeMax, "(J)L%s;", ptrCls);
		g_m_jnaPointer_getPointer_L = java_getMethodID(env, g_cls_jnaPointer,
				"getPointer", sig_jnaPointer_getPointer_L);
		if (g_m_jnaPointer_getPointer_L == NULL) return false;

		g_cls_intByRef = java_findClass(env, ibrCls);
		if (g_cls_intByRef == NULL) return false;
		g_cls_intByRef = java_makeGlobalRef(env, g_cls_intByRef, ibrCls);
		if (g_cls_intByRef == NULL) return false;
		g_m_intByRef_ctor_I =
				java_getMethodID(env, g_cls_intByRef, "<init>", "(I)V");
		if (g_m_intByRef_ctor_I == NULL) return false;
		static const size_t sig_intByRef_getPointer_sizeMax = 128;
		char sig_intByRef_getPointer[sig_intByRef_getPointer_sizeMax];
		SNPRINTF(sig_intByRef_getPointer, sig_intByRef_getPointer_sizeMax,
				"()L%s;", ptrCls);
		g_m_intByRef_getPointer = java_getMethodID(env, g_cls_intByRef,
				"getPointer", sig_intByRef_getPointer);
		if (g_m_intByRef_getPointer == NULL) return false;
	}*/

	return true;
}

static jobject java_creatJnaPointer(JNIEnv* env, const void* nativePointer) {

	jobject jnaPointer = NULL;

	// create a Java JNA Pointer to the AI Callback
	jlong jniPointer = ptr_to_jlong(nativePointer);
	jnaPointer = (*env)->NewObject(env, g_cls_jnaPointer,
			g_m_jnaPointer_ctor_long, jniPointer);
	if (jnaPointer == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Failed creating JNA pointer");
		(*env)->ExceptionDescribe(env);
		jnaPointer = NULL;
	}
	jnaPointer = java_makeGlobalRef(env, jnaPointer, "JNA Pointer");

/*
	// create the JNA pointer as described here:
	// https://jna.dev.java.net/#ptr_values
	// This way:
	// new IntByReference(value).getPointer().getPointer(0)

	jobject jnaIntByRef = (*env)->NewObject(env, g_cls_intByRef,
			g_m_intByRef_ctor_I, (jint) nativePointer);
	if (jnaIntByRef == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed creating JNA pointer (new IntByReference(value))");
		(*env)->ExceptionDescribe(env);
		return NULL;
	}

	jobject jnaIntermediatePointer = (*env)->CallObjectMethod(env, jnaIntByRef,
			g_m_intByRef_getPointer);
	if (jnaIntermediatePointer == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed creating JNA pointer (ibr.getPointer())");
		(*env)->ExceptionDescribe(env);
		return NULL;
	}

	jnaPointer = (*env)->CallObjectMethod(env, jnaIntermediatePointer,
			g_m_jnaPointer_getPointer_L, (jlong)0);
	if (jnaPointer == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Failed creating JNA pointer (ptr.getPointer(0))");
		(*env)->ExceptionDescribe(env);
		return NULL;
	}
	jnaPointer = java_makeGlobalRef(env, jnaPointer, "JNA Pointer");
	if (jnaPointer == NULL) return NULL;
*/

	return jnaPointer;
}

static jobject java_createAICallback(JNIEnv* env, const struct SSkirmishAICallback* aiCallback) {

	jobject o_clb = NULL;

	// initialize the AI Callback class, if not yet done
	if (g_cls_aiCallback == NULL) {
		// get the AI Callback class
#ifdef _WIN32
		static const char* const aiCallbackClassName = "com/springrts/ai/Win32AICallback";
#else
		static const char* const aiCallbackClassName = "com/springrts/ai/DefaultAICallback";
#endif

		g_cls_aiCallback = java_findClass(env, aiCallbackClassName);
		g_cls_aiCallback = java_makeGlobalRef(env, g_cls_aiCallback,
				aiCallbackClassName);

		// get no-arg constructor
		static const size_t signature_sizeMax = 512;
		char signature[signature_sizeMax];
		SNPRINTF(signature, signature_sizeMax, "(Lcom/sun/jna/Pointer;)L%s;", aiCallbackClassName);
		g_m_aiCallback_getInstance = java_getStaticMethodID(env, g_cls_aiCallback, "getInstance", signature);
	}

	// create a Java JNA Pointer to the AI Callback
	jobject jnaPointerToClb = java_creatJnaPointer(env, aiCallback);

	o_clb = (*env)->CallStaticObjectMethod(env, g_cls_aiCallback, g_m_aiCallback_getInstance, jnaPointerToClb);
	if (o_clb == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "Failed creating Java AI Callback instance");
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionDescribe(env);
		}
		o_clb = NULL;
	}

	o_clb = java_makeGlobalRef(env, o_clb, "AI callback instance");

	return o_clb;
}


bool java_releaseStatic() {

	size_t sai;
	for (sai = 0; sai < skirmishAiImpl_sizeMax; ++sai) {
		if (skirmishAiImpl_methods[sai] != NULL) {
			FREE(skirmishAiImpl_methods[sai]);
		}
	}

	FREE(team_skirmishAiImpl);

	FREE(skirmishAiImpl_className);
	FREE(skirmishAiImpl_instance);
	FREE(skirmishAiImpl_methods);
	FREE(skirmishAiImpl_classLoader);

	sharedLib_unload(jvmSharedLib);
	jvmSharedLib = NULL;

	return true;
}

static inline bool java_getSkirmishAIAndMethod(size_t teamId, jobject* o_ai,
		size_t methodIndex, jmethodID* mth) {

	bool success = false;

	size_t implId = team_skirmishAiImpl[teamId];
	*o_ai = skirmishAiImpl_instance[implId];
	*mth = skirmishAiImpl_methods[implId][methodIndex];
	success = ((*mth) != NULL);

	return success;
}


static bool java_loadSkirmishAI(JNIEnv* env,
		const char* shortName, const char* version, const char* className,
		jobject* o_ai, jmethodID* methods, jobject* o_aiClassLoader) {

	// convert className from "com.myai.AI" to "com/myai/AI"
	const size_t classNameP_sizeMax = strlen(className) + 1;
	char classNameP[classNameP_sizeMax];
	STRCPYS(classNameP, classNameP_sizeMax, className);
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
	*o_ai = java_makeGlobalRef(env, o_local_ai, "AI instance");


	// get the AIs methods

	// init
	methods[MTH_INDEX_SKIRMISH_AI_INIT] = (*env)->GetMethodID(env, cls_ai,
			MTH_SKIRMISH_AI_INIT, SIG_SKIRMISH_AI_INIT);
	if (methods[MTH_INDEX_SKIRMISH_AI_INIT] == NULL
			|| (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Method not found: %s.%s%s", className,
				MTH_SKIRMISH_AI_INIT, SIG_SKIRMISH_AI_INIT);
		return false;
	}

	// release
	methods[MTH_INDEX_SKIRMISH_AI_RELEASE] = (*env)->GetMethodID(env, cls_ai,
			MTH_SKIRMISH_AI_RELEASE, SIG_SKIRMISH_AI_RELEASE);
	if (methods[MTH_INDEX_SKIRMISH_AI_RELEASE] == NULL
			|| (*env)->ExceptionCheck(env)) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
				"Method not found: %s.%s%s", className,
				MTH_SKIRMISH_AI_RELEASE, SIG_SKIRMISH_AI_RELEASE);
		return false;
	}

	// handleEvent
	methods[MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT] = (*env)->GetMethodID(env,
			cls_ai, MTH_SKIRMISH_AI_HANDLE_EVENT, SIG_SKIRMISH_AI_HANDLE_EVENT);
	if (methods[MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT] == NULL
			|| (*env)->ExceptionCheck(env)) {
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
		const char* const className,
		int teamId) {

	bool success = false;

	// see if an AI for className is instantiated already
	size_t sai;
	for (sai = 0; sai < skirmishAiImpl_size; ++sai) {
		if (strcmp(skirmishAiImpl_className[sai], className) == 0) {
			break;
		}
	}
	// sai is now either the instantiated one, or a free one

	// instantiate AI (if needed)
	if (skirmishAiImpl_className[sai] == NULL) {
		establishJavaEnv();
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

		skirmishAiImpl_methods[sai] = (jmethodID*) calloc(MTHS_SIZE_SKIRMISH_AI,
				sizeof(jmethodID));
		success = java_loadSkirmishAI(env, shortName, version, className,
				&(skirmishAiImpl_instance[sai]), skirmishAiImpl_methods[sai],
				&(skirmishAiImpl_classLoader[sai]));
		establishSpringEnv();
		if (success) {
			skirmishAiImpl_className[sai] = util_allocStrCpy(className);
			skirmishAiImpl_size++;
		} else {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Class loading failed for class: %s", className);
		}
	} else {
		success = true;
	}

	if (success) {
		team_skirmishAiImpl[teamId] = sai;
	}

	return success;
}

bool java_releaseSkirmishAIClass(const char* className) {

	bool success = false;

	// see if an AI for className is instantiated
	size_t sai;
	for (sai = 0; sai < skirmishAiImpl_size; ++sai) {
		if (strcmp(skirmishAiImpl_className[sai], className) == 0) {
			break;
		}
	}
	// sai is now either the instantiated one, or a free one

	// release AI (if needed)
	if (skirmishAiImpl_className[sai] != NULL) {
		establishJavaEnv();
		JNIEnv* env = java_getJNIEnv();

		bool successPart;

		// delete the AI class-loader global reference,
		// so it will be garbage collected
		successPart = java_deleteGlobalRef(env, skirmishAiImpl_classLoader[sai],
				"AI class-loader");
		success = successPart;

		// delete the AI global reference,
		// so it will be garbage collected
		successPart = java_deleteGlobalRef(env, skirmishAiImpl_instance[sai],
				"AI instance");
		success = success && successPart;
		establishSpringEnv();

		if (success) {
			FREE(skirmishAiImpl_classLoader[sai]);
			FREE(skirmishAiImpl_instance[sai]);
			FREE(skirmishAiImpl_methods[sai]);
			FREE(skirmishAiImpl_className[sai]);
		}
	}

	return success;
}
bool java_releaseAllSkirmishAIClasses() {

	bool success = true;

	const char* className;
	size_t sai;
	for (sai = 0; sai < skirmishAiImpl_size; ++sai) {
		className = skirmishAiImpl_className[sai];
		if (className != NULL) {
			success = success && java_releaseSkirmishAIClass(className);
		}
	}

	return success;
}


// const struct SSkirmishAICallback* java_getSkirmishAICCallback(int teamId) {
// 	return teamId_cCallback[teamId];
// }


int java_skirmishAI_init(int teamId,
		const struct SSkirmishAICallback* aiCallback) {

	int res = -1;

	jmethodID mth = NULL;
	jobject o_ai = NULL;

	bool success = java_getSkirmishAIAndMethod(teamId, &o_ai,
			MTH_INDEX_SKIRMISH_AI_INIT, &mth);

	JNIEnv* env = NULL;
	if (success) {
		establishJavaEnv();
		env = java_getJNIEnv();
	}

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
		success = (res == 0 && !checkException(env, "calling Java AI method init(teamId, callback) failed."));
		if (success) {
			simpleLog_logL(SIMPLELOG_LEVEL_FINE, "done.");
		} else {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR, "failed!");
		}
	}
	if (success) {
		success = java_deleteGlobalRef(env, o_aiCallback, "AI callback instance");
	}
	establishSpringEnv();

	return res;
}

int java_skirmishAI_release(int teamId) {

	int res = -1;

	jmethodID mth = NULL;
	jobject o_ai = NULL;
	bool success = java_getSkirmishAIAndMethod(teamId, &o_ai,
			MTH_INDEX_SKIRMISH_AI_RELEASE, &mth);

	if (success) {
		establishJavaEnv();
		JNIEnv* env = java_getJNIEnv();
		simpleLog_logL(SIMPLELOG_LEVEL_FINE,
				"calling Java AI method release(teamId)...");
		res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId);
		if (checkException(env, "calling Java AI method release(teamId) failed.")) {
			res = -99;
		}
		establishSpringEnv();
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
		establishJavaEnv();
		JNIEnv* env = java_getJNIEnv();

		jobject jnaPointerToData = java_creatJnaPointer(env, data);
		if (jnaPointerToData == NULL) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"handleEvent: creating JNA pointer to data failed");
			res = -3;
			success = false;
		}

		if (success) {
			env = java_reattachCurrentThread();
			res = (int) (*env)->CallIntMethod(env, o_ai, mth, (jint) teamId, (jint) topic, jnaPointerToData);
			if ((*env)->ExceptionCheck(env)) {
				simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
						"handleEvent: call failed");
				(*env)->ExceptionDescribe(env);
				res = -2;
			}
			java_deleteGlobalRef(env, jnaPointerToData,
					"JNA Pointer to handleEvent data");
		}
		establishSpringEnv();
	}

	return res;
}
