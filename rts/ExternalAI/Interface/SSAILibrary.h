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

#ifndef _SSAILIBRARY_H
#define _SSAILIBRARY_H

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
 * example: "http://spring.clan-sy.com/wiki/AI:RAI"
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
#include "exportdefines.h"

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
struct SSAILibrary {

	// static AI library functions

	/**
	 * Level of Support for a specific engine version and AI interface version.
	 *
	 * [optional]
	 * An AI not exporting this function is still valid.
	 *
	 * @return	the level of support for the supplied engine and AI interface
	 *			versions
	 */
	enum LevelOfSupport (CALLING_CONV *getLevelOfSupportFor)(int teamId,
			const char* engineVersionString, int engineVersionNumber,
			const char* aiInterfaceShortName, const char* aiInterfaceVersion);


	// team instance functions

	/**
	 * This function is called, when an AI instance shall be created for teamId.
	 * It is called before the first call to handleEvent() for teamId.
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
	 * @param	teamId        the teamId this library shall create an instance for
	 * @param	info          info about this AI (nessesary for non C/C++ AIs)
	 * @param	numInfoItems  now many items are stored in info
	 * @param	optionKeys    user specified option values (keys)
	 * @param	optionValues  user specified option values (values)
	 * @param	numOptions    user specified option values (size)
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *init)(int teamId,
			unsigned int infoSize,
			const char** infoKeys, const char** infoValues,
			unsigned int optionsSize,
			const char** optionsKeys, const char** optionsValues);

	/**
	 * This function is called, when an AI instance shall be deleted.
	 * It is called after the last call to handleEvent() for teamId.
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
	 * @param	teamId	the teamId the library shall release the instance of
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *release)(int teamId);

	/**
	 * Through this function, the AI receives events from the engine.
	 * For details about events that may arrive here, see file AISEvents.h.
	 *
	 * @param	teamId	the instance of the AI that the event is addressed to
//	 * @param	fromId	the id of the AI the event comes from, or FROM_ENGINE_ID
//	 *					if it comes from spring
//	 * @param	eventId	used on asynchronous events. this allows the AI to
//	 *					identify a possible result message, which was sent with
//	 *					the same eventId
	 * @param	topic	unique identifyer of a message
	 *					(see EVENT_* defines in AISEvents.h)
	 * @param	data	an topic specific struct, which contains the data
	 *					associatedwith the event
	 *					(see S*Event structs in AISEvents.h)
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *handleEvent)(int teamId, int topic, const void* data);
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _SSAILIBRARY_H
