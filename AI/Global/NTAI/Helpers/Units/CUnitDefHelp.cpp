#include "../../Core/helper.h"

CUnitDefHelp::CUnitDefHelp(Global* GL){
	G = GL;
}
bool CUnitDefHelp::IsEnergy(const UnitDef* ud){
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
bool CUnitDefHelp::IsFactory(const UnitDef* ud){
	if(ud == 0) return false;
	if((ud->builder == true)&&(!ud->movedata)&&(ud->canfly == false)){
		return true;
	}else{
		return false;
	}
}
bool CUnitDefHelp::IsMex(const UnitDef* ud){
	if(ud == 0) return false;
	if(ud->type == string("MetalExtractor")){
		return true;
	}
	return false;
}

bool CUnitDefHelp::IsAirCraft(const UnitDef* ud){
	if(ud == 0) return false;
	if(ud->type == string("Fighter"))	return true;
	if(ud->type == string("Bomber"))	return true;
	if((ud->canfly==true)&&(ud->movedata == 0)) return true;
	return false;
}

bool CUnitDefHelp::IsGunship(const UnitDef* ud){
	if(ud == 0) return false;
	if(IsAirCraft(ud)&&ud->hoverAttack) return true;
	return false;
}

bool CUnitDefHelp::IsFighter(const UnitDef* ud){
	if(ud == 0) return false;
	if(IsAirCraft(ud)&&(ud->hoverAttack == false)) return true;
	return false;
}

bool CUnitDefHelp::IsBomber(const UnitDef* ud){
	if(ud == 0) return false;
	if(IsAirCraft(ud)&&(ud->type == string("Bomber"))) return true;
	return false;
}