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
#include "SkirmishAIData.h"

#include "Util.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Game/GameSetup.h"

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


static std::string noSlashAtEnd(const std::string& dir) {

	std::string resDir = dir;

	char lastChar = dir.at(dir.size()-1);
	if (lastChar == '/' || lastChar == '\\') {
		resDir = resDir.substr(0, dir.size()-1);
	}

	return resDir;
}

CAILibraryManager::CAILibraryManager() : usedSkirmishAIInfos_initialized(false) {

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
		if (infoFile.size() > 0) { // interface info is available

			// generate and store the interface info
			CAIInterfaceLibraryInfo* interfaceInfo =
					new CAIInterfaceLibraryInfo(infoFile.at(0));

			interfaceInfo->SetDataDir(noSlashAtEnd(possibleDataDir));
			interfaceInfo->SetDataDirCommon(
					std::string(possibleDataDir) + "common");

			AIInterfaceKey interfaceKey = interfaceInfo->GetKey();

			interfaceKeys.insert(interfaceKey);
			interfaceInfos[interfaceKey] = interfaceInfo;

			// so we can check if one interface is specified multiple times
			duplicateInterfaceInfoCheck[interfaceKey].insert(infoFile.at(0));
		}
	}

	// filter out interfaces that are specified multiple times
	for (T_dupInt::const_iterator info = duplicateInterfaceInfoCheck.begin();
			info != duplicateInterfaceInfoCheck.end(); ++info) {
		if (info->second.size() >= 2) {
			duplicateInterfaceInfos[info->first] = info->second;

			logOutput.Print("WARNING: Duplicate AI Interface Info found:");
			logOutput.Print("\tfor interface: %s %s", info->first.GetShortName().c_str(),
					info->first.GetVersion().c_str());
			logOutput.Print("\tin files:");
			const std::string* lastDir = NULL;
			std::set<std::string>::const_iterator dir;
			for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
				logOutput.Print("\t%s", dir->c_str());
				lastDir = &(*dir);
			}
			logOutput.Print("\tusing: %s", lastDir->c_str());
		}
	}

	// Read from Skirmish AI info and option files
	// we are looking for:
	// {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIInfo.lua
	T_dirs skirmishAIDataDirs = filesystem.FindDirsInDirectSubDirs(SKIRMISH_AI_DATA_DIR);
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

			skirmishAIInfo->SetDataDir(noSlashAtEnd(possibleDataDir));
			skirmishAIInfo->SetDataDirCommon(
					std::string(possibleDataDir) + "common");

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
			logOutput.Print("\tfor Skirmish AI: %s %s", info->first.GetShortName().c_str(),
					info->first.GetVersion().c_str());
			logOutput.Print("\tin files:");
			const std::string* lastDir = NULL;
			std::set<std::string>::const_iterator dir;
			for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
				logOutput.Print("\t%s", dir->c_str());
				lastDir = &(*dir);
			}
			logOutput.Print("\tusing: %s", lastDir->c_str());
		}
	}
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

	interfaceInfos.clear();
	skirmishAIInfos.clear();

	interfaceKeys.clear();
	skirmishAIKeys.clear();
}

const std::vector<std::string> CAILibraryManager::EMPTY_OPTION_VALUE_KEYS;
const std::vector<std::string>& CAILibraryManager::GetSkirmishAIOptionValueKeys(int teamId) const {

	const SkirmishAIData* aiData = gameSetup->GetSkirmishAIDataForTeam(teamId);
	if (aiData != NULL) {
		return aiData->optionKeys;
	} else {
		return EMPTY_OPTION_VALUE_KEYS;
	}
}
const std::map<std::string, std::string> CAILibraryManager::EMPTY_OPTION_VALUES;
const std::map<std::string, std::string>& CAILibraryManager::GetSkirmishAIOptionValues(int teamId) const {

	const SkirmishAIData* aiData = gameSetup->GetSkirmishAIDataForTeam(teamId);
	if (aiData != NULL) {
		return aiData->options;
	} else {
		return EMPTY_OPTION_VALUES;
	}
}

