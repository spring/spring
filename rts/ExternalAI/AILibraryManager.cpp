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

#include "AILibraryManager.h"

#include "Interface/aidefines.h"
#include "Interface/SAIInterfaceLibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "AIInterfaceLibrary.h"
#include "SkirmishAILibraryInfo.h"

#include <boost/filesystem.hpp>
#include "Util.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "FileSystem/FileHandler.h"
#include "Game/GlobalSynced.h"
#include "LogOutput.h"
#include "Game/Team.h"

#include <string>
#include <set>
#include <sstream>

#include <limits.h>
#include <string.h>


void CAILibraryManager::reportError(const char* topic, const char* msg) {
	handleerror(NULL, msg, topic, MBF_OK | MBF_EXCL);
}
void CAILibraryManager::reportError1(const char* topic, const char* msg, const char* arg0) {
	
	const int MAX_MSG_LENGTH = 511;
	char s_msg[MAX_MSG_LENGTH + 1];
	SNPRINTF(s_msg, MAX_MSG_LENGTH, msg, arg0);
	reportError(topic, s_msg);
}
void CAILibraryManager::reportError2(const char* topic, const char* msg, const char* arg0, const char* arg1) {
	
	const int MAX_MSG_LENGTH = 511;
	char s_msg[MAX_MSG_LENGTH + 1];
	SNPRINTF(s_msg, MAX_MSG_LENGTH, msg, arg0, arg1);
	reportError(topic, s_msg);
}

std::string CAILibraryManager::extractFileName(const std::string& libFile, bool includeExtension) {
	
	std::string::size_type firstChar = libFile.find_last_of("/\\");
	std::string::size_type lastChar = std::string::npos;
	if (!includeExtension) {
		lastChar = libFile.find_last_of('.');
	}
	
	return libFile.substr(firstChar+1, lastChar - firstChar -1);
}

CAILibraryManager::CAILibraryManager() : usedSkirmishAIInfos_initialized(false) {
	//GetAllInfosFromLibraries();
	GetAllInfosFromCache();
}

/*
void CAILibraryManager::GetAllInfosFromLibraries() {
	
	ClearAllInfos();
	
	// look for AI interface library files
	std::vector<std::string> interfaceLibFiles =
			FindFiles(std::string(PATH_TO_SPRING_HOME) +
			std::string("") + AI_INTERFACES_IMPLS_DIR,
			std::string(".") + SharedLib::GetLibExtension());
	
	// initialize the interface infos
	std::vector<std::string>::const_iterator libFile;
	for (libFile=interfaceLibFiles.begin(); libFile!=interfaceLibFiles.end(); libFile++) { // interfaces
		
		std::string fileName = std::string(extractFileName(*libFile, false));
		
		// load the interface
		IAIInterfaceLibrary* interfaceLib = new CAIInterfaceLibrary(fileName);
		if (interfaceLib == NULL) {
			reportError1("AI Error",
					"Failed to load interface shared library \"%s\"",
					libFile->c_str());
		}
		
		SAIInterfaceSpecifier interfaceSpecifier = interfaceLib->GetSpecifier();
		interfaceSpecifier = copySAIInterfaceSpecifier(&interfaceSpecifier);
		interfaceSpecifiers.push_back(interfaceSpecifier);
		
		// generate and store the interface info
		CAIInterfaceLibraryInfo* interfaceInfo = new CAIInterfaceLibraryInfo(*interfaceLib);
		interfaceInfos[interfaceSpecifier] = interfaceInfo;
		
		// generate and store the pure file name
		//interfaceFileNames[interfaceSpecifier] = fileName;
		interfaceInfo->SetFileName(fileName);
		
		// fetch the info of all Skirmish AIs available through the interface
		std::vector<SSAISpecifier> sass = interfaceLib->GetSkirmishAILibrarySpecifiers();
		std::vector<SSAISpecifier>::const_iterator sas;
		for (sas=sass.begin(); sas!=sass.end(); sas++) { // AIs
			const ISkirmishAILibrary* skirmishAI = interfaceLib->FetchSkirmishAILibrary(*sas);
			CSkirmishAILibraryInfo* skirmishAIInfo = new CSkirmishAILibraryInfo(*skirmishAI, interfaceSpecifier);
			SSAIKey skirmishAIKey = {interfaceSpecifier, copySSAISpecifier(&(*sas))};
			skirmishAIKeys.push_back(skirmishAIKey);
			skirmishAIInfos[skirmishAIKey] = skirmishAIInfo;
			interfaceLib->ReleaseSkirmishAILibrary(*sas);
		}
		
		// fetch the info of all Group AIs available through the interface
		std::vector<SGAISpecifier> gass = interfaceLib->GetGroupAILibrarySpecifiers();
		std::vector<SGAISpecifier>::const_iterator gas;
		for (gas=gass.begin(); gas!=gass.end(); gas++) { // AIs
			const IGroupAILibrary* groupAI = interfaceLib->FetchGroupAILibrary(*gas);
			CGroupAILibraryInfo* groupAIInfo = new CGroupAILibraryInfo(*groupAI, interfaceSpecifier);
			SGAIKey groupAIKey = {interfaceSpecifier, copySGAISpecifier(&(*gas))};
			groupAIKeys.push_back(groupAIKey);
			groupAIInfos[groupAIKey] = groupAIInfo;
			interfaceLib->ReleaseGroupAILibrary(*gas);
		}
		
		delete interfaceLib;
    }
}
*/

