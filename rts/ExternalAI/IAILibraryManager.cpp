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

#include "IAILibraryManager.h"

#include "AILibraryManager.h"
#include "AIInterfaceKey.h"
#include "SkirmishAIKey.h"

#include <iostream>

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
		return fittingKeys[0];
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
