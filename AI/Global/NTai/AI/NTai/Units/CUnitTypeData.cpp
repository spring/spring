#include "../Core/helper.h"

CUnitTypeData::CUnitTypeData(){
	//
	this->singleBuild = false;
	this->singleBuildActive = false;
}

CUnitTypeData::~CUnitTypeData(){
	//
}

void CUnitTypeData::Init(Global* G, const UnitDef* ud){
	// Set up the values that need the passed parameters
	this->ud = ud;
	this->G = G;
	string n = ud->name;
	trim(n);
	tolowercase(n);
	this->unit_name = n;
}

const UnitDef* CUnitTypeData::GetUnitDef(){
	return ud;
}

string CUnitTypeData::GetName(){
	return unit_name;
}

bool CUnitTypeData::IsEnergy(){
	NLOG("CUnitTypeData::IsEnergy");
	if(ud == 0){
		G->L << "error UntiDef==0 in CUnitDefHelp::IsEnergy" << endline;
		return false;
	}

	if(ud->needGeo==true){
		return false;
	}
	if(ud->energyUpkeep<-1) return true;
	if((ud->windGenerator>0)&&(G->cb->GetMaxWind()>9)) return true;
	if(ud->tidalGenerator>0) return true;
	if(ud->energyMake>5) return true;
	return false;
}

bool CUnitTypeData::IsMobile(){
	NLOG("CUnitTypeData::IsMobile");
	if(ud == 0) return false;
	if(ud->speed>0) return true;
	if(ud->canmove) return true;
	if(ud->movedata != 0) return true;
	if(ud->canfly == true) return true;
	if(ud->canhover == true) return true;
	return false;
}

bool CUnitTypeData::IsFactory(){
	if(ud == 0) return false;
	//if(ud->buildOptions.empty()== true) return false;
	if(ud->type == string("Factory")) return true;
	return false;//(ud->builder && (IsMobile(ud)==false));
}

bool CUnitTypeData::IsHub(){
	if(ud == 0) return false;
	//if(ud->buildOptions.empty()) return false;
	return ((ud->type == string("Builder"))&&(!IsMobile()));
}

bool CUnitTypeData::IsMex(){
	if(ud == 0) return false;
	if(ud->type == string("MetalExtractor")){
		return true;
	}
	return false;
}

bool CUnitTypeData::IsMetalMaker(){
	if(ud == 0) return false;
	if(!ud->onoffable){
		return false;
	}
	return (ud->metalMake > 0.0f);
}

bool CUnitTypeData::IsAirCraft(){
	if(ud == 0) return false;
	if(ud->type == string("Fighter"))	return true;
	if(ud->type == string("Bomber"))	return true;
	if((ud->canfly==true)&&(ud->movedata == 0)) return true;
	return false;
}

bool CUnitTypeData::IsGunship(){
	if(ud == 0) return false;
	if(ud->type == string("Bomber"))	return false;
	if(IsAirCraft()&&ud->hoverAttack) return true;
	return false;
}

bool CUnitTypeData::IsFighter(){
	if(ud == 0) return false;
	if(ud->type == string("Bomber"))	return false;
	if(ud->builder)	return false;
	if(IsAirCraft()&&(ud->hoverAttack == false)) return true;
	return false;
}

bool CUnitTypeData::IsBomber(){
	if(ud == 0) return false;
	if(IsAirCraft()&&(ud->type == string("Bomber"))) return true;
	return false;
}

bool CUnitTypeData::IsAIRSUPPORT(){
	NLOG("CUnitTypeData::IsAIRSUPPORT");
	if(ud == 0) return false;
	//if(ud->) return false;
	if(ud->movedata) return true;
	if(ud->canfly) return true;
	return false;
}

bool CUnitTypeData::IsAttacker(){
	NLOG("CUnitTypeData::IsAttacker");
	bool atk = false;
	if(G->info->dynamic_selection == false){
		vector<string> v;
		CTokenizer<CIsComma>::Tokenize(v, G->Get_mod_tdf()->SGetValueMSG("AI\\attackers"), CIsComma());
		//v = bds::set_cont(v,G->Get_mod_tdf()->SGetValueMSG("AI\\attackers"));
		if(v.empty() == false){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				string u = ud->name;
				tolowercase(u);
				trim(u);
				string v = *vi;
				tolowercase(v);
				trim(v);
				if(v == u){
					atk = true;
					break;
				}
			}
		}
		//if(ud->builder == true) atk = false; // no builders either
		//if(!G->UnitDefHelper->IsMobile(ud)) atk = false; // no buildings
	}else{
		// determine dynamically if this is an attacker
		atk =true;
		if(ud->speed > G->info->scout_speed) atk = false; // cant be a scout either
		if(ud->isCommander == true) atk = false; //no commanders
		if(ud->builder == true) atk = false; // no builders either
		if(ud->weapons.empty() == true) atk = false; // has to have a weapon
		if((ud->canfly == false)&&(ud->movedata == 0)) atk = false; // no buildings
		if(ud->transportCapacity != 0) atk = false; // no armed transports
		if(ud->hoverAttack == true) atk = true; // obviously a gunship type thingymajig
	}
	return atk;
//	return false;
}

bool CUnitTypeData::GetSingleBuild(){
	//
	return this->singleBuild;
}

void CUnitTypeData::SetSingleBuild(bool value){
	//
	singleBuild = value;
}

bool CUnitTypeData::GetSingleBuildActive(){
	//
	return singleBuildActive;
}

void CUnitTypeData::SetSingleBuildActive(bool value){
	//
	singleBuildActive = value;
}


