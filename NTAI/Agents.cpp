// Agent Definitions
//#include "IGlobalAICallback.h"
#include "helper.h"
#include "Agents.h"
#include <deque>
#include <stdarg.h>
#include "./Helpers/Log.h"

//bool startedgame=false;
IAICallback* callback;
THelper* helper;
//vector<float3> sightings;
//int sights;
CMetalHandler* cm;
Log* L;

//////////////////////////////////////////////////////////////////////////////////////////
///////////// Assigner

//// Functions
float3 basepos;

void Assigner::InitAI(IAICallback* callback, THelper* helper){
	::callback = callback;
	::helper = helper;
	L = new Log(callback);
	L->Open();
	string mu = callback->GetMapName();
	L->print(mu.c_str());
	mu = callback->GetModName();
	L->print(mu.c_str());
	char cf[20];
	sprintf(cf,"map size: %i x %i ",callback->GetMapWidth()/16,callback->GetMapHeight()/16);
	L->print(cf);
	L->print("starting cains cmetalhandler class ");
	cm = new CMetalHandler(callback);
	L->print("metal handler loading states ");
	cm->loadState();
	L->print("metal handler loading states completed ");
	//sights = 0;
	//assigned = false;
}

void Assigner::GotChatMsg(const char *msg,int player){
	L->Message(msg,player);
}

void Assigner::UnitFinished(int unit){
	L->print("Assigner::Unitfinished");
	const UnitDef* ud=callback->GetUnitDef(unit);
	if(ud->energyUpkeep>0 && ud->makesMetal>0){
		UnitInfo info;
		if(callback->IsUnitActivated(unit)){
				info.turnedOn=true;
		} else{
				info.turnedOn=false;
		}
		info.energyUse=ud->energyUpkeep;
		myUnits[unit]=info;
	}
}
void Assigner::Update(){
	int frameNum=callback->GetCurrentFrame();
	if(lastUpdate<=frameNum-32){
		lastUpdate=frameNum;
		float energy=callback->GetEnergy();
		float estore=callback->GetEnergyStorage();
		float dif=energy-lastEnergy;
		lastEnergy=energy;
		if(energy<estore*0.2){
			//L->print("positive maker assigner");
			float needed=-dif+5;		//how much energy we need to save to turn positive
			for(map<int,UnitInfo>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn off
				if(needed<0)
					break;
				if(ui->second.turnedOn){
					needed-=ui->second.energyUse;
					Command* c = new Command;
					c->id=CMD_ONOFF;
					c->params.push_back(0);
					callback->GiveOrder(ui->first,c);
					ui->second.turnedOn=false;
				}
			}
		} else if(energy>estore*0.6){
			//L->print("negative maker assigner");
			float needed=dif+5;		//how much energy we need to start using to turn negative
			for(map<int,UnitInfo>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn on
				if(needed<0)
					break;
				if(!ui->second.turnedOn){
					needed-=ui->second.energyUse;
					Command* c = new Command;
					c->id=CMD_ONOFF;
					c->params.push_back(1);
					callback->GiveOrder(ui->first,c);
					ui->second.turnedOn=true;
				}
			}
		}
	}
}

//// chaser

void Chaser::InitAI(IAICallback* callback, THelper* helper){
	L->print("Chaser::InitAI ");
}

void Chaser::Add(int unit){
	L->print("Chaser::Add() ");
	Attackers.push_back(unit);
}

