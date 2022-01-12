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
static struct Properties jvmCfgProps = {0, NULL, NULL};

static       size_t numSkirmishAIs = 0;
// static const size_t maxSkirmishAIs = 255; // MAX_AIS
#define maxSkirmishAIs 255

static size_t skirmishAIId_skirmishAiImpl[maxSkirmishAIs] = {999999};

static char* jAIClassNames[maxSkirmishAIs] = {NULL};

static jobject jAIInstances[maxSkirmishAIs] = {NULL};
static jobject jAIClassLoaders[maxSkirmishAIs] = {NULL};
static jobject jAICallBacks[maxSkirmishAIs] = {NULL};

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
	// only detach in java_unloadJNIEnv
	// (*g_jvm)->DetachCurrentThread(g_jvm);
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
static bool java_createClassPath(char* classPathStr, const size_t classPathStr_sizeMax)
{
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
		simpleLog_logL(LOG_LEVEL_ERROR, "Couldn't find %s", JAVA_AI_INTERFACE_LIBRARY_FILE_NAME);
		return false;
	}

	classPath[classPath_size++] = util_allocStrCpy(mainJarPath);

	if (!util_getParentDir(mainJarPath)) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Retrieving the parent dir of the path to AIInterface.jar (%s) failed.", mainJarPath);
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
	for (jd = 0; (jd < jarDirs_size) && (classPath_size < classPath_sizeMax); ++jd) {
		if (util_fileExists(jarDirs[jd])) {
			// add the dir directly
			// For this to work properly with URLClassPathHandler,
			// we have to ensure there is a '/' at the end,
			// for the class-path-part to be recognized as a directory.
			classPath[classPath_size++] = util_allocStrCat(2, jarDirs[jd], "/");

			// add the contained jars recursively
			static const size_t jarFiles_sizeMax = 128;
			char**              jarFiles = (char**) calloc(jarFiles_sizeMax, sizeof(char*));
			const size_t        jarFiles_size = util_listFiles(jarDirs[jd], ".jar", jarFiles, true, jarFiles_sizeMax);

			for (jf = 0; (jf < jarFiles_size) && (classPath_size < classPath_sizeMax); ++jf) {
				classPath[classPath_size++] = util_allocStrCatFSPath(2, jarDirs[jd], jarFiles[jf]);
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

	for (cp = 1; cp < classPath_size; ++cp) {
		if (classPath[cp] == NULL)
			continue;

		STRCAT_T(classPathStr, classPathStr_sizeMax, ENTRY_DELIM);
		STRCAT_T(classPathStr, classPathStr_sizeMax, classPath[cp]);
		FREE(classPath[cp]);
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
static size_t java_createAIClassPath(
	const char* shortName,
	const char* version,
	char** classPathParts,
	const size_t classPathParts_sizeMax
) {
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
	for (jd = 0; (jd < jarDirs_size) && (classPathParts_size < classPathParts_sizeMax); ++jd) {
		if (jarDirs[jd] != NULL && util_fileExists(jarDirs[jd])) {
			// add the jar dir (for .class files)
			// For this to work properly with URLClassPathHandler,
			// we have to ensure there is a '/' at the end,
			// for the class-path-part to be recognized as a directory.
			classPathParts[classPathParts_size++] = util_allocStrCat(2, jarDirs[jd], "/");

			// add the jars in the dir
			const size_t subJarFiles_sizeMax = classPathParts_sizeMax - classPathParts_size;
			char**       subJarFiles = (char**) calloc(subJarFiles_sizeMax, sizeof(char*));
			const size_t subJarFiles_size = util_listFiles(jarDirs[jd], ".jar", subJarFiles, true, subJarFiles_sizeMax);

			for (sjf = 0; (sjf < subJarFiles_size) && (classPathParts_size < classPathParts_sizeMax); ++sjf) {
				// .../[*].jar
				classPathParts[classPathParts_size++] = util_allocStrCatFSPath(2, jarDirs[jd], subJarFiles[sjf]);
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

static jobject java_createAIClassLoader(JNIEnv* env, const char* shortName, const char* version)
{
	static const size_t classPathParts_sizeMax = 512;
	char**              classPathParts = (char**) calloc(classPathParts_sizeMax, sizeof(char*));
	const size_t        classPathParts_size = java_createAIClassPath(shortName, version, classPathParts, classPathParts_sizeMax);

	jobject o_jClsLoader = NULL;
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
		if ((o_jClsLoader = jniUtil_createURLClassLoader(env, o_cppURLs)) != NULL)
			o_jClsLoader = jniUtil_makeGlobalRef(env, o_jClsLoader, "Skirmish AI class-loader");
	}

	FREE(classPathParts);
	return o_jClsLoader;
}

/**
 * Load the interfaces JVM properties file.
 */
static bool java_readJvmCfgFile(struct Properties* props)
{
	bool read = false;

	const size_t props_sizeMax = 256;

	props->size   = 0;
	props->keys   = (const char**) calloc(props_sizeMax, sizeof(char*));
	props->values = (const char**) calloc(props_sizeMax, sizeof(char*));

	// ### read JVM options config file ###
	char* jvmPropFile = callback->DataDirs_allocatePath(interfaceId, JVM_PROPERTIES_FILE, false, false, false, false);

	// if the version specific file does not exist,
	// try to get the common one
	if (jvmPropFile == NULL)
		jvmPropFile = callback->DataDirs_allocatePath(interfaceId, JVM_PROPERTIES_FILE, false, false, false, true);

	if (jvmPropFile != NULL) {
		props->size = util_parsePropertiesFile(jvmPropFile, props->keys, props->values, props_sizeMax);
		read = true;
		simpleLog_logL(LOG_LEVEL_INFO, "JVM: arguments loaded from: %s", jvmPropFile);
	} else {
		props->size = 0;
		read = false;
		simpleLog_logL(LOG_LEVEL_INFO, "JVM: arguments NOT loaded from: %s", jvmPropFile);
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
static bool java_createNativeLibsPath(char* libraryPath, const size_t libraryPath_sizeMax)
{
	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/
	const char* const dd_r = callback->AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_DATA_DIR);

	if (dd_r == NULL) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Unable to find read-only data-dir.");
		return false;
	}

	STRCPY_T(libraryPath, libraryPath_sizeMax, dd_r);

	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/{version}/lib/
	char* dd_lib_r = callback->DataDirs_allocatePath(interfaceId, NATIVE_LIBS_DIR, false, false, true, false);

	if (dd_lib_r == NULL) {
		simpleLog_logL(LOG_LEVEL_NOTICE, "Unable to find read-only native libs data-dir (optional): %s", NATIVE_LIBS_DIR);
	} else {
		STRCAT_T(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
		STRCAT_T(libraryPath, libraryPath_sizeMax, dd_lib_r);
		FREE(dd_lib_r);
	}


	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/
	const char* const dd_r_common = callback->AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_DATA_DIR_COMMON);

	if (dd_r_common == NULL || !util_fileExists(dd_r_common)) {
		simpleLog_logL(LOG_LEVEL_NOTICE, "Unable to find common read-only data-dir (optional).");
	} else {
		STRCAT_T(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
		STRCAT_T(libraryPath, libraryPath_sizeMax, dd_r_common);
	}


	// {spring-data-dir}/{AI_INTERFACES_DATA_DIR}/Java/common/lib/
	if (dd_r_common != NULL) {
		char* dd_lib_r_common = callback->DataDirs_allocatePath(interfaceId, NATIVE_LIBS_DIR, false, false, true, true);

		if (dd_lib_r_common == NULL || !util_fileExists(dd_lib_r_common)) {
			simpleLog_logL(LOG_LEVEL_NOTICE, "Unable to find common read-only native libs data-dir (optional).");
		} else {
			STRCAT_T(libraryPath, libraryPath_sizeMax, ENTRY_DELIM);
			STRCAT_T(libraryPath, libraryPath_sizeMax, dd_lib_r_common);
			FREE(dd_lib_r_common);
		}
	}

	return true;
}


static bool java_createJavaVMInitArgs(struct JavaVMInitArgs* vm_args, const struct Properties* jvmProps)
{
	// jint jniVersion = JNI_VERSION_1_1;
	// jint jniVersion = JNI_VERSION_1_2;
	jint jniVersion = JNI_VERSION_1_4;
	// jint jniVersion = JNI_VERSION_1_6;

	char   classPath   [8 * 1024] = {0};
	char   classPathOpt[8 * 1024] = {0};
	char libraryPath   [4 * 1024] = {0};
	char libraryPathOpt[4 * 1024] = {0};

	bool ignoreUnrecognized = true;

	if (jvmProps != NULL) {
		const char* jniVersionFromCfg = java_getValueByKey(jvmProps, "jvm.jni.version");
		const char* ignoreUnrecognizedFromCfg = java_getValueByKey(jvmProps, "jvm.arguments.ignoreUnrecognized");

		// ### evaluate JNI version to use ###
		if (jniVersionFromCfg != NULL) {
			const unsigned long int jniVersion_tmp = strtoul(jniVersionFromCfg, NULL, 16);

			if (jniVersion_tmp != 0 /*&& jniVersion_tmp != ULONG_MAX*/)
				jniVersion = (jint) jniVersion_tmp;
		}

		// ### check if unrecognized JVM options should be ignored ###
		// if false, the JVM creation will fail if an
		// unknown or invalid option was specified
		if (ignoreUnrecognizedFromCfg != NULL && !util_strToBool(ignoreUnrecognizedFromCfg))
			ignoreUnrecognized = false;
	}

	simpleLog_logL(LOG_LEVEL_INFO, "JVM: JNI version: %#x", jniVersion);
	vm_args->version = jniVersion;


	if (ignoreUnrecognized) {
		simpleLog_logL(LOG_LEVEL_INFO, "JVM: ignoring unrecognized options");
		vm_args->ignoreUnrecognized = JNI_TRUE;
	} else {
		simpleLog_logL(LOG_LEVEL_INFO, "JVM: NOT ignoring unrecognized options");
		vm_args->ignoreUnrecognized = JNI_FALSE;
	}


	// ### create the Java class-path option ###
	// autogenerate the class path
	if (!java_createClassPath(classPath, sizeof(classPath))) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed creating Java class-path.");
		return false;
	}
	if (jvmProps != NULL) {
		// ..., and append the part from the jvm options properties file,
		// if it is specified there
		const char* clsPathFromCfg = java_getValueByKey(jvmProps, "jvm.option.java.class.path");

		if (clsPathFromCfg != NULL) {
			STRCAT_T(classPath, sizeof(classPath), ENTRY_DELIM);
			STRCAT_T(classPath, sizeof(classPath), clsPathFromCfg);
		}
	}


	// create the java.class.path option
	STRCPY_T(classPathOpt, sizeof(classPathOpt), "-Djava.class.path=");
	STRCAT_T(classPathOpt, sizeof(classPathOpt), classPath);


	// ### create the Java library-path option ###
	// autogenerate the java library path
	if (!java_createNativeLibsPath(libraryPath, sizeof(libraryPath))) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed creating Java library-path.");
		return false;
	}
	if (jvmProps != NULL) {
		// ..., and append the part from the jvm options properties file,
		// if it is specified there
		const char* libPathFromCfg = java_getValueByKey(jvmProps, "jvm.option.java.library.path");

		if (libPathFromCfg != NULL) {
			STRCAT_T(libraryPath, sizeof(libraryPath), ENTRY_DELIM);
			STRCAT_T(libraryPath, sizeof(libraryPath), libPathFromCfg);
		}
	}

	// create the java.library.path option ...
	// autogenerate it, and append the part from the jvm options file,
	// if it is specified there
	STRCPY_T(libraryPathOpt, sizeof(libraryPathOpt), "-Djava.library.path=");
	STRCAT_T(libraryPathOpt, sizeof(libraryPathOpt), libraryPath);


	// ### create and set all JVM options ###
	const char* strOptions[64];
	size_t numOpts = 0;

	strOptions[numOpts++] = classPathOpt;
	strOptions[numOpts++] = libraryPathOpt;

	static const char* const JCPVAL = "-Djava.class.path=";
	static const char* const JLPVAL = "-Djava.library.path=";
	const size_t JCPVAL_size = strlen(JCPVAL);
	const size_t JLPVAL_size = strlen(JCPVAL);

	if (jvmProps != NULL) {
		// ### add string options from the JVM config file with property name "jvm.option.x" ###
		int i;
		for (i = 0; i < jvmProps->size; ++i) {
			if (strcmp(jvmProps->keys[i], "jvm.option.x") != 0)
				continue;

			const char* const val = jvmProps->values[i];
			const size_t val_size = strlen(val);
			// ignore "-Djava.class.path=..."
			// and "-Djava.library.path=..." options
			if (strncmp(val, JCPVAL, minSize(val_size, JCPVAL_size)) != 0 &&
				strncmp(val, JLPVAL, minSize(val_size, JLPVAL_size)) != 0) {
				strOptions[numOpts++] = val;
			}
		}
	} else {
		// ### ... or set default ones, if the JVM config file was not found ###
		simpleLog_logL(LOG_LEVEL_WARNING, "JVM: properties file ("JVM_PROPERTIES_FILE") not found; using default options.");

		strOptions[numOpts++] = "-Xms64M";
		strOptions[numOpts++] = "-Xmx512M";
		strOptions[numOpts++] = "-Xss512K";
		strOptions[numOpts++] = "-Xoss400K";

		#if defined DEBUG
		strOptions[numOpts++] = "-Xcheck:jni";
		strOptions[numOpts++] = "-verbose:jni";
		strOptions[numOpts++] = "-XX:+UnlockDiagnosticVMOptions";
		strOptions[numOpts++] = "-XX:+LogVMOutput";

		strOptions[numOpts++] = "-Xdebug";
		strOptions[numOpts++] = "-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=7777";
		// disable JIT (required for debugging under the classical VM)
		strOptions[numOpts++] = "-Djava.compiler=NONE";
		// disable old JDB
		strOptions[numOpts++] = "-Xnoagent";
		#endif // defined DEBUG
	}

	vm_args->options = (struct JavaVMOption*) calloc(numOpts, sizeof(struct JavaVMOption));
	vm_args->nOptions = 0;

	// fill strOptions into the JVM options
	simpleLog_logL(LOG_LEVEL_INFO, "JVM: options:", numOpts);
	char* dd_rw = callback->DataDirs_allocatePath(interfaceId, "", true, true, true, false);
	size_t i;

	for (i = 0; i < numOpts; ++i) {
		char* tmpOptionString = util_allocStrReplaceStr(strOptions[i], "${home-dir}", dd_rw);

		// do not add empty options
		if (tmpOptionString == NULL)
			continue;

		if (strlen(tmpOptionString) == 0) {
			free(tmpOptionString);
			tmpOptionString = NULL;
			continue;
		}

		vm_args->options[vm_args->nOptions  ].optionString = tmpOptionString;
		vm_args->options[vm_args->nOptions++].extraInfo = NULL;

		simpleLog_logL(LOG_LEVEL_INFO, "JVM option %ul: %s", vm_args->nOptions - 1, tmpOptionString);
	}

	FREE(dd_rw);

	simpleLog_logL(LOG_LEVEL_INFO, "");
	return true;
}



static JNIEnv* java_reattachCurrentThread(JavaVM* jvm)
{
	simpleLog_logL(LOG_LEVEL_DEBUG, "Reattaching current thread...");

	JNIEnv* env = NULL;

	// const jint res = (*jvm)->AttachCurrentThreadAsDaemon(jvm, (void**) &env, NULL);
	const jint res = (*jvm)->AttachCurrentThread(jvm, (void**) &env, NULL);

	if (res != 0) {
		env = NULL;
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed attaching JVM to current thread(2): %i - %s", res, jniUtil_getJniRetValDescription(res));
	}

	return env;
}

static JNIEnv* java_getJNIEnv(bool preload)
{
	if (g_jvm != NULL)
		return java_reattachCurrentThread(g_jvm);

	assert(preload);
	simpleLog_logL(LOG_LEVEL_INFO, "Creating the JVM.");

	JNIEnv* ret = NULL;
	JNIEnv* env = NULL;
	JavaVM* jvm = NULL;

	struct JavaVMInitArgs vm_args;
	memset(&vm_args, 0, sizeof(vm_args));

	jint res = 0;

	if (!java_createJavaVMInitArgs(&vm_args, &jvmCfgProps)) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed initializing JVM init-arguments.");
		goto end;
	}

	// Looking for existing JVMs might be problematic: they could
	// have been initialized with other JVM-arguments then we need.
	// But as we can not use DestroyJavaVM (it makes creating a new
	// one for the same process impossible, a (SUN?) JVM limitation)
	// we have to do this anyway to support /aireload and /aicontrol
	// for Java Skirmish AIs.
	//
	simpleLog_logL(LOG_LEVEL_INFO, "looking for existing JVMs ...");
	jsize numJVMsFound = 0;

	// grab the first Java VM that has been created
	if ((res = JNI_GetCreatedJavaVMs_f(&jvm, 1, &numJVMsFound)) != 0) {
		jvm = NULL;
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed fetching list of running JVMs: %i - %s", res, jniUtil_getJniRetValDescription(res));
		goto end;
	}

	simpleLog_logL(LOG_LEVEL_INFO, "number of existing JVMs: %i", numJVMsFound);

	if (numJVMsFound > 0) {
		simpleLog_logL(LOG_LEVEL_INFO, "using an already running JVM.");
	} else {
		simpleLog_logL(LOG_LEVEL_INFO, "creating JVM...");

		if ((res = JNI_CreateJavaVM_f(&jvm, (void**) &env, &vm_args)) != 0 || env == NULL || (*env)->ExceptionCheck(env)) {
			simpleLog_logL(LOG_LEVEL_ERROR, "Failed to create Java VM: %i - %s", res, jniUtil_getJniRetValDescription(res));
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
		if ((*env)->ExceptionCheck(env))
			(*env)->ExceptionDescribe(env);

		simpleLog_logL(LOG_LEVEL_ERROR, "Failed to attach JVM to current thread: %i - %s", res, jniUtil_getJniRetValDescription(res));
		goto end;
	}

end:
	if (env == NULL || jvm == NULL || (*env)->ExceptionCheck(env) || res != 0) {
		simpleLog_logL(LOG_LEVEL_ERROR, "JVM: Failed creating.");

		if (env != NULL && (*env)->ExceptionCheck(env))
			(*env)->ExceptionDescribe(env);

		if (jvm != NULL) {
			// never destroy the JVM, doing so prevents it from being created again for the same thread
			// res = (*jvm)->DestroyJavaVM(jvm);
		}

		g_jvm = NULL;
		ret = NULL;
	} else {
		g_jvm = jvm;
		ret = env;
	}

	return ret;
}


bool java_unloadJNIEnv()
{
	if (g_jvm == NULL)
		return true;

	simpleLog_logL(LOG_LEVEL_INFO, "JVM: Unloading ...");

	#if 0
	// JNIEnv* jniEnv = java_getJNIEnv(false);

	// We have to be the ONLY running thread (native and Java)
	// this may not help, but will not hurt either
	//
	// // jint res = (*g_jvm)->AttachCurrentThreadAsDaemon(g_jvm, (void**) &g_jniEnv, NULL);
	// jint res = jvm->AttachCurrentThread((void**) &jniEnv, NULL);
	#endif

	#if 0
	if (res < 0 || (*g_jniEnv)->ExceptionCheck(g_jniEnv)) {
		if ((*g_jniEnv)->ExceptionCheck(g_jniEnv))
			(*g_jniEnv)->ExceptionDescribe(g_jniEnv);

		simpleLog_logL(LOG_LEVEL_ERROR, "JVM: Can not Attach to the current thread: %i - %s", res, jniUtil_getJniRetValDescription(res));
		return false;
	}
	#endif


	const jint res = (*g_jvm)->DetachCurrentThread(g_jvm);

	if (res != 0) {
		simpleLog_logL(LOG_LEVEL_ERROR, "JVM: Failed detaching current thread: %i - %s", res, jniUtil_getJniRetValDescription(res));
		return false;
	}

	// never destroy the JVM because then it can not be created again
	// in the same thread; will always fail with return value -1 (see
	// java_getJNIEnv)
	#if 0
	if ((res = (*g_jvm)->DestroyJavaVM(g_jvm)) != 0) {
		simpleLog_logL(LOG_LEVEL_ERROR, "JVM: Failed destroying: %i - %s", res, jniUtil_getJniRetValDescription(res));
		return false;
	}

	simpleLog_logL(LOG_LEVEL_NOTICE, "JVM: Successfully destroyed");
	g_jvm = NULL;
	#endif

	java_establishSpringEnv();
	return true;
}


bool java_initStatic(int _interfaceId, const struct SAIInterfaceCallback* _callback)
{
	interfaceId = _interfaceId;
	callback = _callback;

	// Read the jvm properties config file
	java_readJvmCfgFile(&jvmCfgProps);

	size_t t;
	for (t = 0; t < maxSkirmishAIs; ++t) {
		skirmishAIId_skirmishAiImpl[t] = 999999;
	}

	size_t sai;
	for (sai = 0; sai < maxSkirmishAIs; ++sai) {
		jAIClassNames[sai] = NULL;
		jAIInstances[sai] = NULL;
		jAIClassLoaders[sai] = NULL;
	}

	// dynamically load the JVM
	char* jreLocationFile = callback->DataDirs_allocatePath(interfaceId, JRE_LOCATION_FILE, false, false, false, false);
	char jrePath[1024];
	char jvmLibPath[1024];

	if (!GetJREPath(jrePath, sizeof(jrePath), jreLocationFile, NULL)) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed locating a JRE installation, you may specify the JAVA_HOME env var.");
		return false;
	}

	simpleLog_logL(LOG_LEVEL_NOTICE, "Using JRE (can be changed with JAVA_HOME): %s", jrePath);
	FREE(jreLocationFile);

#if defined __arch64__
	static const char* defJvmType = "server";
#else
	static const char* defJvmType = "client";
#endif
	const char* jvmType = java_getValueByKey(&jvmCfgProps, "jvm.type");

	if (jvmType == NULL)
		jvmType = defJvmType;

	if (!GetJVMPath(jrePath, jvmType, jvmLibPath, sizeof(jvmLibPath), NULL)) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed locating the %s version of the JVM, please contact spring devs.", jvmType);
		return false;
	}

	if (!sharedLib_isLoaded(jvmSharedLib = sharedLib_load(jvmLibPath))) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed to load the JVM at \"%s\".", jvmLibPath);
		return false;
	}

	simpleLog_logL(LOG_LEVEL_NOTICE, "Successfully loaded the JVM shared library at \"%s\".", jvmLibPath);

	if ((JNI_GetDefaultJavaVMInitArgs_f = (JNI_GetDefaultJavaVMInitArgs_t*) sharedLib_findAddress(jvmSharedLib, "JNI_GetDefaultJavaVMInitArgs")) == NULL) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed to load the JVM, function \"%s\" not exported.", "JNI_GetDefaultJavaVMInitArgs");
		return false;
	}

	if ((JNI_CreateJavaVM_f = (JNI_CreateJavaVM_t*) sharedLib_findAddress(jvmSharedLib, "JNI_CreateJavaVM")) == NULL) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed to load the JVM, function \"%s\" not exported.", "JNI_CreateJavaVM");
		return false;
	}

	if ((JNI_GetCreatedJavaVMs_f = (JNI_GetCreatedJavaVMs_t*) sharedLib_findAddress(jvmSharedLib, "JNI_GetCreatedJavaVMs")) == NULL) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed to load the JVM, function \"%s\" not exported.", "JNI_GetCreatedJavaVMs");
		return false;
	}

	java_establishJavaEnv();
	JNIEnv* env = java_getJNIEnv(true);
	const bool loaded = (env != NULL);
	const bool inited = (loaded && eventsJniBridge_initStatic(env, maxSkirmishAIs) == 0);
	java_establishSpringEnv();

	return inited;
}

