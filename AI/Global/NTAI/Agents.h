//// Agent class declarations
#ifndef A_H
#define A_H
#include <list>
//#include "IAICallback.h"
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"
//#include <iostream>
//#include <fstream>
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

class Factor{
public:
	Factor(){	
		energy == "";
		race =0;
		water = false;
		G=0;
		start = false;
	}
	virtual ~Factor(){}
	bool CBuild(string name,int unit, int spacing = 9);
	bool FBuild(string name,int unit,int quantity);
	void UnitDamaged(int damaged, int attacker, float damage, float3 dir);
	void UnitFinished(int unit);
	void Update();
	void InitAI(Global* GLI);
	void UnitIdle(int unit);
	void UnitDestroyed(int unit);
	vector<Tunit> builders;
	Global* G;
	string energy;
	int race;
	//map<int,int> cfluke;
	bool water;
	vector<float3> pmex;
	bool start;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#endif 
