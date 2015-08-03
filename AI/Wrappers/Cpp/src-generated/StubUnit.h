/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBUNIT_H
#define _CPPWRAPPER_STUBUNIT_H

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
class StubUnit : public Unit {

protected:
	virtual ~StubUnit();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int unitId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitId(int unitId);
	// @Override
	virtual int GetUnitId() const;
	/**
	 * Returns the number of units a team can have, after which it can not build
	 * any more. It is possible that a team has more units then this value at
	 * some point in the game. This is possible for example with taking,
	 * reclaiming or capturing units.
	 * This value is usefull for controlling game performance, and will
	 * therefore often be set on games with old hardware to prevent lagging
	 * because of too many units.
	 */
private:
	int limit;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLimit(int limit);
	// @Override
	virtual int GetLimit();
	/**
	 * Returns the maximum total number of units that may exist at any one point
	 * in time induring the current game.
	 */
private:
	int max;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMax(int max);
	// @Override
	virtual int GetMax();
	/**
	 * Returns the unit's unitdef struct from which you can read all
	 * the statistics of the unit, do NOT try to change any values in it.
	 */
private:
	springai::UnitDef* def;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDef(springai::UnitDef* def);
	// @Override
	virtual springai::UnitDef* GetDef();
	/**
	 * This is a set of parameters that is created by SetUnitRulesParam() and may change during the game.
	 * Each parameter is uniquely identified only by its id (which is the index in the vector).
	 * Parameters may or may not have a name.
	 * @return visible to skirmishAIId parameters.
	 * If cheats are enabled, this will return all parameters.
	 */
private:
	std::vector<springai::UnitRulesParam*> unitRulesParams;/* = std::vector<springai::UnitRulesParam*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitRulesParams(std::vector<springai::UnitRulesParam*> unitRulesParams);
	// @Override
	virtual std::vector<springai::UnitRulesParam*> GetUnitRulesParams();
	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
	// @Override
	virtual springai::UnitRulesParam* GetUnitRulesParamByName(const char* rulesParamName);
	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
	// @Override
	virtual springai::UnitRulesParam* GetUnitRulesParamById(int rulesParamId);
private:
	int team;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTeam(int team);
	// @Override
	virtual int GetTeam();
private:
	int allyTeam;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAllyTeam(int allyTeam);
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
private:
	int aiHint;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAiHint(int aiHint);
	// @Override
	virtual int GetAiHint();
private:
	int stockpile;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStockpile(int stockpile);
	// @Override
	virtual int GetStockpile();
private:
	int stockpileQueued;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStockpileQueued(int stockpileQueued);
	// @Override
	virtual int GetStockpileQueued();
private:
	float currentFuel;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCurrentFuel(float currentFuel);
	// @Override
	virtual float GetCurrentFuel();
	/**
	 * The unit's max speed
	 */
private:
	float maxSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxSpeed(float maxSpeed);
	// @Override
	virtual float GetMaxSpeed();
	/**
	 * The furthest any weapon of the unit can fire
	 */
private:
	float maxRange;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxRange(float maxRange);
	// @Override
	virtual float GetMaxRange();
	/**
	 * The unit's max health
	 */
private:
	float maxHealth;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxHealth(float maxHealth);
	// @Override
	virtual float GetMaxHealth();
	/**
	 * How experienced the unit is (0.0f - 1.0f)
	 */
private:
	float experience;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetExperience(float experience);
	// @Override
	virtual float GetExperience();
	/**
	 * Returns the group a unit belongs to, -1 if none
	 */
private:
	int group;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGroup(int group);
	// @Override
	virtual int GetGroup();
private:
	std::vector<springai::Command*> currentCommands;/* = std::vector<springai::Command*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCurrentCommands(std::vector<springai::Command*> currentCommands);
	// @Override
	virtual std::vector<springai::Command*> GetCurrentCommands();
	/**
	 * The commands that this unit can understand, other commands will be ignored
	 */
private:
	std::vector<springai::CommandDescription*> supportedCommands;/* = std::vector<springai::CommandDescription*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSupportedCommands(std::vector<springai::CommandDescription*> supportedCommands);
	// @Override
	virtual std::vector<springai::CommandDescription*> GetSupportedCommands();
	/**
	 * The unit's current health
	 */
private:
	float health;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHealth(float health);
	// @Override
	virtual float GetHealth();
private:
	float speed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSpeed(float speed);
	// @Override
	virtual float GetSpeed();
	/**
	 * Indicate the relative power of the unit,
	 * used for experience calulations etc.
	 * This is sort of the measure of the units overall power.
	 */
private:
	float power;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPower(float power);
	// @Override
	virtual float GetPower();
	// @Override
	virtual float GetResourceUse(Resource* resource);
	// @Override
	virtual float GetResourceMake(Resource* resource);
private:
	springai::AIFloat3 pos;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPos(springai::AIFloat3 pos);
	// @Override
	virtual springai::AIFloat3 GetPos();
private:
	springai::AIFloat3 vel;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetVel(springai::AIFloat3 vel);
	// @Override
	virtual springai::AIFloat3 GetVel();
private:
	bool isActivated;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetActivated(bool isActivated);
	// @Override
	virtual bool IsActivated();
	/**
	 * Returns true if the unit is currently being built
	 */
private:
	bool isBeingBuilt;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBeingBuilt(bool isBeingBuilt);
	// @Override
	virtual bool IsBeingBuilt();
private:
	bool isCloaked;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCloaked(bool isCloaked);
	// @Override
	virtual bool IsCloaked();
private:
	bool isParalyzed;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetParalyzed(bool isParalyzed);
	// @Override
	virtual bool IsParalyzed();
private:
	bool isNeutral;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNeutral(bool isNeutral);
	// @Override
	virtual bool IsNeutral();
	/**
	 * Returns the unit's build facing (0-3)
	 */
private:
	int buildingFacing;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildingFacing(int buildingFacing);
	// @Override
	virtual int GetBuildingFacing();
	/**
	 * Number of the last frame this unit received an order from a player.
	 */
private:
	int lastUserOrderFrame;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLastUserOrderFrame(int lastUserOrderFrame);
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
}; // class StubUnit

}  // namespace springai

#endif // _CPPWRAPPER_STUBUNIT_H

