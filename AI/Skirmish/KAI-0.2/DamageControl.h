#ifndef DAMAGECONTROL_H
#define DAMAGECONTROL_H

#include "GlobalAI.h"

class CDamageControl
{
public:
	CDamageControl(AIClasses* ai);
	virtual ~CDamageControl();
	void GenerateDPSTables();
	void UpdateTheirDistribution();
	void UpdateMyDistribution();
	void UpdateCombatScores();
	//checks the combat potential of this unit vs all active enemy units
	float GetCurrentDamageScore(const UnitDef* unit);
	//Gets the average Damage Per second a unit can cause against a equally expensive mix of all enemies
	float GetDPS(const UnitDef* unit);
	//Finds the actual dps versus a specific enemy unit
	float GetDPSvsUnit(const UnitDef* unit,const UnitDef* victim);
	float TheirTotalCost;
	float TheirTotalArmyCost; // Defence and attack only
	float MyTotalCost;

	vector<EconomyTotal> MyUnitsAll;
	vector<EconomyTotal> MyDefences;
	vector<EconomyTotal> MyArmy;
	vector<EconomyTotal> TheirUnitsAll;
	vector<EconomyTotal> TheirDefences;
	vector<EconomyTotal> TheirArmy;

private:
	int NumOfUnitTypes;

	AIClasses* ai;
};

#endif /* DAMAGECONTROL_H */
