#ifndef ATTACKGROUP_H
#define ATTACKGROUP_H

#include "System/StdAfx.h"
#include "creg/creg.h"
#include "creg/STL_List.h"

#include "ExternalAI/aibase.h"					// DLL exports and definitions
#include "ExternalAI/IGlobalAI.h"				// Main AI file
#include "Definitions.h"

using std::vector;
using std::list;

class CAttackGroup;

class AIClasses;

class CAttackGroup
{
	CR_DECLARE(CAttackGroup);
	public:
	CAttackGroup(AIClasses* ai, int groupID_in);
	CAttackGroup();

	virtual ~CAttackGroup();

	//update-related functions:
	void MicroUpdate(float3 groupPosition);
	void MoveOrderUpdate(float3 groupPosition);
	void StuckUnitFix();
	void Update();

	//Set/Get / Add/Remove
	bool Defending();
	int Size();
	int GetGroupID();
	float3 GetGroupPos();
	int PopStuckUnit();
	float DPS();
	void Log();
	unsigned GetWorstMoveType();
	float GetLowestUnitSpeed();
	float GetHighestUnitSpeed();
	float GetLowestAttackRange();
	float GetHighestAttackRange();
	vector<int>* GetAllUnits(); // for combining groups
	list<int>* GetAssignedEnemies();
	void AddUnit(int unitID);
	bool RemoveUnit(int unitID);
	int PopDamagedUnit();

	//attack order related functions, used from AH:
	void ClearTarget();
	bool NeedsNewTarget();
	void AssignTarget(vector<float3> path, float3 target, float radius);
	void Flee();
	void Defend();

	private:
	float3 CheckCoordinates(float3 pos);
	void DrawGroupPosition();
	bool FindDefenseTarget(float3 groupPosition);
	void PathToBase(float3 groupPosition);
	bool CloakedFix(int enemy);
	void RecalcGroupProperties();
	bool ConfirmPathToTargetIsOK();

	AIClasses *ai;	
	vector<int> units;
	float lowestAttackRange;
	float highestAttackRange;
	float lowestUnitSpeed;
	float highestUnitSpeed;
	float groupPhysicalSize;//close to the size of the radius of the unit
	float highestTurnRadius;

	unsigned worstMoveType;

	float3 attackPosition;
	float attackRadius;

	vector<float3> pathToTarget;
	int pathIterator;

	//static:
	int unitArray[MAXUNITS];

	int groupID;
	//states:
	bool isMoving;
	bool isFleeing;
	bool isDefending;
	bool isShooting;

	float groupDPS;
	void RecalcDPS();

	float3 groupPos;
	void RecalcGroupPos();
	int lastGroupPosUpdateFrame;

	float3 baseReference;
	int lastBaseReferenceUpdate;

	void RecalcAssignedEnemies();
	list<int> assignedEnemies;
};

#endif /* ATTACKGROUP_H */
