#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::InitAI(Global* GLI){
	G = GLI;
	if(G->L.FirstInstance() == true){
		LOG << " :: " << G->cb->GetMapName() << endline << " :: " << G->cb->GetModName() << endline << " :: map size " << G->cb->GetMapWidth()/64 << " x "  << G->cb->GetMapHeight()/64 << endline;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::UnitFinished(int unit){
	const UnitDef* ud=G->cb->GetUnitDef(unit);
	if( (ud->energyUpkeep>0 && ud->makesMetal>0) || ((ud->energyUpkeep>50) && ((ud->onoffable == true)||(ud->extractsMetal>0)))) {
		UnitInfo info;
		if(G->cb->IsUnitActivated(unit)){
			info.turnedOn=true;
		}else{
			info.turnedOn=false;
		}
		info.energyUse=ud->energyUpkeep;
		myUnits[unit]=info;
	}
	if(ud->isCommander){
		G->comID = unit;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::Update(){
	int frameNum=G->cb->GetCurrentFrame();
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Every second check our metal makers. (nearly the
	// same code as that in the metal maker AI)
	if(EVERY_(2 SECONDS)){
		float energy=G->cb->GetEnergy();
		float estore=G->cb->GetEnergyStorage();
		float dif=energy-lastEnergy;
		lastEnergy=energy;
		if(energy<estore*0.35f){
			float needed=-dif+5;		//how much energy we need to save to turn positive
			for(map<int,UnitInfo>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn off
				if(needed<0)
					break;
				if(ui->second.turnedOn){
					needed-=ui->second.energyUse;
					TCommand tc;
					tc.created = G->cb->GetCurrentFrame();
					tc.Priority = tc_low;
					tc.unit = ui->first;
					tc.c.id=CMD_ONOFF;
					tc.c.params.push_back(0);
					G->GiveOrder(tc);
					ui->second.turnedOn=false;
				}
			}
		} else if(energy>estore*0.7f){
			//G->L.print("negative maker assigner");
			float needed=dif+5;		//how much energy we need to start using to turn negative
			for(map<int,UnitInfo>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn on
				if(needed<0)
					break;
				if(!ui->second.turnedOn){
					needed-=ui->second.energyUse;
					TCommand tc;
					tc.created = G->cb->GetCurrentFrame();
					tc.Priority = tc_low;
					tc.unit = ui->first;
					tc.c.id=CMD_ONOFF;
					tc.c.params.push_back(1);
					G->GiveOrder(tc);
					ui->second.turnedOn=true;
				}
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::UnitDestroyed(int unit){
	if(myUnits.empty() == false){
		for(map<int,UnitInfo>::iterator h = myUnits.begin(); h != myUnits.end();++h){
			if(h->first == unit){
				myUnits.erase(h);
				break;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

