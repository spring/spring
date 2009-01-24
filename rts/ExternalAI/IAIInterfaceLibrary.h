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

#ifndef _IAIINTERFACELIBRARY_H
#define	_IAIINTERFACELIBRARY_H

#include "ISkirmishAILibrary.h"
#include <string>

class CSkirmishAILibraryInfo;
class AIInterfaceKey;
class SkirmishAIKey;

class IAIInterfaceLibrary {
public:
	virtual ~IAIInterfaceLibrary() {}

	virtual AIInterfaceKey GetKey() const = 0;

	virtual LevelOfSupport GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber)
			const = 0;

	/**
	 * @brief	how many times is this interface loaded
	 * Thought the AI library may be loaded only once, it can be logically
	 * loaded multiple times.
	 * Example: If we load one RAI and two AAIs over this interface,
	 * the interface load counter will be three.
	 */
	virtual int GetLoadCount() const = 0;


	/**
	 * @brief	loads the AI library
	 * This only loads the AI library, and does not yet create an instance
	 * for a team.
	 * For the C and C++ AI interface eg, this will load a shared library.
	 * Increments the load counter.
	 */
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(
			const CSkirmishAILibraryInfo& skirmishAIInfo) = 0;
	/**
	 * @brief	unloads the Skirmish AI library
	 * This unloads the Skirmish AI library.
	 * For the C and C++ AI interface eg, this will unload a shared library.
	 * This should not be done when any instances
	 * of that AI are still in use, as it will result in a crash.
	 * Decrements the load counter.
	 */
	virtual int ReleaseSkirmishAILibrary(const SkirmishAIKey& key) = 0;
	/**
	 * @brief	is the Skirmish AI library loaded
	 */
	bool IsSkirmishAILibraryLoaded(const SkirmishAIKey& key) const {
		return GetSkirmishAILibraryLoadCount(key) > 0;
	}
	/**
	 * @brief	how many times is the Skirmish AI loaded
	 * Thought the AI library may be loaded only once, it can be logically
	 * loaded multiple times (load counter).
	 */
	virtual int GetSkirmishAILibraryLoadCount(const SkirmishAIKey& key)
			const = 0;
	/**
	 * @brief	unloads all AIs
	 * Unloads all AI libraries currently loaded through this interface.
	 */
	virtual int ReleaseAllSkirmishAILibraries() = 0;

};

#endif // _IAIINTERFACELIBRARY_H
