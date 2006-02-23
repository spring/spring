#ifndef CHASER_H
#define CHASER_H
#include <list>
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// The class used to store info about a task force and what units
// it has in it. I am going to ahve to move this towards using Tunit
// structrues at soem point and globalising the unit structures.
// Then I can keep thigns down in filesize and make everything
// that much nicer.
enum a_type {a_mexraid, a_normal, a_random};
class ackforce : public ugroup{
public:
	ackforce(){}
	ackforce(Global* GLI){
		G = GLI;
		marching  = false;
	}
	virtual ~ackforce(){}
	bool marching;
//	int acknum;
	float3 starg;
	a_type type;
	int i;
};
struct T_Targetting{
	Global* G;
	ackforce* i;
	bool upthresh;
};

struct Ins{
	Global* G;
	ackforce i;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// The structure used to store grid data

struct agrid{
	float ebuildings;
	float ground_threat;
	set<int> buildings;
	float3 centre;
	float3 location;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// The chaser class, deals with atatcking and defending.



class Chaser{
public:
	Chaser(){
		//enemies = new int[10000];
		threshold = 4; // NTAI will never send a task force smaller than this number.
		G = 0;
		lock = false;
	}
	virtual ~Chaser();
	void InitAI(Global* GLI);
	void Add(int unit);
	void UnitDestroyed(int unit);
	void UnitFinished(int unit);
	void EnemyDestroyed(int enemy);
	void FindTarget(ackforce* i, bool upthresh=true);
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);
	void UnitIdle(int unit);
	void Update();
	void FireSilos();
	void MakeTGA();

	int threshold;
	map< int, map<int,agrid> > sector;
	int xSectors, ySectors;		// number of sectors
	int xSize, ySize;			// size of sectors (in unit pos coordinates)
	int xSizeMap, ySizeMap;		// size of sectors (in map coodrinates = 1/8 xSize)
	set<int> Attackers;
	set<int> sweap;
	float3 swtarget;
	bool lock;
//	int acknum;
	Global* G;
	vector<ackforce> groups;
	int enemynum;
	int forces;
	
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#endif
