#ifndef MATHS_H
#define MATHS_H

#include "GlobalAI.h"

#if defined WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sys/time.h>
  #define LARGE_INTEGER struct timeval
#endif

class CMaths {
	public:
		CMaths(AIClasses* ai);
		~CMaths();

		// sets the float3 so it is inside the game area
		void F3MapBound(float3* pos);
		// Returns a random point inside a circle centered on pos
		float3 F3Randomize(float3 pos, float radius);//Returns a random point inside a circle centered on pos

		// Converts a float3 into X Y coordinates for use in arrays, divided by this resolution
		void F32XY(float3 pos, int* x, int* y, int resolution = 1);
		// Converts X Y coordinates into a float3 for use in commands etc, divided by this resolution
		float3 XY2F3(int x, int y, int resolution = 1);

		// Returns the metal per second needed to build this unit with this builder
		float BuildMetalPerSecond(const UnitDef* builder, const UnitDef* built);
		// Returns the energy per second needed to build this unit with this builder
		float BuildEnergyPerSecond(const UnitDef* builder, const UnitDef* built);
		// Returns the time taken for the builder to build the unit unassisted and with available resources
		float BuildTime(const UnitDef* builder, const UnitDef* built);

		// Returns whether building this unit will put resource reserves below the specified amounts with the current income and expenditure
		bool FeasibleConstruction(const UnitDef* builder, const UnitDef* built, float MinMpc = FEASIBLEMSTORRATIO, float MinEpc = FEASIBLEESTORRATIO);
		// Returns whether building this unit will put metal reserves below the specified amount with the current income and expenditure
		bool MFeasibleConstruction(const UnitDef* builder, const UnitDef* built, float MinMpc = FEASIBLEMSTORRATIO);
		// Returns whether building this unit will put energy reserves below the specified amount with the current income and expenditure
		bool EFeasibleConstruction(const UnitDef* builder, const UnitDef* built, float MinEpc = FEASIBLEESTORRATIO);

		float GetUnitCost(const UnitDef* unit);
		float GetUnitCost(int unit);

		// Estimated time for the unit to arrive at location in a flat straight line, in seconds
		float ETA(int unit, float3 destination);
		// Estimated time for this buildtask to complete with the current builders with full resources
		float ETT(BuildTask bt);

		// RNGs
		float RandNormal(float m, float s, bool positiveonly = false);
		float RandFloat();
		unsigned int RandInt();

		// Timer Stuff
		void TimerStart();
		int TimerTicks();
		float TimerSecs();

	private:
		// Timer Stuff
		LARGE_INTEGER tick_start, tick_laststop, tick_end, ticksPerSecond;

		// Mersenne Twisters
		MTRand_int32 MTRandInt;
		MTRand MTRandFloat;

		AIClasses* ai;
		int mapfloat3height, mapfloat3width;
};


#endif