bool java_releaseStatic()
{
	sharedLib_unload(jvmSharedLib);
	jvmSharedLib = NULL;

	FREE(jvmCfgProps.keys);
	FREE(jvmCfgProps.values);
	return true;
}



static jobject java_createAICallback(JNIEnv* env, const struct SSkirmishAICallback* aiCallback, int skirmishAIId) {
	(void) aiCallback;

	// initialize the AI Callback class, if not yet done
	if (g_cls_aiCallback == NULL) {
		// get the AI Callback class
		if ((g_cls_aiCallback = jniUtil_findClass(env, CLS_AI_CALLBACK)) == NULL)
			return NULL;
		if ((g_cls_aiCallback = jniUtil_makeGlobalRef(env, g_cls_aiCallback, CLS_AI_CALLBACK)) == NULL)
			return NULL;
		// get (int skirmishAIId) constructor
		if ((g_m_aiCallback_ctor_I = jniUtil_getMethodID(env, g_cls_aiCallback, "<init>", "(I)V")) == NULL)
			return NULL;
	}

	#if 1
	// reuse callback if reloading
	// this should be safe since the callback objects created in java_skirmishAI_init are not
	// broken by java_skirmishAI_handleEvent, which also calls java_reattachCurrentThread and
	// mightget a different env-pointer back
	if (jAICallBacks[skirmishAIId] != NULL)
		return jAICallBacks[skirmishAIId];
	#endif

	jobject o_clb = (*env)->NewObject(env, g_cls_aiCallback, g_m_aiCallback_ctor_I, skirmishAIId);

	if (jniUtil_checkException(env, "Failed creating Java AI Callback instance"))
		return NULL;

	// return (jAICallBacks[skirmishAIId] = jniUtil_makeGlobalRef(env, o_clb, "AI callback instance"));
	return (jAICallBacks[skirmishAIId] = o_clb);
}



