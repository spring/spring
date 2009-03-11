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

#ifndef _IAILIBRARYMANAGER_H
#define	_IAILIBRARYMANAGER_H

#include "IAIInterfaceLibrary.h"
#include "ISkirmishAILibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"

#include <vector>
#include <map>
#include <set>

class AIInterfaceKey;
class SkirmishAIKey;

/**
 * @brief manages AIs and AI interfaces
 * @see CAILibraryManager
 */
class IAILibraryManager {

public:
	virtual ~IAILibraryManager() {}

	typedef std::set<AIInterfaceKey> T_interfaceSpecs;
	typedef std::set<SkirmishAIKey> T_skirmishAIKeys;

	virtual const T_interfaceSpecs& GetInterfaceKeys() const = 0;
	virtual const T_skirmishAIKeys& GetSkirmishAIKeys() const = 0;

	typedef std::map<const AIInterfaceKey, CAIInterfaceLibraryInfo*>
			T_interfaceInfos;
	typedef std::map<const SkirmishAIKey, CSkirmishAILibraryInfo*>
			T_skirmishAIInfos;

	virtual const T_interfaceInfos& GetInterfaceInfos() const = 0;
	virtual const T_skirmishAIInfos& GetSkirmishAIInfos() const = 0;

	virtual const std::vector<std::string>& GetSkirmishAIOptionValueKeys(int teamId) const = 0;
	virtual const std::map<std::string, std::string>& GetSkirmishAIOptionValues(int teamId) const = 0;

	virtual const T_skirmishAIInfos& GetUsedSkirmishAIInfos() = 0;

	typedef std::map<const AIInterfaceKey, std::set<std::string> > T_dupInt;
	typedef std::map<const SkirmishAIKey, std::set<std::string> >
			T_dupSkirm;

	// The following three methods return sets of files which contain duplicate
	// infos. These methods can be used for issueing warnings.
	virtual const T_dupInt& GetDuplicateInterfaceInfos() const = 0;
	virtual const T_dupSkirm& GetDuplicateSkirmishAIInfos() const = 0;

	SkirmishAIKey ResolveSkirmishAIKey(const SkirmishAIKey& skirmishAIKey)
			const;
protected:
	virtual std::vector<SkirmishAIKey> FittingSkirmishAIKeys(
			const SkirmishAIKey& skirmishAIKey) const = 0;
public:
	/**
	 * A Skirmish AI (its library) is only really loaded when it is not yet
	 * loaded.
	 */
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(
			const SkirmishAIKey& skirmishAIKey) = 0;
	/**
	 * A Skirmish AI is only unloaded when ReleaseSkirmishAILibrary() is called
	 * as many times as GetSkirmishAILibrary() was.
	 * loading and unloading of the interfaces
	 * is handled internally/automatically.
	 */
	virtual void ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey) = 0;
	/** Unloads all currently Skirmish loaded AIs. */
	virtual void ReleaseAllSkirmishAILibraries() = 0;

	/** Unloads all currently loaded AIs and interfaces. */
	virtual void ReleaseEverything() = 0;

public:
	/** Guaranteed to not return NULL. */
	static IAILibraryManager* GetInstance();
	static void OutputAIInterfacesInfo();
	static void OutputSkirmishAIInfo();
private:
	static IAILibraryManager* myAILibraryManager;
};

#endif // _IAILIBRARYMANAGER_H
