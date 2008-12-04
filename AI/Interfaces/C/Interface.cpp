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

#include "Log.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SSAILibrary.h"
#include "ExternalAI/Interface/SStaticGlobalData.h"

#include "System/Platform/SharedLib.h"
#include "System/Util.h"

#include <sys/stat.h>	// used for check if a file exists
#ifdef	WIN32
#include <direct.h>	// mkdir()
#else	// WIN32
#include <sys/stat.h>	// mkdir()
#include <sys/types.h>	// mkdir()
#endif	// WIN32

#define MY_SHORT_NAME "C"
#define MY_VERSION "0.1"
#define MY_NAME "C & C++ AI Interface"

#define MAX_INFOS 128

/*
std::string CInterface::relSkirmishAIImplsDir =
		std::string(SKIRMISH_AI_IMPLS_DIR) + PS;
std::string CInterface::relGroupAIImplsDir =
		std::string(GROUP_AI_IMPLS_DIR) + PS;
*/
static std::string local_getValueByKey(
		const std::map<std::string, std::string>& map, std::string key) {

	std::map<std::string, std::string>::const_iterator it = map.find(key);
	if (it != map.end()) {
		return it->second;
	} else {
		return "";
	}
}

CInterface::CInterface(const std::map<std::string, std::string>& myInfo,
		const SStaticGlobalData* staticGlobalData)
		: myInfo(myInfo), staticGlobalData(staticGlobalData) {

	for (unsigned int i=0; i < staticGlobalData->numDataDirs; ++i) {
		springDataDirs.push_back(staticGlobalData->dataDirs[i]);
	}

	// example: "AI/Interfaces/C"
	std::string myDataDirRelative =
			std::string(AI_INTERFACES_DATA_DIR) + PS + MY_SHORT_NAME;
	// example: "AI/Interfaces/C/0.1"
	std::string myDataDirVersRelative = myDataDirRelative + PS + MY_VERSION;

	// "C:/Games/spring/AI/Interfaces/C"
	myDataDir = FindDir(myDataDirRelative, true, true);
	if (!FileExists(myDataDir)) {
		MakeDirRecursive(myDataDir);
	}
	// "C:/Games/spring/AI/Interfaces/C/0.1"
	myDataDirVers = FindDir(myDataDirVersRelative, true, true);
	if (!FileExists(myDataDirVers)) {
		MakeDirRecursive(myDataDirVers);
	}

	std::string logFileName = myDataDirVers + PS + "log.txt";
	simpleLog_init(logFileName.c_str(), true);

	simpleLog_log("This is the log-file of the %s version %s", MY_NAME,
			MY_VERSION);
	simpleLog_log("Using data-directory (version-less): %s",
			myDataDir.c_str());
	simpleLog_log("Using data-directory (version specific): %s",
			myDataDirVers.c_str());
	simpleLog_log("Using log file: %s", logFileName.c_str());
}

//unsigned int CInterface::GetInfo(InfoItem info[], unsigned int maxInfoItems) {
//
//	unsigned int i = 0;
//
//	if (myInfo.empty()) {
//		InfoItem ii_0 = {AI_INTERFACE_PROPERTY_SHORT_NAME, MY_SHORT_NAME, NULL}; myInfo.push_back(ii_0);
//		InfoItem ii_1 = {AI_INTERFACE_PROPERTY_VERSION, MY_VERSION, NULL}; myInfo.push_back(ii_1);
//		InfoItem ii_2 = {AI_INTERFACE_PROPERTY_NAME, MY_NAME, NULL}; myInfo.push_back(ii_2);
//		InfoItem ii_3 = {AI_INTERFACE_PROPERTY_DESCRIPTION, "This AI Interface library is needed for Skirmish and Group AIs written in C or C++. It is neded for legacy C++ AIs as well.", NULL}; myInfo.push_back(ii_3);
//		InfoItem ii_4 = {AI_INTERFACE_PROPERTY_URL, "http://spring.clan-sy.com/wiki/AIInterface:C", NULL}; myInfo.push_back(ii_4);
//		InfoItem ii_5 = {AI_INTERFACE_PROPERTY_SUPPORTED_LANGUAGES, "C, C++", NULL}; myInfo.push_back(ii_5);
//	}
//
//	std::vector<InfoItem>::const_iterator inf;
//	for (inf=myInfo.begin(); inf!=myInfo.end() && i < maxInfoItems; inf++) {
//		info[i] = *inf;
//		i++;
//	}
//
//	return i;
//}

