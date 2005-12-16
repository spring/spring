#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::InitAI(Global* GLI){
	G = GLI;

	// calculate number of sectors
	xSectors = int(floor(0.5 + ((float) G->cb->GetMapWidth())/50));
	ySectors = int(floor(0.5 + ((float) G->cb->GetMapHeight())/50));

	// calculate effective sector size
	xSizeMap = int(floor( ((float) G->cb->GetMapWidth()) / ((float) xSectors) ));
	ySizeMap = int(floor( ((float) G->cb->GetMapHeight()) / ((float) ySectors) )); 
	xSize = 8 * xSizeMap;
	ySize = 8 * ySizeMap,
	
	// create field of sectors
	sector = new agrid*[xSectors];
	for(int i = 0; i < xSectors; i++){
		sector[i] = new agrid[ySectors];

		for(int j = 0; j < ySectors; j++){
			// set coordinates of the sectors
			sector[i][j].centre = float3(((xSize*i)+(xSize*(i+1)))/2,0, ((ySize * j)+(ySize * (j+1)))/2 );
			sector[i][j].location = float3(i,0,j);
			sector[i][j].ground_threat = 0;
			sector[i][j].ebuildings = 0;
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::Add(int unit){
	G->L->print("Chaser::Add()");
	Attackers.insert(unit);
	acknum++;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDestroyed(int unit){
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0) return;
	if(groups.empty() == false){
        for(vector<ackforce>::iterator at = groups.begin();at !=groups.end();++at){
			for(set<int>::iterator aa = at->units.begin(); aa != at->units.end();++aa){
				if(*aa == unit){
					at->units.erase(aa);
					at->acknum -= 1;
					if(at->units.empty()) groups.erase(at);
					if(at->units.empty() == true) threshold++;
					return;
				}
			}
		}
	}
	if(Attackers.empty() == false){
        for(set<int>::iterator aiy = Attackers.begin(); aiy != Attackers.end();++aiy){
			if(*aiy == unit){
				Attackers.erase(aiy);
				acknum--;
				break;
			}
		}
	}
	if(sweap.empty() == false){
		if(sweap.find(unit) != sweap.end()){
			sweap.erase(unit);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	const UnitDef* ud = G->cb->GetUnitDef(damaged);
	if(ud == 0) return;
	Command Cmd;
	if(ud->weapons.empty() == false){
		for(vector<UnitDef::UnitDefWeapon>::const_iterator wu = ud->weapons.begin();wu !=ud->weapons.end();++wu){
			if(wu->def->manualfire == true){
				/*If a unit needs to fire a special weapon and its building soemthing then it is likely to either die or its next building
				project has a high chance of dieing, or being destroyed by the special weapon. It isnt worth continuing the task fi you
				encounter a new set of circumstances, that would be unflexible, and that is not what i want for my AI
				I keep this code here for those who can make use of it*/
				Cmd.id = CMD_DGUN;
				Cmd.timeOut = 200;
				Cmd.params.push_back(attacker);
				G->cb->GiveOrder(damaged, &Cmd);
				break;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitFinished(int unit){
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0){
		G->L->print("UnitDef call filed, was this unit blown up as soon as it was created?");
		return;
	}
	Command Cmd;
	if(ud->weapons.empty() == false){
        if(ud->weapons.front().def->stockpile == true){
    		Cmd.id = CMD_STOCKPILE;
			for(int i = 0;i<110;i++){
				G->cb->GiveOrder(unit, &Cmd);
			}
			sweap.insert(unit);
		}
	}
	vector<string> v;
	v = bds::set_cont(v, "CORKROG, CORPYRO, CORSUMO, CORHRK, TLLAAK, TLLPBOT, TLLPRIVATE, ARMZEUS, ARMFIDO, ARMMAV, ARMPW, ARMJETH, ARMHAM, TLLLASBOT, TLLARTYBOT, CORBATS, CORCRUS, CORSHARK, CORSSUB, CORMSHIP, CORSUB, CORROY, CORGOL, CORSENT, CORRAID, CORAK, CORTHUD, CORMIST, CORLEVLR, CORSEAL, CORCAN, CORMORT, CORSTORM, CORCRASH");
	bool atk = false;
	for(vector<string>::iterator vi = v.begin(); vi != v.end();++vi){
		if(*vi == ud->name){
			atk = true;
			break;
		}
	}
	if(atk == true){
		Command* c = new Command;
		c->id = CMD_MOVE_STATE;
		c->params.push_back(3);
		G->cb->GiveOrder(unit,c);
		Command* c2 = new Command;
		c2->id = CMD_TRAJECTORY;
		c2->params.push_back(3);
		G->cb->GiveOrder(unit,c);
		Add(unit);
		string mess = "new attacker added :";
		mess += ud->name.c_str();
		G->L->print(mess.c_str());
		acknum++;
		if(acknum>threshold){
			bool idled = false;
			if((groups.empty() == false)&&(groups.back().gid != groups.front().gid)){
				for(vector<ackforce>::iterator ad = groups.begin();ad != groups.end();++ad){
					if(ad->idle){
						ad->acknum += acknum;
						for(set<int>::iterator acf = Attackers.begin();acf != Attackers.end();++acf){
							ad->units.insert(*acf);
						}
						idled = true;
						Attackers.clear();
						acknum = 0;
						FindTarget(&*ad);
						break;
					}
				}
			}
			if(idled == false){
                ackforce ac(G);
				ac.acknum = this->acknum;
				forces++;
				ac.gid = forces;
				ac.idle = true;
				ac.units = Attackers;
				groups.push_back(ac);
				Attackers.clear();
				acknum = 0;
				FindTarget(&groups.back());
			}
		} else {
			Attackers.insert(unit);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Chaser::~Chaser(){
	if(sector){
		for(int i = 0; i < xSectors; i++){
			delete [] sector[i];
		}
		delete [] sector;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::EnemyDestroyed(int enemy){
	/*for(int i = 0; i < xSectors; i++){
		for(int j = 0; j < ySectors; j++){
			sector[i][j].ebuildings *= 0.999;
		}
	}*/
	/*if(groups.empty() == false){
		for(vector<ackforce>::iterator ig = groups.begin(); ig != groups.end();++ig){
			if( ig->target == enemy){
				ackforce* a;
				a = &*ig;
				a->idle = true;
				FindTarget(a);
			}
		}
	}*/
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::EnemyLeaveLOS(int enemy){
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::EnemyEnterLOS(int enemy){
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::EnemyEnterRadar(int enemy){
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::FindTarget(ackforce* i){
	G->L->print("Findtarget()");
	if(i->units.empty() == true){
		i->idle =  true;
		return;
	}
	/////
	float best_rating , my_rating, best_ground, my_ground;
	best_rating = my_rating = best_ground = my_ground = 0;
	float3 dest = ZeroVector;
	float3 ground_dest = ZeroVector;
	int sx1,sy1 = 0;
	int sx3,sy3 = 0;

	// TODO: improve destination sector selection
	for(int x = 0; x < xSectors; x++){
		for(int y = 0; y < ySectors; y++){
			my_rating = sector[x][y].ebuildings;
			if(my_rating > best_rating){
				if(i->starg != sector[x][y].centre){
					dest= sector[x][y].centre;
					best_rating = my_rating;
					sx1 = x;
					sy1 = y;
				}
			}
			my_ground = sector[x][y].ground_threat;
			if(my_ground > best_ground){
				if(i->starg != sector[x][y].centre){
					ground_dest= sector[x][y].centre;
					best_ground = my_ground;
					sx3 = x;
					sy3 = y;
				}
			}
		}
	}

	////
	if((best_rating<10)&&(best_ground<10)) return;
	//ma = false;
	//if(ma == true){
	
	float3 final_dest= ZeroVector;
	if(best_rating>(best_ground/2)){
		final_dest = dest;
		if(final_dest == ZeroVector) return;
		i->starg.x = sx1;
		i->starg.y = sy1;
	}else{
		final_dest = ground_dest;
		if(final_dest == ZeroVector) return;
		i->starg.x = sx3;
		i->starg.y = sy3;
	}
	Command* c = new Command;
	c->id = CMD_MOVE;
	c->params.push_back(final_dest.x);
	c->params.push_back(G->cb->GetElevation(final_dest.x,final_dest.z));
	c->params.push_back(final_dest.z);
	
	float3 zpositions = final_dest;
	zpositions.y += 400;
	G->Pl->DrawLine(zpositions,final_dest,60,10,true);
	if(i->units.empty() == false){
		for(set<int>::const_iterator aik = i->units.begin();aik != i->units.end();++aik){
			if(G->cb->GetUnitHealth(*aik) >1){
				G->cb->GiveOrder(*aik, c);
			}
		}
	}
	threshold += 1;
	i->marching = true;
	i->idle = false;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitIdle(int unit){
	vector<ackforce>::iterator af;
	bool a = false;
	vector<ackforce>::iterator bf;
	bool b = false;
	if(groups.empty() == true) return;
	for(vector<ackforce>::iterator ad = groups.begin();ad != groups.end();++ad){
		if(ad->idle){
			if(a == false){
				af = ad;
				a = true;
				continue;
			} else if(b == false){
				bf = ad;
				b = true;
				for(set<int>::iterator acf = bf->units.begin();acf != bf->units.end();++acf){
					af->units.insert(*acf);
					af->acknum++;
				}
				bf->units.clear();
				groups.erase(bf);
				FindTarget(&*af);
				a = false;
				b = false;
				continue;
			}
		}
	}
	/*float3 pos = G->cb->GetUnitPos(unit);
	if(pos != ZeroVector){
		int* exnum = new int[1000];
		int unum = G->cb->GetEnemyUnits(exnum,pos,max(xSize,ySize));
		if((unum == 0)&&(groups.empty() == false)){
			for(vector<ackforce>::iterator ad = groups.begin();ad != groups.end();++ad){
				if(ad->marching == true){
					if(ad->units.find(unit) != ad->units.end()){
						ad->marching = false;
						this->FindTarget(&*ad);
					}
				} 
			}
		}
	}*/
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Fire units in the sweap array, such as nuke silos etc.

void Chaser::FireSilos(){
	int fnum = G->cb->GetCurrentFrame(); // Get frame
	if((fnum%200 == 0)&&(sweap.empty() == false)){ // Every 30 seconds
		float best_rating = 0;
		float my_rating = 0; // Search for the area with the highest threat level.
		for(int x = 0; x < xSectors; x++){
			for(int y = 0; y < ySectors; y++){
				my_rating = sector[x][y].ebuildings;
				if(my_rating > best_rating){
					swtarget= sector[x][y].centre;
					best_rating = my_rating;
				}
			}
		}
		Command* c = new Command;
		c->id = CMD_AREA_ATTACK; // Attack chosen area.
		c->params.push_back(swtarget.x);
		c->params.push_back(swtarget.y);
		c->params.push_back(swtarget.z);
		c->params.push_back(max(xSize,ySize));
		for(set<int>::iterator si = sweap.begin(); si != sweap.end();++si){
			G->cb->GiveOrder(*si,c);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::Update(){
	// decrease threat values of all sectors..
	for(int xa = 0; xa < xSectors; xa++){
		for(int ya = 0; ya < ySectors; ya++){
			if(sector[xa][ya].ebuildings != 0)	sector[xa][ya].ebuildings *= 0.999f;
			if(sector[xa][ya].ebuildings != 0)	sector[xa][ya].ground_threat *= 0.985f;
			//if(sector[x][y].ebuildings!= 0)	sector[x][y].ebuildings *= 1+(sector[x][y].buildings.size()/10);
		}
	}
	// Run the code that handles stockpiled weapons.
	if(sweap.empty() == false){
		FireSilos();
	}
	// calculate new threads
	float3 pos = ZeroVector;
	int x, y = 0; 

	// get all enemies in los
	int* en = new int[10000];
	int unum = G->cb->GetEnemyUnits(en);

	// go through the list 
	if(unum > 1){
		for(int i = 0; i < 10000; i++){
			// if 0 end of array reached
			if(en[i] == 0){
				continue;
			}else{
				const UnitDef* def = G->cb->GetUnitDef(en[i]);
				if(def == 0) continue;
				pos = G->cb->GetUnitPos(en[i]);
				// if pos is one of the below then it will cause a divide by zero error and throw an exception, so we must prevent this.
				if(pos == ZeroVector) continue;
				if(pos == UpVector) continue;

				// calculate in which sector unit is located
				x = int(pos.x/xSize);
				y = int(pos.z/ySize);

				// check if gorund unit or ship
				if(def != 0){
					if(G->cb->UnitBeingBuilt(en[i]) == true){ // this unti is being built, which signifies the presence of a construction unit or factory, which means a base.
						sector[x][y].ebuildings += ((def->metalCost+def->energyCost+0.1)/2)+def->energyMake+((def->extractsMetal+0.1)*30)+def->buildSpeed;
					}else if(def->movedata){
						if(def->builder != 0 ){
							sector[x][y].ebuildings += ((def->metalCost+def->energyCost+0.1)/2)+def->energyMake+((def->extractsMetal+0.1)*30)+def->buildSpeed;
						}else{
							sector[x][y].ground_threat += def->power;
						}
					}else if(def->canfly == false){ // its a building
						sector[x][y].ebuildings += ((def->metalCost+def->energyCost+0.1)/2)+def->energyMake+((def->extractsMetal+0.1)*30)+def->buildSpeed;
					}
				}
				en[i] = 0;
			}
		}
	}
	delete en;

	// NTAI attack group handling code

	/*vector<ackforce>::iterator af;
	bool a = false;
	vector<ackforce>::iterator bf;
	bool b = false;
	if(groups.empty() == false){
		// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		// merge idle attack forces together.
		for(vector<ackforce>::iterator ad = groups.begin();ad != groups.end();++ad){
			if(ad->idle){
				if(a == false){
					af = ad;
					a = true;
					continue;
				} else if(b == false){
					bf = ad;
					b = true;
					for(set<int>::iterator acf = bf->units.begin();(acf != bf->units.end())&&(bf->units.empty() == false);++acf){
						af->units.insert(*acf);
						af->acknum++;
					}
					bf->units.clear();
					groups.erase(bf);
					FindTarget(&*af);
					a = false;
					b = false;
					break;
				}
			}
		}
	}*/
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// re-evaluate the target of an attack force
	if(G->cb->GetCurrentFrame()%240 == 0){
		if(groups.empty() == false){
			for(vector<ackforce>::iterator af = groups.begin();af != groups.end();++af){
				if(af->units.empty() == false) FindTarget(&*af);
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

