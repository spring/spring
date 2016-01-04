/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIInterfaceLibrary.h"

#include "Interface/aidefines.h"
#include "SkirmishAILibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"
#include "SAIInterfaceCallbackImpl.h"

#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "IAILibraryManager.h"


CAIInterfaceLibrary::CAIInterfaceLibrary(const CAIInterfaceLibraryInfo& _info)
		: interfaceId(-1)
		, initialized(false)
		, sAIInterfaceLibrary({ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL})
		, info(_info)
{
	libFilePath = FindLibFile();

	sharedLib = SharedLib::Instantiate(libFilePath);

	if (sharedLib == NULL) {
		LOG_L(L_ERROR,
				"Loading AI Interface library from file \"%s\".",
				libFilePath.c_str());
		return;
	}

	if (InitializeFromLib(libFilePath) == 0) {
		InitStatic();
	}
}

CAIInterfaceLibrary::~CAIInterfaceLibrary() {

	ReleaseStatic();
	SafeDelete(sharedLib);
}

void CAIInterfaceLibrary::InitStatic() {

	if (sAIInterfaceLibrary.initStatic != NULL) {
		if (interfaceId == -1) {
			interfaceId = aiInterfaceCallback_getInstanceFor(&info, &callback);
		}
		int ret = sAIInterfaceLibrary.initStatic(interfaceId, &callback);
		if (ret != 0) {
			// initializing the library failed!
			LOG_L(L_ERROR,
					"Initializing AI Interface library from file \"%s\"."
					" The call to initStatic() returned unsuccessfuly.",
					libFilePath.c_str());
			aiInterfaceCallback_release(interfaceId);
			interfaceId = -1;
			initialized = false;
		} else {
			initialized = true;
		}
	}
}
void CAIInterfaceLibrary::ReleaseStatic() {

	if (sAIInterfaceLibrary.releaseStatic != NULL) {
		int ret = sAIInterfaceLibrary.releaseStatic();
		if (ret != 0) {
			// releasing the library failed!
			LOG_L(L_ERROR,
					"Releasing AI Interface Library from file \"%s\"."
					" The call to releaseStatic() returned unsuccessfuly.",
					libFilePath.c_str());
		} else {
			initialized = false;
		}
	}
	if (interfaceId != -1) {
		aiInterfaceCallback_release(interfaceId);
		interfaceId = -1;
	}
}

AIInterfaceKey CAIInterfaceLibrary::GetKey() const {
	return info.GetKey();
}

LevelOfSupport CAIInterfaceLibrary::GetLevelOfSupportFor(
		const std::string& engineVersionString, int engineVersionNumber) const {

	//if (sAIInterfaceLibrary.getLevelOfSupportFor != NULL) {
	//	return sAIInterfaceLibrary.getLevelOfSupportFor(
	//			engineVersionString.c_str(), engineVersionNumber);
	//} else {
		return LOS_Unknown;
	//}
}

int CAIInterfaceLibrary::GetSkirmishAICount() const {

	int numLookupSkirmishAIs = 0;

	if (sAIInterfaceLibrary.listSkirmishAILibraries != NULL) {
		numLookupSkirmishAIs = sAIInterfaceLibrary.listSkirmishAILibraries(interfaceId);
	}

	return numLookupSkirmishAIs;
}

std::map<std::string, std::string> CAIInterfaceLibrary::GetSkirmishAIInfos(int aiIndex) const {

	std::map<std::string, std::string> lookupInfos;

	const bool libSupportsLookup =
			(sAIInterfaceLibrary.listSkirmishAILibraries != NULL)
			&& (sAIInterfaceLibrary.listSkirmishAILibraryInfos != NULL)
			&& (sAIInterfaceLibrary.listSkirmishAILibraryInfoKey != NULL)
			&& (sAIInterfaceLibrary.listSkirmishAILibraryInfoValue != NULL);
	if (libSupportsLookup) {
		const int numLookupInfos = sAIInterfaceLibrary.listSkirmishAILibraryInfos(interfaceId, aiIndex);
		for (int i = 0; i < numLookupInfos; ++i) {
			const char* key = sAIInterfaceLibrary.listSkirmishAILibraryInfoKey(interfaceId, aiIndex, i);
			const char* value = sAIInterfaceLibrary.listSkirmishAILibraryInfoValue(interfaceId, aiIndex, i);
			lookupInfos[key] = value;
		}
	}

	return lookupInfos;
}

