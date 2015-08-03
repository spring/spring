/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_UNIT_H
#define _CPPWRAPPER_UNIT_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Unit {

public:
	virtual ~Unit(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetUnitId() const = 0;

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
	virtual int GetLimit() = 0;

	/**
	 * Returns the maximum total number of units that may exist at any one point
	 * in time induring the current game.
	 */
public:
	virtual int GetMax() = 0;

	/**
	 * Returns the unit's unitdef struct from which you can read all
	 * the statistics of the unit, do NOT try to change any values in it.
	 */
public:
	virtual springai::UnitDef* GetDef() = 0;

	/**
	 * This is a set of parameters that is created by SetUnitRulesParam() and may change during the game.
	 * Each parameter is uniquely identified only by its id (which is the index in the vector).
	 * Parameters may or may not have a name.
	 * @return visible to skirmishAIId parameters.
	 * If cheats are enabled, this will return all parameters.
	 */
public:
	virtual std::vector<springai::UnitRulesParam*> GetUnitRulesParams() = 0;

	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
public:
	virtual springai::UnitRulesParam* GetUnitRulesParamByName(const char* rulesParamName) = 0;

	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
public:
	virtual springai::UnitRulesParam* GetUnitRulesParamById(int rulesParamId) = 0;

public:
	virtual int GetTeam() = 0;

public:
	virtual int GetAllyTeam() = 0;

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
	virtual int GetAiHint() = 0;

public:
	virtual int GetStockpile() = 0;

public:
	virtual int GetStockpileQueued() = 0;

public:
	virtual float GetCurrentFuel() = 0;

	/**
	 * The unit's max speed
	 */
public:
	virtual float GetMaxSpeed() = 0;

	/**
	 * The furthest any weapon of the unit can fire
	 */
public:
	virtual float GetMaxRange() = 0;

	/**
	 * The unit's max health
	 */
public:
	virtual float GetMaxHealth() = 0;

	/**
	 * How experienced the unit is (0.0f - 1.0f)
	 */
public:
	virtual float GetExperience() = 0;

	/**
	 * Returns the group a unit belongs to, -1 if none
	 */
public:
	virtual int GetGroup() = 0;

public:
	virtual std::vector<springai::Command*> GetCurrentCommands() = 0;

	/**
	 * The commands that this unit can understand, other commands will be ignored
	 */
public:
	virtual std::vector<springai::CommandDescription*> GetSupportedCommands() = 0;

	/**
	 * The unit's current health
	 */
public:
	virtual float GetHealth() = 0;

public:
	virtual float GetSpeed() = 0;

	/**
	 * Indicate the relative power of the unit,
	 * used for experience calulations etc.
	 * This is sort of the measure of the units overall power.
	 */
public:
	virtual float GetPower() = 0;

public:
	virtual float GetResourceUse(Resource* resource) = 0;

public:
	virtual float GetResourceMake(Resource* resource) = 0;

public:
	virtual springai::AIFloat3 GetPos() = 0;

public:
	virtual springai::AIFloat3 GetVel() = 0;

public:
	virtual bool IsActivated() = 0;

	/**
	 * Returns true if the unit is currently being built
	 */
public:
	virtual bool IsBeingBuilt() = 0;

public:
	virtual bool IsCloaked() = 0;

public:
	virtual bool IsParalyzed() = 0;

public:
	virtual bool IsNeutral() = 0;

	/**
	 * Returns the unit's build facing (0-3)
	 */
public:
	virtual int GetBuildingFacing() = 0;

	/**
	 * Number of the last frame this unit received an order from a player.
	 */
public:
	virtual int GetLastUserOrderFrame() = 0;

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
	virtual void Build(UnitDef* toBuildUnitDef, const springai::AIFloat3& buildPos, int facing, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Stop(short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Wait(short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void WaitFor(int time, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void WaitForDeathOf(Unit* toDieUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void WaitForSquadSize(int numUnits, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void WaitForAll(short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void MoveTo(const springai::AIFloat3& toPos, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void PatrolTo(const springai::AIFloat3& toPos, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Fight(const springai::AIFloat3& toPos, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Attack(Unit* toAttackUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void AttackArea(const springai::AIFloat3& toAttackPos, float radius, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Guard(Unit* toGuardUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void AiSelect(short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void AddToGroup(Group* toGroup, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void RemoveFromGroup(short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Repair(Unit* toRepairUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetFireState(int fireState, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetMoveState(int moveState, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetBase(const springai::AIFloat3& basePos, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SelfDestruct(short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetWantedMaxSpeed(float wantedMaxSpeed, short options = 0, int timeOut = INT_MAX) = 0;

	/**
	 * @param options  see enum UnitCommandOptions
	 */
public:
	virtual void LoadUnits(std::vector<springai::Unit*> toLoadUnitIds_list, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void LoadUnitsInArea(const springai::AIFloat3& pos, float radius, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void LoadOnto(Unit* transporterUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Unload(const springai::AIFloat3& toPos, Unit* toUnloadUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void UnloadUnitsInArea(const springai::AIFloat3& toPos, float radius, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetOn(bool on, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void ReclaimUnit(Unit* toReclaimUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void ReclaimFeature(Feature* toReclaimFeature, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void ReclaimInArea(const springai::AIFloat3& pos, float radius, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Cloak(bool cloak, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Stockpile(short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void DGun(Unit* toAttackUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void DGunPosition(const springai::AIFloat3& pos, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void RestoreArea(const springai::AIFloat3& pos, float radius, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetRepeat(bool repeat, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetTrajectory(int trajectory, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Resurrect(Feature* toResurrectFeature, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void ResurrectInArea(const springai::AIFloat3& pos, float radius, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void Capture(Unit* toCaptureUnit, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void CaptureInArea(const springai::AIFloat3& pos, float radius, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetAutoRepairLevel(int autoRepairLevel, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void SetIdleMode(int idleMode, short options = 0, int timeOut = INT_MAX) = 0;

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
	virtual void ExecuteCustomCommand(int cmdId, std::vector<float> params_list, short options = 0, int timeOut = INT_MAX) = 0;

}; // class Unit

}  // namespace springai

#endif // _CPPWRAPPER_UNIT_H