LevelOfSupport CInterface::GetLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
	return LOS_Working;
}

SSkirmishAISpecifier CInterface::ExtractSpecifier(
		const std::map<std::string, std::string>& infoMap) {

	const char* sn = infoMap.find(SKIRMISH_AI_PROPERTY_SHORT_NAME)->second.c_str();
	const char* v = infoMap.find(SKIRMISH_AI_PROPERTY_VERSION)->second.c_str();

	SSkirmishAISpecifier spec = {sn, v};

	return spec;
}
//SGAISpecifier extractSGAISpecifier(
//		const std::map<std::string, InfoItem>& infoMap) {
//
//	const char* sn = infoMap.find(GROUP_AI_PROPERTY_SHORT_NAME)->second.value;
//	const char* v = infoMap.find(GROUP_AI_PROPERTY_VERSION)->second.value;
//
//	SGAISpecifier specifier = {sn, v};
//
//	return specifier;
//}

const SSAILibrary* CInterface::LoadSkirmishAILibrary(
		const std::map<std::string, std::string>& infoMap) {

	SSAILibrary* ai;

	SSkirmishAISpecifier spec = ExtractSpecifier(infoMap);

	mySkirmishAISpecifiers.insert(spec);
	mySkirmishAIInfos[spec] = infoMap;

	T_skirmishAIs::iterator skirmishAI;
	skirmishAI = myLoadedSkirmishAIs.find(spec);
	if (skirmishAI == myLoadedSkirmishAIs.end()) {
		ai = new SSAILibrary;
		SharedLib* lib = Load(spec, ai);
		myLoadedSkirmishAIs[spec] = ai;
		myLoadedSkirmishAILibs[spec] = lib;
	} else {
		ai = skirmishAI->second;
	}

	return ai;
}
int CInterface::UnloadSkirmishAILibrary(
		const std::map<std::string, std::string>& infoMap) {

	SSkirmishAISpecifier spec = ExtractSpecifier(infoMap);

	T_skirmishAIs::iterator skirmishAI =
			myLoadedSkirmishAIs.find(spec);
	T_skirmishAILibs::iterator skirmishAILib =
			myLoadedSkirmishAILibs.find(spec);
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
		UnloadSkirmishAILibrary(
				mySkirmishAIInfos[myLoadedSkirmishAIs.begin()->first]);
	}

	return 0;
}



