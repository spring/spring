// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Helper class implementation.

#include "include.h"


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
using namespace std;

struct TCommand{
	TCommand():
		Priority(tc_pseudoinstant),
		created(0),
		unit(0),
		delay(0),
			clear(false){};
	int unit; // subject of this command
	int created; // frame number this command was created on
	t_priority Priority; // The priority of this command
	Command c; // The command itself
	bool clear;
	int delay;
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

	//miscellaneous helpers + data
	CMetalHandler* M;
	Log L;
	Planning* Pl;
	Global* G;
	int unitallyteam;
	int team;
	int comID;

	// callback interfaces
	IAICallback* cb;
	IGlobalAICallback* gcb;
	IAICheats* ccb;

	// Agents
	Assigner* As;
	Factor* Fa;
	Scouter* Sc;
	Chaser* Ch;	
	
	// Interface functions
	void InitAI(IAICallback* callback, int team);
	void UnitCreated(int unit){}
	void UnitFinished(int unit);
	void UnitDestroyed(int unit, int attacker);
	void EnemyEnterLOS(int enemy){}
	void EnemyLeaveLOS(int enemy){}
	void EnemyEnterRadar(int enemy){}
	void EnemyLeaveRadar(int enemy){}
	void EnemyDestroyed(int enemy,int attacker);
	void UnitIdle(int unit);
	void GotChatMsg(const char* msg, int player);
	void UnitDamaged(int damaged, int attacker, float damage,float3 dir);
	void UnitMoveFailed(int unit);
	void Update();
	void Crash();
	
	bool abstract;

	// cached enemy positions to speed up the process of calling the callback interface for the same data so many times
	int GetEnemyUnits(int* units, const float3 &pos, float radius);
	int GetEnemyUnits(int* units);
	map<int,float3> GetEnemys();
	map<int,float3> GetEnemys(float3 pos, float radius);
	map<int,float3> enemy_cache;

	int* encache;
	int enemy_number;
	int lastcacheupdate;
	
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

	// Command cache

	int GiveOrder(TCommand c);
	list<TCommand> InstantCache;
	list<TCommand> PseudoCache;
	list<TCommand> AssignCache;
	list<TCommand> LowCache;
	int badcount;
	
	// Map stuff
	float3 basepos;
	vector<float3> base_positions;
	float3 nbasepos(float3 pos)/* returns the base position nearest to the given float3*/{ 
		if(base_positions.empty() == false){
			float3 best;
			float best_score = 20000000;
			for(vector<float3>::iterator i = base_positions.begin(); i != base_positions.end(); ++i){
				if( i->distance2D(pos) < best_score){
					best = *i;
					best_score = i->distance2D(pos);
				}
			}
			return best;
		}else{
			return basepos;
		}
	}
	int WhichCorner(float3 pos)/*returns a value signifying which corner of the map this location is in*/;
	vector<float3> Global::GetSurroundings(float3 pos)/*returns the surrounding grid squares or locations of a co-ordinate*/;
	float3 Global::WhichSector(float3 pos)/*converts a normal co-ordinate into a Sector co-ordinate on the threat matrix*/;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
