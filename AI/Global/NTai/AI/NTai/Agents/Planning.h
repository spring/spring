#ifndef PLANNING_H
#define PLANNING_H

namespace ntai {

	class Planning{
	public:
		Planning(Global* GLI);
		virtual ~Planning(){}
		void InitAI();
		bool feasable(string s, int builder); // Antistall algorithm
		bool feasable(CUnitTypeData* building, CUnitTypeData* builder); // Antistall algorithm
		void Update();

		bool equalsIgnoreCase(string a ,string b);

		float GetEnergyIncome();

		/* Simulates the fluctuations of the resource based on the parameters given and returns the time in seconds at which the amount of resources stored falls below the given value */
		float FramesTillZeroRes(float in, float out, float starting, float maxres, float mtime, float minRes);

		// AAI algorithm
		// returns true if enough metal for constr.
		bool MetalForConstr(const UnitDef* ud, int workertime = 175);

		// AAI algorithm
		// returns true if enough energy for construction.
		bool EnergyForConstr(const UnitDef* ud, int workertime = 175);

		float GetMetalIncome();

		//int DrawLine(float3 LocA, float3 LocB, int LifeTime, float Width = 5.0f, bool Arrow = false);
		//int DrawLine(ARGB colours, float3 LocA, float3 LocB, int LifeTime, float Width = 5.0f, bool Arrow = false);
		float a;
		float b;
		float c;
		float d;
		vector<string> NoAntiStall;
		vector<string> AlwaysAntiStall;
		int fnum;

		// KAI economy antistall algorithm

		// Construction

		/*	Returns the metal per second needed to build this unit with this builder */
		float BuildMetalPerSecond(const UnitDef* builder,const UnitDef* built);

		/*	Returns the energy per second needed to build this unit with this builder */
		float BuildEnergyPerSecond(const UnitDef* builder,const UnitDef* built);

		/*	Returns the time taken for the builder to build the unit unassisted and with available resources
		 */
		float BuildTime(const UnitDef* builder,const UnitDef* built);

		/*	Returns whether building this unit will put resource reserves below the specified amounts with the current
		 *	income and expenditure */
		bool FeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinMpc = FEASIBLEMSTORRATIO, float MinEpc = FEASIBLEESTORRATIO);
		
		/*	Returns whether building this unit will put metal reserves below the specified amount with the current 
		 *	income and  expenditure*/
		bool MFeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinMpc = FEASIBLEMSTORRATIO);
		
		/*	Returns whether building this unit will put energy reserves below the specified amount with the current
		 *	income and expenditure */
		bool EFeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinEpc = FEASIBLEESTORRATIO);
		
		float ReclaimTime(const UnitDef* builder,const UnitDef* built,float health);
		float CaptureTime(const UnitDef* builder,const UnitDef* built,float health);

	private:
		Global* G;
	};
}

#endif
