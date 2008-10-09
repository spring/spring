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

#include "Interface.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"

#include "Log.h"
#include "Platform/SharedLib.h"

CInterface::CInterface() {}

int CInterface::GetInfos(InfoItem infos[], unsigned int max) {
	
	unsigned int i = 0;
	
	if (myInfos.empty()) {
		InfoItem ii_0 = {AI_INTERFACE_PROPERTY_SHORT_NAME, "C", NULL}; myInfos.push_back(ii_0);
		InfoItem ii_1 = {AI_INTERFACE_PROPERTY_VERSION, "0.1", NULL}; myInfos.push_back(ii_1);
		InfoItem ii_2 = {AI_INTERFACE_PROPERTY_NAME, "default C & C++ (legacy and new)", NULL}; myInfos.push_back(ii_2);
		InfoItem ii_3 = {AI_INTERFACE_PROPERTY_DESCRIPTION, "This AI Interface library is needed for Skirmish and Group AIs written in C or C++.", NULL}; myInfos.push_back(ii_3);
		InfoItem ii_4 = {AI_INTERFACE_PROPERTY_URL, "http://spring.clan-sy.com/wiki/AIInterface:C", NULL}; myInfos.push_back(ii_4);
		InfoItem ii_5 = {AI_INTERFACE_PROPERTY_SUPPORTED_LANGUAGES, "C, C++", NULL}; myInfos.push_back(ii_5);
	}
	
	std::vector<InfoItem>::const_iterator inf;
	for (inf=myInfos.begin(); inf!=myInfos.end() && i < max; inf++) {
		infos[i] = *inf;
		i++;
	}
	
	return i;
}

LevelOfSupport CInterface::GetLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
	return LOS_Working;
}

void copyToInfoMap(std::map<std::string, InfoItem>& infoMap, const InfoItem infos[], unsigned int numInfos) {

	for (unsigned int i=0; i < numInfos; ++i) {
		infoMap[infos[i].key] = copyInfoItem(&(infos[i]));
	}
}
SSAISpecifier extractSSAISpecifier(const std::map<std::string, InfoItem>& infoMap) {

	const char* sn = infoMap.find(SKIRMISH_AI_PROPERTY_SHORT_NAME)->second.value;
	const char* v = infoMap.find(SKIRMISH_AI_PROPERTY_VERSION)->second.value;

	SSAISpecifier specifier = {sn, v};

	return specifier;
}
SGAISpecifier extractSGAISpecifier(const std::map<std::string, InfoItem>& infoMap) {

	const char* sn = infoMap.find(GROUP_AI_PROPERTY_SHORT_NAME)->second.value;
	const char* v = infoMap.find(GROUP_AI_PROPERTY_VERSION)->second.value;

	SGAISpecifier specifier = {sn, v};

	return specifier;
}

const SSAILibrary* CInterface::LoadSkirmishAILibrary(const InfoItem infos[], unsigned int numInfos) {

	SSAILibrary* ai;

	std::map<std::string, InfoItem> infoMap;
	copyToInfoMap(infoMap, infos, numInfos);
	SSAISpecifier sAISpecifier = extractSSAISpecifier(infoMap);
	mySkirmishAISpecifiers.push_back(sAISpecifier);
	mySkirmishAIInfos[sAISpecifier] = infoMap;

	T_skirmishAIs::iterator skirmishAI;
	skirmishAI = myLoadedSkirmishAIs.find(sAISpecifier);
	if (skirmishAI == myLoadedSkirmishAIs.end()) {
		ai = new SSAILibrary;
		SharedLib* lib = Load(&sAISpecifier, ai);
		myLoadedSkirmishAIs[sAISpecifier] = ai;
		myLoadedSkirmishAILibs[sAISpecifier] = lib;
	} else {
		ai = skirmishAI->second;
	}

	return ai;
}
int CInterface::UnloadSkirmishAILibrary(const SSAISpecifier* const sAISpecifier) {

	T_skirmishAIs::iterator skirmishAI = myLoadedSkirmishAIs.find(*sAISpecifier);
	T_skirmishAILibs::iterator skirmishAILib = myLoadedSkirmishAILibs.find(*sAISpecifier);
	if (skirmishAI == myLoadedSkirmishAIs.end()) {
		// to unload AI is not loaded -> no problem, do nothing
	} else {
		delete skirmishAI->second;
		myLoadedSkirmishAIs.erase(skirmishAI);
		delete skirmishAILib->second;
		myLoadedSkirmishAILibs.erase(skirmishAILib);
	}

	return 0;
}
int CInterface::UnloadAllSkirmishAILibraries() {

	while (myLoadedSkirmishAIs.size() > 0) {
		UnloadSkirmishAILibrary(&(myLoadedSkirmishAIs.begin()->first));
	}

	return 0;
}



