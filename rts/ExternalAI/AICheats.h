#ifndef AICHEATS_H
#define AICHEATS_H

#include "IAICheats.h"
class CGlobalAI;

class CAICheats :
	public IAICheats
{
	CGlobalAI* ai;
public:
	CAICheats(CGlobalAI* ai);
	~CAICheats(void);

	void SetMyHandicap(float handicap);			//same as the value in the game setup file (currently gives bonus on collected resources)

	void GiveMeMetal(float amount);					//give selected amount of metal
	void GiveMeEnergy(float amount);				//give selected amount of energy;

	int CreateUnit(const char* name,float3 pos);		//creates a unit with the selected name at pos

	//the following commands works exactly like those in the standard interface except that they dont do any los checks
	const UnitDef* GetUnitDef(int unitid);	//this returns the units unitdef struct from which you can read all the statistics of the unit, dont try to change any values in it, dont use this if you dont have to risk of changes in it
	float3 GetUnitPos(int unitid);				//note that x and z are the horizontal axises while y represent height
	int GetEnemyUnits(int *units);					//returns all known enemy units
	int GetEnemyUnits(int *units,const float3& pos,float radius); //returns all known enemy units within radius from pos

	int GetUnitTeam (int unitid);
	int GetUnitAllyTeam (int unitid);
	float GetUnitHealth (int unitid);			//the units current health
	float GetUnitMaxHealth (int unitid);		//the units max health
	float GetUnitPower(int unitid);				//sort of the measure of the units overall power
	float GetUnitExperience (int unitid);	//how experienced the unit is (0.0f-1.0f)
	bool IsUnitActivated (int unitid); 
	bool UnitBeingBuilt (int unitid);			//returns true if the unit is currently being built
	bool GetUnitResourceInfo (int unitid, UnitResourceInfo* resourceInfo);
	const deque<Command>* GetCurrentUnitCommands(int unitid);

	int GetBuildingFacing(int unitid);		//returns building facing (0-3)
	bool IsUnitCloaked(int unitid);
	bool IsUnitParalyzed(int unitid);

	bool OnlyPassiveCheats();
	void EnableCheatEvents(bool enable);

	// future callback extensions
	bool GetProperty(int unit, int property, void *dst);
	bool GetValue(int id, void *dst);
	int HandleCommand(int commandId, void *data); 
};

#endif
