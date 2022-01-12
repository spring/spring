/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIInterfaceLibrary.h"

#include "Interface/aidefines.h"
#include "SkirmishAILibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"
#include "SAIInterfaceCallbackImpl.h"

#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"
#include "AILibraryManager.h"


CAIInterfaceLibrary::CAIInterfaceLibrary(const CAIInterfaceLibraryInfo& _info)
	: interfaceId(-1)
	, initialized(false)
	, sAIInterfaceLibrary({ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr})
	, info(_info)
{
	libFilePath = FindLibFile();

	sharedLib = SharedLib::Instantiate(libFilePath);

	if (sharedLib == nullptr) {
		LOG_L(L_ERROR, "Loading AI Interface library from file \"%s\".", libFilePath.c_str());
		return;
	}

	if (InitializeFromLib(libFilePath) == 0) {
		InitStatic();
	}
}

CAIInterfaceLibrary::~CAIInterfaceLibrary() {
	ReleaseStatic();
	spring::SafeDelete(sharedLib);
}

void CAIInterfaceLibrary::InitStatic() {
	if (sAIInterfaceLibrary.initStatic == nullptr)
		return;

	if (interfaceId == -1)
		interfaceId = aiInterfaceCallback_getInstanceFor(&info, &callback);

	const int ret = sAIInterfaceLibrary.initStatic(interfaceId, &callback);
	const char* fmt =
		"Initializing AI Interface library from file \"%s\"."
		" The call to initStatic() returned unsuccessfuly.";

	if (ret == 0) {
		initialized = true;
		return;
	}

	// initializing the library failed!
	LOG_L(L_ERROR, fmt, libFilePath.c_str());

	aiInterfaceCallback_release(interfaceId);

	interfaceId = -1;
	initialized = false;
}

