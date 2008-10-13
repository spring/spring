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
#include "Interface/SStaticGlobalData.h"
#include "SkirmishAILibrary.h"
#include "GroupAILibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"
#include "GroupAILibraryInfo.h"

#include "System/Util.h"
#include "System/Platform/errorhandler.h"
#include "System/FileSystem/FileHandler.h"

/*
CAIInterfaceLibrary::CAIInterfaceLibrary(
		const SAIInterfaceSpecifier& interfaceSpecifier,
		const std::string& libFileName)
		: specifier(interfaceSpecifier) {
*/
CAIInterfaceLibrary::CAIInterfaceLibrary(const CAIInterfaceLibraryInfo* _info)
		: info(_info) {
	
	std::string libFilePath;

	std::string libFileName = info->GetFileName();
	if (libFileName.empty()) {
		handleerror(NULL, "Error while loading AI Interface Library."
				" No file name specified in AIInterface.lua",
				"AI Interface Error", MBF_OK | MBF_EXCL);
	}
	libFilePath = FindLibFile(libFileName);
	
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
	
	InitStatic();
}

CAIInterfaceLibrary::~CAIInterfaceLibrary() {
	
	ReleaseStatic();
	delete sharedLib;
}

void CAIInterfaceLibrary::InitStatic() {
	
	if (sAIInterfaceLibrary.initStatic != NULL) {
		staticGlobalData = createStaticGlobalData();
		sAIInterfaceLibrary.initStatic(staticGlobalData);
	} else {
		staticGlobalData = NULL;
	}
}
void CAIInterfaceLibrary::ReleaseStatic() {
	
	if (sAIInterfaceLibrary.releaseStatic != NULL) {
		sAIInterfaceLibrary.releaseStatic();
	}
	if (staticGlobalData != NULL) {
		freeStaticGlobalData(staticGlobalData);
	}
}

SAIInterfaceSpecifier CAIInterfaceLibrary::GetSpecifier() const {
	return info->GetSpecifier();
}

LevelOfSupport CAIInterfaceLibrary::GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber) const {
	
	if (sAIInterfaceLibrary.getLevelOfSupportFor != NULL) {
		return sAIInterfaceLibrary.getLevelOfSupportFor(engineVersionString.c_str(), engineVersionNumber);
	} else {
		return LOS_Unknown;
	}
}

std::map<std::string, InfoItem> CAIInterfaceLibrary::GetInfo() const {
	
	std::map<std::string, InfoItem> info;
	
	if (sAIInterfaceLibrary.getInfo != NULL) {
		InfoItem infs[MAX_INFOS];
		int num = sAIInterfaceLibrary.getInfo(infs, MAX_INFOS);

		int i;
		for (i=0; i < num; ++i) {
			InfoItem newII = copyInfoItem(&infs[i]);
			info[std::string(newII.key)] = newII;
		}
	}

	return info;
}

int CAIInterfaceLibrary::GetLoadCount() const {
	
	int totalSkirmishAILibraryLoadCount = 0;
	std::map<const SSAISpecifier, int>::const_iterator salc;
	for (salc=skirmishAILoadCount.begin();  salc != skirmishAILoadCount.end(); salc++) {
		totalSkirmishAILibraryLoadCount += salc->second;
	}
	
	int totalGroupAILibraryLoadCount = 0;
	std::map<const SGAISpecifier, int>::const_iterator galc;
	for (galc=groupAILoadCount.begin();  galc != groupAILoadCount.end(); galc++) {
		totalGroupAILibraryLoadCount += galc->second;
	}
	
	return totalSkirmishAILibraryLoadCount + totalGroupAILibraryLoadCount;
}


