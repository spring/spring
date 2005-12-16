#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::InitAI(Global* GLI){
	G = GLI;
	string mu = G->cb->GetMapName();
	G->L->print(mu);
	mu = G->cb->GetModName();
	G->L->print(mu);
	char cf[20];
	sprintf(cf,"map size: %i x %i ",G->cb->GetMapWidth()/64,G->cb->GetMapHeight()/64);
	G->L->print(cf);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Assigner::UnitFinished(int unit){
	G->L->print("Assigner::Unitfinished");
	const UnitDef* ud=G->cb->GetUnitDef(unit);
	if(ud->energyUpkeep>0 && ud->makesMetal>0){
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
	// Detection of Other AI's. The only bit I've done here is the
	// adding of OTAI detection, but this should work for OTAI 1.0 but
	// I havent a clue as to 1.02 as I dont ahve the source. I've also
	// commented this out for fear it is actually damaging OTAI during
	// gameplay.
	/*if(frameNum == 5){
		const int Height = G->cb->GetMapHeight()/2, Width = G->cb->GetMapWidth()/2;
		char c[16];
		// Detect if OTAI is in this game with NTAI.
		for(int i = 0;i<6;i++){
			sprintf(c, "MV%d", i);
			unsigned char* MetalValue = (unsigned char*)G->cb->CreateSharedMemArea(c, Width * Height);
			const unsigned char *MM = G->cb->GetMetalMap();
			for(int y=0; y<Height; y++)
				for(int x=0; x<Width; x++)
					if(MetalValue[y*Width + x] == MM[y*Width + x]){
						G->players[i] = AI_OTAI;
						break;
					}
		}
		// set ourselves as NTAI
		G->players[G->cb->GetMyAllyTeam()] = AI_NTAI;
	}*/
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Every second check our metal makers. (nearly the
	// same code as that in the metal maker AI)
	if(frameNum%32 == 0){
		float energy=G->cb->GetEnergy();
		float estore=G->cb->GetEnergyStorage();
		float dif=energy-lastEnergy;
		lastEnergy=energy;
		if(energy<estore*0.2){
			float needed=-dif+5;		//how much energy we need to save to turn positive
			for(map<int,UnitInfo>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn off
				if(needed<0)
					break;
				if(ui->second.turnedOn){
					needed-=ui->second.energyUse;
					Command* c = new Command;
					c->id=CMD_ONOFF;
					c->params.push_back(0);
					G->cb->GiveOrder(ui->first,c);
					ui->second.turnedOn=false;
				}
			}
		} else if(energy>estore*0.6){
			//G->L->print("negative maker assigner");
			float needed=dif+5;		//how much energy we need to start using to turn negative
			for(map<int,UnitInfo>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn on
				if(needed<0)
					break;
				if(!ui->second.turnedOn){
					needed-=ui->second.energyUse;
					Command* c = new Command;
					c->id=CMD_ONOFF;
					c->params.push_back(1);
					G->cb->GiveOrder(ui->first,c);
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

