/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "JavaBridge.h"

#include "InterfaceDefines.h"
#include "JvmLocater.h"
#include "JniUtil.h"
#include "EventsJNIBridge.h"
#include "CUtils/Util.h"
#include "CUtils/SimpleLog.h"
#include "CUtils/SharedLibrary.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SAIInterfaceCallback.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "System/SafeCStrings.h"
#include "lib/streflop/streflopC.h"

#include <jni.h>

#include <string.h>	// strlen(), strcat(), strcpy()
#include <stdlib.h>	// malloc(), calloc(), free()
#include <assert.h>

struct Properties {
	size_t       size;
	const char** keys;
	const char** values;
};

static int interfaceId = -1;
static const struct SAIInterfaceCallback* callback = NULL;
static struct Properties* jvmCfgProps = NULL;

static size_t  skirmishAIId_sizeMax = 0;
static size_t* skirmishAIId_skirmishAiImpl;

/**
 * Marks the actual storage capacity limit for AI implementations.
 * There can not be more then this many implementations in use at any time.
 */
static size_t   skirmishAiImpl_sizeMax = 0;
/**
 * No more implementations can be found at indices equal or greater to this one,
 * though there can also be free indices lower then this one.
 */
static size_t   skirmishAiImpl_size = 0;
static char**   skirmishAiImpl_className;
static jobject* skirmishAiImpl_instance;
static jobject* skirmishAiImpl_classLoader;

// vars used to integrate the JVM
// it is loaded at runtime, not at loadtime
static sharedLib_t jvmSharedLib = NULL;
typedef jint (JNICALL JNI_GetDefaultJavaVMInitArgs_t)(void* vmArgs);
typedef jint (JNICALL JNI_CreateJavaVM_t)(JavaVM** vm, void** jniEnv, void* vmArgs);
typedef jint (JNICALL JNI_GetCreatedJavaVMs_t)(JavaVM** vms, jsize vms_sizeMax, jsize* vms_size);
static JNI_GetDefaultJavaVMInitArgs_t* JNI_GetDefaultJavaVMInitArgs_f;
static JNI_CreateJavaVM_t*             JNI_CreateJavaVM_f;
static JNI_GetCreatedJavaVMs_t*        JNI_GetCreatedJavaVMs_f;


// ### JNI global vars ###

/// Java VM instance reference
static JavaVM* g_jvm = NULL;

/// AI Callback class
static jclass g_cls_aiCallback = NULL;
/// AI Callback Constructor: AICallback(int skirmishAIId)
static jmethodID g_m_aiCallback_ctor_I = NULL;

/// Basic AI interface.
static jclass g_cls_ai_int = NULL;


// ### General helper functions following ###

/// Sets the FPU state to how spring likes it
static inline void java_establishSpringEnv() {

	//(*g_jvm)->DetachCurrentThread(g_jvm);
	streflop_init_Simple();
}
/// The JVM sets the environment it wants automatically, so this is a no-op
static inline void java_establishJavaEnv() {}


static inline size_t minSize(size_t size1, size_t size2) {
	return (size1 < size2) ? size1 : size2;
}

static const char* java_getValueByKey(const struct Properties* props, const char* key) {
	return util_map_getValueByKey(props->size, props->keys, props->values, key);
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

/**
 * Creates the AI Interface global Java class path.
 *
 * It will consist of the following:
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/AIInterface.jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/(j)?config/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/(j)?config/[*].jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/(j)?resources/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/(j)?resources/[*].jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/(j)?script/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/(j)?script/[*].jar
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/
 * {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/jlib/[*].jar
 * TODO: {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/jlib/
 * TODO: {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/jlib/[*].jar
 */
static bool java_createClassPath(char* classPathStr, const size_t classPathStr_sizeMax) {

	// the dirs and .jar files in the following array
	// will be concatenated with intermediate path separators
	// to form the classPathStr
	static const size_t classPath_sizeMax = 128;
	char**              classPath = (char**) calloc(classPath_sizeMax, sizeof(char*));
	size_t              classPath_size = 0;

	// the Java AI Interfaces java library file path (.../AIInterface.jar)
	// We need to search for this jar, instead of looking only where
	// the AIInterface.so/InterfaceInfo.lua is, because on some systems
	// (eg. Debian), the .so is in /usr/lib, and the .jar's  are in /usr/shared.
	char* mainJarPath = callback->DataDirs_allocatePath(interfaceId,
			JAVA_AI_INTERFACE_LIBRARY_FILE_NAME,
			false, false, false, false);
	if (mainJarPath == NULL) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Couldn't find %s", JAVA_AI_INTERFACE_LIBRARY_FILE_NAME);
		return false;
	}

	classPath[classPath_size++] = util_allocStrCpy(mainJarPath);

	bool ok = util_getParentDir(mainJarPath);
	if (!ok) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Retrieving the parent dir of the path to AIInterface.jar (%s) failed.",
				mainJarPath);
		return false;
	}
	char* jarsDataDir = mainJarPath;
	mainJarPath = NULL;

	// the directories in the following list will be searched for .jar files
	// which will then be added to the classPathStr, plus the dirs will be added
	// to the classPathStr directly, so you can keep .class files in there
	static const size_t jarDirs_sizeMax = 128;
	char**              jarDirs = (char**) calloc(jarDirs_sizeMax, sizeof(char*));
	size_t              jarDirs_size = 0;

	// add to classpath:
	// {spring-data-dir}/Interfaces/Java/0.1/${x}/
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, jarsDataDir, "jconfig");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, jarsDataDir, "config");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, jarsDataDir, "jresources");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, jarsDataDir, "resources");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, jarsDataDir, "jscript");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, jarsDataDir, "script");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, jarsDataDir, "jlib");
	// "lib" is for native libs only

	// add the jar dirs (for .class files) and all contained .jars recursively
	size_t jd, jf;
	for (jd=0; (jd < jarDirs_size) && (classPath_size < classPath_sizeMax); ++jd) {
		if (util_fileExists(jarDirs[jd])) {
			// add the dir directly
			// For this to work properly with URLClassPathHandler,
			// we have to ensure there is a '/' at the end,
			// for the class-path-part to be recognized as a directory.
			classPath[classPath_size++] = util_allocStrCat(2, jarDirs[jd], "/");

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
		STRCAT_T(classPathStr, classPathStr_sizeMax, classPath[0]);
		FREE(classPath[0]);
	}
	size_t cp;
	for (cp=1; cp < classPath_size; ++cp) {
		if (classPath[cp] != NULL) {
			STRCAT_T(classPathStr, classPathStr_sizeMax, ENTRY_DELIM);
			STRCAT_T(classPathStr, classPathStr_sizeMax, classPath[cp]);
			FREE(classPath[cp]);
		}
	}
	FREE(classPath);

	return true;
}

