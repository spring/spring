#ifndef _GLOBALAICALLBACK_H
#define _GLOBALAICALLBACK_H

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

#endif /* _GLOBALAICALLBACK_H */
