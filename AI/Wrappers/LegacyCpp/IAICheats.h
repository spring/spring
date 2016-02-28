/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_AI_CHEATS_H
#define I_AI_CHEATS_H

#include "IAICallback.h"
#include "Sim/Misc/GlobalConstants.h" // needed for MAX_UNITS
#include "System/float3.h"


namespace springLegacyAI {

struct UnitDef;
struct UnitResourceInfo;

/**
 * note that all of the non-readonly commands are network-
 * insecure: they aren't transmitted over the network and
 * thus will cause sync errors, so do not use them when 2
 * or more players are around (although you actually can't,
 * since they call OnlyPassiveCheats() internally)
 */
class IAICheats
{
public:
	/**
	 * This function has the same effect as setting a handicap value
	 * in the GameSetup script (currently gives a bonus on collected
	 * resources)
	 */
	virtual void SetMyHandicap(float handicap) = 0;

	/// give selected amount of metal
	virtual void GiveMeMetal(float amount) = 0;
	/// give selected amount of energy
	virtual void GiveMeEnergy(float amount) = 0;

	/// creates a unit with the selected name at pos
	virtual int CreateUnit(const char* name, float3 pos) = 0;


	/*
	 * the following commands work exactly like those in the standard
	 * callback interface, except that they don't do any LOS checks
	 */

	virtual const UnitDef* GetUnitDef(int unitId) = 0;

	virtual float3 GetUnitPos(int unitId) = 0;
	virtual float3 GetUnitVel(int unitId) = 0;

	virtual int GetEnemyUnits(int* unitIds, int unitIds_max = MAX_UNITS) = 0;
	virtual int GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = MAX_UNITS) = 0;
	virtual int GetNeutralUnits(int* unitIds, int unitIds_max = MAX_UNITS) = 0;
	virtual int GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = MAX_UNITS) = 0;

	virtual int GetFeatures(int *features, int max) = 0;
	virtual int GetFeatures(int *features, int max, const float3& pos,
			float radius) = 0;

	virtual int GetUnitTeam(int unitid) = 0;
	virtual int GetUnitAllyTeam(int unitid) = 0;
	virtual float GetUnitHealth(int unitid) = 0;
	virtual float GetUnitMaxHealth(int unitid) = 0;
	virtual float GetUnitPower(int unitid) = 0;
	virtual float GetUnitExperience(int unitid) = 0;
	virtual bool IsUnitActivated(int unitid) = 0;
	virtual bool UnitBeingBuilt(int unitid) = 0;
	virtual bool IsUnitNeutral(int unitid) = 0;
	virtual bool GetUnitResourceInfo(int unitid, UnitResourceInfo* resourceInfo) = 0;
	virtual const CCommandQueue* GetCurrentUnitCommands(int unitid) = 0;

	virtual int GetBuildingFacing(int unitid) = 0;
	virtual bool IsUnitCloaked(int unitid) = 0;
	virtual bool IsUnitParalyzed(int unitid) = 0;

	/// registers an AI as recipient of EnemyCreated, etc. events
	virtual void EnableCheatEvents(bool enable) = 0;

	virtual bool GetProperty(int id, int property, void* dst) = 0;
	virtual bool GetValue(int id, void* dst) = 0;
	virtual int HandleCommand(int commandId, void* data) = 0;

	// use virtual instead of pure virtual,
	// because pure virtual is not well supported
	// among different OSs and compilers,
	// and pure virtual has no advantage
	// if we have other pure virtual functions
	// in the class
	virtual ~IAICheats() {}
};

} // namespace springLegacyAI

#endif // I_AI_CHEATS_H