const SGAILibrary* CInterface::LoadGroupAILibrary(const struct InfoItem infos[], unsigned int numInfos) {

	SGAILibrary* ai = NULL;

	std::map<std::string, InfoItem> infoMap;
	copyToInfoMap(infoMap, infos, numInfos);
	SGAISpecifier gAISpecifier = extractSGAISpecifier(infoMap);
	myGroupAISpecifiers.push_back(gAISpecifier);
	myGroupAIInfos[gAISpecifier] = infoMap;

	T_groupAIs::iterator groupAI;
	groupAI = myLoadedGroupAIs.find(gAISpecifier);
	if (groupAI == myLoadedGroupAIs.end()) {
		ai = new SGAILibrary;
		SharedLib* lib = Load(&gAISpecifier, ai);
		myLoadedGroupAIs[gAISpecifier] = ai;
		myLoadedGroupAILibs[gAISpecifier] = lib;
	} else {
		ai = groupAI->second;
	}

	return ai;
}
int CInterface::UnloadGroupAILibrary(const SGAISpecifier* const gAISpecifier) {

	T_groupAIs::iterator groupAI = myLoadedGroupAIs.find(*gAISpecifier);
	T_groupAILibs::iterator groupAILib = myLoadedGroupAILibs.find(*gAISpecifier);
	if (groupAI == myLoadedGroupAIs.end()) {
		// to unload AI is not loaded -> no problem, do nothing
	} else {
		delete groupAI->second;
		myLoadedGroupAIs.erase(groupAI);
		delete groupAILib->second;
		myLoadedGroupAILibs.erase(groupAILib);
	}

	return 0;
}
int CInterface::UnloadAllGroupAILibraries() {
	
	while (myLoadedGroupAIs.size() > 0) {
		UnloadGroupAILibrary(&(myLoadedGroupAIs.begin()->first));
	}
	
	return 0;
}

// private functions following

SharedLib* CInterface::Load(const SSAISpecifier* const sAISpecifier, SSAILibrary* skirmishAILibrary) {
	return LoadSkirmishAILib(GenerateLibFilePath(*sAISpecifier), skirmishAILibrary);
}
SharedLib* CInterface::LoadSkirmishAILib(const std::string& libFilePath, SSAILibrary* skirmishAILibrary) {

	SharedLib* sharedLib = SharedLib::Instantiate(libFilePath);

	if (sharedLib == NULL) {
		reportError(std::string("Failed loading shared library: ") + libFilePath);
	}

	// initialize the AI library
	std::string funcName;

	funcName = "getInfos";
	skirmishAILibrary->getInfos = (unsigned int (CALLING_CONV_FUNC_POINTER *)(InfoItem[], unsigned int)) sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->getInfos == NULL) {
		// do nothing: this is permitted, if the AI supplies infos through an AIInfo.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
	}

	funcName = "getOptions";
	skirmishAILibrary->getOptions = (unsigned int (CALLING_CONV_FUNC_POINTER *)(Option[], unsigned int max)) sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->getOptions == NULL) {
		// do nothing: this is permitted, if the AI supplies options through an AIOptions.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
	}

	funcName = "getLevelOfSupportFor";
	skirmishAILibrary->getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(const char*, int, const char*, const char*)) sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	funcName = "init";
	skirmishAILibrary->init = (int (CALLING_CONV_FUNC_POINTER *)(int)) sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->init == NULL) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_INIT instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	funcName = "release";
	skirmishAILibrary->release = (int (CALLING_CONV_FUNC_POINTER *)(int)) sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->release == NULL) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_RELEASE instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	funcName = "handleEvent";
	skirmishAILibrary->handleEvent = (int (CALLING_CONV_FUNC_POINTER *)(int, int, const void*)) sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->handleEvent == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	return sharedLib;
}


