#ifndef IGLOBALAICALLBACK_H
#define IGLOBALAICALLBACK_H

#include <vector>
#include <deque>
#include "float3.h"
#include "Game/command.h"
struct UnitDef;
struct FeatureDef;
class IAICheats;
class IAICallback;

class IGlobalAICallback
{
public:
	virtual IAICheats* GetCheatInterface()=0;	//this returns zero if .cheats is not enabled or there is several players in the game
	virtual IAICallback* GetAICallback()=0;
	virtual ~IGlobalAICallback();
};

#endif
