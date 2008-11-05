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

#ifndef _AILIBRARYMANAGER_H
#define	_AILIBRARYMANAGER_H

#include "IAILibraryManager.h"

#include "AIInterfaceLibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"
#include "GroupAILibraryInfo.h"

#include <vector>
#include <map>
#include <set>

class CAILibraryManager : public IAILibraryManager {
private:
//	typedef s_cont<ISkirmishAILibraryInterfaceInfo>::vector T_interfaceInfos;
//	typedef s_cont<CSkirmishAILibraryInfoKey>::vector T_infoKeys;
////	typedef s_cont<ISkirmishAILibraryInfo>::vector T_aiInfos;
//	typedef s_cont<std::string>::vector T_specifiers;
//	
//	typedef s_assoc<ISkirmishAILibraryInterfaceInfo, std::string>::map T_fileNames;
//	typedef s_assoc<ISkirmishAILibraryInterfaceInfo, ISkirmishAILibraryInterface>::map T_loadedInterfaces;
	
public:
	CAILibraryManager(); // looks for interface and AIs supported by them (ret != 0: error)
	~CAILibraryManager(); // unloads all shared libraries that are currently loaded (interfaces and implementations)
	
	virtual const T_interfaceSpecs* GetInterfaceSpecifiers() const;
	virtual const T_skirmishAIKeys* GetSkirmishAIKeys() const;
	virtual const T_groupAIKeys* GetGroupAIKeys() const;
	
	virtual const T_interfaceInfos* GetInterfaceInfos() const;
	virtual const T_skirmishAIInfos* GetSkirmishAIInfos() const;
	virtual const T_groupAIInfos* GetGroupAIInfos() const;
	
	virtual const T_skirmishAIInfos* GetUsedSkirmishAIInfos();
	
	virtual const T_dupInt* GetDuplicateInterfaceInfos() const;
	virtual const T_dupSkirm* GetDuplicateSkirmishAIInfos() const;
	virtual const T_dupGroup* GetDuplicateGroupAIInfos() const;

	virtual std::vector<SSAIKey> ResolveSkirmishAIKey(const SSAISpecifier& skirmishAISpecifier) const;
	virtual std::vector<SSAIKey> ResolveSkirmishAIKey(const std::string& skirmishAISpecifier) const;
	// a Skirmish AI (its library) is only really loaded when it is not yet loaded.
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const SSAIKey& skirmishAIKey);
	// a Skirmish AI is only unloaded when ReleaseSkirmishAILibrary() is called
	// as many times as GetSkirmishAILibrary() was.
	// loading and unloading of the interfaces
	// is handled internally/automatically.
	virtual void ReleaseSkirmishAILibrary(const SSAIKey& skirmishAIKey);
	virtual void ReleaseAllSkirmishAILibraries(); // unloads all currently Skirmish loaded AIs
	
	virtual std::vector<SGAIKey> ResolveGroupAIKey(const std::string& groupAISpecifier) const;
	// a Group AI (its library) is only really loaded when it is not yet loaded.
	virtual const IGroupAILibrary* FetchGroupAILibrary(const SGAIKey& groupAIKey);
	// a Group AI is only unloaded when ReleaseSkirmishAILibrary() is called
	// as many times as GetSkirmishAILibrary() was.
	// loading and unloading of the interfaces
	// is handled internally/automatically.
	virtual void ReleaseGroupAILibrary(const SGAIKey& groupAIKey);
	virtual void ReleaseAllGroupAILibraries(); // unloads all currently loaded Group AIs
	
	virtual void ReleaseEverything(); // unloads all currently loaded AIs and interfaces
	
private:
	typedef std::map<const SAIInterfaceSpecifier, IAIInterfaceLibrary*, SAIInterfaceSpecifier_Comparator> T_loadedInterfaces;
	T_loadedInterfaces loadedAIInterfaceLibraries;
	
	T_interfaceSpecs interfaceSpecifiers;
	T_skirmishAIKeys skirmishAIKeys;
	T_groupAIKeys groupAIKeys;
//	std::vector<const SAIInterfaceSpecifier> interfaceLibrarySpecifiers;
//	std::vector<const SSAIKey> skirmishAILibrarySpecifiers;
//	std::vector<const SGAIKey> groupAILibrarySpecifiers;
	//std::map<const SAIInterfaceSpecifier, std::string, SAIInterfaceSpecifier_Comparator> interfaceFileNames; // file name of the AI interface library and (LUA-)info cache file (these two have to be the same [without extension])
	
	T_interfaceInfos interfaceInfos;
	T_skirmishAIInfos skirmishAIInfos;
	T_groupAIInfos groupAIInfos;
	
	T_skirmishAIInfos usedSkirmishAIInfos;
	bool usedSkirmishAIInfos_initialized;
	
	T_dupInt duplicateInterfaceInfos;
	T_dupSkirm duplicateSkirmishAIInfos;
	T_dupGroup duplicateGroupAIInfos;
