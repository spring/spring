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

#ifndef _JAVABRIDGE_H
#define _JAVABRIDGE_H


#define JVM_PROPERTIES_FILE "jvm.properties"

#if defined DEBUG
	#define JVM_LOGGING
	#define JVM_DEBUGGING
	#define JVM_DEBUG_PORT "7777"
#endif // defined DEBUG


#define PKG_MAIN	"com/springrts/ai/"
#define PKG_EVENT	"com/springrts/ai/event/"
#define PKG_COMMAND	"com/springrts/ai/command/"
#define CLS_AI			PKG_MAIN"AI"
#define CLS_AI_EVENT	PKG_MAIN"AIEvent"
#define CLS_AI_CALLBACK	PKG_MAIN"AICallback"

// #############################################################################
// Skirmish AI methods

#define MTH_INDEX_SKIRMISH_AI_INIT          0
#define MTH_SKIRMISH_AI_INIT "init"
#define SIG_SKIRMISH_AI_INIT "(ILcom/springrts/ai/AICallback;)I"

#define MTH_INDEX_SKIRMISH_AI_RELEASE       1
#define MTH_SKIRMISH_AI_RELEASE "release"
#define SIG_SKIRMISH_AI_RELEASE "(I)I"

#define MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT  2
#define MTH_SKIRMISH_AI_HANDLE_EVENT "handleEvent"
//#define SIG_SKIRMISH_AI_HANDLE_EVENT "(IL"CLS_AI_EVENT";)I"
#define SIG_SKIRMISH_AI_HANDLE_EVENT "(IILcom/sun/jna/Pointer;)I"


#define MTHS_SIZE_SKIRMISH_AI               3
// #############################################################################


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

#include <stdbool.h>	// bool, true, false

struct SAIInterfaceCallback;
struct SSkirmishAICallback;

///**
// * Returns a JNI environment, which includes a JVM.
// * Only one will exist at a time.
// * It is lazyly created.
// *
// * JNI = Java Native Interface
// * JVM = Java Virtual Machine
// */
//JNIEnv* getJNIEnv();
bool java_preloadJNIEnv();
bool java_unloadJNIEnv();
bool java_initStatic(int interfaceId,
		const struct SAIInterfaceCallback* callback);
bool java_releaseStatic();
/**
 * Instantiates an instance of the specified className.
 *
 * @param  shortName  further specifies the the AI to load
 * @param  version  further specifies the the AI to load
 * @param  className  fully qualified name of a Java class that implements
 *                    interface com.springrts.ai.AI, eg:
 *                    "com.myai.AI"
 * @param  teamI   The team that will be using this AI.
 *                 Multiple teams may use the same AI implementation.
 * @return  true, if the AI implementation is now loaded
 */
bool java_initSkirmishAIClass(
		const char* const shortName,
		const char* const version,
		const char* const className,
		int teamId);
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
//const struct SSkirmishAICallback* java_getSkirmishAICCallback(int teamId);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _JAVABRIDGE_H