// Skirmish AI methods
/*
std::vector<SSAISpecifier> CAIInterfaceLibrary::GetSkirmishAILibrarySpecifiers() const {
	
	std::vector<SSAISpecifier> specs;
	
	int max = 128;
	SSAISpecifier specifiers[max];
	int num = sAIInterfaceLibrary.getSkirmishAISpecifiers(specifiers, max);
    for (int i=0; i < num; ++i) {
        specs.push_back(specifiers[i]);
    }
	
	return specs;
}
*/
//const ISkirmishAILibrary* CAIInterfaceLibrary::FetchSkirmishAILibrary(const SSAISpecifier& sAISpecifier) {
//const ISkirmishAILibrary* CAIInterfaceLibrary::FetchSkirmishAILibrary(const InfoItem* info, unsigned int numInfo) {
const ISkirmishAILibrary* CAIInterfaceLibrary::FetchSkirmishAILibrary(const CSkirmishAILibraryInfo* aiInfo) {
	
	ISkirmishAILibrary* ai = NULL;
	
	const unsigned int MAX_INFOS = 128;
	InfoItem info[MAX_INFOS];
	unsigned int num = aiInfo->GetInfoCReference(info, MAX_INFOS);
	
	SSAISpecifier sAISpecifier = aiInfo->GetSpecifier();
	if (skirmishAILoadCount[sAISpecifier] == 0) {
		//const SSAILibrary* sLib = sAIInterfaceLibrary.loadSkirmishAILibrary(&sAISpecifier);
		const SSAILibrary* sLib = sAIInterfaceLibrary.loadSkirmishAILibrary(info, num);
		ai = new CSkirmishAILibrary(*sLib, sAISpecifier);
		loadedSkirmishAILibraries[sAISpecifier] = ai;
	} else {
		ai = loadedSkirmishAILibraries[sAISpecifier];
	}
	
	skirmishAILoadCount[sAISpecifier]++;
	
	return ai;
}
int CAIInterfaceLibrary::ReleaseSkirmishAILibrary(const SSAISpecifier& sAISpecifier) {
	
	if (skirmishAILoadCount[sAISpecifier] == 0) {
		return 0;
	}
	
	skirmishAILoadCount[sAISpecifier]--;
	
	if (skirmishAILoadCount[sAISpecifier] == 0) {
		loadedSkirmishAILibraries.erase(sAISpecifier);
		sAIInterfaceLibrary.unloadSkirmishAILibrary(&sAISpecifier);
	}
	
	return skirmishAILoadCount[sAISpecifier];
}
int CAIInterfaceLibrary::GetSkirmishAILibraryLoadCount(const SSAISpecifier& sAISpecifier) const {
	return skirmishAILoadCount.at(sAISpecifier);
}
int CAIInterfaceLibrary::ReleaseAllSkirmishAILibraries() {
	
	int releasedAIs = sAIInterfaceLibrary.unloadAllSkirmishAILibraries();
	loadedSkirmishAILibraries.clear();
	skirmishAILoadCount.clear();
	
	return releasedAIs;
}



// Group AI methods
/*
std::vector<SGAISpecifier> CAIInterfaceLibrary::GetGroupAILibrarySpecifiers() const {
	
	std::vector<SGAISpecifier> specs;
	
	int max = 128;
	SGAISpecifier specifiers[max];
	int num = sAIInterfaceLibrary.getGroupAISpecifiers(specifiers, max);
    for (int i=0; i < num; ++i) {
        specs.push_back(specifiers[i]);
    }
	
	return specs;
}
*/
//const IGroupAILibrary* CAIInterfaceLibrary::FetchGroupAILibrary(const SGAISpecifier& gAISpecifier) {
const IGroupAILibrary* CAIInterfaceLibrary::FetchGroupAILibrary(const CGroupAILibraryInfo* aiInfo) {

	IGroupAILibrary* ai = NULL;
	
	const unsigned int MAX_INFOS = 128;
	InfoItem info[MAX_INFOS];
	unsigned int num = aiInfo->GetInfoCReference(info, MAX_INFOS);
	
	SGAISpecifier gAISpecifier = aiInfo->GetSpecifier();
	if (groupAILoadCount[gAISpecifier] == 0) {
		const SGAILibrary* gLib = sAIInterfaceLibrary.loadGroupAILibrary(info, num);
		ai = new CGroupAILibrary(*gLib, gAISpecifier);
		loadedGroupAILibraries[gAISpecifier] = ai;
	} else {
		ai = loadedGroupAILibraries[gAISpecifier];
	}
	
	groupAILoadCount[gAISpecifier]++;
	
	return ai;
}
int CAIInterfaceLibrary::ReleaseGroupAILibrary(const SGAISpecifier& gAISpecifier) {
	
	if (groupAILoadCount[gAISpecifier] == 0) {
		return 0;
	}
	
	groupAILoadCount[gAISpecifier]--;
	
	if (groupAILoadCount[gAISpecifier] == 0) {
		loadedGroupAILibraries.erase(gAISpecifier);
		sAIInterfaceLibrary.unloadGroupAILibrary(&gAISpecifier);
	}
	
	return groupAILoadCount[gAISpecifier];
}
int CAIInterfaceLibrary::GetGroupAILibraryLoadCount(const SGAISpecifier& gAISpecifier) const {
	return groupAILoadCount.at(gAISpecifier);
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
	
	funcName = "initStatic";
    sAIInterfaceLibrary.initStatic = (int (CALLING_CONV_FUNC_POINTER *)(const SStaticGlobalData* staticGlobalData)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.initStatic == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(&libFilePath, &funcName);
        //return -1;
    }
	
	funcName = "releaseStatic";
    sAIInterfaceLibrary.releaseStatic = (int (CALLING_CONV_FUNC_POINTER *)()) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.releaseStatic == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(&libFilePath, &funcName);
        //return -1;
    }
	
	funcName = "getLevelOfSupportFor";
    sAIInterfaceLibrary.getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(const char*, int)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(&libFilePath, &funcName);
        //return -2;
    }
	
	funcName = "getInfo";
    sAIInterfaceLibrary.getInfo = (unsigned int (CALLING_CONV_FUNC_POINTER *)(InfoItem[], unsigned int)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.getInfo == NULL) {
		// do nothing: this is permitted, if the AI supplies info through an AIInfo.lua file
		//reportInterfaceFunctionError(&libFilePath, &funcName);
        //return -1;
    }
	