static bool java_loadSkirmishAI(
	JNIEnv* env,
	const char* shortName,
	const char* version,
	const char* className,
	jobject* o_ai,
	jobject* o_aiClassLoader
) {
	#if 0
	// convert className from "com.myai.AI" to "com/myai/AI"
	const size_t classNameP_sizeMax = strlen(className) + 1;
	char classNameP[classNameP_sizeMax];
	STRCPY_T(classNameP, classNameP_sizeMax, className);
	util_strReplaceChar(classNameP, '.', '/');
	#endif

	// get the AIs private class-loader
	jobject o_global_aiClassLoader = java_createAIClassLoader(env, shortName, version);

	if (o_global_aiClassLoader == NULL)
		return false;

	*o_aiClassLoader = o_global_aiClassLoader;

	// get the AI interface (from AIInterface.jar)
	if (g_cls_ai_int == NULL) {
		if ((g_cls_ai_int = jniUtil_findClass(env, INT_AI)) == NULL)
			return false;
		if ((g_cls_ai_int = jniUtil_makeGlobalRef(env, g_cls_ai_int, "AI interface class")) == NULL)
			return false;
	}

	// get the AI implementation class (from SkirmishAI.jar)
	jclass cls_ai = jniUtil_findClassThroughLoader(env, o_global_aiClassLoader, className);

	if (cls_ai == NULL)
		return false;

	const bool implementsAIInt = (bool) (*env)->IsAssignableFrom(env, cls_ai, g_cls_ai_int);

	if (!implementsAIInt || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(LOG_LEVEL_ERROR, "AI class not assignable from interface "INT_AI": %s", className);
		simpleLog_logL(LOG_LEVEL_ERROR, "possible reasons (this list could be incomplete):");
		simpleLog_logL(LOG_LEVEL_ERROR, "* "INT_AI" interface not implemented");
		simpleLog_logL(LOG_LEVEL_ERROR, "* The AI is not compiled for the Java AI Interface version in use");

		if (implementsAIInt)
			(*env)->ExceptionDescribe(env);

		return false;
	}


	// get factory no-arg ctor
	jmethodID m_ai_ctor = jniUtil_getMethodID(env, cls_ai, "<init>", "()V");

	if (m_ai_ctor == NULL)
		return false;


	// get AI instance
	jobject o_local_ai = (*env)->NewObject(env, cls_ai, m_ai_ctor);

	if (o_local_ai == NULL || (*env)->ExceptionCheck(env)) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Failed fetching AI instance for class: %s", className);

		if (o_local_ai != NULL)
			(*env)->ExceptionDescribe(env);

		return false;
	}

	// make the AI a global reference, so it will not be garbage collected even after this method returns
	*o_ai = jniUtil_makeGlobalRef(env, o_local_ai, "AI instance");
	return true;
}


