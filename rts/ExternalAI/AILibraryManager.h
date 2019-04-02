/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_LIBRARY_MANAGER_H
#define	AI_LIBRARY_MANAGER_H

#include "AIInterfaceLibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"

#include <memory>
#include <map>
#include <vector>
#include <set>

class AIInterfaceKey;
class SkirmishAIKey;
class CSkirmishAILibrary;

/**
 * Manages AI Interfaces and Skirmish AIs; their info and current status.
 * It loads info about AI Interfaces and Skirmish AIs from info files
 * in the data directories, from InterfaceInfo.lua and AIInfo.lua files.
 * In addition to that, it keeps track fo which Interfaces and AIs
 * are currently loaded, and unloads them when they are not needed anymore.
 */
class AILibraryManager {
public:
	typedef std::set<AIInterfaceKey> T_interfaceSpecs;
	typedef std::set<SkirmishAIKey> T_skirmishAIKeys;

	typedef std::map<const AIInterfaceKey, CAIInterfaceLibraryInfo> T_interfaceInfos;
	typedef std::map<const SkirmishAIKey, CSkirmishAILibraryInfo> T_skirmishAIInfos;

	typedef std::map<const AIInterfaceKey, std::set<std::string> > T_dupInt;
	typedef std::map<const SkirmishAIKey, std::set<std::string> > T_dupSkirm;


public:
	static AILibraryManager* GetInstance(bool init = true);

	static void Create() { GetInstance(false); }
	static void Destroy() { GetInstance(false)->Kill(); }
	static void OutputAIInterfacesInfo();
	static void OutputSkirmishAIInfo();


	void Init();
	/**
	 * Unloads all Interface and AI shared libraries that are currently loaded.
	 */
	void Kill();

	bool Initialized() const { return initialized; }

	/**
	 * Returns a resolved aikey
	 * @see SkirmishAIKey::IsUnspecified()
	 */
	SkirmishAIKey ResolveSkirmishAIKey(const SkirmishAIKey& skirmishAIKey) const;


	const T_interfaceSpecs& GetInterfaceKeys() const { return interfaceKeys; }
	const T_skirmishAIKeys& GetSkirmishAIKeys() const { return skirmishAIKeys; }

	const T_interfaceInfos& GetInterfaceInfos() const { return interfaceInfos; }
	const T_skirmishAIInfos& GetSkirmishAIInfos() const { return skirmishAIInfos; }

	/**
	 * Returns a set of files which contain duplicate Skirmish AI infos.
	 * This can be used for issueing warnings.
	 */
	const T_dupSkirm& GetDuplicateSkirmishAIInfos() const { return duplicateSkirmishAIInfos; }

	std::vector<SkirmishAIKey> FittingSkirmishAIKeys(const SkirmishAIKey& skirmishAIKey) const;

	/**
	 * A Skirmish AI (its library) is only really loaded when it is not yet
	 * loaded.
	 */
	const CSkirmishAILibrary* FetchSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey);

	/**
	 * A Skirmish AI is only unloaded when ReleaseSkirmishAILibrary() is called
	 * as many times as GetSkirmishAILibrary() was.
	 * loading and unloading of the interfaces
	 * is handled internally/automatically.
	 */
	void ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey);

