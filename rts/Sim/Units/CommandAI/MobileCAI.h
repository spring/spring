/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOBILE_CAI_H
#define MOBILE_CAI_H

#include "CommandAI.h"
#include "Sim/Misc/GlobalConstants.h" // for SQUARE_SIZE
#include "System/float3.h"

#include <vector>

class CUnit;
class CFeature;
class CWeapon;
struct Command;

class CMobileCAI : public CCommandAI
{
public:
	CR_DECLARE(CMobileCAI)
	CMobileCAI(CUnit* owner);
	CMobileCAI();
	virtual ~CMobileCAI();

	virtual void SetGoal(const float3& pos, const float3& curPos, float goalRadius = SQUARE_SIZE);
	virtual void SetGoal(const float3& pos, const float3& curPos, float goalRadius, float speed);
	virtual void BuggerOff(const float3& pos, float radius);
	bool SetFrontMoveCommandPos(const float3& pos);
	void StopMove();
	void StopMoveAndFinishCommand();
	void StopMoveAndKeepPointing(const float3& p, const float r, bool b);

	bool AllowedCommand(const Command& c, bool fromSynced);
	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	void SlowUpdate();
	void GiveCommandReal(const Command& c, bool fromSynced = true);
	void NonMoving();
	void FinishCommand();
	bool CanSetMaxSpeed() const { return true; }
	void StopSlowGuard();
	void StartSlowGuard(float speed);
	void ExecuteAttack(Command& c);
	void ExecuteStop(Command& c);

	virtual void Execute();
	virtual void ExecuteGuard(Command& c);
	virtual void ExecuteFight(Command& c);
	virtual void ExecutePatrol(Command& c);
	virtual void ExecuteMove(Command& c);
	virtual void ExecuteSetWantedMaxSpeed(Command& c);
	virtual void ExecuteLoadOnto(Command& c);

	virtual void ExecuteUnloadUnit(Command& c);
	virtual void ExecuteUnloadUnits(Command& c);
	virtual void ExecuteLoadUnits(Command& c);

	int GetCancelDistance() { return cancelDistance; }

	virtual bool IsValidTarget(const CUnit* enemy) const;
	virtual bool CanWeaponAutoTarget(const CWeapon* weapon) const;

	void SetTransportee(CUnit* unit);
	bool FindEmptySpot(const float3& center, float radius, float spread, float3& found, const CUnit* unitToUnload, bool fromSynced = true);
	bool FindEmptyDropSpots(float3 startpos, float3 endpos, std::vector<float3>& dropSpots);
	CUnit* FindUnitToTransport(float3 center, float radius);
	bool LoadStillValid(CUnit* unit);
	bool SpotIsClear(float3 pos, CUnit* u);
	bool SpotIsClearIgnoreSelf(float3 pos, CUnit* unitToUnload);

	void UnloadUnits_Land(Command& c);
	void UnloadUnits_Drop(Command& c);
	void UnloadUnits_LandFlood(Command& c);
	void UnloadLand(Command& c);
	void UnloadDrop(Command& c);
	void UnloadLandFlood(Command& c);

	float3 lastBuggerGoalPos;
	float3 lastUserGoal;

	int lastIdleCheck;
	bool tempOrder;

	/// helps avoid infinate loops
	int lastPC;

	int lastBuggerOffTime;
	float3 buggerOffPos;
	float buggerOffRadius;

	float repairBelowHealth;
	/**
	 * Used to avoid stuff in maneuvre mode moving too far away from patrol path
	 */
	float3 commandPos1;
	float3 commandPos2;

protected:
	int cancelDistance;
	int lastCloseInTry;
	bool slowGuard;
	bool moveDir;

	void PushOrUpdateReturnFight() {
		CCommandAI::PushOrUpdateReturnFight(commandPos1, commandPos2);
	}

	void CalculateCancelDistance();

private:
	bool MobileAutoGenerateTarget();
	bool GenerateAttackCmd();
};

#endif /* MOBILE_CAI_H */
