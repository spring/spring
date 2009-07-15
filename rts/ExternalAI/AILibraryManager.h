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

#ifndef _AILIBRARYMANAGER_H
#define	_AILIBRARYMANAGER_H

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

	virtual const std::vector<std::string>& GetSkirmishAIOptionValueKeys(int teamId) const;
	virtual const std::map<std::string, std::string>& GetSkirmishAIOptionValues(int teamId) const;

	virtual const T_skirmishAIInfos& GetUsedSkirmishAIInfos();

	virtual const T_dupInt& GetDuplicateInterfaceInfos() const;
	virtual const T_dupSkirm& GetDuplicateSkirmishAIInfos() const;

	virtual std::vector<SkirmishAIKey> FittingSkirmishAIKeys(
			const SkirmishAIKey& skirmishAIKey) const;
	/**
	 * A Skirmish AI (its library) is only really loaded
	 * when it is not yet loaded.
	 */
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(
			const SkirmishAIKey& skirmishAIKey);
	/**
	 * A Skirmish AI is only unloaded when ReleaseSkirmishAILibrary() is called
	 * as many times as GetSkirmishAILibrary() was loading and unloading
	 * of the interfaces is handled internally/automatically.
	 */
	virtual void ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey);
	/** Unloads all currently Skirmish loaded AIs. */
	virtual void ReleaseAllSkirmishAILibraries();

	/** Unloads all currently loaded AIs and interfaces. */
	virtual void ReleaseEverything();

private:
	typedef std::map<const AIInterfaceKey, IAIInterfaceLibrary*>
			T_loadedInterfaces;
	T_loadedInterfaces loadedAIInterfaceLibraries;

	T_interfaceSpecs interfaceKeys;
	T_skirmishAIKeys skirmishAIKeys;
	T_interfaceInfos interfaceInfos;
	T_skirmishAIInfos skirmishAIInfos;

	std::map<int, std::vector<std::string> > teamId_skirmishAIOptionValueKeys;
	static const std::vector<std::string> EMPTY_OPTION_VALUE_KEYS;
	std::map<int, std::map<std::string, std::string> > teamId_skirmishAIOptionValues;
	static const std::map<std::string, std::string> EMPTY_OPTION_VALUES;

	T_skirmishAIInfos usedSkirmishAIInfos;
	bool usedSkirmishAIInfos_initialized;

	T_dupInt duplicateInterfaceInfos;
	T_dupSkirm duplicateSkirmishAIInfos;

private:
	/**
	 * Loads the interface if it is not yet loaded; increments load count.
	 */
	IAIInterfaceLibrary* FetchInterface(const AIInterfaceKey& interfaceKey);
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
	// helper functions
	static void reportError(const char* topic, const char* msg);
	static void reportError1(const char* topic, const char* msg, const char* arg0);
	static void reportError2(const char* topic, const char* msg, const char* arg0, const char* arg1);
	static void reportInterfaceFunctionError(const std::string* libFileName, const std::string* functionName);
	static std::string extractFileName(const std::string& libFile, bool includeExtension);
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
	 * @see #versionCompare()
	 */
	static AIInterfaceKey FindFittingInterfaceSpecifier(
			const std::string& shortName,
			const std::string& minVersion,
			const T_interfaceSpecs& specs);
	/**
	 * Compares two version strings.
	 * Splits the version sptirngs at the '.' signs, and compares the parts.
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
	static int versionCompare(
			const std::string& version1,
			const std::string& version2);
};

#endif // _AILIBRARYMANAGER_H
