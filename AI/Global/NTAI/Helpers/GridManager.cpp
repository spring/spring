#include "GridManager.h"
IAICallback* acb;
int mapwidth;
int mapheight;

int Gridset::GetIndex(float3 position){
	int x,z;
	x = position.x / gdata.x;
	z = position.z / gdata.z;
	if (x < 0) x=0;
	if (z < 0) z=0;
	if (x>=mapwidth) x=mapwidth-1;
	if (z>=mapheight) z=mapheight-1;
	return  z*mapwidth+x;
}

void Gridset::Init(int scalex,int scalez,int defaultval){
	acb->SendTextMsg("Gridset::init()",1);
	int mapw = mapwidth/scalex;
	if(mapw%scalex>0) mapw++;
	int maph = mapheight/scalez;
	acb->SendTextMsg("Gridset::init()2",1);
	if(maph%scalez>0) maph++;
	maxindex = mapw*maph;
	//Grid = new int [maxindex];
	Grid.assign(maxindex,defaultval);
	acb->SendTextMsg("Gridset::init()3",1);
	/*for(int hj = 0; hj < maxindex;hj++){
		Grid[hj] = defaultval;
	}*/
	acb->SendTextMsg("Gridset::init()4",1);
	gdata.x = mapw;
	gdata.z = maph;
	gdata.SectorWidth = scalex;
	acb->SendTextMsg("Gridset::init()5",1);
	gdata.SectorHeight = scalez;
}
Gridset::~Gridset(){
	//delete Grid;
}
int Gridset::GetMaxIndex(){
	return maxindex;
}

float3 Gridset::GetPos(int index){
	float3 pos;
	pos.z = (int)index/gdata.z;
	pos.x = (int)index%gdata.z;
	return pos;
}


GridManager::GridManager(IAICallback* aicb){
	acb=aicb;
	mapwidth = acb->GetMapWidth();
	mapheight = acb->GetMapHeight();
	gridnum = 0;
}
GridManager::~GridManager(){
}
int GridManager::create(int defaultval,int width,int height){
	acb->SendTextMsg("GridManager::create()",1);
	Gridset g;
	g.Init(width,height,defaultval);
	Grids.push_back(g);
	gridnum++;
	acb->SendTextMsg("GridManager::create() completed",1);
	return gridnum;
}
int GridManager::Get(int index,float3 pos){
	Gridset g;
	g = Grids[index];
	return g.Grid[g.GetIndex(pos)];
}
void GridManager::level(int index,int percentage){
	GridData gd = Grids[index].gdata;
	for(int qw = 0; qw<= (gd.z*gd.x);qw++){
		Grids[index].Grid[qw] *= percentage;
	}
}
void GridManager::level(int index,int percentage, float3 pos){
	Grids[index].Grid[Grids[index].GetIndex(pos)] *= percentage;
}
int GridManager::Put(int index,float3 pos,int value){
	Grids[index].Grid[Grids[index].GetIndex(pos)] = value;	
	return true;
}
int GridManager::PutVal(int Grid, int GridIndex,int value){
	acb->SendTextMsg("gv1",1);
	Gridset g = Grids[Grid];
	acb->SendTextMsg("gv2",1);
	g.Grid[GridIndex] = value;
	acb->SendTextMsg("gv3",1);
	acb->SendTextMsg("gv4",1);
	return true;
}

bool Gridset::DrawGrid(string fname){
	// open file
	FILE *fp=fopen(fname.c_str(), "wb");
	if(!fp)
		return false;

	// fill & write header
	char targaheader[18];
	memset(targaheader, 0, sizeof(targaheader));

	targaheader[2] = 2;		/* image type = uncompressed gray-scale */
	targaheader[12] = (char) (gdata.x & 0xFF);
	targaheader[13] = (char) (gdata.x >> 8);
	targaheader[14] = (char) (gdata.z & 0xFF);
	targaheader[15] = (char) (gdata.z >> 8);
	targaheader[16] = 24; /* 24 bit color */
	targaheader[17] = 0x20;	/* Top-down, non-interlaced */

	fwrite(targaheader, 18, 1, fp);
	// write file
	char out[3];
	for (int y=0;y<gdata.z;y++)
		for (int x=0;x<gdata.x;x++)
		{
			const int div=4;
			out[0]=min(255,(int)((y*gdata.z)+x) / div);
			out[1]=min(255,(int)((y*gdata.z)+x) / div);
			out[2]=min(255,(int)((y*gdata.z)+x) / div);
			fwrite(out, 3, 1, fp);
		}
	fclose(fp);

	return true;
}