std::string CAIInterfaceLibrary::GetSkirmishAIOptions(int aiIndex) const {

	std::string lookupOptions;

	const bool libSupportsLookup =
			(sAIInterfaceLibrary.listSkirmishAILibraries != NULL)
			&& (sAIInterfaceLibrary.listSkirmishAILibraryOptions != NULL);
	if (libSupportsLookup) {
		const char* rawLuaOptions = sAIInterfaceLibrary.listSkirmishAILibraryOptions(interfaceId, aiIndex);
		if (rawLuaOptions != NULL) {
			lookupOptions = rawLuaOptions;
		}
	}

	return lookupOptions;
}

int CAIInterfaceLibrary::GetLoadCount() const {

	int totalSkirmishAILibraryLoadCount = 0;
	std::map<const SkirmishAIKey, int>::const_iterator salc;
	for (salc=skirmishAILoadCount.begin(); salc != skirmishAILoadCount.end();
			++salc) {
		totalSkirmishAILibraryLoadCount += salc->second;
	}

	return totalSkirmishAILibraryLoadCount;
}


/// used as fallback, when an AI could not be found
static int CALLING_CONV handleEvent_empty(int teamId, int receiver, const void* data) {
	return 0; // signaling: OK
}


SSkirmishAILibrary CAIInterfaceLibrary::EmptyInterfaceLib()
{
	SSkirmishAILibrary sLibEmpty;

	sLibEmpty.getLevelOfSupportFor = NULL;
	sLibEmpty.init = NULL;
	sLibEmpty.release = NULL;
	sLibEmpty.handleEvent = handleEvent_empty;

	return sLibEmpty;
}

// Skirmish AI methods
const CSkirmishAILibrary* CAIInterfaceLibrary::FetchSkirmishAILibrary(const CSkirmishAILibraryInfo& aiInfo)
{
	const SkirmishAIKey& skirmishAIKey = aiInfo.GetKey();

	if (skirmishAILoadCount[skirmishAIKey] == 0) {
		const SSkirmishAILibrary* sLib = sAIInterfaceLibrary.loadSkirmishAILibrary(aiInfo.GetShortName().c_str(), aiInfo.GetVersion().c_str());

		if (sLib == NULL) {
			LOG_L(L_ERROR,
				"Skirmish AI %s-%s not found!\n"
				"The game will go on without it.\n"
				"This usually indicates a problem in the used "
				"AI Interface library (%s-%s),\n"
				"or the Skirmish AI library is not in the same "
				"place as its AIInfo.lua.",
				skirmishAIKey.GetShortName().c_str(),
				skirmishAIKey.GetVersion().c_str(),
				skirmishAIKey.GetInterface().GetShortName().c_str(),
				skirmishAIKey.GetInterface().GetVersion().c_str()
			);

			loadedSkirmishAILibraries[skirmishAIKey] = CSkirmishAILibrary(EmptyInterfaceLib(), skirmishAIKey);
		} else {
			loadedSkirmishAILibraries[skirmishAIKey] = CSkirmishAILibrary(*sLib, skirmishAIKey);
		}
	}

	skirmishAILoadCount[skirmishAIKey]++;

	return &loadedSkirmishAILibraries[skirmishAIKey];
}

int CAIInterfaceLibrary::ReleaseSkirmishAILibrary(const SkirmishAIKey& key) {

	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	const CSkirmishAILibraryInfo* aiInfo = libMan->GetSkirmishAIInfos().find(key)->second;

	if (skirmishAILoadCount[key] == 0) {
		return 0;
	}

	skirmishAILoadCount[key]--;

	if (skirmishAILoadCount[key] == 0) {
		loadedSkirmishAILibraries.erase(key);

		sAIInterfaceLibrary.unloadSkirmishAILibrary(aiInfo->GetShortName().c_str(), aiInfo->GetVersion().c_str());
	}

	return skirmishAILoadCount[key];
}



