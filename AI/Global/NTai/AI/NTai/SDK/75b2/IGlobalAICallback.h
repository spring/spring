#ifndef IGLOBALAICALLBACK_H
#define IGLOBALAICALLBACK_H

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
