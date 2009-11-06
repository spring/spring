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
 * @param  teamId  The team that will be using this AI.
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

int java_skirmishAI_init(int teamId,
		const struct SSkirmishAICallback* callback);

int java_skirmishAI_release(int teamId);

int java_skirmishAI_handleEvent(int teamId, int topic, const void* data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _JAVABRIDGE_H
