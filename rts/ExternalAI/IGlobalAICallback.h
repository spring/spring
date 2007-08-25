#ifndef IGLOBALAICALLBACK_H
#define IGLOBALAICALLBACK_H

#include <vector>
#include <deque>
#include "aibase.h"
#include "float3.h"
#include "Sim/Units/CommandAI/Command.h"
struct UnitDef;
struct FeatureDef;
class IAICheats;
class IAICallback;

class SPRING_API IGlobalAICallback
{
public:
	virtual IAICheats* GetCheatInterface()=0;	//this returns zero if .cheats is not enabled or there is several players in the game
	virtual IAICallback* GetAICallback()=0;
	DECLARE_PURE_VIRTUAL(~IGlobalAICallback())
};

#endif