//const SGAILibrary* CInterface::LoadGroupAILibrary(const struct InfoItem info[],
//		unsigned int numInfoItems) {
//
//	SGAILibrary* ai = NULL;
//
//	std::map<std::string, InfoItem> infoMap;
//	copyToInfoMap(infoMap, info, numInfoItems);
//	SGAISpecifier gAISpecifier = extractSGAISpecifier(infoMap);
//	myGroupAISpecifiers.push_back(gAISpecifier);
//	myGroupAIInfos[gAISpecifier] = infoMap;
//
//	T_groupAIs::iterator groupAI;
//	groupAI = myLoadedGroupAIs.find(gAISpecifier);
//	if (groupAI == myLoadedGroupAIs.end()) {
//		ai = new SGAILibrary;
//		SharedLib* lib = Load(&gAISpecifier, ai);
//		myLoadedGroupAIs[gAISpecifier] = ai;
//		myLoadedGroupAILibs[gAISpecifier] = lib;
//	} else {
//		ai = groupAI->second;
//	}
//
//	return ai;
//}
//int CInterface::UnloadGroupAILibrary(const SGAISpecifier* const gAISpecifier) {
//
//	T_groupAIs::iterator groupAI = myLoadedGroupAIs.find(*gAISpecifier);
//	T_groupAILibs::iterator groupAILib =
//			myLoadedGroupAILibs.find(*gAISpecifier);
//	if (groupAI == myLoadedGroupAIs.end()) {
//		// to unload AI is not loaded -> no problem, do nothing
//	} else {
//		delete groupAI->second;
//		myLoadedGroupAIs.erase(groupAI);
//		delete groupAILib->second;
//		myLoadedGroupAILibs.erase(groupAILib);
//	}
//
//	return 0;
//}
//int CInterface::UnloadAllGroupAILibraries() {
//
//	while (myLoadedGroupAIs.size() > 0) {
//		UnloadGroupAILibrary(&(myLoadedGroupAIs.begin()->first));
//	}
//
//	return 0;
//}

// private functions following

SharedLib* CInterface::Load(const SSkirmishAISpecifier& spec, SSAILibrary* skirmishAILibrary) {
	return LoadSkirmishAILib(FindLibFile(spec), skirmishAILibrary);
}
SharedLib* CInterface::LoadSkirmishAILib(const std::string& libFilePath,
		SSAILibrary* skirmishAILibrary) {

	SharedLib* sharedLib = SharedLib::Instantiate(libFilePath);

	if (sharedLib == NULL) {
		reportError(std::string("Failed loading shared library: ") + libFilePath);
	}

	// initialize the AI library
	std::string funcName;

/*
	funcName = "getInfo";
	skirmishAILibrary->getInfo = (unsigned int (CALLING_CONV_FUNC_POINTER *)(int teamId, struct InfoItem[], unsigned int)) sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->getInfo == NULL) {
		// do nothing: this is permitted, if the AI supplies info through an AIInfo.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
*/
//	skirmishAILibrary->getInfo = NULL;

/*
	funcName = "getOptions";
	skirmishAILibrary->getOptions = (unsigned int (CALLING_CONV_FUNC_POINTER *)(int teamId, Option[], unsigned int max)) sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->getOptions == NULL) {
		// do nothing: this is permitted, if the AI supplies options through an AIOptions.lua file
		//reportInterfaceFunctionError(libFilePath, funcName);
	}
*/
//	skirmishAILibrary->getOptions = NULL;

	funcName = "getLevelOfSupportFor";
	skirmishAILibrary->getLevelOfSupportFor
			= (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(int teamId,
			const char*, int, const char*, const char*))
			sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
	}

	funcName = "init";
	skirmishAILibrary->init
			= (int (CALLING_CONV_FUNC_POINTER *)(int teamId,
			unsigned int infoSize,
			const char** infoKeys, const char** infoValues,
			unsigned int optionsSize,
			const char** optionsKeys, const char** optionsValues))
			sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->init == NULL) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_INIT instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}

	funcName = "release";
	skirmishAILibrary->release
			= (int (CALLING_CONV_FUNC_POINTER *)(int teamId))
			sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->release == NULL) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_RELEASE instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}

	funcName = "handleEvent";
	skirmishAILibrary->handleEvent
			= (int (CALLING_CONV_FUNC_POINTER *)(int teamId,
			int, const void*))
			sharedLib->FindAddress(funcName.c_str());
	if (skirmishAILibrary->handleEvent == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
	}

	return sharedLib;
}


