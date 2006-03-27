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
	Factor();
	virtual ~Factor(){
	//upsig.disconnect();
	}
	bool CBuild(string name,int unit, int spacing = 9);
	bool FBuild(string name,int unit,int quantity);
	void UnitDamaged(int damaged, int attacker, float damage, float3 dir);
	void UnitFinished(int unit);
	void Update();
	void InitAI(Global* GLI);
	void UnitIdle(int unit);
	void UnitDestroyed(int unit);
	Global* G;
	string energy;
	int race;
	map<int,int> cfluke;
	bool water;
	vector<float3> pmex;
	//map<int,float3> plans;
	//map<int,int> creations;
	//connection_t upsig;
	map<string, vector<string> > metatags;
	bool rand_tag;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#endif 
