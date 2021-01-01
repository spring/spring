/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKIRMISH_AI_LIBRARY_H
#define SKIRMISH_AI_LIBRARY_H

#include "SkirmishAIKey.h"
#include "Interface/SSkirmishAILibrary.h"

class AIInterfaceKey;
struct SSkirmishAICallback;

/**
 * The engines container for a Skirmish AI library.
 * An instance of this class may represent RAI or KAIK.
 * @see CSkirmishAI
 */
class CSkirmishAILibrary {
public:
	CSkirmishAILibrary() {}
	CSkirmishAILibrary(const SSkirmishAILibrary& ai, const SkirmishAIKey& key);

	const SkirmishAIKey& GetKey() const { return aiKey; }

	/**
	 * Level of Support for a specific engine version and ai interface.
	 * @return see enum LevelOfSupport (higher values could be used optionally)
	 */
	LevelOfSupport GetLevelOfSupportFor(
		const std::string& engineVersionString,
		const int engineVersionNumber,
		const AIInterfaceKey& interfaceKey
	) const;

	bool Init(int skirmishAIId, const SSkirmishAICallback* c_callback) const;
	bool Release(int skirmishAIId) const;
	int HandleEvent(int skirmishAIId, int topic, const void* data) const;

private:
	SSkirmishAILibrary aiLib;
	SkirmishAIKey aiKey;
};

#endif // SKIRMISH_AI_LIBRARY_H
