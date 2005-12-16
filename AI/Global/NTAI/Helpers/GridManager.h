#include <map>
#include "float3.h"
#include "../rts/iaicallback.h"

struct GridData{
	int gridnumber;
	int SectorWidth;
	int SectorHeight;
	int x;
	int z;
};

class Gridset{
public:
	Gridset(){}
	void Init(int scalex,int scalez,int defaultval);
	~Gridset();
	int GetIndex(float3 position);
	float3 GetPos(int index);
	//vector<float3> GetHighSpots();
	//float3 GetHighNPos(float3 pos);
	bool DrawGrid(string fname);
	int GetMaxIndex();
	int maxindex;
	GridData gdata;
	vector<int> Grid;
};

class GridManager{
public:
	GridManager(){}
	GridManager(IAICallback* aicb);
	~GridManager();
	int create(int defaultval,int width,int height);
	int Get(int index,float3 pos);
	void level(int index,int percentage);
	void level(int index,int percentage, float3 pos);
	int Put(int index,float3 pos,int value);
	int PutVal(int Grid, int GridIndex,int value);
	int GetIndex(int Gridnum, float3 pos){
		return Grids[Gridnum].GetIndex(pos);
	}
	float3 GetPos(int Gridnum, int index){
		return Grids[Gridnum].GetPos(index);
	}
	bool DrawGrid(int Grid,string fname){
		return Grids[Grid].DrawGrid(fname);
	}
	int GetMaxIndex(int Grid){
		return Grids[Grid].GetMaxIndex();
	}
private:
    vector<Gridset> Grids;
	int gridnum;
	int mapwidth;
	int mapheight;
};