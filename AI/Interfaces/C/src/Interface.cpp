/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Interface.h"

#include "CUtils/SimpleLog.h"
#include "CUtils/SharedLibrary.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "ExternalAI/Interface/SAIInterfaceCallback.h"
#include "System/StringUtil.h"

CInterface::CInterface(int interfaceId, const SAIInterfaceCallback* callback):
	interfaceId(interfaceId),
	callback(callback)
{
	simpleLog_initcallback(interfaceId, "C Interface", callback->Log_logsl, LOG_LEVEL_INFO);

	const char* const myShortName = callback->AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_SHORT_NAME);
	const char* const myVersion = callback->AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_VERSION);

	simpleLog_log("This is the log-file of the %s v%s AI Interface", myShortName, myVersion);
}

//LevelOfSupport CInterface::GetLevelOfSupportFor(
//		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
//	return LOS_Working;
//}

const SSkirmishAILibrary* CInterface::LoadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
) {
	SSkirmishAISpecifier spec;
	spec.shortName = shortName;
	spec.version = version;

	mySkirmishAISpecifiers.insert(spec);

	auto skirmishAI = myLoadedSkirmishAIs.find(spec);

	if (skirmishAI != myLoadedSkirmishAIs.end())
		return &(skirmishAI->second);

	SSkirmishAILibrary ai;
	sharedLib_t lib = Load(spec, &ai);

	// failure
	if (!sharedLib_isLoaded(lib))
		return nullptr;

	// success
	myLoadedSkirmishAIs[spec] = ai;
	myLoadedSkirmishAILibs[spec] = lib;

	return (&myLoadedSkirmishAIs[spec]);
}

int CInterface::UnloadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
) {
	SSkirmishAISpecifier spec;
	spec.shortName = shortName;
	spec.version = version;

	auto skirmishAI = myLoadedSkirmishAIs.find(spec);
	auto skirmishAILib = myLoadedSkirmishAILibs.find(spec);

	// if to-unload-AI is not loaded, just do nothing
	if (skirmishAI != myLoadedSkirmishAIs.end()) {
		sharedLib_unload(skirmishAILib->second);
		myLoadedSkirmishAIs.erase(skirmishAI);
		myLoadedSkirmishAILibs.erase(skirmishAILib);
	}

	return 0;
}

int CInterface::UnloadAllSkirmishAILibraries()
{
	while (!myLoadedSkirmishAIs.empty()) {
		const auto it = mySkirmishAISpecifiers.cbegin();
		const SSkirmishAISpecifier& spec = *it;

		UnloadSkirmishAILibrary(spec.shortName, spec.version);
	}

	return 0; // signal: ok
}


// private functions following

sharedLib_t CInterface::Load(
	const SSkirmishAISpecifier& spec,
	SSkirmishAILibrary* skirmishAILibrary
) {
	return LoadSkirmishAILib(FindLibFile(spec), skirmishAILibrary);
}

sharedLib_t CInterface::LoadSkirmishAILib(
	const std::string& libFilePath,
	SSkirmishAILibrary* skirmishAILibrary
) {
	sharedLib_t sharedLib = sharedLib_load(libFilePath.c_str());

	if (!sharedLib_isLoaded(sharedLib)) {
		reportError(std::string("Failed loading shared library: ") + libFilePath);
		return sharedLib;
	}


	// initialize the AI library functions
	std::string funcName = "getLevelOfSupportFor";
	void* funcAddr = sharedLib_findAddress(sharedLib, funcName.c_str());

	skirmishAILibrary->getLevelOfSupportFor = (LevelOfSupport (CALLING_CONV_FUNC_POINTER *)(
		const char* aiShortName,
		const char* aiVersion,
		const char* engineVersionString,
		int engineVersionNumber,
		const char* aiInterfaceShortName,
		const char* aiInterfaceVersion
	)) funcAddr;

	if (skirmishAILibrary->getLevelOfSupportFor == nullptr) {
		// do nothing: it is permitted that an AI does not export this function
		//reportInterfaceFunctionError(libFilePath, funcName);
	}


	funcName = "init";
	funcAddr = sharedLib_findAddress(sharedLib, funcName.c_str());

	skirmishAILibrary->init = (int (CALLING_CONV_FUNC_POINTER *)(
		int skirmishAIId,
		const SSkirmishAICallback*
	)) funcAddr;

	if (skirmishAILibrary->init == nullptr) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_INIT instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}


	funcName = "release";
	funcAddr = sharedLib_findAddress(sharedLib, funcName.c_str());

	skirmishAILibrary->release = (int (CALLING_CONV_FUNC_POINTER *)(
		int skirmishAIId
	)) funcAddr;

	if (skirmishAILibrary->release == nullptr) {
		// do nothing: it is permitted that an AI does not export this function,
		// as it can still use EVENT_RELEASE instead
		//reportInterfaceFunctionError(libFilePath, funcName);
	}


	funcName = "handleEvent";
	funcAddr = sharedLib_findAddress(sharedLib, funcName.c_str());

	skirmishAILibrary->handleEvent = (int (CALLING_CONV_FUNC_POINTER *)(
		int skirmishAIId,
		int topicId,
		const void* data
	)) funcAddr;

	if (skirmishAILibrary->handleEvent == nullptr) {
		reportInterfaceFunctionError(libFilePath, funcName);
	}

	return sharedLib;
}


void CInterface::reportInterfaceFunctionError(
	const std::string& libFilePath,
	const std::string& functionName
) {
	std::string msg;
	msg += ("Failed loading AI Library from file \"" + libFilePath);
	msg += ("\": no \"" + functionName + "\" function exported");
	reportError(msg);
}

void CInterface::reportError(const std::string& msg) {
	simpleLog_logL(LOG_LEVEL_ERROR, msg.c_str());
}


std::string CInterface::FindLibFile(const SSkirmishAISpecifier& spec)
{
	const char* sn = spec.shortName;
	const char* sv = spec.version;

	std::string dataDir = callback->SkirmishAIs_Info_getValueByKey(interfaceId, sn, sv, SKIRMISH_AI_PROPERTY_DATA_DIR);

	if (dataDir.empty()) {
		reportError(std::string("Missing Skirmish AI data-dir for ") + sn + " " + sv);
		return dataDir;
	}

	// eg. "libSkirmishAI.so" or "SkirmishAI.dll"
	char libFileName[512];
	sharedLib_createFullLibName("SkirmishAI", libFileName, sizeof(libFileName));

	return (dataDir + cPS + libFileName);
}

