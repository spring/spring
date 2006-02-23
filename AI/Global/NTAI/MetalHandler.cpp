
/*#include "./MetalHandler.h"
#include <string.h>
#include <vector>
#include <list>
#include <iterator>
#include "GlobalStuff.h"
#include "Sim/Units/UnitDef.h"*/
#include "helper.h"

#define SAVEGAME_VER 4

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
/** MAGIC NUMBERS **/

//#define MIN_HOTSPOT_METAL			60
//#define MIN_PATCH_METAL				9
//#define SEARCH_PASS					32
#define HOT_SPOT_RADIUS_MULTIPLYER	1.3f
#define NEAR_RADIUS			800
#define NEAR_RADIUS_2			1200
#define NEAR_RADIUS_3			2000
#define NEAR_RADIUS_4			4050
#define NEAR_RADIUS_5			8000
#define NEAR_RADIUS_6			12000
#define NEAR_RADIUS_7			20000

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

CMetalHandler::CMetalHandler(IAICallback *callback){
	cb=callback;
	m = new MetalMap(cb);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

CMetalHandler::~CMetalHandler(void){};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool metalcmp(float3 a, float3 b) { return a.y > b.y; }

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

//std::vector<float3> *CMetalHandler::parseMap(){
	/*std::vector<float3> *ov;
	std::list<float3> *mp=new std::list<float3>();
	float x,z;
	float offset=SEARCH_PASS;
	bool cont=false;
	const UnitDef* ud;
	const UnitDef* uw;
	if(cb->GetUnitDef("cormex") != 0) ud = cb->GetUnitDef("cormex");
	if(cb->GetUnitDef("armmex") != 0) ud = cb->GetUnitDef("armmex");
	if(cb->GetUnitDef("rhymex") != 0) ud = cb->GetUnitDef("rhymex");
	if(cb->GetUnitDef("TLLMEX") != 0) ud = cb->GetUnitDef("TLLMEX");
	if(cb->GetUnitDef("XCLMEX")!= 0) ud = cb->GetUnitDef("XCLMEX");
	if(cb->GetUnitDef("RACMETAL")!= 0) ud = cb->GetUnitDef("RACMETAL");
	if(cb->GetUnitDef("coruwmex") != 0) uw = cb->GetUnitDef("coruwmex");
	if(cb->GetUnitDef("coruwmex") != 0) uw = cb->GetUnitDef("coruwmex");
	for (x=0;x<cb->GetMapWidth()*8.0f;x+=offset){
		z=0;
		if (cont) z+=offset/2.0f;
		cont=!cont;
		for (z;z<cb->GetMapHeight()*8.0f;z+=offset){
				float3 pos((float)x,this->getExtractionRanged((x),(z)),(float)z);
				if (pos.y>MIN_PATCH_METAL){
					float m=pos.y;
					pos.y=cb->GetElevation(pos.x,pos.z);
					if(ud != 0){
                        if (cb->CanBuildAt(ud,pos)) {
							pos.y=m;
							mp->push_back(pos);
						}
					}
					if(uw != 0){
                        if (cb->CanBuildAt(uw,pos)) {
							pos.y=m;
							mp->push_back(pos);
						}
					}
				}
		}
	}
	ov=new std::vector<float3>();
	mp->sort(metalcmp);
	while (mp->size()>0) {
		float3 pos=mp->front();
		ov->push_back(pos);
		mp->pop_front();
	}
	delete(mp);
	return ov;*/
//};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void CMetalHandler::loadState() {
	m->init();
	hotspot = m->VectoredSpots;
	metalpatch = m->VectoredSpots;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

float CMetalHandler::getExtractionRanged(float x, float z) {
	float x1=x/16.0f;
	float z1=z/16.0f;
	float rad=cb->GetExtractorRadius()/16.0f;
	rad=rad*0.866f;
	float count=0.0f;
	float total=0.0f;
	for (float i=-rad;i<rad;i++){
		for (float j=-rad;j<rad;j++){
			total+=getMetalAmount((int)(x1+i),(int)(z1+j));
			++count;
		}
	}
	if (count==0) return 0.0;
	float ret=total/count;
	return ret;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

float CMetalHandler::getMetalAmount(int x, int z) {
	int sizex=(int)(cb->GetMapWidth()/2.0);
	int sizez=(int)(cb->GetMapHeight()/2.0);
	if(x<0)
		x=0;
	else if(x>=sizex)
		x=sizex-1;
	if(z<0)
		z=0;
	else if(z>=sizez)
		z=sizez-1;
	float val = (cb->GetMetalMap()[x + z*sizex]);
	//if(val<(cb->GetMaxMetal()*0.35f)) val *= 0.4f;
	//if(val>(cb->GetMaxMetal()*0.55f)) val *= 2.5f;
	return val;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

std::vector<float3> *CMetalHandler::getHotSpots(){
	return &(this->hotspot);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

std::vector<float3> *CMetalHandler::getMetalPatch(float3 pos, float minMetal,/*float radius,*/ float depth){
	std::vector<float3> *v=new std::vector<float3>();
	unsigned int i;
	float mrad=cb->GetExtractorRadius() + 0.1f;
	//min extractor dist = 2* radius (squared)
	mrad=3.0f*mrad*mrad;
	//research radius
	//radius *=radius;
	bool valid;
	for (i=0;i<this->metalpatch.size();i++){
		valid=true;
		float3 t1=metalpatch.at(i);
		if (t1.y<minMetal) break;
		//float d=(pos.x-t1.x)*(pos.x-t1.x)+(pos.z-t1.z)*(pos.z-t1.z);
		//if (d>radius) continue;
		for(std::vector<int>::iterator it=mex.begin();it!=mex.end();++it){
			if (const UnitDef *def=cb->GetUnitDef(*it)){
				if (def->extractsMetal<depth)	continue;
			} 
			float3 t2=cb->GetUnitPos((*it));
			float d1=(t1.x-t2.x)*(t1.x-t2.x)+(t1.z-t2.z)*(t1.z-t2.z);
			if (d1<mrad){
				valid=false;
				break;
			}
		}
		if (valid){
			v->push_back(t1);
		}
	}
	return v;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Cycle through each getting an array. If an array is returned with
// a value then return teh first value in the array. If the array
// returned is empty, wident eh search parameters and try again.
// If after 6 tries no spots have been found still, then return an error

float3 CMetalHandler::getNearestPatch(float3 pos, float minMetal, float depth, const UnitDef* ud) {
	std::vector<float3> *v=getMetalPatch(pos,minMetal,/*NEAR_RADIUS,*/depth);
	float3 posx = ZeroVector;
	float d = 1000000;
	float temp = 0;
	if(v->empty() == false){
		posx = v->front();
		d = posx.distance(pos);
		for(vector<float3>::iterator vi = v->begin(); vi != v->end(); ++vi){
			temp = vi->distance(pos);
			if((temp < d)&&(cb->CanBuildAt(ud,*vi) == true)){
				posx = *vi;
				d = temp;
				temp = 1000000;
			}
		}
	}
	delete(v);
	return posx;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
