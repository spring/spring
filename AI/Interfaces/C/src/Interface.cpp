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
#include "CUtils/SharedLibrary.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "ExternalAI/Interface/SAIInterfaceCallback.h"
struct SSkirmishAICallback;

#include "System/Util.h"

CInterface::CInterface(int interfaceId,
		const struct SAIInterfaceCallback* callback)
		: interfaceId(interfaceId), callback(callback) {

	char* logFileName = util_allocStrCatFSPath(2, "log", "interface-log.txt");
	bool timeStamps = true;
	int logLevel = SIMPLELOG_LEVEL_ERROR;
#if defined DEBUG
	logLevel = SIMPLELOG_LEVEL_FINE;
#endif // defined DEBUG
	static const unsigned int logFilePath_sizeMax = 1024;
	char logFilePath[logFilePath_sizeMax];
	// eg: "~/.spring/AI/Interfaces/C/log/interface-log.txt"
	bool ok = callback->DataDirs_locatePath(interfaceId,
			logFilePath, logFilePath_sizeMax,
			logFileName, true, true, false, false);
	if (!ok) {
		simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
			"Failed locating the log file %s.", logFileName);
	}

	simpleLog_init(logFilePath, timeStamps, logLevel, false);

	const char* const myShortName = callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* const myVersion = callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_VERSION);

	simpleLog_log("This is the log-file of the %s version %s",
			myShortName, myVersion);
	simpleLog_log("Using read/write data-directory: %s",
			callback->DataDirs_getWriteableDir(interfaceId));
	simpleLog_log("Using log file: %s", logFileName);

	FREE(logFileName);
}

//LevelOfSupport CInterface::GetLevelOfSupportFor(
//		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
//	return LOS_Working;
//}

// SSkirmishAISpecifier CInterface::ExtractSpecifier(
// 		const std::map<std::string, std::string>& infoMap) {
//
// 	const char* const skirmDD =
// 			callback->SkirmishAIs_Info_getValueByKey(interfaceId,
// 			shortName, version,
// 			SKIRMISH_AI_PROPERTY_DATA_DIR);
// 	const char* sn = callback->AIInterface_Info_getValueByKey)(int interfaceId, const char* const key);
// 	.find(SKIRMISH_AI_PROPERTY_SHORT_NAME)->second.c_str();
// 	const char* v = infoMap.find(SKIRMISH_AI_PROPERTY_VERSION)->second.c_str();
//
// 	SSkirmishAISpecifier spec = {sn, v};
//
// 	return spec;
// }

const SSkirmishAILibrary* CInterface::LoadSkirmishAILibrary(
		const char* const shortName,
		const char* const version) {

	SSkirmishAILibrary* ai = NULL;

	SSkirmishAISpecifier spec;
	spec.shortName = shortName;
	spec.version = version;

	mySkirmishAISpecifiers.insert(spec);

	T_skirmishAIs::iterator skirmishAI;
	skirmishAI = myLoadedSkirmishAIs.find(spec);
	if (skirmishAI == myLoadedSkirmishAIs.end()) {
		ai = new SSkirmishAILibrary;
		sharedLib_t lib = Load(spec, ai);
		if (sharedLib_isLoaded(lib)) {
			// success
			myLoadedSkirmishAIs[spec] = ai;
			myLoadedSkirmishAILibs[spec] = lib;
		} else {
			// failure
			free(ai);
			ai = NULL;
		}
	} else {
		ai = skirmishAI->second;
	}

	return ai;
}
int CInterface::UnloadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
) {
	SSkirmishAISpecifier spec;
	spec.shortName = shortName;
	spec.version = version;

	T_skirmishAIs::iterator skirmishAI = myLoadedSkirmishAIs.find(spec);
	T_skirmishAILibs::iterator skirmishAILib = myLoadedSkirmishAILibs.find(spec);

	if (skirmishAI == myLoadedSkirmishAIs.end()) {
		// to unload AI is not loaded -> no problem, do nothing
	} else {
		delete skirmishAI->second;
		myLoadedSkirmishAIs.erase(skirmishAI);
		sharedLib_unload(skirmishAILib->second);
		myLoadedSkirmishAILibs.erase(skirmishAILib);
	}

	return 0;
}
int CInterface::UnloadAllSkirmishAILibraries() {

	while (myLoadedSkirmishAIs.size() > 0) {
		T_skirmishAISpecifiers::const_iterator ai =
				mySkirmishAISpecifiers.begin();
		UnloadSkirmishAILibrary((*ai).shortName, (*ai).version);
	}

	return 0; // signal: ok
}


// private functions following

sharedLib_t CInterface::Load(const SSkirmishAISpecifier& spec, SSkirmishAILibrary* skirmishAILibrary) {
	return LoadSkirmishAILib(FindLibFile(spec), skirmishAILibrary);
}
sharedLib_t CInterface::LoadSkirmishAILib(const std::string& libFilePath,
		SSkirmishAILibrary* skirmishAILibrary) {

	sharedLib_t sharedLib = sharedLib_load(libFilePath.c_str());

	if (!sharedLib_isLoaded(sharedLib)) {
		reportError(std::string("Failed loading shared library: ") + libFilePath);
		return sharedLib;
	}

	// initialize the AI library
	std::string funcName;

	funcName = "getLevelOfSupportFor";
	skirmishAILibrary->getLevelOfSupportFor
			= (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(int teamId,
			const char*, int, const char*, const char*))
			sharedLib_findAddress(sharedLib, funcName.c_str());
	if (skirmishAILibrary->getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
	}

	funcName = "init";
	skirmishAILibrary->init
			= (int (CALLING_CONV_FUNC_POINTER *)(int teamId,
			const struct SSkirmishAICallback*))
			sharedLib_findAddress(sharedLib, funcName.c_str());
	if (skirmishAILibrary->init == NULL) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_INIT instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}

	funcName = "release";
	skirmishAILibrary->release
			= (int (CALLING_CONV_FUNC_POINTER *)(int teamId))
			sharedLib_findAddress(sharedLib, funcName.c_str());
	if (skirmishAILibrary->release == NULL) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_RELEASE instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}

	funcName = "handleEvent";
	skirmishAILibrary->handleEvent
			= (int (CALLING_CONV_FUNC_POINTER *)(int teamId,
			int, const void*))
			sharedLib_findAddress(sharedLib, funcName.c_str());
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

	const char* const skirmDD =
			callback->SkirmishAIs_Info_getValueByKey(interfaceId,
			spec.shortName, spec.version,
			SKIRMISH_AI_PROPERTY_DATA_DIR);
	if (skirmDD == NULL) {
		reportError(std::string("Missing Skirmish AI data-dir for ")
				+ spec.shortName + " " + spec.version);
	}

	static const size_t libFileName_sizeMax = 512;
	// eg. "libSkirmishAI.so" or "SkirmishAI.dll"
	char libFileName[libFileName_sizeMax];
	sharedLib_createFullLibName("SkirmishAI", libFileName, libFileName_sizeMax);

	return util_allocStrCatFSPath(2, skirmDD, libFileName);
}

bool CInterface::FitsThisInterface(const std::string& requestedShortName,
		const std::string& requestedVersion) {

	const char* const myShortName = callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* const myVersion = callback->AIInterface_Info_getValueByKey(interfaceId,
			AI_INTERFACE_PROPERTY_VERSION);

	bool shortNameFits = (requestedShortName == myShortName);
	bool versionFits = (requestedVersion == myVersion);

	return shortNameFits && versionFits;
}