SharedLib* CInterface::Load(const SGAISpecifier* const gAISpecifier, SGAILibrary* groupAILibrary) {
	return LoadGroupAILib(GenerateLibFilePath(*gAISpecifier), groupAILibrary);
}
SharedLib* CInterface::LoadGroupAILib(const std::string& libFilePath, SGAILibrary* groupAILibrary) {
	
	SharedLib* sharedLib = SharedLib::Instantiate(libFilePath);
	
	// initialize the AI library
	std::string funcName;
	
	funcName = "getInfos";
	groupAILibrary->getInfos = (unsigned int (CALLING_CONV_FUNC_POINTER *)(InfoItem[], unsigned int max)) sharedLib->FindAddress(funcName.c_str());
	if (groupAILibrary->getInfos == NULL) {
		// do nothing: this is permitted, if the AI supplies infos through an AIInfo.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	funcName = "getLevelOfSupportFor";
	groupAILibrary->getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(const char*, int, const char*, const char*)) sharedLib->FindAddress(funcName.c_str());
	if (groupAILibrary->getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	funcName = "getOptions";
	groupAILibrary->getOptions = (unsigned int (CALLING_CONV_FUNC_POINTER *)(Option[], unsigned int max)) sharedLib->FindAddress(funcName.c_str());
	if (groupAILibrary->getOptions == NULL) {
		// do nothing: this is permitted, if the AI supplies options through an AIOptions.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	funcName = "init";
	groupAILibrary->init = (int (CALLING_CONV_FUNC_POINTER *)(int, int)) sharedLib->FindAddress(funcName.c_str());
	if (groupAILibrary->init == NULL) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_INIT instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	funcName = "release";
	groupAILibrary->release = (int (CALLING_CONV_FUNC_POINTER *)(int, int)) sharedLib->FindAddress(funcName.c_str());
	if (groupAILibrary->release == NULL) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_RELEASE instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	funcName = "handleEvent";
	groupAILibrary->handleEvent = (int (CALLING_CONV_FUNC_POINTER *)(int, int, int, const void*)) sharedLib->FindAddress(funcName.c_str());
	if (groupAILibrary->handleEvent == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
	}
	
	return sharedLib;
}


void CInterface::reportInterfaceFunctionError(const std::string& libFilePath, const std::string& functionName) {
	
	std::string msg("Failed loading AI Library from file \"");
	msg += libFilePath + "\": no \"" + functionName + "\" function exported";
	reportError(msg);
}

void CInterface::reportError(const std::string& msg) {
	///handleerror(NULL, msg.c_str(), "C AI Interface Error", MBF_OK | MBF_EXCL);
	logFatalError(msg.c_str());
}


std::string CInterface::GenerateLibFilePath(const SSAISpecifier& sAISpecifier) {
	
	static const std::string path = std::string(PATH_TO_SPRING_HOME""SKIRMISH_AI_IMPLS_DIR"/");
	
	T_skirmishAIInfos::const_iterator info = mySkirmishAIInfos.find(sAISpecifier);
	if (info == mySkirmishAIInfos.end()) {
		reportError(std::string("Missing Skirmish-AI info for ") + sAISpecifier.shortName + " " + sAISpecifier.version);
	}
	std::map<std::string, InfoItem>::const_iterator fileName = info->second.find(SKIRMISH_AI_PROPERTY_FILE_NAME);
	if (fileName == info->second.end()) {
		reportError(std::string("Missing Skirmish-AI file name for ") + sAISpecifier.shortName + " " + sAISpecifier.version);
	}
	
	std::string libFileName = fileName->second.value; // eg. RAI-0.600
#ifndef _WIN32
	libFileName = "lib" + libFileName; // eg. libRAI-0.600
#endif
	
	libFileName.append("."); // eg. libRAI-0.600.
	libFileName.append(SharedLib::GetLibExtension()); // eg. libRAI-0.600.so
	
	return path + libFileName;
}

std::string CInterface::GenerateLibFilePath(const SGAISpecifier& gAISpecifier) {
	
	static const std::string path = std::string(PATH_TO_SPRING_HOME""GROUP_AI_IMPLS_DIR"/");
	
	T_groupAIInfos::const_iterator info = myGroupAIInfos.find(gAISpecifier);
	if (info == myGroupAIInfos.end()) {
		reportError(std::string("Missing Group-AI info for ") + gAISpecifier.shortName + " " + gAISpecifier.version);
	}
	std::map<std::string, InfoItem>::const_iterator fileName = info->second.find(GROUP_AI_PROPERTY_FILE_NAME);
	if (fileName == info->second.end()) {
		reportError(std::string("Missing Group-AI file name for ") + gAISpecifier.shortName + " " + gAISpecifier.version);
	}
	
	std::string libFileName = fileName->second.value; // eg. RAI-0.600
#ifndef _WIN32
	libFileName = "lib" + libFileName; // eg. libRAI-0.600
#endif
	
	libFileName.append("."); // eg. libRAI-0.600.
	libFileName.append(SharedLib::GetLibExtension()); // eg. libRAI-0.600.so
	
	return path + libFileName;
}

SSAISpecifier CInterface::extractSpecifier(const SSAILibrary& skirmishAILib) {
	
	SSAISpecifier skirmishAISpecifier;
	
	InfoItem infos[MAX_INFOS];
	int numInfos =  skirmishAILib.getInfos(infos, MAX_INFOS);

	std::string spsn = std::string(SKIRMISH_AI_PROPERTY_SHORT_NAME);
	std::string spv = std::string(SKIRMISH_AI_PROPERTY_VERSION);
	for (int i=0; i < numInfos; ++i) {
		std::string key = std::string(infos[i].key);
		if (key == spsn) {
			skirmishAISpecifier.shortName = infos[i].value;
		} else if (key == spv) {
			skirmishAISpecifier.version = infos[i].value;
		}
	}
	
	return skirmishAISpecifier;
}

SGAISpecifier CInterface::extractSpecifier(const SGAILibrary& groupAILib) {
	
	SGAISpecifier groupAISpecifier;
	
	InfoItem infos[MAX_INFOS];
	int numInfos =  groupAILib.getInfos(infos, MAX_INFOS);

	std::string spsn = std::string(GROUP_AI_PROPERTY_SHORT_NAME);
	std::string spv = std::string(GROUP_AI_PROPERTY_VERSION);
	for (int i=0; i < numInfos; ++i) {
		std::string key = std::string(infos[i].key);
		if (key == spsn) {
			groupAISpecifier.shortName = infos[i].value;
		} else if (key == spv) {
			groupAISpecifier.version = infos[i].value;
		}
	}
	
	return groupAISpecifier;
}

bool CInterface::FitsThisInterface(const std::string& requestedShortName, const std::string& requestedVersion) {
	
	bool shortNameFits = false;
	bool versionFits = false;
	
	InfoItem infos[MAX_INFOS];
	int num = GetInfos(infos, MAX_INFOS);
	
	std::string shortNameKey(AI_INTERFACE_PROPERTY_SHORT_NAME);
	std::string versionKey(AI_INTERFACE_PROPERTY_VERSION);
	for (int i=0; i < num; ++i) {
		if (shortNameKey == infos[i].key) {
			if (requestedShortName == infos[i].value) {
				shortNameFits = true;
			}
		} else if (versionKey == infos[i].key) {
			if (requestedVersion == infos[i].value) {
				versionFits = true;
			}
		}
	}
	
	return shortNameFits && versionFits;
}
