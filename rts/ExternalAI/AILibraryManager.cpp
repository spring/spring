/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AILibraryManager.h"

#include "Interface/aidefines.h"
#include "Interface/SAIInterfaceLibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "AIInterfaceLibrary.h"
#include "SkirmishAILibraryInfo.h"
#include "SkirmishAIData.h"

#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/SharedLib.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileSystemHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Game/GameSetup.h"

#include <string>
#include <set>
#include <sstream>
#include <limits.h>


CAILibraryManager::CAILibraryManager() {

	GetAllInfosFromCache();
}

void CAILibraryManager::GetAllInfosFromCache() {

	ClearAllInfos();

	typedef std::vector<std::string> T_dirs;

	// cause we use CFileHandler for searching files,
	// we are automatically searching in all data-dirs

	// Read from AI Interface info files
	// we are looking for:
	// {AI_INTERFACES_DATA_DIR}/{*}/{*}/InterfaceInfo.lua
	T_dirs aiInterfaceDataDirs =
			filesystem.FindDirsInDirectSubDirs(AI_INTERFACES_DATA_DIR);
	typedef std::map<const AIInterfaceKey, std::set<std::string> > T_dupInt;
	T_dupInt duplicateInterfaceInfoCheck;
	for (T_dirs::iterator dir = aiInterfaceDataDirs.begin();
			dir != aiInterfaceDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		T_dirs infoFile =
				CFileHandler::FindFiles(possibleDataDir, "InterfaceInfo.lua");
		if (!infoFile.empty()) { // interface info is available

			// generate and store the interface info
			CAIInterfaceLibraryInfo* interfaceInfo =
					new CAIInterfaceLibraryInfo(infoFile.at(0));

			interfaceInfo->SetDataDir(FileSystemHandler::EnsureNoPathSepAtEnd(possibleDataDir));
			interfaceInfo->SetDataDirCommon(
					FileSystemHandler::GetParent(possibleDataDir) + "common");

			AIInterfaceKey interfaceKey = interfaceInfo->GetKey();

			interfaceKeys.insert(interfaceKey);
			interfaceInfos[interfaceKey] = interfaceInfo;

			// for debug-info, in case one interface is specified multiple times
			duplicateInterfaceInfoCheck[interfaceKey].insert(infoFile.at(0));
		}
	}

	// filter out interfaces that are specified multiple times
	for (T_dupInt::const_iterator info = duplicateInterfaceInfoCheck.begin();
			info != duplicateInterfaceInfoCheck.end(); ++info) {
		if (info->second.size() >= 2) {
			duplicateInterfaceInfos[info->first] = info->second;

			if (LOG_IS_ENABLED(L_WARNING)) {
				LOG_L(L_WARNING, "Duplicate AI Interface Info found:");
				LOG_L(L_WARNING, "\tfor interface: %s %s",
						info->first.GetShortName().c_str(),
						info->first.GetVersion().c_str());
				LOG_L(L_WARNING, "\tin files:");
				const std::string* lastDir = NULL;
				std::set<std::string>::const_iterator dir;
				for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
					LOG_L(L_WARNING, "\t%s", dir->c_str());
					lastDir = &(*dir);
				}
				LOG_L(L_WARNING, "\tusing: %s", lastDir->c_str());
			}
		}
	}

	// Read from Skirmish AI info and option files
	// we are looking for:
	// {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIInfo.lua
	// {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIOptions.lua
	T_dirs skirmishAIDataDirs = filesystem.FindDirsInDirectSubDirs(SKIRMISH_AI_DATA_DIR);
	T_dupSkirm duplicateSkirmishAIInfoCheck;
	for (T_dirs::iterator dir = skirmishAIDataDirs.begin();
			dir != skirmishAIDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		T_dirs infoFile = CFileHandler::FindFiles(possibleDataDir,
				"AIInfo.lua");
		if (!infoFile.empty()) { // skirmish AI info is available
			std::string optionFileName = "";
			T_dirs optionFile = CFileHandler::FindFiles(possibleDataDir,
					"AIOptions.lua");
			if (!optionFile.empty()) {
				optionFileName = optionFile.at(0);
			}
			// generate and store the ai info
			CSkirmishAILibraryInfo* skirmishAIInfo =
					new CSkirmishAILibraryInfo(infoFile.at(0), optionFileName);

			skirmishAIInfo->SetDataDir(FileSystemHandler::EnsureNoPathSepAtEnd(possibleDataDir));
			skirmishAIInfo->SetDataDirCommon(
					FileSystemHandler::GetParent(possibleDataDir) + "common");
			skirmishAIInfo->SetLuaAI(false);

			SkirmishAIKey aiKey = skirmishAIInfo->GetKey();
			AIInterfaceKey interfaceKey =
					FindFittingInterfaceSpecifier(
							skirmishAIInfo->GetInterfaceShortName(),
							skirmishAIInfo->GetInterfaceVersion(),
							interfaceKeys);
			if (!interfaceKey.IsUnspecified()) {
				SkirmishAIKey skirmishAIKey = SkirmishAIKey(aiKey, interfaceKey);
				skirmishAIKeys.insert(skirmishAIKey);
				skirmishAIInfos[skirmishAIKey] = skirmishAIInfo;

				// for debug-info, in case one AI is specified multiple times
				duplicateSkirmishAIInfoCheck[skirmishAIKey].insert(infoFile.at(0));
			} else {
				LOG_L(L_ERROR,
						"Required AI Interface for Skirmish AI %s %s not found.",
						skirmishAIInfo->GetShortName().c_str(),
						skirmishAIInfo->GetVersion().c_str());
			}
		}
	}

	// filter out skirmish AIs that are specified multiple times
	for (T_dupSkirm::const_iterator info = duplicateSkirmishAIInfoCheck.begin();
			info != duplicateSkirmishAIInfoCheck.end(); ++info) {
		if (info->second.size() >= 2) {
			duplicateSkirmishAIInfos[info->first] = info->second;

			if (LOG_IS_ENABLED(L_WARNING)) {
				LOG_L(L_WARNING, "Duplicate Skirmish AI Info found:");
				LOG_L(L_WARNING, "\tfor Skirmish AI: %s %s", info->first.GetShortName().c_str(),
						info->first.GetVersion().c_str());
				LOG_L(L_WARNING, "\tin files:");
				const std::string* lastDir = NULL;
				std::set<std::string>::const_iterator dir;
				for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
					LOG_L(L_WARNING, "\t%s", dir->c_str());
					lastDir = &(*dir);
				}
				LOG_L(L_WARNING, "\tusing: %s", lastDir->c_str());
			}
		}
	}
}

