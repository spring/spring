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

#include "AIInterfaceLibrary.h"

#include "Interface/aidefines.h"
#include "Interface/SAIInterfaceLibrary.h"
#include "SkirmishAILibrary.h"
#include "GroupAILibrary.h"

#include "StdAfx.h"
#include "Platform/errorhandler.h"

#include <string>


/*
CAIInterfaceLibrary::CAIInterfaceLibrary(const CAIInterfaceLibrary& interface) {
	int loadCount = 0;
	SAIInterfaceLibrary sAIInterfaceLibrary;wecd
}
CAIInterfaceLibrary::CAIInterfaceLibrary(SharedLib* sharedLib) {TODO;}
*/

CAIInterfaceLibrary::CAIInterfaceLibrary(
		const SAIInterfaceSpecifyer& interfaceSpecifyer,
		const std::string& libFileName)
		: specifyer(interfaceSpecifyer) {
	
	std::string libFilePath;
	
	if (libFileName.empty()) {
		libFilePath = GenerateLibFilePath(specifyer);
	} else {
		libFilePath = GenerateLibFilePath(libFileName);
	}
	
	sharedLib = SharedLib::Instantiate(libFilePath);
	if (sharedLib == NULL) {
		const int MAX_MSG_LENGTH = 511;
		char s_msg[MAX_MSG_LENGTH + 1];
		SNPRINTF(s_msg, MAX_MSG_LENGTH,
				"Error while loading AI Interface Library from file \"%s\"",
				libFilePath.c_str());
		handleerror(NULL, s_msg, "AI Interface Error", MBF_OK | MBF_EXCL);
	}
	
	InitializeFromLib(libFilePath);
}
/*
CAIInterfaceLibrary::CAIInterfaceLibrary(const std::string& libFileName,
		const SAIInterfaceSpecifyer& interfaceSpecifyer)
		: specifyer(interfaceSpecifyer) {
	
	std::string libFilePath = GenerateLibFilePath(libFileName);
	
	sharedLib = SharedLib::Instantiate(libFilePath);
	
	InitializeFromLib(libFilePath);
*/
	
/*
	std::map<std::string, InfoItem> infos = GetInfos();
	specifyer.shortName = infos.at(AI_INTERFACE_PROPERTY_SHORT_NAME).value;
	specifyer.version = infos.at(AI_INTERFACE_PROPERTY_VERSION).value;
*/
//}

CAIInterfaceLibrary::~CAIInterfaceLibrary() {
	delete sharedLib;
}

SAIInterfaceSpecifyer CAIInterfaceLibrary::GetSpecifyer() const {
	return specifyer;
}

LevelOfSupport CAIInterfaceLibrary::GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber) const {
	
	if (sAIInterfaceLibrary.getLevelOfSupportFor != NULL) {
		return sAIInterfaceLibrary.getLevelOfSupportFor(engineVersionString.c_str(), engineVersionNumber);
	} else {
		return LOS_Unknown;
	}
}

std::map<std::string, InfoItem> CAIInterfaceLibrary::GetInfos() const {
	
	std::map<std::string, InfoItem> infos;
	
	if (sAIInterfaceLibrary.getInfos != NULL) {
		InfoItem infs[MAX_INFOS];
		int num = sAIInterfaceLibrary.getInfos(infs, MAX_INFOS);

		int i;
		for (i=0; i < num; ++i) {
			InfoItem newII = copyInfoItem(&infs[i]);
			infos[std::string(newII.key)] = newII;
		}
	}

	return infos;
}

int CAIInterfaceLibrary::GetLoadCount() const {
	
	int totalSkirmishAILibraryLoadCount = 0;
	std::map<const SSAISpecifyer, int>::const_iterator salc;
	for (salc=skirmishAILoadCount.begin();  salc != skirmishAILoadCount.end(); salc++) {
		totalSkirmishAILibraryLoadCount += salc->second;
	}
	
	int totalGroupAILibraryLoadCount = 0;
	std::map<const SGAISpecifyer, int>::const_iterator galc;
	for (galc=groupAILoadCount.begin();  galc != groupAILoadCount.end(); galc++) {
		totalGroupAILibraryLoadCount += galc->second;
	}
	
	return totalSkirmishAILibraryLoadCount + totalGroupAILibraryLoadCount;
}