/*
std::vector<InfoItem> ParseInfos(
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes)
{
	std::vector<InfoItem> infos;
	
	//static const int MAX_INFOS = 128;
#define MAX_INFOS 128
	InfoItem tmpInfos[MAX_INFOS];
	unsigned int num = ParseInfos(fileName.c_str(), fileModes.c_str(), accessModes.c_str(), tmpInfos, MAX_INFOS);
	for (unsigned int i=0; i < num; ++i) {
		infos.push_back(copyInfoItem(&(tmpInfos[i])));
    }
	
	return infos;
}
*/

void CAILibraryManager::GetAllInfosFromCache() {
	
	ClearAllInfos();
	
/*
	if (!(vfsHandler)) {
		reportError("Initialization Error", Call InitArchiveScanner before GetAllInfosFromCache.");
	}
*/
	typedef std::vector<std::string> T_dirs;
	
	// cause we use CFileHandler for searching files,
	// we are automatically searching in all data-dirs
	
	// Read from AI Interface info files
	// we are looking for:
	// AI/Interfaces/data/ * /InterfaceInfo.lua
	// AI/Interfaces/data/ * / * /InterfaceInfo.lua
	T_dirs aiInterfaceDataDirs =
			FindDirsAndDirectSubDirs(AI_INTERFACES_DATA_DIR);
	typedef std::map<const SAIInterfaceSpecifier, std::set<std::string>,
			SAIInterfaceSpecifier_Comparator> T_dupInt;
	T_dupInt duplicateInterfaceInfoCheck;
	for (T_dirs::iterator dir = aiInterfaceDataDirs.begin();
			dir != aiInterfaceDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		T_dirs infoFile =
				CFileHandler::FindFiles(possibleDataDir, "InterfaceInfo.lua");
		if (infoFile.size() > 0) { // interface info is available
			
			// generate and store the interface info
			CAIInterfaceLibraryInfo* interfaceInfo =
					new CAIInterfaceLibraryInfo(infoFile.at(0));
			
			std::string sn = interfaceInfo->GetShortName();
			std::string v = interfaceInfo->GetVersion();
			SAIInterfaceSpecifier interfaceSpecifier = {sn.c_str(), v.c_str()};
			interfaceSpecifier = copySAIInterfaceSpecifier(&interfaceSpecifier);
			interfaceSpecifiers.insert(interfaceSpecifier);
			interfaceInfos[interfaceSpecifier] = interfaceInfo;
			
			// so we can check if one interface is specified multiple times
			duplicateInterfaceInfoCheck[interfaceSpecifier].insert(infoFile.at(0));
		}
	}
	
	// filter out interfaces that are specified multiple times
	for (T_dupInt::const_iterator info = duplicateInterfaceInfoCheck.begin();
			info != duplicateInterfaceInfoCheck.end(); ++info) {
		if (info->second.size() >= 2) {
			duplicateInterfaceInfos[info->first] = info->second;
			
			logOutput.Print("WARNING: Duplicate AI Interface Info found:");
			logOutput.Print("\tfor interface: %s %s", info->first.shortName,
					info->first.version);
			logOutput.Print("\tin files:");
			std::set<std::string>::const_iterator dir;
			for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
				logOutput.Print("\t%s", dir->c_str());
			}
		}
	}
	
	// Read from Skirmish AI info and option files
	// we are looking for:
	// AI/Skirmish/data/ * /AIInfo.lua
	// AI/Skirmish/data/ * / * /AIInfo.lua
	T_dirs skirmishAIDataDirs = FindDirsAndDirectSubDirs(SKIRMISH_AI_DATA_DIR);
	T_dupSkirm duplicateSkirmishAIInfoCheck;
	for (T_dirs::iterator dir = skirmishAIDataDirs.begin();
			dir != skirmishAIDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		T_dirs infoFile = CFileHandler::FindFiles(possibleDataDir,
				"AIInfo.lua");
		if (infoFile.size() > 0) { // skirmish AI info is available
			std::string optionFileName = "";
			T_dirs optionFile = CFileHandler::FindFiles(possibleDataDir,
					"AIOptions.lua");
			if (optionFile.size() > 0) {
				optionFileName = optionFile.at(0);
			}
			// generate and store the ai info
			CSkirmishAILibraryInfo* skirmishAIInfo =
					new CSkirmishAILibraryInfo(infoFile.at(0), optionFileName);
			
			std::string sn = skirmishAIInfo->GetShortName();
			std::string v = skirmishAIInfo->GetVersion();
			SSAISpecifier aiSpecifier = {sn.c_str(), v.c_str()};
			SAIInterfaceSpecifier interfaceSpecifier =
					FindFittingInterfaceSpecifier(
							skirmishAIInfo->GetInterfaceShortName(),
							skirmishAIInfo->GetInterfaceVersion(),
							interfaceSpecifiers);
			if (interfaceSpecifier.shortName != NULL) {
				aiSpecifier = copySSAISpecifier(&aiSpecifier);
				SSAIKey skirmishAIKey = {interfaceSpecifier, aiSpecifier};
				skirmishAIKeys.insert(skirmishAIKey);
				skirmishAIInfos[skirmishAIKey] = skirmishAIInfo;
			
				// so we can check if one skirmish AI is specified multiple times
				duplicateSkirmishAIInfoCheck[skirmishAIKey].insert(infoFile.at(0));
			}
		}
	}
	
	// filter out skirmish AIs that are specified multiple times
	for (T_dupSkirm::const_iterator info = duplicateSkirmishAIInfoCheck.begin();
			info != duplicateSkirmishAIInfoCheck.end(); ++info) {
		if (info->second.size() >= 2) {
			duplicateSkirmishAIInfos[info->first] = info->second;
			
			logOutput.Print("WARNING: Duplicate Skirmish AI Info found:");
			logOutput.Print("\tfor Skirmish AI: %s %s", info->first.ai.shortName,
					info->first.ai.version);
			logOutput.Print("\tin files:");
			std::set<std::string>::const_iterator dir;
			for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
				logOutput.Print("\t%s", dir->c_str());
			}
		}
	}
	
	
	// Read from Group AI info and option files
	// we are looking for:
	// AI/Group/data/ * /AIInfo.lua
	// AI/Group/data/ * / * /AIInfo.lua
	T_dirs groupAIDataDirs = FindDirsAndDirectSubDirs(GROUP_AI_DATA_DIR);
	T_dupGroup duplicateGroupAIInfoCheck;
	for (T_dirs::iterator dir = groupAIDataDirs.begin();
			dir != groupAIDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		T_dirs infoFile = CFileHandler::FindFiles(possibleDataDir,
				"AIInfo.lua");
		if (infoFile.size() > 0) { // group AI info is available
			std::string optionFileName = "";
			T_dirs optionFile = CFileHandler::FindFiles(possibleDataDir,
					"AIOptions.lua");
			if (optionFile.size() > 0) {
				optionFileName = optionFile.at(0);
			}
			// generate and store the ai info
			CGroupAILibraryInfo* groupAIInfo =
					new CGroupAILibraryInfo(infoFile.at(0), optionFileName);
			
			std::string sn = groupAIInfo->GetShortName();
			std::string v = groupAIInfo->GetVersion();
			SGAISpecifier aiSpecifier = {sn.c_str(), v.c_str()};
			SAIInterfaceSpecifier interfaceSpecifier =
					FindFittingInterfaceSpecifier(
							groupAIInfo->GetInterfaceShortName(),
							groupAIInfo->GetInterfaceVersion(),
							interfaceSpecifiers);
			if (interfaceSpecifier.shortName != NULL) {
				aiSpecifier = copySGAISpecifier(&aiSpecifier);
				SGAIKey groupAIKey = {interfaceSpecifier, aiSpecifier};
				groupAIKeys.insert(groupAIKey);
				groupAIInfos[groupAIKey] = groupAIInfo;
			
				// so we can check if one group AI is specified multiple times
				duplicateGroupAIInfoCheck[groupAIKey].insert(infoFile.at(0));
			}
		}
	}
	
	// filter out group AIs that are specified multiple times
	for (T_dupGroup::const_iterator info = duplicateGroupAIInfoCheck.begin();
			info != duplicateGroupAIInfoCheck.end(); ++info) {
		if (info->second.size() >= 2) {
			duplicateGroupAIInfos[info->first] = info->second;
			
			logOutput.Print("WARNING: Duplicate Group AI Info found:");
			logOutput.Print("\tfor Group AI: %s %s", info->first.ai.shortName,
					info->first.ai.version);
			logOutput.Print("\tin files:");
			std::set<std::string>::const_iterator dir;
			for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
				logOutput.Print("\t%s", dir->c_str());
			}
		}
	}
	
