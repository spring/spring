#include "../../Core/helper.h"

bool CDTHandler::NoDT = false;

float3 CDTHandler::GetDTBuildSite(int uid){
	if(DTRings.empty() == true) return UpVector;
	vector<float3> locations;
	for(vector<DTRing>::iterator i = this->DTRings.begin(); i != DTRings.end(); ++i){
		// go through DT Rings and add all locations without a DT on them to the locations container;
		for(vector<float3>::iterator d = i->DTPos.begin(); d != i->DTPos.end(); ++d){
			int* f = new int[2];
			int h = G->cb->GetFeatures(f,1,*d,20);
			delete [] f;
			if(h > 0) locations.push_back(*d);
		}
	}
	if(locations.empty()==true) return UpVector;
	float3 closest = locations.front();
	float distance = 5000000;
	float3 pos = G->GetUnitPos(uid);
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
    string filename = G->info->datapath + slash + "DTBuildData" + slash + G->info->tdfpath + string(".DTBuildData");
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
                    //LOG(" " << pud2->name << "(" << pud2->wreckName << ")");
                    DTBuildDataOut << n << pud2->id;
                    break;
                }
            }
        }
    }
    DTBuildDataOut.close();
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
				//Nudge Right
                pos.x+=8;
            }else if(A < 3.0f * PI_4){
				//Nudge Down
                pos.z+=8;
            }else if(A < 5.0f * PI_4){
				//Nudge Left
                pos.x-=8;
            }else if(A < 7.0f * PI_4){
				//Nudge Up
                pos.z-=8;
            }else{
				//Nudge Right
                pos.x+=8;
            }
        }
        if(pos.x < 0.0f || pos.x > maxx){
			//LOG("Off map (X)" << endl);
            continue;
        }
        if(pos.z < 0.0f || pos.z > maxz){
			//LOG("Off map (Y)" << endl);
            continue;
        }
        if(G->cb->GetElevation(pos.x, pos.z) < 0){
//            //LOG(n << "Too Low" << endl);
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
	if(NoDT)
        return false;
	return true;
}
bool CDTHandler::IsDragonsTeeth(const int Feature){
	return IsDragonsTeeth(G->cb->GetFeatureDef(Feature)->myName);
}

bool CDTHandler::IsDragonsTeeth(const char *FeatureName){
	return IsDragonsTeeth(string(FeatureName));
}

bool CDTHandler::IsDragonsTeeth(const string FeatureName){
	string FN(FeatureName);
	int unum = G->cb->GetNumUnitDefs();
	const UnitDef** UnitDefList = new const UnitDef*[unum];
	G->cb->GetUnitDefList(UnitDefList);
	if(UnitDefList == 0) return false;
	for(map<int, int>::iterator DTit = DTBuilds.begin(); DTit != DTBuilds.end(); DTit++){
    	if(UnitDefList[DTit->second]->wreckName == FN)
            return true;
    }
    return false;
}
