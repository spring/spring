/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_INTERFACE_LIBRARY_H
#define AI_INTERFACE_LIBRARY_H

#include "System/Platform/SharedLib.h"
#include "ExternalAI/Interface/ELevelOfSupport.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SAIInterfaceCallback.h"
#include "ExternalAI/SkirmishAIKey.h"

#include <string>
#include <map>

class CAIInterfaceLibraryInfo;
class CSkirmishAILibrary;
class CSkirmishAILibraryInfo;
struct SSkirmishAILibrary;

/**
 * The engines container for an AI Interface library.
 * An instance of this class may represent the Java or the C AI Interface.
 */
class CAIInterfaceLibrary {
public:
	CAIInterfaceLibrary(const CAIInterfaceLibraryInfo& info);
	~CAIInterfaceLibrary();

	AIInterfaceKey GetKey() const;

	LevelOfSupport GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber) const;

	int GetSkirmishAICount() const;
	std::map<std::string, std::string> GetSkirmishAIInfos(int aiIndex) const;
	std::string GetSkirmishAIOptions(int aiIndex) const;

	/**
	 * @brief	how many times is this interface loaded
	 * Thought the AI library may be loaded only once, it can be logically
	 * loaded multiple times.
	 * Example: If we load one RAI and two AAIs over this interface,
	 * the interface load counter will be three.
	 */
	int GetLoadCount() const;

	// Skirmish AI methods
	/**
	 * @brief	loads the AI library
	 * This only loads the AI library, and does not yet create an instance
	 * for a team.
	 * For the C and C++ AI interface eg, this will load a shared library.
	 * Increments the load counter.
	 */
	const CSkirmishAILibrary* FetchSkirmishAILibrary(const CSkirmishAILibraryInfo& aiInfo);
	/**
	 * @brief	unloads the Skirmish AI library
	 * This unloads the Skirmish AI library.
	 * For the C and C++ AI interface eg, this will unload a shared library.
	 * This should not be done when any instances
	 * of that AI are still in use, as it will result in a crash.
	 * Decrements the load counter.
	 */
	int ReleaseSkirmishAILibrary(const SkirmishAIKey& sAISpecifier);
	/**
	 * @brief	how many times is the Skirmish AI loaded
	 * Thought the AI library may be loaded only once, it can be logically
	 * loaded multiple times (load counter).
	 */
	int GetSkirmishAILibraryLoadCount(const SkirmishAIKey& sAISpecifier) const;
	/**
	 * @brief	is the Skirmish AI library loaded
	 */
	bool IsSkirmishAILibraryLoaded(const SkirmishAIKey& key) const {
		return (GetSkirmishAILibraryLoadCount(key) > 0);
	}

	/**
	 * @brief	unloads all AIs
	 * Unloads all AI libraries currently loaded through this interface.
	 */
	int ReleaseAllSkirmishAILibraries();

	/**
	 * @brief	path to the library file
	 * Returns the path to the shared library file wrapped by this class.
	 */
	const std::string& GetLibraryFilePath() const { return libFilePath; }

	/**
	 * @brief	whether this interface is successfully initialized
	 */
	const bool IsInitialized() const { return initialized; }

private:
	void InitStatic();
	void ReleaseStatic();

	static void reportInterfaceFunctionError(
		const std::string& libFileName,
		const std::string& functionName
	);
	int InitializeFromLib(const std::string& libFilePath);

	std::string FindLibFile();
	std::string libFilePath;

	SSkirmishAILibrary EmptyInterfaceLib();

	int interfaceId;
	bool initialized;

	SharedLib* sharedLib;

	SAIInterfaceCallback callback;
	SAIInterfaceLibrary sAIInterfaceLibrary;
	const CAIInterfaceLibraryInfo& info;

	std::map<const SkirmishAIKey, CSkirmishAILibrary> loadedSkirmishAILibraries;
	std::map<const SkirmishAIKey, int> skirmishAILoadCount;

	static const int MAX_INFOS = 128;
};

#endif // AI_INTERFACE_LIBRARY_H
