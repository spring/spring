#ifndef IAICHEATS_H
#define IAICHEATS_H

#include "Game/command.h"
#include "System/float3.h"
struct UnitDef;

class IAICheats
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
};

#endif