// Skirmish AI methods
std::vector<SSAISpecifyer> CAIInterfaceLibrary::GetSkirmishAILibrarySpecifyers() const {
	
	std::vector<SSAISpecifyer> specs;
	
	int max = 128;
	SSAISpecifyer specifyers[max];
	int num = sAIInterfaceLibrary.getSkirmishAISpecifyers(specifyers, max);
    for (int i=0; i < num; ++i) {
        specs.push_back(specifyers[i]);
    }
	
	return specs;
}
const ISkirmishAILibrary* CAIInterfaceLibrary::FetchSkirmishAILibrary(const SSAISpecifyer& sAISpecifyer) {
	
	ISkirmishAILibrary* ai = NULL;
	
	if (skirmishAILoadCount[sAISpecifyer] == 0) {
		const SSAILibrary* sLib = sAIInterfaceLibrary.loadSkirmishAILibrary(&sAISpecifyer);
		ai = new CSkirmishAILibrary(*sLib, sAISpecifyer);
		loadedSkirmishAILibraries[sAISpecifyer] = ai;
	} else {
		ai = loadedSkirmishAILibraries[sAISpecifyer];
	}
	
	skirmishAILoadCount[sAISpecifyer]++;
	
	return ai;
}
int CAIInterfaceLibrary::ReleaseSkirmishAILibrary(const SSAISpecifyer& sAISpecifyer) {
	
	if (skirmishAILoadCount[sAISpecifyer] == 0) {
		return 0;
	}
	
	skirmishAILoadCount[sAISpecifyer]--;
	
	if (skirmishAILoadCount[sAISpecifyer] == 0) {
		loadedSkirmishAILibraries.erase(sAISpecifyer);
		sAIInterfaceLibrary.unloadSkirmishAILibrary(&sAISpecifyer);
	}
	
	return skirmishAILoadCount[sAISpecifyer];
}
int CAIInterfaceLibrary::GetSkirmishAILibraryLoadCount(const SSAISpecifyer& sAISpecifyer) const {
	return skirmishAILoadCount.at(sAISpecifyer);
}
int CAIInterfaceLibrary::ReleaseAllSkirmishAILibraries() {
	
	int releasedAIs = sAIInterfaceLibrary.unloadAllSkirmishAILibraries();
	loadedSkirmishAILibraries.clear();
	skirmishAILoadCount.clear();
	
	return releasedAIs;
}



// Group AI methods
std::vector<SGAISpecifyer> CAIInterfaceLibrary::GetGroupAILibrarySpecifyers() const {
	
	std::vector<SGAISpecifyer> specs;
	
	int max = 128;
	SGAISpecifyer specifyers[max];
	int num = sAIInterfaceLibrary.getGroupAISpecifyers(specifyers, max);
    for (int i=0; i < num; ++i) {
        specs.push_back(specifyers[i]);
    }
	
	return specs;
}
const IGroupAILibrary* CAIInterfaceLibrary::FetchGroupAILibrary(const SGAISpecifyer& gAISpecifyer) {
	
	IGroupAILibrary* ai = NULL;
	
	if (groupAILoadCount[gAISpecifyer] == 0) {
		const SGAILibrary* gLib = sAIInterfaceLibrary.loadGroupAILibrary(&gAISpecifyer);
		ai = new CGroupAILibrary(*gLib);
		loadedGroupAILibraries[gAISpecifyer] = ai;
	} else {
		ai = loadedGroupAILibraries[gAISpecifyer];
	}
	
	groupAILoadCount[gAISpecifyer]++;
	
	return ai;
}
int CAIInterfaceLibrary::ReleaseGroupAILibrary(const SGAISpecifyer& gAISpecifyer) {
	
	if (groupAILoadCount[gAISpecifyer] == 0) {
		return 0;
	}
	
	groupAILoadCount[gAISpecifyer]--;
	
	if (groupAILoadCount[gAISpecifyer] == 0) {
		loadedGroupAILibraries.erase(gAISpecifyer);
		sAIInterfaceLibrary.unloadGroupAILibrary(&gAISpecifyer);
	}
	
	return groupAILoadCount[gAISpecifyer];
}
int CAIInterfaceLibrary::GetGroupAILibraryLoadCount(const SGAISpecifyer& gAISpecifyer) const {
	return groupAILoadCount.at(gAISpecifyer);
}
int CAIInterfaceLibrary::ReleaseAllGroupAILibraries() {
	
	int releasedAIs = sAIInterfaceLibrary.unloadAllGroupAILibraries();
	loadedGroupAILibraries.clear();
	groupAILoadCount.clear();
	
	return releasedAIs;
}
	
	

