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
#include "IAILibraryManager.h"

CAIInterfaceLibrary::CAIInterfaceLibrary(const CAIInterfaceLibraryInfo& _info)
		: info(_info) {

	libFilePath = FindLibFile();

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
	sharedLib = NULL;
}

void CAIInterfaceLibrary::InitStatic() {

	if (sAIInterfaceLibrary.initStatic != NULL) {
		staticGlobalData = createStaticGlobalData();
		unsigned int infSize = info.GetInfo().size();
		const char** infKeys = info.GetCInfoKeys();
		const char** infValues = info.GetCInfoValues();
		int ret = sAIInterfaceLibrary.initStatic(infSize, infKeys, infValues,
				staticGlobalData);
		if (ret != 0) {
			// initializing the library failed!
			const int MAX_MSG_LENGTH = 511;
			char s_msg[MAX_MSG_LENGTH + 1];
			SNPRINTF(s_msg, MAX_MSG_LENGTH,
					"Error initializing AI Interface library from file\n\"%s\".\n"
					"The call to initStatic() returned unsuccessfuly.",
					libFilePath.c_str());
			handleerror(NULL, s_msg, "AI Interface Error", MBF_OK | MBF_EXCL);
		}
	} else {
		staticGlobalData = NULL;
	}
}
void CAIInterfaceLibrary::ReleaseStatic() {

	if (sAIInterfaceLibrary.releaseStatic != NULL) {
		int ret = sAIInterfaceLibrary.releaseStatic();
		if (ret != 0) {
			// releasing the library failed!
			const int MAX_MSG_LENGTH = 511;
			char s_msg[MAX_MSG_LENGTH + 1];
			SNPRINTF(s_msg, MAX_MSG_LENGTH,
					"Error releasing AI Interface Library from file\n\"%s\".\n"
					"The call to releaseStatic() returned unsuccessfuly.",
					libFilePath.c_str());
			handleerror(NULL, s_msg, "AI Interface Error", MBF_OK | MBF_EXCL);
		}
	}
	if (staticGlobalData != NULL) {
		freeStaticGlobalData(staticGlobalData);
	}
}

AIInterfaceKey CAIInterfaceLibrary::GetKey() const {
	return info.GetKey();
}

LevelOfSupport CAIInterfaceLibrary::GetLevelOfSupportFor(
		const std::string& engineVersionString, int engineVersionNumber) const {

	if (sAIInterfaceLibrary.getLevelOfSupportFor != NULL) {
		return sAIInterfaceLibrary.getLevelOfSupportFor(
				engineVersionString.c_str(), engineVersionNumber);
	} else {
		return LOS_Unknown;
	}
}

int CAIInterfaceLibrary::GetLoadCount() const {

	int totalSkirmishAILibraryLoadCount = 0;
	std::map<const SkirmishAIKey, int>::const_iterator salc;
	for (salc=skirmishAILoadCount.begin(); salc != skirmishAILoadCount.end();
			salc++) {
		totalSkirmishAILibraryLoadCount += salc->second;
	}

	return totalSkirmishAILibraryLoadCount;
}


