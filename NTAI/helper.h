///////////////////////////////////////////
// Helper class implementation.
///////////////////////////////////////////
#ifndef H_H
#define H_H
#include "IAICallback.h"
#include "IGlobalAICallback.h"
#include "UnitDef.h"
#include <list>
#include <map>
#include <set>

using namespace std;
class D_Message{
public:
	D_Message(){}
	D_Message(string esource, string etype){this->flush(esource, etype);}
	~D_Message(){}
	void flush(){
		priority = 1;
		description = "";
		pos.x = pos.y = pos.z = 0;
		items.empty();
		source = "Debug:: Source unknown";
		items.clear();
		type = "Unknown";
	}
	void flush(string esource, string etype){
		flush();
		source = esource;
		type = etype;
	}
	string source;
	int format;
	string type;
	int priority;
	string description;
	float3 pos;
	vector<string> items;
};

class Offset : public float3{
public:
	Offset(){}
	Offset(float3 position);
	Offset(int x, int y, int z);
	Offset(int x, int y);
	~Offset(){}
	int get(int matrix){return matrixes[matrix];}
	void set(int matrix, int value){matrixes[matrix]=value;}
	void raise(int matrix, int value){matrixes[matrix]+=value;}
	void lower(int matrix, int value){matrixes[matrix]-=value;}
private:
	map<int,int> matrixes;
};
class TOffset : public Offset{
public:
	TOffset(){}
	TOffset(Offset* o){ this->offsets.push_back(o);}
	TOffset(float3 position);
	TOffset(vector<float3> positions);
	TOffset(vector<Offset*> tOffsets){offsets = tOffsets;}
	~TOffset(){}
	int get(int value); // returns an average (mean value.)
	void set(int matrix, int value);
	void raise(int matrix, int value);
	void lower(int matrix, int value);
	int mark;
	vector<Offset*> offsets;
};



class TUnit{
public:
	TUnit(int uid){id = uid;}
	~TUnit();
	int id;
	bool Taken;
	void attack(TOffset o);
	void move(TOffset o);
	void guard(TUnit* u);
	void destruct();
	void patrol(TOffset offsets);
	void execute(Command* c);
	void execute(vector<Command*> cv);
	const UnitDef* ud;
	int ownerteam;
};

struct GridData{
	int gridnumber;
	int SectorWidth;
	int SectorHeight;
	int x;
	int y;
};

class Gridset{
public:
	Gridset(){}
	Gridset(int x,int y,int gridset);
	~Gridset(){
		for(map<int,TOffset*>::iterator mi = Grid.begin(); mi != Grid.end(); ++mi){
			delete mi->second;
		}
	}
	Offset* TransCord(float3 position);
	float3 TransOff(Offset* o);
	GridData* gdata;
	map<int,TOffset*> Grid;
};
#define RACE_ARM 1
#define RACE_CORE 2
class THelper{
public:
	~THelper();
	THelper(IAICallback* callback);//Initialises the sector map and calls the threat matrix functions
	void print(string message);
	void post(string usource, string utype, string udescription);
	void msg(D_Message msg);
	string GameTime();
	bool Verbose();
	// below functions have not been written yet vvvvvvv
	void AddGridset(int gridsetnum, int sectwidth, int sectheight);
	Gridset* GetGridData(int gridsetnum){ return Grids[gridsetnum];}
	void DestroyGrid(int gridsetnum);
	void AddMatrix(int gridsetnum, int matrix, int defacto); // Add and initialise a matrix to a certain value for all offsets
	void DestroyMatrix(int gridsetnum, int matrix);
	bool MatrixPresent(int gridsetnum, int matrix); // Has this matrix been added?
	void MatrixPercentage(int gridsetnum, int matrix, int percentage); // increase or decrease all values for the specified matrix by this percentage.
	int MatrixDefault(int gridsetnum, int matrix){ return matrixes[matrix];}
	void AddUnit(int unit);
	void RemoveUnit(int unit){Unitmap.erase(unit);}
	TUnit* GetUnit(int unit){return Unitmap[unit];}
	map<int,TUnit*> GetUnitMap(){return Unitmap;}
	//TOffset getMetalPatch(TOffset pos, float radius, TUnit* tu);
	//float3 getNearestPatch(float3 pos, const UnitDef* ud);
	//void markpatch(TUnit* tu);
	//void unmarkpatch(TUnit* tu);
	Offset* TransCord(int Gridset, float3 position){
		return Grids[Gridset]->TransCord(position);
	}
	float3 TransOff(int Gridset, Offset* o){
		return this->Grids[Gridset]->TransOff(o);
	}
	int GetCommander(){return commander; }
	int GetRace(){return race;}
	float3 GetStartPos(){return startpos;}
	void Init();
private:
	bool verbose;
	map<int,int> matrixes;
	map<int,Gridset*> Grids;
	map<int,TUnit*> Unitmap;
	//CMetalHandler* MHandler;
	int race;
	int commander;
	float3 startpos;
};
class Agent{
public:
	Agent(){}
	~Agent(){}
	void InitAI(){}
	void Update(){}
};

