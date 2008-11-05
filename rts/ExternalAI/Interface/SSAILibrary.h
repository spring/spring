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
#define	_SSAILIBRARY_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "SInfo.h"
#include "SOption.h"
#include "ELevelOfSupport.h"
#include "exportdefines.h"

#define SKIRMISH_AI_PROPERTY_DATA_DIR                "dataDir"               // [string] absolute data dir containing the AIs AIInfo.lua file. this property is by the engine, not read from any file.
#define SKIRMISH_AI_PROPERTY_FILE_NAME               "fileName"              // [string] when the library file is "libRAI-0.600.so" or "RAI-0.600.dll", this value should be "RAI-0.600"
#define SKIRMISH_AI_PROPERTY_SHORT_NAME              "shortName"             // [string: [a-zA-Z0-9_.]*]
#define SKIRMISH_AI_PROPERTY_VERSION                 "version"               // [string: [a-zA-Z0-9_.]*]
#define SKIRMISH_AI_PROPERTY_NAME                    "name"                  // [string]
#define SKIRMISH_AI_PROPERTY_DESCRIPTION             "description"           // [string]
#define SKIRMISH_AI_PROPERTY_URL                     "url"                   // [string]
#define SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED          "loadSupported"         // [bool: "yes" | "no"]
#define SKIRMISH_AI_PROPERTY_ENGINE_VERSION          "engineVersion"         // [int] the engine version number the AI was compiled, but may work with newer or older ones too
#define SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME    "interfaceShortName"    // [string: [a-zA-Z0-9_.]*] this interface has to be used to load the AI
#define SKIRMISH_AI_PROPERTY_INTERFACE_VERSION       "interfaceVersion"      // [string: [a-zA-Z0-9_.]*] the interface version number the AI was compiled, but may work with newer or older ones too

#if !defined BUILDING_AI
/**
 * @brief struct Skirmish Artificial Intelligence Specifier
 */
struct SSAISpecifier {
	const char* shortName; // [may not contain: spaces, '_', '#']
	const char* version; // [may not contain: spaces, '_', '#']
};

struct SSAISpecifier copySSAISpecifier(const struct SSAISpecifier* const orig);
void deleteSSAISpecifier(const struct SSAISpecifier* const spec);

#if defined __cplusplus
struct SSAISpecifier_Comparator {
	/**
	 * The key comparison function, a Strict Weak Ordering;
	 * it returns true if its first argument is less
	 * than its second argument, and false otherwise.
	 * This is also defined as map::key_compare.
	 */
	bool operator()(const struct SSAISpecifier& a,
			const struct SSAISpecifier& b) const;
	static bool IsEmpty(const struct SSAISpecifier& spec);
};
#endif	// defined __cplusplus
#endif	// !defined BUILDING_AI

/**
 * This is the interface between the engine and an implementation of a
 * Skirmish AI.
 *
 * @brief struct Skirmish Artificial Intelligence
 */
struct SSAILibrary {

	// static AI library functions

	/**
	 * Level of Support for a specific engine version and AI interface version.
	 *
	 * NOTE: this method is optional. An AI not exporting this function is still
	 * valid.
	 *
	 * @return	the level of support for the supplied engine and AI interface
	 *			versions
	 */
	enum LevelOfSupport (CALLING_CONV *getLevelOfSupportFor)(int teamId,
			const char* engineVersionString, int engineVersionNumber,
			const char* aiInterfaceShortName, const char* aiInterfaceVersion);

	/**
	 * Returns info about this AI library.
	 *
	 * NOTE: this method is optional. An AI not exporting this function is still
	 * valid.
	 *
	 * @param	info	where the info about this AI library shall be stored to
	 * @param	max	the maximum number of elements to store into param info
	 * @return	number of elements stored into parameter info
	 */
	unsigned int (CALLING_CONV *getInfo)(int teamId, struct InfoItem info[],
			unsigned int maxInfoItems);

	/**
	 * Returns options that can be set on this AI.
	 *
	 * NOTE: this method is optional. An AI not exporting this function is still
	 * valid.
	 *
	 * @param	infos	where the options of this AI library shall be stored to
	 * @param	max	the maximum number of elements to store into param options
	 * @return	number of elements stored into parameter options
	 */
	unsigned int (CALLING_CONV *getOptions)(int teamId, struct Option options[],
			unsigned int maxOptions);


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
	 * NOTE: this method is optional. An AI not exporting this function is still
	 * valid.
	 *
	 * @param	teamId	the teamId this library shall create an instance for
	 * @param	info	info about this AI (a technical nessecity for non C/C++ AIs)
	 * @param	numInfoItems	now many items are stored in info
	 * @return	ok: 0, error: != 0
	 */
	int (CALLING_CONV *init)(int teamId, const struct InfoItem info[],
			unsigned int numInfoItems);

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
	 * NOTE: this method is optional. An AI not exporting this function is still
	 * valid.
	 *
	 * @param	teamId	the teamId the library shall release the instance of
	 * @return	ok: 0, error: != 0
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
	 * @return	ok: 0, error: != 0
	 */
	int (CALLING_CONV *handleEvent)(int teamId, int topic, const void* data);
};

#ifdef	__cplusplus
}	// extern "C"
#endif

#endif	// _SSAILIBRARY_H