/*
	std::vector<std::string> infoFiles =
			FindFiles(std::string(
			std::string("") + AI_INTERFACES_DATA_DIR,
			std::string(".") + SharedLib::GetLibExtension());
	
	// initialize the interface infos
	std::vector<std::string>::const_iterator libFile;
	for (libFile=interfaceLibFiles.begin(); libFile!=interfaceLibFiles.end(); libFile++) { // interfaces
		
		std::string fileName = std::string(extractFileName(*libFile, false));
		
		// load the interface
		IAIInterfaceLibrary* interfaceLib = new CAIInterfaceLibrary(fileName);
		if (interfaceLib == NULL) {
			reportError1("AI Error",
					"Failed to load interface shared library \"%s\"",
					libFile->c_str());
		}
		
		SAIInterfaceSpecifier interfaceSpecifier = interfaceLib->GetSpecifier();
		interfaceSpecifier = copySAIInterfaceSpecifier(&interfaceSpecifier);
		interfaceSpecifiers.push_back(interfaceSpecifier);
		
		// generate and store the interface info
		CAIInterfaceLibraryInfo* interfaceInfo = new CAIInterfaceLibraryInfo(*interfaceLib);
		interfaceInfos[interfaceSpecifier] = interfaceInfo;
		
		// generate and store the pure file name
		interfaceFileNames[interfaceSpecifier] = fileName;
		
		// fetch the info of all Skirmish AIs available through the interface
		std::vector<SSAISpecifier> sass = interfaceLib->GetSkirmishAILibrarySpecifiers();
		std::vector<SSAISpecifier>::const_iterator sas;
		for (sas=sass.begin(); sas!=sass.end(); sas++) { // AIs
			const ISkirmishAILibrary* skirmishAI = interfaceLib->FetchSkirmishAILibrary(*sas);
			CSkirmishAILibraryInfo* skirmishAIInfo = new CSkirmishAILibraryInfo(*skirmishAI, interfaceSpecifier);
			SSAIKey skirmishAIKey = {interfaceSpecifier, copySSAISpecifier(&(*sas))};
			skirmishAIKeys.push_back(skirmishAIKey);
			skirmishAIInfos[skirmishAIKey] = skirmishAIInfo;
			interfaceLib->ReleaseSkirmishAILibrary(*sas);
		}
		
		// fetch the info of all Group AIs available through the interface
		std::vector<SGAISpecifier> gass = interfaceLib->GetGroupAILibrarySpecifiers();
		std::vector<SGAISpecifier>::const_iterator gas;
		for (gas=gass.begin(); gas!=gass.end(); gas++) { // AIs
			const IGroupAILibrary* groupAI = interfaceLib->FetchGroupAILibrary(*gas);
			CGroupAILibraryInfo* groupAIInfo = new CGroupAILibraryInfo(*groupAI, interfaceSpecifier);
			SGAIKey groupAIKey = {interfaceSpecifier, copySGAISpecifier(&(*gas))};
			groupAIKeys.push_back(groupAIKey);
			groupAIInfos[groupAIKey] = groupAIInfo;
			interfaceLib->ReleaseGroupAILibrary(*gas);
		}
		
		delete interfaceLib;
    }
*/
}
void CAILibraryManager::ClearAllInfos() {
	
	IAILibraryManager::T_interfaceInfos::iterator iii;
	for (iii=interfaceInfos.begin(); iii!=interfaceInfos.end(); iii++) {
		delete iii->second;
		iii->second = NULL;
	}
	
	IAILibraryManager::T_skirmishAIInfos::iterator sai;
	for (sai=skirmishAIInfos.begin(); sai!=skirmishAIInfos.end(); sai++) {
		delete sai->second;
		sai->second = NULL;
	}
	
	IAILibraryManager::T_groupAIInfos::iterator gai;
	for (gai=groupAIInfos.begin(); gai!=groupAIInfos.end(); gai++) {
		delete gai->second;
		gai->second = NULL;
	}
	
	interfaceInfos.clear();
	skirmishAIInfos.clear();
	groupAIInfos.clear();
	
	interfaceSpecifiers.clear();
	skirmishAIKeys.clear();
	groupAIKeys.clear();
}

