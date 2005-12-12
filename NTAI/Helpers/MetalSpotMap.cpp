
//#include "BaseAIDef.h"

#include "MetalSpotMap.h"
#include "unitdef.h"

#define PI 3.141592654f
#define MAX_WORLD_SIZE 1000000;
#define SQUARE_SIZE 8
#define GAME_SPEED 30
#define RANDINT_MAX 0x7fff
#define MAX_VIEW_RANGE 8000
#define MAX_TEAMS 10
#define MAX_PLAYERS 16
#define METAL_MAP_SQUARE_SIZE (SQUARE_SIZE*2)
MetalSpotMap::MetalSpotMap(){
	w=h=0;

	metalblockw = 0;
	blockw=0;
	//debugShowSpots=0;
}

MetalSpotMap::~MetalSpotMap(){}
float3 MetalSpotMap::MPos2BuildPos(const UnitDef* ud, float3 pos)
{
	if(ud->xsize&2)
		pos.x=floor((pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.x=floor((pos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	if(ud->ysize&2)
		pos.z=floor((pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.z=floor((pos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	//pos.y=cb->GetElevation(pos.x,pos.z);
	//if(ud->floater && pos.y<0)
		pos.y =0;// -ud->waterline;

	return pos;
}
bool metalcomp(float3 a, float3 b) { return a.y > b.y; }

int2 MetalSpotMap::GetFrom3D (const float3& pos)
{
	int2 r;

	r.x = pos.x / (blockw*SQUARE_SIZE);
	r.y = pos.z / (blockw*SQUARE_SIZE);

	if (r.x < 0) r.x=0;
	if (r.y < 0) r.y=0;
	if (r.x >= w) r.x=w-1;
	if (r.y >= h) r.y=h-1;

	return r;
}

#define METAL_MAP_SQUARE_SIZE (SQUARE_SIZE*2)

void MetalSpotMap::Initialize (IAICallback* cb) {
	//const float* hm = cb->GetHeightMap();
	int hmw = cb->GetMapWidth ();
	int hmh = cb->GetMapHeight ();

	float exRadius = cb->GetExtractorRadius () / METAL_MAP_SQUARE_SIZE;
	//logPrintf("exRadius= %f\n", exRadius);
	bool isMetalMap = false;

	metalblockw = exRadius*2;
	
	const unsigned char* metalmap=cb->GetMetalMap ();


	//ulong totalMetal=0;
	unsigned char minMetalOnSpot = 50;
	freeSquare = new unsigned char[(hmw*hmh)/4];
	int halfMohSize=3;
	int minProduction=exRadius*exRadius*50;
	int count=0;
	for (int k=0; k<((hmw*hmh)/4); k++){ 
		freeSquare[k]=2; //2-free metal square; 0-mex structure is on it; 1-in mex radius(but not on mex building)
		if(metalmap[k]<50) count++;
	}
	if (count< ((hmw*hmh)/16)) isMetalMap=true;
	
	
	/*if(isMetalMap) {
		//int xRad=int(floor(exRadius))+1;
		//int xDiam=2*xRad;
		//if (metalblockw < 6) { // putting it on 6 means a temporary optimization, otherwise GetEmptySpot() will just slow things down too much
		//metalblockw = 6;
		//isMetalMap = true;
	//	}
		/*blockw = metalblockw*2;

		w = 1 + hmw / blockw;
		h = 1 + hmh / blockw;
		logPrintf ("Extractor Radius: %f, w=%d, h=%d, blockw=%d, metalblockw=%d\n", exRadius, w,h,blockw,metalblockw);
		
		int rad=int(floor(exRadius))+1;
		for (int my=rad;my<(hmh/2-rad);my++){
			for (int mx=rad;mx<(hmw/2-rad);mx++){
			float3 tmpProd1(0,0,0);
			
			float3 p(mx*16, 0, my*16);
			float3 pos = MPos2BuildPos(cb->GetUnitDef("cormex"), p);
			tmpProd1.x=pos.x;
			tmpProd1.z=pos.z;
			tmpProd1.y = (metalmap [my*hmw/2 + mx]*exRadius*exRadius*3)/4; 
							
			metalpatch.push_back(tmpProd1);
			}
		}
			//float3 pos = MPos2BuildPos(
	}
	*/
	if(!isMetalMap){

	for (int j = 0; j< (hmh/2); j++) {
		for (int i= 0; i<(hmw/2); i++) {
		
			if(metalmap[j*(hmw/2) +i] > minMetalOnSpot && freeSquare[j*(hmw/2) +i]==2) {
				int minx1=std::max(0, i-int(floor(exRadius)));
				int maxx1=std::min (hmw/2, i+int(floor(exRadius)));
				int maxy1=std::min(hmh/2, j+int(floor(exRadius)));
				
				std::list<float3> *tmpProd=new std::list<float3>();
 

				
				
				for (int mz=j; mz<maxy1;mz++) {
					for (int mx=minx1; mx<maxx1;mx++) {
						float3 tmpProd1(0,0,0);
						bool tmp=true;
						int dx1=mx-i;
						int dz1=mz-j;
						
						/*
							int xB = max(0, (mx - halfMohSize));
							int xE = min((hmw/2-1),(mx + halfMohSize));
							int zB = max(0,(mz - halfMohSize));
							int zE = min((hmh/2-1),(mz + halfMohSize));
							for (int z=zB;z<zE;z++) {
								for (int x=xB;x<xE;x++) {			
									
									if (freeSquare[z*hmw/2 +x]==0) { 
										tmp=false;
									
									}
								}
							}

						if(!tmp) continue;
						*/
						if (dx1*dx1+dz1*dz1 <= exRadius * exRadius) { 						
							//if (dx1*dx1+dz1*dz1 == exRadius * exRadius) logPrintf("%d:%d:DISTEQRADIUS\n",mx,mz);
							int xBegin = max(0, (mx - int(floor(exRadius))));
							int xEnd = min(hmw/2-1,(mx + int(floor(exRadius))));
							int zBegin = max(0,(mz - int(floor(exRadius))));
							int zEnd = min(hmh/2-1,(mz + int(floor(exRadius))));
							for (int z=zBegin;z<zEnd;z++) {
								for (int x=xBegin;x<xEnd;x++) {			
									int dx=x-mx;
									int dz=z-mz;
									if (dx*dx+dz*dz <= exRadius * exRadius) {
												if (freeSquare[z*hmw/2 +x]==2) {
										tmpProd1.y += metalmap [z*hmw/2 + x]; 
							
										}
									}
								}
							}
		

							tmpProd1.x=mx;
							tmpProd1.z=mz;
						
						tmpProd->push_back(tmpProd1);
						
						}
					}
				}
				tmpProd->sort(metalcomp);

	float3 bestSpot=tmpProd->front();
				
			if(bestSpot.y>minProduction) { 
				metalpatch.push_back(bestSpot);						
				int xBegin = max(0, (int(bestSpot.x) - int(floor(exRadius))));
				int xEnd = min(hmw/2-1,(int(bestSpot.x) + int(floor(exRadius))));
				int zBegin = max(0,(int(bestSpot.z) - int(floor(exRadius))));
				int zEnd = min(hmh/2-1,(int(bestSpot.z) + int(floor(exRadius))));
				for (int z=zBegin;z<zEnd;z++) {
					for (int x=xBegin;x<xEnd;x++) {			
						int dx=x-int(bestSpot.x);
						int dz=z-int(bestSpot.z);

						//if(dx<=halfMohSize && dz<=halfMohSize) { freeSquare[z*hmw/2 +x]=0; continue; }
						//else 
						if (dx*dx+dz*dz < exRadius * exRadius) { 
							freeSquare[z*hmw/2 +x]=1;
									
						}
					}
				}
			}				//freeSquare[bestSpot.z*hmw/2 +bestSpot.x]=0;
							//std::list<float3>::iterator it2=tmpProd.begin();
							//std::list<float3>::iterator it3=tmpProd.end();
							delete(tmpProd);//.erase(it2, it3);


				//numspots++;
				//averageProduction += b->metalProduction;

		}
	}
	
	}
	}
	metalpatch.sort(metalcomp);
	for (std::list<float3>::iterator it=metalpatch.begin();it!=metalpatch.end();++it) {
			it->x=it->x*16;
			it->z=it->z*16;
			//pos.y=cb->GetElevation(pos.x, pos.z);
			//cb->DrawUnit ("CORMEX", pos, 0.0f, 2000000, cb->GetMyAllyTeam(), false, true);
		}
}


