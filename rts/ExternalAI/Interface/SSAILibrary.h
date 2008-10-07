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

#define SKIRMISH_AI_PROPERTY_FILE_NAME "fileName"                      // [string] when the library file is "libRAI-0.600.so" or "RAI-0.600.dll", this value should be "RAI-0.600"
#define SKIRMISH_AI_PROPERTY_SHORT_NAME "shortName"                    // [string: [a-zA-Z0-9_.]*]
#define SKIRMISH_AI_PROPERTY_VERSION "version"                         // [string: [a-zA-Z0-9_.]*]
#define SKIRMISH_AI_PROPERTY_NAME "name"                               // [string]
#define SKIRMISH_AI_PROPERTY_DESCRIPTION "description"                 // [string]
#define SKIRMISH_AI_PROPERTY_URL "url"                                 // [string]
#define SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED "loadSupported"            // [bool: "yes" | "no"]
#define SKIRMISH_AI_PROPERTY_ENGINE_VERSION "engineVersion"            // [int] the engine version number the AI was compiled, but may work with newer or older ones too
#define SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME "interfaceShortName" // [string: [a-zA-Z0-9_.]*] this interface has to be used to load the AI
#define SKIRMISH_AI_PROPERTY_INTERFACE_VERSION "interfaceVersion"      // [string: [a-zA-Z0-9_.]*] the interface version number the AI was compiled, but may work with newer or older ones too

/**
 * @brief struct Skirmish Artificial Intelligence Specifyer
 */
struct SSAISpecifyer {
	const char* shortName; // [may not contain: spaces, '_', '#']
	const char* version; // [may not contain: spaces, '_', '#']
};

SSAISpecifyer copySSAISpecifyer(const struct SSAISpecifyer* const orig);
void deleteSSAISpecifyer(const struct SSAISpecifyer* const spec);

#ifdef	__cplusplus
struct SSAISpecifyer_Comparator {
	/**
	 * The key comparison function, a Strict Weak Ordering;
	 * it returns true if its first argument is less
	 * than its second argument, and false otherwise.
	 * This is also defined as map::key_compare.
	 */
	bool operator()(const struct SSAISpecifyer& a, const struct SSAISpecifyer& b) const;
	static bool IsEmpty(const struct SSAISpecifyer& spec);
};
#endif /* __cplusplus */

/**
 * @brief struct Skirmish Artificial Intelligence
 * This is the interface between the engine and an implementation of a Skirmish AI.
 */
struct SSAILibrary {
	// static AI library functions
	/**
	 * Level of Support for a specific engine version.
	 * NOTE: this method is optional. An AI not exporting this function is still
	 * valid.
	 */
	enum LevelOfSupport (CALLING_CONV *getLevelOfSupportFor)(
			const char* engineVersionString, int engineVersionNumber,
			const char* aiInterfaceShortName, const char* aiInterfaceVersion);
	/**
	 * Returns static properties with info about this AI library.
	 * NOTE: this method is optional. An AI not exporting this function is still
	 * valid.
	 * @return number of elements stored into parameter infos
	 */
	int (CALLING_CONV *getInfos)(struct InfoItem infos[], int max);
	/**
	 * Returns options that can be set on this AI.
	 * NOTE: this method is optional. An AI not exporting this function is still
	 * valid.
	 * @return number of elements stored into parameter options
	 */
	int (CALLING_CONV *getOptions)(struct Option options[], int max);

	// team instance functions
	int (CALLING_CONV *init)(int teamId);
	int (CALLING_CONV *release)(int teamId);
	int (CALLING_CONV *handleEvent)(int teamId, int topic, const void* data);
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SSAILIBRARY_H */