/**
 * Creates a Skirmish AI local Java class path.
 *
 * It will consist of the following:
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/SkirmishAI.jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/(j)?config/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/(j)?config/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/(j)?resources/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/(j)?resources/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/(j)?script/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/(j)?script/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/{ai-version}/jlib/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/(j)?config/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/(j)?config/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/(j)?resources/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/(j)?resources/[*].jar
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/(j)?script/
 * {spring-data-dir}/{SKIRMISH_AI_DATA_DIR}/{ai-name}/common/(j)?script/[*].jar
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
		simpleLog_logL(LOG_LEVEL_ERROR,
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

	// add to classpath ...

	// {spring-data-dir}/Skirmish/MyJavaAI/0.1/SkirmishAI/
	// this can be usefull for AI devs while testing,
	// if they do not want to put everything into a jar all the time
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "SkirmishAI");

	// add to classpath:
	// {spring-data-dir}/Skirmish/MyJavaAI/0.1/${x}/
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "jconfig");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "config");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "jresources");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "resources");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "jscript");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "script");
	jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDD, "jlib");
	// "lib" is for native libs only

	// add the dir common for all versions of the Skirmish AI,
	// if it is specified and exists
	const char* const skirmDDCommon =
			callback->SkirmishAIs_Info_getValueByKey(interfaceId,
			shortName, version,
			SKIRMISH_AI_PROPERTY_DATA_DIR_COMMON);
	if (skirmDDCommon != NULL) {
		// add to classpath:
		// {spring-data-dir}/Skirmish/MyJavaAI/common/${x}/
		jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDDCommon, "jconfig");
		jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDDCommon, "config");
		jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDDCommon, "jresources");
		jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDDCommon, "resources");
		jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDDCommon, "jscript");
		jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDDCommon, "script");
		jarDirs[jarDirs_size++] = util_allocStrCatFSPath(2, skirmDDCommon, "jlib");
		// "lib" is for native libs only
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
			// For this to work properly with URLClassPathHandler,
			// we have to ensure there is a '/' at the end,
			// for the class-path-part to be recognized as a directory.
			classPathParts[classPathParts_size++] = util_allocStrCat(2, jarDirs[jd], "/");

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


	jobject o_jClsLoader = NULL;

	static const size_t classPathParts_sizeMax = 512;
	char**              classPathParts = (char**) calloc(classPathParts_sizeMax, sizeof(char*));
	const size_t        classPathParts_size =
			java_createAIClassPath(shortName, version, classPathParts, classPathParts_sizeMax);

	jobjectArray o_cppURLs = jniUtil_createURLArray(env, classPathParts_size);
	if (o_cppURLs != NULL) {
#ifdef _WIN32
		static const char* FILE_URL_PREFIX = "file:///";
#else // _WIN32
		static const char* FILE_URL_PREFIX = "file://";
#endif // _WIN32
		size_t cpp;
		for (cpp = 0; cpp < classPathParts_size; ++cpp) {
			#ifdef _WIN32
			// we can not use windows path separators in file URLs
			util_strReplaceChar(classPathParts[cpp], '\\', '/');
			#endif

			char* str_fileUrl = util_allocStrCat(2, FILE_URL_PREFIX, classPathParts[cpp]);
			// TODO: check/test if this is allowed/ok
			FREE(classPathParts[cpp]);
			simpleLog_logL(LOG_LEVEL_INFO,
					"Skirmish AI %s %s class-path part %i: \"%s\"",
					shortName, version, cpp, str_fileUrl);
			jobject jurl_fileUrl = jniUtil_createURLObject(env, str_fileUrl);
			FREE(str_fileUrl);
			if (jurl_fileUrl == NULL) {
				simpleLog_logL(LOG_LEVEL_ERROR,
						"Skirmish AI %s %s class-path part %i (\"%s\"): failed to create a URL",
						shortName, version, cpp, str_fileUrl);
				o_cppURLs = NULL;
				break;
			}
			const bool inserted = jniUtil_insertURLIntoArray(env, o_cppURLs, cpp, jurl_fileUrl);
			if (!inserted) {
				simpleLog_logL(LOG_LEVEL_ERROR,
						"Skirmish AI %s %s class-path part %i (\"%s\"): failed to insert",
						shortName, version, cpp, str_fileUrl);
				o_cppURLs = NULL;
				break;
			}
		}
	}

	if (o_cppURLs != NULL) {
		o_jClsLoader = jniUtil_createURLClassLoader(env, o_cppURLs);
		if (o_jClsLoader != NULL) {
			o_jClsLoader = jniUtil_makeGlobalRef(env, o_jClsLoader, "Skirmish AI class-loader");
		}
	}

	FREE(classPathParts);

	return o_jClsLoader;
}

/**
 * Load the interfaces JVM properties file.
 */