int CAIInterfaceLibrary::GetSkirmishAILibraryLoadCount(const SkirmishAIKey& key) const {
	return *&skirmishAILoadCount.find(key)->second;
}

int CAIInterfaceLibrary::ReleaseAllSkirmishAILibraries() {

	const int releasedAIs = sAIInterfaceLibrary.unloadAllSkirmishAILibraries();

	loadedSkirmishAILibraries.clear();
	skirmishAILoadCount.clear();

	return releasedAIs;
}



void CAIInterfaceLibrary::reportInterfaceFunctionError(
	const std::string& libFileName,
	const std::string& functionName
) {
	LOG_L(L_ERROR,
		"Loading AI Interface library from file \"%s\"."
		" No \"%s\" function exported.",
		libFileName.c_str(), functionName.c_str());
}

int CAIInterfaceLibrary::InitializeFromLib(const std::string& libFilePath) {

	// TODO: version checking

	std::string funcName;

	funcName = "initStatic";
	sAIInterfaceLibrary.initStatic
			= (int (CALLING_CONV_FUNC_POINTER *)(
			int interfaceId, const struct SAIInterfaceCallback* callback))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.initStatic == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
		//return -1;
	}

	funcName = "releaseStatic";
	sAIInterfaceLibrary.releaseStatic
			= (int (CALLING_CONV_FUNC_POINTER *)())
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.releaseStatic == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
		//return -2;
	}

/*
	funcName = "getLevelOfSupportFor";
	sAIInterfaceLibrary.getLevelOfSupportFor
			= (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(const char*, int))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
		//return -3;
	}
*/

	funcName = "loadSkirmishAILibrary";
	sAIInterfaceLibrary.loadSkirmishAILibrary
			= (const SSkirmishAILibrary* (CALLING_CONV_FUNC_POINTER *)(
			const char* const shortName,
			const char* const version))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.loadSkirmishAILibrary == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
		return -4;
	}

	funcName = "unloadSkirmishAILibrary";
	sAIInterfaceLibrary.unloadSkirmishAILibrary
			= (int (CALLING_CONV_FUNC_POINTER *)(
			const char* const shortName,
			const char* const version))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.unloadSkirmishAILibrary == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
		return -5;
	}

	funcName = "unloadAllSkirmishAILibraries";
	sAIInterfaceLibrary.unloadAllSkirmishAILibraries
			= (int (CALLING_CONV_FUNC_POINTER *)())
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.unloadAllSkirmishAILibraries == NULL) {
		reportInterfaceFunctionError(libFilePath, funcName);
		return -6;
	}

	sAIInterfaceLibrary.listSkirmishAILibraries
			= (int (CALLING_CONV_FUNC_POINTER *)(int interfaceId))
			sharedLib->FindAddress(funcName.c_str());

	sAIInterfaceLibrary.listSkirmishAILibraryInfos
			= (int (CALLING_CONV_FUNC_POINTER *)(int interfaceId, int aiIndex))
			sharedLib->FindAddress(funcName.c_str());

	sAIInterfaceLibrary.listSkirmishAILibraryInfoKey
			= (const char* (CALLING_CONV_FUNC_POINTER *)(int interfaceId,
			int aiIndex, int infoIndex))
			sharedLib->FindAddress(funcName.c_str());

	sAIInterfaceLibrary.listSkirmishAILibraryInfoValue
			= (const char* (CALLING_CONV_FUNC_POINTER *)(int interfaceId,
			int aiIndex, int infoIndex))
			sharedLib->FindAddress(funcName.c_str());

	sAIInterfaceLibrary.listSkirmishAILibraryOptions
			= (const char* (CALLING_CONV_FUNC_POINTER *)(int interfaceId,
			int aiIndex))
			sharedLib->FindAddress(funcName.c_str());

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

	return dataDir + sPS + libFileName;
}