// Skirmish AI methods
const ISkirmishAILibrary* CAIInterfaceLibrary::FetchSkirmishAILibrary(
		const CSkirmishAILibraryInfo& aiInfo) {

	ISkirmishAILibrary* ai = NULL;

	unsigned int infSize = aiInfo.GetInfo().size();
	const char** infKeys = aiInfo.GetCInfoKeys();
	const char** infValues = aiInfo.GetCInfoValues();

	const SkirmishAIKey& skirmishAIKey = aiInfo.GetKey();
	if (skirmishAILoadCount[skirmishAIKey] == 0) {
		const SSAILibrary* sLib =
				sAIInterfaceLibrary.loadSkirmishAILibrary(
				infSize, infKeys, infValues);
		ai = new CSkirmishAILibrary(*sLib, skirmishAIKey);
		loadedSkirmishAILibraries[skirmishAIKey] = ai;
	} else {
		ai = loadedSkirmishAILibraries[skirmishAIKey];
	}

	skirmishAILoadCount[skirmishAIKey]++;

	return ai;
}
int CAIInterfaceLibrary::ReleaseSkirmishAILibrary(const SkirmishAIKey& key) {

	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	const CSkirmishAILibraryInfo* aiInfo =
			libMan->GetSkirmishAIInfos().find(key)->second;
	unsigned int infSize = aiInfo->GetInfo().size();
	const char** infKeys = aiInfo->GetCInfoKeys();
	const char** infValues = aiInfo->GetCInfoValues();

	if (skirmishAILoadCount[key] == 0) {
		return 0;
	}

	skirmishAILoadCount[key]--;

	if (skirmishAILoadCount[key] == 0) {
		loadedSkirmishAILibraries.erase(key);
		sAIInterfaceLibrary.unloadSkirmishAILibrary(infSize, infKeys, infValues);
	}

	return skirmishAILoadCount[key];
}
int CAIInterfaceLibrary::GetSkirmishAILibraryLoadCount(const SkirmishAIKey& key)
		const {
	return skirmishAILoadCount.at(key);
}
int CAIInterfaceLibrary::ReleaseAllSkirmishAILibraries() {

	int releasedAIs = sAIInterfaceLibrary.unloadAllSkirmishAILibraries();
	loadedSkirmishAILibraries.clear();
	skirmishAILoadCount.clear();

	return releasedAIs;
}



void CAIInterfaceLibrary::reportInterfaceFunctionError(
		const std::string* libFileName, const std::string* functionName) {

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
	sAIInterfaceLibrary.initStatic
			= (int (CALLING_CONV_FUNC_POINTER *)(
			unsigned int infoSize,
			const char** infoKeys, const char** infoValues,
			const struct SStaticGlobalData* staticGlobalData))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.initStatic == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(&libFilePath, &funcName);
		//return -1;
	}

	funcName = "releaseStatic";
	sAIInterfaceLibrary.releaseStatic
			= (int (CALLING_CONV_FUNC_POINTER *)())
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.releaseStatic == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(&libFilePath, &funcName);
		//return -1;
	}

	funcName = "getLevelOfSupportFor";
	sAIInterfaceLibrary.getLevelOfSupportFor
			= (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(const char*, int))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(&libFilePath, &funcName);
		//return -2;
	}

	funcName = "loadSkirmishAILibrary";
	sAIInterfaceLibrary.loadSkirmishAILibrary
			= (const SSAILibrary* (CALLING_CONV_FUNC_POINTER *)(
			unsigned int infoSize,
			const char** infoKeys, const char** infoValues))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.loadSkirmishAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
		return -4;
	}

	funcName = "unloadSkirmishAILibrary";
	sAIInterfaceLibrary.unloadSkirmishAILibrary
			= (int (CALLING_CONV_FUNC_POINTER *)(
			unsigned int infoSize,
			const char** infoKeys, const char** infoValues))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.unloadSkirmishAILibrary == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
		return -5;
	}

	funcName = "unloadAllSkirmishAILibraries";
	sAIInterfaceLibrary.unloadAllSkirmishAILibraries
			= (int (CALLING_CONV_FUNC_POINTER *)())
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.unloadAllSkirmishAILibraries == NULL) {
		reportInterfaceFunctionError(&libFilePath, &funcName);
		return -6;
	}

	return 0;
}


std::string CAIInterfaceLibrary::FindLibFile() {

	const std::string& dataDir = info.GetDataDir();

	std::string libFileName("AIInterface"); // "AIInterface"
	#ifndef _WIN32
		libFileName = "lib" + libFileName; // "libAIInterface"
	#endif

	// eg. libAIInterface.so
	libFileName = libFileName + "." + SharedLib::GetLibExtension();

	return dataDir + PS + libFileName;
}
