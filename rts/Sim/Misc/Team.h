/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAM_H
#define TEAM_H

#include <string>
#include <vector>
#include <map>
#include <list>
#include <boost/utility.hpp> //! boost::noncopyable

#include "TeamBase.h"
#include "TeamStatistics.h"
#include "Sim/Units/UnitSet.h"
#include "ExternalAI/SkirmishAIKey.h"
#include "Lua/LuaRulesParams.h"
#include "System/Sync/SyncedPrimitive.h" //! SyncedFloat

class CTeam : public TeamBase, private boost::noncopyable //! cannot allow shallow copying of Teams, contains pointers
{
	CR_DECLARE(CTeam);
public:
	CTeam();
	~CTeam();
public:

	/**
	 * This has to be called for every team before SlowUpdates start,
	 * otherwise values get overwritten.
	 */
	void ResetFrameVariables();
	void SlowUpdate();

	void AddMetal(float amount, bool handicap = true);
	void AddEnergy(float amount, bool handicap = true);
	bool UseEnergy(float amount);
	bool UseMetal(float amount);
	bool UseEnergyUpkeep(float amount);
	bool UseMetalUpkeep(float amount);

	void GiveEverythingTo(const unsigned toTeam);

	void Died();

	void StartposMessage(const float3& pos);

	CTeam& operator=(const TeamBase& base);

	std::string GetControllerName() const;

	enum AddType {
		AddBuilt,
		AddCaptured,
		AddGiven
	};

	enum RemoveType {
		RemoveDied,
		RemoveCaptured,
		RemoveGiven
	};

	void AddUnit(CUnit* unit, AddType type);
	void RemoveUnit(CUnit* unit, RemoveType type);

	int teamNum;
	bool isDead;
	bool gaia;
	int lineageRoot;

	/// color info is unsynced
	unsigned char origColor[4];

	CUnitSet units;

	SyncedFloat metal;
	SyncedFloat energy;

	float metalPull,    prevMetalPull;
	float metalIncome,  prevMetalIncome;
	float metalExpense, prevMetalExpense;
	float metalUpkeep,  prevMetalUpkeep;

	float energyPull,    prevEnergyPull;
	float energyIncome,  prevEnergyIncome;
	float energyExpense, prevEnergyExpense;
	float energyUpkeep,  prevEnergyUpkeep;

	SyncedFloat metalStorage, energyStorage;

	float metalShare, energyShare;
	SyncedFloat delayedMetalShare, delayedEnergyShare; // excess that might be shared next SlowUpdate

	float metalSent;
	float metalReceived;
	float energySent;
	float energyReceived;

	int nextHistoryEntry;
	TeamStatistics* currentStats;
	std::list<TeamStatistics> statHistory;
	typedef TeamStatistics Statistics; //! for easier access via CTeam::Statistics

	/// number of units with commander tag in team, if it reaches zero with cmd ends the team dies
	int numCommanders;

	void CommanderDied(CUnit* commander);
	void LeftLineage(CUnit* unit);

	/// mod controlled parameters
	LuaRulesParams::Params  modParams;
	LuaRulesParams::HashMap modParamsMap; /// name map for mod parameters

	float highlight;
};

#endif /* TEAM_H */