private:
	/** Unloads all currently loaded AIs and interfaces. */
	void ReleaseAll();

	/**
	 * Loads the interface if it is not yet loaded; increments load count.
	 */
	CAIInterfaceLibrary* FetchInterface(const AIInterfaceKey& interfaceKey);
	/**
	 * Unloads the interface if its load count reaches 0.
	 */
	void ReleaseInterface(const AIInterfaceKey& interfaceKey);
	/**
	 * Loads info about available AI Interfaces from Lua info-files.
	 *
	 * The files are searched in all data-dirs (see fs.GetDataDirectories())
	 * in the following sub-dirs:
	 * {AI_INTERFACES_DATA_DIR}/{*}/InterfaceInfo.lua
	 * {AI_INTERFACES_DATA_DIR}/{*}/{*}/InterfaceInfo.lua
	 *
	 * examples:
	 * AI/Interfaces/C/0.1/InterfaceInfo.lua
	 * AI/Interfaces/Java/0.1/InterfaceInfo.lua
	 */
	void GatherInterfaceLibInfo();
	/**
	 * Loads info about available Skirmish AIs from Lua info- and option-files.
	 * -> AI libraries can not corrupt the engines memory
	 *
	 * The files are searched in all data-dirs (see fs.GetDataDirectories())
	 * in the following sub-dirs:
	 * {SKIRMISH_AI_DATA_DIR}/{*}/AIInfo.lua
	 * {SKIRMISH_AI_DATA_DIR}/{*}/AIOptions.lua
	 * {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIInfo.lua
	 * {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIOptions.lua
	 *
	 * examples:
	 * AI/Skirmish/KAIK-0.13/AIInfo.lua
	 * AI/Skirmish/RAI/0.601/AIInfo.lua
	 * AI/Skirmish/RAI/0.601/AIOptions.lua
	 */
	void GatherSkirmishAILibInfo();

	void GatherSkirmishAILibInfoFromLuaFiles(T_dupSkirm& duplicateSkirmishAIInfoCheck);
	void GatherSkirmishAILibInfoFromInterfaceLib(T_dupSkirm& duplicateSkirmishAIInfoCheck);
	void StoreSkirmishAILibInfo(T_dupSkirm& duplicateSkirmishAIInfoCheck, CSkirmishAILibraryInfo& skirmishAIInfo, const std::string& sourceDesc);
	/// Filter out Skirmish AIs that are specified multiple times
	void FilterDuplicateSkirmishAILibInfo(const T_dupSkirm& duplicateSkirmishAIInfoCheck);

	/**
	 * Clears info about available AIs.
	 */
	void ClearAll();

private:
	/**
	 * Finds the best fitting interface.
	 * The short name has to fit perfectly, and the version of the interface
	 * has to be equal or higher then the requested one.
	 * If there are multiple fitting interfaces, the one with the next higher
	 * version is selected, eg:
	 * wanted: 0.2
	 * available: 0.1, 0.3, 0.5
	 * chosen: 0.3
	 *
	 * @see AILibraryManager::VersionCompare()
	 */
	static AIInterfaceKey FindFittingInterfaceKey(
		const std::string& shortName,
		const std::string& minVersion,
		const T_interfaceSpecs& specs
	);

	/**
	 * Compares two version strings.
	 * Splits the version strings at the '.' signs, and compares the parts.
	 * If the number of parts do not match, then the string with less parts
	 * is filled up with '.0' parts at its right, eg:
	 * version 1: 0.1.2   -> 0.1.2.0
	 * version 2: 0.1.2.3 -> 0.1.2.3
	 * The left most part has the highest significance.
	 * Comparison of the individual parts is done with std::string::compare(),
	 * which implies for example that letters > numbers.
	 * examples:
	 * ("2", "1") -> 1
	 * ("1", "1") -> 0
	 * ("1", "2") -> -1
	 * ("0.1.1", "0.1") -> 1
	 * ("1.a", "1.9") -> 1
	 * ("1.a", "1.A") -> 1
	 */
	static int VersionCompare(const std::string& version1, const std::string& version2);

private:
	std::map<const AIInterfaceKey, std::unique_ptr<CAIInterfaceLibrary> > loadedAIInterfaceLibs;

	T_interfaceSpecs interfaceKeys;
	T_skirmishAIKeys skirmishAIKeys;

	T_interfaceInfos interfaceInfos;
	T_skirmishAIInfos skirmishAIInfos;

	T_dupInt duplicateInterfaceInfos;
	T_dupSkirm duplicateSkirmishAIInfos;

	bool initialized = false;
};

#define aiLibManager AILibraryManager::GetInstance(true)
#endif // AI_LIBRARY_MANAGER_H

