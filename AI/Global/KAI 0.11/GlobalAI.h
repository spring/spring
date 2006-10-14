#pragma once


#include "include.h"

const char AI_NAME[]="KAI";

using namespace std;
//#pragma warning(disable: 4244 4018 4996 4129)

//edit:
class CAttackHandler;

class CGlobalAI : public IGlobalAI  
{
public:
	CGlobalAI();
	virtual ~CGlobalAI();

	void InitAI(IGlobalAICallback* callback, int team);

	void UnitCreated(int unit);									//called when a new unit is created on ai team
	void UnitFinished(int unit);								//called when an unit has finished building
	void UnitDestroyed(int unit,int attacker);								//called when a unit is destroyed

	void EnemyEnterLOS(int enemy);
	void EnemyLeaveLOS(int enemy);

	void EnemyEnterRadar(int enemy);				
	void EnemyLeaveRadar(int enemy);				
	
	void EnemyDestroyed(int enemy,int attacker);							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)

	void UnitIdle(int unit);										//called when a unit go idle and is not assigned to any group

	void GotChatMsg(const char* msg,int player);					//called when someone writes a chat msg

	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);					//called when one of your units are damaged
	void EnemyDamaged(int damaged,int attacker,float damage,float3 dir);	//called when an enemy inside los or radar is damaged

	void UnitMoveFailed(int unit);
	int HandleEvent (int msg,const void *data);

	//called every frame
	void Update();


	AIClasses *ai;
	vector<CUNIT> MyUnits;

	char c[512];



};