CAILibraryManager::~CAILibraryManager() {
	
	ReleaseEverything();

	// delete all strings contained in skirmish AI specifiers
	IAILibraryManager::T_skirmishAIKeys::iterator sSpec;
	for (sSpec=skirmishAIKeys.begin(); sSpec!=skirmishAIKeys.end(); sSpec++) {
		deleteSSAISpecifier(&(sSpec->ai));
	}
	
	// delete all strings contained in group AI specifiers
	IAILibraryManager::T_groupAIKeys::iterator gSpec;
	for (gSpec=groupAIKeys.begin(); gSpec!=groupAIKeys.end(); gSpec++) {
		deleteSGAISpecifier(&(gSpec->ai));
	}
	
	// delete all strings contained in interface specifiers
	IAILibraryManager::T_interfaceSpecs::iterator iSpec;
	for (iSpec=interfaceSpecifiers.begin(); iSpec!=interfaceSpecifiers.end(); iSpec++) {
		deleteSAIInterfaceSpecifier(&(*iSpec));
	}
}

const IAILibraryManager::T_interfaceSpecs* CAILibraryManager::GetInterfaceSpecifiers() const {
	return &interfaceSpecifiers;
}
const IAILibraryManager::T_skirmishAIKeys* CAILibraryManager::GetSkirmishAIKeys() const {
	return &skirmishAIKeys;
}
const IAILibraryManager::T_groupAIKeys* CAILibraryManager::GetGroupAIKeys() const {
	return &groupAIKeys;
}

