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
#include "GroupAILibraryInfo.h"

#include <vector>
#include <map>
#include <set>

struct SSAIKey;
struct SGAIKey;

/**
 * @brief manages AIs and AI interfaces
 */
class IAILibraryManager {
	
public:
	virtual ~IAILibraryManager() {}
	
	typedef std::set<SAIInterfaceSpecifier, SAIInterfaceSpecifier_Comparator> T_interfaceSpecs;
	typedef std::set<SSAIKey, SSAIKey_Comparator> T_skirmishAIKeys;
	typedef std::set<SGAIKey, SGAIKey_Comparator> T_groupAIKeys;
	
	virtual const T_interfaceSpecs* GetInterfaceSpecifiers() const = 0;
	virtual const T_skirmishAIKeys* GetSkirmishAIKeys() const = 0;
	virtual const T_groupAIKeys* GetGroupAIKeys() const = 0;
	
	typedef std::map<const SAIInterfaceSpecifier, CAIInterfaceLibraryInfo*,
			SAIInterfaceSpecifier_Comparator> T_interfaceInfos;
	typedef std::map<const SSAIKey, CSkirmishAILibraryInfo*, SSAIKey_Comparator>
			T_skirmishAIInfos;
	typedef std::map<const SGAIKey, CGroupAILibraryInfo*, SGAIKey_Comparator>
			T_groupAIInfos;
	
	virtual const T_interfaceInfos* GetInterfaceInfos() const = 0;
	virtual const T_skirmishAIInfos* GetSkirmishAIInfos() const = 0;
	virtual const T_groupAIInfos* GetGroupAIInfos() const = 0;
	
	typedef std::map<const SAIInterfaceSpecifier, std::set<std::string>,
			SAIInterfaceSpecifier_Comparator> T_dupInt;
	typedef std::map<const SSAIKey, std::set<std::string>, SSAIKey_Comparator>
			T_dupSkirm;
	typedef std::map<const SGAIKey, std::set<std::string>, SGAIKey_Comparator>
			T_dupGroup;
	
	// The following three methods return sets of files which contain duplicate
	// infos. These methods can be used for issueing warnings.
	virtual const T_dupInt* GetDuplicateInterfaceInfos() const = 0;
	virtual const T_dupSkirm* GetDuplicateSkirmishAIInfos() const = 0;
	virtual const T_dupGroup* GetDuplicateGroupAIInfos() const = 0;

	virtual std::vector<SSAIKey> ResolveSkirmishAIKey(const SSAISpecifier& skirmishAISpecifier) const = 0;
	virtual std::vector<SSAIKey> ResolveSkirmishAIKey(const std::string& skirmishAISpecifier) const = 0;
	// a Skirmish AI (its library) is only really loaded when it is not yet loaded.
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const SSAIKey& skirmishAIKey) = 0;
	// a Skirmish AI is only unloaded when ReleaseSkirmishAILibrary() is called
	// as many times as GetSkirmishAILibrary() was.
	// loading and unloading of the interfaces
	// is handled internally/automatically.
	virtual void ReleaseSkirmishAILibrary(const SSAIKey& skirmishAIKey) = 0;
	virtual void ReleaseAllSkirmishAILibraries() = 0; // unloads all currently Skirmish loaded AIs
	
	virtual std::vector<SGAIKey> ResolveGroupAIKey(const std::string& groupAISpecifier) const = 0;
	// a Group AI (its library) is only really loaded when it is not yet loaded.
	virtual const IGroupAILibrary* FetchGroupAILibrary(const SGAIKey& groupAIKey) = 0;
	// a Group AI is only unloaded when ReleaseSkirmishAILibrary() is called
	// as many times as GetSkirmishAILibrary() was.
	// loading and unloading of the interfaces
	// is handled internally/automatically.
	virtual void ReleaseGroupAILibrary(const SGAIKey& groupAIKey) = 0;
	virtual void ReleaseAllGroupAILibraries() = 0; // unloads all currently loaded Group AIs
	
	virtual void ReleaseEverything() = 0; // unloads all currently loaded AIs and interfaces
	
public:
	/* guaranteed to not return NULL */
	static IAILibraryManager* GetInstance();
	static void OutputAIInterfacesInfo();
	static void OutputSkirmishAIInfo();
	static void OutputGroupAIInfo();
private:
	static IAILibraryManager* myAILibraryManager;
	
protected:
	static bool SplittAIKey(const std::string& key,
			std::string* aiName,
			std::string* aiVersion,
			std::string* interfaceName,
			std::string* interfaceVersion);
};

#endif	/* _IAILIBRARYMANAGER_H */
