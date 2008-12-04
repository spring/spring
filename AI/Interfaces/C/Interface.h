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
#include "ExternalAI/Interface/SSkirmishAISpecifier.h"
//#include "ExternalAI/Interface/SGAILibrary.h"

#include <map>
#include <set>
#include <vector>
#include <string>

enum LevelOfSupport;
class SharedLib;
struct SStaticGlobalData;
//struct SSkirmishAISpecifier;
//struct SSkirmishAISpecifier_Comparator;

class CInterface {
public:
	//CInterface();
	CInterface(const std::map<std::string, std::string>& infoMap,
			const SStaticGlobalData* staticGlobalData);

	// static properties
//	unsigned int GetInfo(InfoItem info[], unsigned int maxInfoItems);
	LevelOfSupport GetLevelOfSupportFor(
			const char* engineVersion, int engineAIInterfaceGeneratedVersion);

	// skirmish AI methods
	const SSAILibrary* LoadSkirmishAILibrary(
			const std::map<std::string, std::string>& infoMap);
	int UnloadSkirmishAILibrary(
			const std::map<std::string, std::string>& infoMap);
	int UnloadAllSkirmishAILibraries();

//	// group AI methods
//	const SGAILibrary* LoadGroupAILibrary(const struct InfoItem info[],
//			unsigned int numInfoItems);
//	int UnloadGroupAILibrary(const SGAISpecifier* const gAISpecifier);
//	int UnloadAllGroupAILibraries();

private:
	static SSkirmishAISpecifier ExtractSpecifier(
		const std::map<std::string, std::string>& infoMap);
	// these functions actually load and unload the libraries
	SharedLib* Load(const SSkirmishAISpecifier& aiKeyHash, SSAILibrary* ai);
	SharedLib* LoadSkirmishAILib(const std::string& libFilePath,
			SSAILibrary* ai);

//	SharedLib* Load(const SGAISpecifier* const gAISpecifier, SGAILibrary* ai);
//	SharedLib* LoadGroupAILib(const std::string& libFilePath, SGAILibrary* ai);

	static void reportInterfaceFunctionError(const std::string& libFileName,
			const std::string& functionName);
	static void reportError(const std::string& msg);
	std::string FindLibFile(const SSkirmishAISpecifier& sAISpecifier);
//	std::string FindLibFile(const SGAISpecifier& gAISpecifier);
	/**
	 * Searches for a file in all data-dirs.
	 * If not found, the input param relativeFilePath is returned.
	 */
	std::string FindFile(const std::string& relativeFilePath);
	/**
	 * Searches for a dir in all data-dirs.
	 * @param	create	if true, and the dir can not be found, it is created
	 */
	std::string FindDir(const std::string& relativeDirPath,
			bool searchOnlyWriteable, bool pretendAvailable);
	/**
	 * Returns true if the file or directory exists.
	 */
	static bool FileExists(const std::string& filePath);
	/**
	 * Creates the directory if it does not yet exist.
	 *
	 * @return	true if the directory was created or already existed
	 */
	static bool MakeDir(const std::string& dirPath);
	/**
	 * Creates the directory and all parent directories that do not yet exist.
	 *
	 * @return	true if the directory was created or already existed
	 */
	static bool MakeDirRecursive(const std::string& dirPath);
//	static SSAISpecifier ExtractSpecifier(const SSAILibrary& skirmishAILib);
//	static SGAISpecifier ExtractSpecifier(const SGAILibrary& groupAILib);

	bool FitsThisInterface(const std::string& requestedShortName,
			const std::string& requestedVersion);
private:
//	static std::string relSkirmishAIImplsDir;
//	static std::string relGroupAIImplsDir;
	const std::map<std::string, std::string> myInfo;
	const SStaticGlobalData* staticGlobalData;

	std::vector<std::string> springDataDirs;
	/**
	 * All accompanying data for this interface that is not version specifc
	 * should go in here.
	 */
	std::string myDataDir;
	/**
	 * All accompanying data for this interface that is version specifc
	 * should go in here.
	 */
	std::string myDataDirVers;
	//const SStaticGlobalData* staticGlobalData;
	//std::string skirmishAIsLibDir;
	//std::string groupAIsLibDir;
//	std::vector<InfoItem> myInfo;

//	std::vector<SSAISpecifier> mySkirmishAISpecifiers;
//	typedef std::map<SSAISpecifier, std::map<std::string, std::string>,SSAISpecifier_Comparator> T_skirmishAIInfos;
//	typedef std::map<SSAISpecifier, SSAILibrary*, SSAISpecifier_Comparator> T_skirmishAIs;
//	typedef std::map<SSAISpecifier, SharedLib*, SSAISpecifier_Comparator> T_skirmishAILibs;
	std::set<SSkirmishAISpecifier, SSkirmishAISpecifier_Comparator> mySkirmishAISpecifiers;
	typedef std::map<const SSkirmishAISpecifier, std::map<std::string, std::string>, SSkirmishAISpecifier_Comparator> T_skirmishAIInfos;
	typedef std::map<const SSkirmishAISpecifier, SSAILibrary*, SSkirmishAISpecifier_Comparator> T_skirmishAIs;
	typedef std::map<const SSkirmishAISpecifier, SharedLib*, SSkirmishAISpecifier_Comparator> T_skirmishAILibs;
	T_skirmishAIInfos mySkirmishAIInfos;
	T_skirmishAIs myLoadedSkirmishAIs;
	T_skirmishAILibs myLoadedSkirmishAILibs;

//	std::vector<SGAISpecifier> myGroupAISpecifiers;
//	typedef std::map<SGAISpecifier, std::map<std::string, InfoItem>, SGAISpecifier_Comparator> T_groupAIInfos;
//	T_groupAIInfos myGroupAIInfos;
//	typedef std::map<SGAISpecifier, SGAILibrary*, SGAISpecifier_Comparator> T_groupAIs;
//	T_groupAIs myLoadedGroupAIs;
//	typedef std::map<SGAISpecifier, SharedLib*, SGAISpecifier_Comparator> T_groupAILibs;
//	T_groupAILibs myLoadedGroupAILibs;
};

#endif // _INTERFACE_H