const IAILibraryManager::T_interfaceInfos* CAILibraryManager::GetInterfaceInfos() const {
	return &interfaceInfos;
}
const IAILibraryManager::T_skirmishAIInfos* CAILibraryManager::GetSkirmishAIInfos() const {
	return &skirmishAIInfos;
}
const IAILibraryManager::T_groupAIInfos* CAILibraryManager::GetGroupAIInfos() const {
	return &groupAIInfos;
}

const IAILibraryManager::T_skirmishAIInfos* CAILibraryManager::GetUsedSkirmishAIInfos() {
	
	if (!usedSkirmishAIInfos_initialized) {
/*
		std::vector<CGameSetup::TeamData>::const_iterator td;
		for (td = gameSetup->teamStartingData.begin(); td != gameSetup->teamStartingData.end(); ++td) {
*/
		const CTeam* team = NULL;
		for (unsigned int t = 0; t < (unsigned int)MAX_TEAMS; ++t) {
			team = gs->Team(t);
			if (team != NULL && team->isAI) {
				IAILibraryManager::T_skirmishAIInfos::const_iterator aiInfo;
/*
				const std::string& t_sn = td->skirmishAIShortName;
				const std::string& t_v = td->skirmishAIVersion;
*/
				const char * tmpStr = team->skirmishAISpecifier.ai.shortName;
				const std::string& t_sn = tmpStr != NULL ? tmpStr : "";
				tmpStr = team->skirmishAISpecifier.ai.version;
				const std::string& t_v = tmpStr != NULL ? tmpStr : "";
				for (aiInfo = skirmishAIInfos.begin(); aiInfo != skirmishAIInfos.end(); ++aiInfo) {
					const std::string& ai_sn = aiInfo->second->GetShortName();
					const std::string& ai_v = aiInfo->second->GetVersion();
					// add this AI info if it is used in the current game
					if (ai_sn == t_sn && (t_sn.empty() || ai_v == t_v)) {
						usedSkirmishAIInfos[aiInfo->first] = aiInfo->second;
					}
				}
			}
        }
	}
	
	return &usedSkirmishAIInfos;
}

