#pragma once
#include "GlobalAI.h"

class CMaths
{
public:
	CMaths(AIClasses *ai);	// Constructor
	virtual ~CMaths();		// Destructor

	// Float3 stuff
	void F3MapBound(float3* pos);	// sets the float3 so it is inside the game area
	float3 F3Randomize(float3 pos, float radius);	//Returns a random point inside a circle centered on pos

	void F32XY(float3 pos, int* x, int* y, int resolution = 1);	// Converts a float3 into X Y coordinates for use in arrays, divided by this resolution
	float3 XY2F3(int x ,int y, int resolution = 1);				// Converts X Y coordinates into a float3 for use in commands etc, divided by this resolution
	
	// Construction
	float BuildMetalPerSecond(const UnitDef* builder,const UnitDef* built);		// Returns the metal per second needed to build this unit with this builder
	float BuildEnergyPerSecond(const UnitDef* builder,const UnitDef* built);	// Returns the energy per second needed to build this unit with this builder
	float BuildTime(const UnitDef* builder,const UnitDef* built);				// Returns the time taken for the builder to build the unit unassisted and with available resources
	bool FeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinMpc = FEASIBLEMSTORRATIO, float MinEpc = FEASIBLEESTORRATIO); // Returns whether building this unit will put resource reserves below the specified amounts with the current income and expenditure 
	bool MFeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinMpc = FEASIBLEMSTORRATIO);		// Returns whether building this unit will put metal reserves below the specified amount with the current income and expenditure 
	bool EFeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinEpc = FEASIBLEESTORRATIO);		// Returns whether building this unit will put energy reserves below the specified amount with the current income and expenditure 
	float GetUnitCost(const UnitDef* unit);
	float GetUnitCost(int unit);

	float ETA(int unit, float3 destination); // Estimated time for the unit to arrive at location in a flat straight line, in seconds.
	float ETT(BuildTask bt);				// Estimated time for this buildtask to complete with the current builders with full resources

	// RNGs
	float			RandNormal(float m, float s, bool positiveonly = false);
	float			RandFloat();
	unsigned int	RandInt();

	// Timer Stuff
	void TimerStart();
	int	TimerTicks();
	float TimerSecs();





private:
	// Timer Stuff
	LARGE_INTEGER tick_start, tick_laststop, tick_end, ticksPerSecond;

	// Mersenne Twisters
	MTRand_int32	MTRandInt;
	MTRand			MTRandFloat;

	AIClasses *ai;	
	int mapfloat3height,mapfloat3width;

};