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

#ifndef _SAIINTERFACELIBRARY_H
#define	_SAIINTERFACELIBRARY_H

#ifdef	__cplusplus
#include "creg/creg.h"
#endif /* __cplusplus */

#ifdef	__cplusplus
extern "C" {
#endif

#include "ELevelOfSupport.h"
#include "SSAILibrary.h"
#include "SGAILibrary.h"
#include "exportdefines.h"

#define AI_INTERFACE_PROPERTY_FILE_NAME "fileName"                     // [string] when the library file is "libC-0.1.so" or "C-0.1.dll", this value should be "C-0.1"
#define AI_INTERFACE_PROPERTY_SHORT_NAME "shortName"                   // [string: [a-zA-Z0-9_.]*]
#define AI_INTERFACE_PROPERTY_VERSION "version"                        // [string: [a-zA-Z0-9_.]*]
#define AI_INTERFACE_PROPERTY_NAME "name"                              // [string]
#define AI_INTERFACE_PROPERTY_DESCRIPTION "description"                // [string]
#define AI_INTERFACE_PROPERTY_URL "url"                                // [string]
#define AI_INTERFACE_PROPERTY_SUPPORTED_LANGUAGES "supportedLanguages" // [string]
#define AI_INTERFACE_PROPERTY_ENGINE_VERSION "engineVersion"           // [int] the engine version number the AI was compiled, but may work with newer or older ones too

/**
 * @brief struct Artificial Intelligence Interface Specifyer
 */
struct SAIInterfaceSpecifyer {
	const char* shortName; // [may not contain: spaces, '_', '#']
	const char* version; // [may not contain: spaces, '_', '#']
};

SAIInterfaceSpecifyer copySAIInterfaceSpecifyer(const struct SAIInterfaceSpecifyer* const orig);
void deleteSAIInterfaceSpecifyer(const struct SAIInterfaceSpecifyer* const spec);

#ifdef	__cplusplus
struct SAIInterfaceSpecifyer_Comparator {
	/**
	 * The key comparison function, a Strict Weak Ordering;
	 * it returns true if its first argument is less
	 * than its second argument, and false otherwise.
	 * This is also defined as map::key_compare.
	 */
	bool operator()(const struct SAIInterfaceSpecifyer& a, const struct SAIInterfaceSpecifyer& b) const;
	static bool IsEmpty(const struct SAIInterfaceSpecifyer& spec);
};
#endif /* __cplusplus */

/**
 * @brief struct Skirmish Artificial Intelligence Key
 * Compleetly specifies a skirmish AI together with an interface.
 */
struct SSAIKey {
#ifdef	__cplusplus
	CR_DECLARE_STRUCT(SSAIKey);
#endif /* __cplusplus */
	struct SAIInterfaceSpecifyer interface;
	struct SSAISpecifyer ai;
};

#ifdef	__cplusplus
struct SSAIKey_Comparator {
	/**
	 * The key comparison function, a Strict Weak Ordering;
	 * it returns true if its first argument is less
	 * than its second argument, and false otherwise.
	 * This is also defined as map::key_compare.
	 */
	bool operator()(const struct SSAIKey& a, const struct SSAIKey& b) const;
	static bool IsEmpty(const struct SSAIKey& key);
};
#endif /* __cplusplus */

/**
 * @brief struct Group Artificial Intelligence Key
 * Compleetly specifies a group AI together with an interface.
 */
struct SGAIKey {
#ifdef	__cplusplus
	CR_DECLARE_STRUCT(SGAIKey);
#endif /* __cplusplus */
	struct SAIInterfaceSpecifyer interface;
	struct SGAISpecifyer ai;
};

#ifdef	__cplusplus
struct SGAIKey_Comparator {
	/**
	 * The key comparison function, a Strict Weak Ordering;
	 * it returns true if its first argument is less
	 * than its second argument, and false otherwise.
	 * This is also defined as map::key_compare.
	 */
	bool operator()(const struct SGAIKey& a, const struct SGAIKey& b) const;
	static bool IsEmpty(const struct SGAIKey& key);
};
#endif /* __cplusplus */

/**
 * @brief struct Artificial Intelligence Interface
 */
struct SAIInterfaceLibrary {
	// static interface library methods
	int (CALLING_CONV *getInfos)(InfoItem infos[], int max); // static properties
	enum LevelOfSupport (CALLING_CONV *getLevelOfSupportFor)(
			const char* engineVersionString, int engineVersionNumber);
	
	// skirmish AI methods
	int (CALLING_CONV *getSkirmishAISpecifyers)(struct SSAISpecifyer* sAISpecifyers, int max);
	const struct SSAILibrary* (CALLING_CONV *loadSkirmishAILibrary)(const struct SSAISpecifyer* const sAISpecifyer);
	int (CALLING_CONV *unloadSkirmishAILibrary)(const struct SSAISpecifyer* const sAISpecifyer);
	int (CALLING_CONV *unloadAllSkirmishAILibraries)();
	
	// group AI methods
	int (CALLING_CONV *getGroupAISpecifyers)(struct SGAISpecifyer* gAISpecifyers, int max);
	const struct SGAILibrary* (CALLING_CONV *loadGroupAILibrary)(const struct SGAISpecifyer* const gAISpecifyer);
	int (CALLING_CONV *unloadGroupAILibrary)(const struct SGAISpecifyer* const gAISpecifyer);
	int (CALLING_CONV *unloadAllGroupAILibraries)();
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SAIINTERFACELIBRARY_H */