void CAIInterfaceLibrary::reportInterfaceFunctionError(const std::string* libFileName, const std::string* functionName) {
	
	const int MAX_MSG_LENGTH = 511;
	char s_msg[MAX_MSG_LENGTH + 1];
	SNPRINTF(s_msg, MAX_MSG_LENGTH,
			"Error loading AI Interface Library from file \"%s\": no \"%s\" function exported",
			libFileName->c_str(), functionName->c_str());
	handleerror(NULL, s_msg, "AI Interface Error", MBF_OK | MBF_EXCL);
}
int CAIInterfaceLibrary::InitializeFromLib(const std::string& libFilePath) {
	
    // TODO: version checking
	
	std::string funcName;
	
	funcName = "getInfos";
    sAIInterfaceLibrary.getInfos = (int (CALLING_CONV *)(InfoItem[], int)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.getInfos == NULL) {
		// do nothing: this is permitted, if the AI supplies infos through an AIInfo.lua file
		//reportInterfaceFunctionError(&libFilePath, &funcName);
        //return -1;
    }
	
	funcName = "getLevelOfSupportFor";
    sAIInterfaceLibrary.getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV *)(const char*, int)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(&libFilePath, &funcName);
        //return -2;
    }
	
	funcName = "getSkirmishAISpecifyers";
    sAIInterfaceLibrary.getSkirmishAISpecifyers = (int (CALLING_CONV *)(SSAISpecifyer*, int max)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.getSkirmishAISpecifyers == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -3;
    }
	
	funcName = "loadSkirmishAILibrary";
    sAIInterfaceLibrary.loadSkirmishAILibrary = (const SSAILibrary* (CALLING_CONV *)(const SSAISpecifyer* const)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.loadSkirmishAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -4;
    }
	
	funcName = "unloadSkirmishAILibrary";
    sAIInterfaceLibrary.unloadSkirmishAILibrary = (int (CALLING_CONV *)(const SSAISpecifyer* const)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.unloadSkirmishAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -5;
    }
	
	funcName = "unloadAllSkirmishAILibraries";
    sAIInterfaceLibrary.unloadAllSkirmishAILibraries = (int (CALLING_CONV *)()) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.unloadAllSkirmishAILibraries == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -6;
    }
	
	funcName = "getGroupAISpecifyers";
    sAIInterfaceLibrary.getGroupAISpecifyers = (int (CALLING_CONV *)(SGAISpecifyer*, int max)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.getGroupAISpecifyers == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -7;
    }
	
	funcName = "loadGroupAILibrary";
    sAIInterfaceLibrary.loadGroupAILibrary = (const SGAILibrary* (CALLING_CONV *)(const SGAISpecifyer* const)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.loadGroupAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -8;
    }
	
	funcName = "unloadGroupAILibrary";
    sAIInterfaceLibrary.unloadGroupAILibrary = (int (CALLING_CONV *)(const SGAISpecifyer* const)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.unloadGroupAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -9;
    }
	
	funcName = "unloadAllGroupAILibraries";
    sAIInterfaceLibrary.unloadAllGroupAILibraries = (int (CALLING_CONV *)()) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.unloadAllGroupAILibraries == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -10;
    }
	
	return 0;
}


std::string CAIInterfaceLibrary::GenerateLibFilePath(const std::string& libFileName) {
	return std::string(PATH_TO_SPRING_HOME) +
			std::string(AI_INTERFACES_IMPLS_DIR) // eg AI/Interfaces/impls
			.append("/")
			.append(libFileName) // eg. Java-0.600
			.append(".")
			.append(SharedLib::GetLibExtension()); // eg. dll
}

std::string CAIInterfaceLibrary::GenerateLibFilePath(const SAIInterfaceSpecifyer& interfaceSpecifyer) {
	
	std::string libFileName = std::string(interfaceSpecifyer.shortName) // eg. Java
			.append("-")
			.append(interfaceSpecifyer.version); // eg. 0.600
#ifndef _WIN32
	libFileName = "lib" + libFileName;
#endif
	return GenerateLibFilePath(libFileName);
}

