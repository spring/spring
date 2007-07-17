#ifndef MATHS_H
#define MATHS_H

#include "GlobalAI.h"

struct TimerStruct
{
	LARGE_INTEGER tickStart;
	float sumTime;
};

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
	float ETT(BuildTask* bt);				// Estimated time for this buildtask to complete with the current builders with full resources

	// RNGs
	float			RandNormal(float m, float s, bool positiveonly = false);
	float			RandFloat();
	unsigned int	RandInt();

	// Timer Stuff
	void TimerStart();
	int	TimerTicks();
	float TimerSecs();
	
	/*
	Use this like this:
	static const int groupNumber = ai->cb->GetNewTimerGroupNumber("My function name");
	OPS: static means shared between all KAI's.  So dont use it...
	then:
	StartTimer(groupNumber);
	myFunction(); // Or code to time
	StopTimer(groupNumber);
	
	
	*/
	int GetNewTimerGroupNumber(string timerName);
	inline void StartTimer(int groupNumber)
	{
#ifdef WIN32
		QueryPerformanceCounter(&(timerStruct[groupNumber].tickStart));
#else
		gettimeofday(&(timerStruct[groupNumber].tickStart), NULL);
#endif
	}
	inline float StopTimer(int groupNumber)
	{
#ifdef WIN32
		QueryPerformanceCounter(&tick_temp);
		float time = (float(tick_temp.QuadPart - timerStruct[groupNumber].tickStart.QuadPart)) / ticksPerSecondDivisor;
		timerStruct[groupNumber].sumTime += time;
		return time;
#else
		gettimeofday(&tick_temp, NULL);
		float time = (tick_temp.tv_sec - timerStruct[groupNumber].tickStart.tv_sec) + (tick_temp.tv_usec - timerStruct[groupNumber].tickStart.tv_usec) * 1.0e-6f;
		timerStruct[groupNumber].sumTime += time;
		return time;
#endif
	}
	void PrintAllTimes();






private:
	// Timer Stuff
	LARGE_INTEGER tick_start, tick_laststop, tick_end, ticksPerSecond, tick_temp;
	float ticksPerSecondDivisor;
	// Mersenne Twisters
	MTRand_int32	MTRandInt;
	MTRand			MTRandFloat;

	AIClasses *ai;	
	int mapfloat3height,mapfloat3width;
	
	
	int groupNumberCounter;
	TimerStruct timerStruct[1000]; // Guessing that it wont be used more than 1000 times in the code
	
	string sumTimeNameList[1000]; // Guessing that it wont be used more than 1000 times in the code
};

#endif /* MATHS_H */
