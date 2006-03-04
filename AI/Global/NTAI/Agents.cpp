#include "helper.h"

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

void Factor::UnitFinished(int unit){
	NLOG("Factor::UnitFinished");
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if (ud != 0){
		if (ud->extractsMetal>0){
			G->M->addExtractor(unit);
			float3 gpos = G->cb->GetUnitPos(unit);
			for(vector<float3>::iterator pmi = pmex.begin(); pmi != pmex.end(); ++pmi){
				if(*pmi == gpos){
					pmex.erase(pmi);
					break;
				}
			}
		}
	}
	if(ud == 0){
		exprint("error factor::unitfinished:unitdef failed to load");
		return;
	}
	//cfluke[unit] = 0;
	if(ud->builder == true){
		Tunit uu(G,unit);
		if(ud->movedata == 0){
			uu.repeater = true;
			uu.factory = true;
		}
		this->builders.push_back(uu);
		return;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Building things that need placing on a map.

bool Factor::CBuild(string name, int unit, int spacing){
	NLOG("Factor::CBuild");
	G->L.print("CBuild()" + name);
	if(G->cb->GetUnitDef(unit) == 0) return false;
	const UnitDef* ud = G->cb->GetUnitDef(name.c_str());
	if(G->cb->GetUnitHealth(unit)<1){
		G->L.print("unitfinished doesnt exist?! Could it have been destroyed?");		
		return false;
	}
	if(ud == 0){
		G->L.print("error: a problem occured loading this units unitdefinition : " + name);
		return false;
	} else {
		TCommand tc;
		tc.unit = unit;
		tc.clear = false;
		tc.Priority = tc_pseudoinstant;
		tc.c.id = -ud->id;
		float3 pos;
		float3 unitpos = G->cb->GetUnitPos(unit);
		if(start == false){
			if(ud->extractRange == 0){
                int wc = G->WhichCorner(unitpos);
				if(wc == t_NE) unitpos.z += 200;
				if(wc == t_NW) unitpos.z += 200;
				if(wc == t_SE)	unitpos.z -= 200;
				if(wc == t_SW) unitpos.z -= 200;
				start = true;
			}
		}
		if(ud->extractRange>0){
			pos = G->M->getNearestPatch(unitpos,0.8f,ud->extractsMetal,ud);
			if(pos == ZeroVector){
				G->L.print("zero mex co-ordinates intercepted");
				return false;
			} else if( pmex.empty() == false){
				for(vector<float3>::iterator pmi = pmex.begin(); pmi != pmex.end(); ++pmi){
					if(pmi->distance(pos) < max(ud->xsize*8,ud->ysize*8)){
						pos = G->M->getNearestPatch(*pmi,0.8f,ud->extractsMetal,ud);
						if(pos == ZeroVector){
							G->L.print("zero mex co-ordinates intercepted");
							return false;
						}
					}
				}
				pmex.push_back(pos);
			}
		} else {
			pos = G->cb->ClosestBuildSite(ud,unitpos,(1000+(G->cb->GetCurrentFrame()%640)),spacing);
			if(pos == ZeroVector){
				pos = G->cb->ClosestBuildSite(ud,unitpos,1200,1);
				G->L.print("zero vector returned");
			}else if(pos == UpVector){
				pos = G->cb->ClosestBuildSite(ud,unitpos,1200,1);
				G->L.print("Up vector returned");
			}
			if((pos == ZeroVector) || (pos == UpVector)){
				G->L.print("zero vector returned for " + name);
				return false;
			}
		}
        tc.c.params.push_back(pos.x);
		tc.c.params.push_back(G->cb->GetElevation(pos.x,pos.z));
		tc.c.params.push_back(pos.z);
		tc.c.timeOut = ud->buildTime/6 + G->cb->GetCurrentFrame();
		if(G->GiveOrder(tc)== -1){
			G->L.print("Error::Cbuild Failed Order :: " + name);
			return false;
		}else{
            return true;
		}
	}
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Used for building things through a factory.

bool Factor::FBuild(string name, int unit, int quantity){
	NLOG("Factor::FBuild");
	if(G->cb->GetUnitDef(unit) == 0) return false;
	if(G->cb->GetUnitHealth(unit)<1){
		G->L.print("unitfinished doesnt exist?! Could it have been destroyed? :: " + name);		
		return false;
	}
	const UnitDef* ud = G->cb->GetUnitDef(name.c_str());
	if(ud == 0){
		G->L.print("error: a problem occured loading this units unitdef : " + name);
		return false;
	} else {
		TCommand tc;
		tc.c.id = -ud->id;
		G->L.print("issuing mobile unit :: " + name);
		tc.c.params.push_back(0);
		tc.c.params.push_back(0);
		tc.c.params.push_back(0);
		tc.delay = 2 SECONDS;
		tc.c.timeOut = ud->buildTime*4 + G->cb->GetCurrentFrame() + tc.delay;
		tc.created = G->cb->GetCurrentFrame();
		tc.unit = unit;
		tc.clear = false;
		tc.Priority = tc_pseudoinstant;
		for(int q=1;q==quantity;q++){
			G->GiveOrder(tc);
		}
		return true;
	}
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::Update(){
	NLOG("Factor::Update");
	int Tframe = G->cb->GetCurrentFrame();
	if(builders.empty() == false){
		for(vector<Tunit>::iterator uv = builders.begin(); uv != builders.end();++uv){
			if(uv->tasks.empty() == false){
				for(list<Task>::iterator ti = uv->tasks.begin();(ti != uv->tasks.end())&&(uv->tasks.empty() == false);++ti){
					if(ti->valid == false){
						uv->tasks.erase(ti);
						break;
					}
				}
			}
			if(uv->tasks.empty() == false){
				for(list<Task>::iterator ti = uv->tasks.begin();(ti != uv->tasks.end())&&(uv->tasks.empty() == false);++ti){
					if(ti->valid == false){
						uv->tasks.erase(ti);
						break;
					}
				}
			}
		}
	}
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Check our commanders, and assign them.
	//{
	if(Tframe == 5){

		// Get commander ID
		int* uj = new int[500];
		int unum = G->cb->GetFriendlyUnits(uj);
		if(unum == 0) return;

		// Fill out a unit object
		Tunit uu(G,uj[0]);
		uu.creation = G->cb->GetCurrentFrame();
		uu.frame = 1;
		uu.guarding = false;
		if((uu.ud->movedata == 0)&&(uu.ud->canfly == false)){
			uu.factory = true;
		}else{
            uu.factory = false;
		}
		uu.waiting = false;
		G->unitallyteam = G->cb->GetUnitAllyTeam(uj[0]);
		this->UnitIdle(uu.uid);
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
		
		for(vector<Tunit>::iterator tu = builders.begin(); tu != builders.end();++tu){
			if (tu->bad == true){
				builders.erase(tu);
				break;
			}
		}
		// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		// Check Check for units without build lists and assign them where arppopriate
		for(vector<Tunit>::iterator ui = builders.begin();ui !=builders.end();++ui){
			if(ui->bad == true)	continue;
			if(G->cb->GetUnitDef(ui->uid) == 0){
				ui->bad = true;
				continue;
			}
			if(ui->waiting == true){
				if(ui->tasks.empty() == false){
					bool apax = G->Pl->feasable(ui->tasks.back().targ,ui->uid);
					if(apax == true){
						UnitIdle(ui->uid);
						ui->waiting = false;
						continue;
					}
				}else{
					ui->waiting = false;
				}
			}
			int frame = Tframe - ui->creation;
			if(G->cb->GetUnitHealth(ui->uid)<1){
				LOG << "unitfinished doesnt exist?! Could it have been destroyed? :: " << ui->ud->name;	
				continue;
			}

			// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
			/* Are we going to use solar generation or Wind generation?*/
			//{

			/*string ener = "";
			bool ew = true;
			if(energy == ener){
				if((G->cb->GetMaxWind()>18)&&(G->cb->GetMinWind()>14)){
					ew = false;
				}
			}*/
			if(((ui->factory == true)&&(ui->ud->isCommander == false)&&(ui->tasks.empty() == true)) || ((ui->ud->isCommander == true)&&(ui->tasks.empty() == true)&&(frame == 10)) || ((ui->ud->builder == true)&&(ui->tasks.empty() == true)&&(frame == 150)&&(ui->ud->isCommander == false)) ){
				string filename;
				string buffer;
				int ebsize= 0;
				ifstream fp;
				if(G->abstract == true){
					filename = "NTAI/Default/";
					if(ui->ud->isCommander == true){
						filename += "commander.txt";
					} else if(ui->factory == true){
						filename += "factory.txt";
					}else{
						filename += "builder.txt";
					}
					fp.open(filename.c_str(), ios::in);
					if(fp.is_open() == false){
						G->L.print("error :: " + filename + "could not be opened!!!" );
						continue;
					}else{
						G->L.print("using default abstract buildtree for :: " + ui->ud->humanName + " :: " + filename);
						buffer = "";
						char in_char;
						while(fp.get(in_char)){
							buffer += in_char;
							ebsize++;
						}
					}
				}else{
					filename = "NTAI/";
					filename += G->cb->GetModName();
					filename += "/";
					filename += ui->ud->name;
					filename += ".txt";
					fp.open(filename.c_str(), ios::in);
					if(fp.is_open() == false){
						filename = "NTAI/";
						filename += G->cb->GetModName();
						filename += "/";
						filename += G->cb->GetMapName();
						filename += "/";
						filename += ui->ud->name;
						filename += ".txt";
						fp.close();
						fp.open(filename.c_str(), ios::in);
						if(fp.is_open() == false){
							continue;
						}else{
							G->L.print("using map specific buildtree for :: " + ui->ud->humanName);
							buffer = "";
							char in_char;
							ebsize = 0;
							while(fp.get(in_char)){
								buffer += in_char;
								ebsize++;
							}
						}
					}else{
						char in_char;
						ebsize = 0;
						while(fp.get(in_char)){
							buffer += in_char;
							ebsize++;
						}
					}
				}
				if(fp.is_open() == false){
					continue;
				}
				if(ebsize > 0){
					vector<string> v;
					v = bds::set_cont(v,buffer.c_str());
					if(v.empty() == false){
						for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
							const UnitDef* uj = G->cb->GetUnitDef(vi->c_str());
							string sgh = "";
							if(uj != 0){
								ui->AddTask(*vi);
								if(v.size() == 1) ui->AddTask(*vi);
							} else if(*vi == string("repair")){
								ui->AddTask(true, true, true);
								if(v.size() == 1) ui->AddTask(true, true, true);
							} else if(*vi == string("is_factory")){
								ui->factory = true;
							} else if(*vi == string("is_notfactory")){
								ui->factory = false;
							} else if(*vi == string("repeat")){
								ui->repeater = true;
							} else if(*vi == string("dont_repeat")){
								ui->repeater = false;
							} else if(*vi == string("base_pos")){
								G->base_positions.push_back(G->cb->GetUnitPos(ui->uid));
							} else if(*vi == string("set5_attack_threshold")){
								G->Ch->threshold = 5;
							} else if(*vi == string("set10_attack_threshold")){
								G->Ch->threshold = 10;
							} else if(*vi == string("set20_attack_threshold")){
								G->Ch->threshold = 20;
							} else if(*vi == string("set30_attack_threshold")){
								G->Ch->threshold = 30;
							} else if(*vi == string("B_POWER")){
								sgh = ui->GetBuild(B_POWER,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_MEX")){
								sgh = ui->GetBuild(B_MEX,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_FACTORY_CHEAP")){
								sgh = ui->GetBuild(B_FACTORY_CHEAP,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_FACTORY")){
								sgh = ui->GetBuild(B_FACTORY,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_RAND_ASSAULT")){
								sgh = ui->GetBuild(B_RAND_ASSAULT,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_RANDOM")){
								sgh = ui->GetBuild(B_RANDOM,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_SCOUT")){
								sgh = ui->GetBuild(B_SCOUT,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_ASSAULT")){
								sgh = ui->GetBuild(B_ASSAULT,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_BUILDER")){
								sgh = ui->GetBuild(B_BUILDER,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_DEFENCE")){
								sgh = ui->GetBuild(B_DEFENCE,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else if(*vi == string("B_GEO")){
								sgh = ui->GetBuild(B_GEO,false);
								if(sgh != string("nought")){
									ui->AddTask(sgh);
								}
							} else{
								G->L.iprint("error :: a value :: " + *vi +" :: was parsed in :: "+filename + " :: this does not have a valid UnitDef according to the engine, and is not a Task keyword such as repair");;
							}
						}
					} else{
						G->L.print(" error sorting contents of file :: " + filename + " :: error is most likely because of impropoer formatting as something was loaded into the buffer, yet when the buffer was parsed nothing was present");
					}
					if(ui->ud->isCommander == true){
						G->basepos = G->cb->GetUnitPos(ui->uid);
					}
					if((ui->ud->isCommander== true)||(ui->factory == true)){
						G->base_positions.push_back(G->cb->GetUnitPos(ui->uid));
					}
					UnitIdle(ui->uid);
				} else{
					G->L.print(" error loading contents of  file :: " + filename + " :: buffer empty, most likely because of an empty file");
				}
				fp.close();
				continue;
			}
			//}
			// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
			// Get the unit commands and check if they're valid. This is to prevent untis from stalling and doing nothing by looking for idle untis that ahve tasks.
			//{
			/*if((frame >20)&&(Tframe>300)&&(ui->tasks.empty() == false)&&(ui->waiting == false)&&(ui->guarding == false)&&(ui->scavenge == false)){
				const deque<Command>* dc = G->cb->GetCurrentUnitCommands(ui->uid);
				
				bool stopped = false;
				if(dc == 0){
					stopped = true;
				}else{
					if((dc->empty() == false)){
						for(deque<Command>::const_iterator di = dc->begin();di != dc->end();++di){
							if(di->id == CMD_STOP){
								stopped = true;
								break;
							}
						}
					}else{
						stopped = true;
					}
				}
				if((stopped == true)&&(ui->waiting == false)&&(ui->tasks.empty() == false)&&(ui->guarding == false)&&(ui->scavenge == false)){
					cfluke[ui->uid] += 1;
					if(cfluke[ui->uid] > 10 SECONDS){
						cfluke[ui->uid] = 0;
						G->Fa->UnitIdle(ui->uid);
					}
				}else{
					cfluke[ui->uid] = 0;
				}
			}*/
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitDamaged(int damaged, int attacker, float damage, float3 dir){
	NLOG("Factor::UnitDamaged");
	const UnitDef* ud = G->cb->GetUnitDef(damaged);
	const UnitDef* adg;
	if(attacker <0 ){
		adg = 0;
	}else{
		adg = G->cb->GetUnitDef(attacker);
	}
	if(adg !=0){
		if(adg->isCommander == true){
			if(ud->isCommander == true){
				return;
			}
		}
	}
	if(ud == 0) return;
	if( attacker <0) return;
	if(G->cb->GetUnitAllyTeam(attacker) == G->unitallyteam) return;
	if(builders.empty() == false){
		for(vector<Tunit>::iterator uv = builders.begin(); uv != builders.end();++uv){
			if(uv->uid == damaged){
				if((G->cb->GetUnitHealth(uv->uid)<1)||(uv->reclaiming == true)){	
					break;
				}else{
					if(ud->weapons.empty() == false){ // D-gun will end up being fired so we need a repair task so the unit resumes building once fired
						for(vector<UnitDef::UnitDefWeapon>::const_iterator wu = ud->weapons.begin();wu !=ud->weapons.end();++wu){
							if(wu->def->manualfire == true){
								uv->reclaiming = true;
								uv->tasks.push_back(Task(G,true,true));
								break;
							}
						}
					}else{
						/*TCommand tc;
						tc.c.id = CMD_RECLAIM;
						tc.c.params.push_back(attacker);
						tc.c.timeOut = 400 + G->cb->GetCurrentFrame();
						tc.unit = uv->uid;
						tc.clear = false;
						tc.Priority = tc_instant;
						G->GiveOrder(tc);
						uv->tasks.push_back(Task(G,true,true));*/
						return;
					}
				}
			}
		}
	}
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitIdle(int unit){
	NLOG("Factor::UnitIdle");
	if(G->cb->GetUnitHealth(unit)<1){
		G->L.print("unitfinished doesnt exist?! Could it have been destroyed?");
		return;
	}
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0){
		exprint("UnitDefloaderror factor::UnitIdle");
		return;
	}
	if(builders.empty() == false){
		for(vector<Tunit>::iterator uv = builders.begin(); uv != builders.end();++uv){
			uv->frame++;
			if(G->cb->GetUnitHealth(uv->uid)<1){
				G->L.print("unitidle doesnt exist?! Could it have been destroyed?");		
				continue;
			}
			if(uv->tasks.empty() == true){
				if(uv->uid == unit){
					uv->reclaiming = false;
					if(uv->repeater == true){
						uv->creation = G->cb->GetCurrentFrame();
						continue;
					}
					if((uv->guarding == false)&&(uv->scavenge == false)&&(uv->tasks.empty() != false)&&(uv->frame >200)){
						if(uv->ud->canResurrect == false){
							for(vector<Tunit>::iterator vu = builders.begin(); vu != builders.end(); ++vu){
								if(vu->tasks.empty() == false){
									if(G->cb->GetUnitHealth(vu->uid)<0.1f){
										G->L.print("unitfinished doesnt exist?! Could it have been destroyed?");
										return;
									}
									TCommand tc;
									tc.clear = false;
									tc.Priority = tc_pseudoinstant;
									tc.unit = uv->uid;
									tc.c.id = CMD_GUARD;
									tc.c.params.push_back(vu->uid);
									G->GiveOrder(tc);
									uv->guarding = true;
									break;
								}
							}
						}else{
							TCommand tc;
							tc.c.id = CMD_RESURRECT;
							tc.c.params.push_back(G->nbasepos(G->cb->GetUnitPos(uv->uid)).x);
							tc.c.params.push_back(G->nbasepos(G->cb->GetUnitPos(uv->uid)).y);
							tc.c.params.push_back(G->nbasepos(G->cb->GetUnitPos(uv->uid)).z);
							tc.c.params.push_back(max(G->cb->GetMapWidth(),G->cb->GetMapHeight()));
							tc.unit = uv->uid;
							tc.clear = false;
							tc.Priority = tc_assignment;
							G->GiveOrder(tc);
							uv->scavenge = true;
						}
					}
				}
			} else if(uv->uid == unit){
				/*bool dgunned = false;
				if(uv->ud->weapons.empty() == false){
					for(vector<UnitDef::UnitDefWeapon>::const_iterator wea = uv->ud->weapons.begin(); wea != uv->ud->weapons.end(); ++wea){
						if(wea->def->manualfire == true){ // WE HAVE A D-GUNNER!!!!!!
							if(wea->def->energycost < G->cb->GetEnergy()){
                                int* ten = new int[5000];
								int f = G->GetEnemyUnits(ten,G->cb->GetUnitPos(unit),wea->def->range*2);
								if(f>0){
									TCommand tc;
									tc.unit = unit;
									tc.clear = false;
									tc.Priority = tc_instant;
									tc.c.id = CMD_DGUN;
									int hq = 0;
									for(int fi = 0; fi < f; fi++){
										const UnitDef* fig = G->cb->GetUnitDef(ten[fi]);
										if(fig != 0){
											if(fig->isCommander == true){
												continue;
											}else{
												hq = fi;
												break;
											}
										}
									}
									tc.c.params.push_back(hq);
									tc.c.timeOut = 7 SECONDS + G->cb->GetCurrentFrame();
									tc.created = G->cb->GetCurrentFrame();
									G->GiveOrder(tc);
									dgunned = true;
								}
								delete [] ten;
							}
							break;
						}
					}
				}*/
				//if(/*dgunned !=*/ true){
					for(list<Task>::iterator ti = uv->tasks.begin();(ti != uv->tasks.end())&&(uv->tasks.empty() == false);++ti){
						if(ti->valid == false){
							continue;
						}else{
							/*if(G->Pl->feasable(ti->targ.c_str(),uv->uid) == false){
								uv->waiting = true;
								break;
							}*/
							bool a = ti->execute(uv->uid);
							if(a == false){
								ti->valid = false;
								continue;
							}
							uv->tasks.erase(ti);
							break;
						}
					}
					if(uv->tasks.empty() == true){
						G->L << "info ::  Unit has finished its build tree :: " << uv->ud->humanName << endline;
					}
				///}
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitDestroyed(int unit){
	NLOG("Factor::UnitDestroyed");
	G->M->removeExtractor(unit);
	if(builders.empty() == false){
		for(vector<Tunit>::iterator vu = builders.begin(); vu != builders.end();++vu){
				if(vu->uid == unit){
					builders.erase(vu);
					break;
				}
		}
		/*if(cfluke.empty() == false){
			for(map<int,int>::iterator mi = cfluke.begin(); mi != cfluke.end(); ++mi){
				if(mi->first == unit){
					cfluke.erase(mi);
					break;
				}
			}
		}*/
	}
	float3 gpos = G->cb->GetUnitPos(unit);
	for(vector<float3>::iterator pmi = pmex.begin(); pmi != pmex.end(); ++pmi){
		if(*pmi == gpos){
			pmex.erase(pmi);
			break;
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