//SharedLib* CInterface::Load(const SGAISpecifier* const gAISpecifier,
//		SGAILibrary* groupAILibrary) {
//	return LoadGroupAILib(FindLibFile(*gAISpecifier), groupAILibrary);
//}
//SharedLib* CInterface::LoadGroupAILib(const std::string& libFilePath,
//		SGAILibrary* groupAILibrary) {
//
//	SharedLib* sharedLib = SharedLib::Instantiate(libFilePath);
//
//	if (sharedLib == NULL) {
//		reportError(std::string("Failed loading shared library: ") + libFilePath);
//	}
//
//	// initialize the AI library
//	std::string funcName;
//
///*
//	funcName = "getInfo";
//	groupAILibrary->getInfo = (unsigned int (CALLING_CONV_FUNC_POINTER *)(InfoItem[], unsigned int max)) sharedLib->FindAddress(funcName.c_str());
//	if (groupAILibrary->getInfo == NULL) {
//		// do nothing: this is permitted, if the AI supplies info through an AIInfo.lua file
//		//reportInterfaceFunctionError(libFilePath, funcName);
//	}
//*/
//	groupAILibrary->getInfo = NULL;
//
///*
//	funcName = "getOptions";
//	groupAILibrary->getOptions = (unsigned int (CALLING_CONV_FUNC_POINTER *)(Option[], unsigned int max)) sharedLib->FindAddress(funcName.c_str());
//	if (groupAILibrary->getOptions == NULL) {
//		// do nothing: this is permitted, if the AI supplies options through an AIOptions.lua file
//		//reportInterfaceFunctionError(libFilePath, funcName);
//	}
//*/
//	groupAILibrary->getOptions = NULL;
//
//	funcName = "getLevelOfSupportFor";
//	groupAILibrary->getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(int teamId, int groupId, const char*, int, const char*, const char*)) sharedLib->FindAddress(funcName.c_str());
//	if (groupAILibrary->getLevelOfSupportFor == NULL) {
//		// do nothing: it is permitted that an AI does not export this function
//		//reportInterfaceFunctionError(libFilePath, funcName);
//	}
//
//	funcName = "init";
//	groupAILibrary->init = (int (CALLING_CONV_FUNC_POINTER *)(int teamId, int groupId, const struct InfoItem info[], unsigned int maxInfoItems)) sharedLib->FindAddress(funcName.c_str());
//	if (groupAILibrary->init == NULL) {
//		// do nothing: it is permitted that an AI does not export this function,
//		// as it can still use EVENT_INIT instead
//		//reportInterfaceFunctionError(libFilePath, funcName);
//	}
//
//	funcName = "release";
//	groupAILibrary->release = (int (CALLING_CONV_FUNC_POINTER *)(int teamId, int groupId)) sharedLib->FindAddress(funcName.c_str());
//	if (groupAILibrary->release == NULL) {
//		// do nothing: it is permitted that an AI does not export this function,
//		// as it can still use EVENT_RELEASE instead
//		//reportInterfaceFunctionError(libFilePath, funcName);
//	}
//
//	funcName = "handleEvent";
//	groupAILibrary->handleEvent = (int (CALLING_CONV_FUNC_POINTER *)(int teamId, int groupId, int topic, const void* data)) sharedLib->FindAddress(funcName.c_str());
//	if (groupAILibrary->handleEvent == NULL) {
//		reportInterfaceFunctionError(libFilePath, funcName);
//	}
//
//	return sharedLib;
//}


void CInterface::reportInterfaceFunctionError(const std::string& libFilePath,
		const std::string& functionName) {

	std::string msg("Failed loading AI Library from file \"");
	msg += libFilePath + "\": no \"" + functionName + "\" function exported";
	reportError(msg);
}

void CInterface::reportError(const std::string& msg) {
	///handleerror(NULL, msg.c_str(), "C AI Interface Error", MBF_OK | MBF_EXCL);
	simpleLog_error(-1, msg.c_str());
}


