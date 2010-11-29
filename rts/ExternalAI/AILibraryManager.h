/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_LIBRARY_MANAGER_H
#define	_AI_LIBRARY_MANAGER_H

#include "IAILibraryManager.h"

#include "AIInterfaceLibrary.h"
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"

#include <vector>
#include <map>
#include <set>

/**
 * Manages AI Interfaces and Skirmish AIs; their info and current status.
 * It loads info about AI Interfaces and Skirmish AIs from info files
 * in the data directories, from InterfaceInfo.lua and AIInfo.lua files.
 * In addition to that, it keeps track fo which Interfaces and AIs
 * are currently loaded, and unloads them when they are not needed anymore.
 */
class CAILibraryManager : public IAILibraryManager {
private:

public:
	CAILibraryManager();
	/**
	 * Unloads all Interface and AI shared libraries that are currently loaded.
	 */
	~CAILibraryManager();

	virtual const T_interfaceSpecs& GetInterfaceKeys() const;
	virtual const T_skirmishAIKeys& GetSkirmishAIKeys() const;

	virtual const T_interfaceInfos& GetInterfaceInfos() const;
	virtual const T_skirmishAIInfos& GetSkirmishAIInfos() const;

	virtual const T_dupInt& GetDuplicateInterfaceInfos() const;
	virtual const T_dupSkirm& GetDuplicateSkirmishAIInfos() const;

	virtual std::vector<SkirmishAIKey> FittingSkirmishAIKeys(
			const SkirmishAIKey& skirmishAIKey) const;
	virtual const CSkirmishAILibrary* FetchSkirmishAILibrary(
			const SkirmishAIKey& skirmishAIKey);
	virtual void ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey);

private:
	/** Unloads all currently loaded AIs and interfaces. */
	void ReleaseEverything();

	typedef std::map<const AIInterfaceKey, CAIInterfaceLibrary*>
			T_loadedInterfaces;
	T_loadedInterfaces loadedAIInterfaceLibraries;

	T_interfaceSpecs interfaceKeys;
	T_skirmishAIKeys skirmishAIKeys;
	T_interfaceInfos interfaceInfos;
	T_skirmishAIInfos skirmishAIInfos;

	T_dupInt duplicateInterfaceInfos;
	T_dupSkirm duplicateSkirmishAIInfos;

private:
	/**
	 * Loads the interface if it is not yet loaded; increments load count.
	 */
	CAIInterfaceLibrary* FetchInterface(const AIInterfaceKey& interfaceKey);
	/**
	 * Unloads the interface if its load count reaches 0.
	 */
	void ReleaseInterface(const AIInterfaceKey& interfaceKey);
	/**
	 * Loads info about available AI Interfaces and AIs from LUA Info files.
	 * -> interface and AI libraries can not corrupt the engines memory
	 *
	 * The files are searched in all data-dirs (see fs.GetDataDirectories())
	 * in the following sub-dirs:
	 * {AI_INTERFACES_DATA_DIR}/{*}/InterfaceInfo.lua
	 * {AI_INTERFACES_DATA_DIR}/{*}/{*}/InterfaceInfo.lua
	 * {SKIRMISH_AI_DATA_DIR}/{*}/AIInfo.lua
	 * {SKIRMISH_AI_DATA_DIR}/{*}/{*}/AIInfo.lua
	 * {GROUP_AI_DATA_DIR}/{*}/AIInfo.lua
	 * {GROUP_AI_DATA_DIR}/{*}/{*}/AIInfo.lua
	 *
	 * examples:
	 * AI/Skirmish/KAIK-0.13/AIInfo.lua
	 * AI/Skirmish/RAI/0.601/AIInfo.lua
	 */
	void GetAllInfosFromCache();
	/**
	 * Clears info about available AIs.
	 */
	void ClearAllInfos();

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
	 * @see IAILibraryManager::VersionCompare()
	 */
	static AIInterfaceKey FindFittingInterfaceSpecifier(
			const std::string& shortName,
			const std::string& minVersion,
			const T_interfaceSpecs& specs);
};

#endif // _AI_LIBRARY_MANAGER_H
