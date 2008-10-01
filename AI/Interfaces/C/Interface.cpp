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

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include "StdAfx.h"
#include "System/Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "LogOutput.h"

void CInterface::errorMsgFileNames(const std::string& additionalTxt) const {
	// !!!check everything in mySkirmishAIFileNames and do a reportError!!!;
	std::string msg("mySkirmishAIFileNames:\n");
	char buffer[16];
	sprintf(buffer, "%i", mySkirmishAIFileNames.size());
	msg.append("num: ").append(buffer).append("\n");
	T_skirmishAIFileNames::const_iterator fn;
	for (fn=mySkirmishAIFileNames.begin(); fn!=mySkirmishAIFileNames.end(); fn++) {
		msg.append(fn->first.shortName).append("\t");
		msg.append(fn->first.version).append("\t");
		msg.append(fn->second).append("\n");
	}
	msg.append("additionalTxt:\n").append(additionalTxt).append("\n");
	
	reportError(msg);
}

CInterface::CInterface() {
	
/*
	// look for Skirmish AI library files
	std::vector<std::string> skirmishAILibFiles =
			FindFiles(std::string(PATH_TO_SPRING_HOME) +
			SKIRMISH_AI_IMPLS_DIR,
			std::string(".") + SharedLib::GetLibExtension());
	
	// initialize the skirmish AI specifyers
	std::vector<std::string>::const_iterator libFile;
	for (libFile=skirmishAILibFiles.begin(); libFile!=skirmishAILibFiles.end(); libFile++) { // skimrihs IAs
		
		//std::string fileName = std::string(extractFileName(*libFile, false));
		std::string fileName = boost::filesystem::basename(*libFile);
		
		// load the interface
		SSAILibrary skirmishAILib;
		SharedLib* sharedLib = LoadSkirmishAILib(*libFile, &skirmishAILib);
		if (sharedLib == NULL) {
			reportError(std::string("Failed to load skirmish AI shared library: ") + (*libFile));
		}
		
		SSAISpecifyer skirmishAISpecifyer = extractSpecifyer(skirmishAILib);
		skirmishAISpecifyer = copySSAISpecifyer(&skirmishAISpecifyer);
		mySkirmishAISpecifyers.push_back(skirmishAISpecifyer);
		
		delete sharedLib;
	}
*/
	
	//static const unsigned int MAX_INFOS = 128;
	
	// Read from Skirmish AI info and option files
	std::vector<std::string> skirmishAIDataDirs = FindDirs(SKIRMISH_AI_DATA_DIR);
	for (std::vector<std::string>::iterator dir = skirmishAIDataDirs.begin(); dir != skirmishAIDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		boost::filesystem::path fullPath(possibleDataDir + "/AIInfo.lua");
		if (boost::filesystem::exists(fullPath)) { // skirmish AI info is available
			//logOutput.Print("FFFFFF AIInfo.lua file: %s\n", fullPath.string().c_str());
			// parse the ai info and extract specifyer and filename
			InfoItem tmpInfos[MAX_INFOS];
			unsigned int num = ParseInfosRawFileSystem(fullPath.string().c_str(), tmpInfos, MAX_INFOS);
			std::map<std::string,const char*> infos;
			for (unsigned int i=0; i < num; ++i) {
				infos[std::string(tmpInfos[i].key)] = tmpInfos[i].value;
			}
	
			const char* intSN = infos[SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME];
			const char* intV = infos[SKIRMISH_AI_PROPERTY_INTERFACE_VERSION];
			//logOutput.Print("FFFFFF required interface (SN/V): %s %s\n", intSN, intV);
/*
std::string msg("");
char buffer[128];
sprintf(buffer, "FFF: %s %s\n", intSN, intV);
msg.append(buffer);
reportError(msg);
*/
			bool fits = FitsThisInterface(intSN, intV);
			if (fits) {
				const char* fn = infos[SKIRMISH_AI_PROPERTY_FILE_NAME];
				const char* sn = infos[SKIRMISH_AI_PROPERTY_SHORT_NAME];
				const char* v = infos[SKIRMISH_AI_PROPERTY_VERSION];
				//logOutput.Print("FFFFFF AI (FN/SN/V): %s %s %s\n", fn, sn, v);
/*
std::string msg("");
char buffer[128];
sprintf(buffer, "%s %s %s\n", fn, sn, v);
msg.append(buffer);
reportError(msg);
*/
				if (strlen(fn) == 0 || strlen(sn) == 0 || strlen(v) == 0) {
					//logOutput.Print("FFFFFF something has 0 length -> not added\n");
					// not all needed values are specifyed in AIInfo.lua
				} else {
					SSAISpecifyer skirmishAISpecifyer = {sn, v};
					skirmishAISpecifyer = copySSAISpecifyer(&skirmishAISpecifyer);
					//logOutput.Print("FFFFFF AI specifyer (SN/V): %s %s\n", skirmishAISpecifyer.shortName, skirmishAISpecifyer.version);
					//int pos = mySkirmishAISpecifyers.size();
					mySkirmishAISpecifyers.push_back(skirmishAISpecifyer);
					mySkirmishAIFileNames[skirmishAISpecifyer] = fn;
					//mySkirmishAIFileNames[mySkirmishAISpecifyers.at(pos)] = fn;
				}
			}
		}
	}
	
	
/*
	SSAISpecifyer a = {"A", "0.1"};
	SSAISpecifyer b = {"B", "0.2"};
	SSAISpecifyer c = {"B", "0.2"};
	mySkirmishAIFileNames[a] = "A";
	mySkirmishAIFileNames[b] = "B";
	mySkirmishAIFileNames[c] = "C";
*/
/*
	equalTheyAre = SSAISpecifyer_Comparator()(a, b);
	logOutput.Print("FFFFFF specs are equal? (N): %i\n", equalTheyAre);
	equalTheyAre = SSAISpecifyer_Comparator()(b, c);
	logOutput.Print("FFFFFF specs are equal? (Y): %i\n", equalTheyAre);
*/
	
/*
	logOutput.Print("FFFFFF num filenames and specifyers: %i %i\n", mySkirmishAIFileNames.size(), mySkirmishAISpecifyers.size());
	T_skirmishAIFileNames::const_iterator fn;
	for (fn=mySkirmishAIFileNames.begin(); fn!=mySkirmishAIFileNames.end(); fn++) {
		logOutput.Print("FFFFFF shortName version filename: %s %s %s\n", fn->first.shortName, fn->first.version, fn->second.c_str());
	}
	bool equalTheyAre = SSAISpecifyer_Comparator()(mySkirmishAISpecifyers.at(0), mySkirmishAISpecifyers.at(1));
	logOutput.Print("FFFFFF specs are equal?: %i\n", equalTheyAre);
	std::vector<SSAISpecifyer>::const_iterator spec;
	for (spec=mySkirmishAISpecifyers.begin(); spec!=mySkirmishAISpecifyers.end(); spec++) {
		logOutput.Print("FFFFFF spec: shortName version: %s %s\n", spec->shortName, spec->version);
	}
*/
	
	
	
	// look for Group AI library files
/*
	std::vector<std::string> groupAILibFiles =
			FindFiles(GROUP_AI_IMPLS_DIR,
			std::string("*.") + SharedLib::GetLibExtension());
	
	// initialize the group AI specifyers
	for (libFile=groupAILibFiles.begin(); libFile!=groupAILibFiles.end(); libFile++) { // group IAs
		
		std::string fileName = boost::filesystem::basename(*libFile);
		
		// load the interface
		SGAILibrary groupAILib;
		SharedLib* sharedLib = LoadGroupAILib(*libFile, &groupAILib);
		if (sharedLib == NULL) {
			reportError(std::string("Failed to load group AI shared library: ") + (*libFile));
		}
		
		SGAISpecifyer groupAISpecifyer = extractSpecifyer(groupAILib);
		groupAISpecifyer = copySGAISpecifyer(&groupAISpecifyer);
		myGroupAISpecifyers.push_back(groupAISpecifyer);
		
		delete sharedLib;
	}
*/
	std::vector<std::string> groupAIDataDirs = FindDirs(GROUP_AI_DATA_DIR);
	for (std::vector<std::string>::iterator dir = groupAIDataDirs.begin(); dir != groupAIDataDirs.end(); ++dir) {
		const std::string& possibleDataDir = *dir;
		boost::filesystem::path fullPath(possibleDataDir + "/AIInfo.lua");
		if (boost::filesystem::exists(fullPath)) { // group AI info is available
			// parse the ai info and extract specifyer and filename
			InfoItem tmpInfos[MAX_INFOS];
			unsigned int num = ParseInfosRawFileSystem(fullPath.string().c_str(), tmpInfos, MAX_INFOS);
			std::map<std::string,const char*> infos;
			for (unsigned int i=0; i < num; ++i) {
				infos[std::string(tmpInfos[i].key)] = tmpInfos[i].value;
			}
	
			const char* intSN = infos[GROUP_AI_PROPERTY_INTERFACE_SHORT_NAME];
			const char* intV = infos[GROUP_AI_PROPERTY_INTERFACE_VERSION];
			bool fits = FitsThisInterface(intSN, intV);
			if (fits) {
				const char* fn = infos[GROUP_AI_PROPERTY_FILE_NAME];
				const char* sn = infos[GROUP_AI_PROPERTY_SHORT_NAME];
				const char* v = infos[GROUP_AI_PROPERTY_VERSION];
				if (strlen(fn) == 0 || strlen(sn) == 0 || strlen(v) == 0) {
					// not all needed values are specifyed in AIInfo.lua
				} else {
					SGAISpecifyer groupAISpecifyer = {sn, v};
					myGroupAIFileNames[groupAISpecifyer] = fn;
					myGroupAISpecifyers.push_back(groupAISpecifyer);
				}
			}
		}
	}
}