std::string CInterface::FindLibFile(const SSkirmishAISpecifier& spec) {

	// fetch the data-dir and file-name from the info about the AI to load,
	// which was supplied to us by the engine
	T_skirmishAIInfos::const_iterator info =
			mySkirmishAIInfos.find(spec);
	if (info == mySkirmishAIInfos.end()) {
		reportError(std::string("Missing Skirmish-AI info for ")
				+ spec.shortName + " " + spec.version);
	}

	std::map<std::string, std::string>::const_iterator prop =
			info->second.find(SKIRMISH_AI_PROPERTY_DATA_DIR);
	if (prop == info->second.end()) {
		reportError(std::string("Missing Skirmish-AI data dir for ")
				+ spec.shortName + " " + spec.version);
	}
	const std::string& dataDir(prop->second);

	prop = info->second.find(SKIRMISH_AI_PROPERTY_FILE_NAME);
	if (prop == info->second.end()) {
		reportError(std::string("Missing Skirmish-AI file name for ")
				+ spec.shortName + " " + spec.version);
	}
	const std::string& fileName(prop->second);

	std::string libFileName(fileName); // eg. RAI-0.600
	#ifndef _WIN32
		libFileName = "lib" + libFileName; // eg. libRAI-0.600
	#endif

	// eg. libRAI-0.600.so
	libFileName = libFileName + "." + SharedLib::GetLibExtension();

	//return FindFile(dataDir + PS + libFileName);
	return dataDir + PS + libFileName;
}

//std::string CInterface::FindLibFile(const SGAISpecifier& gAISpecifier) {
//
//	// fetch the data-dir and file-name from the info about the AI to load,
//	// which was supplied to us by the engine
//	T_groupAIInfos::const_iterator info = myGroupAIInfos.find(gAISpecifier);
//	if (info == myGroupAIInfos.end()) {
//		reportError(std::string("Missing Group-AI info for ")
//				+ gAISpecifier.shortName + " " + gAISpecifier.version);
//	}
//
//	std::map<std::string, InfoItem>::const_iterator prop =
//			info->second.find(GROUP_AI_PROPERTY_DATA_DIR);
//	if (prop == info->second.end()) {
//		reportError(std::string("Missing Group-AI data dir for ")
//				+ gAISpecifier.shortName + " " + gAISpecifier.version);
//	}
//	const std::string& dataDir(prop->second.value);
//
//	prop = info->second.find(GROUP_AI_PROPERTY_FILE_NAME);
//	if (prop == info->second.end()) {
//		reportError(std::string("Missing Group-AI file name for ")
//				+ gAISpecifier.shortName + " " + gAISpecifier.version);
//	}
//	const std::string& fileName(prop->second.value);
//
//	std::string libFileName(fileName); // eg. MetalMaker-1.0
//	#ifndef _WIN32
//		libFileName = "lib" + libFileName; // eg. libMetalMaker-1.0
//	#endif
//
//	// eg. libMetalMaker-1.0.so
//	libFileName = libFileName + "." + SharedLib::GetLibExtension();
//
//	//return FindFile(dataDir + PS + libFileName);
//	return dataDir + PS + libFileName;
//}

bool CInterface::FileExists(const std::string& filePath) {

	struct stat fileInfo;
	bool exists;
	int intStat;

	// Attempt to get the file attributes
	intStat = stat(filePath.c_str(), &fileInfo);
	if (intStat == 0) {
		// We were able to get the file attributes
		// so the file obviously exists.
		exists = true;
	} else {
		// We were not able to get the file attributes.
		// This may mean that we don't have permission to
		// access the folder which contains this file. If you
		// need to do that level of checking, lookup the
		// return values of stat which will give you
		// more details on why stat failed.
		exists = false;
	}

	return exists;
}

bool CInterface::MakeDir(const std::string& dirPath) {

	#ifdef	WIN32
	int mkStat = _mkdir(dirPath.c_str());
	if (mkStat == 0) {
		return true;
	} else {
		return false;
	}
	#else	// WIN32
	// with read/write/search permissions for owner and group,
	// and with read/search permissions for others
	int mkStat = mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (mkStat == 0) {
		return true;
	} else {
		return false;
	}
	#endif	// WIN32
}

