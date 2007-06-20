#ifndef TEAM_H
#define TEAM_H
// Team.h: interface for the CTeam class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>
#include <set>
#include <map>
#include <list>
class CUnit;
class CCity;

using namespace std;

class CTeam
{
public:
	CR_DECLARE(CTeam);
	CR_DECLARE_SUB(Statistics);
	CTeam();
	virtual ~CTeam();
	void SlowUpdate();

	void AddMetal(float amount);
	void AddEnergy(float amount);
	bool UseEnergy(float amount);
	bool UseMetal(float amount);
	bool UseEnergyUpkeep(float amount);
	bool UseMetalUpkeep(float amount);

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
	bool active;
	bool isDead;
	bool gaia;
	unsigned char color[4];
	int leader;
	int lineageRoot;

	float handicap;
	std::string side;

	list<CUnit*> units;
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

	float metalStorage, energyStorage;

	float metalShare, energyShare;
	float delayedMetalShare, delayedEnergyShare; //excess that might be shared next SlowUpdate

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
};

#endif /* TEAM_H */