const IAILibraryManager::T_dupInt* CAILibraryManager::GetDuplicateInterfaceInfos() const {
	return &duplicateInterfaceInfos;
}
const IAILibraryManager::T_dupSkirm* CAILibraryManager::GetDuplicateSkirmishAIInfos() const {
	return &duplicateSkirmishAIInfos;
}
const IAILibraryManager::T_dupGroup* CAILibraryManager::GetDuplicateGroupAIInfos() const {
	return &duplicateGroupAIInfos;
}


std::vector<SSAIKey> CAILibraryManager::ResolveSkirmishAIKey(
		const SSAISpecifier& skirmishAISpecifier) const {
	
	std::vector<SSAIKey> applyingKeys;
	
	if (skirmishAISpecifier.shortName == NULL || strlen(skirmishAISpecifier.shortName) == 0) {
		return applyingKeys;
	}
	
	std::string aiName(skirmishAISpecifier.shortName);
	
	bool checkVersion = false;
	std::string aiVersion;
	if (skirmishAISpecifier.version != NULL && strlen(skirmishAISpecifier.version) > 0) {
		aiVersion = std::string(skirmishAISpecifier.version);
		checkVersion = true;
	}
	
	std::set<SSAIKey>::const_iterator sasi;
	for (sasi=skirmishAIKeys.begin(); sasi!=skirmishAIKeys.end(); sasi++) {

		// check if the ai name fits
		if (aiName != sasi->ai.shortName) {
			continue;
		}

		// check if the ai version fits (if one is specifyed)
		if (checkVersion && aiVersion != sasi->ai.version) {
			continue;
		}

		// if the programm raches here, we know that this key fits
		applyingKeys.push_back(*sasi);
	}
	
	return applyingKeys;
}
std::vector<SSAIKey> CAILibraryManager::ResolveSkirmishAIKey(const std::string& skirmishAISpecifier) const {
	
	std::vector<SSAIKey> applyingKeys;
	
	std::string* aiName;
	std::string* aiVersion;
	std::string* interfaceName;
	std::string* interfaceVersion;
	bool isValid = SplittAIKey(skirmishAISpecifier, aiName, aiVersion, interfaceName, interfaceVersion);
	if (!isValid) {
		reportError1("AI Library Error", "Invalid Skirmish AI Key: %s", skirmishAISpecifier.c_str());
		return applyingKeys;
	}
	
	std::set<SSAIKey>::const_iterator sasi;
	for (sasi=skirmishAIKeys.begin(); sasi!=skirmishAIKeys.end(); sasi++) {

		// check if the ai name fits
		if (*aiName != sasi->ai.shortName) {
			continue;
		}

		// check if the ai version fits (if one is specifyed)
		if (aiVersion && *aiVersion != sasi->ai.version) {
			continue;
		}

		// check if the interface name fits (if one is specifyed)
		if (interfaceName && *interfaceName != sasi->interface.shortName) {
			continue;
		}

		// check if the interface version fits (if one is specifyed)
		if (interfaceVersion && *interfaceVersion != sasi->interface.version) {
			continue;
		}

		// if the programm raches here, we know that this key fits
		applyingKeys.push_back(*sasi);
	}
	
	return applyingKeys;
}

const ISkirmishAILibrary* CAILibraryManager::FetchSkirmishAILibrary(const SSAIKey& skirmishAIKey) {
	
	//return FetchInterface(skirmishAIKey.interface)->FetchSkirmishAILibrary(skirmishAIKey.ai);
	T_skirmishAIInfos::const_iterator aiInfo = skirmishAIInfos.find(skirmishAIKey);
	if (aiInfo == skirmishAIInfos.end()) {
		return NULL;
	}
	return FetchInterface(skirmishAIKey.interface)->FetchSkirmishAILibrary(aiInfo->second);
}

