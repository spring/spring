#include "../Core/helper.h"



// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Chaser::Chaser(){
	G = 0;
	lock = false;
}

void Chaser::InitAI(Global* GLI){
	G = GLI;
	NLOG("Chaser::InitAI");
	enemynum=0;
	threshold = atoi(G->Get_mod_tdf()->SGetValueDef("4", "AI\\initial_threat_value").c_str());
	string contents = G->Get_mod_tdf()->SGetValueMSG("AI\\kamikaze");
	if(contents.empty() == false){
		vector<string> v = bds::set_cont(v,contents.c_str());
		if(v.empty() == false){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				*vi = G->lowercase(*vi);
				sd_proxim[*vi] = true;
			}
		}
	}

	// calculate number of sectors
	xSectors = uint(floor(0.5f + ( G->cb->GetMapWidth()*8)/300.0f));
	ySectors = uint(floor(0.5f + ( G->cb->GetMapHeight()*8)/300.0f));

	// calculate effective sector size
	xSize = 300;
	ySize = 300;
	//Targetting = new CPotential(G,float3(float(xSectors),0,float(ySectors)),3.0f,100.0f,30000);
	
	//if(sec_set == false){
	// create field of sectors
	float3 ik(150.0f,0, float(ySize)/2.0f );
	int id = 0;
	for(int i = 0; i < xSectors; i++){
		for(int j = 0; j < ySectors; j++){
			// set coordinates of the sectors
			sector[i][j].location = float3(float(i),0,float(j));
			sector[i][j].centre = float3(float(i*xSize),0,float(j*ySize));
			sector[i][j].centre += ik;
			sector[i][j].index = id;
			id++;
		}
	}
	threat_matrix = new float[id+1];
	max_index = id;
	for(int i = 0; i < max_index; i++){
		threat_matrix[i] = 1.0f;
	}
	fire_at_will = bds::set_cont(fire_at_will,G->Get_mod_tdf()->SGetValueMSG("AI\\fire_at_will"));
	return_fire = bds::set_cont(return_fire,G->Get_mod_tdf()->SGetValueMSG("AI\\return_fire"));
	hold_fire = bds::set_cont(hold_fire,G->Get_mod_tdf()->SGetValueMSG("AI\\hold_fire"));
	roam = bds::set_cont(roam,G->Get_mod_tdf()->SGetValueMSG("AI\\roam"));
	maneouvre = bds::set_cont(maneouvre,G->Get_mod_tdf()->SGetValueMSG("AI\\maneouvre"));
	hold_pos = bds::set_cont(hold_pos,G->Get_mod_tdf()->SGetValueMSG("AI\\hold_pos"));
	if(G->Sc->start_pos.empty() == false){
		for(list<float3>::iterator i = G->Sc->start_pos.begin(); i != G->Sc->start_pos.end(); ++i){
			float3 tpos = *i;
			if(G->Map->CheckFloat3(tpos)==false){
				continue;
			}
			float3 spos = G->Map->WhichSector(tpos);
			if( spos != UpVector){
				int ind = sector[(int)spos.x][(int)spos.y].index;
				if(ind > max_index) continue;
				threat_matrix[ind] += 60000.0f;
			}
			G->Actions->AddPoint(spos);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::Add(int unit, bool aircraft){
	NO_GAIA(NA)
	G->L.print("Chaser::Add()");
	Attackers.insert(unit);
	attack_units.insert(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDestroyed(int unit, int attacker){
	NLOG("Chaser::UnitDestroyed");
	NO_GAIA(NA)
	dgunning.erase(unit);
	defences.erase(unit);
	dgunners.erase(unit);
	engaged.erase(unit);
	walking.erase(unit);
	attack_units.erase(unit);
	sweap.erase(unit);
	kamikaze_units.erase(unit);
	Attackers.erase(unit);
	if(groups.empty() == false){
		for(vector<ackforce>::iterator at = groups.begin();at !=groups.end();++at){
			set<int>::iterator i = at->units.find(unit);
			if(i != at->units.end()){
				at->units.erase(i);
				break;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	NLOG("Chaser::UnitDamaged");
	NO_GAIA(NA)
	EXCEPTION_HANDLER({
		float3 dpos = G->GetUnitPos(damaged);
		if(G->Map->CheckFloat3(dpos) == false) return;
		float mhealth = G->cb->GetUnitMaxHealth(damaged);
		float chealth = G->cb->GetUnitHealth(damaged);
		if((mhealth != 0)&&(chealth != 0)){
			if((chealth < mhealth*0.35f)||(damage > mhealth*0.5f)){
				// ahk ahk unit is low on health run away! run away!
				// Or unit is being attacked by a weapon that's going to blow it up very quickly
				// Units will want to stay alive!!!
				float3 bpos =G->Map->nbasepos(dpos);
				if(bpos.distance2D(dpos) > 600){
					dpos = G->Map->distfrom(dpos,bpos,400);
					G->Actions->Move(damaged,dpos);
				}
			}
		}
	},"Chaser::UnitDamaged running away routine!!!!!!",NA)
	//EXCEPTION_HANDLER(G->Actions->DGunNearby(damaged),"G->Actions->DGunNearby INSIDE Chaser::UnitDamaged",NA) // make the unit attack nearby enemies with the dgun, the attacker being the one most likely to be the nearest....
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitFinished(int unit){
	NLOG("Chaser::UnitFinished");
	NO_GAIA(NA)
	const UnitDef* ud = G->GetUnitDef(unit);
	if(ud == 0){
		G->L.print("UnitDef call filled, was this unit blown up as soon as it was created?");
		return;
	}
	NLOG("stockpile/dgun");
	if(G->CanDGun(unit)==true){
		dgunners.insert(unit);
	}
	if(ud->weapons.empty() == false){
		for(vector<UnitDef::UnitDefWeapon>::const_iterator i = ud->weapons.begin(); i!= ud->weapons.end(); ++i){
			if(i->def->stockpile == true){
				TCommand TCS(tc,"chaser::unitfinished stockpile");
				tc.unit = unit;
				tc.clear = false;
				tc.Priority = tc_low;
				tc.c.timeOut = G->cb->GetCurrentFrame() + (20 MINUTES);
				tc.c.id = CMD_STOCKPILE;
				for(int i = 0;i<110;i++){
					G->OrderRouter->GiveOrder(tc);
				}
				sweap.insert(unit);
			}
		}
	}
	if((ud->movedata == 0) && (ud->canfly == false) && (ud->weapons.empty() == false)){
		defences.insert(unit);
		return;
	}
	if(maneouvre.empty() == false){
		for(vector<string>::iterator i = maneouvre.begin(); i != maneouvre.end(); ++i){
			//
			if(*i == ud->name){
				TCommand TCS(tc,"setting firing state/movestate");
				tc.unit = unit;
				tc.c.id = CMD_MOVE_STATE;
				tc.c.params.push_back(1);
				tc.type = B_CMD;
				G->OrderRouter->GiveOrder(tc,false);
				break;
			}
		}
	}
	if(hold_pos.empty() == false){
		for(vector<string>::iterator i = hold_pos.begin(); i != hold_pos.end(); ++i){
			//
			if(*i == ud->name){
				TCommand TCS(tc,"setting firing state/movestate");
				tc.unit = unit;
				tc.c.id = CMD_MOVE_STATE;
				tc.c.params.push_back(0);
				tc.type = B_CMD;
				G->OrderRouter->GiveOrder(tc,false);
				break;
			}
		}
	}
	if(roam.empty() == false){
		for(vector<string>::iterator i = roam.begin(); i != roam.end(); ++i){
			//
			if(*i == ud->name){
				TCommand TCS(tc,"setting firing state/movestate");
				tc.unit = unit;
				tc.c.id = CMD_MOVE_STATE;
				tc.c.params.push_back(2);
				tc.type = B_CMD;
				G->OrderRouter->GiveOrder(tc,false);
				break;
			}
		}
	}
	if(fire_at_will.empty() == false){
		for(vector<string>::iterator i = fire_at_will.begin(); i != fire_at_will.end(); ++i){
			//
			if(*i == ud->name){
				TCommand TCS(tc,"setting firing state/movestate");
				tc.unit = unit;
				tc.c.id = CMD_FIRE_STATE;
				tc.c.params.push_back(2);
				tc.type = B_CMD;
				G->OrderRouter->GiveOrder(tc,false);
				break;
			}
		}
	}
	if(return_fire.empty() == false){
		for(vector<string>::iterator i = return_fire.begin(); i != return_fire.end(); ++i){
			//
			if(*i == ud->name){
				TCommand TCS(tc,"setting firing state/movestate");
				tc.unit = unit;
				tc.c.id = CMD_FIRE_STATE;
				tc.c.params.push_back(1);
				tc.type = B_CMD;
				G->OrderRouter->GiveOrder(tc,false);
				break;
			}
		}
	}
	if(hold_fire.empty() == false){
		for(vector<string>::iterator i = hold_fire.begin(); i != hold_fire.end(); ++i){
			//
			if(*i == ud->name){
				TCommand TCS(tc,"setting firing state/movestate");
				tc.unit = unit;
				tc.c.id = CMD_FIRE_STATE;
				tc.c.params.push_back(0);
				tc.type = B_CMD;
				G->OrderRouter->GiveOrder(tc,false);
				break;
			}
		}
	}

	bool atk = false;
	if(G->info->dynamic_selection == false){
		vector<string> v;
		v = bds::set_cont(v,G->Get_mod_tdf()->SGetValueMSG("AI\\Attackers"));
		if(v.empty() == false){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				if(*vi == ud->name){
					atk = true;
					break;
				}
			}
		}
		if(ud->builder == true) atk = false; // no builders either
		if((ud->canfly == false)&&(ud->movedata == 0)) atk = false; // no buildings
	}else{
		// determine dynamically if this is an attacker
		atk =true;
		if(ud->speed > G->info->scout_speed) atk = false; // cant be a scout either
		if(ud->isCommander == true) atk = false; //no commanders
		if(ud->builder == true) atk = false; // no builders either
		if(ud->weapons.empty() == true) atk = false; // has to have a weapon
		if((ud->canfly == false)&&(ud->movedata == 0)) atk = false; // no buildings
		if(ud->transportCapacity != 0) atk = false; // no armed transports
		if(ud->hoverAttack == true) atk = true; // obviously a gunship type thingymajig
	}
	if(atk == true){
		Add(unit);
		G->L.print("new attacker added :: " + ud->name);
		if(((attack_units.size())/min(max(int(groups.size()),1),1) < threshold*0.7f)||(groups.size() > 4)){
			//merge all units into a single mega group
			G->L.print("merging all attack groups into a single mega group");
			if(Attackers.empty() == false){
				Attackers.erase(Attackers.begin(),Attackers.end());
				Attackers.clear();
			}
			if(groups.empty() == false){
				groups.erase(groups.begin(),groups.end());
				groups.clear();
			}
			Attackers = attack_units;
		}
		if((int)Attackers.size()>threshold){
			ackforce ac(G);
			ac.marching = false;
			ac.idle = true;
			if((G->info->mexfirst == true)&&(groups.empty() == true)) ac.type = a_mexraid;
			ac.units = Attackers;
			groups.push_back(ac);
			FindTarget(&groups.back(),false);
			//_beginthread((void (__cdecl *)(void *))AckInsert, 0, (void*)ik);
			Attackers.erase(Attackers.begin(),Attackers.end());
			Attackers.clear();
		} else {
			Attackers.insert(unit);
		}
		if(G->Actions->AttackNear(unit,5.0f)==false){
			if(G->Actions->SeekOutLastAssault(unit)==false){
				if(G->Actions->SeekOutNearestInterest(unit)==true){
					walking.insert(unit);
				}
			}else{
				walking.insert(unit);
			}
		}else{
			engaged.insert(unit);
		}
	}
	NLOG("kamikaze");
	if(sd_proxim.empty() == false){
		string sh = G->lowercase(ud->name);
		if(sd_proxim.find(sh) != sd_proxim.end()){
			kamikaze_units.insert(unit);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Chaser::~Chaser(){
	delete [] threat_matrix;
	//delete Targetting;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::EnemyDestroyed(int enemy, int attacker){
	NLOG("Chaser::EnemyDestroyed");
	NO_GAIA(NA)
	if(enemy < 1) return;
	engaged.erase(attacker);
	float3 dir = G->GetUnitPos(enemy);
	if(ensites.find(enemy) != ensites.end()) ensites.erase(enemy);
	if(dir == ZeroVector) return;
	if(G->Map->CheckFloat3(dir) == false){
		return;
	}else{
		dir = G->Map->WhichSector(dir);
		const UnitDef* def = G->GetEnemyDef(enemy);
		if(def == 0){
			if(threat_matrix[sector[uint(dir.x)][uint(dir.z)].index] <10) return;
			threat_matrix[sector[uint(dir.x)][uint(dir.z)].index] -= 120;
			if(threat_matrix[sector[uint(dir.x)][uint(dir.z)].index] <1.0f) threat_matrix[sector[uint(dir.x)][uint(dir.z)].index] = 1.0f;
		}else{
			if(def->extractsMetal > 0) G->M->EnemyExtractorDead(dir,enemy);
			int index = sector[uint(dir.x)][uint(dir.z)].index;
			if(index >-1){
				threat_matrix[index] -= G->GetEfficiency(def->name);
				if(threat_matrix[index] <1.0f) threat_matrix[index] = 1.0f;
			}
			
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool T_FindTarget(T_Targetting* T){
	//NLOG("Chaser::T_FindTarget");
	T->G->Ch->lock = true;
	if(T->i->units.empty() == true){
		T->i->idle =  true;
		delete T;
		return false;
	}
	float3 final_dest = UpVector;
	if(T->i->type == a_mexraid){
		final_dest = T->G->Sc->GetMexScout(T->i->i);
		T->i->i++;
		if(T->G->Map->CheckFloat3(final_dest) == false){
			delete T;
			return false;
		}
	}else{
		float best_rating , my_rating;
		best_rating = my_rating = 0;
		float3 dest = UpVector;
		float sx1 = -1,sy1 = -1;

		// TODO: improve destination sector selection
		for(int x = 0; x < T->G->Ch->xSectors; x++){
			for(int y = 0; y < T->G->Ch->ySectors; y++){
				my_rating =  T->G->Ch->threat_matrix[T->G->Ch->sector[x][y].index];
				if(my_rating > best_rating){
					if(T->i->starg != T->G->Ch->sector[x][y].centre){
						dest = T->G->Ch->sector[x][y].centre;
						best_rating = my_rating;
						sx1 = float(x);
						sy1 = float(y);
					}
				}
			}
		}
		T->G->Ch->threat_matrix[T->G->Ch->sector[(int)sx1][(int)sy1].index]=1.0f;
		if(best_rating < 30){
			delete T;
			return false;
		}
		if((sx1 == -1)||(sy1 == -1)){
			delete T;
			return false;
		}
		final_dest = dest;
	}
	if(T->G->Map->CheckFloat3(final_dest) == false){
		T->G->L.print("find target exiting on up vector target");
		delete T;
		return false;
	}else{
		if(T->i->units.empty() == false){
			for(set<int>::iterator aik = T->i->units.begin();aik != T->i->units.end();++aik){
				if(T->G->cb->GetUnitHealth(*aik) >1){
					TCommand TCS(tc,"chaser::_TFindTarget attacking move.area_attack");
					tc.clear = false;
					tc.Priority = tc_pseudoinstant;
					if(T->i->type == a_mexraid){
						tc.c.id = CMD_AREA_ATTACK;
						tc.c.params.push_back(final_dest.x);
						tc.c.params.push_back(T->G->cb->GetElevation(final_dest.x,final_dest.z));
						tc.c.params.push_back(final_dest.z);
						tc.created = T->G->cb->GetCurrentFrame();
						tc.c.timeOut = tc.created + 10 MINUTES;
						tc.c.params.push_back(300.0f);
						tc.unit = *aik;
						if(tc.c.params.empty() == false)T->G->OrderRouter->GiveOrder(tc);
					}else {
						float3 npos = T->G->Map->distfrom(T->G->GetUnitPos(*aik),final_dest,T->G->cb->GetUnitMaxRange(*aik)*0.95f);
						npos.y = T->G->cb->GetElevation(npos.x,npos.z);
						T->G->Actions->Move(*aik,npos);
					}
				}
			}
		}
		if(T->upthresh == true){
			T->G->Ch->threshold++;
			char c[10];
			sprintf(c,"%i",T->G->Ch->threshold);
			T->G->L.print(string("new threshold :: ") + c);
		}
		T->i->marching = true;
		T->i->idle = false;
	}
	T->G->Ch->lock = false;
	delete T;
	return true;
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
bool Chaser::FindTarget(ackforce* i, bool upthresh){
	NLOG("Chaser::FindTarget");
	NO_GAIA(false)
	/*float3 final_dest = UpVector;
	if(i->type == a_mexraid){
		final_dest = G->Sc->GetMexScout(i->i);
		i->i++;
		if(G->Map->CheckFloat3(final_dest) == false){
			return false;
		}else{
			return true;
		}
	}else{*/
		T_Targetting* T = new T_Targetting;
		T->G = G;
		T->i = i;
		T->upthresh = upthresh;
		NLOG("attempting to find a target");
		bool ab= T_FindTarget(T);
		if(ab==true){
			NLOG("a target was found");
			return true;
		}else{
			NLOG("a target wasnt found");
			return false;
		}
	/*}
		G->L.print("targeting started");
		Targetting->GetPoints(threat_matrix);
		G->L.print("targeting done");
		G->L.print("wiping caches");
		Targetting->WipeCache();
		G->L.print("caches wiped");
		if(Targetting->VectoredSpots.empty() == true){
			G->L.print("no targets found");
			return false;
		}
		G->L.print("checking targets");
		for(vector<float3>::iterator ik = Targetting->VectoredSpots.begin(); ik != Targetting->VectoredSpots.end(); ++ik){
			if(ik->y > final_dest.y){
				if(i->starg != *ik){
					final_dest = *ik;
				}
			}
		}
		G->L.print("targets checked");
	}
	if(G->Map->CheckFloat3(final_dest) == false){
		G->L.print(_T("find target exiting on up vector target"));
		return false;
		//_endthread();
	}else{
		if(i->units.empty() == false){
			i->starg = final_dest;
			final_dest = sector[uint(final_dest.x)][uint(final_dest.z)].centre;
			for(set<int>::const_iterator aik = i->units.begin();aik != i->units.end();++aik){
				if(G->cb->GetUnitHealth(*aik) >1){
					TCommand TCS(tc,_T("chaser::_TFindTarget attacking move.area_attack"));
					tc.clear = false;
					tc.Priority = tc_pseudoinstant;
					if(i->type == a_mexraid){
						tc.c.id = CMD_AREA_ATTACK;
					}else {
						tc.c.id = CMD_MOVE;
					}
					float3 npos = G->Map->distfrom(G->GetUnitPos(*aik),final_dest,G->cb->GetUnitMaxRange(*aik)*0.98f);
					tc.c.params.push_back(npos.x);
					tc.c.params.push_back(G->cb->GetElevation(npos.x,npos.z));
					tc.c.params.push_back(npos.z);
					tc.created = G->cb->GetCurrentFrame();
					tc.c.timeOut = tc.created + 10 MINUTES;
					if(i->type == a_mexraid) tc.c.params.push_back(300.0f);
					tc.unit = *aik;
					if(tc.c.params.empty() == false)G->OrderRouter->GiveOrder(tc);
				}
			}
		}
		if(upthresh == true){
			threshold++;
			char c[10];
			sprintf(c,"%i",threshold);
			G->L.print(string(_T("new threshold :: ")) + c);
		}
		i->marching = true;
		i->idle = false;
	}
	return true;*/
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitIdle(int unit){
	NLOG("Chaser::UnitIdle");
	NO_GAIA(NA)
	if(dgunning.empty() == false){
		dgunning.erase(unit);
	}
	engaged.erase(unit);
	walking.erase(unit);
	const UnitDef* ud = G->GetUnitDef(unit);
	if(ud == 0) return;
	if(G->Cached->enemies.empty() == true) return;
	if(ud->builder == true) return;
	if(ud->isCommander == true) return;
	if(ud->weapons.empty() == true) return;
	//if(G->Actions->AttackNear(unit)==false){
		/* find attack group*/
	bool found = false;
	if(this->Attackers.empty() == false){
		for(set<int>::iterator sd = Attackers.begin(); sd != Attackers.end(); ++sd){
			if(*sd == unit){
				found = true;
				float3 pos = G->GetUnitPos(unit);
				if(G->Map->CheckFloat3(pos)==false) continue;
				if(engaged.find(unit) != engaged.end()) continue;
				if(G->Actions->AttackNear(unit,1) == false){
					if(G->Actions->AttackNear(unit,2)==false){
						if(G->Actions->AttackNear(unit,3)==false){
							if(G->Actions->AttackNear(unit,4)==false){
								if(walking.find(unit) == walking.end()){
									if(G->Actions->SeekOutInterest(unit)==false){
										if(G->Actions->SeekOutLastAssault(unit)==false){
											if(G->Actions->SeekOutNearestInterest(unit)==false){
												if(G->Actions->RandomSpiral(unit)==false){
													//	G->Actions->ScheduleIdle(*aa);
												}else{
													walking.insert(unit);
												}
											}else{
												walking.insert(unit);
											}
										}else{
											walking.insert(unit);
										}
									}else{
										walking.insert(unit);
									}
								}else{
									continue;
								}
							}else{
								engaged.insert(unit);
								Beacon(pos,600);
							}
						}else{
							engaged.insert(unit);
							Beacon(pos,600);
						}
					}else{
						engaged.insert(unit);
						Beacon(pos,600);
					}
				}else{
					engaged.insert(unit);
					Beacon(pos,600);
				}
				return;
			}
		}
	}
	if(this->groups.empty() == false){
		if(found == false){
			for(vector<ackforce>::iterator si = groups.begin(); si != groups.end(); ++si){
				if(si->units.empty() == false){
					for(set<int>::iterator sd = si->units.begin(); sd != si->units.end(); ++sd){
						if(*sd == unit){
							found = true;
							if(FindTarget(&*si,false) == false){
								float3 pos = G->GetUnitPos(unit);
								if(G->Map->CheckFloat3(pos)==false) continue;
								if(engaged.find(unit) != engaged.end()) continue;
								if(G->Actions->AttackNear(unit,1) == false){
									if(G->Actions->AttackNear(unit,2)==false){
										if(G->Actions->AttackNear(unit,3)==false){
											if(G->Actions->AttackNear(unit,4)==false){
												if(walking.find(unit) == walking.end()){
													if(G->Actions->SeekOutInterest(unit)==false){
														if(G->Actions->SeekOutLastAssault(unit)==false){
															if(G->Actions->SeekOutNearestInterest(unit)==false){
																if(G->Actions->RandomSpiral(unit)==false){
																	//	G->Actions->ScheduleIdle(*aa);
																}else{
																	walking.insert(unit);
																}
															}else{
																walking.insert(unit);
															}
														}else{
															walking.insert(unit);
														}
													}else{
														walking.insert(unit);
													}
												}else{
													continue;
												}
											}else{
												engaged.insert(unit);
												Beacon(pos,600);
											}
										}else{
											engaged.insert(unit);
											Beacon(pos,600);
										}
									}else{
										engaged.insert(unit);
										Beacon(pos,600);
									}
								}else{
									engaged.insert(unit);
									Beacon(pos,600);
								}
							}
							break;
						}
					}
				}
				if(found == true) break;
			}
		}
	}
	return;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Fire units in the sweap array, such as nuke silos etc.

void Chaser::FireSilos(){
	NLOG("Chaser::FireSilos");
	NO_GAIA(NA)
	int fnum = G->cb->GetCurrentFrame(); // Get frame
	if((fnum%200 == 0)&&(sweap.empty() == false)){ // Every 30 seconds
		for(set<int>::iterator si = sweap.begin(); si != sweap.end();++si){
			float maxrange = G->cb->GetUnitMaxRange(*si);
			float best_rating = 0;
			float my_rating = 0; // Search for the area with the highest threat level.
			for(int x = 0; x < xSectors; x++){
				for(int y = 0; y < ySectors; y++){
					if(sector[x][y].centre.distance2D(G->GetUnitPos(*si)) > maxrange) continue;
					my_rating = threat_matrix[sector[x][y].index];
					if(my_rating > best_rating){
						swtarget = sector[x][y].centre;
						best_rating = my_rating;
					}
				}
			}
			G->Actions->Attack(*si,swtarget);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#define TGA_RES 8

void ThreatTGA(Global* G){
	NLOG("ThreatTGA");
	// open file
	char c [4];
	sprintf(c,"%i",G->Cached->team);
	string filename = G->info->datapath + slash + string("threatdebug") + slash + string("threatmatrix") + c + string(".tga");
	FILE *fp=fopen(filename.c_str(), "wb");
	//#ifdef _MSC_VER
 	//	if(!fp)	_endthread();
	//#else
		if(!fp) return;
	//#endif
	map< int, map<int,agrid> > sec = G->Ch->sector;

	// fill & write header
	char targaheader[18];
	memset(targaheader, 0, sizeof(targaheader));
	float biggestebuild = 1;
	float biggestheight = 1;
	float smallestheight = 0;
	float p;
	for(int xa = 0; xa < G->Ch->xSectors; xa++){
		for(int ya = 0; ya < G->Ch->ySectors; ya++){
			if(G->Ch->threat_matrix[sec[xa][ya].index] / (sec[xa][ya].centre.distance(G->Map->nbasepos(sec[xa][ya].centre))/4) > biggestebuild)	biggestebuild = G->Ch->threat_matrix[sec[xa][ya].index] / (sec[xa][ya].centre.distance(G->Map->nbasepos(sec[xa][ya].centre))/4);
			float g = G->cb->GetElevation(sec[xa][ya].centre.x,sec[xa][ya].centre.z);
			if(g > biggestheight) biggestheight = g;
			if(g < smallestheight) smallestheight = g;
		}
	}
	if(smallestheight < 0.0f) smallestheight *= -1;
	p = (smallestheight + biggestheight)/255;
	if( p == 0.0f) p = 0.01f;

	targaheader[2] = 2;		/* image type = uncompressed gray-scale */
	targaheader[12] = (char) (G->Ch->xSectors*TGA_RES & 0xFF);
	targaheader[13] = (char) (G->Ch->xSectors*TGA_RES >> 8);
	targaheader[14] = (char) (G->Ch->ySectors*TGA_RES & 0xFF);
	targaheader[15] = (char) (G->Ch->ySectors*TGA_RES >> 8);
	targaheader[16] = 24; /* 24 bit color */
	targaheader[17] = 0x20;	/* Top-down, non-interlaced */

	fwrite(targaheader, 18, 1, fp);

	// write file
	uchar out[3];
	float ebuildp = (biggestebuild)/255;
	if(ebuildp <= 0.0f) ebuildp = 1;
	for (int z=0;z<G->Ch->ySectors*TGA_RES;z++){
		for (int x=0;x<G->Ch->xSectors*TGA_RES;x++){
			const int div=4;
			int x1 = x;
			int z1 = z;
			if(z%TGA_RES != 0) z1 = z-(z%TGA_RES); z1 = z1/TGA_RES;
			if(x%TGA_RES != 0) x1 = x-(x%TGA_RES); x1 = x1/TGA_RES;
			out[0]=(uchar)/*min(255, (int)*/max(1.0f,(sec[x1][z1].centre.y+smallestheight)/p);//);//Blue Heightmap
			//out[1]=(uchar)/*min(255,(int)*/max(1.0f,(sec[x1][z1].index] / (sec[x1][z1].centre.distance(G->Map->nbasepos(sec[x1][z1].centre))/4)/groundp) / div);//);//Green Ground threat
			out[2]=(uchar)/*min(255,(int)*/max(1.0f,(G->Ch->threat_matrix[sec[x1][z1].index] / (sec[x1][z1].centre.distance(G->Map->nbasepos(sec[x1][z1].centre))/4)/ebuildp)/div);//);//Red Building threat

			fwrite (out, 3, 1, fp);
		}
	}
	fclose(fp);
	#ifdef _MSC_VER
	system(filename.c_str());
	//_endthread();
	#endif
	return;
}
void Chaser::MakeTGA(){
	NLOG("Chaser::MakeTGA");
	//#ifdef _MSC_VER
 	//_beginthread((void (__cdecl *)(void *))ThreatTGA, 0, (void*)G);
	//#else
	ThreatTGA(G);
	//#endif
 }
void Chaser::DepreciateMatrix(){
	threat_matrix[0] *= 0.9999f;
	if(EVERY_(3 FRAMES)){
		for(map<int, map<int,agrid> >::iterator i = sector.begin(); i != sector.end(); ++i){
			for(map<int,agrid>::iterator j = i->second.begin(); j != i->second.end(); ++j){
				int index = j->second.index;
				if(index > max_index) continue;
				if(index >-1){
					if(G->Cached->cheating==true){
						threat_matrix[index] *= 0.95f;
					}else{
						threat_matrix[index] *= 0.9995f;
					}
				}
			}
		}
	}
}
void Chaser::UpdateMatrixFriendlyUnits(){
	NLOG("Chaser::Update :: threat matrix update friendly units");
	int* funits = new int[6000];
	int fu = G->cb->GetFriendlyUnits(funits);
	if(fu > 0){
		for(int f = 0; f <fu; f++){
			const UnitDef* urd = G->GetUnitDef(funits[f]);
			if(urd != 0){
				float3 fupos = G->GetUnitPos(funits[f]);
				float3 fusec = G->Map->WhichSector(fupos);
				if(G->Map->CheckFloat3(fusec) == false) continue;
				int z = min(ySectors,int(fusec.z-1));
				int x = min(xSectors,int(fusec.x-1));
				if(urd->losRadius > max(xSize,ySize)){
					int* eg = new int[1000];
					int iy = G->GetEnemyUnits(eg,fupos,300.0f);
					if(eg ==  0){
						threat_matrix[sector[x][z].index]= 1.0f;
						threat_matrix[sector[x][z].index] = 1.0f;
					}
					delete [] eg;
				}
			}
		}
	}
	delete [] funits;
}
//
void Chaser::CheckKamikaze(){
	if((kamikaze_units.empty() == false)&&(G->Cached->enemies.empty() == false)){
		for(set<int>::iterator i = kamikaze_units.begin(); i != kamikaze_units.end(); ++i){
			int* funits = new int [G->Cached->enemies.size()];
			int fu = G->GetEnemyUnits(funits,G->GetUnitPos(*i),G->cb->GetUnitMaxRange(*i));
			if(fu > 0 ){
				TCommand TCS(tc,"chaser::update selfd");
				tc.unit = *i;
				tc.Priority = tc_instant;
				tc.c.id = CMD_SELFD;
				tc.created = G->cb->GetCurrentFrame();
				tc.delay= 0;
				G->OrderRouter->GiveOrder(tc);
			}
			delete [] funits;
		}
	}
}
//
void Chaser::RemoveBadSites(){
	int* ef = new int[10000];
	int ecount=0;
	for(map<int,esite>::iterator i = ensites.begin(); i != ensites.end(); ++i){
		esite e =i->second;
		if(e.ground == false){
			if(G->Cached->cheating == false){
				if(G->InLOS(e.position) == true){
					int ecount = G->GetEnemyUnits(ef,e.position,10);
					if(ecount < 1){
						int index = sector[(uint)e.sector.x][(uint)e.sector.z].index;
						if(index > max_index){
							ensites.erase(i);
							break;
						}
						threat_matrix[index] = threat_matrix[index] - 10*e.efficiency;
						if(threat_matrix[index] < 1.0f) threat_matrix[index] = 1.0f;
						ensites.erase(i);
						break;
					}else{
						ecount = 0;
						continue;
					}
				}else{
					continue;
				}
			}else{
				ecount = G->GetEnemyUnits(ef,e.position,10);
				if(ecount < 1){
					ensites.erase(i);
					break;
				}else{
					ecount = 0;
					continue;
				}
			}
		}else{
			if(G->cb->GetCurrentFrame() - (3 SECONDS) > e.created){
				ensites.erase(i);
				ecount = 0;
				break;
			}
		}
	}
	delete [] ef;
}
//
void Chaser::UpdateSites(){
	for(map<int,esite>::iterator i = ensites.begin(); i != ensites.end(); ++i){
		esite e =i->second;
		int x = min(int(e.sector.x),xSectors-1);
		int z = min(int(e.sector.z),ySectors-1);
		if(e.ground == false){
			threat_matrix[sector[x][z].index] += e.efficiency;
		}else{
			threat_matrix[sector[x][z].index] += e.efficiency;
		}
	}
}
//
void Chaser::UpdateMatrixEnemies(){
	NLOG("Chaser::Update :: update threat matrix enemy targets");
	float3 pos = UpVector;
	// get all enemies in los
	int* en = new int[10000];
	int unum = G->GetEnemyUnitsInRadarAndLos(en);//GetEnemyUnits(en);

	// go through the list 
	if(unum > 0){
		for(int i = 0; i < unum; i++){
			if(en[i] < 1){
				continue;
			}else{
				if(G->cb->GetUnitAllyTeam(en[i]) == G->Cached->unitallyteam) continue;
				pos = G->GetUnitPos(en[i]);
				esite es;
				es.position = pos;
				pos = G->Map->WhichSector(pos);
				es.sector = pos;
				if(G->Map->CheckFloat3(pos) == false) continue;
				int x = min(int(pos.x),xSectors-1);
				int y = min(int(pos.z),ySectors-1);
				int index = sector[x][y].index;
				if(index > max_index){
					continue;
				}
				const UnitDef* def = G->GetEnemyDef(en[i]);
				if(def == 0){
					threat_matrix[index] += 2000;
					continue;
				}else{
					if(def->extractsMetal > 0.0f) G->M->EnemyExtractor(pos,en[i]);
					float ef = G->GetEfficiency(def->name);
					es.efficiency = ef;
					G->Actions->AddPoint(es.position);
					if((def->movedata == 0)&&(def->canfly == false)){ // its a building
						threat_matrix[index] += ef;
						es.ground = false;
						ensites[en[i]] = es;
					}else{
						threat_matrix[index] *= ef;
						es.ground = true;
						es.created  = G->cb->GetCurrentFrame();
						ensites[en[i]] = es;
					}
				}

			}
		}
	}
	delete [] en;
}
//


void Chaser::Update(){
	NLOG("Chaser::Update");
	NO_GAIA(NA)
	// decrease threat values of all sectors..
	EXCEPTION_HANDLER(DepreciateMatrix(),"Chaser::DepreciateMatrix()",NA)
	if(EVERY_(60 FRAMES)){
		EXCEPTION_HANDLER(UpdateMatrixFriendlyUnits(),"Chaser::UpdateMatrixFriendlyUnits()",NA)
	}
	// Run the code that handles stockpiled weapons.
	EXCEPTION_HANDLER({
		if(EVERY_((10 SECONDS))){
			if((sweap.empty() == false)&&(G->Cached->enemies.empty() == false)){
				NLOG("Chaser::Update :: firing silos");
				FireSilos();
			}
		}
	},"Chaser::Update :: firing silos",NA)
	if(EVERY_((3 SECONDS))){
		EXCEPTION_HANDLER(CheckKamikaze(),"Chaser::CheckKamikaze()",NA)
	}
	if((EVERY_((2 FRAMES)))&&(ensites.empty() == false)){ // remove enemy sites logged if they're in LOS and they cant be seen....
		EXCEPTION_HANDLER(RemoveBadSites(),"Chaser::RemoveBadSites()",NA)
	}
	if((EVERY_((11 FRAMES)))&&(ensites.empty() == false)){
		EXCEPTION_HANDLER(UpdateSites(),"Chaser::UpdateSites()",NA)
	}
	if((EVERY_((10 FRAMES)))&&(G->Cached->enemies.empty() == false)){
		EXCEPTION_HANDLER(UpdateMatrixEnemies(),"Chaser::UpdateMatrixEnemies()",NA)
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	// re-evaluate the target of an attack force
	if(EVERY_(919 FRAMES)){
		EXCEPTION_HANDLER(EvaluateTargets(),"Chaser::EvaluateTargets()",NA)
	}
	if(EVERY_(120 FRAMES)/*&&(G->enemies.empty() == false)*/){
		EXCEPTION_HANDLER(FireWeaponsNearby(),"Chaser::FireWeaponsNearby()",NA)
	}
	if(EVERY_(120 FRAMES)&&(G->Cached->enemies.empty() == false)){
		EXCEPTION_HANDLER(FireDefences(),"Chaser::FireDefences()",NA)
	}
	if(EVERY_(30 FRAMES)){
		EXCEPTION_HANDLER(FireDgunsNearby(),"Chaser::FireDgunsNearby()",NA)
	}
	NLOG("Chaser::Update :: DONE");
}

void Chaser::EvaluateTargets(){
	NLOG("Chaser::Update :: re-evaluate attack targets");
	if(groups.empty() == false){
		for(vector<ackforce>::iterator af = groups.begin();af != groups.end();++af){
			if(af->units.empty() == false){
				if(FindTarget(&*af,false)==false){
					for(set<int>::iterator i = af->units.begin(); i != af->units.end(); ++i){
						if(G->Actions->AttackNear(*i,1)==false){
							if(G->Actions->AttackNear(*i,2)==false){
								if(G->Actions->AttackNear(*i,3)==false){
									if(G->Actions->AttackNear(*i,4)==false){
										if(G->Actions->SeekOutInterest(*i)==false){
											if(G->Actions->RandomSpiral(*i) == false){
												//	G->Actions->ScheduleIdle(*i);
											}
										}
									}
								}
							}
						}
					}
				}
			}else{
				groups.erase(af); // a group with empty units in it shouldn't be kept intact else this container will keep getting bigger and bigger and distort other calculations...
			}
		}
	}
	NLOG("Chaser::Update :: re-evaluate attack targets done");
}

void Chaser::Beacon(float3 pos, float radius){
	if(walking.empty() == false){
		set<int> rejects;
		for(set<int>::iterator i = walking.begin(); i != walking.end(); ++i){
			float3 rpos = G->GetUnitPos(*i);
			if(G->Map->CheckFloat3(rpos)==true){
				if (rpos.distance2D(pos)<radius){
					G->Actions->Move(*i,pos);
				}
			}
		}
	}
}
void Chaser::FireWeaponsNearby(){
	NLOG("Chaser::Update :: make attackers attack nearby targets");
	if(Attackers.empty() == false){
		for(set<int>::iterator aa = Attackers.begin(); aa != Attackers.end();++aa){
			float3 pos = G->GetUnitPos(*aa);
			if(G->Map->CheckFloat3(pos)==false) continue;
			if(engaged.find(*aa) != engaged.end()) continue;
			if(G->Actions->AttackNear(*aa,1) == false){
				if(G->Actions->AttackNear(*aa,2)==false){
					if(G->Actions->AttackNear(*aa,3)==false){
						if(G->Actions->AttackNear(*aa,4)==false){
							if(walking.find(*aa) == walking.end()){
								if(G->Actions->SeekOutInterest(*aa)==false){
									if(G->Actions->SeekOutLastAssault(*aa)==false){
										if(G->Actions->SeekOutNearestInterest(*aa)==false){
											if(G->Actions->RandomSpiral(*aa)==false){
												//	G->Actions->ScheduleIdle(*aa);
											}else{
												walking.insert(*aa);
											}
										}else{
												walking.insert(*aa);
											}
									}else{
										walking.insert(*aa);
									}
								}else{
									walking.insert(*aa);
								}
							}else{
								continue;
							}
						}else{
							engaged.insert(*aa);
							Beacon(pos,600);
						}
					}else{
						engaged.insert(*aa);
						Beacon(pos,600);
					}
				}else{
					engaged.insert(*aa);
					Beacon(pos,600);
				}
			}else{
				engaged.insert(*aa);
				Beacon(pos,600);
			}
		}
	}
	NLOG("Chaser::Update :: make sent attackers attack nearby targets");
	if(groups.empty() == false){
		for(vector<ackforce>::iterator k = groups.begin(); k != groups.end(); ++k){
			if(k->units.empty() == false){
				if(FindTarget(&*k,false)==false){
					for(set<int>::iterator aa = k->units.begin(); aa != k->units.end();++aa){
						float3 pos = G->GetUnitPos(*aa);
						if(G->Map->CheckFloat3(pos)==false) continue;
						if(engaged.find(*aa) != engaged.end()) continue;
						if(G->Actions->AttackNear(*aa,1.5) == false){
							if(G->Actions->AttackNear(*aa,3)==false){
								if(G->Actions->AttackNear(*aa,4)==false){
									if(G->Actions->AttackNear(*aa,5)==false){
										if(walking.find(*aa) == walking.end()){
											if(G->Actions->SeekOutInterest(*aa)==false){
												if(G->Actions->SeekOutLastAssault(*aa)==false){
													if(G->Actions->SeekOutNearestInterest(*aa)==false){
														if(G->Actions->RandomSpiral(*aa)==false){
															//	G->Actions->ScheduleIdle(*aa);
														}else{
															walking.insert(*aa);
														}
													}else{
														walking.insert(*aa);
													}
												}else{
													walking.insert(*aa);
												}
											}else{
												walking.insert(*aa);
											}
										}else{
											continue;
										}
									}else{
										engaged.insert(*aa);
										Beacon(pos,2000);
									}
								}else{
									engaged.insert(*aa);
									Beacon(pos,2000);
								}
							}else{
								engaged.insert(*aa);
								Beacon(pos,2000);
							}
						}else{
							engaged.insert(*aa);
							Beacon(pos,2000);
						}
					}
				}
			}else{
				groups.erase(k);
			}
		}
	}
}

void Chaser::FireDgunsNearby(){
	NLOG("Chaser::Update :: make units dgun nearby enemies nearby targets");
	if(dgunners.empty() == false){
		for(set<int>::iterator aa = dgunners.begin(); aa != dgunners.end();++aa){
			G->Actions->DGunNearby(*aa);
		}
	}
}

void Chaser::FireDefences(){
	NLOG("Chaser::Update :: make defences attack nearby targets");
	if(defences.empty() == false){
		for(set<int>::iterator aa = defences.begin(); aa != defences.end();++aa){
			G->Actions->AttackNear(*aa,1.0f);
		}
	}
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

