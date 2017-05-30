/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This file has to be C99 compatible, as it is not only used by the engine,
 * but also by AIs.
 */

#ifndef AI_DEFINES_H
#define AI_DEFINES_H

#include "System/MainDefines.h"
#include "System/ExportDefines.h"

#define ENGINE_VERSION_STRING VERSION_STRING
#define ENGINE_VERSION_NUMBER 1000

// Changing these structs breaks the AI Interface ABI.
// Though we can only keep track of the number of function pointers
// in the callback structs, and not their arguments.
// Therefore, this number will stay the same, if you only change parameters
// of function pointers, which is why it is only partly representing
// the real ABI.
// Files that have to be included when using this define:
// * ExternalAI/Interface/ELevelOfSupport.h
// * ExternalAI/Interface/SSkirmishAILibrary.h
// * ExternalAI/Interface/SSkirmishAICallback.h
// * ExternalAI/Interface/SAIInterfaceLibrary.h
// * ExternalAI/Interface/SAIInterfaceCallback.h
// * ExternalAI/Interface/AISEvents.h
// * ExternalAI/Interface/AISCommands.h
/**
 * Returns the Application Binary Interface version, fail part.
 * If the engine and the AI Interface differ in this,
 * the AI Interface will not be used.
 * Changes here usually indicate that struct members were
 * added or removed.
 */
#define AIINTERFACE_ABI_VERSION_FAIL ( \
	  sizeof(enum LevelOfSupport) \
	+ sizeof(struct SSkirmishAILibrary) \
	+ sizeof(struct SSkirmishAICallback) \
	+ sizeof(struct SAIInterfaceLibrary) \
	+ sizeof(struct SAIInterfaceCallback) \
	+ AIINTERFACE_EVENTS_ABI_VERSION \
	+ AIINTERFACE_COMMANDS_ABI_VERSION \
	+ __archBits__   * 10000 \
	+ sizeof(int)    * 1001 \
	+ sizeof(char)   * 1002 \
	+ sizeof(void*)  * 1003 \
	+ sizeof(size_t) * 1005 \
	+ sizeof(float)  * 1007 \
	+ sizeof(short)  * 1011 \
	+ sizeof(bool)   * 1013 \
	)
/**
 * Returns the Application Binary Interface version, warning part.
 * Interface and engine will try to run/work together,
 * if they differ only in the warning part of the ABI version.
 * Changes here could indicate that function arguments got changed,
 * which could cause a crash, but it could be unimportant changes
 * like added comments or code reformatting as well.
 */
#define AIINTERFACE_ABI_VERSION_WARNING ( \
	  0 \
	)

/**
 * @brief max Skirmish AIs
 *
 * Defines the maximum number of skirmish AIs.
 * As there can not be more then spring allows teams, this is the upper limit.
 * (currently (February 2010) 255 real teams)
 */
//const unsigned int MAX_SKIRMISH_AIS = MAX_TEAMS - 1;
#define MAX_SKIRMISH_AIS 255

//const char* const AI_INTERFACES_DATA_DIR = "AI/Interfaces";
#define AI_INTERFACES_DATA_DIR "AI/Interfaces"

#define SKIRMISH_AI_DATA_DIR "AI/Skirmish"

// Size of buffer for response from lua UI/Rules, including '\0'
#define MAX_RESPONSE_SIZE 10240

#endif // AI_DEFINES_H
