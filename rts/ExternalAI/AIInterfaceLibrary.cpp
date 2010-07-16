/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIInterfaceLibrary.h"

#include "Interface/aidefines.h"
#include "SkirmishAILibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"
#include "SAIInterfaceCallbackImpl.h"

#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"
#include "IAILibraryManager.h"
#include "LogOutput.h"


CAIInterfaceLibrary::CAIInterfaceLibrary(const CAIInterfaceLibraryInfo& _info)
		: interfaceId(-1)
		, initialized(false)
		, info(_info)
{

	libFilePath = FindLibFile();

	sharedLib = SharedLib::Instantiate(libFilePath);
	if (sharedLib == NULL) {
		logOutput.Print(
				"ERROR: Loading AI Interface library from file \"%s\".",
				libFilePath.c_str());
		return;
	}

	if (InitializeFromLib(libFilePath) == 0) {
		InitStatic();
	}
}

CAIInterfaceLibrary::~CAIInterfaceLibrary() {

	ReleaseStatic();
	delete sharedLib;
	sharedLib = NULL;
}

void CAIInterfaceLibrary::InitStatic() {

	if (sAIInterfaceLibrary.initStatic != NULL) {
		if (interfaceId == -1) {
			interfaceId = aiInterfaceCallback_getInstanceFor(&info, &callback);
		}
		int ret = sAIInterfaceLibrary.initStatic(interfaceId, &callback);
		if (ret != 0) {
			// initializing the library failed!
			logOutput.Print(
					"ERROR: Initializing AI Interface library from file \"%s\"."
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
			logOutput.Print(
					"ERROR: Releasing AI Interface Library from file \"%s\"."
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

int CAIInterfaceLibrary::GetLoadCount() const {

	int totalSkirmishAILibraryLoadCount = 0;
	std::map<const SkirmishAIKey, int>::const_iterator salc;
	for (salc=skirmishAILoadCount.begin(); salc != skirmishAILoadCount.end();
			salc++) {
		totalSkirmishAILibraryLoadCount += salc->second;
	}

	return totalSkirmishAILibraryLoadCount;
}

bool CAIInterfaceLibrary::IsSkirmishAILibraryLoaded(const SkirmishAIKey& key) const {
	return GetSkirmishAILibraryLoadCount(key) > 0;
}

const std::string& CAIInterfaceLibrary::GetLibraryFilePath() const {
	return libFilePath;
}

const bool CAIInterfaceLibrary::IsInitialized() const {
	return initialized;
}

/// used as fallback, when an AI could not be found
static int CALLING_CONV handleEvent_empty(int teamId, int receiver, const void* data) {
	return 0; // signaling: OK
}



// Skirmish AI methods
const CSkirmishAILibrary* CAIInterfaceLibrary::FetchSkirmishAILibrary(const CSkirmishAILibraryInfo& aiInfo) {

	CSkirmishAILibrary* ai = NULL;

	const SkirmishAIKey& skirmishAIKey = aiInfo.GetKey();

	if (skirmishAILoadCount[skirmishAIKey] == 0) {
		const SSkirmishAILibrary* sLib = sAIInterfaceLibrary.loadSkirmishAILibrary(aiInfo.GetShortName().c_str(), aiInfo.GetVersion().c_str());

		if (sLib == NULL) {
			logOutput.Print(
				"ERROR: Skirmish AI %s-%s not found!\n"
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
			struct SSkirmishAILibrary* sLib_empty = new SSkirmishAILibrary();
			sLib_empty->getLevelOfSupportFor = NULL;
			sLib_empty->init = NULL;
			sLib_empty->release = NULL;
			sLib_empty->handleEvent = handleEvent_empty;
			// NOTE: this causes a memory leack
			// as it is never freed anywhere()
			// no problem because it is used till the end of the game anyway.
			sLib = sLib_empty;
		}
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

	int releasedAIs = sAIInterfaceLibrary.unloadAllSkirmishAILibraries();
	loadedSkirmishAILibraries.clear();
	skirmishAILoadCount.clear();

	return releasedAIs;
}



void CAIInterfaceLibrary::reportInterfaceFunctionError(
		const std::string* libFileName, const std::string* functionName) {

	logOutput.Print(
			"ERROR: Loading AI Interface library from file \"%s\"."
			" No \"%s\" function exported.",
			libFileName->c_str(), functionName->c_str());
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
		//return -2;
	}

/*
	funcName = "getLevelOfSupportFor";
	sAIInterfaceLibrary.getLevelOfSupportFor
			= (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(const char*, int))
			sharedLib->FindAddress(funcName.c_str());
	if (sAIInterfaceLibrary.getLevelOfSupportFor == NULL) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(&libFilePath, &funcName);
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
		reportInterfaceFunctionError(&libFilePath, &funcName);
		return -4;
	}

	funcName = "unloadSkirmishAILibrary";
	sAIInterfaceLibrary.unloadSkirmishAILibrary
			= (int (CALLING_CONV_FUNC_POINTER *)(
			const char* const shortName,
			const char* const version))
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

	return dataDir + sPS + libFileName;
}
