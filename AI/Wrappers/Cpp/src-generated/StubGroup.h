/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBGROUP_H
#define _CPPWRAPPER_STUBGROUP_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Group.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubGroup : public Group {

protected:
	virtual ~StubGroup();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int groupId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGroupId(int groupId);
	// @Override
	virtual int GetGroupId() const;
private:
	std::vector<springai::CommandDescription*> supportedCommands;/* = std::vector<springai::CommandDescription*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSupportedCommands(std::vector<springai::CommandDescription*> supportedCommands);
	// @Override
	virtual std::vector<springai::CommandDescription*> GetSupportedCommands();
private:
	bool isSelected;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSelected(bool isSelected);
	// @Override
	virtual bool IsSelected();
	/**
	 * Creates a group and returns the id it was given, returns -1 on failure
	 */
	// @Override
	virtual int Create();
	/**
	 * Erases a specified group
	 */
	// @Override
	virtual void Erase();
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 * @param facing  set it to UNIT_COMMAND_BUILD_NO_FACING, if you do not want to specify a certain facing
	 */
	// @Override
	virtual void Build(UnitDef* toBuildUnitDef, const springai::AIFloat3& buildPos, int facing, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Stop(short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Wait(short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 * @param time  the time in seconds to wait
	 */
	// @Override
	virtual void WaitFor(int time, short options, int timeOut);
	/**
	 * Wait until another unit is dead, units will not wait on themselves.
	 * Example:
	 * A group of aircrafts waits for an enemy's anti-air defenses to die,
	 * before passing over their ruins to attack.
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 * @param toDieUnitId  wait until this unit is dead
	 */
	// @Override
	virtual void WaitForDeathOf(Unit* toDieUnit, short options, int timeOut);
	/**
	 * Wait for a specific ammount of units.
	 * Usually used with factories, but does work on groups without a factory too.
	 * Example:
	 * Pick a factory and give it a rallypoint, then add a SquadWait command
	 * with the number of units you want in your squads.
	 * Units will wait at the initial rally point until enough of them
	 * have arrived to make up a squad, then they will continue along their queue.
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void WaitForSquadSize(int numUnits, short options, int timeOut);
	/**
	 * Wait for the arrival of all units included in the command.
	 * Only makes sense for a group of units.
	 * Use it after a movement command of some sort (move / fight).
	 * Units will wait until all members of the GatherWait command have arrived
	 * at their destinations before continuing.
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void WaitForAll(short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void MoveTo(const springai::AIFloat3& toPos, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void PatrolTo(const springai::AIFloat3& toPos, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Fight(const springai::AIFloat3& toPos, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Attack(Unit* toAttackUnit, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void AttackArea(const springai::AIFloat3& toAttackPos, float radius, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Guard(Unit* toGuardUnit, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void AiSelect(short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void AddToGroup(Group* toGroup, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void RemoveFromGroup(short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Repair(Unit* toRepairUnit, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 * @param fireState  can be: 0=hold fire, 1=return fire, 2=fire at will
	 */
	// @Override
	virtual void SetFireState(int fireState, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 * @param moveState  0=hold pos, 1=maneuvre, 2=roam
	 */
	// @Override
	virtual void SetMoveState(int moveState, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void SetBase(const springai::AIFloat3& basePos, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void SelfDestruct(short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void SetWantedMaxSpeed(float wantedMaxSpeed, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 */
	// @Override
	virtual void LoadUnits(std::vector<springai::Unit*> toLoadUnitIds_list, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void LoadUnitsInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void LoadOnto(Unit* transporterUnit, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Unload(const springai::AIFloat3& toPos, Unit* toUnloadUnit, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void UnloadUnitsInArea(const springai::AIFloat3& toPos, float radius, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void SetOn(bool on, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void ReclaimUnit(Unit* toReclaimUnit, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void ReclaimFeature(Feature* toReclaimFeature, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void ReclaimInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Cloak(bool cloak, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Stockpile(short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void DGun(Unit* toAttackUnit, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void DGunPosition(const springai::AIFloat3& pos, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void RestoreArea(const springai::AIFloat3& pos, float radius, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void SetRepeat(bool repeat, short options, int timeOut);
	/**
	 * Tells weapons that support it to try to use a high trajectory
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 * @param trajectory  0: low-trajectory, 1: high-trajectory
	 */
	// @Override
	virtual void SetTrajectory(int trajectory, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Resurrect(Feature* toResurrectFeature, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void ResurrectInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void Capture(Unit* toCaptureUnit, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void CaptureInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut);
	/**
	 * Set the percentage of health at which a unit will return to a save place.
	 * This only works for a few units so far, mainly aircraft.
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 * @param autoRepairLevel  0: 0%, 1: 30%, 2: 50%, 3: 80%
	 */
	// @Override
	virtual void SetAutoRepairLevel(int autoRepairLevel, short options, int timeOut);
	/**
	 * Set what a unit should do when it is idle.
	 * This only works for a few units so far, mainly aircraft.
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 * @param idleMode  0: fly, 1: land
	 */
	// @Override
	virtual void SetIdleMode(int idleMode, short options, int timeOut);
	/**
	 * @param options  see enum UnitCommandOptions
	 * @param timeOut  At which frame the command will time-out and consequently be removed,
	 *                 if execution of it has not yet begun.
	 *                 Can only be set locally, is not sent over the network, and is used
	 *                 for temporary orders.
	 *                 default: MAX_INT (-> do not time-out)
	 *                 example: currentFrame + 15
	 */
	// @Override
	virtual void ExecuteCustomCommand(int cmdId, std::vector<float> params_list, short options, int timeOut);
}; // class StubGroup

}  // namespace springai

#endif // _CPPWRAPPER_STUBGROUP_H

