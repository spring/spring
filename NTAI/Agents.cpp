#include "helper.h"

//26 build trees

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Defines
// Used to signify Build orders.

// CORE
#define CCORCOM 1
#define CORCON 2
#define CORNECRO 3
#define CORACK 4
#define CORLAB 5
#define CORALAB 6
#define CORGANT 7
#define CORAP 8
#define CORVP 9
#define CORSY 10
#define CORAVP 11
#define CORASY 12
#define CORCS 13

//ARM
#define CARMCOM 15
#define CARMALAB 19
#define CARMCON 20
#define CARMLAB 17

// TLL
#define CTLLCOM 14
#define CTLLLAB 16
#define CTLLALAB 18
#define CTLLCON 21

//MYNN
#define CRACCOM 22

//Xect
#define CXCLCOM 23

//Rhyoss
#define CRHYCOM 24

// Rebel Alliance
#define CREBCOM 25

// Imperial Empire
#define CIMPCON 26

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//Races
#define R_ARM 1
#define R_CORE 2
#define R_TLL 3
#define R_XECT 4
#define R_MYNN 5
#define R_RHY 6
// SWS
#define R_REBEL 7
#define R_IMPERIAL_EMPIRE 8


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Assigner Class

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// This was a function I wrote that made use of some code I
// found, it would load a list from a file then shove it into a format
// I could use to list build tasks.
// For some reason i couldnt open the file and thus it never got
// used, so I left it ehre commented out till a future date.

/*vector<string> FVStream(string file){
	ifstream UnitFile (file.c_str();
    if(!UnitFile){
		G->L->print("error, iostream couldnt be ran");
	}
	string f = "";
	vector<string> $vectors;
	while(UnitFile >> f){
		$vectors.push_back(f);
	}
	return $vectors;
	//initutil example:
	
}*/

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// This is a remnant from when all the agents where all in this file
// (hence the name Agents.cpp). It was an experiment trying to
// create a scouting system based on how JCAI did it.
// I may re-implement this at a later point as the current scouting
// is hopelessly reliant on cains metal class and becomes useless
// on water maps, or maps with no metal whatsoever.

