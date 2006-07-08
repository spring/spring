// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Helper class implementation.

#include "include.h"


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
using namespace std;

struct STGA{
	STGA(){
		data = (unsigned char*)0;
		width = 0;
		height = 0;
		byteCount = 0;
	}
	~STGA(){
		delete[] data;
		data = 0;
	}

	void destroy(){
		delete[] data;
		data = 0;
	}

	int width;
	int height;
	unsigned char byteCount;
	unsigned char* data;
};

class ctri { // structure used to store data on triangle markers
public:
	int creation;
	int lifetime;
	float3 position;
	float3 a;
	float3 b;
	float3 c;
	int d;
	int fade;
	float alpha;
	ARGB colour;
	float speed;
	bool bad;
	bool flashy;
};
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

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

	Assigner* As;// Assigner Agent, it's the equivalent of the metal maker AI but it handles moho mexes too
	Scouter* Sc;// Scouter agent, this deals with scouting the map
	Chaser* Ch;// Chaser Agent, deals with attacking and things such as kamikaze units/dgunning/stockpiling missiles/several attack unit behaviours

	CManufacturer* Manufacturer; // Construction Agent
	CEconomy* Economy; // Construction rules (AAI/OTAI/JCAI style building selection)
	COrderRouter* OrderRouter; // Handles the caching of orders sent to the engine, aswell as a few other things like delayed orders etc...
	CCached* Cached;
	CMap* Map;
	CActions* Actions;
	CInfo* info;
	CDTHandler* DTHandler; // handles dragon teeth rings
	CRadarHandler* RadarHandler;
	//CTaskFactory* TaskFactory;

	map<string,float> efficiency;
	map<string,string> unit_names; //unitname -> human name
	map<string,string> unit_descriptions; //unitname -> human name
	int iterations;
	bool loaded;
	bool firstload;
	bool saved;

	bool DrawTGA(string filename,float3 position);
	
	bool LoadTGA(const char *filename, STGA& tgaFile);
	set<int> idlenextframe;
	// Interface functions
	void InitAI(IAICallback* callback, int team); // initialize the AI
	void UnitCreated(int unit); // a unit has been created, but this is currently not used.....
	void UnitFinished(int unit); // a unit has been finished (finished being built)
	void UnitDestroyed(int unit, int attacker); // a unit has been destroyed
	void EnemyEnterLOS(int enemy); // an enemy has entered LOS
	void EnemyLeaveLOS(int enemy); // An enemy has left LOS
	void EnemyEnterRadar(int enemy); // an enemy has entered radar
	void EnemyLeaveRadar(int enemy); // an enemy has left radar
	void EnemyDestroyed(int enemy,int attacker); // an enemy has been destroyed
	void UnitIdle(int unit); // a unit is now idle
	void GotChatMsg(const char* msg, int player); // received a console message
	void UnitDamaged(int damaged, int attacker, float damage,float3 dir); // a unit has been damaged
	void UnitMoveFailed(int unit); // a unit has failed to mvoe to its given location
	void Update();// called every frame (usually 30 frames per second)
	void SortSolobuilds(int unit);

	string ComName;
	void Crash();// Used to crash NTAI when the user types the ".crash" command 
	
	bool InLOS(float3 pos);
	
	int GetEnemyUnits(int* units, const float3 &pos, float radius);
	int GetEnemyUnits(int* units);
	int GetEnemyUnitsInRadarAndLos(int* units);
	set<int> LOSGetEnemy(){// returns enemies in LOS (faster than the callback function)
		return Cached->enemies;
	}
	bool ReadFile(string filename, string* buffer); // reads a file in from the given path and shoves it in the buffer string provided
	string lowercase(string s);// convert a string to all lower case
	TdfParser* Get_mod_tdf();// returns a TdfParser object loaded with the contents of mod.tdf

	//Triangle & marker stuff
	void Draw(ctri triangle); // draws a triangle marker structure on the map
	ctri Tri(float3 pos, int size=20, float speed=6, int lifetime=900, int fade=3, int creation=1); // initializes a triangle marker structure
	void Increment(vector<ctri>::iterator triangle, int frame); // rotates and fades a triangle
	
	bool CanDGun(int uid);

	//learning stuff
	float GetEfficiency(string s); // returns the efficiency of the unit with name s.
	float GetTargettingWeight(string unit, string target);
	bool LoadUnitData(); // loads unit efficiency data from the learning file
	bool SaveUnitData(); // saves unit efficiency data back to file
	
	int GetCurrentFrame();
	const UnitDef* GetEnemyDef(int enemy);
	//int GiveOrder(TCommand c, bool newer=true);// Command cache

	const UnitDef* GetUnitDef(int unitid){
			return chcb->GetUnitDef(unitid);
	}
	const UnitDef* GetUnitDef(string s){
		return cb->GetUnitDef(s.c_str());
	}
	struct temp_pos{
		float3 pos;
		int last_update;
		bool enemy;
		string name;
	};
	map<int,temp_pos> positions;
	float3 GetUnitPos(int unitid,int enemy=0);// 1 = true, 2 = false, 0 = find out for us/wedunno
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
