//// Agent class declarations
#ifndef A_H
#define A_H
#include <map>
#include <vector>
#include "UnitDef.h"
#include "helper.h"
#include "MetalHandler.h"

class Assigner : public base{
public:
	Assigner(){}
	~Assigner(){
		myUnits.clear();
	}
	void InitAI(IAICallback* callback, THelper* helper);
	void UnitFinished(int unit);
	void Update();
	void GotChatMsg(const char *msg,int player);
	struct UnitInfo{
		float energyUse;
		bool turnedOn;
	};
	map<int,UnitInfo> myUnits;
	int lastUpdate;
	float lastEnergy;
};

// TAI Agents

// The chaser agent gets given units and when they
// reach a certain number, the agent searches
// for the nearest known enemy unit and attacks it
// then repeats the process untill all units
// are destroyed. Basically what the OTA AI did
class Chaser : public base{
public:
	Chaser(){}
	~Chaser();
	void InitAI(IAICallback* callback, THelper* helper);
	void Add(int unit);
	void UnitDestroyed(int unit);
	void UnitFinished(int unit);
	void EnemyDestroyed(int enemy);
	void EnemyEnterLOS(int enemy);
	void EnemyEnterRadar(int enemy);
	void FindTarget();
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);
private:
	list<int> Attackers;
	int acknum;
	float3 target;
	//Command* c;
};

// The scouter agent determines a patrol path that will
// take the unit around the entire map, thus showing
// some enemy units at some point for chaser to chase,
// and providing scouting and exploration. It takes these
// patrol paths and gives them to any scout unit that is
// built. Because of this, any aircraft that cannot build will
// be added to this, though this will change surely in 
// the future.
#define START 1
#define MOVINGTOCENTER 2
#define SCOUTING 3
class Scouter : public base{
public:
	Scouter(){}
	~Scouter(){
		delete c;
	}
	void InitAI(IAICallback* callback, THelper* helper);
	void UnitFinished(int unit);
private:
	float3 target;
	bool found;
	Command* c;
};

class Factor : public base{
public:
	Factor(){	}
	~Factor(){}
	void CBuild(string name,int unit);
	void FBuild(string name,int unit,int quantity);
	void UnitFinished(int unit);
	void Update();
	int GetId(string name);
	void UnitIdle(int unit);
private:
	struct udar{
		int unit;
		int frame;
		int creation;
		int BList;
		vector<string> tasks;
		int tasknum;
		bool guarding;
	};
	//struct fudar{
	///	int unit;
	//	int guardingme;
	//};
	vector<float3> history;
	vector<udar> builders;
	//vector<fudar> factorys;
	int klab1;
};
#endif 