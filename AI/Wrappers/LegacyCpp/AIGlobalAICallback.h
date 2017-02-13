/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_GLOBAL_AI_CALLBACK_H
#define _AI_GLOBAL_AI_CALLBACK_H

#include <memory>

#include "IGlobalAICallback.h"

struct SSkirmishAICallback;


namespace springLegacyAI {

class CAIAICallback;
class CAIAICheats;

/**
 * The AI side wrapper over the C AI interface for IGlobalAICallback.
 */
class CAIGlobalAICallback : public IGlobalAICallback {
public:
	CAIGlobalAICallback();
	CAIGlobalAICallback(const SSkirmishAICallback* _sAICallback, int _skirmishAIId);

	virtual IAICheats* GetCheatInterface();
	virtual IAICallback* GetAICallback();

	const SSkirmishAICallback* GetInnerCallback() const { return sAICallback; }

private:
	const SSkirmishAICallback* sAICallback;
	int skirmishAIId;

	std::unique_ptr<CAIAICallback> wrappedAICallback;
	std::unique_ptr<CAIAICheats> wrappedAICheats;
};

} // namespace springLegacyAI

#endif // _AI_GLOBAL_AI_CALLBACK_H
