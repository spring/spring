//////////////////////////////////////////////////////////////////////////
#include "../Core/helper.h"

CEconomy::CEconomy(Global* GL){
	G = GL;
}

btype CEconomy::Get(bool extreme,bool factory){
	//G->L.print("BuildMex");
	if(BuildMex(extreme)==true){
		//G->L.print("BuildMex(extreme)==true");
		return B_MEX;
	}
	//G->L.print("BuildPowe");
	if(BuildPower(extreme)==true){
		//G->L.print("BuildPower(extreme)==true");
		return B_POWER;
	}
	//G->L.print("BuildMaker");
	if(BuildMaker(extreme)==true){
		//G->L.print("BuildMaker(extreme)==true");
		return B_METAL_MAKER;
	}
	//G->L.print("BuildFactory");
	if(BuildFactory(extreme)==true){
		//G->L.print("BuildFactory(extreme)==true");
		return B_FACTORY;
	}
	//G->L.print("BuildEnergyStorage");
	if (BuildEnergyStorage(extreme)==true){
		//G->L.print("BuildEnergyStorage(extreme)==true");
		return B_ESTORE;
	}
	//G->L.print("BuildMetalStorage");
	if(BuildMetalStorage(extreme)==true){
		//G->L.print("BuildMetalStorage(extreme)==true");
		return B_MSTORE;
	}
	//G->L.print("return B_NA;");
	return B_NA;
}
bool CEconomy::BuildFactory(bool extreme){
	NLOG("CEconomy::BuildFactory");
	if(extreme == true){
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
		}else if(G->cb->GetMetalIncome() - G->cb->GetMetalUsage()*1.2f > c){
			return true;
		}else{
			return false;
		}
	}
	return false;
}

bool CEconomy::BuildPower(bool extreme){
	NLOG("CEconomy::BuildPower");
	if(extreme == true){
		float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.3","ECONOMY\\RULES\\EXTREME\\power").c_str());
		if(G->cb->GetEnergy() < G->cb->GetEnergyStorage()*x){ 
			return true;
		}else{
			if(G->cb->GetEnergy()<400.0f){
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
			return false;
		}
	}
}

bool CEconomy::BuildMex(bool extreme){
	NLOG("CEconomy::BuildMex");
	if(extreme == true){
		float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.4","ECONOMY\\RULES\\EXTREME\\mex").c_str());
		if(G->cb->GetMetal() < G->cb->GetMetalStorage()*x){ // if below 20% metal stored then yah we need mexes!!
			return true;
		}else{
			if(G->cb->GetMetal() < 400.0f){
				return true;
			}
			return false;
		}
	}else{
		float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.5","ECONOMY\\RULES\\mex").c_str());
		if(G->cb->GetMetal() < G->cb->GetMetalStorage()*x){ // if below 50% metal stored then yah we need mexes!!
			return true;
		}else{
			if(G->cb->GetMetal() < 500.0f){
				return true;
			}
			return false;
		}
	}
	return false;
}

bool CEconomy::BuildEnergyStorage(bool extreme){
	NLOG("CEconomy::BuildEnergyStorage");
	float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\energystorage").c_str());
	if(G->cb->GetEnergy() > G->cb->GetEnergyStorage () * x){
		// randomly do this so there's a 1/3 chance
		srand(uint(time(NULL) +G->Cached->team +  G->Cached->randadd));
		G->Cached->randadd++;
		int randnum = rand()%4;
		randnum = max(randnum,1);
		if(G->cb->GetCurrentFrame()%randnum == 0){
			return true;
		}else{
			return false;
		}
	}else{
		return false;
	}
	// for now I've used a half fudged rule I took from JCAI
	// erm, take the amount of energy currently being used, then see how long ti would take to drain away to zero if there was no energy income and if it's smaller than a threshold then say yes, else say no
}
bool CEconomy::BuildMetalStorage(bool extreme){
	NLOG("CEconomy::BuildMetalStorage");
	float x = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\metalstorage").c_str());
	x = max(x,0.01f);
	if(G->cb->GetMetal() >G->cb->GetMetalStorage () * x){
		srand(uint(time(NULL) +G->Cached->team +  G->Cached->randadd));
		G->Cached->randadd++;
		int randnum = rand()%4;
		randnum = max(1,randnum);
		if(G->cb->GetCurrentFrame()%randnum == 0){
			return true;
		}else{
			return false;
		}
	}else{
		return false;
	}
	// for now I've used a half fudged rule I took from JCAI
	// erm, take the amount of metal currently being used, then see how long it would take to drain away to zero if there was no metal income and if it's smaller than a threshold then say yes, else say no
}

bool CEconomy::BuildMaker(bool extreme){
	NLOG("CEconomy::BuildMaker");
	if(extreme == true){
		float a = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.1","ECONOMY\\RULES\\EXTREME\\makermetal").c_str());
		float b = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.8","ECONOMY\\RULES\\EXTREME\\makerenergy").c_str());
		if((G->cb->GetMetal() < G->cb->GetMetalStorage()*a)&&(G->cb->GetMetalIncome() < G->cb->GetMetalUsage())){ // if below 10% metal stored then yah we need makers
			if((G->cb->GetEnergy() > G->cb->GetEnergyStorage()*b)&&(G->cb->GetEnergyIncome() > G->cb->GetEnergyUsage())){ // we need the energy to do this else we'll be even worse off with no metal or energy...
				return true;
			}else{
				return false;
			}
		}else{
			return false;
		}
	}else{
		float a = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.3","ECONOMY\\RULES\\EXTREME\\makermetal").c_str());
		float b = (float)atof(G->Get_mod_tdf()->SGetValueDef("0.75","ECONOMY\\RULES\\EXTREME\\makerenergy").c_str());
		if((G->cb->GetMetal() < G->cb->GetMetalStorage()*a)&&(G->cb->GetMetalIncome() < G->cb->GetMetalUsage())){ // if below 30% metal stored then yah we need makers
			if((G->cb->GetEnergy() > G->cb->GetEnergyStorage()*b)&&(G->cb->GetEnergyIncome() > G->cb->GetEnergyUsage())){ // we need the energy to do this else we'll be even worse off with no metal or energy...
				return true;
			}else{
				return false;
			}
		}else{
			return false;
		}
	}
	return false;
}
