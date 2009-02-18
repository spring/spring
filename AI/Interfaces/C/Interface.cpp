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

#include "CUtils/Util.h"
#include "CUtils/SimpleLog.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SSAILibrary.h"
#include "ExternalAI/Interface/SStaticGlobalData.h"

#include "System/Platform/SharedLib.h"
#include "System/Util.h"

#include <sys/stat.h>   // used for check if a file exists
#ifdef WIN32
#include <direct.h>     // mkdir()
#else // WIN32
#include <sys/stat.h>   // mkdir()
#include <sys/types.h>  // mkdir()
#endif // WIN32

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

	// "C:/Games/spring/AI/Interfaces/C/0.1"
	const char* dd_versioned_rw = util_dataDirs_allocDir("", true);
	if (dd_versioned_rw != NULL) {
		util_makeDir(dd_versioned_rw, true);
	} else {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
			"Failed retrieving read-write data-dir path");
	}

// 	// "C:/Games/spring/AI/Interfaces/C"
// 	const char* dd_unversioned_rw = util_getDataDir(false, true);
// 	if (dd_unversioned_rw != NULL) {
// 		util_makeDir(dd_unversioned_rw, true);
// 	}

	char* logFileName = util_allocStrCatFSPath(3,
			dd_versioned_rw, "log", "interface-log.txt");
	bool timeStamps = true;
	int logLevel = SIMPLELOG_LEVEL_ERROR;
#if defined DEBUG
	logLevel = SIMPLELOG_LEVEL_FINE;
#endif // defined DEBUG
	simpleLog_init(logFileName, timeStamps, logLevel);

	const char* myShortName = util_getMyInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* myVersion = util_getMyInfo(AI_INTERFACE_PROPERTY_VERSION);

	simpleLog_log("This is the log-file of the %s version %s", myShortName,
			myVersion);
	simpleLog_log("Using read-write data-directory (version specific): %s",
			dd_versioned_rw);
// 	simpleLog_log("Using read-write data-directory (version-less): %s",
// 			dd_unversioned_rw);
	simpleLog_log("Using log file: %s", logFileName);

	free(logFileName);
	logFileName = NULL;
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
	simpleLog_logL(SIMPLELOG_LEVEL_ERROR, msg.c_str());
}


std::string CInterface::FindLibFile(const SSkirmishAISpecifier& spec) {

	// fetch the data-dir and file-name from the info about the AI to load,
	// which was supplied to us by the engine
	T_skirmishAIInfos::const_iterator info =
			mySkirmishAIInfos.find(spec);
	if (info == mySkirmishAIInfos.end()) {
		reportError(std::string("Missing Skirmish AI info for ")
				+ spec.shortName + " " + spec.version);
	}

	std::map<std::string, std::string>::const_iterator prop =
			info->second.find(SKIRMISH_AI_PROPERTY_DATA_DIR);
	if (prop == info->second.end()) {
		reportError(std::string("Missing Skirmish AI data-dir for ")
				+ spec.shortName + " " + spec.version);
	}
	const std::string& dataDir(prop->second);

	const std::string& fileName("SkirmishAI");

	std::string libFileName(fileName); // "SkirmishAI"
	#ifndef _WIN32
		libFileName = "lib" + libFileName; // "libSkirmishAI"
	#endif

	// eg. "libSkirmishAI.so"
	libFileName = libFileName + "." + SharedLib::GetLibExtension();

	return util_allocStrCatFSPath(2, dataDir.c_str(), libFileName.c_str());
}

bool CInterface::FitsThisInterface(const std::string& requestedShortName,
		const std::string& requestedVersion) {

	std::string myShortName = local_getValueByKey(myInfo, AI_INTERFACE_PROPERTY_SHORT_NAME);
	std::string myVersion = local_getValueByKey(myInfo, AI_INTERFACE_PROPERTY_VERSION);

	bool shortNameFits = (requestedShortName == myShortName);
	bool versionFits = (requestedVersion == myVersion);

	return shortNameFits && versionFits;
}
