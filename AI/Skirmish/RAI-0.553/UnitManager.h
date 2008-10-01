// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_UNIT_MANAGER_H
#define RAI_UNIT_MANAGER_H

struct sRAIGroup;
class cUnitManager;

#include "RAI.h"
//#include "LogFile.h"
//#include "ExternalAI/IAICallback.h"
//#include "Sim/Units/UnitDef.h"
//#include "Sim/Weapons/WeaponDefHandler.h"
//#include <map>

struct sRAIGroup
{
	sRAIGroup(int Index);
	~sRAIGroup();

	int index;
	map<int,UnitInfo*> Units;
	map<int,EnemyInfo*> Enemies;

	float3 RallyPoint;
	float3 ScoutPoint;
//	UnitInfo* Radar;
//	UnitInfo* Jammer;
//	UnitInfo* AntiMis;
//	UnitInfo* Engineer;
};

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
	void GroupAddUnit(int unit, UnitInfo* U, sRAIGroup* Gr);
	void GroupAddEnemy(int enemy, EnemyInfo *E, sRAIGroup* Gr);
	void GroupRemoveEnemy(int enemy, EnemyInfo *E, sRAIGroup* Gr);
	void GroupResetRallyPoint(sRAIGroup* Gr);

//	bool AssaultScouts;

	sRAIGroup* Group[25];
//	sRAIGroup* Commander;
	int GroupSize;
private:

	bool AttackOrders;
	typedef pair<sRAIGroup*,sRAIGroup*> ggPair;

	int MaxGroupMSize;
	void Assign(int unit,UnitInfo *U);
	void SendAttackGroups();

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

	struct sScoutLocation
	{
		int ScoutID;
		float3 location;
	};
	sScoutLocation *SL[20];
	int SLSize;
	struct sScoutUnitInfo
	{
		sScoutUnitInfo() { ScoutLocAssigned=false; };
		sScoutLocation *SL;
		bool ScoutLocAssigned;
	};
	typedef pair<int,sScoutUnitInfo> isPair;
	map<int,sScoutUnitInfo> UScout;

	IAICallback *cb;
	cRAI* G;
	cLogFile *l;
};

#endif
