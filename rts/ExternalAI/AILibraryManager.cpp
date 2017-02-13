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
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileSystem.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"

#include <string>
#include <set>
#include <sstream>
#include <limits.h>


CAILibraryManager::CAILibraryManager() {
	ClearAllInfos();
	GatherInterfaceLibrariesInfos();
	GatherSkirmishAIsLibrariesInfos();
}

void CAILibraryManager::GatherInterfaceLibrariesInfos() {
	typedef std::vector<std::string> T_dirs;

	// cause we use CFileHandler for searching files,
	// we are automatically searching in all data-dirs

	// Read from AI Interface info files
	// we are looking for:
	// {AI_INTERFACES_DATA_DIR}/{*}/{*}/InterfaceInfo.lua
	T_dirs aiInterfaceDataDirs = dataDirsAccess.FindDirsInDirectSubDirs(AI_INTERFACES_DATA_DIR);
	typedef std::map<const AIInterfaceKey, std::set<std::string> > T_dupInt;
	T_dupInt duplicateInterfaceInfoCheck;

	for (auto dir = aiInterfaceDataDirs.cbegin(); dir != aiInterfaceDataDirs.cend(); ++dir) {
		const std::string& possibleDataDir = *dir;
		const T_dirs infoFiles = CFileHandler::FindFiles(possibleDataDir, "InterfaceInfo.lua");

		if (infoFiles.empty())
			continue;

		// interface info is available
		const std::string& infoFile = infoFiles.at(0);

		// generate and store the interface info
		CAIInterfaceLibraryInfo interfaceInfo = CAIInterfaceLibraryInfo(infoFile);

		interfaceInfo.SetDataDir(FileSystem::EnsureNoPathSepAtEnd(possibleDataDir));
		interfaceInfo.SetDataDirCommon(FileSystem::GetParent(possibleDataDir) + "common");

		AIInterfaceKey interfaceKey = interfaceInfo.GetKey();

		interfaceKeys.insert(interfaceKey);

		// if no interface info with this key yet, store it
		if (interfaceInfos.find(interfaceKey) == interfaceInfos.end())
			interfaceInfos[interfaceKey] = interfaceInfo;

		// for debug-info, in case one interface is specified multiple times
		duplicateInterfaceInfoCheck[interfaceKey].insert(infoFile);
	}

	// filter out interfaces that are specified multiple times
	for (auto info = duplicateInterfaceInfoCheck.cbegin(); info != duplicateInterfaceInfoCheck.cend(); ++info) {
		if (info->second.size() < 2)
			continue;

		duplicateInterfaceInfos[info->first] = info->second;

		if (LOG_IS_ENABLED(L_ERROR)) {
			LOG_L(L_ERROR, "Duplicate AI Interface Info found:");
			LOG_L(L_ERROR, "\tfor interface: %s %s",
					info->first.GetShortName().c_str(),
					info->first.GetVersion().c_str());
			LOG_L(L_ERROR, "\tin files:");
			const std::string* lastDir = NULL;
			std::set<std::string>::const_iterator dir;
			for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
				LOG_L(L_ERROR, "\t%s", dir->c_str());
				lastDir = &(*dir);
			}
			LOG_L(L_ERROR, "\tusing: %s", lastDir->c_str());
		}
	}
}

void CAILibraryManager::GatherSkirmishAIsLibrariesInfos() {
	T_dupSkirm duplicateSkirmishAIInfoCheck;

	GatherSkirmishAIsLibrariesInfosFromLuaFiles(duplicateSkirmishAIInfoCheck);
	GatherSkirmishAIsLibrariesInfosFromInterfaceLibrary(duplicateSkirmishAIInfoCheck);
	FilterDuplicateSkirmishAILibrariesInfos(duplicateSkirmishAIInfoCheck);
}

