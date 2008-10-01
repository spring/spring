/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/
#include "../Core/include.h"

/*
AF 2004+
LGPL 2
*/

namespace ntai {


	CEconomy::CEconomy(Global* GL){
		G = GL;
	}

	btype CEconomy::Get(bool extreme,bool factory){

		if(BuildMaker(extreme)){
			return B_METAL_MAKER;
		}
		if(BuildMex(extreme)){
			return B_MEX;
		}
		if(BuildPower(extreme)){
			return B_POWER;
		}
		if(factory){
			if(BuildFactory(extreme)){
				return B_FACTORY;
			}
			if (BuildEnergyStorage(extreme)){
				return B_ESTORE;
			}
			if(BuildMetalStorage(extreme)){
				return B_MSTORE;
			}
		}
		return B_NA;
	}

	bool CEconomy::BuildFactory(bool extreme){
		NLOG("CEconomy::BuildFactory");

		if(extreme){
			float a = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\EXTREME\\factorymetal").c_str());
			float b = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\EXTREME\\factoryenergy").c_str());

			if(G->cb->GetMetal() > G->cb->GetMetalStorage()*a){
				if(G->cb->GetEnergy() > G->cb->GetEnergyStorage()*b){
					return true;
				}else{
					return false;
				}
			}else{
				return false;
			}
		}else{
			float a = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\factorymetal").c_str());
			float b = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\factoryenergy").c_str());
			float c = (float)atof(G->Get_mod_tdf()->SGetValueDef("7","ECONOMY\\RULES\\factorymetalgap").c_str());

			if(G->cb->GetMetal() > G->cb->GetMetalStorage()*a){
				if(G->cb->GetEnergy() > G->cb->GetEnergyStorage()*b){
					return true;
				}else{
					return false;
				}
			}else if(G->Pl->GetMetalIncome() - G->cb->GetMetalUsage()*1.2f > c){
				return true;
			}else{
				return false;
			}
		}
		return false;
	}

	bool CEconomy::BuildPower(bool extreme){
		NLOG("CEconomy::BuildPower");

		if(extreme){
			float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.3","ECONOMY\\RULES\\EXTREME\\power").c_str());
			if(G->cb->GetEnergy() < G->cb->GetEnergyStorage()*x){ 
				return true;
			}else{
				if(G->cb->GetEnergy()<400.0f){
					return true;
				}
				if(G->Pl->GetEnergyIncome() < G->cb->GetEnergyUsage()*(1-x)){
					return true;
				}
				return false;
			}
		}else{
			float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.4","ECONOMY\\RULES\\power").c_str());
			if(G->cb->GetEnergy() < G->cb->GetEnergyStorage()*x){
				return true;
			}else{
				if(G->cb->GetEnergy()<500.0f){
					return true;
				}
				if(G->Pl->GetEnergyIncome() < G->cb->GetEnergyUsage()*(1-x)){
					return true;
				}
				return false;
			}
		}
	}

	bool CEconomy::BuildMex(bool extreme){
		NLOG("CEconomy::BuildMex");

		if(extreme){
			float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.4","ECONOMY\\RULES\\EXTREME\\mex").c_str());
			
			// if below 20% metal stored then yah we need mexes!!
			if(G->cb->GetMetal() < G->cb->GetMetalStorage()*x){
				return true;
			}else{
				if(G->cb->GetMetal() < 400.0f){
					return true;
				}
				if(G->Pl->GetMetalIncome() < G->cb->GetMetalUsage()*(1-x)){
					return true;
				}
				return false;
			}
		}else{

			float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.5","ECONOMY\\RULES\\mex").c_str());
			
			// if below 50% metal stored then yah we need mexes!!
			if(G->cb->GetMetal() < G->cb->GetMetalStorage()*x){ 
				return true;
			}else{
				if(G->cb->GetMetal() < 500.0f){
					return true;
				}
				if(G->Pl->GetMetalIncome() < G->cb->GetMetalUsage()*(1-x)){
					return true;
				}
				return false;
			}
		}
		return false;
	}

	bool CEconomy::BuildEnergyStorage(bool extreme){
		NLOG("CEconomy::BuildEnergyStorage");
		
		/* for now I've used a half fudged rule I took from JCAI
		 erm, take the amount of energy currently being used, then
		 see how long it would take to drain away to zero if there
		 was no energy income and if it's smaller than a threshold
		 then say yes, else say no */
		
		float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\energystorage").c_str());
		
		if(G->cb->GetEnergy() > G->cb->GetEnergyStorage () * x){

			// randomly do this so there's a 1/3 chance
			int randnum = G->mrand()%4;
			randnum = max(randnum,1);
			return (G->cb->GetCurrentFrame()%randnum == 0);

		}else{
			return false;
		}

	}

	bool CEconomy::BuildMetalStorage(bool extreme){
		NLOG("CEconomy::BuildMetalStorage");

		/* for now I've used a half fudged rule I took from JCAI
		 erm, take the amount of metal currently being used, then
		 see how long it would take to drain away to zero if there
		 was no metal income and if it's smaller than a threshold
		 then say yes, else say no */

		float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\metalstorage").c_str());
		x = max(x,0.01f);

		if(G->cb->GetMetal() >G->cb->GetMetalStorage () * x){
			
			// randomly do this so there's a 1/3 chance
			int randnum = G->mrand()%4;
			randnum = max(1,randnum);
			return (G->cb->GetCurrentFrame()%randnum == 0);

		}else{
			return false;
		}
		
	}

	bool CEconomy::BuildMaker(bool extreme){
		NLOG("CEconomy::BuildMaker");

		if(extreme){
			float a = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.1","ECONOMY\\RULES\\EXTREME\\makermetal").c_str());
			float b = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\EXTREME\\makerenergy").c_str());

			// if below 10% metal stored then yah we need makers
			if( (G->cb->GetMetal() < G->cb->GetMetalStorage()*a) && (G->Pl->GetMetalIncome() < G->cb->GetMetalUsage()) ){
				
				// we need the energy to do this else we'll be even worse off with no metal or energy...
				bool r = (G->cb->GetEnergy() > G->cb->GetEnergyStorage()*b)&&(G->Pl->GetEnergyIncome() > G->max_energy_use);
				return r;

			}else{
				return false;
			}
		}else{

			float a = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.3","ECONOMY\\RULES\\EXTREME\\makermetal").c_str());
			float b = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.75","ECONOMY\\RULES\\EXTREME\\makerenergy").c_str());

			// if below 30% metal stored then yah we need makers
			if((G->cb->GetMetal() < G->cb->GetMetalStorage()*a)&&(G->Pl->GetMetalIncome() < G->cb->GetMetalUsage())){

				// we need the energy to do this else we'll be even worse off with no metal or energy...
				bool r = (G->cb->GetEnergy() > G->cb->GetEnergyStorage()*b)&& (G->Pl->GetEnergyIncome() > G->cb->GetEnergyUsage());
				return r;

			}else{
				return false;
			}
		}
		return false;
	}

}
