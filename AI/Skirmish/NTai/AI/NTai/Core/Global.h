/*
AF 2004+
LGPL 2
*/

namespace ntai {

	void trim(string &str);
	void tolowercase(string &str);
	bool ValidUnitID(int id);


	// Global class implementation.

	class Global{
	public:
		//destructor/constructor
		virtual ~Global(); // destructor
		Global(IGlobalAICallback* callback); // constructor

		CMetalHandler* M; // mex positioning
		Log L; // Logging class

		Planning* Pl; // Deals with planning things, (only feasible() is actually used)
		Global* G; // pointer to this class so that macros work

		IAICheats* chcb;
		IAICallback* cb;// engine callback interface
		IGlobalAICallback* gcb;// global AI engine callback interface

		Chaser* Ch;// Chaser Agent, deals with attacking and things such as kamikaze units/dgunning/stockpiling missiles/several attack unit behaviours
		CUnitDefLoader* UnitDefLoader;

		boost::shared_ptr<CManufacturer> Manufacturer; // Construction helper
		boost::shared_ptr<CBuildingPlacer> BuildingPlacer; // Building placement algorithm
		CEconomy* Economy; // Construction rules (AAI/OTAI/JCAI style building selection)
		COrderRouter* OrderRouter; // Handles the caching of orders sent to the engine, aswell as a few other things like delayed orders etc...
		
		CMap* Map;
		CActions* Actions;
		
		CCached* Cached; // cached data or other data used across the AI at runtime
		CConfigData* info; // stuff loaded from configs that ideally should not change

		CDTHandler* DTHandler; // handles dragon teeth rings
		CRadarHandler* RadarHandler;
		//CTaskFactory* TaskFactory;


		std::map<std::string,std::string> unit_names; //unitname -> human name
		std::map<std::string,std::string> unit_descriptions; //unitname -> human name


		float max_energy_use;


		std::set<int> idlenextframe;
		// Interface functions
		void InitAI(IAICallback* callback, int team); // initialize the AI
		void UnitCreated(int unit); // a unit has been created, but this is currently not used.....
		void UnitFinished(int unit); // a unit has been finished (finished being built)
		void UnitDestroyed(int unit, int attacker); // a unit has been destroyed
		void EnemyEnterLOS(int enemy); // an enemy has entered LOS
		void EnemyLeaveLOS(int enemy); // An enemy has left LOS
		void EnemyEnterRadar(int enemy); // an enemy has entered radar
		void EnemyLeaveRadar(int enemy); // an enemy has left radar
		void EnemyDamaged(int damaged,int attacker,float damage,float3 dir);
		void EnemyDestroyed(int enemy,int attacker); // an enemy has been destroyed
		void UnitIdle(int unit); // a unit is now idle
		void GotChatMsg(const char* msg, int player); // received a console message
		void UnitDamaged(int damaged, int attacker, float damage,float3 dir); // a unit has been damaged
		void UnitMoveFailed(int unit); // a unit has failed to mvoe to its given location
		void Update();// called every frame (usually 30 frames per second)
		void SortSolobuilds(int unit);

		std::string ComName;
		void Crash();// Used to crash NTAI when the user types the ".crash" command

		bool InLOS(float3 pos);

		int GetEnemyUnits(int* units, const float3 &pos, float radius);
		int GetEnemyUnits(int* units);
		int GetEnemyUnitsInRadarAndLos(int* units);
		std::set<int> LOSGetEnemy(){// returns enemies in LOS (faster than the callback function)
			return Cached->enemies;
		}
		bool ReadFile(string filename, string* buffer); // reads a file in from the given path and shoves it in the buffer string provided
		TdfParser* Get_mod_tdf();// returns a TdfParser object loaded with the contents of mod.tdf

		//learning stuff
		void SetEfficiency(std::string s, float e);
		float GetEfficiency(std::string s, float def_value=500.0f); // returns the efficiency of the unit with name s.
		float GetEfficiency(std::string s,std::set<std::string>& doneconstructors,int techlevel=1); // returns the efficiency of the unit with name s, but not including those in the vector constructors if it's a builder to prevent recursive loops
		float GetTargettingWeight(std::string unit, std::string target);
		bool LoadUnitData(); // loads unit efficiency data from the learning file
		bool SaveUnitData(); // saves unit efficiency data back to file

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

		float3 GetUnitPos(int unitid,int enemy=0);// 1 = true, 2 = false, 0 = find out for us/wedunno

		MTRand_int32 mrand;

		bool HasUnit(int unit);
		CUnit* GetUnit(int unit);

		// event handling
		void RegisterMessageHandler(boost::shared_ptr<IModule> handler);
		void FireEvent(CMessage &message);
		void DestroyHandler(boost::shared_ptr<IModule> handler);
		void RemoveHandler(boost::shared_ptr<IModule> handler);

		CUnit** unit_array;

	private:
		std::set<boost::shared_ptr<IModule> > dead_handlers;
		std::set<boost::shared_ptr<IModule> > handlers;
		//std::map<int,boost::shared_ptr<IModule> > units;

		
		std::vector<CMessage> msgqueue;
	};

}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
