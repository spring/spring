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
	CR_DECLARE_DERIVED(CMobileCAI)
	CMobileCAI(CUnit* owner);
	CMobileCAI();
	virtual ~CMobileCAI();

	virtual void SetGoal(const float3& pos, const float3& curPos, float goalRadius = SQUARE_SIZE);
	virtual void SetGoal(const float3& pos, const float3& curPos, float goalRadius, float speed);
	virtual void BuggerOff(const float3& pos, float radius) override;
	bool SetFrontMoveCommandPos(const float3& pos);
	void StopMove() override;
	void StopMoveAndFinishCommand();
	void StopMoveAndKeepPointing(const float3& p, const float r, bool b);

	bool AllowedCommand(const Command& c, bool fromSynced) override;
	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature) override;
	void SlowUpdate() override;
	void GiveCommandReal(const Command& c, bool fromSynced = true) override;
	void NonMoving();
	void FinishCommand() override;
	void StopSlowGuard();
	void StartSlowGuard(float speed);
	void ExecuteAttack(Command& c) override;
	void ExecuteStop(Command& c) override;

	virtual void Execute();
	virtual void ExecuteGuard(Command& c);
	virtual void ExecuteFight(Command& c);
	virtual void ExecutePatrol(Command& c);
	virtual void ExecuteMove(Command& c);
	virtual void ExecuteLoadOnto(Command& c);

	virtual void ExecuteUnloadUnit(Command& c);
	virtual void ExecuteUnloadUnits(Command& c);
	virtual void ExecuteLoadUnits(Command& c);

	int GetCancelDistance() { return cancelDistance; }

	virtual bool IsValidTarget(const CUnit* enemy, CWeapon* weapon) const;
	virtual bool CanWeaponAutoTarget(const CWeapon* weapon) const override;

	void SetTransportee(CUnit* unit);
	bool FindEmptySpot(const CUnit* unloadee, const float3& center, float radius, float spread, float3& found, bool fromSynced = true);
	bool FindEmptyDropSpots(float3 startpos, float3 endpos, std::vector<float3>& dropSpots);
	CUnit* FindUnitToTransport(float3 center, float radius);
	bool LoadStillValid(CUnit* unit);
	bool SpotIsClear(float3 pos, CUnit* u);
	bool SpotIsClearIgnoreSelf(float3 pos, CUnit* unloadee);

	void UnloadUnits_Land(Command& c);
	void UnloadUnits_Drop(Command& c);
	void UnloadUnits_LandFlood(Command& c);
	void UnloadLand(Command& c);
	void UnloadDrop(Command& c);
	void UnloadLandFlood(Command& c);

	float3 lastBuggerGoalPos;
	float3 lastUserGoal;
	/**
	 * Used to avoid stuff in maneuver-mode moving too far away from patrol path
	 */
	float3 commandPos1;
	float3 commandPos2;

	float3 buggerOffPos;

	float buggerOffRadius;
	float repairBelowHealth;

	bool tempOrder;

protected:
	bool slowGuard;
	bool moveDir;

	int cancelDistance = 1024;

	/// last frame certain types of area-commands were handled, helps avoid infinite loops
	int lastCommandFrame = -1;
	int lastCloseInTry = -1;
	int lastBuggerOffTime = -BUGGER_OFF_TTL;
	int lastIdleCheck = 0;
	int numNonMovingCalls = 0;

	static constexpr int MAX_CLOSE_IN_RETRY_TICKS = 30;
	static constexpr int BUGGER_OFF_TTL = 200;

	static constexpr float MAX_USERGOAL_TOLERANCE_DIST = 100.0f;
	static constexpr float AIRTRANSPORT_DOCKING_RADIUS = 16.0f;
	static constexpr float AIRTRANSPORT_DOCKING_ANGLE = 50.0f;

	enum {
		UNLOAD_LAND = 0,
		UNLOAD_DROP = 1,
		UNLOAD_LANDFLOOD = 2,
	};

	void PushOrUpdateReturnFight() {
		CCommandAI::PushOrUpdateReturnFight(commandPos1, commandPos2);
	}

	void CalculateCancelDistance();

private:
	void ExecuteObjectAttack(Command& c);
	void ExecuteGroundAttack(Command& c);

	bool MobileAutoGenerateTarget();
	bool GenerateAttackCmd();
};

#endif /* MOBILE_CAI_H */
