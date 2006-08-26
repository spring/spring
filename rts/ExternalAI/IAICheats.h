#ifndef IAICHEATS_H
#define IAICHEATS_H

#include "aibase.h"
#include "Game/command.h"
#include "float3.h"
#include "IAICallback.h"
struct UnitDef;

class SPRING_API IAICheats
{
public:
	//note that all of these commands are network insecure
	//that is they are not transmitted over the network and will cause sync errors
	//so only use them with a single player

	virtual void SetMyHandicap(float handicap)=0;			//same as the value in the game setup file (currently gives bonus on collected resources)

	virtual void GiveMeMetal(float amount)=0;					//give selected amount of metal
	virtual void GiveMeEnergy(float amount)=0;				//give selected amount of energy;

	virtual int CreateUnit(const char* name,float3 pos)=0;		//creates a unit with the selected name at pos

	//the following commands works exactly like those in the standard interface except that they dont do any los checks
	virtual const UnitDef* GetUnitDef(int unitid)=0;	//this returns the units unitdef struct from which you can read all the statistics of the unit, dont try to change any values in it, dont use this if you dont have to risk of changes in it
	virtual float3 GetUnitPos(int unitid)=0;				//note that x and z are the horizontal axises while y represent height
	virtual int GetEnemyUnits(int *units)=0;					//returns all known enemy units
	virtual int GetEnemyUnits(int *units,const float3& pos,float radius)=0; //returns all known enemy units within radius from pos

	virtual int GetUnitTeam (int unitid)=0;
	virtual int GetUnitAllyTeam (int unitid)=0;
	virtual float GetUnitHealth (int unitid)=0;			//the units current health
	virtual float GetUnitMaxHealth (int unitid)=0;		//the units max health
	virtual float GetUnitPower(int unitid)=0;				//sort of the measure of the units overall power
	virtual float GetUnitExperience (int unitid)=0;	//how experienced the unit is (0.0-1.0)
	virtual bool IsUnitActivated (int unitid)=0; 
	virtual bool UnitBeingBuilt (int unitid)=0;			//returns true if the unit is currently being built
	virtual bool GetUnitResourceInfo (int unitid, UnitResourceInfo* resourceInfo)=0;
	virtual const deque<Command>* GetCurrentUnitCommands(int unitid)=0;
	
	virtual int GetBuildingFacing(int unitid)=0;		//returns building facing (0-3)
	virtual bool IsUnitCloaked(int unitid)=0;
	virtual bool IsUnitParalyzed(int unitid)=0;

	virtual bool OnlyPassiveCheats()=0;
	virtual void EnableCheatEvents(bool enable)=0;

	// future callback extensions
	virtual bool GetProperty(int id, int property, void *dst)=0;
	virtual bool GetValue(int id, void *dst)=0;
	virtual int HandleCommand(int commandId, void *data)=0;

	DECLARE_PURE_VIRTUAL(~IAICheats())
};

#endif
