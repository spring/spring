#include "../helper.h"




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
	string contents = G->Get_mod_tdf()->SGetValueMSG("AI\\metatags");
	if(contents.empty() == false){
		vector<string> v = bds::set_cont(v,contents.c_str());
		if(v.empty() == false){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				string sdg = "TAGS\\";
				sdg += *vi;
				vector<string> vh = bds::set_cont(vh,G->Get_mod_tdf()->SGetValueMSG(sdg.c_str()));
				metatags[*vi]= vh;
			}
		}
	}
	G->Get_mod_tdf()->GetDef(rand_tag, "1", "AI\\metatag_efficiency");
	//upsig = G->update_signal.connect(boost::bind(&Factor::Update, this)); // remnants of an experiment with the boost::signals library
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitFinished(int unit){
	NLOG("Factor::UnitFinished");
	if(unit < 1) return;
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if (ud != 0){
		if (ud->extractsMetal>0){
			G->M->addExtractor(unit);
			float3 gpos = G->cb->GetUnitPos(unit);
			G->base_positions.push_back(gpos);
			for(vector<float3>::iterator pmi = pmex.begin(); pmi != pmex.end(); ++pmi){
				if(*pmi == gpos){
					pmex.erase(pmi);
					break;
				}
			}
		}
	}else {
		exprint("error factor::unit finished:unit def failed to load");
		return;
	}
	//cfluke[unit] = 0;
	if(ud->builder == true){
		Tunit uu(G,unit);
		//if(ud->builder == true){
			if(ud->movedata == 0){
				uu.repeater = true;
				if((ud->canfly == false)&&(ud->buildOptions.empty() == false)){
					uu.factory = true;
					uu.builder = false;
				}else{
					uu.builder = true;
					uu.factory = true;
				}
			}else{
				uu.builder = true;
			}
		//}
		if(ud->isCommander == true) uu.iscommander = true;
		// wartesting.sdz fix
		if(ud->humanName == string(_T("GD Command Vehicle"))) uu.iscommander = true;
		if(ud->humanName == string(_T("URC Mobile Command"))) uu.iscommander = true;

		G->units.push_back(uu);
		return;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Building things that need placing on a map.

bool Factor::CBuild(string name, int unit, int spacing){
	NLOG(_T("Factor::CBuild"));
	G->L.print(_T("CBuild() :: ") + name);
	if(G->cb->GetUnitDef(unit) == 0) return false;
	const UnitDef* ud = G->cb->GetUnitDef(name.c_str());
	const UnitDef* uda = G->cb->GetUnitDef(unit);
	if(G->cb->GetUnitHealth(unit)<1){
		G->L.print(_T("unit finished does not exist?! Could it have been destroyed?"));		
		return false;
	}
	if(ud == 0){
		G->L.print(_T("error: a problem occurred loading this units unit definition : ") + name);
		return false;
	} else if(uda == 0){
		return false;
	}else {
		TCommand TCS(tc,_T("CBuild"));
		tc.unit = unit;
		tc.clear = false;
		tc.Priority = tc_pseudoinstant;
		tc.c.id = -ud->id;
		float3 pos;
		float3 unitpos = G->cb->GetUnitPos(unit);
		if(ud->extractRange == 0){
			// push the unitpos out so that the commander does not try to build on top of itself leading to stall (especially important for something like nanoblobz
			float3 epos = unitpos;
			epos.z -= max(uda->ysize*14,uda->xsize*14);
			epos.z -= uda->buildDistance*0.6f;
			srand(time(NULL)+G->randadd+G->team);
			G->randadd++;
			float angle = rand()%320;
			unitpos = G->Rotate(epos,angle,unitpos);
			// move the new position if it's off map
			if(unitpos.x > G->cb->GetMapWidth() * SQUARE_SIZE){
				unitpos.x = (G->cb->GetMapWidth() * SQUARE_SIZE)-(unitpos.x-(G->cb->GetMapWidth() * SQUARE_SIZE));
				unitpos.x -= 500;
			}
			if(unitpos.z > G->cb->GetMapHeight() * SQUARE_SIZE){
				unitpos.z = (G->cb->GetMapHeight() * SQUARE_SIZE)-(unitpos.z-(G->cb->GetMapHeight() * SQUARE_SIZE));
				unitpos.z -= 500;
			}
			if(unitpos.x < 0){
				unitpos.x *= (-1);
				unitpos.x += 500;
			}
			if(unitpos.z <0){
				unitpos.z *= (-1);
				unitpos.z += 500;
			}
			int wc = G->WhichCorner(unitpos);
			if(wc == t_NE){
				unitpos.z += 40;
				unitpos.x -= 40;
			}
			if(wc == t_NW){
				unitpos.z += 40;
				unitpos.x += 40;
			}
			if(wc == t_SE){
				unitpos.z -= 40;
				unitpos.x -= 40;
			}
			if(wc == t_SW){
				unitpos.z -= 40;
				unitpos.x += 40;
			}
		}

		if(ud->extractRange>0){
			pos = G->M->getNearestPatch(unitpos,0.8f,ud->extractsMetal,ud);
			if((pos == ZeroVector)||(pos == UpVector)){
				G->L.print(_T("zero mex co-ordinates intercepted"));
				return false;
			} else if( pmex.empty() == false){
				for(vector<float3>::iterator pmi = pmex.begin(); pmi != pmex.end(); ++pmi){
					if(pmi->distance(pos) < max(ud->xsize*8,ud->ysize*8)){
						pos = G->M->getNearestPatch(*pmi,0.8f,ud->extractsMetal,ud);
						if((pos == ZeroVector)||(pos == UpVector)){
							G->L.print(_T("zero mex co-ordinates intercepted"));
							return false;
						}
					}
				}
				pmex.push_back(pos);
			}
		} else {
			pos = G->cb->ClosestBuildSite(ud,unitpos,(1000+(G->cb->GetCurrentFrame()%640)),spacing);
			if(pos == ZeroVector){
				pos = G->cb->ClosestBuildSite(ud,unitpos,2000,spacing);
				G->L.print(_T("zero vector returned"));
			}else if(pos == UpVector){
				pos = G->cb->ClosestBuildSite(ud,unitpos,2000,spacing);
				G->L.print(_T("Up vector returned"));
			}
			if((pos == ZeroVector) || (pos == UpVector)){
				G->L.print(_T("zero/up vector returned for ") + name);
				return false;
			}
		}
		pos.y = G->cb->GetElevation(pos.x,pos.z);
		if(G->cb->CanBuildAt(ud,pos) == false) return false;
		tc.c.params.push_back(pos.x);
		tc.c.params.push_back(pos.y);
		tc.c.params.push_back(pos.z);
		tc.c.timeOut = ud->buildTime/6 + G->cb->GetCurrentFrame();
		tc.created = G->cb->GetCurrentFrame();
		tc.delay=0;
		if(G->GiveOrder(tc)== -1){
			G->L.print(_T("Error::Cbuild Failed Order :: ") + name);
			//plans[unit] = pos;
			return false;
		}else{
			return true;
		}
	}
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Factor::Factor(){
	race =0;
	water = false;
	G=0;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Used for building things through a factory.

bool Factor::FBuild(string name, int unit, int quantity){
	NLOG(_T("Factor::FBuild"));
	if(G->cb->GetUnitDef(unit) == 0) return false;
	if(G->cb->GetUnitHealth(unit)<1){
		G->L.print(_T("unit finished doesnt exist?! Could it have been destroyed? :: ") + name);		
		return false;
	}
	const UnitDef* ud = G->cb->GetUnitDef(name.c_str());
	if(ud == 0){
		G->L.print(_T("error: a problem occurred loading this units unit def : ") + name);
		return false;
	} else {
		TCommand TCS(tc,_T("FBuild"));
		tc.c.id = -ud->id;
		G->L.print(_T("issuing mobile unit :: ") + name);
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
	NLOG(_T("Factor::Update"));
	int Tframe = G->cb->GetCurrentFrame();
	if(EVERY_( 10 FRAMES)){
		if(G->units.empty() == false){
			for(vector<Tunit>::iterator uv = G->units.begin(); uv != G->units.end();++uv){
				if((uv->builder != true)&&(uv->factory != true)) continue; // this isnt a builder/factory etc so we need to skip it!
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
	}
	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Check our commanders, and assign them.
	//{
	if(Tframe == 5){

		// Get commander ID
		int* uj = new int[1000];
		int unum = G->cb->GetFriendlyUnits(uj);
		if(unum == 0) return;

		// Fill out a unit object
		Tunit uu(G,uj[0]);
		uu.creation = G->cb->GetCurrentFrame();
		uu.frame = 1;
		uu.guarding = false;
		uu.iscommander = true;
		if((uu.ud->movedata == 0)&&(uu.ud->canfly == false)){
			uu.factory = true;
			uu.builder = false;
		}else{
            uu.factory = false;
			uu.builder = true;
		}
		uu.waiting = false;
		G->unitallyteam = G->cb->GetUnitAllyTeam(uj[0]);
		TCommand TCS(tc,_T("firestate factor::update"));
		tc.c.id = CMD_FIRE_STATE;
		tc.Priority = tc_instant;
		tc.created = G->cb->GetCurrentFrame();
		tc.c.params.push_back(G->fire_state_commanders);
		tc.unit = uu.uid;
		G->GiveOrder(tc);
#ifdef TC_SOURCE
		tc.source = _T("movestate factor::update");
#endif
		tc.c.id = CMD_MOVE_STATE;
		tc.c.params.erase(tc.c.params.begin(),tc.c.params.end());
		tc.c.params.clear();
		tc.c.params.push_back(G->move_state_commanders);
		G->GiveOrder(tc,false);
		this->UnitIdle(uu.uid);
		delete [] uj;
	}
	//}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// Cycle through builders, here we will assign construction Tasks
	// to newly built units, or factories that have finished their tasks.
	// We will also check if the unit has stalled and give it a shove in
	// the right direction if it has.
	//{
	if(EVERY_(8 FRAMES)){
		if((G->units.empty() == false)&&(Tframe > 8)){
			// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
			// Check our builders for null pointers.
			
			for(vector<Tunit>::iterator tu = G->units.begin(); tu != G->units.end();++tu){
				if (tu->bad == true){
					G->units.erase(tu);
					break;
				}
			}
			// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
			// Check Check for units without build lists and assign them where appropriate
			for(vector<Tunit>::iterator ui = G->units.begin();ui != G->units.end();++ui){
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
					LOG << _T("unit finished doesnt exist?! Could it have been destroyed? :: ") << ui->ud->name;	
					continue;
				}
				//TODO: change back then optimize
				if(((ui->factory == true)&&(ui->ud->isCommander == false)&&(ui->tasks.empty() == true)&&(frame > 1 SECOND)&&(ui->bad == false)) || ((ui->ud->isCommander == true)&&(ui->tasks.empty() == true)&&(frame > 10)&&(ui->bad == false)) || ((ui->ud->builder == true)&&(ui->tasks.empty() == true)&&(frame > 150)&&(ui->ud->isCommander == false)&&(ui->bad == false))){
					string filename;
					string* buffer= new string(_T(""));
					/* if abstract = true and we're not to use mod specific buidler.txt etc then load from the default folder
					*/if((G->abstract == true)&&(G->use_modabstracts == false)){ 
						filename = _T("aidll/globalai/NTAI") + string(slash) + _T("Default") + string(slash);
						if(ui->ud->isCommander == true){
							filename += _T("commander.txt");
						}else if(ui->iscommander == true){
							filename += _T("commander.txt");
						}else if(ui->factory == true){
							filename += _T("factory.txt");
						}else{
							filename += _T("builder.txt");
						}
						if(G->ReadFile(filename,buffer) == false){
							G->L.print(_T("error :: ") + filename + _T(" could not be opened!!! (1)") );
							continue;
						}
					}/*
						Otherwise is mod_abstracts = true then load from ntai\modname\builder.txt etc
						*/else if(G->use_modabstracts == true){
						filename = _T("aidll/globalai/NTAI") + string(slash) + G->tdfpath + string(slash);
						if(ui->ud->isCommander == true){
							filename += _T("commander.txt");
						}else if(ui->iscommander == true){
							filename += _T("commander.txt");
						}else if(ui->factory == true){
							filename += _T("factory.txt");
						}else{
							filename += _T("builder.txt");
						}
						if(G->ReadFile(filename,buffer) == false){
							G->L.print(_T("error :: ") + filename + _T(" could not be opened!!! (2)") );
							continue;
						}
					}/*
						load map specific
						*/else {
						filename = _T("aidll/globalai/NTAI") + string(slash) + G->tdfpath + string(slash) + G->cb->GetMapName() + string(slash) + G->lowercase(ui->ud->name) + string(_T(".txt"));
						// map specific doesnt exist, load mod specific instead
						if(G->ReadFile(filename,buffer) == false){
							filename = _T("aidll/globalai/NTAI") + string(slash) + G->tdfpath + string(slash) + G->lowercase(ui->ud->name) + string(_T(".txt"));
							if(G->ReadFile(filename,buffer) == false){
								// absent_abstract is true so we'll load the builder.txt etc now that the definite data cannot be loaded
								if(G->absent_abstract == true){
									filename = string(_T("aidll/globalai/NTAI")) + string(slash) + G->tdfpath + slash;
									if(ui->ud->isCommander == true){
										filename += _T("commander.txt");
									}else if(ui->iscommander == true){
										filename += _T("commander.txt");
									}else if(ui->factory == true){
										filename += _T("factory.txt");
									}else{
										filename += _T("builder.txt");
									}
									if(G->ReadFile(filename,buffer) == false){
										G->L.print(_T("error :: ") + filename + _T(" could not be opened!!! (3)") );
										continue;
									}
								}else{
									continue;
								}
							}
						}
					}
					vector<string> v;
					string s = *buffer;
					if(s.empty() == true){
						G->L.print(_T(" error loading contents of  file :: ") + filename + _T(" :: buffer empty, most likely because of an empty file"));
						continue;
					}
					v = bds::set_cont(v,s.c_str());
					if(v.empty() == false){
						for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
							const UnitDef* uj = G->cb->GetUnitDef(vi->c_str());
							if(uj != 0){
								ui->AddTask(*vi);
								if(v.size() == 1) ui->AddTask(*vi);
							} else if (metatags.find(*vi) != metatags.end()){
								ui->AddTask(ui->GetBuild(metatags[*vi],rand_tag));
							}else if(*vi == string(_T("repair"))){
								ui->AddTask(B_REPAIR);
								if(v.size() == 1) ui->AddTask(B_REPAIR);
							} else if(*vi == string(_T("is_factory"))){
								ui->factory = true;
							} else if(*vi == string(_T("is_notfactory"))){
								ui->factory = false;
							} else if(*vi == string(_T("repeat"))){
								ui->repeater = true;
							} else if(*vi == string(_T("dont_repeat"))){
								ui->repeater = false;
							} else if(*vi == string(_T("base_pos"))){
								G->base_positions.push_back(G->cb->GetUnitPos(ui->uid));
							} else if(*vi == string(_T("gaia"))){
								G->gaia = true;
							} else if(*vi == string(_T("not_gaia"))){
								G->gaia = false;
							} else if(*vi == string(_T("switch_gaia"))){
								G->gaia = !G->gaia;
							} else if(*vi == string(_T("set5_attack_threshold"))){
								G->Ch->threshold = 5;
							} else if(*vi == string(_T("set10_attack_threshold"))){
								G->Ch->threshold = 10;
							} else if(*vi == string(_T("set20_attack_threshold"))){
								G->Ch->threshold = 20;
							} else if(*vi == string(_T("set30_attack_threshold"))){
								G->Ch->threshold = 30;
							} else if(*vi == string(_T("B_POWER"))){
								ui->AddTask(B_POWER);
							} else if(*vi == string(_T("B_MEX"))){
								ui->AddTask(B_MEX);
							} else if(*vi == string(_T("B_FACTORY_CHEAP"))){
								ui->AddTask(B_FACTORY_CHEAP);
							} else if(*vi == string(_T("B_FACTORY"))){
								ui->AddTask(B_FACTORY);
							} else if(*vi == string(_T("B_RAND_ASSAULT"))){
								ui->AddTask(B_RAND_ASSAULT);
							} else if(*vi == string(_T("B_RANDOM"))){
								ui->AddTask(B_RANDOM);
							} else if(*vi == string(_T("B_SCOUT"))){
								ui->AddTask(B_SCOUT);
							} else if(*vi == string(_T("B_ASSAULT"))){
								ui->AddTask(B_ASSAULT);
							} else if(*vi == string(_T("B_BUILDER"))){
								ui->AddTask(B_BUILDER);
							} else if(*vi == string(_T("B_DEFENCE"))){
								ui->AddTask(B_DEFENCE);
							} else if(*vi == string(_T("B_DEFENSE"))){
								ui->AddTask(B_DEFENCE);
							} else if(*vi == string(_T("B_METAL_MAKER"))){
								ui->AddTask(B_METAL_MAKER);
							}else if(*vi == string(_T("B_GEO"))){
								ui->AddTask(B_GEO);
							} else if(*vi == string(_T("B_RADAR"))){
								ui->AddTask(B_RADAR);
							} else if(*vi == string(_T("B_FORTIFICATION"))){
								ui->AddTask(B_FORTIFICATION);
							} else if(*vi == string(_T("B_MINE"))){
								ui->AddTask(B_MINE);
							} else{
								G->L.iprint(_T("error :: a value :: ") + *vi +_T(" :: was parsed in :: ")+filename + _T(" :: this does not have a valid UnitDef according to the engine, and is not a Task keyword such as repair"));;
							}
						}
						if(ui->ud->isCommander == true)	G->basepos = G->cb->GetUnitPos(ui->uid);
						if((ui->ud->isCommander== true)||(ui->factory == true))	G->base_positions.push_back(G->cb->GetUnitPos(ui->uid));
						UnitIdle(ui->uid);
					} else{
						G->L.print(_T(" error loading contents of  file :: ") + filename + _T(" :: buffer empty, most likely because of an empty file"));
					}
					continue;
				}
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitDamaged(int damaged, int attacker, float damage, float3 dir){
	NLOG(_T("Factor::UnitDamaged"));
	NO_GAIA
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
	if(G->units.empty() == false){
		for(vector<Tunit>::iterator uv = G->units.begin(); uv != G->units.end();++uv){
			if(uv->uid == damaged){
				if((G->cb->GetUnitHealth(uv->uid)<1)||(uv->reclaiming == true)){	
					break;
				}else{
					if(uv->can_dgun == cd_yes){ // D-gun will end up being fired so we need a repair task so the unit resumes building once fired
						uv->reclaiming = true;
						uv->tasks.push_front(Task(G,B_REPAIR, &*uv));
						break;
					}else{
						return;
					}
				}
			}
		}
	}
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Factor::UnitIdle(int unit){
	NLOG(_T("Factor::UnitIdle"));
	if(G->cb->GetUnitHealth(unit)<1){
		G->L.print(_T("unit finished doesnt exist?! Could it have been destroyed?"));
		return;
	}
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	if(ud == 0){
		exprint(_T("UnitDefloaderror factor::UnitIdle"));
		return;
	}
	if(G->units.empty() == false){
		for(vector<Tunit>::iterator uv = G->units.begin(); uv != G->units.end();++uv){
			uv->frame++;
			if(G->cb->GetUnitHealth(uv->uid)<1){
				G->L.print(_T("unitidle doesnt exist?! Could it have been destroyed?"));		
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
							for(vector<Tunit>::iterator vu = G->units.begin(); vu != G->units.end(); ++vu){
								if(vu->tasks.empty() == false){
									if(G->cb->GetUnitHealth(vu->uid)<0.1f){
										G->L.print(_T("unit finished doesnt exist?! Could it have been destroyed?"));
										return;
									}
									TCommand TCS(tc,_T("CMD_GUARD factor::unitidle"));
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
							TCommand TCS(tc,_T("cmd_resurrect factor::unitidle"));
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
						G->L << _T("info ::  Unit has finished its build tree :: ") << uv->ud->humanName << endline;
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
	if(G->units.empty() == false){
		for(vector<Tunit>::iterator vu = G->units.begin(); vu != G->units.end();++vu){
				if(vu->uid == unit){
					G->units.erase(vu);
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
	if(pmex.empty() == false){
		float3 gpos = G->cb->GetUnitPos(unit);
		for(vector<float3>::iterator pmi = pmex.begin(); pmi != pmex.end(); ++pmi){
			if(*pmi == gpos){
				pmex.erase(pmi);
				break;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

