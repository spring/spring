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

#ifndef _AIINTERFACELIBRARY_H
#define	_AIINTERFACELIBRARY_H

#include "IAIInterfaceLibrary.h"

#include "Platform/SharedLib.h"
#include "Interface/SAIInterfaceLibrary.h"

#include <string>
#include <map>

class CAIInterfaceLibraryInfo;
class CSkirmishAILibraryInfo;
class CGroupAILibraryInfo;

class CAIInterfaceLibrary : public IAIInterfaceLibrary {
public:
//	CAIInterfaceLibrary(const CAIInterfaceLibrary& interface);
//	CAIInterfaceLibrary(const std::string& libFileName);
	//CAIInterfaceLibrary(const std::string& libFileName, const SAIInterfaceSpecifier& interfaceSpecifier);
	//CAIInterfaceLibrary(const SAIInterfaceSpecifier& interfaceSpecifier, const std::string& libFileName = "");
	CAIInterfaceLibrary(const CAIInterfaceLibraryInfo* info);
	virtual ~CAIInterfaceLibrary();
	
//	static CAIInterfaceLibrary* Load(const std::string& libFileName); // increments load counter
//	static CAIInterfaceLibrary* Get(const std::string& shortName, const std::string& version); // does not increment load counter
	
	virtual SAIInterfaceSpecifier GetSpecifier() const;
	virtual LevelOfSupport GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber) const;
	
//    virtual std::string GetProperty(const std::string& propertyName) const;
    virtual std::map<std::string, InfoItem> GetInfo() const;
	virtual int GetLoadCount() const;
	
	// Skirmish AI methods
	//virtual std::vector<SSAISpecifier> GetSkirmishAILibrarySpecifiers() const;
	//virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const SSAISpecifier& sAISpecifier);
	//virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const InfoItem* info, unsigned int numInfo);
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const CSkirmishAILibraryInfo* aiInfo);
	virtual int ReleaseSkirmishAILibrary(const SSAISpecifier& sAISpecifier);
	virtual int GetSkirmishAILibraryLoadCount(const SSAISpecifier& sAISpecifier) const;
	virtual int ReleaseAllSkirmishAILibraries();
	
	// Group AI methods
	//virtual std::vector<SGAISpecifier> GetGroupAILibrarySpecifiers() const;
	//virtual const IGroupAILibrary* FetchGroupAILibrary(const SGAISpecifier& gAISpecifier);
	//virtual const IGroupAILibrary* FetchGroupAILibrary(const InfoItem* info, unsigned int numInfo);
	virtual const IGroupAILibrary* FetchGroupAILibrary(const CGroupAILibraryInfo* aiInfo);
	virtual int ReleaseGroupAILibrary(const SGAISpecifier& gAISpecifier);
	virtual int GetGroupAILibraryLoadCount(const SGAISpecifier& gAISpecifier) const;
	virtual int ReleaseAllGroupAILibraries();
	
private:
	SStaticGlobalData* staticGlobalData;
	void InitStatic();
	void ReleaseStatic();
	
private:
	//std::string aiInterfacesLibDir;
	std::string libFilePath;
	SharedLib* sharedLib;
	SAIInterfaceLibrary sAIInterfaceLibrary;
	//SAIInterfaceSpecifier specifier;
	const CAIInterfaceLibraryInfo* info;
	std::map<const SSAISpecifier, ISkirmishAILibrary*, SSAISpecifier_Comparator> loadedSkirmishAILibraries;
	std::map<const SSAISpecifier, int, SSAISpecifier_Comparator> skirmishAILoadCount;
	std::map<const SGAISpecifier, IGroupAILibrary*, SGAISpecifier_Comparator> loadedGroupAILibraries;
	std::map<const SGAISpecifier, int, SGAISpecifier_Comparator> groupAILoadCount;
	
private:
	static const int MAX_INFOS = 128;
	
	static void reportInterfaceFunctionError(const std::string* libFileName,
			const std::string* functionName);
	int InitializeFromLib(const std::string& libFilePath);
	
	//static std::string FindLibFile(const std::string& fileNameMainPart);
	std::string FindLibFile();
};

#endif	/* _AIINTERFACELIBRARY_H */