void CAILibraryManager::ClearAllInfos() {

	IAILibraryManager::T_interfaceInfos::iterator iii;
	for (iii = interfaceInfos.begin(); iii != interfaceInfos.end(); ++iii) {
		delete iii->second;
		iii->second = NULL;
	}
	interfaceInfos.clear();

	IAILibraryManager::T_skirmishAIInfos::iterator sai;
	for (sai = skirmishAIInfos.begin(); sai != skirmishAIInfos.end(); ++sai) {
		delete sai->second;
		sai->second = NULL;
	}
	skirmishAIInfos.clear();

	interfaceKeys.clear();
	skirmishAIKeys.clear();
}

CAILibraryManager::~CAILibraryManager() {

	ReleaseEverything();
	ClearAllInfos();
}

const IAILibraryManager::T_interfaceSpecs& CAILibraryManager::GetInterfaceKeys() const {
	return interfaceKeys;
}
const IAILibraryManager::T_skirmishAIKeys& CAILibraryManager::GetSkirmishAIKeys() const {
	return skirmishAIKeys;
}

const IAILibraryManager::T_interfaceInfos& CAILibraryManager::GetInterfaceInfos() const {
	return interfaceInfos;
}
const IAILibraryManager::T_skirmishAIInfos& CAILibraryManager::GetSkirmishAIInfos() const {
	return skirmishAIInfos;
}

const IAILibraryManager::T_dupInt& CAILibraryManager::GetDuplicateInterfaceInfos() const {
	return duplicateInterfaceInfos;
}
const IAILibraryManager::T_dupSkirm& CAILibraryManager::GetDuplicateSkirmishAIInfos() const {
	return duplicateSkirmishAIInfos;
}


std::vector<SkirmishAIKey> CAILibraryManager::FittingSkirmishAIKeys(
		const SkirmishAIKey& skirmishAIKey) const {

	std::vector<SkirmishAIKey> applyingKeys;

	if (skirmishAIKey.IsUnspecified()) {
		return applyingKeys;
	}

	bool checkVersion = false;
	if (skirmishAIKey.GetVersion() != "") {
		checkVersion = true;
	}

	std::set<SkirmishAIKey>::const_iterator sasi;
	for (sasi = skirmishAIKeys.begin(); sasi != skirmishAIKeys.end(); ++sasi) {

		// check if the ai name fits
		if (skirmishAIKey.GetShortName() != sasi->GetShortName()) {
			continue;
		}

		// check if the ai version fits (if one is specified)
		if (checkVersion && skirmishAIKey.GetVersion() != sasi->GetVersion()) {
			continue;
		}

		// if the programm raches here, we know that this key fits
		applyingKeys.push_back(*sasi);
	}

	return applyingKeys;
}