void Chaser::UnitDestroyed(int unit){
	L->print("Chaser::UnitDestroyed \n");
	for(list<int>::iterator aiy = Attackers.begin(); aiy != Attackers.end();++aiy){
		if(*aiy == unit){
			Attackers.erase(aiy);
			break;
		}
	}
	acknum--;
}
void Chaser::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	L->print("Chaser::UnitDamaged");
	//float3 qpos = callback->GetUnitPos(damaged);
	//sightings.push_back(qpos);
	//sights++;
	if(acknum>10){
			FindTarget();
	}
}
void Chaser::UnitFinished(int unit){
	L->print("chaser::UnitFinished");
	const UnitDef* ud = callback->GetUnitDef(unit);
	if((ud->health == 615)||(ud->health == 1302)||(ud->health == 910)||(ud->health == 5010)||(ud->health == 710)||(ud->health == 2240)||(ud->health == 980)||(ud->health == 740)){
		Add(unit);
		string mess = "new attacker added :";
		mess += ud->name.c_str();
		L->print(mess.c_str());
		acknum++;
		if(acknum>10){
			FindTarget();
		}
	}
}
Chaser::~Chaser(){
	L->Close();
	delete L;
}

void Chaser::EnemyDestroyed(int enemy){
		//if(acknum>9){
		//	FindTarget();
		///}
}

void Chaser::EnemyEnterLOS(int enemy){
	//sightings.push_back(callback->GetUnitPos(enemy));
	//sights++;
}

void Chaser::EnemyEnterRadar(int enemy){
	//sightings.push_back(callback->GetUnitPos(enemy));
	//sights++;
}

void Chaser::FindTarget(){
	L->print("Findtarget()");
	if(target.x == 0){
		target.x= (callback->GetMapWidth()*8) - basepos.x;
		target.y= 100;
		target.z= (callback->GetMapHeight()*8) - basepos.z -100;
		L->print("Findtarget()2");
		char ch[150];
		sprintf(ch,"basepos: %f, %f, %f      MapHeight: %i MapWidth: %i    target: %f, %f, %f",basepos.x,basepos.y,basepos.z,callback->GetMapHeight(),callback->GetMapWidth(),target.x,target.y,target.z);
		L->iprint(ch);
	}
	L->print("Findtarget()3");
	Command* c = new Command;
	c->params.push_back(target.x);
	c->params.push_back(target.y);
	c->params.push_back(target.z);
	c->id=CMD_MOVE;
	for(list<int>::iterator aik = Attackers.begin();aik != Attackers.end();aik++){
		callback->GiveOrder(*aik, c);
	}
	L->print("Findtarget()4");
	//sightings.pop_back();
	//sights = sights - 1;
}

//// Scouter

void Scouter::InitAI(IAICallback* callback, THelper* helper){
	found = false;
}
void Scouter::UnitFinished(int unit){
     if(callback->GetUnitMaxHealth(unit) == 910){
		 if(found == false){
			target.x= (callback->GetMapWidth()*8) - basepos.x;
			target.y= 100;
			target.z= (callback->GetMapHeight()*8) - basepos.z;
			c = new Command;
			c->params.push_back(target.x);
			c->params.push_back(target.y);
			c->params.push_back(target.z);
			c->id=CMD_MOVE;
			found = true;
			char ch[250];
			printf(ch,"basepos: %f, %f, %f     <br>MapHeight: %i MapWidth: %i   <br>target: %f, %f, %f <b>",basepos.x,basepos.y,basepos.z,callback->GetMapHeight(),callback->GetMapWidth(),target.x,target.y,target.z);
			L->print(ch);
		}
		L->print("scouter found");
		callback->GiveOrder(unit,c);
		//if(sightings.empty()) return;
		//Command* c = new Command;
		//float3 tempo = sightings.back();*/
		/*sightings.pop_back();
		delete c;*/
	 }
}


//// Factor

