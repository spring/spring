#pragma once
#include "GlobalAI.h"


class CAttackGroup
{
	public:
		CAttackGroup(AIClasses* ai, int groupID_in);

	virtual ~CAttackGroup();

	//update-related functions:
	void CAttackGroup::MicroUpdate(float3 groupPosition);
	void CAttackGroup::MoveOrderUpdate(float3 groupPosition);
	void CAttackGroup::StuckUnitFix();
	void CAttackGroup::Update();

	//Set/Get / Add/Remove
	bool CAttackGroup::Defending();
	int CAttackGroup::Size();
	int CAttackGroup::GetGroupID();
	float3 CAttackGroup::GetGroupPos();
	int CAttackGroup::PopStuckUnit();
	float CAttackGroup::DPS();
	void CAttackGroup::Log();
	unsigned CAttackGroup::GetWorstMoveType();
	float CAttackGroup::GetLowestUnitSpeed();
	float CAttackGroup::GetHighestUnitSpeed();
	float CAttackGroup::GetLowestAttackRange();
	float CAttackGroup::GetHighestAttackRange();
	vector<int>* CAttackGroup::GetAllUnits(); // for combining groups
	list<int>* CAttackGroup::GetAssignedEnemies();
	void CAttackGroup::AddUnit(int unitID);
	bool CAttackGroup::RemoveUnit(int unitID);
	int CAttackGroup::PopDamagedUnit();

	//attack order related functions, used from AH:
	void CAttackGroup::ClearTarget();
	bool CAttackGroup::NeedsNewTarget();
	void CAttackGroup::AssignTarget(vector<float3> path, float3 target, float radius);
	void CAttackGroup::Flee();
	void CAttackGroup::Defend();

	private:
	void CAttackGroup::DrawGroupPosition();
	bool CAttackGroup::FindDefenseTarget(float3 groupPosition);
	void CAttackGroup::PathToBase(float3 groupPosition);
	bool CAttackGroup::CloakedFix(int enemy);
	void CAttackGroup::RecalcGroupProperties();
	bool CAttackGroup::ConfirmPathToTargetIsOK();

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
	void CAttackGroup::RecalcDPS();

	float3 groupPos;
	void CAttackGroup::RecalcGroupPos();
	int lastGroupPosUpdateFrame;

	float3 baseReference;
	int lastBaseReferenceUpdate;

	void CAttackGroup::RecalcAssignedEnemies();
	list<int> assignedEnemies;
};
