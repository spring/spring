#if !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Iincludes
#include "UnitDef.h"
#include "IGlobalAI.h"
#include "helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// The name NTAI gives to the spring engine.

const char AI_NAME[]= {"NTAI"};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
using namespace std;
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

class CGlobalAI : public IGlobalAI{
public:
	CGlobalAI(int aindex);
	virtual ~CGlobalAI();
	void InitAI(IGlobalAICallback* callback, int team);
	void UnitCreated(int unit);
	void UnitFinished(int unit);
	void UnitDestroyed(int unit, int attacker);
	void EnemyEnterLOS(int enemy);
	void EnemyLeaveLOS(int enemy);
	void EnemyEnterRadar(int enemy);
	void EnemyLeaveRadar(int enemy);
	void EnemyDestroyed(int enemy,int attacker);
	void UnitIdle(int unit);
	void GotChatMsg(const char* msg,int player);
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);
	void UnitMoveFailed(int unit);
	void Update();
	int HandleEvent (int msg, const void *data);
	int tteam;
	bool badmap;
	int ai_index;
	int teamx;
	bool alone;
	Global* GL;
	IGlobalAICallback* cg;
	IAICallback* acallback;
	bool helped;
};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
