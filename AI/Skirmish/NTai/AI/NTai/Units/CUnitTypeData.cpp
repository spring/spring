/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/
#include "../Core/include.h"

namespace ntai {

	CUnitTypeData::CUnitTypeData(){
		//
		this->singleBuild = false;
		this->singleBuildActive = false;
		this->soloBuild = false;
		this->soloBuildActive = false;
		exclusionRange=0;
		repairDeferRange=0;
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
		
		G->Get_mod_tdf()->GetDef(repairDeferRange,"0","Resource\\ConstructionRepairRanges\\"+unit_name);

		G->Get_mod_tdf()->GetDef(exclusionRange,"0","Resource\\ConstructionExclusionRanges\\"+unit_name);

		canConstruct = (ud->buildOptions.empty()==false);


		// precalc wether this unit type is an attacker
		if(G->info->dynamic_selection == false){
			attacker = false;
			vector<string> v;
			CTokenizer<CIsComma>::Tokenize(v, G->Get_mod_tdf()->SGetValueMSG("AI\\attackers"), CIsComma());
			//v = bds::set_cont(v,G->Get_mod_tdf()->SGetValueMSG("AI\\attackers"));
			if(v.empty() == false){
				for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
					string v = *vi;
					tolowercase(v);
					trim(v);
					if(v == GetName()){
						attacker = true;
						break;
					}
				}
			}

		}else{
			// determine dynamically if this is an attacker
			attacker =true;
			if(ud->speed > G->info->scout_speed){
				attacker = false; // cant be a scout either
			} else if(ud->isCommander){
				attacker = false; //no commanders
			} else if(ud->builder){
				attacker = false; // no builders either
			} else if(ud->weapons.empty()){
				attacker = false; // has to have a weapon
			} else if((ud->canfly == false)&&(ud->movedata == 0)){
				attacker = false; // no buildings
			} else if(ud->transportCapacity != 0){
				attacker = false; // no armed transports
			} else if(ud->hoverAttack){
				attacker = true; // obviously a gunship type thingymajig
			}
		}


		// dgun stuff
		canDGun = ud->canDGun;
		if(canDGun){
			for(vector<UnitDef::UnitDefWeapon>::const_iterator k = ud->weapons.begin();k != ud->weapons.end();++k){
				if(k->def->manualfire){
					dgunCost = k->def->energycost;
					break;
				}
			}
		}

		buildSpacing = 4;

		if(IsMobile()){
			buildSpacing = 1;
		} else if(IsFactory()){
			G->Get_mod_tdf()->GetDef(buildSpacing,"4", "AI\\factory_spacing");
		} else if (!IsMobile() && !ud->weapons.empty()){
			G->Get_mod_tdf()->GetDef(buildSpacing,"4", "AI\\defence_spacing");
		} else if (IsEnergy()){
			G->Get_mod_tdf()->GetDef(buildSpacing,"3", "AI\\power_spacing");
		}else{
			G->Get_mod_tdf()->GetDef(buildSpacing,"1", "AI\\default_spacing");
		}
		float r = sqrt(pow((float)ud->xsize*8,2)+pow((float)ud->zsize*8,2))/2;
		r += (buildSpacing*8);
		buildSpacing = r;

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
		return attacker;
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

	bool CUnitTypeData::GetSoloBuild(){
		//
		return this->singleBuild;
	}

	void CUnitTypeData::SetSoloBuild(bool value){
		//
		soloBuild = value;
	}

	bool CUnitTypeData::GetSoloBuildActive(){
		//
		return soloBuildActive;
	}

	void CUnitTypeData::SetSoloBuildActive(bool value){
		//
		soloBuildActive = value;
	}

	void CUnitTypeData::SetExclusionRange(int value){
		//
		exclusionRange = value;
	}

	int CUnitTypeData::GetExclusionRange(){
		//
		return exclusionRange;
	}

	void CUnitTypeData::SetDeferRepairRange(float value){
		//
		repairDeferRange = value;
	}

	float CUnitTypeData::GetDeferRepairRange(){
		//
		return repairDeferRange;
	}

	float CUnitTypeData::GetDGunCost(){
		//
		return dgunCost;
	}

	bool CUnitTypeData::CanDGun(){
		return canDGun;
	}

	bool CUnitTypeData::CanConstruct(){
		return canConstruct;
	}

	float CUnitTypeData::GetSpacing(){
		return buildSpacing;
	}
}
