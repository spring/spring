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
#include "IGroupAILibrary.h"
#include <string>

class CSkirmishAILibraryInfo;
class CGroupAILibraryInfo;

class IAIInterfaceLibrary {
public:
	virtual ~IAIInterfaceLibrary() {}

	virtual SAIInterfaceSpecifier GetSpecifier() const = 0;

	virtual LevelOfSupport GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber) const = 0;

//    virtual std::string GetProperty(const std::string& propertyName) const = 0;
//    virtual std::map<std::string, InfoItem> GetInfo() const = 0;

	/**
	 * @brief	how many times is this interface loaded
	 * Thought the AI library may be loaded only once, it can be logically
	 * loaded multiple times.
	 * Example: If we load one RAI and two AAIs over this interface,
	 * the interface load counter will be three.
	 */
	virtual int GetLoadCount() const = 0;



//	virtual std::vector<ISkirmishAILibraryInfo*> GetSkirmishAILibraryInfo(bool forceLoadFromLibrary = false) const;
	/**
	 * Returns the specifiers for all Skirmish AIs available through this interface.
	 */
	//virtual std::vector<SSAISpecifier> GetSkirmishAILibrarySpecifiers() const = 0;
	/**
	 * @brief	loads the AI library
	 * This only loads the AI library, and does not yet create an instance
	 * for a team.
	 * For the C and C++ AI interface eg, this will load a shared library.
	 * Increments the load counter.
	 */
	//virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const SSAISpecifier& sAISpecifier) = 0;
	//virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const InfoItem* info, unsigned int numInfo) = 0;
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const CSkirmishAILibraryInfo* skirmishAIInfo) = 0;
	/**
	 * @brief	unloads the Skirmish AI library
	 * This unloads the Skirmish AI library.
	 * For the C and C++ AI interface eg, this will unload a shared library.
	 * This should not be done when any instances
	 * of that AI are still in use, as it will result in a crash.
	 * Decrements the load counter.
	 */
	virtual int ReleaseSkirmishAILibrary(const SSAISpecifier& sAISpecifier) = 0;
	/**
	 * @brief	is the Skirmish AI library loaded
	 */
	bool IsSkirmishAILibraryLoaded(const SSAISpecifier& sAISpecifier) const {
		return GetSkirmishAILibraryLoadCount(sAISpecifier) > 0;
	}
	/**
	 * @brief	how many times is the Skirmish AI loaded
	 * Thought the AI library may be loaded only once, it can be logically
	 * loaded multiple times (load counter).
	 */
	virtual int GetSkirmishAILibraryLoadCount(const SSAISpecifier& sAISpecifier) const = 0;
	/**
	 * @brief	unloads all AIs
	 * Unloads all AI libraries currently loaded through this interface.
	 */
	virtual int ReleaseAllSkirmishAILibraries() = 0;



////	virtual std::vector<IGroupAILibraryInfo*> GetGroupAILibraryInfo(bool forceLoadFromLibrary = false) const;
//	//virtual std::vector<SGAISpecifier> GetGroupAILibrarySpecifiers() const = 0;
//	//virtual const IGroupAILibrary* FetchGroupAILibrary(const SGAISpecifier& gAISpecifier) = 0;
//	//virtual const IGroupAILibrary* FetchGroupAILibrary(const InfoItem* info, unsigned int numInfo) = 0;
//	virtual const IGroupAILibrary* FetchGroupAILibrary(const CGroupAILibraryInfo* aiInfo) = 0;
//	virtual int ReleaseGroupAILibrary(const SGAISpecifier& gAISpecifier) = 0;
//	virtual int GetGroupAILibraryLoadCount(const SGAISpecifier& gAISpecifier) const = 0;
//	bool IsGroupAILibraryLoaded(const SGAISpecifier& gAISpecifier) const {
//		return GetGroupAILibraryLoadCount(gAISpecifier) > 0;
//	}
//	virtual int ReleaseAllGroupAILibraries() = 0;
};

#endif // _IAIINTERFACELIBRARY_H
