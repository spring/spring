/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_AI_CALLBACK_H_
#define _GLOBAL_AI_CALLBACK_H_

#include "IGlobalAICallback.h"

#include "AICallback.h"

class CSkirmishAIWrapper;
class CAICheats;

class CGlobalAICallback :
	public IGlobalAICallback
{
	CSkirmishAIWrapper* ai;

public:
	CGlobalAICallback(CSkirmishAIWrapper* ai);
	~CGlobalAICallback();

	CAICheats* cheatCallback;
	bool noMessages;
	CAICallback callback;

	virtual IAICheats* GetCheatInterface();
	virtual IAICallback* GetAICallback();
};

#endif // _GLOBAL_AI_CALLBACK_H_
