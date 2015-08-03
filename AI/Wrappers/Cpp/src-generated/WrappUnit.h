/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPUNIT_H
#define _CPPWRAPPER_WRAPPUNIT_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Unit.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappUnit : public Unit {

private:
	int skirmishAIId;
	int unitId;

	WrappUnit(int skirmishAIId, int unitId);
	virtual ~WrappUnit();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetUnitId() const;
public:
	static Unit* GetInstance(int skirmishAIId, int unitId);

	/**
	 * Returns the number of units a team can have, after which it can not build
	 * any more. It is possible that a team has more units then this value at
	 * some point in the game. This is possible for example with taking,
	 * reclaiming or capturing units.
	 * This value is usefull for controlling game performance, and will
	 * therefore often be set on games with old hardware to prevent lagging
	 * because of too many units.
	 */
public:
	// @Override
	virtual int GetLimit();

	/**
	 * Returns the maximum total number of units that may exist at any one point
	 * in time induring the current game.
	 */
public:
	// @Override
	virtual int GetMax();

	/**
	 * Returns the unit's unitdef struct from which you can read all
	 * the statistics of the unit, do NOT try to change any values in it.
	 */
public:
	// @Override
	virtual springai::UnitDef* GetDef();

	/**
	 * This is a set of parameters that is created by SetUnitRulesParam() and may change during the game.
	 * Each parameter is uniquely identified only by its id (which is the index in the vector).
	 * Parameters may or may not have a name.
	 * @return visible to skirmishAIId parameters.
	 * If cheats are enabled, this will return all parameters.
	 */
public:
	// @Override
	virtual std::vector<springai::UnitRulesParam*> GetUnitRulesParams();

	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
public:
	// @Override
	virtual springai::UnitRulesParam* GetUnitRulesParamByName(const char* rulesParamName);

	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
public:
	// @Override
	virtual springai::UnitRulesParam* GetUnitRulesParamById(int rulesParamId);

public:
	// @Override
	virtual int GetTeam();

public:
	// @Override
	virtual int GetAllyTeam();

	/**
	 * Indicates the units main function.
	 * This can be used as help for (skirmish) AIs.
	 * 
	 * example:
	 * A unit can shoot, build and transport other units.
	 * To human players, it is obvious that transportation is the units
	 * main function, as it can transport a lot of units,
	 * but has only weak build- and fire-power.
	 * Instead of letting the AI developers write complex
	 * algorithms to find out the same, mod developers can set this value.
	 * 
	 * @return  0: ???
	 *          1: ???
	 *          2: ???
	 *          ...
	 * @deprecated
	 */
public:
	// @Override
	virtual int GetAiHint();

public:
	// @Override
	virtual int GetStockpile();

public:
	// @Override
	virtual int GetStockpileQueued();

public:
	// @Override
	virtual float GetCurrentFuel();

	/**
	 * The unit's max speed
	 */
public:
	// @Override
	virtual float GetMaxSpeed();

	/**
	 * The furthest any weapon of the unit can fire
	 */
public:
	// @Override
	virtual float GetMaxRange();

	/**
	 * The unit's max health
	 */
public:
	// @Override
	virtual float GetMaxHealth();

	/**
	 * How experienced the unit is (0.0f - 1.0f)
	 */
public:
	// @Override
	virtual float GetExperience();

	/**
	 * Returns the group a unit belongs to, -1 if none
	 */
public:
	// @Override
	virtual int GetGroup();

public:
	// @Override
	virtual std::vector<springai::Command*> GetCurrentCommands();

	/**
	 * The commands that this unit can understand, other commands will be ignored
	 */
public:
	// @Override
	virtual std::vector<springai::CommandDescription*> GetSupportedCommands();

	/**
	 * The unit's current health
	 */
public:
	// @Override
	virtual float GetHealth();

public:
	// @Override
	virtual float GetSpeed();

	/**
	 * Indicate the relative power of the unit,
	 * used for experience calulations etc.
	 * This is sort of the measure of the units overall power.
	 */
public:
	// @Override
	virtual float GetPower();

public:
	// @Override
	virtual float GetResourceUse(Resource* resource);

public:
	// @Override
	virtual float GetResourceMake(Resource* resource);

public:
	// @Override
	virtual springai::AIFloat3 GetPos();

public:
	// @Override
	virtual springai::AIFloat3 GetVel();

public:
	// @Override
	virtual bool IsActivated();

	/**
	 * Returns true if the unit is currently being built
	 */
public:
	// @Override
	virtual bool IsBeingBuilt();

public:
	// @Override
	virtual bool IsCloaked();

public:
	// @Override
	virtual bool IsParalyzed();

public:
	// @Override
	virtual bool IsNeutral();

	/**
	 * Returns the unit's build facing (0-3)
	 */
public:
	// @Override
	virtual int GetBuildingFacing();

	/**
	 * Number of the last frame this unit received an order from a player.
	 */
public:
	// @Override
	virtual int GetLastUserOrderFrame();

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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
	// @Override
	virtual void SetWantedMaxSpeed(float wantedMaxSpeed, short options, int timeOut);

	/**
	 * @param options  see enum UnitCommandOptions
	 */
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
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
public:
	// @Override
	virtual void ExecuteCustomCommand(int cmdId, std::vector<float> params_list, short options, int timeOut);
}; // class WrappUnit

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPUNIT_H

