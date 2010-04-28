/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SKIRMISH_AI_LIBRARY_H
#define _SKIRMISH_AI_LIBRARY_H

#include "SkirmishAIKey.h"
#include "Interface/SSkirmishAILibrary.h"
#include <vector>

class AIInterfaceKey;
struct SSkirmishAICallback;

/**
 * The engines container for a Skirmish AI library.
 * An instance of this class may represent RAI or KAIK.
 * @see CSkirmishAI
 */
class CSkirmishAILibrary {
public:
	CSkirmishAILibrary(const SSkirmishAILibrary& ai, const SkirmishAIKey& key);
	~CSkirmishAILibrary();

	SkirmishAIKey GetKey() const;

	/**
	 * Level of Support for a specific engine version and ai interface.
	 * @return see enum LevelOfSupport (higher values could be used optionally)
	 */
	LevelOfSupport GetLevelOfSupportFor(int teamId,
			const std::string& engineVersionString, int engineVersionNumber,
			const AIInterfaceKey& interfaceKey) const;

	bool Init(int teamId, const SSkirmishAICallback* c_callback) const;
	bool Release(int teamId) const;
	int HandleEvent(int teamId, int topic, const void* data) const;

private:
	SSkirmishAILibrary sSAI;
	SkirmishAIKey key;
};

#endif // _SKIRMISH_AI_LIBRARY_H