bool java_initSkirmishAIClass(
	const char* const shortName,
	const char* const version,
	const char* const className,
	int skirmishAIId
) {
	bool success = false;

	// see if an AI for className is instantiated already
	size_t sai;
	size_t firstFree = numSkirmishAIs;

	for (sai = 0; sai < numSkirmishAIs; ++sai) {
		if (jAIClassNames[sai] == NULL) {
			firstFree = sai;
			break;
		}
	}

	// sai is now either the instantiated one, or a free one
	// instantiate AI (if not already instantiated)
	assert(sai < maxSkirmishAIs);

	if (jAIClassNames[sai] == NULL) {
		sai = firstFree;
		java_establishJavaEnv();
		JNIEnv* env = java_getJNIEnv(false);

		jobject instance    = NULL;
		jobject classLoader = NULL;

		success = java_loadSkirmishAI(env, shortName, version, className, &instance, &classLoader);
		java_establishSpringEnv();

		if (success) {
			jAIInstances[sai] = instance;
			jAIClassLoaders[sai] = classLoader;
			jAIClassNames[sai] = util_allocStrCpy(className);

			numSkirmishAIs += (firstFree == numSkirmishAIs);
		} else {
			simpleLog_logL(LOG_LEVEL_ERROR, "Class loading failed for class: %s", className);
		}
	} else {
		success = true;
	}

	if (success)
		skirmishAIId_skirmishAiImpl[skirmishAIId] = sai;

	return success;
}

