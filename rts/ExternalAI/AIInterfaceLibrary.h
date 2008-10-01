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

class CAIInterfaceLibrary : public IAIInterfaceLibrary {
public:
//	CAIInterfaceLibrary(const CAIInterfaceLibrary& interface);
//	CAIInterfaceLibrary(const std::string& libFileName);
	CAIInterfaceLibrary(const SAIInterfaceSpecifyer& interfaceSpecifyer, const std::string& libFileName = "");
	//CAIInterfaceLibrary(const std::string& libFileName, const SAIInterfaceSpecifyer& interfaceSpecifyer);
	virtual ~CAIInterfaceLibrary();
	
//	static CAIInterfaceLibrary* Load(const std::string& libFileName); // increments load counter
//	static CAIInterfaceLibrary* Get(const std::string& shortName, const std::string& version); // does not increment load counter
	
	virtual SAIInterfaceSpecifyer GetSpecifyer() const;
	virtual LevelOfSupport GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber) const;
	
//    virtual std::string GetProperty(const std::string& propertyName) const;
    virtual std::map<std::string, InfoItem> GetInfos() const;
	virtual int GetLoadCount() const;
	
	// Skirmish AI methods
	virtual std::vector<SSAISpecifyer> GetSkirmishAILibrarySpecifyers() const;
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const SSAISpecifyer& sAISpecifyer);
	virtual int ReleaseSkirmishAILibrary(const SSAISpecifyer& sAISpecifyer);
	virtual int GetSkirmishAILibraryLoadCount(const SSAISpecifyer& sAISpecifyer) const;
	virtual int ReleaseAllSkirmishAILibraries();
	
	// Group AI methods
	virtual std::vector<SGAISpecifyer> GetGroupAILibrarySpecifyers() const;
	virtual const IGroupAILibrary* FetchGroupAILibrary(const SGAISpecifyer& gAISpecifyer);
	virtual int ReleaseGroupAILibrary(const SGAISpecifyer& gAISpecifyer);
	virtual int GetGroupAILibraryLoadCount(const SGAISpecifyer& gAISpecifyer) const;
	virtual int ReleaseAllGroupAILibraries();
	
private:
//	static s_assoc<const std::string, const std::string> specifyers; // "libFileName" -> "shortName#version"
//	static s_assoc<const std::string, CSkirmishAILibraryInterface>::map interfaces; // "shortName#version" -> interface
	
private:
	SharedLib* sharedLib;
	SAIInterfaceLibrary sAIInterfaceLibrary;
	SAIInterfaceSpecifyer specifyer;
	std::map<const SSAISpecifyer, ISkirmishAILibrary*, SSAISpecifyer_Comparator> loadedSkirmishAILibraries;
	std::map<const SSAISpecifyer, int, SSAISpecifyer_Comparator> skirmishAILoadCount;
	std::map<const SGAISpecifyer, IGroupAILibrary*, SGAISpecifyer_Comparator> loadedGroupAILibraries;
	std::map<const SGAISpecifyer, int, SGAISpecifyer_Comparator> groupAILoadCount;
	
private:
	static const int MAX_INFOS = 128;
	
	static void reportInterfaceFunctionError(const std::string* libFileName, const std::string* functionName);
	int InitializeFromLib(const std::string& libFilePath);
	
	static std::string GenerateLibFilePath(const SAIInterfaceSpecifyer& interfaceSpecifyer);
	static std::string GenerateLibFilePath(const std::string& libFileName);
};

#endif	/* _AIINTERFACELIBRARY_H */

