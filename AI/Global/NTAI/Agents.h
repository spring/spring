//// Agent class declarations
#ifndef A_H
#define A_H
#include <list>
//#include "IAICallback.h"
#include "AICallback.h"
#include "UnitDef.h"
//#include <iostream>
//#include <fstream>
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

class Factor : public base{
public:
	Factor(){	
		energy == "";
		race =0;
		water = false;
		threem = false;
		G=0;
	}
	virtual ~Factor(){}
	bool CBuild(string name,int unit);
	bool FBuild(string name,int unit,int quantity);
	void UnitFinished(int unit);
	void Update();
	void InitAI(Global* GLI);
	int GetId(string name);
	void UnitIdle(int unit);
	void UnitDestroyed(int unit);
	void UnitCreated(int unit);
	vector<Tunit*> builders;
	Global* G;
	string energy;
	int race;
	map<int,int> cfluke;
	bool water;
	bool threem;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#endif 
