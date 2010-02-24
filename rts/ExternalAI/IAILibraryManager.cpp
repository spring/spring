/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IAILibraryManager.h"

#include "AILibraryManager.h"
#include "AIInterfaceKey.h"
#include "SkirmishAIKey.h"

#include <iostream>
#include <limits.h>
#include <string.h>

IAILibraryManager* IAILibraryManager::myAILibraryManager = NULL;

IAILibraryManager* IAILibraryManager::GetInstance() {

	if (myAILibraryManager == NULL) {
		myAILibraryManager = new CAILibraryManager();
	}

	return myAILibraryManager;
}

std::string fillUpTo(const std::string& str, unsigned int numChars) {

	std::string filler = "";

	filler.append(numChars - str.size(), ' ');

	return filler;
}

void IAILibraryManager::OutputAIInterfacesInfo() {

	const IAILibraryManager* myLibManager = IAILibraryManager::GetInstance();
	const T_interfaceSpecs& keys =
			myLibManager->GetInterfaceKeys();

	std::cout << "#" << std::endl;
	std::cout << "# Available Spring Skirmish AIs" << std::endl;
	std::cout << "# -----------------------------" << std::endl;
	std::cout << "# [Name]              [Version]" << std::endl;

	T_interfaceSpecs::const_iterator key;
	for (key=keys.begin(); key != keys.end(); key++) {
		std::cout << "  ";
		std::cout << key->GetShortName() << fillUpTo(key->GetShortName(), 20);
		std::cout << key->GetVersion() << fillUpTo(key->GetVersion(), 20)
				<< std::endl;
	}

	std::cout << "#" << std::endl;
}

SkirmishAIKey IAILibraryManager::ResolveSkirmishAIKey(
		const SkirmishAIKey& skirmishAIKey) const {

	std::vector<SkirmishAIKey> fittingKeys
			= FittingSkirmishAIKeys(skirmishAIKey);
	if (fittingKeys.size() > 0) {
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

	std::cout << "#" << std::endl;
	std::cout << "# Available Spring Skirmish AIs" << std::endl;
	std::cout << "# -----------------------------" << std::endl;
	std::cout << "# [Name]              [Version]           "
			"[Interface-name]    [Interface-version]" << std::endl;

	T_skirmishAIKeys::const_iterator key;
	for (key=keys.begin(); key != keys.end(); key++) {
		std::cout << "  ";
		std::cout << key->GetShortName() << fillUpTo(key->GetShortName(), 20);
		std::cout << key->GetVersion() << fillUpTo(key->GetVersion(), 20);
		std::cout << key->GetInterface().GetShortName()
				<< fillUpTo(key->GetInterface().GetShortName(), 20);
		std::cout << key->GetInterface().GetVersion() << std::endl;
	}

	const T_dupSkirm& duplicateSkirmishAIInfos =
			myLibManager->GetDuplicateSkirmishAIInfos();
	for (T_dupSkirm::const_iterator info = duplicateSkirmishAIInfos.begin();
			info != duplicateSkirmishAIInfos.end(); ++info) {
		std::cout << "# WARNING: Duplicate Skirmish AI Info found:"
				<< std::endl;
		std::cout << "# \tfor Skirmish AI: " << info->first.GetShortName()
				<< " " << info->first.GetVersion() << std::endl;
		std::cout << "# \tin files:" << std::endl;
		std::set<std::string>::const_iterator dir;
		for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
			std::cout << "# \t" << dir->c_str() << std::endl;
		}
	}

	std::cout << "#" << std::endl;
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
int IAILibraryManager::VersionCompare(
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
