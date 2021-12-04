/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IAILibraryManager.h"

#include "AILibraryManager.h"
#include "AIInterfaceKey.h"
#include "SkirmishAIKey.h"
#include "System/FileSystem/SimpleParser.h" // for Split()

#include <climits>
#include <cstdio>
#include <cstring>

IAILibraryManager* IAILibraryManager::gAILibraryManager = NULL;

IAILibraryManager* IAILibraryManager::GetInstance() {
	if (gAILibraryManager == NULL)
		gAILibraryManager = new CAILibraryManager();

	return gAILibraryManager;
}

void IAILibraryManager::Destroy() {
	// spring::SafeDelete
	IAILibraryManager* tmp = gAILibraryManager;
	gAILibraryManager = NULL;
	delete tmp;
}

void IAILibraryManager::OutputAIInterfacesInfo() {

	const IAILibraryManager* myLibManager = IAILibraryManager::GetInstance();
	const T_interfaceSpecs& keys =
			myLibManager->GetInterfaceKeys();

	printf("#\n");
	printf("# Available Spring Skirmish AIs\n");
	printf("# -----------------------------\n");
	printf("# %-20s %s\n", "[Name]", "[Version]");

	T_interfaceSpecs::const_iterator key;
	for (key = keys.begin(); key != keys.end(); ++key) {
		printf("  %-20s %s\n",
				key->GetShortName().c_str(), key->GetVersion().c_str());
	}

	printf("#\n");
}

SkirmishAIKey IAILibraryManager::ResolveSkirmishAIKey(
		const SkirmishAIKey& skirmishAIKey) const {

	std::vector<SkirmishAIKey> fittingKeys
			= FittingSkirmishAIKeys(skirmishAIKey);
	if (!fittingKeys.empty()) {
		// look for the one with the highest version number,
		// in case there are multiple fitting ones.
		size_t bestIndex = 0;
		const std::string* bestVersion = &(fittingKeys[0].GetVersion());
		for (size_t k = 1; k < fittingKeys.size(); ++k) {
			if (IAILibraryManager::VersionCompare(fittingKeys[k].GetVersion(), *bestVersion) > 0) {
				bestIndex = k;
				bestVersion = &(fittingKeys[k].GetVersion());
			}
		}
		bestVersion = NULL;
		return fittingKeys[bestIndex];
	} else {
		return SkirmishAIKey();
	}
}

void IAILibraryManager::OutputSkirmishAIInfo() {

	const IAILibraryManager* myLibManager = IAILibraryManager::GetInstance();
	const T_skirmishAIKeys& keys = myLibManager->GetSkirmishAIKeys();

	printf("#\n");
	printf("# Available Spring Skirmish AIs\n");
	printf("# -----------------------------\n");
	printf("# %-20s %-20s %-20s %s\n",
			"[Name]", "[Version]", "[Interface-name]", "[Interface-version]");

	T_skirmishAIKeys::const_iterator key;
	for (key = keys.begin(); key != keys.end(); ++key) {
		printf("# %-20s %-20s %-20s %s\n",
				key->GetShortName().c_str(),
				key->GetVersion().c_str(),
				key->GetInterface().GetShortName().c_str(),
				key->GetInterface().GetVersion().c_str());
	}

	const T_dupSkirm& duplicateSkirmishAIInfos =
			myLibManager->GetDuplicateSkirmishAIInfos();
	for (T_dupSkirm::const_iterator info = duplicateSkirmishAIInfos.begin();
			info != duplicateSkirmishAIInfos.end(); ++info) {
		printf("# WARNING: Duplicate Skirmish AI Info found:\n");
		printf("# \tfor Skirmish AI: %s %s\n",
				info->first.GetShortName().c_str(),
				info->first.GetVersion().c_str());
		printf("# \tin files:\n");
		std::set<std::string>::const_iterator dir;
		for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
			printf("# \t%s\n", dir->c_str());
		}
	}

	printf("#\n");
}

int IAILibraryManager::VersionCompare(
		const std::string& version1,
		const std::string& version2)
{
	const std::vector<std::string>& parts1 = CSimpleParser::Split(version1, ".");
	const std::vector<std::string>& parts2 = CSimpleParser::Split(version2, ".");
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
