#ifndef TEAM_H
#define TEAM_H
// Team.h: interface for the CTeam class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <set>
#include <list>
class CUnit;
class CCity;

using namespace std;

class CTeam  
{
public:
	CTeam();
	virtual ~CTeam();
	void Update();

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
	unsigned char color[4];
	int colorNum;
	int leader;

	float handicap;
	std::string side;

	set<CUnit*> units;
	float3 startPos;

	double metal;
	double energy;

	float metalIncome,oldMetalIncome;
	float energyIncome,oldEnergyIncome;
	float tempMetalIncome;

	float metalUpkeep,oldMetalUpkeep;
	float energyUpkeep,oldEnergyUpkeep;

	float metalExpense,oldMetalExpense;
	float energyExpense,oldEnergyExpense;

	float metalStorage,energyStorage;

	float metalShare,energyShare;

	struct Statistics {
		double metalUsed,energyUsed;
		double metalProduced,energyProduced;
		double metalExcess,energyExcess;
		double metalReceived,energyReceived;					//received from allies
		double metalSent,energySent;									//sent to allies

		int unitsProduced;
		int unitsDied;
		int unitsReceived;
		int unitsSent;
		int unitsCaptured;				//units captured from enemy by us
		int unitsOutCaptured;			//units captured from us by enemy
		int unitsKilled;	//how many enemy units have been killed by this teams units
	};
	Statistics currentStats;

	int lastStatSave;
	int numCommanders;		//number of unints with commander tag in team, if it reaches zero with cmd ends the team dies
	std::list<Statistics> statHistory;
	void CommanderDied(CUnit* commander);
};

#endif /* TEAM_H */
