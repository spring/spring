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

IAILibraryManager* IAILibraryManager::myAILibraryManager = NULL;

IAILibraryManager* IAILibraryManager::GetInstance() {

	if (myAILibraryManager == NULL) {
		myAILibraryManager = SAFE_NEW CAILibraryManager();
	}

	return myAILibraryManager;
}

void IAILibraryManager::OutputSkirmishAIInfo() {

	const IAILibraryManager* myLibManager = IAILibraryManager::GetInstance();
	const std::vector<SSAIKey>* keys = myLibManager->GetSkirmishAIKeys();
	
	std::cout << "#" << std::endl;
	std::cout << "# Available Spring Skirmish AIs" << std::endl;
	std::cout << "# -----------------------------" << std::endl;
	std::cout << "# [Name]\t[Version]\t[Interface-name]\t[Interface-version]" << std::endl;
	
	std::vector<SSAIKey>::const_iterator key;
    for (key=keys->begin(); key != keys->end(); key++) {
		std::cout << "  ";
		std::cout << key->ai.shortName << "\t\t";
		std::cout << key->ai.version << "\t\t";
		std::cout << key->interface.shortName << "\t\t\t";
		std::cout << key->interface.version;
		std::cout << std::endl;
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
