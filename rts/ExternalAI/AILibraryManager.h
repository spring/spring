/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_LIBRARY_MANAGER_H
#define	AI_LIBRARY_MANAGER_H

#include "IAILibraryManager.h"

#include "AIInterfaceLibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"

#include <memory>
#include <vector>
#include <map>

/**
 * Manages AI Interfaces and Skirmish AIs; their info and current status.
 * It loads info about AI Interfaces and Skirmish AIs from info files
 * in the data directories, from InterfaceInfo.lua and AIInfo.lua files.
 * In addition to that, it keeps track fo which Interfaces and AIs
 * are currently loaded, and unloads them when they are not needed anymore.
 */
class CAILibraryManager : public IAILibraryManager {
public:
	CAILibraryManager();
	/**
	 * Unloads all Interface and AI shared libraries that are currently loaded.
	 */
	~CAILibraryManager();

	const T_interfaceSpecs& GetInterfaceKeys() const { return interfaceKeys; }
	const T_skirmishAIKeys& GetSkirmishAIKeys() const { return skirmishAIKeys; }

	const T_interfaceInfos& GetInterfaceInfos() const { return interfaceInfos; }
	const T_skirmishAIInfos& GetSkirmishAIInfos() const { return skirmishAIInfos; }

	const T_dupInt& GetDuplicateInterfaceInfos() const { return duplicateInterfaceInfos; }
	const T_dupSkirm& GetDuplicateSkirmishAIInfos() const { return duplicateSkirmishAIInfos; }

	std::vector<SkirmishAIKey> FittingSkirmishAIKeys(const SkirmishAIKey& skirmishAIKey) const;
	const CSkirmishAILibrary* FetchSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey);
	void ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey);

private:
	/** Unloads all currently loaded AIs and interfaces. */
	void ReleaseEverything();

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
	void GatherInterfaceLibrariesInfos();
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
	void GatherSkirmishAIsLibrariesInfos();

	void GatherSkirmishAIsLibrariesInfosFromLuaFiles(T_dupSkirm duplicateSkirmishAIInfoCheck);
	void GatherSkirmishAIsLibrariesInfosFromInterfaceLibrary(T_dupSkirm duplicateSkirmishAIInfoCheck);
	void StoreSkirmishAILibraryInfos(T_dupSkirm duplicateSkirmishAIInfoCheck, CSkirmishAILibraryInfo& skirmishAIInfo, const std::string& sourceDesc);
	/// Filter out Skirmish AIs that are specified multiple times
	void FilterDuplicateSkirmishAILibrariesInfos(T_dupSkirm duplicateSkirmishAIInfoCheck);

	/**
	 * Clears info about available AIs.
	 */
	void ClearAllInfos();

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
	 * @see IAILibraryManager::VersionCompare()
	 */
	static AIInterfaceKey FindFittingInterfaceSpecifier(
			const std::string& shortName,
			const std::string& minVersion,
			const T_interfaceSpecs& specs);


	typedef std::map<const AIInterfaceKey, std::unique_ptr<CAIInterfaceLibrary> > T_loadedInterfaces;

	T_loadedInterfaces loadedAIInterfaceLibraries;

	T_interfaceSpecs interfaceKeys;
	T_skirmishAIKeys skirmishAIKeys;
	T_interfaceInfos interfaceInfos;
	T_skirmishAIInfos skirmishAIInfos;

	T_dupInt duplicateInterfaceInfos;
	T_dupSkirm duplicateSkirmishAIInfos;
};

#endif // AI_LIBRARY_MANAGER_H