void CAILibraryManager::StoreSkirmishAILibraryInfos(
	T_dupSkirm duplicateSkirmishAIInfoCheck,
	CSkirmishAILibraryInfo& skirmishAIInfo,
	const std::string& sourceDesc
) {
	skirmishAIInfo.SetLuaAI(false);

	SkirmishAIKey aiKey = skirmishAIInfo.GetKey();
	AIInterfaceKey interfaceKey = FindFittingInterfaceSpecifier(
		skirmishAIInfo.GetInterfaceShortName(),
		skirmishAIInfo.GetInterfaceVersion(),
		interfaceKeys
	);

	if (!interfaceKey.IsUnspecified()) {
		SkirmishAIKey skirmishAIKey = SkirmishAIKey(aiKey, interfaceKey);
		skirmishAIKeys.insert(skirmishAIKey);

		// if no AI info with this key yet, store it
		if (skirmishAIInfos.find(skirmishAIKey) == skirmishAIInfos.end())
			skirmishAIInfos[skirmishAIKey] = skirmishAIInfo;

		// for debug-info, in case one AI is specified multiple times
		duplicateSkirmishAIInfoCheck[skirmishAIKey].insert(sourceDesc);
	} else {
		LOG_L(L_ERROR,
			"Required AI Interface for Skirmish AI %s %s not found.",
			skirmishAIInfo.GetShortName().c_str(),
			skirmishAIInfo.GetVersion().c_str());
	}
}

void CAILibraryManager::GatherSkirmishAIsLibrariesInfosFromLuaFiles(T_dupSkirm duplicateSkirmishAIInfoCheck) {
	typedef std::vector<std::string> T_dirs;

	// Read from Skirmish AI info and option files
	// we are looking for:
	// {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIInfo.lua
	// {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIOptions.lua
	T_dirs skirmishAIDataDirs = dataDirsAccess.FindDirsInDirectSubDirs(SKIRMISH_AI_DATA_DIR);

	for (auto dir = skirmishAIDataDirs.cbegin(); dir != skirmishAIDataDirs.cend(); ++dir) {
		const std::string& possibleDataDir = *dir;
		const T_dirs infoFiles = CFileHandler::FindFiles(possibleDataDir, "AIInfo.lua");

		if (infoFiles.empty())
			continue;

		// skirmish AI info is available
		const std::string& infoFile = infoFiles.at(0);

		std::string optionFileName = "";
		const T_dirs optionFile = CFileHandler::FindFiles(possibleDataDir, "AIOptions.lua");

		if (!optionFile.empty())
			optionFileName = optionFile.at(0);

		// generate and store the ai info
		CSkirmishAILibraryInfo skirmishAIInfo = CSkirmishAILibraryInfo(infoFile, optionFileName);

		skirmishAIInfo.SetDataDir(FileSystem::EnsureNoPathSepAtEnd(possibleDataDir));
		skirmishAIInfo.SetDataDirCommon(FileSystem::GetParent(possibleDataDir) + "common");

		StoreSkirmishAILibraryInfos(duplicateSkirmishAIInfoCheck, skirmishAIInfo, infoFile);
	}
}

void CAILibraryManager::GatherSkirmishAIsLibrariesInfosFromInterfaceLibrary(T_dupSkirm duplicateSkirmishAIInfoCheck) {
	const T_interfaceInfos& intInfs = GetInterfaceInfos();

	for (auto intInfIt = intInfs.begin(); intInfIt != intInfs.end(); ++intInfIt) {
		// only try to lookup Skirmish AI infos through the Interface library
		// if it explicitly states support for this in InterfaceInfo.lua
		if (!(intInfIt->second).IsLookupSupported())
			continue;

		const CAIInterfaceLibrary* intLib = FetchInterface((intInfIt->second).GetKey());
		const int aiCount = intLib->GetSkirmishAICount();

		for (int aii = 0; aii < aiCount; ++aii) {
			const std::map<std::string, std::string>& rawInfos = intLib->GetSkirmishAIInfos(aii);
			const std::string& rawLuaOptions = intLib->GetSkirmishAIOptions(aii);

			// generate and store the ai info
			//
			// NOTE We do not set the data-dir(s) for interface looked-up
			//   AIs. This is the duty of the AI Interface plugin.
			CSkirmishAILibraryInfo skirmishAIInfo = CSkirmishAILibraryInfo(rawInfos, rawLuaOptions);

			StoreSkirmishAILibraryInfos(duplicateSkirmishAIInfoCheck, skirmishAIInfo, intInfIt->first.ToString());
		}
	}
}

