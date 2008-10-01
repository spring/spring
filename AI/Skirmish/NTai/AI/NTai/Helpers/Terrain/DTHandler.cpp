/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../../Core/include.h"

namespace ntai {

	float3 CDTHandler::GetDTBuildSite(float3 pos){
		if(DTRings.empty() == true) return UpVector;
		if(G->Map->CheckFloat3(pos)==false){
			return UpVector;
		}
		vector<float3> locations;
		for(vector<DTRing>::iterator i = this->DTRings.begin(); i != DTRings.end(); ++i){
			// go through DT Rings and add all locations without a DT on them to the locations container;
			for(vector<float3>::iterator d = i->DTPos.begin(); d != i->DTPos.end(); ++d){
				int* f = new int[3];
				int h = G->cb->GetFeatures(f,2,*d,40);
				delete [] f;
				if(h == 0) locations.push_back(*d);
			}
		}
		if(locations.empty()==true) return UpVector;
		float3 closest = locations.front();
		
		float distance = pos.distance2D(closest);
		float tempdist=0;
		for(vector<float3>::iterator j = locations.begin(); j != locations.end(); ++j){
			tempdist = j->distance2D(pos);
			if(tempdist < distance){
				distance = tempdist;
				closest = *j;
			}
		}
		
		return closest;
	}

	CDTHandler::CDTHandler(Global* GL){
		//NLOG("CDTHandler::CDTHandler()");
		G = GL;
		// Find ID of DT
		/*string filename = G->info->datapath + slash + "DTBuildData" + slash + G->info->tdfpath + string(".DTBuildData");
		ofstream DTBuildDataOut(filename.c_str(), ios::binary);
		int unum = G->cb->GetNumUnitDefs();
		const UnitDef** UnitDefList = new const UnitDef*[unum];
		G->cb->GetUnitDefList(UnitDefList);
		for(int n=0; n < unum; n++){
			const UnitDef* pud = UnitDefList[n];
			if(pud->buildSpeed > 0.0f){
				for(map<int, string>::const_iterator bit = pud->buildOptions.begin(); bit != pud->buildOptions.end(); bit++){
            		const UnitDef* pud2 = G->cb->GetUnitDef(bit->second.c_str());
					if(pud2->isFeature && !pud2->floater){
						DTBuilds.insert(map<int, int>::value_type(n, pud2->id));
						DTBuildDataOut << n << pud2->id;
						break;
					}
				}
			}
		}
		DTBuildDataOut.close();*/
	}

	void CDTHandler::AddRing(float3 _loc, float _Radius , float _Twist){
		NLOG("CDTHandler::AddRing()");
		// Setup Dragon's Teeth
		//Setting up DT Positions...
		float Angle = 0.0f;
		const float maxx = G->cb->GetMapWidth()*8.0f;
		const float maxz = G->cb->GetMapHeight()*8.0f;
		int CurDT = 0;
		// Find direction to start, and start with 90 Degrees from that.
		float Rotate = (float)atan((maxz/2 - _loc.z) / (maxx/2 - _loc.x)) - (float)PI_2;
		int NumSteps = ((int)(PIx2 * _Radius / DT_SIZE));
		const float AngleDelta = (1 / (_Radius / DT_SIZE));
		int LastX = 0, LastY = 0;
		DTRing NewRing;
		for(int n=0; n < NumSteps; n++){
			Angle += AngleDelta;
			if(Angle > PIx2){
				break;
			}
			if(n % (NumSteps / DT_NUM_GAPS) < DT_GAP_WIDTH){
				continue;
			}
			float3 pos;
			pos.x = _Radius * sin(Angle+Rotate+_Twist) + _loc.x;
			pos.z = _Radius * -cos(Angle+Rotate+_Twist) + _loc.z;
			pos.x = float((int)(pos.x / 8) * 8);
			pos.z = float((int)(pos.z / 8) * 8);
			while(G->Map->Overlap(pos.x - DT_SIZE/2, pos.z - DT_SIZE/2, DT_SIZE, DT_SIZE, (float)LastX, (float)LastY, DT_SIZE, DT_SIZE)){
				float A = Angle+Rotate+_Twist;
				while(A < 0)
					A+= (float)PIx2;
				while(A > PIx2)
					A-= (float)PIx2;
				// Nudging
				if(A < PI_4){
					pos.x+=8;//Nudge Right
				}else if(A < 3.0f * PI_4){
					pos.z+=8;//Nudge Down
				}else if(A < 5.0f * PI_4){
					pos.x-=8;//Nudge Left
				}else if(A < 7.0f * PI_4){
					pos.z-=8;//Nudge Up
				}else{
					pos.x+=8;//Nudge Right
				}
			}
			if(G->Map->CheckFloat3(pos)==false){
				continue;
			}
			if(G->cb->GetElevation(pos.x, pos.z) < 0){
				continue;
			}
			pos.y = G->cb->GetElevation(pos.x, pos.z);
			NewRing.DTPos.push_back(pos);
			LastX = int(pos.x) - DT_SIZE/2;
			LastY = int(pos.z) - DT_SIZE/2;
			Angle = atan2f(pos.z - _loc.z, pos.x - _loc.x);
			CurDT++;
		}
		//LOG("Number of Dragon's Teeth Needed: " << CurDT << endl);
		DTRings.push_back(NewRing);
		NLOG("AddRing() ended");
	}

	CDTHandler::~CDTHandler(){
	}

	bool CDTHandler::DTNeeded(){
		//if(NoDT)
	   //     return false;
		return (!DTRings.empty());
	//	return true;
	}



	bool CDTHandler::IsDragonsTeeth(const char* FeatureName){
		return IsDragonsTeeth(string(FeatureName));
	}

	bool CDTHandler::IsDragonsTeeth(const string FeatureName){
		CUnitTypeData* ud = G->UnitDefLoader->GetUnitTypeDataByName(FeatureName);
		return IsDragonsTeeth(ud->GetUnitDef());
	}
	bool CDTHandler::IsDragonsTeeth(const UnitDef* ud){
		if(ud == 0) return false;
		return ud->isFeature;
	}
}