void CAIInterfaceLibrary::ReleaseStatic() {
	if (sAIInterfaceLibrary.releaseStatic != nullptr) {
		const int ret = sAIInterfaceLibrary.releaseStatic();
		const char* fmt =
			"Releasing AI Interface Library from file \"%s\"."
			" The call to releaseStatic() returned unsuccessfuly.";

		if (ret != 0) {
			// releasing the library failed!
			LOG_L(L_ERROR, fmt, libFilePath.c_str());
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
	const std::string& engineVersionString,
	int engineVersionNumber
) const {
	// if (sAIInterfaceLibrary.getLevelOfSupportFor != nullptr)
	//   return sAIInterfaceLibrary.getLevelOfSupportFor(engineVersionString.c_str(), engineVersionNumber);

	return LOS_Unknown;
}

int CAIInterfaceLibrary::GetSkirmishAICount() const {
	int numLookupSkirmishAIs = 0;

	if (sAIInterfaceLibrary.listSkirmishAILibraries != nullptr)
		numLookupSkirmishAIs = sAIInterfaceLibrary.listSkirmishAILibraries(interfaceId);

	return numLookupSkirmishAIs;
}

std::map<std::string, std::string> CAIInterfaceLibrary::GetSkirmishAIInfos(int aiIndex) const {
	std::map<std::string, std::string> lookupInfos;

	const bool libSupportsLookup =
		(sAIInterfaceLibrary.listSkirmishAILibraries != nullptr) &&
		(sAIInterfaceLibrary.listSkirmishAILibraryInfos != nullptr) &&
		(sAIInterfaceLibrary.listSkirmishAILibraryInfoKey != nullptr) &&
		(sAIInterfaceLibrary.listSkirmishAILibraryInfoValue != nullptr);

	if (!libSupportsLookup)
		return lookupInfos;

	const int numLookupInfos = sAIInterfaceLibrary.listSkirmishAILibraryInfos(interfaceId, aiIndex);

	for (int i = 0; i < numLookupInfos; ++i) {
		const char* key = sAIInterfaceLibrary.listSkirmishAILibraryInfoKey(interfaceId, aiIndex, i);
		const char* value = sAIInterfaceLibrary.listSkirmishAILibraryInfoValue(interfaceId, aiIndex, i);

		lookupInfos[key] = value;
	}

	return lookupInfos;
}

std::string CAIInterfaceLibrary::GetSkirmishAIOptions(int aiIndex) const {
	std::string lookupOptions;

	const bool libSupportsLookup =
		(sAIInterfaceLibrary.listSkirmishAILibraries != nullptr) &&
		(sAIInterfaceLibrary.listSkirmishAILibraryOptions != nullptr);

	if (!libSupportsLookup)
		return lookupOptions;

	const char* rawLuaOptions = sAIInterfaceLibrary.listSkirmishAILibraryOptions(interfaceId, aiIndex);

	if (rawLuaOptions != nullptr)
		lookupOptions = rawLuaOptions;

	return lookupOptions;
}

int CAIInterfaceLibrary::GetLoadCount() const {
	int totalSkirmishAILibraryLoadCount = 0;

	for (const auto& item: skirmishAILoadCount)
		totalSkirmishAILibraryLoadCount += item.second;

	return totalSkirmishAILibraryLoadCount;
}


/// used as fallback, when an AI could not be found
static int CALLING_CONV handleEvent_empty(int teamId, int receiver, const void* data) {
	return 0; // signaling: OK
}


SSkirmishAILibrary CAIInterfaceLibrary::EmptyInterfaceLib()
{
	SSkirmishAILibrary sLibEmpty;

	sLibEmpty.getLevelOfSupportFor = nullptr;
	sLibEmpty.init = nullptr;
	sLibEmpty.release = nullptr;
	sLibEmpty.handleEvent = handleEvent_empty;

	return sLibEmpty;
}

// Skirmish AI methods
const CSkirmishAILibrary* CAIInterfaceLibrary::FetchSkirmishAILibrary(const CSkirmishAILibraryInfo& aiInfo)
{
	const SkirmishAIKey& skirmishAIKey = aiInfo.GetKey();

	if (skirmishAILoadCount[skirmishAIKey] == 0) {
		const SSkirmishAILibrary* sLib = sAIInterfaceLibrary.loadSkirmishAILibrary(aiInfo.GetShortName().c_str(), aiInfo.GetVersion().c_str());

		if (sLib == nullptr) {
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
	const AILibraryManager* libMan = AILibraryManager::GetInstance();
	const CSkirmishAILibraryInfo& aiInfo = (libMan->GetSkirmishAIInfos()).find(key)->second;

	if (skirmishAILoadCount[key] == 0)
		return 0;

	skirmishAILoadCount[key]--;

	if (skirmishAILoadCount[key] == 0) {
		sAIInterfaceLibrary.unloadSkirmishAILibrary(aiInfo.GetShortName().c_str(), aiInfo.GetVersion().c_str());
		loadedSkirmishAILibraries.erase(key);
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
	const char* funcName = "initStatic";
	const void* funcAddr = sharedLib->FindAddress(funcName);

	sAIInterfaceLibrary.initStatic = (int (CALLING_CONV_FUNC_POINTER *)(
		int interfaceId,
		const SAIInterfaceCallback* callback
	)) funcAddr;

	if (sAIInterfaceLibrary.initStatic == nullptr) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
		//return -1;
	}


	funcName = "releaseStatic";
	funcAddr = sharedLib->FindAddress(funcName);

	sAIInterfaceLibrary.releaseStatic = (int (CALLING_CONV_FUNC_POINTER *)(
	)) funcAddr;

	if (sAIInterfaceLibrary.releaseStatic == nullptr) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
		//return -2;
	}


/*
	funcName = "getLevelOfSupportFor";
	funcAddr = sharedLib->FindAddress(funcName);

	sAIInterfaceLibrary.getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(
		const char*,
		int
	)) funcAddr;

	if (sAIInterfaceLibrary.getLevelOfSupportFor == nullptr) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
		//return -3;
	}
*/


	funcName = "loadSkirmishAILibrary";
	funcAddr = sharedLib->FindAddress(funcName);

	sAIInterfaceLibrary.loadSkirmishAILibrary = (const SSkirmishAILibrary* (CALLING_CONV_FUNC_POINTER *)(
		const char* const shortName,
		const char* const version
	)) funcAddr;

	if (sAIInterfaceLibrary.loadSkirmishAILibrary == nullptr) {
		reportInterfaceFunctionError(libFilePath, funcName);
		return -4;
	}


	funcName = "unloadSkirmishAILibrary";
	funcAddr = sharedLib->FindAddress(funcName);

	sAIInterfaceLibrary.unloadSkirmishAILibrary = (int (CALLING_CONV_FUNC_POINTER *)(
		const char* const shortName,
		const char* const version
	)) funcAddr;

	if (sAIInterfaceLibrary.unloadSkirmishAILibrary == nullptr) {
		reportInterfaceFunctionError(libFilePath, funcName);
		return -5;
	}


	funcName = "unloadAllSkirmishAILibraries";
	funcAddr = sharedLib->FindAddress(funcName);

	sAIInterfaceLibrary.unloadAllSkirmishAILibraries = (int (CALLING_CONV_FUNC_POINTER *)(
	)) funcAddr;

	if (sAIInterfaceLibrary.unloadAllSkirmishAILibraries == nullptr) {
		reportInterfaceFunctionError(libFilePath, funcName);
		return -6;
	}


	funcAddr = nullptr;
	sAIInterfaceLibrary.listSkirmishAILibraries = (int (CALLING_CONV_FUNC_POINTER *)(
		int interfaceId
	)) funcAddr;

	funcAddr = nullptr;
	sAIInterfaceLibrary.listSkirmishAILibraryInfos = (int (CALLING_CONV_FUNC_POINTER *)(
		int interfaceId,
		int aiIndex
	)) funcAddr;

	funcAddr = nullptr;
	sAIInterfaceLibrary.listSkirmishAILibraryInfoKey = (const char* (CALLING_CONV_FUNC_POINTER *)(
		int interfaceId,
		int aiIndex,
		int infoIndex
	)) funcAddr;

	funcAddr = nullptr;
	sAIInterfaceLibrary.listSkirmishAILibraryInfoValue = (const char* (CALLING_CONV_FUNC_POINTER *)(
		int interfaceId,
		int aiIndex,
		int infoIndex
	)) funcAddr;

	funcAddr = nullptr;
	sAIInterfaceLibrary.listSkirmishAILibraryOptions = (const char* (CALLING_CONV_FUNC_POINTER *)(
		int interfaceId,
		int aiIndex
	)) funcAddr;

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

	return (dataDir + sPS + libFileName);
}
