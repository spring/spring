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


#include "StdAfx.h"
#include "AILibraryManager.h"

#include <iostream>
#include <c++/4.2/bits/basic_string.h>

IAILibraryManager* IAILibraryManager::myAILibraryManager = NULL;

IAILibraryManager* IAILibraryManager::GetInstance() {

	if (myAILibraryManager == NULL) {
		myAILibraryManager = SAFE_NEW CAILibraryManager();
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
	const T_interfaceSpecs* specs =
			myLibManager->GetInterfaceSpecifiers();
	
	std::cout << "#" << std::endl;
	std::cout << "# Available Spring Skirmish AIs" << std::endl;
	std::cout << "# -----------------------------" << std::endl;
	std::cout << "# [Name]              [Version]" << std::endl;
	
	T_interfaceSpecs::const_iterator spec;
    for (spec=specs->begin(); spec != specs->end(); spec++) {
		std::cout << "  ";
		std::cout << spec->shortName << fillUpTo(spec->shortName, 20);
		std::cout << spec->version << fillUpTo(spec->version, 20) << std::endl;
    }
	
	std::cout << "#" << std::endl;
}

void IAILibraryManager::OutputSkirmishAIInfo() {

	const IAILibraryManager* myLibManager = IAILibraryManager::GetInstance();
	const T_skirmishAIKeys* keys = myLibManager->GetSkirmishAIKeys();
	
	std::cout << "#" << std::endl;
	std::cout << "# Available Spring Skirmish AIs" << std::endl;
	std::cout << "# -----------------------------" << std::endl;
	std::cout << "# [Name]              [Version]           "
			"[Interface-name]    [Interface-version]" << std::endl;
	
	T_skirmishAIKeys::const_iterator key;
    for (key=keys->begin(); key != keys->end(); key++) {
		std::cout << "  ";
		std::cout << key->ai.shortName << fillUpTo(key->ai.shortName, 20);
		std::cout << key->ai.version << fillUpTo(key->ai.version, 20);
		std::cout << key->interface.shortName
				<< fillUpTo(key->interface.shortName, 20);
		std::cout << key->interface.version << std::endl;
    }
	
	const T_dupSkirm* duplicateSkirmishAIInfos =
			myLibManager->GetDuplicateSkirmishAIInfos();
	for (T_dupSkirm::const_iterator info = duplicateSkirmishAIInfos->begin();
			info != duplicateSkirmishAIInfos->end(); ++info) {
		std::cout << "# WARNING: Duplicate Skirmish AI Info found:"
				<< std::endl;
		std::cout << "# \tfor Skirmish AI: " << info->first.ai.shortName
				<< " " << info->first.ai.version << std::endl;
		std::cout << "# \tin files:" << std::endl;
		std::set<std::string>::const_iterator dir;
		for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
			std::cout << "# \t" << dir->c_str() << std::endl;
		}
	}
	
	std::cout << "#" << std::endl;
}

void IAILibraryManager::OutputGroupAIInfo() {
	
	const IAILibraryManager* myLibManager = IAILibraryManager::GetInstance();
	const T_groupAIKeys* keys = myLibManager->GetGroupAIKeys();
	
	std::cout << "#" << std::endl;
	std::cout << "# Available Spring Group AIs" << std::endl;
	std::cout << "# --------------------------" << std::endl;
	std::cout << "# [Name]              [Version]           "
			"[Interface-name]    [Interface-version]" << std::endl;
	
	T_groupAIKeys::const_iterator key;
    for (key=keys->begin(); key != keys->end(); key++) {
		std::cout << "  ";
		std::cout << key->ai.shortName << fillUpTo(key->ai.shortName, 20);
		std::cout << key->ai.version << fillUpTo(key->ai.version, 20);
		std::cout << key->interface.shortName
				<< fillUpTo(key->interface.shortName, 20);
		std::cout << key->interface.version << std::endl;
    }
	
	const T_dupGroup* duplicateGroupAIInfos =
			myLibManager->GetDuplicateGroupAIInfos();
	for (T_dupGroup::const_iterator info = duplicateGroupAIInfos->begin();
			info != duplicateGroupAIInfos->end(); ++info) {
		std::cout << "# WARNING: Duplicate Group AI Info found:"
				<< std::endl;
		std::cout << "# \tfor Group AI: " << info->first.ai.shortName
				<< " " << info->first.ai.version << std::endl;
		std::cout << "# \tin files:" << std::endl;
		std::set<std::string>::const_iterator dir;
		for (dir = info->second.begin(); dir != info->second.end(); ++dir) {
			std::cout << "# \t" << dir->c_str() << std::endl;
		}
	}
	
	std::cout << "#" << std::endl;
}
	
bool IAILibraryManager::SplittAIKey(const std::string& key,
		std::string* aiName,
		std::string* aiVersion,
		std::string* interfaceName,
		std::string* interfaceVersion) {

	bool isValid = true;

	size_t sepPos;
	size_t versionSepPos;
	std::string tmp;

	// interface
	sepPos = key.find('_');
	if (sepPos != std::string::npos) { // interface info is available
		tmp = key.substr(0, sepPos); // interface part
		versionSepPos = tmp.find('#');
		if (versionSepPos != std::string::npos) { // interface version info available
			interfaceName = new std::string(tmp.substr(0, versionSepPos));
			interfaceVersion = new std::string(tmp.substr(versionSepPos+1));
		} else {
			interfaceName = new std::string(tmp);
		}

		tmp = key.substr(sepPos+1); // ai part
	} else {
		tmp = key; // ai part
	}

	// ai
	versionSepPos = tmp.find('#');
	if (versionSepPos != std::string::npos) { // ai version info available
		aiName = new std::string(tmp.substr(0, versionSepPos));
		aiVersion = new std::string(tmp.substr(versionSepPos+1));
	} else {
		aiName = new std::string(tmp);
	}

	return isValid;
}