static bool java_readJvmCfgFile(struct Properties* props) {

	bool read = false;

	const size_t props_sizeMax = 256;
	props->size   = 0;
	props->keys   = (const char**) calloc(props_sizeMax, sizeof(char*));
	props->values = (const char**) calloc(props_sizeMax, sizeof(char*));

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
		props->size = util_parsePropertiesFile(jvmPropFile,
				props->keys, props->values, props_sizeMax);
		read = true;
		simpleLog_logL(LOG_LEVEL_INFO,
				"JVM: arguments loaded from: %s", jvmPropFile);
	} else {
		props->size = 0;
		read = false;
		simpleLog_logL(LOG_LEVEL_INFO,
				"JVM: arguments NOT loaded from: %s", jvmPropFile);
	}

	FREE(jvmPropFile);

	return read;
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
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Unable to find read-only data-dir.");
		return false;
	} else {
		STRCPY_T(libraryPath, libraryPath_sizeMax, dd_r);
	}

	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/lib/
	char* dd_lib_r = callback->DataDirs_allocatePath(interfaceId,
			NATIVE_LIBS_DIR, false, false, true, false);
	if (dd_lib_r == NULL) {
		simpleLog_logL(LOG_LEVEL_NOTICE,
				"Unable to find read-only native libs data-dir (optional): %s",
				NATIVE_LIBS_DIR);
	} else {
		STRCAT_T(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
		STRCAT_T(libraryPath, libraryPath_sizeMax, dd_lib_r);
		FREE(dd_lib_r);
	}

	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/
	const char* const dd_r_common =
			callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_DATA_DIR_COMMON);
	if (dd_r_common == NULL || !util_fileExists(dd_r_common)) {
		simpleLog_logL(LOG_LEVEL_NOTICE,
				"Unable to find common read-only data-dir (optional).");
	} else {
		STRCAT_T(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
		STRCAT_T(libraryPath, libraryPath_sizeMax, dd_r_common);
	}

	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/lib/
	if (dd_r_common != NULL) {
		char* dd_lib_r_common = callback->DataDirs_allocatePath(interfaceId,
			NATIVE_LIBS_DIR, false, false, true, true);
		if (dd_lib_r_common == NULL || !util_fileExists(dd_lib_r_common)) {
			simpleLog_logL(LOG_LEVEL_NOTICE,
					"Unable to find common read-only native libs data-dir (optional).");
		} else {
			STRCAT_T(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
			STRCAT_T(libraryPath, libraryPath_sizeMax, dd_lib_r_common);
			FREE(dd_lib_r_common);
		}
	}

	return true;
}
static bool java_createJavaVMInitArgs(struct JavaVMInitArgs* vm_args, const struct Properties* jvmProps) {

	// ### evaluate JNI version to use ###
	//jint jniVersion = JNI_VERSION_1_1;
	//jint jniVersion = JNI_VERSION_1_2;
	jint jniVersion = JNI_VERSION_1_4;
	//jint jniVersion = JNI_VERSION_1_6;
	if (jvmProps != NULL) {
		const char* jniVersionFromCfg = java_getValueByKey(jvmProps,
				"jvm.jni.version");
		if (jniVersionFromCfg != NULL) {
			unsigned long int jniVersion_tmp =
					strtoul(jniVersionFromCfg, NULL, 16);
			if (jniVersion_tmp != 0/* && jniVersion_tmp != ULONG_MAX*/) {
				jniVersion = (jint) jniVersion_tmp;
			}
		}
	}
	simpleLog_logL(LOG_LEVEL_INFO, "JVM: JNI version: %#x", jniVersion);
	vm_args->version = jniVersion;

	// ### check if debug related JVM options should be used ###
	// if false, the JVM creation will fail if an
	// unknown or invalid option was specified
	bool useDebugOptions = true;
	const char* useDebugOptionsStr = "auto";
	if (jvmProps != NULL) {
		const char* useDebugOptionsFromCfg =
				util_map_getValueByKey(
				jvmProps->size, jvmProps->keys, jvmProps->values,
				"jvm.useDebugOptions");
		if (useDebugOptionsFromCfg != NULL) {
			useDebugOptionsStr = useDebugOptionsFromCfg;
		}
	}
	{
		if (strcmp(useDebugOptionsStr, "auto") == 0
				|| strcmp(useDebugOptionsStr, "Auto") == 0
				|| strcmp(useDebugOptionsStr, "AUTO") == 0
				|| strcmp(useDebugOptionsStr, "a") == 0
				|| strcmp(useDebugOptionsStr, "A") == 0)
		{
			// auto
#if       defined DEBUG
			useDebugOptions = true;
#else  // defined DEBUG
			useDebugOptions = false;
#endif // defined DEBUG
		} else {
			// true or false
			useDebugOptions = util_strToBool(useDebugOptionsStr);
		}
	}

	// ### check if unrecognized JVM options should be ignored ###
	// if false, the JVM creation will fail if an
	// unknown or invalid option was specified
	bool ignoreUnrecognized = true;
	if (jvmProps != NULL) {
		const char* ignoreUnrecognizedFromCfg = java_getValueByKey(jvmProps,
				"jvm.arguments.ignoreUnrecognized");
		if (ignoreUnrecognizedFromCfg != NULL
				&& !util_strToBool(ignoreUnrecognizedFromCfg)) {
			ignoreUnrecognized = false;
		}
	}
	if (ignoreUnrecognized) {
		simpleLog_logL(LOG_LEVEL_INFO,
				"JVM: ignoring unrecognized options");
		vm_args->ignoreUnrecognized = JNI_TRUE;
	} else {
		simpleLog_logL(LOG_LEVEL_INFO,
				"JVM: NOT ignoring unrecognized options");
		vm_args->ignoreUnrecognized = JNI_FALSE;
	}

	// ### create the Java class-path option ###
	// autogenerate the class path
	static const size_t classPath_sizeMax = 8 * 1024;
	char* classPath = util_allocStr(classPath_sizeMax);
	// ..., autogenerate it ...
	if (!java_createClassPath(classPath, classPath_sizeMax)) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed creating Java class-path.");
		FREE(classPath);
		return false;
	}
	if (jvmProps != NULL) {
		// ..., and append the part from the jvm options properties file,
		// if it is specified there
		const char* clsPathFromCfg = java_getValueByKey(jvmProps,
				"jvm.option.java.class.path");
		if (clsPathFromCfg != NULL) {
			STRCAT_T(classPath, classPath_sizeMax, ENTRY_DELIM);
			STRCAT_T(classPath, classPath_sizeMax, clsPathFromCfg);
		}
	}
	// create the java.class.path option
	static const size_t classPathOpt_sizeMax = 8 * 1024;
	char* classPathOpt = util_allocStr(classPathOpt_sizeMax);
	STRCPY_T(classPathOpt, classPathOpt_sizeMax, "-Djava.class.path=");
	STRCAT_T(classPathOpt, classPathOpt_sizeMax, classPath);
	FREE(classPath);

	// ### create the Java library-path option ###
	// autogenerate the java library path
	static const size_t libraryPath_sizeMax = 4 * 1024;
	char* libraryPath = util_allocStr(libraryPath_sizeMax);
	// ..., autogenerate it ...
	if (!java_createNativeLibsPath(libraryPath, libraryPath_sizeMax)) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed creating Java library-path.");
		FREE(libraryPath);
		return false;
	}
	if (jvmProps != NULL) {
		// ..., and append the part from the jvm options properties file,
		// if it is specified there
		const char* libPathFromCfg = java_getValueByKey(jvmProps,
				"jvm.option.java.library.path");
		if (libPathFromCfg != NULL) {
			STRCAT_T(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
			STRCAT_T(libraryPath, libraryPath_sizeMax, libPathFromCfg);
		}
	}
	// create the java.library.path option ...
	// autogenerate it, and append the part from the jvm options file,
	// if it is specified there
	static const size_t libraryPathOpt_sizeMax = 4 * 1024;
	char* libraryPathOpt = util_allocStr(libraryPathOpt_sizeMax);
	STRCPY_T(libraryPathOpt, libraryPathOpt_sizeMax, "-Djava.library.path=");
	STRCAT_T(libraryPathOpt, libraryPathOpt_sizeMax, libraryPath);
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
	if (jvmProps != NULL) {
		// ### add string options from the JVM config file with property name "jvm.option.x" ###
		int i;
		for (i=0; i < jvmProps->size; ++i) {
			if (strcmp(jvmProps->keys[i], "jvm.option.x") == 0 ||
					(useDebugOptions && (strcmp(jvmProps->keys[i], "jvm.option.debug.x") == 0))) {
				const char* const val = jvmProps->values[i];
				const size_t val_size = strlen(val);
				// ignore "-Djava.class.path=..."
				// and "-Djava.library.path=..." options
				if (strncmp(val, JCPVAL, minSize(val_size, JCPVAL_size)) != 0 &&
					strncmp(val, JLPVAL, minSize(val_size, JLPVAL_size)) != 0) {
					strOptions[op++] = val;
				}
			}
		}
	} else {
		// ### ... or set default ones, if the JVM config file was not found ###
		simpleLog_logL(LOG_LEVEL_WARNING, "JVM: properties file ("JVM_PROPERTIES_FILE") not found; using default options.");

		strOptions[op++] = "-Xms4M";
		strOptions[op++] = "-Xmx64M";
		strOptions[op++] = "-Xss512K";
		strOptions[op++] = "-Xoss400K";

#if       defined DEBUG
		strOptions[op++] = "-Xcheck:jni";
		strOptions[op++] = "-verbose:jni";
		strOptions[op++] = "-XX:+UnlockDiagnosticVMOptions";
		strOptions[op++] = "-XX:+LogVMOutput";

		strOptions[op++] = "-Xdebug";
		strOptions[op++] = "-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=7777";
		// disable JIT (required for debugging under the classical VM)
		strOptions[op++] = "-Djava.compiler=NONE";
		// disable old JDB
		strOptions[op++] = "-Xnoagent";
#endif // defined DEBUG
	}

	const size_t options_size = op;

	vm_args->options = (struct JavaVMOption*) calloc(options_size, sizeof(struct JavaVMOption));

	// fill strOptions into the JVM options
	simpleLog_logL(LOG_LEVEL_INFO, "JVM: options:", options_size);
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
				simpleLog_logL(LOG_LEVEL_INFO, "JVM option %ul: %s", nOptions, tmpOptionString);
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
	simpleLog_logL(LOG_LEVEL_INFO, "");

	return true;
}



