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

#ifndef _SGAILIBRARY_H
#define	_SGAILIBRARY_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "SInfo.h"
#include "SOption.h"
#include "ELevelOfSupport.h"
#include "exportdefines.h"

#define GROUP_AI_PROPERTY_FILE_NAME "fileName"                      // [string] when the library file is "libRAI-0.600.so" or "RAI-0.600.dll", this value should be "RAI-0.600"
#define GROUP_AI_PROPERTY_SHORT_NAME "shortName"                    // [string: [a-zA-Z0-9_.]*]
#define GROUP_AI_PROPERTY_VERSION "version"                         // [string: [a-zA-Z0-9_.]*]
#define GROUP_AI_PROPERTY_NAME "name"                               // [string]
#define GROUP_AI_PROPERTY_DESCRIPTION "description"                 // [string]
#define GROUP_AI_PROPERTY_URL "url"                                 // [string]
#define GROUP_AI_PROPERTY_LOAD_SUPPORTED "loadSupported"            // [bool: "yes" | "no"]
#define GROUP_AI_PROPERTY_ENGINE_VERSION "engineVersion"            // [int] the engine version number the AI was compiled, but may work with newer or older ones too
#define GROUP_AI_PROPERTY_INTERFACE_SHORT_NAME "interfaceShortName" // [string: [a-zA-Z0-9_.]*] this interface has to be used to load the AI
#define GROUP_AI_PROPERTY_INTERFACE_VERSION "interfaceVersion"      // [string: [a-zA-Z0-9_.]*] the interface version number the AI was compiled, but may work with newer or older ones too

/**
 * @brief struct Group Artificial Intelligence Specifyer
 */
struct SGAISpecifyer {
	const char* shortName; // [may not contain: spaces, '_', '#']
	const char* version; // [may not contain: spaces, '_', '#']
};

SGAISpecifyer copySGAISpecifyer(const struct SGAISpecifyer* const orig);
void deleteSGAISpecifyer(const struct SGAISpecifyer* const spec);

#ifdef	__cplusplus
struct SGAISpecifyer_Comparator {
	/**
	 * The key comparison function, a Strict Weak Ordering;
	 * it returns true if its first argument is less
	 * than its second argument, and false otherwise.
	 * This is also defined as map::key_compare.
	 */
	bool operator()(const struct SGAISpecifyer& a, const struct SGAISpecifyer& b) const;
	static bool IsEmpty(const struct SGAISpecifyer& spec);
};
#endif /* __cplusplus */

/**
 * @brief struct Group Artificial Intelligence
 * This is the interface between the engine and an implementation of a Skirmish AI.
 */
struct SGAILibrary {
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
	int (CALLING_CONV *getOptions)(struct Option optionsInfo[], int max);

	// team and group instance functions
	int (CALLING_CONV *init)(int teamId, int groupId);
	int (CALLING_CONV *release)(int teamId, int groupId);
	int (CALLING_CONV *handleEvent)(int teamId, int groupId, int topic, const void* data);
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SGAILIBRARY_H */