class T_Agent : public Agent{
public:
	virtual void UnitCreated(int unit)=0;
	void UnitCreated(TUnit* unit){UnitCreated(unit->id);}
	virtual void UnitFinished(int unit)=0;
	void UnitFinished(TUnit* unit){UnitFinished(unit->id);}
	virtual void UnitDestroyed(int unit)=0;
	void UnitDestroyed(TUnit* unit){UnitDestroyed(unit->id);}
	virtual void EnemyEnterLOS(int enemy)=0;
	void EnemyEnterLOS(TUnit* unit){EnemyEnterLOS(unit->id);}
	virtual void EnemyLeaveLOS(int enemy)=0;
	void EnemyLeaveLOS(TUnit* unit){EnemyLeaveLOS(unit->id);}
	virtual void EnemyEnterRadar(int enemy)=0;
	void EnemyEnterRadar(TUnit* unit){EnemyEnterRadar(unit->id);}
	virtual void EnemyLeaveRadar(int enemy)=0;
	void EnemyLeaveRadar(TUnit* unit){EnemyLeaveRadar(unit->id);}
	virtual void EnemyDestroyed(int enemy)=0;
	void EnemyDestroyed(TUnit* unit){EnemyDestroyed(unit->id);}
	virtual void UnitIdle(int unit)=0;
	void UnitIdle(TUnit* unit){UnitIdle(unit->id);}
	virtual void GotChatMsg(const char* msg,int player)=0;
	virtual void UnitDamaged(int damaged,int attacker,float damage,float3 dir)=0;
};

class base : public T_Agent{
public:
	base(){}
	~base(){}
	void InitAI(IAICallback* callback, THelper* helper){}
	void Update(){}
	void Add(int unit){}
	void Remove(int unit){}
	std::set<int> ListUnits(){ return Units;}
	void UnitCreated(int unit){}
	void UnitFinished(int unit){}
	void UnitDestroyed(int unit){}
	void EnemyEnterLOS(int enemy){}
	void EnemyLeaveLOS(int enemy){}
	void EnemyEnterRadar(int enemy){}
	void EnemyLeaveRadar(int enemy){}
	void EnemyDestroyed(int enemy){}
	void UnitIdle(int unit){}
	void GotChatMsg(const char* msg,int player){}
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir){}
protected:
	std::set<int> Units;
};
class UGroup : public base{
public:
	UGroup();
	UGroup(TUnit* unit);
	~UGroup();
	void attack(TOffset o);
	void move(TOffset o);
	void guard(TUnit* u);
	void destruct();
	void patrol(TOffset offsets);
	vector<TUnit> GetUnits();
private:
	vector<TUnit*> units;
};


/*class FClass{
public:
	FClass();
	~FClass();
	void Write(string a);
	void Open(string file);
	void Close();
private:
	string filename;
	FILE* debugfile;
};*/


#endif