int CInterface::GetInfos(InfoItem infos[], int max) {
	
	int i = 0;
	
	if (myInfos.empty()) {
		InfoItem ii_0 = {AI_INTERFACE_PROPERTY_SHORT_NAME, "C", NULL}; myInfos.push_back(ii_0);
		InfoItem ii_1 = {AI_INTERFACE_PROPERTY_VERSION, "0.1", NULL}; myInfos.push_back(ii_1);
		InfoItem ii_2 = {AI_INTERFACE_PROPERTY_NAME, "default C & C++ (legacy and new)", NULL}; myInfos.push_back(ii_2);
		InfoItem ii_3 = {AI_INTERFACE_PROPERTY_DESCRIPTION, "This AI Interface library is needed for Skirmish and Group AIs written in C or C++.", NULL}; myInfos.push_back(ii_3);
		InfoItem ii_4 = {AI_INTERFACE_PROPERTY_URL, "http://spring.clan-sy.com/wiki/AIInterface:C", NULL}; myInfos.push_back(ii_4);
		InfoItem ii_5 = {AI_INTERFACE_PROPERTY_SUPPORTED_LANGUAGES, "C, C++", NULL}; myInfos.push_back(ii_5);
	}
	
/*
	std::map<std::string, std::string>::const_iterator prop;
	for (prop=myProperties.begin(); prop!=myProperties.end() && i < max; prop++) {
		properties[i][0] = prop->first.c_str();
		properties[i][1] = prop->second.c_str();
		i++;
	}
*/
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


int CInterface::GetSkirmishAISpecifyers(SSAISpecifyer* sAISpecifyers, int max) const {
	
	int i = 0;
	
	std::vector<SSAISpecifyer>::const_iterator speci;
	for (speci=mySkirmishAISpecifyers.begin(); speci!=mySkirmishAISpecifyers.end() && i < max; speci++) {
		sAISpecifyers[i] = *speci;
		i++;
	}
	
	return i;
}
const SSAILibrary* CInterface::LoadSkirmishAILibrary(const SSAISpecifyer* const sAISpecifyer) {
	
	SSAILibrary* ai;
	
	T_skirmishAIs::iterator skirmishAI;
	skirmishAI = myLoadedSkirmishAIs.find(*sAISpecifyer);
	if (skirmishAI == myLoadedSkirmishAIs.end()) {
		
		ai = new SSAILibrary;
		SharedLib* lib = Load(sAISpecifyer, ai);
		
		myLoadedSkirmishAIs[*sAISpecifyer] = ai;
		myLoadedSkirmishAILibs[*sAISpecifyer] = lib;
	} else {
		ai = skirmishAI->second;
	}
	
	return ai;
}
int CInterface::UnloadSkirmishAILibrary(const SSAISpecifyer* const sAISpecifyer) {
	
	T_skirmishAIs::iterator skirmishAI = myLoadedSkirmishAIs.find(*sAISpecifyer);
	T_skirmishAILibs::iterator skirmishAILib = myLoadedSkirmishAILibs.find(*sAISpecifyer);
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



int CInterface::GetGroupAISpecifyers(SGAISpecifyer* gAISpecifyers, int max) const {
	
	int i = 0;
	
	std::vector<SGAISpecifyer>::const_iterator speci;
	for (speci=myGroupAISpecifyers.begin(); speci!=myGroupAISpecifyers.end() && i < max; speci++) {
		gAISpecifyers[i] = *speci;
		i++;
	}
	
	return i;
}
const SGAILibrary* CInterface::LoadGroupAILibrary(const SGAISpecifyer* const gAISpecifyer) {
	
	SGAILibrary* ai = NULL;
	
	T_groupAIs::iterator groupAI;
	groupAI = myLoadedGroupAIs.find(*gAISpecifyer);
	if (groupAI == myLoadedGroupAIs.end()) {
		//myLoadedGroupAIs[*gAISpecifyer] = Load(gAISpecifyer);
		ai = new SGAILibrary;
		SharedLib* lib = Load(gAISpecifyer, ai);
		myLoadedGroupAIs[*gAISpecifyer] = ai;
		myLoadedGroupAILibs[*gAISpecifyer] = lib;
	} else {
		ai = groupAI->second;
	}
	
	return ai;
}
int CInterface::UnloadGroupAILibrary(const SGAISpecifyer* const gAISpecifyer) {
	
	T_groupAIs::iterator groupAI = myLoadedGroupAIs.find(*gAISpecifyer);
	T_groupAILibs::iterator groupAILib = myLoadedGroupAILibs.find(*gAISpecifyer);
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




SharedLib* CInterface::Load(const SSAISpecifyer* const sAISpecifyer, SSAILibrary* skirmishAILibrary) {
	return LoadSkirmishAILib(GenerateLibFilePath(*sAISpecifyer), skirmishAILibrary);
}
SharedLib* CInterface::LoadSkirmishAILib(const std::string& libFilePath, SSAILibrary* skirmishAILibrary) {
	
	//reportError(std::string("CInterface::LoadSkirmishAILib libFilePath: ") + libFilePath);
	SharedLib* sharedLib = SharedLib::Instantiate(libFilePath);
	
	if (sharedLib == NULL) {
		reportError(std::string("Failed loading shared library: ") + libFilePath);
	}
	
	// initialize the AI library
	std::string funcName;
	
	funcName = "getInfos";
    skirmishAILibrary->getInfos = (int (CALLING_CONV *)(InfoItem[], int)) sharedLib->FindAddress(funcName.c_str());
    if (skirmishAILibrary->getInfos == NULL) {
		// do nothing: this is permitted, if the AI supplies infos through an AIInfo.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "getOptions";
    skirmishAILibrary->getOptions = (int (CALLING_CONV *)(Option*, int max)) sharedLib->FindAddress(funcName.c_str());
    if (skirmishAILibrary->getOptions == NULL) {
		// do nothing: this is permitted, if the AI supplies options through an AIOptions.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "getLevelOfSupportFor";
    skirmishAILibrary->getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV *)(const char*, int, const char*, const char*)) sharedLib->FindAddress(funcName.c_str());
    if (skirmishAILibrary->getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "init";
    skirmishAILibrary->init = (int (CALLING_CONV *)(int)) sharedLib->FindAddress(funcName.c_str());
    if (skirmishAILibrary->init == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "release";
    skirmishAILibrary->release = (int (CALLING_CONV *)(int)) sharedLib->FindAddress(funcName.c_str());
    if (skirmishAILibrary->release == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "handleEvent";
    skirmishAILibrary->handleEvent = (int (CALLING_CONV *)(int, int, const void*)) sharedLib->FindAddress(funcName.c_str());
    if (skirmishAILibrary->handleEvent == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	return sharedLib;
}


SharedLib* CInterface::Load(const SGAISpecifyer* const gAISpecifyer, SGAILibrary* groupAILibrary) {
	return LoadGroupAILib(GenerateLibFilePath(*gAISpecifyer), groupAILibrary);
}
SharedLib* CInterface::LoadGroupAILib(const std::string& libFilePath, SGAILibrary* groupAILibrary) {
	
	SharedLib* sharedLib = SharedLib::Instantiate(libFilePath);
	
	// initialize the AI library
	std::string funcName;
	
	funcName = "getInfos";
    groupAILibrary->getInfos = (int (CALLING_CONV *)(InfoItem[], int)) sharedLib->FindAddress(funcName.c_str());
    if (groupAILibrary->getInfos == NULL) {
		// do nothing: this is permitted, if the AI supplies infos through an AIInfo.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "getLevelOfSupportFor";
    groupAILibrary->getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV *)(const char*, int, const char*, const char*)) sharedLib->FindAddress(funcName.c_str());
    if (groupAILibrary->getLevelOfSupportFor == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "getOptions";
    groupAILibrary->getOptions = (int (CALLING_CONV *)(Option*, int max)) sharedLib->FindAddress(funcName.c_str());
    if (groupAILibrary->getOptions == NULL) {
		// do nothing: this is permitted, if the AI supplies options through an AIOptions.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "init";
    groupAILibrary->init = (int (CALLING_CONV *)(int, int)) sharedLib->FindAddress(funcName.c_str());
    if (groupAILibrary->init == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "release";
    groupAILibrary->release = (int (CALLING_CONV *)(int, int)) sharedLib->FindAddress(funcName.c_str());
    if (groupAILibrary->release == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
    }
	
	funcName = "handleEvent";
    groupAILibrary->handleEvent = (int (CALLING_CONV *)(int, int, int, const void*)) sharedLib->FindAddress(funcName.c_str());
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
	handleerror(NULL, msg.c_str(), "C AI Interface Error", MBF_OK | MBF_EXCL);
}


std::string CInterface::GenerateLibFilePath(const SSAISpecifyer& sAISpecifyer) {
	
/*
	std::string libFileName = std::string(sAISpecifyer.shortName) // eg. RAI
			.append("-")
			.append(sAISpecifyer.version); // eg. 0.600
#ifndef _WIN32
	libFileName = "lib" + libFileName;
#endif
	return GenerateLibFilePath(std::string(PATH_TO_SPRING_HOME) +
			SKIRMISH_AI_IMPLS_DIR, libFileName);
*/
	
/*
	std::string pattern(".*");
	pattern.append(sAISpecifyer.shortName).append(".*");
	pattern.append(sAISpecifyer.version).append(".*");
	pattern.append(SharedLib::GetLibExtension());
	
	return FindFilesRegex(PATH_TO_SPRING_HOME""SKIRMISH_AI_IMPLS_DIR, pattern).at(0);
*/
	static const std::string path = std::string(PATH_TO_SPRING_HOME""SKIRMISH_AI_IMPLS_DIR"/");
	
	T_skirmishAIFileNames::const_iterator fileName = mySkirmishAIFileNames.find(sAISpecifyer);
	if (fileName == mySkirmishAIFileNames.end()) {
		reportError(std::string("Missing Skirmish-AI file name for ") + sAISpecifyer.shortName + " " + sAISpecifyer.version);
	}
	
	std::string libFileName = fileName->second; // eg. RAI-0.600
#ifndef _WIN32
	libFileName = "lib" + libFileName; // eg. libRAI-0.600
#endif
	
	libFileName.append("."); // eg. libRAI-0.600.
	libFileName.append(SharedLib::GetLibExtension()); // eg. libRAI-0.600.so
	
	return path + libFileName;
}

std::string CInterface::GenerateLibFilePath(const SGAISpecifyer& gAISpecifyer) {
	
	static const std::string path = std::string(PATH_TO_SPRING_HOME""GROUP_AI_IMPLS_DIR"/");
	
	T_groupAIFileNames::const_iterator fileName = myGroupAIFileNames.find(gAISpecifyer);
	if (fileName == myGroupAIFileNames.end()) {
		reportError(std::string("Missing Group-AI file name for ") + gAISpecifyer.shortName + " " + gAISpecifyer.version);
	}
	
	std::string libFileName = fileName->second; // eg. RAI-0.600
#ifndef _WIN32
	libFileName = "lib" + libFileName; // eg. libRAI-0.600
#endif
	
	libFileName.append("."); // eg. libRAI-0.600.
	libFileName.append(SharedLib::GetLibExtension()); // eg. libRAI-0.600.so
	
	return path + libFileName;
}

/*
std::string CInterface::GenerateLibFilePath(const std::string& basePath, const std::string& libFileName) {
	return std::string(basePath) // eg AI/Skirmish/impls
			.append("/")
			.append(libFileName) // eg. RAI-0.600
			.append(".")
			.append(SharedLib::GetLibExtension()); // eg. dll
}
*/

SSAISpecifyer CInterface::extractSpecifyer(const SSAILibrary& skirmishAILib) {
	
	SSAISpecifyer skirmishAISpecifyer;
	
	InfoItem infos[MAX_INFOS];
	int numInfos =  skirmishAILib.getInfos(infos, MAX_INFOS);

	std::string spsn = std::string(SKIRMISH_AI_PROPERTY_SHORT_NAME);
	std::string spv = std::string(SKIRMISH_AI_PROPERTY_VERSION);
	for (int i=0; i < numInfos; ++i) {
		std::string key = std::string(infos[i].key);
		if (key == spsn) {
			skirmishAISpecifyer.shortName = infos[i].value;
		} else if (key == spv) {
			skirmishAISpecifyer.version = infos[i].value;
		}
	}
	
	return skirmishAISpecifyer;
}

SGAISpecifyer CInterface::extractSpecifyer(const SGAILibrary& groupAILib) {
	
	SGAISpecifyer groupAISpecifyer;
	
	InfoItem infos[MAX_INFOS];
	int numInfos =  groupAILib.getInfos(infos, MAX_INFOS);

	std::string spsn = std::string(GROUP_AI_PROPERTY_SHORT_NAME);
	std::string spv = std::string(GROUP_AI_PROPERTY_VERSION);
	for (int i=0; i < numInfos; ++i) {
		std::string key = std::string(infos[i].key);
		if (key == spsn) {
			groupAISpecifyer.shortName = infos[i].value;
		} else if (key == spv) {
			groupAISpecifyer.version = infos[i].value;
		}
	}
	
	return groupAISpecifyer;
}

std::vector<std::string> CInterface::FindDirs(const std::string& path, const std::string& pattern) {
	
	std::vector<std::string> found;

	boost::regex regPattern(pattern, boost::regex::perl|boost::regex::icase);
	boost::match_results<std::string::const_iterator> matchResult;
	if (boost::filesystem::exists(path)) {
		  boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		  for (boost::filesystem::directory_iterator itr(path); itr != end_itr; ++itr) {
			  if (boost::filesystem::is_directory(*itr)
					  && boost::regex_match(itr->string(), matchResult, regPattern, boost::match_default) != 0
					  && matchResult[0].matched) {
				  found.push_back(itr->string());
			  }
		  }
	}
	
	return found;
}

std::vector<std::string> CInterface::FindFiles(const std::string& path, const std::string& fileExtension) {
	
	std::vector<std::string> found;

	if (boost::filesystem::exists(path)) {
		  boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		  for (boost::filesystem::directory_iterator itr(path); itr != end_itr; ++itr) {
			  if (!boost::filesystem::is_directory(*itr)
					  && boost::filesystem::extension(*itr) == fileExtension) {
				  found.push_back(itr->string());
			  }
		  }
	}
	
	return found;
}

/*
bool CInterface::FileExists(const std::string& path, const std::string& fileName) {
	
	boost::filesystem::path fullPath(path, fileName);
	
	return boost::filesystem::exists(fullPath);
}
*/

std::vector<std::string> CInterface::FindFilesRegex(const std::string& path, const std::string& pattern) {
	
	std::vector<std::string> found;

	boost::regex regPattern(pattern, boost::regex::perl|boost::regex::icase);
	boost::match_results<std::string::const_iterator> matchResult;
	if (boost::filesystem::exists(path)) {
		  boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		  for (boost::filesystem::directory_iterator itr(path); itr != end_itr; ++itr) {
			  if (!boost::filesystem::is_directory(*itr)
					  && boost::regex_match(itr->string(), matchResult, regPattern, boost::match_default) != 0
					  && matchResult[0].matched) {
				  found.push_back(itr->string());
			  }
		  }
	}
	
	return found;
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