/*
	funcName = "getSkirmishAISpecifiers";
    sAIInterfaceLibrary.getSkirmishAISpecifiers = (int (CALLING_CONV_FUNC_POINTER *)(SSAISpecifier*, int max)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.getSkirmishAISpecifiers == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -3;
    }
*/
	
	funcName = "loadSkirmishAILibrary";
    sAIInterfaceLibrary.loadSkirmishAILibrary = (const SSAILibrary* (CALLING_CONV_FUNC_POINTER *)(const struct InfoItem info[], unsigned int numInfoItems)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.loadSkirmishAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -4;
    }
	
	funcName = "unloadSkirmishAILibrary";
    sAIInterfaceLibrary.unloadSkirmishAILibrary = (int (CALLING_CONV_FUNC_POINTER *)(const SSAISpecifier* const)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.unloadSkirmishAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -5;
    }
	
	funcName = "unloadAllSkirmishAILibraries";
    sAIInterfaceLibrary.unloadAllSkirmishAILibraries = (int (CALLING_CONV_FUNC_POINTER *)()) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.unloadAllSkirmishAILibraries == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -6;
    }
	
/*
	funcName = "getGroupAISpecifiers";
    sAIInterfaceLibrary.getGroupAISpecifiers = (int (CALLING_CONV_FUNC_POINTER *)(SGAISpecifier*, int max)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.getGroupAISpecifiers == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -7;
    }
*/
	
	funcName = "loadGroupAILibrary";
    sAIInterfaceLibrary.loadGroupAILibrary = (const SGAILibrary* (CALLING_CONV_FUNC_POINTER *)(const struct InfoItem info[], unsigned int numInfoItems)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.loadGroupAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -8;
    }
	
	funcName = "unloadGroupAILibrary";
    sAIInterfaceLibrary.unloadGroupAILibrary = (int (CALLING_CONV_FUNC_POINTER *)(const SGAISpecifier* const)) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.unloadGroupAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -9;
    }
	
	funcName = "unloadAllGroupAILibraries";
    sAIInterfaceLibrary.unloadAllGroupAILibraries = (int (CALLING_CONV_FUNC_POINTER *)()) sharedLib->FindAddress(funcName.c_str());
    if (sAIInterfaceLibrary.unloadAllGroupAILibraries == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
        return -10;
    }
	
	return 0;
}


std::string CAIInterfaceLibrary::FindLibFile(
		const std::string& fileNameMainPart) {
	
	std::string libFileName = fileNameMainPart + "." + SharedLib::GetLibExtension();
	#ifndef _WIN32
		libFileName = "lib" + libFileName;
	#endif
	// libFileName should now look about like this: libJava-0.600.so or Java-0.600.dll
	
	std::vector<std::string> libFile =
			CFileHandler::FindFiles(AI_INTERFACES_IMPLS_DIR, libFileName);
	
	// generate a path like this: "C:/Games/spring/AI/Interfaces/impls/Java-0.600.dll"
	std::string path = "";
	if (libFile.size() >= 1) {
		path = libFile.at(0);
	} else {
		// though the file is not available, we still return the relative path
		// when trying ot load the lib, the user will see through
		// the error message, which file is missing where
		path = std::string(AI_INTERFACES_IMPLS_DIR) + "/" + libFileName;
	}
	
	return path;
}

/*
std::string CAIInterfaceLibrary::GenerateLibFilePath(const SAIInterfaceSpecifier& interfaceSpecifier) {
	REDO THIS (use filename attribute, instead of shortName and version)
	std::string libFileName = std::string(interfaceSpecifier.shortName) // eg. Java
			.append("-")
			.append(interfaceSpecifier.version); // eg. 0.600
#ifndef _WIN32
	libFileName = "lib" + libFileName;
#endif
	return GenerateLibFilePath(libFileName);
}
*/