const CSkirmishAILibrary* CAILibraryManager::FetchSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey) {

	const CSkirmishAILibrary* aiLib = NULL;

	T_skirmishAIInfos::const_iterator aiInfo = skirmishAIInfos.find(skirmishAIKey);
	if (aiInfo == skirmishAIInfos.end()) {
		LOG_L(L_ERROR,
				"Unknown skirmish AI specified: %s %s",
				skirmishAIKey.GetShortName().c_str(),
				skirmishAIKey.GetVersion().c_str()
				);
	} else {
		CAIInterfaceLibrary* interfaceLib = FetchInterface(skirmishAIKey.GetInterface());
		if ((interfaceLib != NULL) && interfaceLib->IsInitialized()) {
			aiLib = interfaceLib->FetchSkirmishAILibrary(*(aiInfo->second));
		}
	}

	return aiLib;
}

void CAILibraryManager::ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey) {

	CAIInterfaceLibrary* interfaceLib = FetchInterface(skirmishAIKey.GetInterface());
	if ((interfaceLib != NULL) && interfaceLib->IsInitialized()) {
		interfaceLib->ReleaseSkirmishAILibrary(skirmishAIKey);
		// only releases the library if its load count is 0
		ReleaseInterface(skirmishAIKey.GetInterface());
	} else {
		// Not releasing, because the AI Interface is not initialized,
		// and so neither was the AI.
	}
}


void CAILibraryManager::ReleaseEverything() {
	T_loadedInterfaces::const_iterator lil;

	for (lil = loadedAIInterfaceLibraries.begin(); lil != loadedAIInterfaceLibraries.end(); ++lil) {
		CAIInterfaceLibrary* interfaceLib = FetchInterface(lil->first);
		if ((interfaceLib != NULL) && interfaceLib->IsInitialized()) {
			interfaceLib->ReleaseAllSkirmishAILibraries();
			// only releases the library if its load count is 0
			ReleaseInterface(lil->first);
		}
	}
}



CAIInterfaceLibrary* CAILibraryManager::FetchInterface(const AIInterfaceKey& interfaceKey) {

	CAIInterfaceLibrary* interfaceLib = NULL;

	T_loadedInterfaces::const_iterator interfacePos = loadedAIInterfaceLibraries.find(interfaceKey);
	if (interfacePos == loadedAIInterfaceLibraries.end()) { // interface not yet loaded
		T_interfaceInfos::const_iterator interfaceInfo = interfaceInfos.find(interfaceKey);
		if (interfaceInfo != interfaceInfos.end()) {
			interfaceLib = new CAIInterfaceLibrary(*(interfaceInfo->second));
			if (!interfaceLib->IsInitialized()) {
				delete interfaceLib;
				interfaceLib = NULL;
			}
			// storing this for later use, even if it is NULL (failed to init)
			loadedAIInterfaceLibraries[interfaceKey] = interfaceLib;
		} else {
			// unavailable interface requested, returning NULL
		}
	} else {
		interfaceLib = interfacePos->second;
	}

	return interfaceLib;
}

void CAILibraryManager::ReleaseInterface(const AIInterfaceKey& interfaceKey) {

	T_loadedInterfaces::iterator interfacePos = loadedAIInterfaceLibraries.find(interfaceKey);
	if (interfacePos != loadedAIInterfaceLibraries.end()) {
		CAIInterfaceLibrary* interfaceLib = interfacePos->second;
		if (interfaceLib->GetLoadCount() == 0) {
			loadedAIInterfaceLibraries.erase(interfacePos);
			delete interfaceLib;
			interfaceLib = NULL;
		}
	}
}


AIInterfaceKey CAILibraryManager::FindFittingInterfaceSpecifier(
		const std::string& shortName,
		const std::string& minVersion,
		const IAILibraryManager::T_interfaceSpecs& keys) {

	std::set<AIInterfaceKey>::const_iterator key;
	int minDiff = INT_MAX;
	AIInterfaceKey fittingKey = AIInterfaceKey(); // unspecified key
	for (key = keys.begin(); key != keys.end(); ++key) {
		if (shortName == key->GetShortName()) {
			int diff = IAILibraryManager::VersionCompare(key->GetVersion(), minVersion);
			if (diff >= 0 && diff < minDiff) {
				fittingKey = *key;
				minDiff = diff;
			}
		}
	}

	return fittingKey;
}
