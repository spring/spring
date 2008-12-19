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
#include "Util.h"

#include <sys/stat.h>	// used for check if a file exists
#ifdef	WIN32
#include <direct.h>	// mkdir()
#else	// WIN32
#include <sys/stat.h>	// mkdir()
#include <sys/types.h>	// mkdir()
#endif	// WIN32

//#define MY_SHORT_NAME "C"
//#define MY_VERSION "0.1"
//#define MY_NAME "C & C++ AI Interface"
//
//#define MAX_INFOS 128

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

	// "C:/Games/spring/AI/Interfaces/C/0.1"
	myDataDirVersioned = util_getDataDirVersioned();
	if (!FileExists(myDataDirVersioned)) {
		MakeDirRecursive(myDataDirVersioned);
	}

	// "C:/Games/spring/AI/Interfaces/C"
	myDataDirUnversioned = util_getDataDirUnversioned();
	if (!FileExists(myDataDirUnversioned)) {
		MakeDirRecursive(myDataDirUnversioned);
	}

	std::string logFileName = myDataDirVersioned + PS + "log.txt";
	bool timeStamps = true;
	bool fineLogging = false;
#if defined DEBUG
	fineLogging = true;
#endif // defined DEBUG
	simpleLog_init(logFileName.c_str(), timeStamps, fineLogging);

	const char* myShortName = util_getMyInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* myVersion = util_getMyInfo(AI_INTERFACE_PROPERTY_VERSION);

	simpleLog_log("This is the log-file of the %s version %s", myShortName,
			myVersion);
	simpleLog_log("Using data-directory (version specific): %s",
			myDataDirVersioned.c_str());
	simpleLog_log("Using data-directory (version-less): %s",
			myDataDirUnversioned.c_str());
	simpleLog_log("Using log file: %s", logFileName.c_str());
}

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

//	prop = info->second.find(SKIRMISH_AI_PROPERTY_FILE_NAME);
//	if (prop == info->second.end()) {
//		reportError(std::string("Missing Skirmish-AI file name for ")
//				+ spec.shortName + " " + spec.version);
//	}
//	const std::string& fileName(prop->second);
	const std::string& fileName(spec.shortName);

	std::string libFileName(fileName); // eg. RAI
	#ifndef _WIN32
		libFileName = "lib" + libFileName; // eg. libRAI
	#endif

	// eg. libRAI-0.600.so
	libFileName = libFileName + "." + SharedLib::GetLibExtension();

	//return FindFile(dataDir + PS + libFileName);
	return dataDir + PS + libFileName;
}

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

bool CInterface::FitsThisInterface(const std::string& requestedShortName,
		const std::string& requestedVersion) {

	std::string myShortName = local_getValueByKey(myInfo, AI_INTERFACE_PROPERTY_SHORT_NAME);
	std::string myVersion = local_getValueByKey(myInfo, AI_INTERFACE_PROPERTY_VERSION);

	bool shortNameFits = (requestedShortName == myShortName);
	bool versionFits = (requestedVersion == myVersion);

	return shortNameFits && versionFits;
}