static JNIEnv* java_reattachCurrentThread() {

	JNIEnv* env = NULL;

	simpleLog_logL(LOG_LEVEL_DEBUG, "Reattaching current thread...");
	//const jint res = (*g_jvm)->AttachCurrentThreadAsDaemon(g_jvm, (void**) &env, NULL);
	const jint res = (*g_jvm)->AttachCurrentThread(g_jvm, (void**) &env, NULL);
	if (res != 0) {
		env = NULL;
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed attaching JVM to current thread(2): %i - %s",
				res, jniUtil_getJniRetValDescription(res));
	}

	return env;
}

static JNIEnv* java_getJNIEnv() {

	JNIEnv* ret = NULL;

	if (g_jvm == NULL) {
		simpleLog_logL(LOG_LEVEL_INFO, "Creating the JVM.");

		JNIEnv* env = NULL;
		JavaVM* jvm = NULL;
		struct JavaVMInitArgs vm_args;
		jint res = 0;

		if (!java_createJavaVMInitArgs(&vm_args, jvmCfgProps)) {
			simpleLog_logL(LOG_LEVEL_ERROR,
					"Failed initializing JVM init-arguments.");
			goto end;
		}

		// Looking for existing JVMs might be problematic,
		// cause they could be initialized with other
		// JVM-arguments then we need.
		// But as we can not use DestroyJavaVM, cause it makes
		// creating a new one for hte same process impossible
		// (a (SUN?) JVM limitation), we have to do it this way,
		// to support /aireload and /aicontrol for Java Skirmish AIs.
		simpleLog_logL(LOG_LEVEL_INFO, "looking for existing JVMs ...");
		jsize numJVMsFound = 0;

		// jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);
		// Returns all Java VMs that have been created.
		// Pointers to VMs are written in the buffer vmBuf,
		// in the order they were created.
		static const int maxVmsToFind = 1;
		res = JNI_GetCreatedJavaVMs_f(&jvm, maxVmsToFind, &numJVMsFound);
		if (res != 0) {
			jvm = NULL;
			simpleLog_logL(LOG_LEVEL_ERROR,
					"Failed fetching list of running JVMs: %i - %s",
					res, jniUtil_getJniRetValDescription(res));
			goto end;
		}
		simpleLog_logL(LOG_LEVEL_INFO, "number of existing JVMs: %i", numJVMsFound);

		if (numJVMsFound > 0) {
			simpleLog_logL(LOG_LEVEL_INFO, "using an already running JVM.");
		} else {
			simpleLog_logL(LOG_LEVEL_INFO, "creating JVM...");
			res = JNI_CreateJavaVM_f(&jvm, (void**) &env, &vm_args);
			if (res != 0 || (*env)->ExceptionCheck(env)) {
				simpleLog_logL(LOG_LEVEL_ERROR,
						"Failed to create Java VM: %i - %s",
						res, jniUtil_getJniRetValDescription(res));
				goto end;
			}
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
			simpleLog_logL(LOG_LEVEL_ERROR,
					"Failed to attach JVM to current thread: %i - %s",
					res, jniUtil_getJniRetValDescription(res));
			goto end;
		}

end:
		if (env == NULL || jvm == NULL || (*env)->ExceptionCheck(env)
				|| res != 0) {
			simpleLog_logL(LOG_LEVEL_ERROR, "JVM: Failed creating.");
			if (env != NULL && (*env)->ExceptionCheck(env)) {
				(*env)->ExceptionDescribe(env);
			}
			if (jvm != NULL) {
				// Never destroy the JVM, because it can not be created again
				// for the same thread; would allways fail with return value -1
				//res = (*jvm)->DestroyJavaVM(jvm);
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
		simpleLog_logL(LOG_LEVEL_INFO, "JVM: Unloading ...");

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
			simpleLog_logL(LOG_LEVEL_ERROR,
					"JVM: Can not Attach to the current thread: %i - %s",
					res, jniUtil_getJniRetValDescription(res));
			return false;
		}*/

		const jint res = (*g_jvm)->DetachCurrentThread(g_jvm);
		if (res != 0) {
			simpleLog_logL(LOG_LEVEL_ERROR,
					"JVM: Failed detaching current thread: %i - %s",
					res, jniUtil_getJniRetValDescription(res));
			return false;
		}

		// Never destroy the JVM, because it can not be created again
		// for the same thread; would allways fail with return value -1,
		// which would be a problem when using the /aicontrol or /aireload
		// commands on java AIs
		/*res = (*g_jvm)->DestroyJavaVM(g_jvm);
		if (res != 0) {
			simpleLog_logL(LOG_LEVEL_ERROR,
					"JVM: Failed destroying: %i - %s",
					res, jniUtil_getJniRetValDescription(res));
			return false;
		} else {
			simpleLog_logL(LOG_LEVEL_NOTICE,
					"JVM: Successfully destroyed");
			g_jvm = NULL;
		}*/
		java_establishSpringEnv();
	}

	return true;
}


bool java_preloadJNIEnv() {

	java_establishJavaEnv();
	JNIEnv* env = java_getJNIEnv();
	java_establishSpringEnv();

	return env != NULL;
}


bool java_initStatic(int _interfaceId,
		const struct SAIInterfaceCallback* _callback) {

	interfaceId = _interfaceId;
	callback = _callback;

	// Read the jvm properties config file
	jvmCfgProps = (struct Properties*) malloc(sizeof(struct Properties));
	java_readJvmCfgFile(jvmCfgProps);

	skirmishAIId_sizeMax   = callback->SkirmishAIs_getSize(interfaceId);
	skirmishAiImpl_sizeMax = skirmishAIId_sizeMax;
	skirmishAiImpl_size    = 0;

	skirmishAIId_skirmishAiImpl = (size_t*) calloc(skirmishAIId_sizeMax, sizeof(size_t));
	size_t t;
	for (t = 0; t < skirmishAIId_sizeMax; ++t) {
		skirmishAIId_skirmishAiImpl[t] = 999999;
	}

	skirmishAiImpl_className   = (char**)      calloc(skirmishAiImpl_sizeMax, sizeof(char*));
	skirmishAiImpl_instance    = (jobject*)    calloc(skirmishAiImpl_sizeMax, sizeof(jobject));
	skirmishAiImpl_classLoader = (jobject*)    calloc(skirmishAiImpl_sizeMax, sizeof(jobject));
	size_t sai;
	for (sai = 0; sai < skirmishAiImpl_sizeMax; ++sai) {
		skirmishAiImpl_className[sai]   = NULL;
		skirmishAiImpl_instance[sai]    = NULL;
		skirmishAiImpl_classLoader[sai] = NULL;
	}

	// dynamically load the JVM
	char* jreLocationFile = callback->DataDirs_allocatePath(interfaceId,
			JRE_LOCATION_FILE, false, false, false, false);

	static const size_t jrePath_sizeMax = 1024;
	char jrePath[jrePath_sizeMax];
	bool jreFound = GetJREPath(jrePath, jrePath_sizeMax, jreLocationFile, NULL);
	if (!jreFound) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed locating a JRE installation"
				", you may specify the JAVA_HOME env var.");
		return false;
	} else {
		simpleLog_logL(LOG_LEVEL_NOTICE,
				"Using JRE (can be changed with JAVA_HOME): %s", jrePath);
	}
	FREE(jreLocationFile);

