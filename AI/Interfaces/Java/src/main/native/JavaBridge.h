/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _JAVA_BRIDGE_H
#define _JAVA_BRIDGE_H

#define JVM_PROPERTIES_FILE "jvm.properties"

#define PKG_AI         "com/springrts/ai/"
#define INT_AI          PKG_AI"AI"
#define CLS_AI_CALLBACK PKG_AI"JniAICallback"

// define path entry delimitter, used eg for the java class-path
#ifdef WIN32
#define ENTRY_DELIM ";"
#else
#define ENTRY_DELIM ":"
#endif
#define PATH_DELIM "/"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool, true, false

struct SAIInterfaceCallback;
struct SSkirmishAICallback;

bool java_preloadJNIEnv();
bool java_unloadJNIEnv();

bool java_initStatic(int interfaceId, const struct SAIInterfaceCallback* callback);
bool java_releaseStatic();

/**
 * Instantiates an instance of the specified className.
 *
 * @param  shortName  further specifies the the AI to load
 * @param  version  further specifies the the AI to load
 * @param  className  fully qualified name of a Java class that implements
 *                    interface com.springrts.ai.AI, eg:
 *                    "com.myai.AI"
 * @param  teamId  The team that will be using this AI.
 *                 Multiple teams may use the same AI implementation.
 * @return  true, if the AI implementation is now loaded
 */
bool java_initSkirmishAIClass(
	const char* const shortName,
	const char* const version,
	const char* const className,
	int teamId
);

/**
 * Release the loaded AI specified through a class name.
 *
 * @param  className  fully qualified name of a Java class that implements
 *                    interface com.springrts.ai.AI, eg:
 *                    "com.myai.AI"
 * @return  true, if the AI implementation was loaded and is now
 *          successfully unloaded
 */
bool java_releaseSkirmishAIClass(const char* className);
bool java_releaseAllSkirmishAIClasses();

int java_skirmishAI_init(int teamId, const struct SSkirmishAICallback* callback);
int java_skirmishAI_release(int teamId);
int java_skirmishAI_handleEvent(int teamId, int topic, const void* data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _JAVA_BRIDGE_H
