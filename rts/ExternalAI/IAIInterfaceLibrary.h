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
	
	virtual SAIInterfaceSpecifyer GetSpecifyer() const = 0;
	virtual LevelOfSupport GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber) const = 0;
	
//    virtual std::string GetProperty(const std::string& propertyName) const = 0;
    virtual std::map<std::string, InfoItem> GetInfos() const = 0;
	
	/**
	 * @brief	how many times is this interface loaded
	 * Thought the AI library may be loaded only once, it can be logically
	 * loaded multiple times.
	 * Example: If we load one RAI and two AAIs over this interface,
	 * the interface load counter will be three.
	 */
	virtual int GetLoadCount() const = 0;
	
	
	
//	virtual std::vector<ISkirmishAILibraryInfo*> GetSkirmishAILibraryInfos(bool forceLoadFromLibrary = false) const;
	/**
	 * Returns the specifyers for all Skirmish AIs available through this interface.
	 */
	//virtual std::vector<SSAISpecifyer> GetSkirmishAILibrarySpecifyers() const = 0;
	/**
	 * @brief	loads the AI library
	 * This only loads the AI library, and does not yet create an instance
	 * for a team.
	 * For the C and C++ AI interface eg, this will load a shared library.
	 * Increments the load counter.
	 */
	//virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const SSAISpecifyer& sAISpecifyer) = 0;
	//virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const InfoItem* infos, unsigned int numInfos) = 0;
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const CSkirmishAILibraryInfo* skirmishAIInfo) = 0;
	/**
	 * @brief	unloads the Skirmish AI library
	 * This unloads the Skirmish AI library.
	 * For the C and C++ AI interface eg, this will unload a shared library.
	 * This should not be done when any instances
	 * of that AI are still in use, as it will result in a crash.
	 * Decrements the load counter.
	 */
	virtual int ReleaseSkirmishAILibrary(const SSAISpecifyer& sAISpecifyer) = 0;
	/**
	 * @brief	is the Skirmish AI library loaded
	 */
	bool IsSkirmishAILibraryLoaded(const SSAISpecifyer& sAISpecifyer) const {
		return GetSkirmishAILibraryLoadCount(sAISpecifyer) > 0;
	}
	/**
	 * @brief	how many times is the Skirmish AI loaded
	 * Thought the AI library may be loaded only once, it can be logically
	 * loaded multiple times (load counter).
	 */
	virtual int GetSkirmishAILibraryLoadCount(const SSAISpecifyer& sAISpecifyer) const = 0;
	/**
	 * @brief	unloads all AIs
	 * Unloads all AI libraries currently loaded through this interface.
	 */
	virtual int ReleaseAllSkirmishAILibraries() = 0;
	
	
	
//	virtual std::vector<IGroupAILibraryInfo*> GetGroupAILibraryInfos(bool forceLoadFromLibrary = false) const;
	//virtual std::vector<SGAISpecifyer> GetGroupAILibrarySpecifyers() const = 0;
	//virtual const IGroupAILibrary* FetchGroupAILibrary(const SGAISpecifyer& gAISpecifyer) = 0;
	//virtual const IGroupAILibrary* FetchGroupAILibrary(const InfoItem* infos, unsigned int numInfos) = 0;
	virtual const IGroupAILibrary* FetchGroupAILibrary(const CGroupAILibraryInfo* aiInfo) = 0;
	virtual int ReleaseGroupAILibrary(const SGAISpecifyer& gAISpecifyer) = 0;
	virtual int GetGroupAILibraryLoadCount(const SGAISpecifyer& gAISpecifyer) const = 0;
	bool IsGroupAILibraryLoaded(const SGAISpecifyer& gAISpecifyer) const {
		return GetGroupAILibraryLoadCount(gAISpecifyer) > 0;
	}
	virtual int ReleaseAllGroupAILibraries() = 0;
};

#endif	/* _IAIINTERFACELIBRARY_H */

