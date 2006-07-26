#include "../Core/helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::InitAI(Global* GLI){
	G = GLI;
	G->L << " :: " << G->cb->GetMapName() << endline << " :: " << G->cb->GetModName() << endline << " :: map size " << G->cb->GetMapWidth()/64 << " x "  << G->cb->GetMapHeight()/64 << endline;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::UnitFinished(int unit){
	const UnitDef* ud=G->cb->GetUnitDef(unit);
	if(ud == 0) return;
	if(ud->isCommander){
		G->Cached->comID = unit;
	}
	if((ud->energyUpkeep>20.0f && (ud->makesMetal>0.0f || ud->type == "MetalExtractor"))){
		UnitInfo info;
		info.turnedOn = G->cb->IsUnitActivated(unit);
		info.energyUse=min(ud->energyUpkeep,1.0f);
		info.unit = unit;
		info.efficiency = ud->metalMake/min(ud->energyUpkeep,1.0f);
		if(ud->extractsMetal > 0.00000000f) info.efficiency += (ud->extractsMetal*10000)/min(ud->energyUpkeep,1.0f);
		myUnits.push_back(info);
		myUnits.sort();
	}
	if(ud->canCloak == true){
		if(ud->movedata == 0){
			if(ud->canfly == false){
				if(ud->cloakCost < 40){
					return;
				}
			}
		}
		CloakInfo info;
		info.uid = unit;
		info.cloakCost = ud->cloakCost;
		info.cloakCostMoving = ud->cloakCostMoving;
		if(ud->startCloaked == true){
			info.cloaked = true;
			G->Cached->cloaked_units.insert(unit);
		}else{
			info.cloaked = false;
		}
		info.name = ud->name;
		info.efficiency = G->GetEfficiency(ud->name);
		CloakedUnits.push_back(info);
		CloakedUnits.sort();
	}
	return;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::Update(){
	NLOG("Assigner::Update()");
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Every second check our metal makers. (nearly the
	// same code as that in the metal maker AI)
	if(EVERY_((1 SECONDS))){
		float energy=G->cb->GetEnergy();
		float estore=G->cb->GetEnergyStorage();
		float dif=energy-lastEnergy;
		lastEnergy=energy;
		if(energy<estore*0.3f){
			float needed=-dif+5;		//how much energy we need to save to turn positive
			for(list<UnitInfo>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn off
				if(needed<0)
					break;
				if(ui->turnedOn){
					needed-=ui->energyUse;
					TCommand TCS(tc,"assigner:: turnoff");
					tc.created = G->cb->GetCurrentFrame();
					tc.Priority = tc_low;
					tc.unit = ui->unit;
					tc.ID(CMD_ONOFF);
					tc.Push(0);
					G->OrderRouter->GiveOrder(tc);
					ui->turnedOn=false;
				}
			}
		} else if(energy>estore*0.6f){
			//G->L.print("negative maker assigner");
			float needed=dif+5;		//how much energy we need to start using to turn negative
			for(list<UnitInfo>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn on
				if(needed<0)
					break;
				if(!ui->turnedOn){
					needed-=ui->energyUse;
					TCommand TCS(tc,"assigner:: turnon");
					tc.created = G->cb->GetCurrentFrame();
					tc.Priority = tc_low;
					tc.unit = ui->unit;
					tc.ID(CMD_ONOFF);
					tc.Push(1);
					G->OrderRouter->GiveOrder(tc);
					ui->turnedOn=true;
				}
			}
		}
	}
	if(EVERY_((2 SECONDS))){
		if(G->GetCurrentFrame() < (2 MINUTES)){
			return;
		}
		float energy=G->cb->GetEnergy();
		float estore=G->cb->GetEnergyStorage();
		float dif=energy-lastEnergy;
		lastEnergy=energy;
		if(energy<estore*0.65f){
			float needed=-dif+5;		//how much energy we need to save to turn positive
			for(list<CloakInfo>::iterator ui=CloakedUnits.begin();ui!=CloakedUnits.end();++ui){		//find makers to turn off
				if(needed<0)
					break;
				if(ui->cloaked){
					needed-=ui->cloakCost;
					TCommand TCS(tc,"assigner:: turnoff cloak");
					tc.created = G->cb->GetCurrentFrame();
					tc.Priority = tc_low;
					tc.unit = ui->uid;
					tc.ID(CMD_CLOAK);
					tc.Push(0);
					G->OrderRouter->GiveOrder(tc);
					ui->cloaked=false;
					G->Cached->cloaked_units.erase(ui->uid);
				}
			}
		} else if(energy>estore*0.7f){
			//G->L.print("negative maker assigner");
			float needed=dif+5;		//how much energy we need to start using to turn negative
			for(list<CloakInfo>::iterator ui=CloakedUnits.begin();ui!=CloakedUnits.end();++ui){		//find makers to turn on
				if(needed<0)
					break;
				if(!ui->cloaked){
					needed-=ui->cloakCost;
					TCommand TCS(tc,"assigner:: turnon cloak");
					tc.created = G->cb->GetCurrentFrame();
					tc.Priority = tc_low;
					tc.unit = ui->uid;
					tc.ID(CMD_CLOAK);
					tc.Push(1);
					G->OrderRouter->GiveOrder(tc);
					ui->cloaked=true;
					G->Cached->cloaked_units.insert(ui->uid);
				}
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::UnitDestroyed(int unit){
	if(myUnits.empty() == false){
		for(list<UnitInfo>::iterator i = myUnits.begin(); i != myUnits.end(); ++i){
			if( i->unit == unit){
				myUnits.erase(i);
				break;
			}
		}
	}
	if(CloakedUnits.empty() == false){
		for(list<CloakInfo>::iterator i = CloakedUnits.begin(); i != CloakedUnits.end(); ++i){
			if( i->uid == unit){
				CloakedUnits.erase(i);
				break;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