bool CInterface::MakeDirRecursive(const std::string& dirPath) {

	if (!FileExists(dirPath)) {
		std::string::size_type pos = dirPath.find_last_of("/\\");
		if (pos != std::string::npos) {
			std::string parentDir = dirPath.substr(0, pos);
			bool parentExists = MakeDirRecursive(parentDir);
			if (parentExists) {
				return MakeDir(dirPath);
			}
		}
		return false;
	}

	return true;
}

std::string CInterface::FindFile(const std::string& relativeFilePath) {

	std::string path = relativeFilePath;

	for (unsigned int i=0; i < springDataDirs.size(); ++i) {
		std::string tmpPath = springDataDirs.at(i);
		tmpPath += PS + relativeFilePath;
		if (FileExists(tmpPath)) {
			path = tmpPath;
			break;
		}
	}

	return path;
}
std::string CInterface::FindDir(const std::string& relativeDirPath,
		bool searchOnlyWriteable, bool pretendAvailable) {

	std::string path = relativeDirPath;

	unsigned int numDds = springDataDirs.size();
	if (searchOnlyWriteable && numDds > 1) {
		numDds = 1;
	}

	bool found = false;
	for (unsigned int i=0; i < numDds; ++i) {
		std::string tmpPath = springDataDirs.at(i) + PS + relativeDirPath;
		if (FileExists(tmpPath)) {
			path = tmpPath;
			found = true;
			break;
		}
	}

	if (!found && pretendAvailable && numDds >= 1) {
		path = springDataDirs.at(0) + PS + relativeDirPath;
	}

	return path;
}

/*
SSAISpecifier CInterface::ExtractSpecifier(const SSAILibrary& skirmishAILib) {

	SSAISpecifier skirmishAISpecifier;

	InfoItem info[MAX_INFOS];
	int numInfo =  skirmishAILib.getInfo(info, MAX_INFOS);

	std::string spsn = std::string(SKIRMISH_AI_PROPERTY_SHORT_NAME);
	std::string spv = std::string(SKIRMISH_AI_PROPERTY_VERSION);
	for (int i=0; i < numInfo; ++i) {
		std::string key = std::string(info[i].key);
		if (key == spsn) {
			skirmishAISpecifier.shortName = info[i].value;
		} else if (key == spv) {
			skirmishAISpecifier.version = info[i].value;
		}
	}

	return skirmishAISpecifier;
}
*/

/*
SGAISpecifier CInterface::ExtractSpecifier(const SGAILibrary& groupAILib) {

	SGAISpecifier groupAISpecifier;

	InfoItem info[MAX_INFOS];
	int numInfo =  groupAILib.getInfo(info, MAX_INFOS);

	std::string spsn = std::string(GROUP_AI_PROPERTY_SHORT_NAME);
	std::string spv = std::string(GROUP_AI_PROPERTY_VERSION);
	for (int i=0; i < numInfo; ++i) {
		std::string key = std::string(info[i].key);
		if (key == spsn) {
			groupAISpecifier.shortName = info[i].value;
		} else if (key == spv) {
			groupAISpecifier.version = info[i].value;
		}
	}

	return groupAISpecifier;
}
*/

bool CInterface::FitsThisInterface(const std::string& requestedShortName,
		const std::string& requestedVersion) {

	std::string myShortName = local_getValueByKey(myInfo, AI_INTERFACE_PROPERTY_SHORT_NAME);
	std::string myVersion = local_getValueByKey(myInfo, AI_INTERFACE_PROPERTY_VERSION);

	bool shortNameFits = (requestedShortName == myShortName);
	bool versionFits = (requestedVersion == myVersion);

	return shortNameFits && versionFits;
}