void Factor::UnitFinished(int unit){
	L->print("Factor::UnitFinished()");
	if(callback->GetCurrentFrame() == 2){
		klab1 = 0;
	}
	int mhealth = int(callback->GetUnitMaxHealth(unit));
	cm->addExtractor(unit);
	if((mhealth == 2000)||(mhealth == 473)||(mhealth == 627)||(mhealth == 290)||(mhealth == 713)){
		L->print("adding builder ");
		udar uu;
		uu.unit=unit;
		uu.creation = callback->GetCurrentFrame();
		uu.frame = 1;
		uu.guarding = false;
		uu.tasknum = 0;
		if(mhealth == 2000){
			uu.BList = 1;
			this->builders.push_back(uu);
		} else if(mhealth == 473){
			uu.BList = 2;
			this->builders.push_back(uu);
		} else if(mhealth == 290){
			uu.BList = 2;
			this->builders.push_back(uu);
		}else if(mhealth == 627){
			uu.BList = 2;
			this->builders.push_back(uu);
		}else if(mhealth == 713){
			uu.BList = 4;
			this->builders.push_back(uu);
		}else {
			uu.BList = 0;
			this->builders.push_back(uu);
		}
	}
	//fudar ff;
	//ff.unit = unit;
	//ff.guardingme = 0;
	if(mhealth == 2600){
		if(klab1 == 0)	klab1 = unit;
        FBuild("CORCK",unit,1);
		FBuild("CORAK",unit,4);
		FBuild("CORCRASH",unit,1);
		FBuild("CORTHUD",unit,2);
		FBuild("CORAK",unit,1);
		FBuild("CORCK",unit,1);
		FBuild("CORTHUD",unit,3);
		FBuild("CORAK",unit,3);
		FBuild("CORFAST",unit,1);
		//factorys.push_back(ff);
	}
	if(mhealth == 3770){
        FBuild("CORACK",unit,1);
		FBuild("CORPYRO",unit,5);
		FBuild("CORCAN",unit,1);
		FBuild("CORMORT",unit,3);
		//factorys.push_back(ff);
	}
	if(mhealth == 1925){
		FBuild("CORCA",unit,1);
		FBuild("CORFINK",unit,2);
		//factorys.push_back(ff);
	}
	if(mhealth == 1925){
        FBuild("CORACK",unit,1);
		FBuild("CORPYRO",unit,5);
		FBuild("CORCAN",unit,1);
		FBuild("CORMORT",unit,3);
		//factorys.push_back(ff);
	}
}

int Factor::GetId(string name){
	L->print("Factor~::GetId");
	const UnitDef* ud = callback->GetUnitDef(name.c_str());
	if( ud != 0 )	return ud->id;
	string error = "Factor::GetIdunit couldnt be loaded : ";
	error+=name.c_str();
	L->iprint(error);
	return 0;
}
// ARMMEX ARMRAD ARMMEX ARMLAB ARMSOL ARMLLT ARMSOL ARMMEX
void Factor::CBuild(string name, int unit){
	L->print("CBuild()");
	L->print(name.c_str());
	const UnitDef* ud = callback->GetUnitDef(name.c_str());
	//L->print("CBuild()2");
	if(ud == 0){
		string message = "error: a problem occured laoding this untis unitdefinition : ";
		message += name.c_str();
		L->print(message.c_str());
	} else {
		Command* c = new Command;
		c->id = -ud->id;
		float3 pos;
		L->print("CBuild()3");
		if(ud->extractRange>0){
			L->print("CBuild()3m");
			pos = cm->getNearestPatch(callback->GetUnitPos(unit),1,ud->extractsMetal);
			if(pos.y == -1){
				pos = callback->ClosestBuildSite(ud,pos,100,0);
			}
		} else {
			//L->print("CBuild()3c");
               pos = callback->ClosestBuildSite(ud,callback->GetUnitPos(unit),(650+(callback->GetCurrentFrame()%632)),4+(callback->GetCurrentFrame()%20));
		}
		if(!callback->CanBuildAt(ud,pos)){
			L->print("Error:: closestbuild returned a value that wasnt valid ");
			char ch[200];
			printf(ch,"pos: %f, %f, %f      \nMapHeight: %i MapWidth: %i ",pos.x,pos.y,pos.z,(callback->GetMapHeight()*8),(callback->GetMapWidth()*8));
			L->print(ch);
			pos = callback->ClosestBuildSite(ud,callback->GetUnitPos(unit),(1800-(callback->GetCurrentFrame()%800)),(4+(callback->GetCurrentFrame()%18)));
		}
		if(pos.x == -1){
			L->print("error:: closest buildsite returned -1 ");
		} else {
			for(vector<float3>::iterator ph = history.begin(); ph != history.end();++ph){
				if(ph->x==pos.x){
					if(ph->z==pos.z){
						L->iprint("Build location collision found!");
						return;
					}
				}
			}
			history.push_back(pos);
            c->params.push_back(pos.x);
			c->params.push_back(pos.y);
			c->params.push_back(pos.z);
			callback->GiveOrder(unit,c);
		}
	}
}