bool java_releaseSkirmishAIClass(const char* className)
{
	// see if an AI for className is instantiated
	size_t sai;
	for (sai = 0; sai < numSkirmishAIs; ++sai) {
		if (jAIClassNames[sai] == NULL)
			continue;
		if (strcmp(jAIClassNames[sai], className) == 0)
			break;
	}

	// sai is now either the instantiated one, or a free one
	// release AI (if its instance was found)
	assert(sai < maxSkirmishAIs);

	if (jAIClassNames[sai] == NULL)
		return false;

	java_establishJavaEnv();
	JNIEnv* env = java_getJNIEnv(false);


	// delete the AI class-loader global reference,
	// so it will be garbage collected
	bool successPart = jniUtil_deleteGlobalRef(env, jAIClassLoaders[sai], "AI class-loader");
	bool success = successPart;

	// delete the AI global reference,
	// so it will be garbage collected
	successPart = jniUtil_deleteGlobalRef(env, jAIInstances[sai], "AI instance");
	success = success && successPart;

	java_establishSpringEnv();

	if (success) {
		jAIClassLoaders[sai] = NULL;
		jAIInstances[sai] = NULL;

		FREE(jAIClassNames[sai]);

		// if it is the last implementation
		numSkirmishAIs -= ((sai + 1) == numSkirmishAIs);
	}

	return success;
}

