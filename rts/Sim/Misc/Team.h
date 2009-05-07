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
#include "Platform/byteorder.h"
#include "Sim/Units/UnitSet.h"
#include "ExternalAI/SkirmishAIKey.h"

class CTeam : public TeamBase
{
public:
	CR_DECLARE(CTeam);
	CR_DECLARE_SUB(Statistics);
	CTeam();
	~CTeam();
	void ResetFrameVariables();
	void SlowUpdate();

	void AddMetal(float amount, bool handicap = true);
	void AddEnergy(float amount, bool handicap = true);
	bool UseEnergy(float amount);
	bool UseMetal(float amount);
	bool UseEnergyUpkeep(float amount);
	bool UseMetalUpkeep(float amount);

	void SetBaseMetalStorage(float storage) {metalStorage = storage;};
	void SetBaseEnergyStorage(float storage) {energyStorage = storage;};

	void GiveEverythingTo(const unsigned toTeam);

	void Died();

	void StartposMessage(const float3& pos, const bool isReady);
	void StartposMessage(const float3& pos);

	inline bool IsReadyToStart() const {return readyToStart;};
	void operator=(const TeamBase& base) { TeamBase::operator=(base); };

	enum AddType{
		AddBuilt,
		AddCaptured,
		AddGiven
	};

	enum RemoveType{
		RemoveDied,
		RemoveCaptured,
		RemoveGiven
	};

	void AddUnit(CUnit* unit,AddType type);
	void RemoveUnit(CUnit* unit,RemoveType type);

	int teamNum;
	bool isDead;
	bool gaia;
	int lineageRoot;

	bool isAI;
	std::string luaAI;
	SkirmishAIKey skirmishAIKey;
	std::map<std::string, std::string> skirmishAIOptions;

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


	struct Statistics {
		CR_DECLARE_STRUCT(Statistics);
		float metalUsed,     energyUsed;
		float metalProduced, energyProduced;
		float metalExcess,   energyExcess;
		float metalReceived, energyReceived; // received from allies
		float metalSent,     energySent;     // sent to allies

		float damageDealt,   damageReceived; // Damage taken and dealt to enemy units

		int unitsProduced;
		int unitsDied;
		int unitsReceived;
		int unitsSent;
		/// units captured from enemy by us
		int unitsCaptured;
		/// units captured from us by enemy
		int unitsOutCaptured;
		/// how many enemy units have been killed by this teams units
		int unitsKilled;

		/// Change structure from host endian to little endian or vice versa.
		void swab() {
			metalUsed        = swabfloat(metalUsed);
			energyUsed       = swabfloat(energyUsed);
			metalProduced    = swabfloat(metalProduced);
			energyProduced   = swabfloat(energyProduced);
			metalExcess      = swabfloat(metalExcess);
			energyExcess     = swabfloat(energyExcess);
			metalReceived    = swabfloat(metalReceived);
			energyReceived   = swabfloat(energyReceived);
			metalSent        = swabfloat(metalSent);
			energySent       = swabfloat(energySent);
			damageDealt      = swabfloat(damageDealt);
			damageReceived   = swabfloat(damageReceived);
			unitsProduced    = swabdword(unitsProduced);
			unitsDied        = swabdword(unitsDied);
			unitsReceived    = swabdword(unitsReceived);
			unitsSent        = swabdword(unitsSent);
			unitsCaptured    = swabdword(unitsCaptured);
			unitsOutCaptured = swabdword(unitsOutCaptured);
			unitsKilled      = swabdword(unitsKilled);
		}
	};
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