void CAILibraryManager::FilterDuplicateSkirmishAILibrariesInfos(T_dupSkirm duplicateSkirmishAIInfoCheck) {

	// filter out skirmish AIs that are specified multiple times
	for (auto info = duplicateSkirmishAIInfoCheck.cbegin(); info != duplicateSkirmishAIInfoCheck.cend(); ++info) {
		if (info->second.size() < 2)
			continue;

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

void CAILibraryManager::ClearAllInfos() {
	interfaceInfos.clear();
	skirmishAIInfos.clear();

	interfaceKeys.clear();
	skirmishAIKeys.clear();

	duplicateInterfaceInfos.clear();
	duplicateSkirmishAIInfos.clear();
}

CAILibraryManager::~CAILibraryManager() {

	ReleaseEverything();
	ClearAllInfos();
}



std::vector<SkirmishAIKey> CAILibraryManager::FittingSkirmishAIKeys(const SkirmishAIKey& skirmishAIKey) const {
	std::vector<SkirmishAIKey> applyingKeys;

	if (skirmishAIKey.IsUnspecified())
		return applyingKeys;

	const bool checkVersion = (skirmishAIKey.GetVersion() != "");

	for (auto sasi = skirmishAIKeys.cbegin(); sasi != skirmishAIKeys.cend(); ++sasi) {
		// check if the ai name fits
		if (skirmishAIKey.GetShortName() != sasi->GetShortName())
			continue;

		// check if the ai version fits (if one is specified)
		if (checkVersion && skirmishAIKey.GetVersion() != sasi->GetVersion())
			continue;

		// if the programm raches here, we know that this key fits
		applyingKeys.push_back(*sasi);
	}

	return applyingKeys;
}



const CSkirmishAILibrary* CAILibraryManager::FetchSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey) {
	const CSkirmishAILibrary* aiLib = nullptr;
	const auto aiInfo = skirmishAIInfos.find(skirmishAIKey);

	if (aiInfo == skirmishAIInfos.end()) {
		LOG_L(L_ERROR,
			"Unknown skirmish AI specified: %s %s",
			skirmishAIKey.GetShortName().c_str(),
			skirmishAIKey.GetVersion().c_str()
		);
	} else {
		CAIInterfaceLibrary* interfaceLib = FetchInterface(skirmishAIKey.GetInterface());

		if ((interfaceLib != nullptr) && interfaceLib->IsInitialized()) {
			aiLib = interfaceLib->FetchSkirmishAILibrary(aiInfo->second);
		}
	}

	return aiLib;
}

void CAILibraryManager::ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey) {
	CAIInterfaceLibrary* interfaceLib = FetchInterface(skirmishAIKey.GetInterface());

	if ((interfaceLib != nullptr) && interfaceLib->IsInitialized()) {
		interfaceLib->ReleaseSkirmishAILibrary(skirmishAIKey);
		// only releases the library if its load count is 0
		ReleaseInterface(skirmishAIKey.GetInterface());
	} else {
		// Not releasing, because the AI Interface is not initialized,
		// and so neither was the AI.
	}
}


void CAILibraryManager::ReleaseEverything() {
	for (auto lil = loadedAIInterfaceLibraries.cbegin(); lil != loadedAIInterfaceLibraries.cend(); ++lil) {
		CAIInterfaceLibrary* interfaceLib = FetchInterface(lil->first);

		if ((interfaceLib == nullptr) || !interfaceLib->IsInitialized())
			continue;

		interfaceLib->ReleaseAllSkirmishAILibraries();
		// only releases the library if its load count is 0
		ReleaseInterface(lil->first);
	}
}



CAIInterfaceLibrary* CAILibraryManager::FetchInterface(const AIInterfaceKey& interfaceKey) {
	const auto interfacePos = loadedAIInterfaceLibraries.find(interfaceKey);

	if (interfacePos != loadedAIInterfaceLibraries.end())
		return (interfacePos->second).get();

	// interface not yet loaded
	const auto interfaceInfo = interfaceInfos.find(interfaceKey);

	if (interfaceInfo == interfaceInfos.end())
		return nullptr;

	// storing this for later use, even if it failed to init
	std::unique_ptr<CAIInterfaceLibrary>& ptr = loadedAIInterfaceLibraries[interfaceKey];

	ptr.reset(new CAIInterfaceLibrary(interfaceInfo->second));

	if (!(ptr.get())->IsInitialized())
		ptr.reset();

	return (ptr.get());
}

void CAILibraryManager::ReleaseInterface(const AIInterfaceKey& interfaceKey) {
	const auto interfacePos = loadedAIInterfaceLibraries.find(interfaceKey);

	if (interfacePos == loadedAIInterfaceLibraries.end())
		return;

	CAIInterfaceLibrary* interfaceLib = (interfacePos->second).get();

	if (interfaceLib->GetLoadCount() == 0) {
		loadedAIInterfaceLibraries.erase(interfacePos);
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
