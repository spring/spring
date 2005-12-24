#ifndef TAGENTS_H
#define TAGENTS_H

#include "ExternalAI/IAICallback.h"

class Agent{
public:
	Agent(){}
	virtual ~Agent(){}
	void InitAI(){}
	void Update(){}
};

class T_Agent : public Agent{
public:
	virtual void UnitCreated(int unit)=0;
	virtual void UnitFinished(int unit)=0;
	virtual void UnitDestroyed(int unit)=0;
	virtual void EnemyEnterLOS(int enemy)=0;
	virtual void EnemyLeaveLOS(int enemy)=0;
	virtual void EnemyEnterRadar(int enemy)=0;
	virtual void EnemyLeaveRadar(int enemy)=0;
	virtual void EnemyDestroyed(int enemy)=0;
	virtual void UnitIdle(int unit)=0;
	virtual void GotChatMsg(const char* msg,int player)=0;
	virtual void UnitDamaged(int damaged,int attacker,float damage,float3 dir)=0;
	virtual void UnitMoveFailed(int unit)=0;
};

class base : public T_Agent{
public:
	base(){}
	virtual ~base(){}
	void InitAI(IAICallback* callback){}
	void Update(){}
	//void Add(int unit){}
	//void Remove(int unit){}
	//std::set<int> ListUnits(){ return Units;}
	void UnitCreated(int unit){}
	void UnitFinished(int unit){}
	void UnitDestroyed(int unit){}
	void EnemyEnterLOS(int enemy){}
	void EnemyLeaveLOS(int enemy){}
	void EnemyEnterRadar(int enemy){}
	void EnemyLeaveRadar(int enemy){}
	void EnemyDestroyed(int enemy){}
	void UnitIdle(int unit){}
	void GotChatMsg(const char* msg,int player){}
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir){}
	void UnitMoveFailed(int unit){}
protected:
	//std::set<int> Units;
};

#endif

