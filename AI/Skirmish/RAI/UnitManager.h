// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_UNIT_MANAGER_H
#define RAI_UNIT_MANAGER_H

struct sRAIGroup;
class cUnitManager;

#include "RAI.h"
using std::map;
using std::set;

struct sRAIGroupMilitary
{
	sRAIGroupMilitary()
	{
		count=0;
	};

	float3 RallyPoint;
	float3 ScoutPoint;
	int count;
};

struct sRAIGroupConstruct
{
	sRAIGroupConstruct()
	{
		BuildSpeed=0.0f;
		count=0;
	};

	float BuildSpeed;
	int count;
};

struct sRAIGroup
{
	sRAIGroup(int Index);
	~sRAIGroup();

	int index;
	map<int,UnitInfo*> Units;
	map<int,EnemyInfo*> Enemies;

	sRAIGroupMilitary* M;
	sRAIGroupConstruct* C;
//	UnitInfo* Radar;
//	UnitInfo* Jammer;
//	UnitInfo* AntiMis;
//	UnitInfo* Engineer;
};

#define SCOUT_POSITON_LIST_SIZE 20
#define RAI_GROUP_SIZE 25
class cUnitManager
{
public:
	cUnitManager(IAICallback* callback, cRAI* Global);
	~cUnitManager() {};

	void UnitFinished(int unit,UnitInfo *U);
	void UnitDestroyed(int unit,UnitInfo *U);
	void UnitIdle(int unit,UnitInfo *U);
	void EnemyEnterLOS(int enemy,EnemyInfo *E);
	void EnemyEnterRadar(int enemy,EnemyInfo *E);
	bool UnitMoveFailed(int unit,UnitInfo *U);
	void UpdateGroupSize();
	bool ActiveAttackOrders();
	void GroupAddUnit(int unit, UnitInfo* U, sRAIGroup* group);
	void GroupRemoveUnit(int unit, UnitInfo* U);
	void GroupAddEnemy(int enemy, EnemyInfo *E, sRAIGroup* group);
	void GroupRemoveEnemy(int enemy, EnemyInfo *E, sRAIGroup* group);
	void GroupResetRallyPoint(sRAIGroup* group);

//	bool AssaultScouts;

	sRAIGroup* Group[RAI_GROUP_SIZE];
//	sRAIGroup* Commander;
	int GroupSize;
private:

	bool AttackOrders;

	int MaxGroupMSize;
	void Assign(int unit,UnitInfo *U);
	void SendattackGroups();

	map<int,UnitInfo*> UAssault;	// key = unit id
	map<int,UnitInfo*> USuicide;
	set<int> USupport;

	struct sTransportUnitInfo
	{
		sTransportUnitInfo(const UnitDef *unitdef) { ud=unitdef; AssistID=-1; };
		const UnitDef *ud;
		int AssistID;
	};
	typedef pair<int,sTransportUnitInfo> itPair;
	map<int,sTransportUnitInfo> UTrans;

	struct sScoutPosition
	{
		int ScoutID;
		float3 position;
	};
	sScoutPosition *SL[SCOUT_POSITON_LIST_SIZE];
	int SLSize;
	struct sScoutUnitInfo
	{
		sScoutUnitInfo() {
			ScoutLocAssigned=false; 
			SL = NULL;
		};
		sScoutPosition *SL;
		bool ScoutLocAssigned;
	};
	typedef pair<int,sScoutUnitInfo> isPair;
	map<int,sScoutUnitInfo> UScout;

	IAICallback *cb;
	cRAI* G;
	cLogFile *l;
};

#endif
