#ifndef TEAM_H
#define TEAM_H
// Team.h: interface for the CTeam class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <set>
#include <map>
#include <list>
#include "Platform/byteorder.h"
#include "Sim/Units/UnitSet.h"

class CTeam
{
public:
	CR_DECLARE(CTeam);
	CR_DECLARE_SUB(Statistics);
	CTeam();
	~CTeam();
	void SlowUpdate();


	void AddMetal(float amount);
	void AddEnergy(float amount);
	bool UseEnergy(float amount);
	bool UseMetal(float amount);
	bool UseEnergyUpkeep(float amount);
	bool UseMetalUpkeep(float amount);

	void SetBaseMetalStorage(float storage) {metalStorage = storage;};
	void SetBaseEnergyStorage(float storage) {energyStorage = storage;};
	
	void SelfDestruct();
	void GiveEverythingTo(const unsigned toTeam);
	
	void Died();

	void StartposMessage(const float3& pos, const bool isReady);
	void StartposMessage(const float3& pos);
	
	inline bool IsReadyToStart() const {return readyToStart;};
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
	int leader;
	int lineageRoot;

	float handicap;
	std::string side;

	bool isAI;
	std::string luaAI;
	std::string dllAI;

	// color info is unsynced
	unsigned char color[4];
	unsigned char origColor[4];

	CUnitSet units;

	float3 startPos;

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
	SyncedFloat delayedMetalShare, delayedEnergyShare; //excess that might be shared next SlowUpdate

	float metalSent;
	float metalReceived;
	float energySent;
	float energyReceived;


	struct Statistics {
		CR_DECLARE_STRUCT(Statistics);
		float metalUsed,     energyUsed;
		float metalProduced, energyProduced;
		float metalExcess,   energyExcess;
		float metalReceived, energyReceived; //received from allies
		float metalSent,     energySent;     //sent to allies

		float damageDealt,   damageReceived; // Damage taken and dealt to enemy units

		int unitsProduced;
		int unitsDied;
		int unitsReceived;
		int unitsSent;
		int unitsCaptured;				//units captured from enemy by us
		int unitsOutCaptured;			//units captured from us by enemy
		int unitsKilled;	//how many enemy units have been killed by this teams units

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
	static const int statsPeriod = 15; // every 15th second

	int lastStatSave;
	int numCommanders;		//number of units with commander tag in team, if it reaches zero with cmd ends the team dies
	std::list<Statistics> statHistory;
	void CommanderDied(CUnit* commander);
	void LeftLineage(CUnit* unit);

	std::vector<float>         modParams;    // mod controlled parameters
	std::map<std::string, int> modParamsMap; // name map for mod parameters

private:
	bool readyToStart;
};

#endif /* TEAM_H */