bool java_releaseAllSkirmishAIClasses()
{
	bool success = true;

	const char* className = "";
	size_t sai;

	for (sai = 0; sai < numSkirmishAIs; ++sai) {
		if ((className = jAIClassNames[sai]) == NULL)
			continue;

		success = success && java_releaseSkirmishAIClass(className);
	}

	return success;
}



int java_skirmishAI_init(int skirmishAIId, const struct SSkirmishAICallback* aiCallback)
{
	int res = -1;

	java_establishJavaEnv();

	JNIEnv* env = java_getJNIEnv(false);
	jobject global_javaAICallback = java_createAICallback(env, aiCallback, skirmishAIId);

	if (global_javaAICallback != NULL)
		res = eventsJniBridge_initAI(env, skirmishAIId, global_javaAICallback);

	java_establishSpringEnv();
	return res;
}

int java_skirmishAI_release(int skirmishAIId)
{
	return 0;
}

int java_skirmishAI_handleEvent(int skirmishAIId, int topic, const void* data)
{
	java_establishJavaEnv();

	JNIEnv* env = java_getJNIEnv(false);
	const size_t sai   = skirmishAIId_skirmishAiImpl[skirmishAIId];
	jobject aiInstance = jAIInstances[sai];
	const int res = eventsJniBridge_handleEvent(env, aiInstance, skirmishAIId, topic, data);

	java_establishSpringEnv();
	return res;
}