void Factor::FBuild(string name, int unit, int quantity){
	L->print("FBuild()");
	const UnitDef* ud = callback->GetUnitDef(name.c_str());
	if(ud == 0){
		string message = "error: a problem occured laoding this units unitdef : ";
		message += name.c_str();
		L->print(message.c_str());
	} else {
		Command* c = new Command;
		c->id = -ud->id;
		string issue = "issuing mobile unit :: ";
		issue += name.c_str();
		issue += "\n";
		L->print(issue.c_str());
		c->params.push_back(0);
		c->params.push_back(0);
		c->params.push_back(0);
		for(int q=1;q==quantity;q++){
            callback->GiveOrder(unit,c);
		}
	}
}

void Factor::Update(){
	int Tframe = callback->GetCurrentFrame();
	if(Tframe == 5){
		udar uu;
		uu.unit = helper->GetCommander();
		uu.creation = callback->GetCurrentFrame();
		uu.frame = 1;
		uu.BList = 1;
		this->builders.push_back(uu);
		//L->print("commander 1 ");
		CBuild("CORLLT",helper->GetCommander());
	}
	for(vector<udar>::iterator ui = builders.begin();ui !=builders.end();ui++){
		int frame = Tframe - ui->creation;
		string energy = "CORSOLAR";
		if(callback->GetMaxWind()>18){
			energy = "CORWIN";
		}
		if(ui->BList == 1){
			if(frame == 5){
				ui->tasks.push_back("CORMAKR");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORMSTOR");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORMAKR");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORMEX");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORMEX");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORMSTOR");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORLLT");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORMAKR");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORMEX");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORRAD");ui->tasknum++;
				ui->tasks.push_back("CORMEX");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORLAB");ui->tasknum++;
				ui->tasks.push_back("CORMEX");ui->tasknum++;
				basepos = callback->GetUnitPos(ui->unit);
			}
		}
		if(ui->BList == 2){
			if(frame == 4){
				ui->tasks.push_back("CORALAB");ui->tasknum++;
				ui->tasks.push_back("CORRAD");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORRL");ui->tasknum++;
				ui->tasks.push_back(energy);ui->tasknum++;
				if((Tframe%3)==1){
					ui->tasks.push_back("CORMAKR");ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					if(Tframe<30000){
						ui->tasks.push_back("CORALAB");ui->tasknum++;
					}
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back("CORRL");ui->tasknum++;
				}else if((Tframe%3)==2){
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					if(Tframe>30000){
						ui->tasks.push_back("CORALAB");ui->tasknum++;
					}
					if(Tframe<30000){
						ui->tasks.push_back("CORLAB");ui->tasknum++;
					}
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back("CORRL");ui->tasknum++;
				}else{
					ui->tasks.push_back("CORMEX");ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back("CORHLT");ui->tasknum++;
					ui->tasks.push_back(energy);ui->tasknum++;
					ui->tasks.push_back("CORRL");ui->tasknum++;
					if((Tframe%2)==1){
						ui->tasks.push_back("CORMAKR");ui->tasknum++;
						ui->tasks.push_back(energy);ui->tasknum++;
						ui->tasks.push_back(energy);ui->tasknum++;
					}
				}
				ui->tasks.push_back(energy);ui->tasknum++;
				ui->tasks.push_back("CORMEX");ui->tasknum++;
				ui->tasks.push_back("CORRL");ui->tasknum++;
			}
		}
		if(ui->BList == 3){// core necro
			if(frame == 4){
				ui->tasks.push_back("CORARAD");ui->tasknum++;
				if((Tframe%2)==0){
					ui->tasks.push_back("CORMOHO");ui->tasknum++;
				} else{
					ui->tasks.push_back("CORMOHO");ui->tasknum++;
					ui->tasks.push_back("CORHLT");ui->tasknum++;
				}
				ui->tasks.push_back("CORMOHO");ui->tasknum++;
			}
		}
		if(ui->BList == 4){// core adv. con kbot
			if(frame == 4){
				ui->tasks.push_back("CORARAD");ui->tasknum++;
				if((Tframe%2)==0){
					ui->tasks.push_back("x1corminifus");ui->tasknum++;
					ui->tasks.push_back("x1corminifus");ui->tasknum++;
				} else{
					ui->tasks.push_back("x1corminifus");ui->tasknum++;
					ui->tasks.push_back("CORFUS");ui->tasknum++;
					ui->tasks.push_back("CORMMKR");ui->tasknum++;
				}
				ui->tasks.push_back("x1corminifus");ui->tasknum++;
				ui->tasks.push_back("x1corminifus");ui->tasknum++;
				ui->tasks.push_back("CORMMKR");ui->tasknum++;
			}
		}
	}
}

