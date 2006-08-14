#ifndef CHASER_H
#define CHASER_H
#include <list>
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// The class used to store info about a task force and what units
// it has in it. I am going to have to move this towards using Tunit
// structures at some point and globalising the unit structures.
// Then I can keep things down in file size and make everything
// that much nicer.

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// The structure used to store grid data

struct agrid{
	int index;
	float3 centre;
	float3 location;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// The chaser class, deals with atatcking and defending.

struct esite {
	float efficiency;
	float3 position;
	float3 sector;
	bool ground;
	int created;
};

class Chaser{
public:
	Chaser();
	virtual ~Chaser();
	void InitAI(Global* GLI);
	void Add(int unit,bool aircraft=false);
	void Beacon(float3 pos, float radius);
	void UnitDestroyed(int unit,int attacker);
	void UnitFinished(int unit);
	void EnemyDestroyed(int enemy, int attacker);
//	bool FindTarget(ackforce* i, bool upthresh=true);
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);
	void UnitIdle(int unit);
	void Update();
	void FireSilos();
	void MakeTGA();
	void DepreciateMatrix();
	void UpdateMatrixFriendlyUnits();
	void RemoveBadSites();
	void UpdateSites();
	void UpdateMatrixEnemies();
	void CheckKamikaze();
	void FireDgunsNearby();
	void FireWeaponsNearby();
	void FireDefences();
	void DoUnitStuff(int aa);
	float* threat_matrix;
	int max_index;
	set<int> engaged;
	set<int> walking;

	int threshold;
	map<int, map<int,agrid> > sector;
	int xSectors, ySectors;		// number of sectors
	int xSize, ySize;			// size of sectors (in unit pos coordinates)
	set<int> Attackers;
	set<int> sweap;
	set<int> dgunning;
	set<int> defences;
	set<int> dgunners;
	float3 swtarget;
	map<string,bool> can_dgun;
	bool lock;
//	int acknum;
	Global* G;
	//vector<ackforce> groups;
	set<int> attack_units;
	int enemynum;
	vector<string> hold_pos;
	vector<string> maneouvre;
	vector<string> roam;
	vector<string> hold_fire;
	vector<string> return_fire;
	vector<string> fire_at_will;
	
	map<int,esite> ensites;

	set<int> kamikaze_units;
	map<string,bool> sd_proxim;
	
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#endif