#if defined __arch64__
	static const char* defJvmType = "server";
#else
	static const char* defJvmType = "client";
#endif
	const char* jvmType = java_getValueByKey(jvmCfgProps, "jvm.type");
	if (jvmType == NULL) {
		jvmType = defJvmType;
	}

	static const size_t jvmLibPath_sizeMax = 1024;
	char jvmLibPath[jvmLibPath_sizeMax];
	bool jvmLibFound = GetJVMPath(jrePath, jvmType, jvmLibPath,
			jvmLibPath_sizeMax, NULL);
	if (!jvmLibFound) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed locating the %s version of the JVM, please contact spring devs.", jvmType);
		return false;
	}

	jvmSharedLib = sharedLib_load(jvmLibPath);
	if (!sharedLib_isLoaded(jvmSharedLib)) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed to load the JVM at \"%s\".", jvmLibPath);
		return false;
	} else {
		simpleLog_logL(LOG_LEVEL_NOTICE,
				"Successfully loaded the JVM shared library at \"%s\".", jvmLibPath);

		JNI_GetDefaultJavaVMInitArgs_f = (JNI_GetDefaultJavaVMInitArgs_t*)
				sharedLib_findAddress(jvmSharedLib, "JNI_GetDefaultJavaVMInitArgs");
		if (JNI_GetDefaultJavaVMInitArgs_f == NULL) {
			simpleLog_logL(LOG_LEVEL_ERROR,
					"Failed to load the JVM, function \"%s\" not exported.", "JNI_GetDefaultJavaVMInitArgs");
			return false;
		}

		JNI_CreateJavaVM_f = (JNI_CreateJavaVM_t*)
				sharedLib_findAddress(jvmSharedLib, "JNI_CreateJavaVM");
		if (JNI_CreateJavaVM_f == NULL) {
			simpleLog_logL(LOG_LEVEL_ERROR,
					"Failed to load the JVM, function \"%s\" not exported.", "JNI_CreateJavaVM");
			return false;
		}

		JNI_GetCreatedJavaVMs_f = (JNI_GetCreatedJavaVMs_t*)
				sharedLib_findAddress(jvmSharedLib, "JNI_GetCreatedJavaVMs");
		if (JNI_GetCreatedJavaVMs_f == NULL) {
			simpleLog_logL(LOG_LEVEL_ERROR,
					"Failed to load the JVM, function \"%s\" not exported.", "JNI_GetCreatedJavaVMs");
			return false;
		}
	}

	java_establishJavaEnv();
	JNIEnv* env = java_getJNIEnv();
	bool success = (env != NULL);
	if (success) {
		const int res = eventsJniBridge_initStatic(env, skirmishAIId_sizeMax);
		success = (res == 0);
	}
	java_establishSpringEnv();

	return success;
}

