#ifndef GLOBALAICALLBACK_H
#define GLOBALAICALLBACK_H

#include "IGlobalAICallback.h"
#include "AICallback.h"
class CGlobalAI;
class CAICheats;

class CGlobalAICallback :
	public IGlobalAICallback
{
public:
	CGlobalAICallback(CGlobalAI* ai);
	~CGlobalAICallback(void);

	CGlobalAI* ai;
	CAICheats* cheats;
	bool noMessages;
	CAICallback scb;

	IAICheats* GetCheatInterface();
	IAICallback* GetAICallback();
};

#endif
