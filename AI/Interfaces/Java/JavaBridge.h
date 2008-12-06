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
#define	_JAVABRIDGE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>	// bool, true, false

struct SStaticGlobalData;
struct SAICallback;

/**
 * Returns a JNI environment, which includes a JVM.
 * Only one will exist at a time.
 * It is lazyly created.
 *
 * JNI = Java Native Interface
 * JVM = Java Virtual Machine
 */
//JNIEnv* getJNIEnv();

// #############################################################################
// ### checked till here
// #############################################################################

#define JAI_DIR "AI/Bot-libs/JAI"
#define IMPL_DIR "AI/Bot-libs"
//#define IMPL_DIR JAI_DIR"/impl"
#define LIB_DIR JAI_DIR"/lib"
#define LOG_DIR JAI_DIR"/log"

#define CONFIG_FILE JAI_DIR"/config.xml"
#define LOG_FILE LOG_DIR"/native-log.txt"

#define JVM_LOGGING true
#define JVM_DEBUGGING false
#define JVM_DEBUG_PORT "7777"
#define MAX_JARS 512


#define PKG_MAIN	"com/clan_sy/spring/ai/"
#define PKG_EVENT	"com/clan_sy/spring/ai/event/"
#define PKG_COMMAND	"com/clan_sy/spring/ai/command/"
#define CLS_AI			PKG_MAIN"AI"
#define CLS_AI_EVENT	PKG_MAIN"AIEvent"
#define CLS_AI_CALLBACK	PKG_MAIN"AICallback"

// #############################################################################
// AI methods

#define MTH_INDEX_SKIRMISH_AI_INIT			0
#define MTH_SKIRMISH_AI_INIT "init"
#define SIG_SKIRMISH_AI_INIT "(ILjava/util/Properties;Ljava/util/Properties;)I"

#define MTH_INDEX_SKIRMISH_AI_RELEASE		1
#define MTH_SKIRMISH_AI_RELEASE "release"
#define SIG_SKIRMISH_AI_RELEASE "(I)I"

#define MTH_INDEX_SKIRMISH_AI_HANDLE_EVENT	2
#define MTH_SKIRMISH_AI_HANDLE_EVENT "handleEvent"
//#define SIG_SKIRMISH_AI_HANDLE_EVENT "(IL"CLS_AI_EVENT";)I"
#define SIG_SKIRMISH_AI_HANDLE_EVENT "(IILcom/sun/jna/Pointer;)I"


#define MTHS_SIZE_SKIRMISH_AI				3





#ifdef WIN32
#define ENTRY_DELIM ";"
#else
#define ENTRY_DELIM ":"
#endif
#define PATH_DELIM "/"


//jobject GetFactory(JNIEnv* jniEnv);
//jobject GetNewJGlobalAI(
//IGlobalAI* ConnectJGlobalAI(JNIEnv* jniEnv, jobject javaAi);
//bool ReleaseJavaAI(jobject javaAi);
//
//
//DLL_EXPORT IGlobalAI* GetNewAIByName(const char* jarName);
//DLL_EXPORT void ReleaseAI(IGlobalAI* ai);
//
//bool endsWith(const char* str, const char* suffix);

bool java_preloadJNIEnv();
bool java_unloadJNIEnv();
bool java_initStatic(const struct SStaticGlobalData* staticGlobalData);
bool java_releaseStatic();
bool java_initSkirmishAIClass(const char* className);
bool java_releaseSkirmishAIClass(const char* className);
bool java_releaseAllSkirmishAIClasses();
int java_skirmishAI_init(int teamId,
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		unsigned int optionsSize,
		const char** optionsKeys, const char** optionsValues);
const struct SAICallback* java_getSkirmishAICCallback(int teamId);


#ifdef	__cplusplus
}
#endif

#endif	// _JAVABRIDGE_H

