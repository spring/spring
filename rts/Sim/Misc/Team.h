#ifndef TEAM_H
#define TEAM_H
// Team.h: interface for the CTeam class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <map>
#include <list>

#include "TeamBase.h"
#include "TeamStatistics.h"
#include "Sim/Units/UnitSet.h"
#include "ExternalAI/SkirmishAIKey.h"

class CTeam : public TeamBase
{
	CR_DECLARE(CTeam);
public:
	CTeam();
	~CTeam();
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

	void operator=(const TeamBase& base);

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

	typedef TeamStatistics Statistics;
	Statistics currentStats;
	/// in intervalls of this many seconds, statistics are updated
	static const int statsPeriod = 15;

	int lastStatSave;
	/// number of units with commander tag in team, if it reaches zero with cmd ends the team dies
	int numCommanders;
	std::list<Statistics> statHistory;
	void CommanderDied(CUnit* commander);
	void LeftLineage(CUnit* unit);

	/// mod controlled parameters
	std::vector<float>         modParams;
	/// name map for mod parameters
	std::map<std::string, int> modParamsMap;
};

#endif /* TEAM_H */
