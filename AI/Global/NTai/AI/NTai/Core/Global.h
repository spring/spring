// ||||||||||||||||||||||||||||||||||||||||||||||||
// Global class implementation.

#include "CGlobalProxy.h"

namespace ntai {

	void trim(string &str);
	void tolowercase(string &str);
	bool ValidUnitID(int id);


	class Global{
	public:
		//destructor/constructor
		virtual ~Global(); // destructor
		Global(IGlobalAICallback* callback); // constructor

		/* 
		 * A class that implements what this class implements
		 * using events so that theyre correctly throttle
		 */
		boost::shared_ptr<IModule> gproxy;

		/*
		 * mex positioning
		 */
		CMetalHandler* M; 

		/*
		 * Logging class
		 */
		Log L; 

		/*
		 * Deals with planning things, (only feasible() is
		 * actually used)
		 */
		Planning* Pl;

		/*
		 * pointer to this class so that macros work
		 */
		Global* G;

		IAICheats* chcb;

		/*
		 * engine callback interface
		 */
		IAICallback* cb;

		/*
		 * global AI engine callback interface
		 */
		IGlobalAICallback* gcb;

		/*
		 * Chaser Agent, deals with attacking and things such
		 * as kamikaze units/dgunning/stockpiling missiles/several
		 * attack unit behaviours
		 */
		Chaser* Ch;

		CUnitDefLoader* UnitDefLoader;

		/*
		 * Construction helper
		 */
		boost::shared_ptr<CManufacturer> Manufacturer;

		/*
		 * Building placement algorithm
		 */
		boost::shared_ptr<CBuildingPlacer> BuildingPlacer;

		/*
		 * Construction rules (AAI/OTAI/JCAI style building selection)
		 */
		CEconomy* Economy;

		/* 
		 * Handles the caching of orders sent to the engine, aswell
		 * as a few other things like delayed orders etc...
		 */
		COrderRouter* OrderRouter;
		
		CMap* Map;

		CActions* Actions;
		
		/* 
		 * Cached data or other data used across the AI at runtime
		 */
		CCached* Cached;

		/*
		 * stuff loaded from configs that ideally should not change

		 */
		CConfigData* info;

		/*
		 * handles dragon teeth rings
		 */
		CDTHandler* DTHandler;
		
		CRadarHandler* RadarHandler;


		/*
		 * unitname -> human name
		 */
		std::map<std::string,std::string> unit_names;

		/*
		 * unitname -> human name
		 */
		std::map<std::string,std::string> unit_descriptions;


		float max_energy_use;


		std::set<int> idlenextframe;


		// Interface functions

		/*
		 * initialize the AI
		 */
		void InitAI(IAICallback* callback, int team);

		/*
		 * a unit has been created, but this is currently not used.....
		 */
		void UnitCreated(int unit);

		/*
		 * a unit has been finished (finished being built)
		 */
		void UnitFinished(int unit);

		/*
		 * a unit has been destroyed
		 */
		void UnitDestroyed(int unit, int attacker);

		/*
		 * an enemy has entered LOS
		 */
		void EnemyEnterLOS(int enemy);

		/*
		 * An enemy has left LOS
		 */
		void EnemyLeaveLOS(int enemy);

		/*
		 * an enemy has entered radar
		 */
		void EnemyEnterRadar(int enemy);

		/*
		 * an enemy has left radar
		 */
		void EnemyLeaveRadar(int enemy);

		/*
		 * 
		 */
		void EnemyDamaged(int damaged,int attacker,float damage,float3 dir);

		/*
		 * an enemy has been destroyed
		 */
		void EnemyDestroyed(int enemy,int attacker);

		/*
		 * a unit is now idle
		 */
		void UnitIdle(int unit);

		/*
		 * received a console message
		 */
		void GotChatMsg(const char* msg, int player);

		/*
		 * a unit has been damaged
		 */
		void UnitDamaged(int damaged, int attacker, float damage,float3 dir);

		/*
		 * a unit has failed to move to its given location
		 */
		void UnitMoveFailed(int unit);

		/*
		 * called every game frame (usually 30 frames per second)
		 */
		void Update();// 

		void SortSolobuilds(int unit);

		std::string ComName;

		/*
		 * Used to crash NTAI when the user types the ".crash" command
		 */
		void Crash();

		bool InLOS(float3 pos);

		int GetEnemyUnits(int* units, const float3 &pos, float radius);

		int GetEnemyUnits(int* units);

		int GetEnemyUnitsInRadarAndLos(int* units);

		/*
		 * returns enemies in LOS (faster than the callback function)
		 */
		std::set<int> LOSGetEnemy(){
			return Cached->enemies;
		}
		
		/*
		 * reads a file in from the given path and shoves it in the
		 * buffer string provided
		 */
		bool ReadFile(string filename, string* buffer); // 
		
		/*
		 * returns a TdfParser object loaded with the contents of
		 * mod.tdf
		 */
		TdfParser* Get_mod_tdf();

		//learning stuff
		
		void SetEfficiency(std::string s, float e);
		
		/*
		 * returns the efficiency of the unit with name s.
		 */
		float GetEfficiency(std::string s, float def_value=500.0f);
		
		/*
		 * returns the efficiency of the unit with name s,
		 * but not including those in the vector constructors
		 * if it's a builder to prevent recursive loops.
		 */
		float GetEfficiency(std::string s,std::set<std::string>& doneconstructors,int techlevel=1);
		
		float GetTargettingWeight(std::string unit, std::string target);

		/*
		 * loads unit efficiency data from the learning file
		 */
		bool LoadUnitData();
		
		/*
		 * saves unit efficiency data back to file
		 */
		bool SaveUnitData();

		int GetCurrentFrame();
		const UnitDef* GetEnemyDef(int enemy);

		const UnitDef* GetUnitDef(int unitid){
			if(!ValidUnitID(unitid)){
				return 0;
			}
			//if(Cached->cheating){
				return chcb->GetUnitDef(unitid);
			/*}else{
				return cb->GetUnitDef(unitid);
			}*/
		}

		/*
		 * 1 = true, 2 = false, 0 = find out for us/dunno
		 */
		float3 GetUnitPos(int unitid,int enemy=0);

		MTRand_int32 mrand;

		bool HasUnit(int unit);
		boost::shared_ptr<IModule> GetUnit(int unit);

		// event handling
		void RegisterMessageHandler(boost::shared_ptr<IModule> handler);
		void FireEvent(CMessage &message);
		void DestroyHandler(boost::shared_ptr<IModule> handler);
		void RemoveHandler(boost::shared_ptr<IModule> handler);

	private:
		std::set<boost::shared_ptr<IModule> > dead_handlers;
		std::set<boost::shared_ptr<IModule> > handlers;
		std::map<int,boost::shared_ptr<IModule> > units;
		std::vector<CMessage> msgqueue;
	};

}