static jobject java_createAICallback(JNIEnv* env, const struct SSkirmishAICallback* aiCallback, int skirmishAIId) {

	jobject o_clb = NULL;

	// initialize the AI Callback class, if not yet done
	if (g_cls_aiCallback == NULL) {
		// get the AI Callback class
		g_cls_aiCallback = jniUtil_findClass(env, CLS_AI_CALLBACK);
		if (g_cls_aiCallback == NULL) { return NULL; }
		g_cls_aiCallback = jniUtil_makeGlobalRef(env, g_cls_aiCallback,
				CLS_AI_CALLBACK);
		if (g_cls_aiCallback == NULL) { return NULL; }

		// get (int skirmishAIId) constructor
		g_m_aiCallback_ctor_I = jniUtil_getMethodID(env, g_cls_aiCallback, "<init>", "(I)V");
		if (g_m_aiCallback_ctor_I == NULL) { return NULL; }
	}

	o_clb = (*env)->NewObject(env, g_cls_aiCallback, g_m_aiCallback_ctor_I, skirmishAIId);
	if (jniUtil_checkException(env, "Failed creating Java AI Callback instance")) {
		o_clb = NULL;
	}/* else {
		o_clb = jniUtil_makeGlobalRef(env, o_clb, "AI callback instance");
	}*/

	return o_clb;
}