/*float3 Scouter::GetScoutpos(float3 pos, int Lrad){
	float3 position;
	int highscore=0;
	int frame = G->cb->GetCurrentFrame();
	int gmax = G->G().GetMaxIndex(LOS_GRID);
	for(int ind = 0; ind <= gmax; ind++){
			int score;
			float3 tpos= G->G().GetPos(LOS_GRID,ind);
			int diff = tpos.x-pos.x;
			int diff2 = tpos.z - pos.z;
			if(diff<0) diff *= (-1);
			if(diff2<0) diff2 *= (-1);
			if((diff<Lrad)&&(diff2<Lrad)) continue;
			if((diff>Lrad*5)||(diff2>Lrad*5)) continue;
			score = frame-G->G().Get(LOS_GRID,tpos);
			if(score>highscore){
				highscore = score;
				position = tpos;
			}
	}
	return position;
}*/

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::InitAI(Global* GLI){
	G = GLI;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitCreated(int unit){
	//const UnitDef* ud = G->cb->GetUnitDef(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitFinished(int unit){
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if (ud != 0){
		if (ud->extractsMetal>0){
			G->M->addExtractor(unit);
		}
	}
	if(ud == 0){
		exprint("error factor::unitfinished:unitdef failed to laod");
		return;
	}
	cfluke[unit] = 0;
	//float mhealth = G->cb->GetUnitMaxHealth(unit);
	/*char buf[400];
	string cname;
	cname += G->cb->GetModName();
	cname += ".con.txt";
	bool fexist = G->cb->ReadFile(cname.c_str(),buf,400);
	vector<string> v;
	if(fexist == false){
		G->L->print("unitfinished doesnt exist?! Could it have been destroyed?");
		v = bds::set_cont(v, "CORCK, CORACK, CORNECRO, CORCA, CORCV, CORAP, CCORCOM, CORLAB, CORALAB, CORGANT, CORVP, CORAVP, CORCV, CORACV, CORSY, CORASY, CORCS");
	}else{
		v = bds::set_cont(v, buf);
	}
	bool build = false;
	for(vector<string>::iterator ji = v.begin(); ji != v.end();++ji){
		if(*ji == ud->name){
			build = true;
			break;
		}
	}*/
	if(ud->builder == true){
		Tunit* uu = new Tunit(G,unit);
		//if(uu->bad == true) return;
		bool set = false;
		string na = "CORCA";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORCK";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORCV";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORNECRO";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORNECRO;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORACK";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORACK;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORAP";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORAP;
			int msize = G->cb->GetFileSize("TCORAP.txt");
			if((msize != 0) || (msize != -1)){
				char* mn = new char[msize];
				if(G->cb->ReadFile("TCORAP.txt",mn,msize) == true){
					vector<string> v;
					v = bds::set_cont(v, mn);
					if(v.empty() == false){
						//uu->tasks = v;
					}
				}
			} else {
				G->L->eprint("TCORAP.txt couldnt be loaded");
			}
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORLAB";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORLAB;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "TLLLAB";
		if((ud->name == na)&&(set == false)){
			uu->BList = CTLLLAB;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "ARMLAB";
		if((ud->name == na)&&(set == false)){
			uu->BList = CARMLAB;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORVP";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORVP;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORAVP";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORAVP;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORSY";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORSY;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORASY";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORASY;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORCS";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORCS;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORALAB";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORALAB;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "TLLALAB";
		if((ud->name == na)&&(set == false)){
			uu->BList = CTLLALAB;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "ARMALAB";
		if((ud->name == na)&&(set == false)){
			uu->BList = CARMALAB;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORGANT";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORGANT;
			uu->factory = true;
			uu->repeater = true;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CARMCV";
		if((ud->name == na)&&(set == false)){
			uu->BList = CARMCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "ARMCK";
		if((ud->name == na)&&(set == false)){
			uu->BList = CARMCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "ARMCA";
		if((ud->name == na)&&(set == false)){
			uu->BList = CARMCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "ARMCH";
		if((ud->name == na)&&(set == false)){
			uu->BList = CARMCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "TLLCV";
		if((ud->name == na)&&(set == false)){
			uu->BList = CTLLCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "TLLCK";
		if((ud->name == na)&&(set == false)){
			uu->BList = CTLLCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "TLLCP";
		if((ud->name == na)&&(set == false)){
			uu->BList = CTLLCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "TLLCA";
		if((ud->name == na)&&(set == false)){
			uu->BList = CTLLCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORCV";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORCON;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		na = "CORACV";
		if((ud->name == na)&&(set == false)){
			uu->BList = CORACK;
			this->builders.push_back(uu);
			set = true;
			return;
		}
		if((race != 0)&&(set == false)){
			uu->BList = 0;
			G->L->print(ud->name);
			G->L->print("above unit given build queue of zero");
			this->builders.push_back(uu);
			return;
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// This function is outdated but I keep it here because I may need
// use for it in the future. Takes a units name and returns its ID
// (UnitDef[id])

int Factor::GetId(string name){
	G->L->print("Factor~::GetId");
	const UnitDef* ud = G->cb->GetUnitDef(name.c_str());
	if( ud != 0 )	return ud->id;
	string error = "Factor::GetIdunit couldnt be loaded : ";
	error+=name.c_str();
	G->L->iprint(error);
	return 0;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Building things that need placing on a map.

bool Factor::CBuild(string name, int unit){
	G->L->print("CBuild()");
	if(G->cb->GetUnitDef(unit) == 0) return false;
	G->L->print(name.c_str());
	const UnitDef* ud = G->cb->GetUnitDef(name.c_str());
	if(G->cb->GetUnitHealth(unit)<1){
		G->L->print("unitfinished doesnt exist?! Could it have been destroyed?");		
		return false;
	}
	if(ud == 0){
		string message = "error: a problem occured loading this units unitdefinition : ";
		message += name.c_str();
		G->L->print(message.c_str());
		return false;
	} else {
		Command* c = new Command;
		c->id = -ud->id;
		float3 pos;
		if(ud->extractRange>0){
			pos = G->M->getNearestPatch(G->cb->GetUnitPos(unit),1,ud->extractsMetal);
			if(pos == ZeroVector){
				G->L->eprint("nearest metal patch returned bad");
				return false;
			}
			if(pos == ZeroVector){
				//pos = G->cb->ClosestBuildSite(udx,G->cb->GetUnitPos(unit),700,1);
				G->cb->ClosestBuildSite(ud,G->cb->GetUnitPos(unit),70,0);
				G->L->print("zero mex co-ordinates intercepted");
				return false;
			}
		} else {
			//pos = G->cb->ClosestBuildSite(ud,G->cb->GetUnitPos(unit),700,1);
			pos = G->cb->ClosestBuildSite(ud,G->cb->GetUnitPos(unit),(260+(G->cb->GetCurrentFrame()%640)),8);
			if(pos == ZeroVector){
				pos = G->cb->ClosestBuildSite(ud,G->cb->GetUnitPos(unit),240,0);
				G->L->print("zero vector returned");
			}else if(pos == UpVector){
				pos = G->cb->ClosestBuildSite(ud,G->cb->GetUnitPos(unit),240,0);
				G->L->print("zero vector returned");
			}
			if((pos == ZeroVector) || (pos == UpVector)){
				G->L->eprint("zero vector returned");
				return false;
			}
			/*if(history.empty() == false){
				for(vector<float3>::iterator ph = history.begin(); ph != history.end();++ph){
					if(ph->x==pos.x){
						if(ph->z==pos.z){
							G->L->print("Build location collision found!");
							return false;
						}
					}
				}
			}*/
		}
		//const float3 upos = G->cb->GetUnitPos(unit);
		//G->L->print("calling new build class");
		//if(G->B->FindBuildPosition(ud,upos,pos) == false) G->L->print("CBuild Build function returns false for building position");
		//G->L->print("new build class called");
        c->params.push_back(pos.x);
		c->params.push_back(pos.y);
		c->params.push_back(pos.z);
		c->timeOut = int(ud->buildTime/4);
		if(G->cb->GiveOrder(unit,c)== -1){
				/*const deque<Command>* cd = G->cb->GetCurrentUnitCommands(unit);
				bool done = false;
				for(deque<Command>::const_iterator di = cd->begin();di != cd->end();++di){
					if((di->id == c->id)||(di->id == c->id)){
						done = true;
						break;
					}
				}
				if(done == true){
					return true;
				}else{*/
			G->L->print("Error::Cbuild Failed Order");
			return false;
				//}
		}else{
            return true;
		}
	}
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Used for building things through a factory.

bool Factor::FBuild(string name, int unit, int quantity){
	if(G->cb->GetUnitDef(unit) == 0) return false;
	G->L->print("FBuild()");
	if(G->cb->GetUnitHealth(unit)<1){
		G->L->print("unitfinished doesnt exist?! Could it have been destroyed?");		
		return false;
	}
	const UnitDef* ud = G->cb->GetUnitDef(name.c_str());
	if(ud == 0){
		string message = "error: a problem occured loading this units unitdef : ";
		message += name.c_str();
		G->L->print(message.c_str());
		return false;
	} else {
		Command* c = new Command;
		c->id = -ud->id;
		string issue = "issuing mobile unit :: ";
		issue += name.c_str();
		G->L->print(issue.c_str());
		c->params.push_back(0);
		c->params.push_back(0);
		c->params.push_back(0);
		for(int q=1;q==quantity;q++){
			if(G->cb->GiveOrder(unit,c)== -1){
				//if(G->cb->GiveOrder(unit,c)==0){
					G->L->eprint("Error::Fbuild Failed Order twice");
					const deque<Command>* cd = G->cb->GetCurrentUnitCommands(unit);
					if((cd->front().id == c->id)||(cd->back().id == c->id)){
						return true;
					}else{
                        return false;
					}
				//}
			}
		}
		return true;
	}
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::Update(){
	int Tframe = G->cb->GetCurrentFrame();

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Check our commanders, and assign them.
	//{
	if(Tframe == 5){

		// Get commander ID
		int* uj = new int[500];
		int unum = G->cb->GetFriendlyUnits(uj);
		if(unum == 0) return;

		// Fill out a unit object
		Tunit* uu = new Tunit(G,uj[0]);
		uu->creation = G->cb->GetCurrentFrame();
		uu->frame = 1;
		uu->guarding = false;
		uu->factory = false;
		uu->waiting = false;
		bool set = false;

		// Check what race and build tree to give the commander

		// CORE
		string na = "CORCOM";
		if((uu->ud->name == na)&&(set == false)){
			uu->BList = CCORCOM;
			this->builders.push_back(uu);
			G->L->print("NTAI Core");
			set  = true;
			race = R_CORE;
			return;
		}

		// ARM
		na = "ARMCOM";
		if((uu->ud->name == na)&&(set == false)){
			uu->BList = CARMCOM;
			this->builders.push_back(uu);
			G->L->print("NTAI Arm");
			race = R_ARM;
			set = true;
			return;
		}

		// TLL
		na = "TLLCOM";
		if((uu->ud->name == na)&&(set == false)){
			uu->BList = CTLLCOM;
			this->builders.push_back(uu);
			G->L->print("NTAI TLL");
			this->race = R_TLL;
			set = true;
			return;
		}

		// the questionable races for which some support has been added but not total support.

		// RHYOSS
		na = "Rhycom";
		if((uu->ud->name == na)&&(set == false)){
			uu->BList = CRHYCOM;
			this->builders.push_back(uu);
			G->L->print("NTAI Rhyoss");
			race = R_RHY;
			set = true;
			return;
		}

		// XECT
		na = "XCLCOM";
		if((uu->ud->name == na)&&(set == false)){
			uu->BList = CXCLCOM;
			this->builders.push_back(uu);
			G->L->print("NTAI Xect");
			race = R_XECT;
			set = true;
			return;
		}

		// MYNN
		na = "RACCOM";
		if((uu->ud->name == na)&&(set == false)){
			uu->BList = CRACCOM;
			this->builders.push_back(uu);
			G->L->print("NTAI Mynn");
			race = R_MYNN;
			set = true;
			return;
		}

		this->UnitIdle(uu->uid);
	}
	//}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Cycle through builders, here we will assign construction Tasks
	// to newly built units, or factories that ahve finished their tasks.
	// We will also check if the unit has stalled and give it a shove in
	// the right direction if it has.
	//{
	if((builders.empty() == false)&&(Tframe > 8)){
		// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		// Check our builders for null pointers.
		
		for(vector<Tunit*>::iterator tu = builders.begin(); tu != builders.end();++tu){
			if((*tu) == 0){
				delete (*tu);
				builders.erase(tu);
				break;
			}else if ((*tu)->bad == true){
				delete (*tu);
				builders.erase(tu);
				break;
			}
		}
		// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		// Check Check for untis without build lists and assign them where arppopriate
		for(vector<Tunit*>::iterator ui = builders.begin();ui !=builders.end();++ui){
			Tunit* uz = *ui;
			if(uz == 0){
				G->L->eprint("uz == 0");
				vector<Tunit*>::iterator uj = ui;
				ui++;
				delete uz;
				builders.erase(uj);
				continue;
			}
			if(uz->bad == true){
				G->L->eprint("uz->bad == true");
				vector<Tunit*>::iterator uj = ui;
				ui++;
				delete uz;
				builders.erase(uj);
				continue;
			}
			if(G->cb->GetUnitDef(uz->uid) == 0){
				G->L->eprint("GetUnitDef(uz->uid) == 0");
				vector<Tunit*>::iterator uj = ui;
				ui++;
				delete uz;
				builders.erase(uj);
				continue;
			}
			if(uz->waiting == true){
				if(uz->tasks.empty() == false){
					bool apax = G->Pl->feasable(uz->tasks.back()->targ,uz->uid);
					if(apax == true){
						UnitIdle(uz->uid);
						uz->waiting = false;
						continue;
					}
				}else{
					uz->waiting = false;
				}
			}
			int frame = Tframe - uz->creation;
			if(G->cb->GetUnitHealth(uz->uid)<1){
				G->L->print("unitfinished doesnt exist?! Could it have been destroyed?");		
				continue;
			}

			// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
			/* Are we going to use solar generation or Wind generation?*/
			//{

			string ener = "";
			bool ew = true;
			if(energy == ener){
				if((G->cb->GetMaxWind()>18)&&(G->cb->GetMinWind()>14)){
					ew = false;
				}
			}
			//}

			// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
			// Unit Build Trees.
			// Tasks are added back to front, the first task to eb executed is
			// the task that is assigned last.
			//{
			/* CORE commander */if(uz->BList == CCORCOM){
				if(frame == 5){
					if(ew == true){
						energy = "CORSOLAR";
					}else{
						energy = "CORWIND";
					}
					uz->AddTask("CORAP");
					uz->AddTask("CORMAKR");
					uz->AddTask("CORMAKR");
					uz->AddTask("CORMEX");
					uz->AddTask("CORMAKR");
					uz->AddTask("CORMEX");
					uz->AddTask("CORMAKR");
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORSY");
					uz->AddTask("CORALAB");
					uz->AddTask("CORMEX");
					uz->AddTask("CORMAKR");
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMAKR");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask("CORMEX");
					uz->AddTask("CORALAB");
					uz->AddTask("CORLLT");
					uz->AddTask("CORMAKR");
					uz->AddTask("CORMEX");
					uz->AddTask("CORMAKR");
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMAKR");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORRAD");
					uz->AddTask("CORMEX");
					uz->AddTask("CORMEX");
					uz->AddTask("CORLAB");
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					G->basepos = G->cb->GetUnitPos(uz->uid);
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* ARM commander */if(uz->BList == CARMCOM){
				if(ew == true){
					energy = "ARMSOLAR";
				}else{
					energy = "ARMWIND";
				}
				if(frame == 5){
					uz->AddTask("ARMAP");
					uz->AddTask("ARMMAKR");
					uz->AddTask("ARMMAKR");
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMSY");
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMMAKR");
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMMAKR");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMLAB");
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMRAD");
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMLAB");
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					G->basepos = G->cb->GetUnitPos(uz->uid);
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* RHYOSS commander */if(uz->BList == CRHYCOM){
				if(ew == true){
					energy = "RHYSOLAR";
				}else{
					energy = "Rhywin";
				}
				if(frame == 5){
					uz->AddTask("Rhyairplant");
					uz->AddTask("Rhymaker");
					uz->AddTask("Rhymaker");
					uz->AddTask("RHYMEX");
					uz->AddTask("RHYMEX");
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask("Rhysy");
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask("Rhymaker");
					uz->AddTask(energy);
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask("RHYMEX");
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask("Rhymaker");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask("Rhytrooplab");
					uz->AddTask("RHYMEX");
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask("Rhyradar");
					uz->AddTask("RHYMEX");
					uz->AddTask("RHYMEX");
					uz->AddTask("Rhytrooplab");
					uz->AddTask("RHYMEX");
					uz->AddTask(energy);
					uz->AddTask("RHYMEX");
					G->basepos = G->cb->GetUnitPos(uz->uid);
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* TLL commander */if((uz->BList == CTLLCOM)){
				if(ew == true){
					energy = "TLLSOLAR";
				}else{
					energy = "TLLWINDTRAP";
				}
				if(frame == 5){
					uz->AddTask("TLLAP");
					uz->AddTask("TLLMM");
					uz->AddTask("TLLMM");
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLSY");
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLMM");
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLMM");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLLAB");
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLRADAR");
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLLAB");
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLLFT");
					G->basepos = G->cb->GetUnitPos(uz->uid);
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* Xect commander */if(uz->BList == CXCLCOM){
					energy = "XCLSOLAR";
				if(frame == 5){
					uz->AddTask("XCLGXYLAB");
					uz->AddTask("XCLMAKER");
					uz->AddTask("XCLMAKER");
					uz->AddTask("XCLMEX");
					uz->AddTask("XCLMEX");
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask("XCLLAB");//AIDLL=aidllglobalaiNTAI.dll;
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask("XCLMAKER");
					uz->AddTask(energy);
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask("XCLMEX");
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask("XCLMAKER");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask("XCLUSA_LAB");
					uz->AddTask("XCLMEX");
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask("XCLDAR");
					uz->AddTask("XCLMEX");
					uz->AddTask("XCLMEX");
					uz->AddTask("XCLKAJLAB");
					uz->AddTask("XCLMEX");
					uz->AddTask(energy);
					uz->AddTask("XCLMEX");
					uz->AddTask("XCLLLT");
					G->basepos = G->cb->GetUnitPos(uz->uid);
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* Mynn commander */if(uz->BList == CRACCOM){
				if(ew == true){
					energy = "RACENER";
				}else{
					energy = "RACWIN";
				}
				if(frame == 5){
					uz->AddTask("RACAP");
					uz->AddTask("RACMET");
					uz->AddTask("RACMET");
					uz->AddTask("RACMETAL");
					uz->AddTask("RACMETAL");
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask("RACSY");
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask("RACMET");
					uz->AddTask(energy);
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask("RACMETAL");
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask("RACMET");
					uz->AddTask(energy);
					uz->AddTask(energy);
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask("RACLAB");
					uz->AddTask("RACMETAL");
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask("RACDAR");
					uz->AddTask("RACMETAL");
					uz->AddTask("RACMETAL");
					uz->AddTask("RACLAB");
					uz->AddTask("RACMETAL");
					uz->AddTask(energy);
					uz->AddTask("RACMETAL");
					uz->AddTask("RACLLT");
					G->basepos = G->cb->GetUnitPos(uz->uid);
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* CORE con kbot/aircraft */if(uz->BList == CORCON){
				if(frame == 150){
					uz->AddTask("CORADVSOL");
					uz->AddTask("CORMEX");
					uz->AddTask("CORAVP");
					uz->AddTask("CORMEX");
					uz->AddTask("CORALAB");
					uz->AddTask("CORADVSOL");
					uz->AddTask("CORMEX");
					uz->AddTask("CORAP");
					uz->AddTask("CORESTOR");
					uz->AddTask("CORALAB");
					uz->AddTask("CORRAD");
					uz->AddTask("CORESTOR");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask("CORRL");
					uz->AddTask(energy);
					if((Tframe%3)==1){
						uz->AddTask("CORMAKR");
						uz->AddTask(energy);
						uz->AddTask(energy);
						if(Tframe<15000){
							uz->AddTask("CORALAB");
							uz->AddTask("CORRL");
						} else {
							uz->AddTask("CORTOAST");
							uz->AddTask("CORPUN");
							uz->AddTask("CORMEX");
						}
						uz->AddTask(energy);
					}else if((Tframe%3)==2){
						uz->AddTask(energy);
						if(Tframe>10000){
							uz->AddTask("CORADVSOL");
							uz->AddTask("CORADVSOL");
							uz->AddTask("CORMEX");
							uz->AddTask("CORALAB");
							uz->AddTask("CORTOAST");
							uz->AddTask("CORPUN");
						}
						if(Tframe<10000){
							uz->AddTask("CORMEX");
							uz->AddTask("CORMEX");
							uz->AddTask("CORLAB");
							uz->AddTask("CORRL");
						}
						uz->AddTask("CORRL");
						uz->AddTask(energy);
						uz->AddTask("CORMEX");
						uz->AddTask("CORADVSOL");
						uz->AddTask(energy);
						uz->AddTask("CORRL");
					}else{
						uz->AddTask("CORMEX");
						uz->AddTask(energy);
						uz->AddTask("CORMEX");
						uz->AddTask(energy);
						uz->AddTask(energy);
						uz->AddTask("CORMEX");
						uz->AddTask("CORMEX");
						uz->AddTask("CORHLT");
						uz->AddTask(energy);
						uz->AddTask("CORMEX");
						uz->AddTask("CORRL");
						if((Tframe%2)==1){
							uz->AddTask("CORMEX");
							uz->AddTask("CORRL");
							uz->AddTask("CORMAKR");
							uz->AddTask("CORADVSOL");
							uz->AddTask("CORMEX");
							uz->AddTask("CORMEX");
							uz->AddTask(energy);
							uz->AddTask("CORMEX");
						}
					}
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					if(threem == false){
						uz->AddTask("CORRL");
						uz->AddTask("CORRL");
						uz->AddTask("CORRL");
						threem = true;
					}
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* ARM con kbot/aircraft */if(uz->BList == CARMCON){
				if(ew == true){
					energy = "ARMSOLAR";
				}else{
					energy = "ARMWIND";
				}
				if(frame == 150){
					uz->AddTask("ARMADVSOL");
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMAVP");
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMALAB");
					uz->AddTask("ARMADVSOL");
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMAP");
					uz->AddTask("ARMESTOR");
					uz->AddTask("ARMALAB");
					uz->AddTask("ARMRAD");
					uz->AddTask("ARMESTOR");
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					uz->AddTask("ARMRL");
					uz->AddTask(energy);
					if((Tframe%3)==1){
						uz->AddTask("ARMMAKR");
						uz->AddTask(energy);
						uz->AddTask(energy);
						if(Tframe<15000){
							uz->AddTask("ARMALAB");
							uz->AddTask("ARMRL");
						} else {
							uz->AddTask("ARMTOAST");
							uz->AddTask("ARMPUN");
							uz->AddTask("ARMMEX");
						}
						uz->AddTask(energy);
					}else if((Tframe%3)==2){
						uz->AddTask(energy);
						if(Tframe>10000){
							uz->AddTask("ARMADVSOL");
							uz->AddTask("ARMADVSOL");
							uz->AddTask("ARMMEX");
							uz->AddTask("ARMALAB");
							uz->AddTask("ARMTOAST");
							uz->AddTask("ARMPUN");
						}
						if(Tframe<10000){
							uz->AddTask("ARMMEX");
							uz->AddTask("ARMMEX");
							uz->AddTask("ARMLAB");
							uz->AddTask("ARMRL");
						}
						uz->AddTask("ARMRL");
						uz->AddTask(energy);
						uz->AddTask("ARMMEX");
						uz->AddTask("ARMADVSOL");
						uz->AddTask(energy);
						uz->AddTask("ARMRL");
					}else{
						uz->AddTask("ARMMEX");
						uz->AddTask(energy);
						uz->AddTask("ARMMEX");
						uz->AddTask(energy);
						uz->AddTask(energy);
						uz->AddTask("ARMMEX");
						uz->AddTask("ARMMEX");
						uz->AddTask("ARMHLT");
						uz->AddTask(energy);
						uz->AddTask("ARMMEX");
						uz->AddTask("ARMRL");
						if((Tframe%2)==1){
							uz->AddTask("ARMMEX");
							uz->AddTask("ARMRL");
							uz->AddTask("ARMMAKR");
							uz->AddTask("ARMADVSOL");
							uz->AddTask("ARMMEX");
							uz->AddTask("ARMMEX");
							uz->AddTask(energy);
							uz->AddTask("ARMMEX");
						}
					}
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					uz->AddTask(energy);
					uz->AddTask("ARMMEX");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* TLL con kbot/aircraft */if(uz->BList == CTLLCON){
				if(ew == true){
					energy = "TLLSOLAR";
				}else{
					energy = "TLLWINDTRAP";
				}
				if(frame == 150){
					//uz->AddTask("CORADVSOL");
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLAVP");
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLALAB");
					//uz->AddTask("CORADVSOL");
					uz->AddTask("TLLMEX");
					uz->AddTask("CORAP");
					uz->AddTask("TLLESTOR");
					uz->AddTask("TLLALAB");
					uz->AddTask("TLLRADAR");
					uz->AddTask("TLLESTOR");
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask("TLLRRL");
					uz->AddTask(energy);
					if((Tframe%3)==1){
						uz->AddTask("TLLMM");
						uz->AddTask(energy);
						uz->AddTask(energy);
						if(Tframe<15000){
							uz->AddTask("TLLALAB");
							uz->AddTask("TLLRL");
						} else {
							uz->AddTask("TLLTOAST");
							uz->AddTask("TLLPUN");
							uz->AddTask("TLLMEX");
						}
						uz->AddTask(energy);
					}else if((Tframe%3)==2){
						uz->AddTask(energy);
						if(Tframe>10000){
							uz->AddTask("TLLADVSOL");
							uz->AddTask("TLLADVSOL");
							uz->AddTask("TLLMEX");
							uz->AddTask("TLLALAB");
							uz->AddTask("TLLTOAST");
							uz->AddTask("TLLPUN");
						}
						if(Tframe<10000){
							uz->AddTask("TLLMEX");
							uz->AddTask("TLLMEX");
							uz->AddTask("TLLLAB");
							uz->AddTask("TLLRL");
						}
						uz->AddTask("TLLRL");
						uz->AddTask(energy);
						uz->AddTask("TLLMEX");
						uz->AddTask("TLLADVSOL");
						uz->AddTask(energy);
						uz->AddTask("TLLRL");
					}else{
						uz->AddTask("TLLMEX");
						uz->AddTask(energy);
						uz->AddTask("TLLMEX");
						uz->AddTask(energy);
						uz->AddTask(energy);
						uz->AddTask("TLLMEX");
						uz->AddTask("TLLMEX");
						uz->AddTask("TLLHLT");
						uz->AddTask(energy);
						uz->AddTask("TLLMEX");
						uz->AddTask("TLLRL");
						if((Tframe%2)==1){
							uz->AddTask("TLLMEX");
							uz->AddTask("TLLRL");
							uz->AddTask("TLLMAKR");
							uz->AddTask("TLLADVSOL");
							uz->AddTask("TLLMEX");
							uz->AddTask("TLLMEX");
							uz->AddTask(energy);
							uz->AddTask("TLLMEX");
						}
					}
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					uz->AddTask(energy);
					uz->AddTask("TLLMEX");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* con ship */if((uz->BList == CORCS)){
				if(frame == 150){
					uz->AddTask("CORASY");
					uz->AddTask("CORUWES");
					uz->AddTask("CORSONAR");
					uz->AddTask("CORUWES");
					uz->AddTask("CORTIDE");
					uz->AddTask("CORTIDE");
					uz->AddTask("CORFRT");
					uz->AddTask("CORTIDE");
					if((Tframe%3)==1){
						uz->AddTask("CORFMKR");
						uz->AddTask("CORTIDE");
						uz->AddTask("CORTIDE");
						if(Tframe<15000){
							uz->AddTask("CORALAB");
							uz->AddTask("CORFRT");
						} else {
							uz->AddTask("CORTL");
							uz->AddTask("CORTL");
						}
						uz->AddTask("CORTIDE");
						uz->AddTask("CORFRT");
					}else if((Tframe%3)==2){
						uz->AddTask("CORTIDE");
						if(Tframe>10000){
							uz->AddTask("CORUWMEX");
							uz->AddTask("CORASY");
							uz->AddTask("CORTL");
							uz->AddTask("CORTL");
						}
						if(Tframe<10000){
							uz->AddTask("CORUWMEX");
							uz->AddTask("CORUWMEX");
							uz->AddTask("CORFRT");
						}
						uz->AddTask("CORFRT");
						uz->AddTask("CORTIDE");
						uz->AddTask("CORUWMEX");
						uz->AddTask("CORTIDE");
						uz->AddTask("CORFRT");
					}else{
						uz->AddTask("CORUWMEX");
						uz->AddTask("CORTIDE");
						uz->AddTask("CORUWMEX");
						uz->AddTask("CORTIDE");
						uz->AddTask("CORTIDE");
						uz->AddTask("CORUWMEX");
						uz->AddTask("CORUWMEX");
						uz->AddTask("CORFHLT");
						uz->AddTask("CORTIDE");
						uz->AddTask("CORUWMEX");
						uz->AddTask("CORFRT");
						if((Tframe%2)==1){
							uz->AddTask("CORUWMEX");
							uz->AddTask("CORFRT");
							uz->AddTask("CORMAKR");
							uz->AddTask("CORTIDE");
							uz->AddTask("CORUWMEX");
							uz->AddTask("CORUWMEX");
							uz->AddTask("CORTIDE");
							uz->AddTask("CORUWMEX");
						}
					}
					uz->AddTask(energy);
					uz->AddTask("CORMEX");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core necro */if((uz->BList == CORNECRO)){
				if(frame == 150){
					uz->AddTask("CORMOHO");
					uz->AddTask("CORARAD");
					if((Tframe%2)==0){
						uz->AddTask("CORMOHO");
					} else{
						uz->AddTask("CORMOHO");
						uz->AddTask("CORHLT");
					}
					uz->AddTask("CORMOHO");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core adv. con kbot   */if((uz->BList == CORACK)){
				if(frame == 150){
					uz->AddTask("x1ebcorjtorn");
					srand(Tframe);
					for(int gi = 0; gi != 2;gi += rand()){
						if(((Tframe+gi)%17)==0){
							uz->AddTask("CORGANT");
						} else if(((Tframe+gi)%17)==16){
							uz->AddTask("CORTRON");
						} else if(((Tframe+gi)%17)==15){
							uz->AddTask("CORTRON");
						} else if(((Tframe+gi)%17)==14){
							uz->AddTask("CORDOOM");
						} else if(((Tframe+gi)%17)==13){
							uz->AddTask("CORDOOM");
						} else if(((Tframe+gi)%17)==12){
							uz->AddTask("CORINT");
						} else if(((Tframe+gi)%17)==11){
							uz->AddTask("CORGANT");
						} else if(((Tframe+gi)%17)==10){
							uz->AddTask("CORINT");
						} else if(((Tframe+gi)%17)==8){
							uz->AddTask("CORGANT");
						} else if(((Tframe+gi)%17)==9){
							uz->AddTask("CORGANT");
						} else if(((Tframe+gi)%17)==7){
							uz->AddTask("CORDOOM");
						} else if(((Tframe+gi)%17)==6){
							uz->AddTask("CORPUN");
						} else if(((Tframe+gi)%17)==5){
							uz->AddTask("CORSILO");
						} else if(((Tframe+gi)%17)==4){
							uz->AddTask("CORPUN");
						} else if(((Tframe+gi)%17)==3){
							uz->AddTask("CORSILO");
						} else if(((Tframe+gi)%17)==2){
							uz->AddTask("CORINT");
						} else if(((Tframe+gi)%17)==1){
							uz->AddTask("CORMOHO");
						}
					}
					uz->AddTask("CORARAD");
					if((Tframe%4)==0){
						uz->AddTask("CORMOHO");
						uz->AddTask("CORFLAK");
						uz->AddTask("CORFUS");
					} else if((Tframe%4)==1){
						uz->AddTask("x1corminifus");
						uz->AddTask("CORFUS");
						uz->AddTask("CORMOHO");
						uz->AddTask("CORMMKR");
						uz->AddTask("CORFLAK");
					} else if((Tframe%4) == 2){
						uz->AddTask("CORMOHO");
						uz->AddTask("x1corminifus");
						uz->AddTask("CORMMKR");
					}
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core kbot lab */if(uz->BList == CORLAB){
				if(uz->tasks.empty() == true){
					uz->AddTask("CORFAST");
					uz->AddTask("CORAK");
					uz->AddTask("CORAK");
					uz->AddTask("CORAK");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORAK");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORFAST");
					uz->AddTask("CORAK");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORCRASH");
					uz->AddTask("CORAK");
					uz->AddTask("CORAK");
					uz->AddTask("CORAK");
					uz->AddTask("CORAK");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORAK");
					uz->AddTask("CORTHUD");
					if(frame<30000){
						uz->AddTask("CORCK");
						uz->AddTask("CORCK");
					}
					uz->AddTask("CORAK");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORCRASH");
					uz->AddTask("CORAK");
					uz->AddTask("CORAK");
					uz->AddTask("CORTHUD");
					uz->AddTask("CORAK");
					uz->AddTask("CORAK");
					uz->AddTask("CORFAST");
					if(frame<30000){
						uz->AddTask("CORFAST");
						uz->AddTask("CORCK");
					}
					uz->AddTask("CORCK");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* TLL kbot lab */if((uz->BList == CTLLLAB)){
			if(uz->tasks.empty() == true){
					uz->AddTask("TLLAAK");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLAAK");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLpbot");
					uz->AddTask("TLLAAK");
					if(frame<30000){
						uz->AddTask("TLLCK");
					}
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLPRIVATE");
					uz->AddTask("TLLCK");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* ARM kbot lab */if((uz->BList == CARMLAB)){
				if(uz->tasks.empty() == true){
					uz->AddTask("ARMFAST");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMJETH");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMHAM");
					uz->AddTask("ARMJETH");
					if(frame<30000){
						uz->AddTask("ARMCK");
					}
					uz->AddTask("ARMPW");
					uz->AddTask("ARMFAST");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					uz->AddTask("ARMPW");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core ALAB  */if((uz->BList == CORALAB)){
				if(uz->tasks.empty() == true){
					uz->AddTask("CORSUMO");
					uz->AddTask("CORFAST");
					uz->AddTask("CORPYRO");
					uz->AddTask("CORSUMO");
					uz->AddTask("CORNECRO");
					uz->AddTask("CORMORT");
					uz->AddTask("CORHRK");
					uz->AddTask("CORMORT");
					uz->AddTask("CORCAN");
					uz->AddTask("CORPYRO");
					uz->AddTask("CORSPEC");
					uz->AddTask("CORCAN");
					uz->AddTask("CORPYRO");
					uz->AddTask("CORMORT");
					uz->AddTask("CORPYRO");
					uz->AddTask("CORPYRO");
					uz->AddTask("CORFAST");
					uz->AddTask("CORACK");
					if(Tframe>30000){
						uz->AddTask("CORACK");
						uz->AddTask("CORPYRO");
						uz->AddTask("CORPYRO");
					}
					uz->AddTask("CORHRK");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* arm ALAB  */if((uz->BList == CARMALAB)){
				if(uz->tasks.empty() == true){
					uz->AddTask("ARMMAV");
					uz->AddTask("ARMMAV");
					uz->AddTask("ARMFARK");
					uz->AddTask("ARMZEUS");
					uz->AddTask("ARMFIDO");
					uz->AddTask("ARMZEUS");
					uz->AddTask("ARMZEUS");
					uz->AddTask("ARMZEUS");
					uz->AddTask("ARMSPY");
					uz->AddTask("ARMFIDO");
					uz->AddTask("ARMFIDO");
					uz->AddTask("ARMFIDO");
					uz->AddTask("ARMFIDO");
					uz->AddTask("ARMACK");
					uz->AddTask("ARMFAST");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* TLL ALAB  */if((uz->BList == CTLLALAB)){
				if(uz->tasks.empty() == true){
					uz->AddTask("TLLARTYBOT");
					uz->AddTask("TLLARTYBOT");
					uz->AddTask("TLLARCHNANO");
					uz->AddTask("TLLLASBOT");
					uz->AddTask("TLLLASBOT");
					uz->AddTask("TLLLASBOT");
					uz->AddTask("TLLARTYBOT");
					uz->AddTask("TLLLASBOT");
					uz->AddTask("TLLOBSERVER");
					uz->AddTask("TLLARTYBOT");
					uz->AddTask("TLLARTYBOT");
					uz->AddTask("TLLLASBOT");
					uz->AddTask("TLLLASBOT");
					uz->AddTask("TLLACK");
					uz->AddTask("TLLBUG");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core Krogoth gantry */if((uz->BList == CORGANT)){
				if(uz->tasks.empty() == true){
					uz->AddTask("CORKROG");
					uz->AddTask("CORKROG");
					uz->AddTask("CORKROG");
					uz->AddTask("CORKROG");
					uz->AddTask("CORKROG");
					uz->AddTask("CORKROG");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core Aircraft Plant    */if((uz->BList == CORAP)){
				if(uz->tasks.empty() == true){
					uz->AddTask("CORCA");
					uz->AddTask("CORSFIG");
					uz->AddTask("CORSFIG");
					uz->AddTask("CORCA");
					uz->AddTask("CORSFIG");
					uz->AddTask("CORFINK");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core vehicle plant */if(uz->BList == CORVP){
				if(uz->tasks.empty() == true){
					uz->AddTask("CORCV");
					uz->AddTask("CORLEVLR");
					uz->AddTask("CORLEVLR");
					uz->AddTask("CORLEVLR");
					uz->AddTask("CORFAV");
					uz->AddTask("CORMIST");
					uz->AddTask("CORMIST");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core ADV vehicle plant */if(uz->BList == CORAVP){
				if(uz->tasks.empty() == true){
					uz->AddTask("CORACV");
					uz->AddTask("CORRAID");
					uz->AddTask("CORGOL");
					uz->AddTask("CORSEAL");
					uz->AddTask("CORSENT");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core shipyard */if(uz->BList == CORSY){
				if(uz->tasks.empty() == true){
					uz->AddTask("CORCS");
					uz->AddTask("CORPT");
					uz->AddTask("CORSUB");
					uz->AddTask("CORROY");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			/* core ADV shipyard */if(uz->BList == CORASY){
				if(uz->tasks.empty() == true){
					uz->AddTask("CORACSUB");
					uz->AddTask("CORCRUS");
					uz->AddTask("CORCRUS");
					uz->AddTask("CORSHARK");
					uz->AddTask("CORMSHIP");
					uz->AddTask("CORBATS");
					uz->AddTask("CORCRUS");
					uz->AddTask("CORCARRY");
					uz->AddTask("CORSSUB");
					uz->AddTask("CORARCH");
					uz->AddTask("CORSSUB");
					uz->AddTask("CORCRUS");
					uz->AddTask("CORSJAM");
					uz->AddTask("CORBATS");
					uz->AddTask("CORCRUS");
					uz->AddTask("CORSSUB");
					this->UnitIdle(uz->uid);
					continue;
				}
			}
			//}
			// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
			// Get the unit commands and check if they're valid. This is to prevent untis from stalling and doing nothing by looking for idle untis that ahve tasks.
			//{
			if((frame >20)&&(Tframe>300)&&(uz->tasks.empty() == false)&&(uz->waiting == false)&&(uz->guarding == false)&&(uz->scavenge == false)){
				const deque<Command>* dc = G->cb->GetCurrentUnitCommands(uz->uid);
				
				bool stopped = false;
				bool build = false;
				if(dc == 0){
					stopped = true;
				}else{
					if((dc->empty() == false)){
						for(deque<Command>::const_iterator di = dc->begin();di != dc->end();++di){
							if((di->id == CMD_STOP)||(di->id == CMD_WAIT)){
								stopped = true;
								break;
							}
							if((di->id == CMD_GUARD)&&(uz->tasks.empty() == false)){
								stopped = true;
								break;
							}
							if(di->id < 0){
								build = true;
								stopped = false;
								break;
							}
						}
					}else{
						stopped = true;
					}
				}
				if((stopped == true)&&(build == false)&&(uz->waiting == false)&&(uz->tasks.empty() == false)&&(uz->guarding == false)&&(uz->scavenge == false)){
					cfluke[uz->uid] += 1;
					if(cfluke[uz->uid] > 16){
						cfluke[uz->uid] = 0;
						G->Fa->UnitIdle(uz->uid);
					}
					//continue;
				}else{
					cfluke[uz->uid] = 0;
				}
			}
			//}
		}
	}
	//}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitIdle(int unit){
	if(G->cb->GetUnitHealth(unit)<1){
		G->L->print("unitfinished doesnt exist?! Could it have been destroyed?");		
		return;
	}
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0){
		exprint("UnitDefloaderror factor::UnitIdle");
		return;
	}
	if(builders.empty() == false){
		for(vector<Tunit*>::iterator uj = builders.begin(); uj != builders.end();++uj){
			Tunit* uv = *uj;
			uv->frame++;
			if(G->cb->GetUnitHealth(uv->uid)<1){
				G->L->print("unitidle doesnt exist?! Could it have been destroyed?");		
				continue;
			}
			if(uv->tasks.empty() == true){
				if(uv->uid == unit){
					if(uv->repeater == true){
						uv->creation = G->cb->GetCurrentFrame();
						continue;
					}
					if((uv->guarding == false)&&(uv->scavenge == false)&&(uv->tasks.empty() != false)&&(uv->frame >200)){
						if(uv->ud->canResurrect == false){
							for(vector<Tunit*>::iterator bu = builders.begin();bu != builders.end(); ++bu){
								Tunit* vu = *bu;
								if(vu->tasks.empty() == false){
									if(G->cb->GetUnitHealth(vu->uid)<1){
										G->L->print("unitfinished doesnt exist?! Could it have been destroyed?");
										return;
									}
									Command* c = new Command;
									c->id = CMD_GUARD;
									c->params.push_back(vu->uid);
									G->cb->GiveOrder(uv->uid,c);
									uv->guarding = true;
									break;
								}
							}
						}else{
							Command* c = new Command;
							c->id = CMD_RESURRECT;
							c->params.push_back(G->basepos.x);
							c->params.push_back(G->basepos.y);
							c->params.push_back(G->basepos.z);
							c->params.push_back(max(G->cb->GetMapWidth(),G->cb->GetMapHeight()));
							G->cb->GiveOrder(uv->uid,c);
							Command* c3 = new Command;
							c3->id = CMD_REPEAT;
							c3->params.push_back(1);
							G->cb->GiveOrder(uv->uid,c3);
							uv->scavenge = true;
						}
					}
				}
			} else if(uv->uid == unit){
				if(uv->tasks.empty() == true){
					continue;
				}else{
					bool possible;
					Task* task = 0;
					for(list<Task* >::iterator ti = uv->tasks.begin();(ti != uv->tasks.end())&&(uv->tasks.empty() == false);++ti){
						if(*ti == 0) continue;
						task = *ti;
						if(task == 0){
							//uv->tasks.erase(ti);
							continue;
						}
						if(task->valid == false){
							delete task;
							//uv->tasks.erase(ti);
							task = 0;
							*ti = 0;
							continue;
						}
						possible = false;
						if(G->Pl->feasable(task->targ.c_str(),uv->uid) == false){
							uv->waiting = true;
							break;
						}
						possible = task->execute(uv->uid);
						if(possible == true ){
							delete task;
							uv->tasks.erase(ti);
							task = 0;
							*ti = 0;
							break;
						} else{
							delete task;
							//uv->tasks.erase(ti);
							task = 0;
							*ti = 0;
							G->L->print("error encountered, starting new task iteration");
						}
					}
				}
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitDestroyed(int unit){
	G->M->removeExtractor(unit);
	if(builders.empty() == false){
		for(vector<Tunit*>::iterator wu = builders.begin(); wu != builders.end();++wu){
			Tunit* vu = *wu;
			if(vu !=0){
				if(vu->uid == unit){
					delete vu;
					builders.erase(wu);
					break;
				}
			}
		}
		if(cfluke.empty() == false){
			for(map<int,int>::iterator mi = cfluke.begin(); mi != cfluke.end(); ++mi){
				if(mi->first == unit){
					cfluke.erase(mi);
					break;
				}
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

