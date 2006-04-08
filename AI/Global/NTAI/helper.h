// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Helper class implementation.

#include "include.h"


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
using namespace std;
/* for(int i = 0; i < totalcells; i++)   {
// 	float maxslope = 0;
// 	float tempslope;
// 	if(i+1 < totalcells && (i + 1) % PathMapXSize){   
// 		tempslope = abs(HeightMap[i] - HeightMap[i+1]);
// 		if(tempslope > maxslope)
// 			maxslope = tempslope;
// 	}
// 	if(i - 1 >= 0 && i % PathMapXSize){
// 		tempslope = abs(HeightMap[i] - HeightMap[i-1]);
// 		if(tempslope > maxslope)
// 			maxslope = tempslope;
// 	}
// 	if(i+PathMapXSize < totalcells){
// 		tempslope = abs(HeightMap[i] - HeightMap[i+PathMapXSize]);
// 		if(tempslope > maxslope)
// 			maxslope = tempslope;
// 	}
// 	if(i-PathMapXSize >= 0){
// 		tempslope = abs(HeightMap[i] - HeightMap[i-PathMapXSize]);
// 		if(tempslope > maxslope)
// 			maxslope = tempslope;
// 	}
}*/
/*template< class T> inline std::string to_string( const T & Value){
    std::stringstream streamOut;
    streamOut << Value;
    return streamOut.str( );
}
//  specialization for string.
template< > inline std::string to_string( const std::string & Value){
    return Value;
}
template< class T> inline T from_string( const std::string & ToConvert){
    std::stringstream streamIn( ToConvert);
    T ReturnValue = T( );
    streamIn >> ReturnValue;
    return ReturnValue;
}*/

struct TCommand{
#ifdef TC_SOURCE
	TCommand(string s):
	Priority(tc_na),
		created(0),
		unit(0),
		delay(0),
		source(s),
		clear(false){
			c.id = CMD_STOP;
			c.params.clear();
	}
#endif
#ifndef TC_SOURCE
	TCommand():
	Priority(tc_na),
		created(0),
		unit(0),
		delay(0),
		clear(false){
			c.id = CMD_STOP;
			c.params.clear();
	}
#endif
	bool operator>(TCommand& tc){
		return tc.Priority > Priority;
	}
	bool operator<(TCommand& tc){
		return tc.Priority < Priority;
	}
	int unit; // subject of this command
	int created; // frame number this command was created on
	t_priority Priority; // The priority of this command
	Command c; // The command itself
	bool clear;
	int delay;
#ifdef TC_SOURCE
	string source;
#endif
};

struct ctri {
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

class Global{ // derives from base, thus this is an agent not a global structure.
public:
	//destructor/constructor
	virtual ~Global();
	Global(IGlobalAICallback* callback);
	int randadd;

	//miscellaneous helpers + data
	CMetalHandler* M;
	Log L;
	Planning* Pl;
	Global* G;
	int unitallyteam;
	int team;
	int comID;

	// engine callback interface
	IAICallback* cb;
	
	// global AI engine callback interface
	IGlobalAICallback* gcb;

	// Assigner Agent, it's the equivalent of the metal maker AI but it handles moho mexes too
	Assigner* As;

	// Scouter agent, this deals with scouting the map
	Scouter* Sc;

	// Chaser Agent, deals with attacking and things such as kamikaze units/dgunning/stockpiling missiles/several attack unit behaviours
	Chaser* Ch;

	// Construction Agent
	Factor* Fa;

	// Signals
	//boost::signal0<void> update_signal;

	// Interface functions
	void InitAI(IAICallback* callback, int team);
	void UnitCreated(int unit){}
	void UnitFinished(int unit);
	void UnitDestroyed(int unit, int attacker);
	void EnemyEnterLOS(int enemy){
		enemies.insert(enemy);
	}
	void EnemyLeaveLOS(int enemy){
		enemies.erase(enemy);
	}
	void EnemyEnterRadar(int enemy){
		enemies.insert(enemy);
	}
	void EnemyLeaveRadar(int enemy){
		enemies.erase(enemy);
	}
	void EnemyDestroyed(int enemy,int attacker);
	void UnitIdle(int unit);
	void GotChatMsg(const char* msg, int player);
	void UnitDamaged(int damaged, int attacker, float damage,float3 dir);
	void UnitMoveFailed(int unit);

	// called every frame (usually 30 frames per second)
	void Update();

	// Used to crash NTAI when the user types the ".crash" command 
	void Crash();
	
	set<int> enemies;
	int GetEnemyUnits(int* units, const float3 &pos, float radius);
	int GetEnemyUnits(int* units);
	map<int,float3> GetEnemys();
	map<int,float3> GetEnemys(float3 pos, float radius);
	
	// cached enemy positions to speed up the process of calling the callback interface for the same data so many times
	map<int,float3> enemy_cache;

	int* encache;
	int enemy_number;
	int lastcacheupdate;

	bool ReadFile(string filename, string* buffer);

	// convert a string to all lower case
	string lowercase(string s);
	
	// returns a CSUNParser object loaded with the contents of mod.tdf
	CSunParser* Get_mod_tdf();
	
	//Triangle & marker stuff
	void Draw(ctri triangle);
	ctri Tri(float3 pos, float size=20, float speed=6, int lifetime=900, int fade=3, int creation=1);
	void Increment(vector<ctri>::iterator triangle, int frame);
	float3 Rotate(float3 pos, float angle, float3 origin);
	vector<ctri> triangles;

	//learning stuff
	float GetEfficiency(string s);
	bool LoadUnitData();
	bool SaveUnitData();
/*struct CMDat{

	bool abstract;
	bool gaia;
	bool spacemod;
	bool dynamic_selection;
	bool use_modabstracts;
	bool absent_abstract;
	float scout_speed;
	int fire_state_commanders;
	int move_state_commanders;

	CSunParser* mod_tdf;
};
	CMDat MDat;*/
	bool abstract;
	bool mexfirst;
	bool mexscouting;
	bool gaia;
	bool hardtarget;
	bool spacemod;
	bool dynamic_selection;
	bool use_modabstracts;
	bool absent_abstract;
	float scout_speed;
	int fire_state_commanders;
	int move_state_commanders;

	string tdfpath;

	// Command cache

	int GiveOrder(TCommand c, bool newer=true);
	list<TCommand> CommandCache;
	
	// Map stuff
	float3 basepos;
	vector<float3> base_positions;
	/* returns the base position nearest to the given float3*/
	float3 nbasepos(float3 pos);
	int WhichCorner(float3 pos);/*returns a value signifying which corner of the map this location is in*/
	vector<float3> GetSurroundings(float3 pos);/*returns the surrounding grid squares or locations of a co-ordinate*/
	float3 WhichSector(float3 pos)/*converts a normal co-ordinate into a Sector co-ordinate on the threat matrix*/;

	CSunParser* mod_tdf;
	vector<Tunit> units;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