//	T_interfaceInfos interfaceInfos;
//	T_infoKeys infoKeys;
////	typedef s_cont<ISkirmishAILibraryInfo>::vector T_aiInfos;
////	typedef s_cont<ISkirmishAILibraryInfo>::const_vector T_aiInfos_const;
////	T_aiInfos aiInfos;
//	T_specifiers specifiers;
//	T_fileNames fileNames; // file name of the library and (LUA-)info cache file (without extension)
//	
//	T_loadedInterfaces loadedInterfaces;
////	std::map<const ISkirmishAILibraryInterfaceInfo, ISkirmishAILibraryInterface*> interfaceInfo_interface; // loaded interfaces
////	std::map<const CSkirmishAILibraryInfoKey&, CSkirmishAILibraryKey*> infoKey_key; // loaded AI keys
////	std::map<const ISkirmishAILibraryInterfaceInfo&, SAIInterfaceLibrary*> interfaceInfo_interface; // interface shared-lib file and interface
////	std::map<const ISkirmishAILibraryInterfaceInfo&, int> interfaceInfo_loadCounter; // interface shared-lib file and how many times it is loaded
//	
////	std::map<const std::string, const SAI*> aiLibFile_ai; // ai shared-lib file and ai
////	std::map<const std::string, int> aiLibFile_counter; // ai shared-lib file and how many times it is loaded
	
private:
	/**
	 * Loads the interface if it is not yet loaded; increments load count.
	 */
	IAIInterfaceLibrary* FetchInterface(const SAIInterfaceSpecifier& interfaceSpecifier);
	/**
	 * Unloads the interface if its load count reaches 0.
	 */
	void ReleaseInterface(const SAIInterfaceSpecifier& interfaceSpecifier);
//	// the interface has to be loaded already. if it is not, a pointer to NULL will be returned.
//	s_p<ISkirmishAILibraryInterface> GetInterface(const s_p<const ISkirmishAILibraryInterfaceInfo>& interfaceInfo) const;
//	// if the filename is not found, a pointer to NULL will be returned.
//	s_p<std::string> GetFileName(const s_p<const ISkirmishAILibraryInterfaceInfo>& interfaceInfo) const;
//	// an interface is only really loaded when it is not yet loaded.
//	s_p<ISkirmishAILibraryInterface> LoadInterface(const s_p<const ISkirmishAILibraryInterfaceInfo>& interfaceInfo);
//	// actually loads an interface (always loads it)
//	static s_p<ISkirmishAILibraryInterface> InitializeInterface(const std::string& libFileName);
//	// an interface is only unloaded when UnloadInterface() is called
//	// as many times as LoadInterface() was.
//	void UnloadInterface(const s_p<const ISkirmishAILibraryInterfaceInfo>& interfaceInfo);
//	// actually unloads an interface (always unloads it)
//	static void DeinitializeInterface(const std::string& libFileName);
//	
//	std::string GenerateLibFilePath(const s_p<const ISkirmishAILibraryInterfaceInfo>& interfaceInfo);
////	static std::string GenerateLibFileName(const ISkirmishAILibraryInterfaceInfo& interfaceInfo);
////	static ISkirmishAILibraryInterfaceInfo GenerateInfo(const std::string& libFileName);
	/**
	 * Loads all libraries and queries htem for info about available AIs.
	 */
	//void GetAllInfosFromLibraries();
	/**
	 * Loads info about available AI Interfaces and AIs from LUA Info files.
	 * -> interface and AI libraries can not corrupt the engines memory
	 *
	 * The files are searched in all data-dirs (see fs.GetDataDirectories())
	 * in the following sub-dirs:
	 * {AI_INTERFACES_DATA_DIR}/{*}/InterfaceInfo.lua
	 * {AI_INTERFACES_DATA_DIR}/{*}/{*}/InterfaceInfo.lua
	 * {SKIRMISH_AI_DATA_DIR}/{*}/AIInfo.lua
	 * {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIInfo.lua
	 * {GROUP_AI_DATA_DIR}/{*}/AIInfo.lua
	 * {GROUP_AI_DATA_DIR}/{*}/{*}/AIInfo.lua
	 *
	 * examples:
	 * AI/Skirmish/KAIK-0.13/AIInfo.lua
	 * AI/Skirmish/RAI/0.601/AIInfo.lua
	 */
	void GetAllInfosFromCache();
	/**
	 * Clears info about available AIs.
	 */
	void ClearAllInfos();
	
private:
	// helper functions
	static void reportError(const char* topic, const char* msg);
	static void reportError1(const char* topic, const char* msg, const char* arg0);
	static void reportError2(const char* topic, const char* msg, const char* arg0, const char* arg1);
	static void reportInterfaceFunctionError(const std::string* libFileName, const std::string* functionName);
	static std::string extractFileName(const std::string& libFile, bool includeExtension);
	static std::vector<std::string> FindFiles(const std::string& path, const std::string& fileExtension);
	static std::vector<std::string> FindDirsAndDirectSubDirs(const std::string& path);
	/**
	 * Finds the best fitting interface.
	 * The  short name has to fit perfectly, and the version of the interface
	 * has to be equal or higher then the requested one.
	 * If there are multiple fitting interfaces, the one with the next higher
	 * version is selected, eg:
	 * wanted: 0.2
	 * available: 0.1, 0.3, 0.5
	 * chosen: 0.3
	 */
	static SAIInterfaceSpecifier FindFittingInterfaceSpecifier(
			const std::string& shortName,
			const std::string& minVersion,
			const T_interfaceSpecs& specs);
	static int versionCompare(
			const std::string& version1,
			const std::string& version2);
};

#endif	/* _AILIBRARYMANAGER_H */