bool java_releaseStatic() {

	FREE(skirmishAIId_skirmishAiImpl);

	FREE(skirmishAiImpl_className);
	FREE(skirmishAiImpl_instance);
	FREE(skirmishAiImpl_classLoader);

	sharedLib_unload(jvmSharedLib);
	jvmSharedLib = NULL;

	FREE(jvmCfgProps->keys);
	FREE(jvmCfgProps->values);
	FREE(jvmCfgProps);

	return true;
}


static bool java_loadSkirmishAI(JNIEnv* env,
		const char* shortName, const char* version, const char* className,
		jobject* o_ai, jobject* o_aiClassLoader) {

/*	// convert className from "com.myai.AI" to "com/myai/AI"
	const size_t classNameP_sizeMax = strlen(className) + 1;
	char classNameP[classNameP_sizeMax];
	STRCPY_T(classNameP, classNameP_sizeMax, className);
	util_strReplaceChar(classNameP, '.', '/');*/

	// get the AIs private class-loader
	jobject o_global_aiClassLoader =
			java_createAIClassLoader(env, shortName, version);
	if (o_global_aiClassLoader == NULL) { return false; }
	*o_aiClassLoader = o_global_aiClassLoader;

	// get the AI interface (from AIInterface.jar)
	if (g_cls_ai_int == NULL) {
		g_cls_ai_int = jniUtil_findClass(env, INT_AI);
		if (g_cls_ai_int == NULL) { return false; }
		g_cls_ai_int = jniUtil_makeGlobalRef(env, g_cls_ai_int, "AI interface class");
		if (g_cls_ai_int == NULL) { return false; }
	}

	// get the AI implementation class (from SkirmishAI.jar)
	jclass cls_ai = jniUtil_findClassThroughLoader(env, o_global_aiClassLoader,
			className);
	if (cls_ai == NULL) { return false; }

	const bool implementsAIInt = (bool) (*env)->IsAssignableFrom(env, cls_ai, g_cls_ai_int);
	bool hasException = (*env)->ExceptionCheck(env);
	if (!implementsAIInt || hasException) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"AI class not assignable from interface "INT_AI": %s", className);
		simpleLog_logL(LOG_LEVEL_ERROR, "possible reasons (this list could be incomplete):");
		simpleLog_logL(LOG_LEVEL_ERROR, "* "INT_AI" interface not implemented");
		simpleLog_logL(LOG_LEVEL_ERROR, "* The AI is not compiled for the Java AI Interface version in use");
		if (hasException) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// get factory no-arg ctor
	jmethodID m_ai_ctor = jniUtil_getMethodID(env, cls_ai, "<init>", "()V");
	if (m_ai_ctor == NULL) { return false; }

	// get AI instance
	jobject o_local_ai = (*env)->NewObject(env, cls_ai, m_ai_ctor);
	hasException = (*env)->ExceptionCheck(env);
	if (!o_local_ai || hasException) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed fetching AI instance for class: %s", className);
		if (hasException) {
			(*env)->ExceptionDescribe(env);
		}
		return false;
	}

	// make the AI a global reference,
	// so it will not be garbage collected,
	// even after this method returned
	*o_ai = jniUtil_makeGlobalRef(env, o_local_ai, "AI instance");

	return true;
}