void CAILibraryManager::ReleaseSkirmishAILibrary(const SSAIKey& skirmishAIKey) {
	FetchInterface(skirmishAIKey.interface)->ReleaseSkirmishAILibrary(skirmishAIKey.ai);
	ReleaseInterface(skirmishAIKey.interface); // only releases the library if its load count is 0
}

void CAILibraryManager::ReleaseAllSkirmishAILibraries() {
	
	T_loadedInterfaces::const_iterator lil;
	for (lil=loadedAIInterfaceLibraries.begin(); lil!=loadedAIInterfaceLibraries.end(); lil++) {
		FetchInterface(lil->first)->ReleaseAllSkirmishAILibraries();
		ReleaseInterface(lil->first); // only releases the library if its load count is 0
	}
}


std::vector<SGAIKey> CAILibraryManager::ResolveGroupAIKey(const std::string& groupAISpecifier) const {
	
	std::vector<SGAIKey> applyingKeys;
	
	std::string* aiName;
	std::string* aiVersion;
	std::string* interfaceName;
	std::string* interfaceVersion;
	bool isValid = SplittAIKey(groupAISpecifier, aiName, aiVersion, interfaceName, interfaceVersion);
	if (!isValid) {
		reportError1("AI Library Error", "Invalid Group AI Key: %s", groupAISpecifier.c_str());
		return applyingKeys;
	}
	
	std::set<SGAIKey>::const_iterator gasi;
	for (gasi=groupAIKeys.begin(); gasi!=groupAIKeys.end(); gasi++) {

		// check if the ai name fits
		if (*aiName != gasi->ai.shortName) {
			continue;
		}

		// check if the ai version fits (if one is specifyed)
		if (aiVersion && *aiVersion != gasi->ai.version) {
			continue;
		}

		// check if the interface name fits (if one is specifyed)
		if (interfaceName && *interfaceName != gasi->interface.shortName) {
			continue;
		}

		// check if the interface version fits (if one is specifyed)
		if (interfaceVersion && *interfaceVersion != gasi->interface.version) {
			continue;
		}

		// if the programm raches here, we know that this key fits
		applyingKeys.push_back(*gasi);
	}
	
	return applyingKeys;
}

const IGroupAILibrary* CAILibraryManager::FetchGroupAILibrary(const SGAIKey& groupAIKey) {
	
	//return FetchInterface(groupAIKey.interface)->FetchGroupAILibrary(groupAIKey.ai);
	T_groupAIInfos::const_iterator aiInfo = groupAIInfos.find(groupAIKey);
	if (aiInfo == groupAIInfos.end()) {
		return NULL;
	}
	return FetchInterface(groupAIKey.interface)->FetchGroupAILibrary(aiInfo->second);
}

void CAILibraryManager::ReleaseGroupAILibrary(const SGAIKey& groupAIKey) {
	FetchInterface(groupAIKey.interface)->ReleaseGroupAILibrary(groupAIKey.ai);
	ReleaseInterface(groupAIKey.interface); // only releases the library if its load count is 0
}

void CAILibraryManager::ReleaseAllGroupAILibraries() {
	
	T_loadedInterfaces::const_iterator lil;
	for (lil=loadedAIInterfaceLibraries.begin(); lil!=loadedAIInterfaceLibraries.end(); lil++) {
		FetchInterface(lil->first)->ReleaseAllGroupAILibraries();
		ReleaseInterface(lil->first); // only releases the library if its load count is 0
	}
}




IAIInterfaceLibrary* CAILibraryManager::FetchInterface(const SAIInterfaceSpecifier& interfaceSpecifier) {
	
	IAIInterfaceLibrary* interfaceLib = NULL;
	
	T_loadedInterfaces::const_iterator interfacePos = loadedAIInterfaceLibraries.find(interfaceSpecifier);
	if (interfacePos == loadedAIInterfaceLibraries.end()) { // interface not yet loaded
		T_interfaceInfos::const_iterator interfaceInfo = interfaceInfos.find(interfaceSpecifier);
		if (interfaceInfo != interfaceInfos.end()) {
			//interfaceLib = new CAIInterfaceLibrary(interfaceSpecifier);
			interfaceLib = new CAIInterfaceLibrary(interfaceInfo->second);
			loadedAIInterfaceLibraries[interfaceSpecifier] = interfaceLib;
		} else {
			// unavailable interface requested, returning NULL
		}
	} else {
		interfaceLib = interfacePos->second;
	}

	return interfaceLib;
}