CAILibraryManager::~CAILibraryManager() {

	ReleaseEverything();
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

const IAILibraryManager::T_skirmishAIInfos& CAILibraryManager::GetUsedSkirmishAIInfos() {

	if (!usedSkirmishAIInfos_initialized) {
		const CTeam* team = NULL;
		for (unsigned int t = 0; t < (unsigned int)teamHandler->ActiveTeams(); ++t) {
			team = teamHandler->Team(t);
			if (team != NULL && team->isAI) {
				const std::string& t_sn = team->skirmishAIKey.GetShortName();
				const std::string& t_v = team->skirmishAIKey.GetVersion();

				IAILibraryManager::T_skirmishAIInfos::const_iterator aiInfo;
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

	return usedSkirmishAIInfos;
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
	for (sasi=skirmishAIKeys.begin(); sasi!=skirmishAIKeys.end(); sasi++) {

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

const ISkirmishAILibrary* CAILibraryManager::FetchSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey) {

	T_skirmishAIInfos::const_iterator aiInfo = skirmishAIInfos.find(skirmishAIKey);
	if (aiInfo == skirmishAIInfos.end()) {
		logOutput.Print("Aborting the game, unknown skirmish AI specified: %s %s", skirmishAIKey.GetShortName().c_str(), skirmishAIKey.GetVersion().c_str());
		return NULL;
	}
	return FetchInterface(skirmishAIKey.GetInterface())->FetchSkirmishAILibrary(*(aiInfo->second));
}

void CAILibraryManager::ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey) {
	FetchInterface(skirmishAIKey.GetInterface())->ReleaseSkirmishAILibrary(skirmishAIKey);
	ReleaseInterface(skirmishAIKey.GetInterface()); // only releases the library if its load count is 0
}

void CAILibraryManager::ReleaseAllSkirmishAILibraries() {

	T_loadedInterfaces::const_iterator lil;
	for (lil=loadedAIInterfaceLibraries.begin(); lil!=loadedAIInterfaceLibraries.end(); lil++) {
		FetchInterface(lil->first)->ReleaseAllSkirmishAILibraries();
		ReleaseInterface(lil->first); // only releases the library if its load count is 0
	}
}


IAIInterfaceLibrary* CAILibraryManager::FetchInterface(const AIInterfaceKey& interfaceKey) {

	IAIInterfaceLibrary* interfaceLib = NULL;

	T_loadedInterfaces::const_iterator interfacePos = loadedAIInterfaceLibraries.find(interfaceKey);
	if (interfacePos == loadedAIInterfaceLibraries.end()) { // interface not yet loaded
		T_interfaceInfos::const_iterator interfaceInfo = interfaceInfos.find(interfaceKey);
		if (interfaceInfo != interfaceInfos.end()) {
			interfaceLib = new CAIInterfaceLibrary(*(interfaceInfo->second));
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
}


AIInterfaceKey CAILibraryManager::FindFittingInterfaceSpecifier(
		const std::string& shortName,
		const std::string& minVersion,
		const T_interfaceSpecs& keys) {

	std::set<AIInterfaceKey>::const_iterator key;
	int minDiff = INT_MAX;
	AIInterfaceKey fittingKey = AIInterfaceKey(); // unspecified key
	for (key=keys.begin(); key!=keys.end(); key++) {
		if (shortName == key->GetShortName()) {
			int diff = versionCompare(key->GetVersion(), minVersion);
			if (diff >= 0 && diff < minDiff) {
				fittingKey = *key;
				minDiff = diff;
			}
		}
	}

	return fittingKey;
}

static std::vector<std::string> split(const std::string& str, const char sep) {

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
		const std::string& version2) {

	std::vector<std::string> parts1 = split(version1, '.');
	std::vector<std::string> parts2 = split(version2, '.');
	unsigned int maxParts = parts1.size() > parts2.size() ? parts1.size() : parts2.size();

	int diff = 0;
	for (unsigned int i=0; i < maxParts; ++i) {
		const std::string& v1p = i < parts1.size() ? parts1.at(i) : "0";
		const std::string& v2p = i < parts2.size() ? parts2.at(i) : "0";
		diff += (1<<((maxParts-i)*2)) * v1p.compare(v2p);
	}

	// computed the sing of diff -> 1, 0 or -1
	int sign = (diff != 0) | -(int)((unsigned int)((int)diff) >> (sizeof(int) * CHAR_BIT - 1));

	return sign;
}
