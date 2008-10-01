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
#include "StdAfx.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "FileSystem/FileHandler.h"
#include "LogOutput.h"

#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <limits>
//#include <fstream>
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

CAILibraryManager::CAILibraryManager() {
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
		
		SAIInterfaceSpecifyer interfaceSpecifyer = interfaceLib->GetSpecifyer();
		interfaceSpecifyer = copySAIInterfaceSpecifyer(&interfaceSpecifyer);
		interfaceSpecifyers.push_back(interfaceSpecifyer);
		
		// generate and store the interface info
		CAIInterfaceLibraryInfo* interfaceInfo = new CAIInterfaceLibraryInfo(*interfaceLib);
		interfaceInfos[interfaceSpecifyer] = interfaceInfo;
		
		// generate and store the pure file name
		//interfaceFileNames[interfaceSpecifyer] = fileName;
		interfaceInfo->SetFileName(fileName);
		
		// fetch the info of all Skirmish AIs available through the interface
		std::vector<SSAISpecifyer> sass = interfaceLib->GetSkirmishAILibrarySpecifyers();
		std::vector<SSAISpecifyer>::const_iterator sas;
		for (sas=sass.begin(); sas!=sass.end(); sas++) { // AIs
			const ISkirmishAILibrary* skirmishAI = interfaceLib->FetchSkirmishAILibrary(*sas);
			CSkirmishAILibraryInfo* skirmishAIInfo = new CSkirmishAILibraryInfo(*skirmishAI, interfaceSpecifyer);
			SSAIKey skirmishAIKey = {interfaceSpecifyer, copySSAISpecifyer(&(*sas))};
			skirmishAIKeys.push_back(skirmishAIKey);
			skirmishAIInfos[skirmishAIKey] = skirmishAIInfo;
			interfaceLib->ReleaseSkirmishAILibrary(*sas);
		}
		
		// fetch the info of all Group AIs available through the interface
		std::vector<SGAISpecifyer> gass = interfaceLib->GetGroupAILibrarySpecifyers();
		std::vector<SGAISpecifyer>::const_iterator gas;
		for (gas=gass.begin(); gas!=gass.end(); gas++) { // AIs
			const IGroupAILibrary* groupAI = interfaceLib->FetchGroupAILibrary(*gas);
			CGroupAILibraryInfo* groupAIInfo = new CGroupAILibraryInfo(*groupAI, interfaceSpecifyer);
			SGAIKey groupAIKey = {interfaceSpecifyer, copySGAISpecifyer(&(*gas))};
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
	//reportError("NOT YET IMPLEMENTED", "CAILibraryManager::GetAllInfosFromCache() is not yet implemented.");
	
	ClearAllInfos();
	
	// look for AI data files (AIInfo.lua & AIOptions.lua)
	//int GetSkirmishAICount() {
	
/*
	if (!(vfsHandler)) {
		reportError("Initialization Error", Call InitArchiveScanner before GetSkirmishAICount.");
	}
*/
	
	// Read from AI Interface info files
	std::vector<std::string> aiInterfaceDataDirs = CFileHandler::SubDirs(AI_INTERFACES_DATA_DIR, "*", SPRING_VFS_RAW);
	for (std::vector<std::string>::iterator dir = aiInterfaceDataDirs.begin(); dir != aiInterfaceDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		std::vector<std::string> infoFile = CFileHandler::FindFiles(possibleDataDir, "InterfaceInfo.lua");
		if (infoFile.size() > 0) { // interface info is available
			//aiInterfaceDataDirs.erase(dir);
			
			// generate and store the interface info
			CAIInterfaceLibraryInfo* interfaceInfo = new CAIInterfaceLibraryInfo(infoFile.at(0), SPRING_VFS_RAW, SPRING_VFS_RAW);
			
			//std::vector<InfoItem> infos = ParseInfos(infoFile.at(0), SPRING_VFS_RAW, SPRING_VFS_RAW);
			//options = ParseOptions(skirmishAIDataDirs[index] + "/AIOptions.lua", SPRING_VFS_RAW, SPRING_VFS_RAW);
			
			std::string sn = interfaceInfo->GetShortName();
			std::string v = interfaceInfo->GetVersion();
			SAIInterfaceSpecifyer interfaceSpecifyer = {sn.c_str(), v.c_str()};
			interfaceSpecifyer = copySAIInterfaceSpecifyer(&interfaceSpecifyer);
			//interfaceSpecifyer = copySAIInterfaceSpecifyer(&interfaceSpecifyer);
			interfaceSpecifyers.push_back(interfaceSpecifyer);
/*

			// generate and store the pure file name
			std::string fileName = std::string(extractFileName(*libFile, false));
			interfaceFileNames[interfaceSpecifyer] = fileName;
*/
			interfaceInfos[interfaceSpecifyer] = interfaceInfo;
		}
	}
	
	
	
	// Read from Skirmish AI info and option files
	std::vector<std::string> skirmishAIDataDirs = CFileHandler::SubDirs(SKIRMISH_AI_DATA_DIR, "*", SPRING_VFS_RAW);
	for (std::vector<std::string>::iterator dir = skirmishAIDataDirs.begin(); dir != skirmishAIDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		std::vector<std::string> infoFile = CFileHandler::FindFiles(possibleDataDir, "AIInfo.lua");
		if (infoFile.size() > 0) { // skirmish AI info is available
			std::string optionFileName = "";
			std::vector<std::string> optionFile = CFileHandler::FindFiles(possibleDataDir, "AIOptions.lua");
			if (optionFile.size() > 0) {
				optionFileName = optionFile.at(0);
			}
			// generate and store the ai info
			CSkirmishAILibraryInfo* skirmishAIInfo = new CSkirmishAILibraryInfo(infoFile.at(0), optionFileName, SPRING_VFS_RAW, SPRING_VFS_RAW);
			
			std::string sn = skirmishAIInfo->GetShortName();
			std::string v = skirmishAIInfo->GetVersion();
			SSAISpecifyer skirmishAISpecifyer = {sn.c_str(), v.c_str()};
			SAIInterfaceSpecifyer interfaceSpecifyer = findFittingInterfaceSpecifyer(skirmishAIInfo->GetInterfaceShortName(), skirmishAIInfo->GetInterfaceVersion(), interfaceSpecifyers);
			if (interfaceSpecifyer.shortName != NULL) {
				skirmishAISpecifyer = copySSAISpecifyer(&skirmishAISpecifyer);
				SSAIKey skirmishAIKey = {interfaceSpecifyer, skirmishAISpecifyer};
				skirmishAIKeys.push_back(skirmishAIKey);

				skirmishAIInfos[skirmishAIKey] = skirmishAIInfo;
			}
		}
	}
	
	
	// Read from Group AI info and option files
	std::vector<std::string> groupAIDataDirs = CFileHandler::SubDirs(GROUP_AI_DATA_DIR, "*", SPRING_VFS_RAW);
	for (std::vector<std::string>::iterator dir = groupAIDataDirs.begin(); dir != groupAIDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		std::vector<std::string> infoFile = CFileHandler::FindFiles(possibleDataDir, "AIInfo.lua");
		if (infoFile.size() > 0) { // group AI info is available
			std::string optionFileName = "";
			std::vector<std::string> optionFile = CFileHandler::FindFiles(possibleDataDir, "AIOptions.lua");
			if (optionFile.size() > 0) {
				optionFileName = optionFile.at(0);
			}
			// generate and store the ai info
			CGroupAILibraryInfo* groupAIInfo = new CGroupAILibraryInfo(infoFile.at(0), optionFileName, SPRING_VFS_RAW, SPRING_VFS_RAW);
			
			std::string sn = groupAIInfo->GetShortName();
			std::string v = groupAIInfo->GetVersion();
			SGAISpecifyer groupAISpecifyer = {sn.c_str(), v.c_str()};
			SAIInterfaceSpecifyer interfaceSpecifyer = findFittingInterfaceSpecifyer(groupAIInfo->GetInterfaceShortName(), groupAIInfo->GetInterfaceVersion(), interfaceSpecifyers);
			if (interfaceSpecifyer.shortName != NULL) {
				groupAISpecifyer = copySGAISpecifyer(&groupAISpecifyer);
				SGAIKey groupAIKey = {interfaceSpecifyer, groupAISpecifyer};
				groupAIKeys.push_back(groupAIKey);

				groupAIInfos[groupAIKey] = groupAIInfo;
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
		
		SAIInterfaceSpecifyer interfaceSpecifyer = interfaceLib->GetSpecifyer();
		interfaceSpecifyer = copySAIInterfaceSpecifyer(&interfaceSpecifyer);
		interfaceSpecifyers.push_back(interfaceSpecifyer);
		
		// generate and store the interface info
		CAIInterfaceLibraryInfo* interfaceInfo = new CAIInterfaceLibraryInfo(*interfaceLib);
		interfaceInfos[interfaceSpecifyer] = interfaceInfo;
		
		// generate and store the pure file name
		interfaceFileNames[interfaceSpecifyer] = fileName;
		
		// fetch the info of all Skirmish AIs available through the interface
		std::vector<SSAISpecifyer> sass = interfaceLib->GetSkirmishAILibrarySpecifyers();
		std::vector<SSAISpecifyer>::const_iterator sas;
		for (sas=sass.begin(); sas!=sass.end(); sas++) { // AIs
			const ISkirmishAILibrary* skirmishAI = interfaceLib->FetchSkirmishAILibrary(*sas);
			CSkirmishAILibraryInfo* skirmishAIInfo = new CSkirmishAILibraryInfo(*skirmishAI, interfaceSpecifyer);
			SSAIKey skirmishAIKey = {interfaceSpecifyer, copySSAISpecifyer(&(*sas))};
			skirmishAIKeys.push_back(skirmishAIKey);
			skirmishAIInfos[skirmishAIKey] = skirmishAIInfo;
			interfaceLib->ReleaseSkirmishAILibrary(*sas);
		}
		
		// fetch the info of all Group AIs available through the interface
		std::vector<SGAISpecifyer> gass = interfaceLib->GetGroupAILibrarySpecifyers();
		std::vector<SGAISpecifyer>::const_iterator gas;
		for (gas=gass.begin(); gas!=gass.end(); gas++) { // AIs
			const IGroupAILibrary* groupAI = interfaceLib->FetchGroupAILibrary(*gas);
			CGroupAILibraryInfo* groupAIInfo = new CGroupAILibraryInfo(*groupAI, interfaceSpecifyer);
			SGAIKey groupAIKey = {interfaceSpecifyer, copySGAISpecifyer(&(*gas))};
			groupAIKeys.push_back(groupAIKey);
			groupAIInfos[groupAIKey] = groupAIInfo;
			interfaceLib->ReleaseGroupAILibrary(*gas);
		}
		
		delete interfaceLib;
    }
*/
}
void CAILibraryManager::ClearAllInfos() {
	
	std::map<const SAIInterfaceSpecifyer, CAIInterfaceLibraryInfo*>::iterator iii;
	for (iii=interfaceInfos.begin(); iii!=interfaceInfos.end(); iii++) {
		delete iii->second;
		iii->second = NULL;
	}
	
	std::map<const SSAIKey, CSkirmishAILibraryInfo*>::iterator sai;
	for (sai=skirmishAIInfos.begin(); sai!=skirmishAIInfos.end(); sai++) {
		delete sai->second;
		sai->second = NULL;
	}
	
	std::map<const SGAIKey, CGroupAILibraryInfo*>::iterator gai;
	for (gai=groupAIInfos.begin(); gai!=groupAIInfos.end(); gai++) {
		delete gai->second;
		gai->second = NULL;
	}
	
	interfaceInfos.clear();
	skirmishAIInfos.clear();
	groupAIInfos.clear();
	
	interfaceSpecifyers.clear();
	skirmishAIKeys.clear();
	groupAIKeys.clear();
}

CAILibraryManager::~CAILibraryManager() {
	
	ReleaseEverything();

	// delete all strings contained in skirmish AI specifyers
	std::vector<SSAIKey>::iterator sSpec;
	for (sSpec=skirmishAIKeys.begin(); sSpec!=skirmishAIKeys.end(); sSpec++) {
		deleteSSAISpecifyer(&(sSpec->ai));
	}
	
	// delete all strings contained in group AI specifyers
	std::vector<SGAIKey>::iterator gSpec;
	for (gSpec=groupAIKeys.begin(); gSpec!=groupAIKeys.end(); gSpec++) {
		deleteSGAISpecifyer(&(gSpec->ai));
	}
	
	// delete all strings contained in interface specifyers
	std::vector<SAIInterfaceSpecifyer>::iterator iSpec;
	for (iSpec=interfaceSpecifyers.begin(); iSpec!=interfaceSpecifyers.end(); iSpec++) {
		deleteSAIInterfaceSpecifyer(&(*iSpec));
	}
}

const std::vector<SAIInterfaceSpecifyer>* CAILibraryManager::GetInterfaceSpecifyers() const {
	return &interfaceSpecifyers;
}
const std::vector<SSAIKey>* CAILibraryManager::GetSkirmishAIKeys() const {
	return &skirmishAIKeys;
}
const std::vector<SGAIKey>* CAILibraryManager::GetGroupAIKeys() const {
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



std::vector<SSAIKey> CAILibraryManager::ResolveSkirmishAIKey(const SSAISpecifyer& skirmishAISpecifyer) const {
	
	std::vector<SSAIKey> applyingKeys;
	
	if (skirmishAISpecifyer.shortName == NULL || strlen(skirmishAISpecifyer.shortName) == 0) {
		return applyingKeys;
	}
	
	std::string aiName(skirmishAISpecifyer.shortName);
	
	bool checkVersion = false;
	std::string aiVersion;
	if (skirmishAISpecifyer.version != NULL && strlen(skirmishAISpecifyer.version) > 0) {
		aiVersion = std::string(skirmishAISpecifyer.version);
		checkVersion = true;
	}
	
	std::vector<SSAIKey>::const_iterator sasi;
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
std::vector<SSAIKey> CAILibraryManager::ResolveSkirmishAIKey(const std::string& skirmishAISpecifyer) const {
	
	std::vector<SSAIKey> applyingKeys;
	
	std::string* aiName;
	std::string* aiVersion;
	std::string* interfaceName;
	std::string* interfaceVersion;
	bool isValid = SplittAIKey(skirmishAISpecifyer, aiName, aiVersion, interfaceName, interfaceVersion);
	if (!isValid) {
		reportError1("AI Library Error", "Invalid Skirmish AI Key: %s", skirmishAISpecifyer.c_str());
		return applyingKeys;
	}
	
	std::vector<SSAIKey>::const_iterator sasi;
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
	return FetchInterface(skirmishAIKey.interface)->FetchSkirmishAILibrary(skirmishAIKey.ai);
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


std::vector<SGAIKey> CAILibraryManager::ResolveGroupAIKey(const std::string& groupAISpecifyer) const {
	
	std::vector<SGAIKey> applyingKeys;
	
	std::string* aiName;
	std::string* aiVersion;
	std::string* interfaceName;
	std::string* interfaceVersion;
	bool isValid = SplittAIKey(groupAISpecifyer, aiName, aiVersion, interfaceName, interfaceVersion);
	if (!isValid) {
		reportError1("AI Library Error", "Invalid Group AI Key: %s", groupAISpecifyer.c_str());
		return applyingKeys;
	}
	
	std::vector<SGAIKey>::const_iterator gasi;
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
	return FetchInterface(groupAIKey.interface)->FetchGroupAILibrary(groupAIKey.ai);
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




IAIInterfaceLibrary* CAILibraryManager::FetchInterface(const SAIInterfaceSpecifyer& interfaceSpecifyer) {
	
	IAIInterfaceLibrary* interfaceLib;
	
	T_loadedInterfaces::const_iterator interfacePos = loadedAIInterfaceLibraries.find(interfaceSpecifyer);
	if (interfacePos == loadedAIInterfaceLibraries.end()) { // interface not yet loaded
		interfaceLib = new CAIInterfaceLibrary(interfaceSpecifyer);
		loadedAIInterfaceLibraries[interfaceSpecifyer] = interfaceLib;
	} else {
		interfaceLib = interfacePos->second;
	}

	return interfaceLib;
}

void CAILibraryManager::ReleaseInterface(const SAIInterfaceSpecifyer& interfaceSpecifyer) {
	
	T_loadedInterfaces::iterator interfacePos = loadedAIInterfaceLibraries.find(interfaceSpecifyer);
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

SAIInterfaceSpecifyer CAILibraryManager::findFittingInterfaceSpecifyer(
		const std::string& shortName,
		const std::string& minVersion,
		const std::vector<SAIInterfaceSpecifyer>& specs) {
	
	std::vector<SAIInterfaceSpecifyer>::const_iterator spec;
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
	
	SAIInterfaceSpecifyer found = {chosenShortName, chosenVersion};
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