void CAILibraryManager::ReleaseInterface(const SAIInterfaceSpecifier& interfaceSpecifier) {
	
	T_loadedInterfaces::iterator interfacePos = loadedAIInterfaceLibraries.find(interfaceSpecifier);
	if (interfacePos != loadedAIInterfaceLibraries.end()) {
		IAIInterfaceLibrary* interfaceLib = interfacePos->second;
		if (interfaceLib->GetLoadCount() == 0) {
			loadedAIInterfaceLibraries.erase(interfacePos);
			delete interfaceLib;
			interfaceLib = NULL;
		}
	}
}


/** unloads all interfaces and AIs */
void CAILibraryManager::ReleaseEverything() {
	
	ReleaseAllSkirmishAILibraries();
	ReleaseAllGroupAILibraries();
}

std::vector<std::string> CAILibraryManager::FindFiles(const std::string& path, const std::string& fileExtension) {
	
	std::vector<std::string> found;

	if (boost::filesystem::exists(path)) {
		  boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		  for (boost::filesystem::directory_iterator itr(path); itr != end_itr; ++itr) {
			  if (!boost::filesystem::is_directory(*itr)
					  && boost::filesystem::extension(*itr) == fileExtension) {
				  found.push_back(itr->string());
			  }
		  }
	}
	
	return found;
}

std::vector<std::string> CAILibraryManager::FindDirsAndDirectSubDirs(
		const std::string& path) {
	
	std::vector<std::string> found;
	
	 std::string pattern = "*";
	
	// find dirs
	std::vector<std::string> mainDirs = CFileHandler::SubDirs(path, pattern,
			SPRING_VFS_RAW);
	found = mainDirs;
	
	// find sub-dirs
	for (std::vector<std::string>::iterator dir = mainDirs.begin();
			dir != mainDirs.end(); ++dir) {
		std::vector<std::string> sub_dirs = CFileHandler::SubDirs(*dir, pattern,
				SPRING_VFS_RAW);
		found.insert(found.end(), sub_dirs.begin(), sub_dirs.end());
	}
	
	return found;
}

SAIInterfaceSpecifier CAILibraryManager::FindFittingInterfaceSpecifier(
		const std::string& shortName,
		const std::string& minVersion,
		const T_interfaceSpecs& specs) {
	
	std::set<SAIInterfaceSpecifier>::const_iterator spec;
	int minDiff = INT_MAX;
	const char* chosenShortName = NULL;
	const char* chosenVersion = NULL;
	for (spec=specs.begin(); spec!=specs.end(); spec++) {
		if (shortName == spec->shortName) {
			int diff = versionCompare(spec->version, minVersion);
			if (diff >= 0 && diff < minDiff) {
				chosenShortName = spec->shortName;
				chosenVersion = spec->version;
				minDiff = diff;
			}
		}
	}
	
	SAIInterfaceSpecifier found = {chosenShortName, chosenVersion};
	return found;
}

std::vector<std::string> split(const std::string& str, const char sep) {
	
/*
	std::vector<std::string> parts;
	
	std::istringstream s(str);
	std::string temp;

	while (std::getline(s, temp, sep)) {
		parts.push_back(temp);
	}
	
	return parts;
*/
	std::vector<std::string> tokens;
	std::string delimitters = ".";
	
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimitters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimitters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(sep, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimitters, lastPos);
    }
	
	return tokens;
}
int CAILibraryManager::versionCompare(
		const std::string& version1,
		const std::string& version2) {/*TODO: test this function!*/
	
	std::vector<std::string> parts1 = split(version1, '.');
	std::vector<std::string> parts2 = split(version2, '.');
	//int partsDiff = parts1.size() - parts2.size();
	unsigned int maxParts = parts1.size() > parts2.size() ? parts1.size() : parts2.size();
	
	int diff = 0;
	int mult = maxParts;
    for (unsigned int i=0; i < maxParts; ++i) {
		const std::string& v1p = i < parts1.size() ? parts1.at(i) : "";
		const std::string& v2p = i < parts2.size() ? parts2.at(i) : "";
		diff += (10^(mult*mult)) * v1p.compare(v2p);
    }

	return diff;
}

