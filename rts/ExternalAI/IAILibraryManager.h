/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_AI_LIBRARY_MANAGER_H
#define I_AI_LIBRARY_MANAGER_H

#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"

#include <vector>
#include <map>
#include <set>

class AIInterfaceKey;
class SkirmishAIKey;
class CSkirmishAILibrary;

/**
 * @brief manages AIs and AI interfaces
 * @see CAILibraryManager
 */
class IAILibraryManager {
public:
	IAILibraryManager() {}
	virtual ~IAILibraryManager() {}

	typedef std::set<AIInterfaceKey> T_interfaceSpecs;
	typedef std::set<SkirmishAIKey> T_skirmishAIKeys;

	virtual const T_interfaceSpecs& GetInterfaceKeys() const = 0;
	virtual const T_skirmishAIKeys& GetSkirmishAIKeys() const = 0;

	typedef std::map<const AIInterfaceKey, CAIInterfaceLibraryInfo> T_interfaceInfos;
	typedef std::map<const SkirmishAIKey, CSkirmishAILibraryInfo> T_skirmishAIInfos;

	virtual const T_interfaceInfos& GetInterfaceInfos() const = 0;
	virtual const T_skirmishAIInfos& GetSkirmishAIInfos() const = 0;

	typedef std::map<const AIInterfaceKey, std::set<std::string> > T_dupInt;
	typedef std::map<const SkirmishAIKey, std::set<std::string> >
			T_dupSkirm;

	/**
	 * Returns a set of files which contain duplicate AI Interface infos.
	 * This can be used for issueing warnings.
	 */
	virtual const T_dupInt& GetDuplicateInterfaceInfos() const = 0;
	/**
	 * Returns a set of files which contain duplicate Skirmish AI infos.
	 * This can be used for issueing warnings.
	 */
	virtual const T_dupSkirm& GetDuplicateSkirmishAIInfos() const = 0;
	/**
	 * Returns a resolved aikey
	 * @see SkirmishAIKey::IsUnspecified()
	 */
	SkirmishAIKey ResolveSkirmishAIKey(const SkirmishAIKey& skirmishAIKey)
			const;

	/**
	 * A Skirmish AI (its library) is only really loaded when it is not yet
	 * loaded.
	 */
	virtual const CSkirmishAILibrary* FetchSkirmishAILibrary(
			const SkirmishAIKey& skirmishAIKey) = 0;
	/**
	 * A Skirmish AI is only unloaded when ReleaseSkirmishAILibrary() is called
	 * as many times as GetSkirmishAILibrary() was.
	 * loading and unloading of the interfaces
	 * is handled internally/automatically.
	 */
	virtual void ReleaseSkirmishAILibrary(const SkirmishAIKey& skirmishAIKey) = 0;

	/** Guaranteed to not return NULL. */
	static IAILibraryManager* GetInstance();
	/** Should only be called at end of game/process */
	static void Destroy();
	static void OutputAIInterfacesInfo();
	static void OutputSkirmishAIInfo();

protected:
	virtual std::vector<SkirmishAIKey> FittingSkirmishAIKeys(
			const SkirmishAIKey& skirmishAIKey) const = 0;

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
	static int VersionCompare(
			const std::string& version1,
			const std::string& version2);

private:
	static IAILibraryManager* gAILibraryManager;
};

#define aiLibManager IAILibraryManager::GetInstance()

#endif // I_AI_LIBRARY_MANAGER_H
