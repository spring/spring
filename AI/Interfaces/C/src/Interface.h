/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "SSkirmishAISpecifier.h"
#include "CUtils/SharedLibrary.h"

#include <map>
#include <set>
#include <vector>
#include <string>

enum LevelOfSupport;
struct SStaticGlobalData;

class CInterface {
public:
	CInterface(int interfaceId,
			const struct SAIInterfaceCallback* callback);

//	// static properties
//	LevelOfSupport GetLevelOfSupportFor(
//			const char* engineVersion, int engineAIInterfaceGeneratedVersion);

	// skirmish AI methods
	const SSkirmishAILibrary* LoadSkirmishAILibrary(
		const char* const shortName,
		const char* const version);
	int UnloadSkirmishAILibrary(
		const char* const shortName,
		const char* const version);
	int UnloadAllSkirmishAILibraries();

private:
	// these functions actually load and unload the libraries
	sharedLib_t Load(const SSkirmishAISpecifier& aiKeyHash, SSkirmishAILibrary* ai);
	sharedLib_t LoadSkirmishAILib(const std::string& libFilePath,
			SSkirmishAILibrary* ai);

	static void reportInterfaceFunctionError(const std::string& libFileName,
			const std::string& functionName);
	static void reportError(const std::string& msg);
	std::string FindLibFile(const SSkirmishAISpecifier& sAISpecifier);

	bool FitsThisInterface(const std::string& requestedShortName,
			const std::string& requestedVersion);
private:
	const int interfaceId;
	const struct SAIInterfaceCallback* callback;

	typedef std::set<SSkirmishAISpecifier, SSkirmishAISpecifier_Comparator>
			T_skirmishAISpecifiers;
// 	typedef std::map<const SSkirmishAISpecifier,
// 			std::map<std::string, std::string>,
// 			SSkirmishAISpecifier_Comparator> T_skirmishAIInfos;
	typedef std::map<const SSkirmishAISpecifier, SSkirmishAILibrary*,
			SSkirmishAISpecifier_Comparator> T_skirmishAIs;
	typedef std::map<const SSkirmishAISpecifier, sharedLib_t,
			SSkirmishAISpecifier_Comparator> T_skirmishAILibs;

	T_skirmishAISpecifiers mySkirmishAISpecifiers;
// 	T_skirmishAIInfos mySkirmishAIInfos;
	T_skirmishAIs myLoadedSkirmishAIs;
	T_skirmishAILibs myLoadedSkirmishAILibs;
};

#endif // _INTERFACE_H
