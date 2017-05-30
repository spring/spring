/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S_SKIRMISH_AI_LIBRARY_H
#define S_SKIRMISH_AI_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * [string]
 * Absolute data dir containing the AIs AIInfo.lua file.
 * This property is set by the engine, not read from any file.
 * example: "/home/john/spring/AI/Skirmish/RAI/0.601"
 */
#define SKIRMISH_AI_PROPERTY_DATA_DIR                "dataDir"

/**
 * [string]
 * Absolute, version independent data dir.
 * This property is set by the engine, not read from any file.
 * example: "/home/john/spring/AI/Skirmish/RAI/common"
 */
#define SKIRMISH_AI_PROPERTY_DATA_DIR_COMMON         "dataDirCommon"

/**
 * [string: [a-zA-Z0-9_.]*]
 * example: "RAI"
 */
#define SKIRMISH_AI_PROPERTY_SHORT_NAME              "shortName"

/**
 * [string: [a-zA-Z0-9_.]*]
 * example: "0.601"
 */
#define SKIRMISH_AI_PROPERTY_VERSION                 "version"

/**
 * [string]
 * example: "Reth's Skirmish AI"
 */
#define SKIRMISH_AI_PROPERTY_NAME                    "name"

/**
 * [string]
 * example: "no config files required, works well with TA based mods and more"
 */
#define SKIRMISH_AI_PROPERTY_DESCRIPTION             "description"

/**
 * [string]
 * example: "https://springrts.com/wiki/AI:RAI"
 */
#define SKIRMISH_AI_PROPERTY_URL                     "url"

/** [bool: "yes" | "no"] */
#define SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED          "loadSupported"

/**
 * [int]
 * The engine version number the AI was compiled for,
 * though it may work with newer or older engine versions too.
 */
#define SKIRMISH_AI_PROPERTY_ENGINE_VERSION          "engineVersion"

/**
 * [string: [a-zA-Z0-9_.]*]
 * This AI Interface has to be used to load the AI.
 * example: "C"
 */
#define SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME    "interfaceShortName"

/**
 * [string: [a-zA-Z0-9_.]*]
 * The AI Interface version number the AI was written for.
 * This value is seen as a minimum requirement,
 * so loading the AI with an older version of
 * the AI Interface will not be attempted.
 * example: "0.1"
 */
#define SKIRMISH_AI_PROPERTY_INTERFACE_VERSION       "interfaceVersion"


#include "ELevelOfSupport.h"
#include "System/ExportDefines.h"

struct SSkirmishAICallback;

/**
 * @brief Skirmish Artificial Intelligence library interface
 *
 * This is the interface between the engine and an implementation of a
 * Skirmish AI.
 * The engine will address AIs through this interface, but AIs will
 * not actually implement it. It is the job of the AI Interface library,
 * to make sure the engine can address AI implementations through
 * instances of this struct.
 *
 * An example of loading a C AI through the C AI Interface:
 * The C AI exports functions fitting the function pointers in this
 * struct. When the engine requests C-AI-X on the C AI Interface,
 * the interface loads the shared library, eg C-AI-X.dll, creates
 * a new instance of this struct, and sets the member function
 * pointers to the addresses of the fitting functions exported
 * by the shared AI library. This struct then goes to the engine
 * which calls the functions appropriately.
 */
struct SSkirmishAILibrary {

	// static AI library functions

	/**
	 * Level of Support for a specific engine version and AI interface version.
	 *
	 * [optional]
	 * An AI not exporting this function is still valid.
	 *
	 * @param  aiShortName  this is indisposable for non-native interfaces,
	 *                      as they need a means of redirecting from
	 *                      one wrapper function to the respective non-native
	 *                      libraries
	 * @param  aiVersion  see aiShortName
	 * @return the level of support for the supplied engine and AI interface
	 *         versions
	 */
	enum LevelOfSupport (CALLING_CONV *getLevelOfSupportFor)(
			const char* aiShortName, const char* aiVersion,
			const char* engineVersionString, int engineVersionNumber,
			const char* aiInterfaceShortName, const char* aiInterfaceVersion);


	// team instance functions

	/**
	 * This function is called, when an AI instance shall be created
	 * for skirmishAIId. It is called before the first call to handleEvent()
	 * for that AI instance.
	 *
	 * A typical series of events (engine point of view, conceptual):
	 * [code]
	 * KAIK.init(1)
	 * KAIK.handleEvent(EVENT_INIT, InitEvent(1))
	 * RAI.init(2)
	 * RAI.handleEvent(EVENT_INIT, InitEvent(2))
	 * KAIK.handleEvent(EVENT_UPDATE, UpdateEvent(0))
	 * RAI.handleEvent(EVENT_UPDATE, UpdateEvent(0))
	 * KAIK.handleEvent(EVENT_UPDATE, UpdateEvent(1))
	 * RAI.handleEvent(EVENT_UPDATE, UpdateEvent(1))
	 * ...
	 * [/code]
	 *
	 * This method exists only for performance reasons, which come into play on
	 * OO languages. For non-OO language AIs, this method can be ignored,
	 * because using only EVENT_INIT will cause no performance decrease.
	 *
	 * [optional]
	 * An AI not exporting this function is still valid.
	 *
	 * @param skirmishAIId  the ID this library shall create an instance for
	 * @param callback      the callback for this Skirmish AI
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *init)(int skirmishAIId,
			const struct SSkirmishAICallback* callback);

	/**
	 * This function is called, when an AI instance shall be deleted.
	 * It is called after the last call to handleEvent() for that AI instance.
	 *
	 * A typical series of events (engine point of view, conceptual):
	 * [code]
	 * ...
	 * KAIK.handleEvent(EVENT_UPDATE, UpdateEvent(654321))
	 * RAI.handleEvent(EVENT_UPDATE, UpdateEvent(654321))
	 * KAIK.handleEvent(EVENT_UPDATE, UpdateEvent(654322))
	 * RAI.handleEvent(EVENT_UPDATE, UpdateEvent(654322))
	 * KAIK.handleEvent(EVENT_RELEASE, ReleaseEvent(1))
	 * KAIK.release(1)
	 * RAI.handleEvent(EVENT_RELEASE, ReleaseEvent(2))
	 * RAI.release(2)
	 * [/code]
	 *
	 * This method exists only for performance reasons, which come into play on
	 * OO languages. For non-OO language AIs, this method can be ignored,
	 * because using only EVENT_RELEASE will cause no performance decrease.
	 *
	 * [optional]
	 * An AI not exporting this function is still valid.
	 *
	 * @param skirmishAIId  the ID the library shall release the instance of
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *release)(int skirmishAIId);

	/**
	 * Through this function, the AI receives events from the engine.
	 * For details about events that may arrive here, see file AISEvents.h.
	 *
	 * @param skirmishAIId  the AI instance the event is addressed to
//	 * @param fromId        the id of the AI the event comes from,
//	 *                      or FROM_ENGINE_ID if it comes from the engine
//	 * @param eventId       used on asynchronous events. this allows the AI to
//	 *                      identify a possible result message, which was sent
//	 *                      with the same eventId
	 * @param topicId       unique identifier of a message
	 *                      (see EVENT_* defines in AISEvents.h)
	 * @param data          an topic specific struct, which contains the data
	 *                      associatedwith the event
	 *                      (see S*Event structs in AISEvents.h)
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *handleEvent)(int skirmishAIId, int topicId,
			const void* data);
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif // S_SKIRMISH_AI_LIBRARY_H
