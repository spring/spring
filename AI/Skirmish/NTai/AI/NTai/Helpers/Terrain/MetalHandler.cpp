#include "../../Core/include.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
/** MAGIC NUMBERS **/

#define HOT_SPOT_RADIUS_MULTIPLYER	1.3f

namespace ntai {
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	CMetalHandler::CMetalHandler(Global* GLI){
		G = GLI;
		cb=G->cb;
		m = new CMetalMap(G);
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	CMetalHandler::~CMetalHandler(){
		delete m;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	bool metalcmp(float3 a, float3 b) {
		return a.y > b.y;
	}


	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void CMetalHandler::loadState() {
		m->Init();
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

		if (count==0){
			return 0.0f;
		}

		float ret=total/count;
		return ret;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	float CMetalHandler::getMetalAmount(int x, int z) {

		int sizex=(int)(cb->GetMapWidth()/2.0);
		int sizez=(int)(cb->GetMapHeight()/2.0);

		if(x<0){
			x=0;
		}else if(x>=sizex){
			x=sizex-1;
		}
		
		if(z<0){
			z=0;
		}else if(z>=sizez){
			z=sizez-1;
		}

		float val = (cb->GetMetalMap()[x + z*sizex]);

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

		mrad=3.0f*mrad*mrad;

		bool valid;
		for (i=0;i<this->metalpatch.size();i++){
			valid=true;

			float3 t1=metalpatch.at(i);

			if (t1.y<minMetal){
				continue;
			}

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
			
			float3 secpos = G->Ch->Grid.MaptoGrid(t1);
			
			if (valid){
				v->push_back(t1);
			}
		}
		return v;
	}

	bool CMetalHandler::Viable(){
		return true;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Cycle through each getting an array. If an array is returned with
	// a value then return teh first value in the array. If the array
	// returned is empty, wident eh search parameters and try again.
	// If after 6 tries no spots have been found still, then return an error

	float3 CMetalHandler::getNearestPatch(float3 pos, float minMetal, float depth, const UnitDef* ud) {
		std::vector<float3> *v=getMetalPatch(pos,minMetal,/*NEAR_RADIUS,*/depth);

		float3 posx = UpVector;

		float d = 6000.0f;
		float temp = 999999999.0f;
		float e_rad;

		G->Get_mod_tdf()->GetDef(e_rad,"900","AI\\mexnoweaponradius");

		if(v->empty() == false){
			posx = v->front();

			for(vector<float3>::iterator vi = v->begin(); vi != v->end(); ++vi){

				temp = vi->distance2D(pos);

				if((temp < d)&&(cb->CanBuildAt(ud,*vi) == true)){

					// check to see if this is in range of an enemy..............
					int* e = new int[2000];
					int en = G->GetEnemyUnits(e,*vi,e_rad);

					if( en > 0){
						for(int i = 0; i < en; i++){
							const UnitDef* ud = G->GetEnemyDef(e[i]);
							if(ud == 0){
								continue;
							}else{
								if(ud->weapons.empty()==false){
									continue;
								}
							}
							//
						}
					}

					delete [] e;

					// ok add it
					posx = *vi;
					d = temp;
					temp = 999999999.0f;
				}
			}
		}
		delete(v);
		return posx;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
}