void Factor::UnitIdle(int unit){
	if(callback->GetUnitMaxHealth(unit)==2600){
            FBuild("CORCK",unit,1);
			FBuild("CORAK",unit,4);
			FBuild("CORCRASH",unit,1);
			FBuild("CORTHUD",unit,2);
			FBuild("CORAK",unit,1);
			FBuild("CORTHUD",unit,3);
			FBuild("CORAK",unit,3);
			FBuild("CORAK",unit,3);
			FBuild("CORCRASH",unit,1);
			FBuild("CORTHUD",unit,2);
			FBuild("CORAK",unit,1);
			FBuild("CORTHUD",unit,3);
			FBuild("CORAK",unit,3);
	}
	if(callback->GetUnitMaxHealth(unit) == 3770){
		FBuild("CORACK",unit,1);
		FBuild("CORPYRO",unit,5);
		FBuild("CORMORT",unit,3);
		FBuild("CORCAN",unit,1);
		FBuild("CORNECRO",unit,1);
	}
	if(callback->GetUnitMaxHealth(unit) == 1925){
		FBuild("CORCA",unit,1);
	}
	//L->print("factor::UnitIdle() new task555");
	for(vector<udar>::iterator uv = builders.begin(); uv != builders.end();++uv){
		if(uv->tasknum<1){
			if(uv->unit == unit){
				if((uv->tasknum == 0)&&(uv->guarding == false)){
					L->print("idle builder repair");
					for(vector<udar>::iterator vu = builders.begin();vu != builders.end(); ++vu){
						if(vu->tasknum>0){
							L->iprint("guarding");
							Command* c = new Command;
							c->id = CMD_GUARD;
							c->params.push_back(vu->unit);
							callback->GiveOrder(uv->unit,c);
							uv->guarding = true;
							break;
						}
					}
				}
			}
		} else if ((uv->unit == unit)&&(uv->tasknum > 0)){
			L->print("factor::UnitIdle() new task");
			if(uv->tasknum<0) L->print("error! tasknum less than zero?!?!!?!? negative cookies arent allowed!!! ");
			if(uv->tasknum==1) L->print("tasknum at one? ");
			if(uv->tasks.empty()){
				L->print("Empty task list?!?!?!");
				uv->tasknum = 0;
				return;
			}
			//L->print("factor::UnitIdle() new task12");
			string task = "";
			task = uv->tasks.back();
			//L->print("factor::UnitIdle() new task121");
			CBuild(task,uv->unit);
			//L->print("factor::UnitIdle() new task21");
			uv->tasks.pop_back();
			//L->print("factor::UnitIdle() new task23");
			uv->tasknum = uv->tasknum - 1;
			//L->print("factor::UnitIdle() new task34");
		}
	}
}
