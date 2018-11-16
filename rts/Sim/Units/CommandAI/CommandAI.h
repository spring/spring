/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COMMAND_AI_H
#define _COMMAND_AI_H

#include <functional>
#include <vector>

#include "System/Object.h"
#include "CommandDescription.h"
#include "CommandQueue.h"
#include "System/float3.h"
#include "System/UnorderedSet.hpp"

class CUnit;
class CFeature;
class CWeapon;
struct Command;

class CCommandAI : public CObject
{
	CR_DECLARE(CCommandAI)

public:
	CCommandAI(CUnit* owner);
	CCommandAI();
	virtual ~CCommandAI();

	void DependentDied(CObject* o);

	static void InitCommandDescriptionCache();
	static void KillCommandDescriptionCache();

	inline void SetOrderTarget(CUnit* o);

	virtual void AddDeathDependence(CObject* o, DependenceType dep);
	virtual void DeleteDeathDependence(CObject* o, DependenceType dep);
	void AddCommandDependency(const Command& c);
	void ClearCommandDependencies();
	/// feeds into GiveCommandReal()
	void GiveCommand(const Command& c, bool fromSynced = true);
	void ClearTargetLock(const Command& fc);
	void WeaponFired(CWeapon* weapon, const bool searchForNewTarget);

	virtual bool CanWeaponAutoTarget(const CWeapon* weapon) const { return true; }
	virtual int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	virtual void SlowUpdate();
	virtual void GiveCommandReal(const Command& c, bool fromSynced = true);
	virtual void FinishCommand();

	virtual void BuggerOff(const float3& pos, float radius) {}
	virtual void StopMove() {}

	void StopAttackingTargetIf(const std::function<bool(const CUnit*)>& pred);
	void StopAttackingAllyTeam(int ally);

	/**
	 * @brief Determines if c will cancel a queued command
	 * @return true if c will cancel a queued command
	 */
	bool WillCancelQueued(const Command& c) const;

	bool HasCommand(int cmdID) const;
	bool HasMoreMoveCommands(bool skipFirstCmd = true) const;

	int CancelCommands(const Command& c, CCommandQueue& queue, bool& first);
	/**
	 * @brief Finds the queued command that would be canceled by the Command c
	 * @return An iterator pointing at the command, or commandQue.end(),
	 *   if no such queued command exists
	 */
	CCommandQueue::const_iterator GetCancelQueued(const Command& c, const CCommandQueue& queue) const;
	/**
	 * @brief Returns commands that overlap c, but will not be canceled by c
	 * @return a vector containing commands that overlap c
	 */
	std::vector<Command> GetOverlapQueued(const Command& c) const;
	std::vector<Command> GetOverlapQueued(const Command& c, const CCommandQueue& queue) const;

	const std::vector<const SCommandDescription*>& GetPossibleCommands() const { return possibleCommands; }

	/**
	 * @brief Causes this CommandAI to execute the attack order c
	 */
	virtual void ExecuteAttack(Command& c);

	/**
	 * @brief executes the stop command c
	 */
	virtual void ExecuteStop(Command& c);

	void UpdateCommandDescription(unsigned int cmdDescIdx, const Command& cmd);
	void UpdateCommandDescription(unsigned int cmdDescIdx, SCommandDescription&& modCmdDesc);
	void InsertCommandDescription(unsigned int cmdDescIdx, SCommandDescription&& cmdDesc);
	bool RemoveCommandDescription(unsigned int cmdDescIdx);

	void UpdateNonQueueingCommands();

	void SetCommandDescParam0(const Command& c);
	bool ExecuteStateCommand(const Command& c);

	void ExecuteInsert(const Command& c, bool fromSynced = true);
	void ExecuteRemove(const Command& c);

	void AddStockpileWeapon(CWeapon* weapon);
	void StockpileChanged(CWeapon* weapon);
	void UpdateStockpileIcon();
	bool CanChangeFireState() const;

	virtual bool AllowedCommand(const Command& c, bool fromSynced);

	CWeapon* stockpileWeapon;

	std::vector<const SCommandDescription*> possibleCommands;
	spring::unordered_set<int> nonQueingCommands;

	CCommandQueue commandQue;

	int lastUserCommand;
	int selfDCountdown;
	int lastFinishCommand;

	CUnit* owner;
	CUnit* orderTarget;

	bool targetDied;
	bool inCommand;
	bool repeatOrders;
	int lastSelectedCommandPage;

protected:
	// return true by default so non-AirCAI's trigger FinishCommand
	virtual bool SelectNewAreaAttackTargetOrPos(const Command& ac) { return true; }

	bool IsAttackCapable() const;
	bool SkipParalyzeTarget(const CUnit* target) const;

	void GiveAllowedCommand(const Command& c, bool fromSynced = true);
	void GiveWaitCommand(const Command& c);
	/**
	 * @brief Returns the command that keeps the unit close to the path
	 * @return a Fight Command with 6 arguments, the first three being where to
	 *   return to (the current position of the unit), and the second being
	 *   the location of the original command.
	 */
	void PushOrUpdateReturnFight(const float3& cmdPos1, const float3& cmdPos2);
	int UpdateTargetLostTimer(int unitID);
	void DrawDefaultCommand(const Command& c) const;

private:
	// FIXME make synced?
	spring::unsynced_set<CObject*> commandDeathDependences;
	/**
	 * continuously set to some non-zero value while target is in radar
	 * decremented by 1 every SlowUpdate (!), command is canceled when
	 * timer reaches 0
	 */
	int targetLostTimer;
};

inline void CCommandAI::SetOrderTarget(CUnit* o) {
	if (orderTarget != NULL) {
		// NOTE As we do not include Unit.h,
		//   the compiler does not know that CUnit derives from CObject,
		//   and thus we can not use static_cast<CObject*>(...) here.
		DeleteDeathDependence(reinterpret_cast<CObject*>(orderTarget), DEPENDENCE_ORDERTARGET);
	}
	orderTarget = o;
	if (orderTarget != NULL) {
		AddDeathDependence(reinterpret_cast<CObject*>(orderTarget), DEPENDENCE_ORDERTARGET);
	}
}

#endif // _COMMAND_AI_H
