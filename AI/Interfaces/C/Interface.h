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

#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "ExternalAI/Interface/SSAILibrary.h"
#include "ExternalAI/Interface/SGAILibrary.h"

#include <map>
#include <vector>
#include <string>

enum LevelOfSupport;
class SharedLib;

class CInterface {
public:
	CInterface();
    
	// static properties
	int GetInfos(InfoItem infos[], int max);
	LevelOfSupport GetLevelOfSupportFor(
			const char* engineVersion, int engineAIInterfaceGeneratedVersion);

	// skirmish AI methods
	int GetSkirmishAISpecifyers(SSAISpecifyer* sAISpecifyers, int max) const;
	const SSAILibrary* LoadSkirmishAILibrary(const SSAISpecifyer* const sAISpecifyer);
	int UnloadSkirmishAILibrary(const SSAISpecifyer* const sAISpecifyer);
	int UnloadAllSkirmishAILibraries();

	// group AI methods
	int GetGroupAISpecifyers(SGAISpecifyer* gAISpecifyers, int max) const;
	const SGAILibrary* LoadGroupAILibrary(const SGAISpecifyer* const gAISpecifyer);
	int UnloadGroupAILibrary(const SGAISpecifyer* const gAISpecifyer);
	int UnloadAllGroupAILibraries();
	
private:
	// these functions actually load and unload the libraries
	SharedLib* Load(const SSAISpecifyer* const sAISpecifyer, SSAILibrary* ai);
	SharedLib* LoadSkirmishAILib(const std::string& libFilePath, SSAILibrary* ai);
	
	SharedLib* Load(const SGAISpecifyer* const gAISpecifyer, SGAILibrary* ai);
	SharedLib* LoadGroupAILib(const std::string& libFilePath, SGAILibrary* ai);
	
	static void reportInterfaceFunctionError(const std::string& libFileName, const std::string& functionName);
	static void reportError(const std::string& msg);
	std::string GenerateLibFilePath(const SSAISpecifyer& sAISpecifyer);
	std::string GenerateLibFilePath(const SGAISpecifyer& gAISpecifyer);
	//static std::string GenerateLibFilePath(const std::string& basePath, const std::string& libFileName);
#define MAX_INFOS 128
	static SSAISpecifyer extractSpecifyer(const SSAILibrary& skirmishAILib);
	static SGAISpecifyer extractSpecifyer(const SGAILibrary& groupAILib);
	
	static std::vector<std::string> FindDirs(const std::string& path, const std::string& pattern = ".*");
	/**
	 * Searches non-recursively for files under path, with a given file extension.
	 * The fileExtension you supplied have to contain the '.' as well, it will not be prepended.
	 */
	static std::vector<std::string> FindFiles(const std::string& path, const std::string& fileExtension);
	//static bool FileExists(const std::string& path, const std::string& fileName);
	static std::vector<std::string> FindFilesRegex(const std::string& path, const std::string& pattern);
	
	bool FitsThisInterface(const std::string& requestedShortName, const std::string& requestedVersion);
	void errorMsgFileNames(const std::string& additionalTxt = "") const;
private:
//	std::map<std::string, std::string> myProperties;
	std::vector<InfoItem> myInfos;
	
	std::vector<SSAISpecifyer> mySkirmishAISpecifyers;
	typedef std::map<SSAISpecifyer, std::string, SSAISpecifyer_Comparator> T_skirmishAIFileNames;
	T_skirmishAIFileNames mySkirmishAIFileNames;
	typedef std::map<SSAISpecifyer, SSAILibrary*, SSAISpecifyer_Comparator> T_skirmishAIs;
	T_skirmishAIs myLoadedSkirmishAIs;
	typedef std::map<SSAISpecifyer, SharedLib*, SSAISpecifyer_Comparator> T_skirmishAILibs;
	T_skirmishAILibs myLoadedSkirmishAILibs;
	
	std::vector<SGAISpecifyer> myGroupAISpecifyers;
	typedef std::map<SGAISpecifyer, std::string, SGAISpecifyer_Comparator> T_groupAIFileNames;
	T_groupAIFileNames myGroupAIFileNames;
	typedef std::map<SGAISpecifyer, SGAILibrary*, SGAISpecifyer_Comparator> T_groupAIs;
	T_groupAIs myLoadedGroupAIs;
	typedef std::map<SGAISpecifyer, SharedLib*, SGAISpecifyer_Comparator> T_groupAILibs;
	T_groupAILibs myLoadedGroupAILibs;
};

#endif	/* _INTERFACE_H */

