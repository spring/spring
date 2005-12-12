// GroupAI.h: interface for the TGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_
//// Iincludes
#include <vector>
#include "UnitDef.h"
#include <list>
#include "IGlobalAI.h"
#include <map>
#include <set>
#include "helper.h"

const char AI_NAME[]= {"NTAI"};
using namespace std;


///// CGlobalAI

//Main interface called by Spring Engine, has been somewhat modified

class CGlobalAI : public IGlobalAI{
public:
	CGlobalAI();
	virtual ~CGlobalAI();
	int HandleEvent(int, const void*);
	void InitAI(IGlobalAICallback* callback, int team);
	void UnitCreated(int unit);
	void UnitFinished(int unit);
	void UnitDestroyed(int unit, int attacker);
	void EnemyEnterLOS(int enemy);
	void EnemyLeaveLOS(int enemy);
	void EnemyEnterRadar(int enemy);
	void EnemyLeaveRadar(int enemy);
	void EnemyDestroyed(int enemy, int attacker);
	void UnitIdle(int unit);
	void GotChatMsg(const char* msg,int player);
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);
	void UnitMoveFailed(int unit);
	void Update();
	void AddTAgent(T_Agent* a);
	void RemoveAgent(T_Agent* a);
	set<int> myUnits;
	set<int> enemies;
	int team;
	list<T_Agent*> TAgents;
};


/**
	UnitDef::Types
	MetalExtractor
	Transport
	Builder
	Factory
	Bomber
	Fighter
	GroundUnit
	Building
**/
#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)