bool java_initSkirmishAIClass(
		const char* const shortName,
		const char* const version,
		const char* const className,
		int skirmishAIId) {

	bool success = false;

	// see if an AI for className is instantiated already
	size_t sai;
	size_t firstFree = skirmishAiImpl_size;
	for (sai = 0; sai < skirmishAiImpl_size; ++sai) {
		if (skirmishAiImpl_className[sai] == NULL) {
			firstFree = sai;
		}
	}
	// sai is now either the instantiated one, or a free one

	// instantiate AI (if not already instantiated)
	assert(sai < skirmishAiImpl_sizeMax);
	if (skirmishAiImpl_className[sai] == NULL) {
		sai = firstFree;
		java_establishJavaEnv();
		JNIEnv* env = java_getJNIEnv();

		jobject    instance    = NULL;
		jobject    classLoader = NULL;

		success = java_loadSkirmishAI(env, shortName, version, className,
				&(instance), &(classLoader));
		java_establishSpringEnv();

		if (success) {
			skirmishAiImpl_instance[sai]    = instance;
			skirmishAiImpl_classLoader[sai] = classLoader;
			skirmishAiImpl_className[sai]   = util_allocStrCpy(className);
			if (firstFree == skirmishAiImpl_size) {
				skirmishAiImpl_size++;
			}
		} else {
			simpleLog_logL(LOG_LEVEL_ERROR,
					"Class loading failed for class: %s", className);
		}
	} else {
		success = true;
	}

	if (success) {
		skirmishAIId_skirmishAiImpl[skirmishAIId] = sai;
	}

	return success;
}

bool java_releaseSkirmishAIClass(const char* className) {

	bool success = false;

	// see if an AI for className is instantiated
	size_t sai;
	for (sai = 0; sai < skirmishAiImpl_size; ++sai) {
		if ((skirmishAiImpl_className[sai] != NULL) &&
				(strcmp(skirmishAiImpl_className[sai], className) == 0))
		{
			break;
		}
	}
	// sai is now either the instantiated one, or a free one

	// release AI (if its instance was found)
	assert(sai < skirmishAiImpl_sizeMax);
	if (skirmishAiImpl_className[sai] != NULL) {
		java_establishJavaEnv();
		JNIEnv* env = java_getJNIEnv();

		bool successPart;

		// delete the AI class-loader global reference,
		// so it will be garbage collected
		successPart = jniUtil_deleteGlobalRef(env,
				skirmishAiImpl_classLoader[sai], "AI class-loader");
		success = successPart;

		// delete the AI global reference,
		// so it will be garbage collected
		successPart = jniUtil_deleteGlobalRef(env, skirmishAiImpl_instance[sai],
				"AI instance");
		success = success && successPart;
		java_establishSpringEnv();

		if (success) {
			skirmishAiImpl_classLoader[sai] = NULL;
			skirmishAiImpl_instance[sai] = NULL;
			FREE(skirmishAiImpl_className[sai]);
			// if it is the last implementation
			if (sai+1 == skirmishAiImpl_size) {
				skirmishAiImpl_size--;
			}
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


int java_skirmishAI_init(int skirmishAIId,
		const struct SSkirmishAICallback* aiCallback) {

	int res = -1;

	java_establishJavaEnv();
	JNIEnv* env = java_getJNIEnv();
	jobject global_javaAICallback = java_createAICallback(env, aiCallback, skirmishAIId);
	if (global_javaAICallback != NULL) {
		res = eventsJniBridge_initAI(env, skirmishAIId, global_javaAICallback);
	}
	java_establishSpringEnv();

	return res;
}

int java_skirmishAI_release(int skirmishAIId) {

	int res = 0;

	return res;
}

int java_skirmishAI_handleEvent(int skirmishAIId, int topic, const void* data) {

	java_establishJavaEnv();
	JNIEnv* env = java_getJNIEnv();
	const size_t sai   = skirmishAIId_skirmishAiImpl[skirmishAIId];
	jobject aiInstance = skirmishAiImpl_instance[sai];
	int res = eventsJniBridge_handleEvent(env, aiInstance, skirmishAIId, topic, data);
	java_establishSpringEnv();

	return res;
}
