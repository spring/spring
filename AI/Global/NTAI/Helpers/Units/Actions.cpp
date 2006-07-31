// Actions
#include "../../Core/helper.h"

CActions::CActions(Global* GL){
	G = GL;
	last_attack = UpVector;
}

bool CActions::Attack(int uid, float3 pos){
	NLOG("CActions::Attack Position");
	if(uid < 1) return false;
	if(G->Map->CheckFloat3(pos)==false) return false;
	TCommand TCS(tc,"Atatck Pos CActions");
	tc.ID(CMD_ATTACK);
	tc.PushFloat3(pos);
	tc.created = G->cb->GetCurrentFrame();
	tc.unit = uid;
	return G->OrderRouter->GiveOrder(tc,true);
}
bool CActions::Trajectory(int unit,int traj){
	NLOG("CActions::Trajectory");
	TCommand TCS(tc,"Trajectory CAction");
	tc.c.timeOut = 400+(G->cb->GetCurrentFrame()%600) + G->cb->GetCurrentFrame();
	tc.Priority = tc_low;
	tc.unit = unit;
	tc.ID(CMD_TRAJECTORY);
	tc.Push(traj);
	return G->OrderRouter->GiveOrder(tc);
}

void CActions::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	float3 pos = G->GetUnitPos(attacker);
	if(G->Map->CheckFloat3(pos)==true){
		last_attack=pos;
	}else{
		pos = G->GetUnitPos(damaged);
		if(G->Map->CheckFloat3(pos)==true){
			last_attack=pos;
		}
	}
}

bool CActions::Attack(int unit, int target,bool mobile){
	NLOG("CActions::Attack Target");
	if(unit < 1) return false;
	if(target < 1) return false;
	float3 pos = G->GetUnitPos(target);
	if(G->Map->CheckFloat3(pos)==true){
		last_attack=pos;
	}else{
		pos = G->GetUnitPos(unit);
		if(G->Map->CheckFloat3(pos)==true){
			last_attack=pos;
		}
	}
	TCommand TCS(tc,"attack target CActions");
	tc.ID(CMD_ATTACK);
	tc.Push(target);
	tc.created = G->cb->GetCurrentFrame();
	tc.unit = unit;
	return G->OrderRouter->GiveOrder(tc,true);
	//if(G->OrderRouter->GiveOrder(tc,true)==true){
		//if(mobile==true) attackers.insert(unit);
	//	return true;
	//}else{
	//	return false;
	//}
}

bool CActions::Capture(int uid,int enemy){
	NLOG("CActions::Capture");
	if(uid < 1) return false;
	if(enemy < 1) return false;
	TCommand TCS(tc,"capture CActions");
	tc.ID(CMD_CAPTURE);
	tc.Push(enemy);
	tc.created = G->GetCurrentFrame();
	tc.unit = uid;
	return G->OrderRouter->GiveOrder(tc,true);
}

bool CActions::Repair(int uid,int unit){
	NLOG("CActions::Repair");
	if(uid < 1) return false;
	if(unit < 1) return false;
	const UnitDef* udi = G->GetUnitDef(unit);
	if(udi == 0) return false;
	const UnitDef* bud = G->GetUnitDef(unit);
	if(bud == 0) return false;
	float Remainder = 1.0f - G->cb->GetUnitHealth(unit) / G->cb->GetUnitMaxHealth(unit);
	float RemainingTime = bud->buildTime / udi->buildSpeed * Remainder;
	if (RemainingTime < 30.0f) return false;
	float EPerS = bud->energyCost / (bud->buildTime / udi->buildSpeed);
	float MPerS = bud->metalCost / (bud->buildTime / udi->buildSpeed);
	if (	G->cb->GetMetal() + (G->Pl->GetMetalIncome() - MPerS) * RemainingTime > 0	&& G->cb->GetEnergy() + (G->Pl->GetEnergyIncome() - EPerS) * RemainingTime > 0	){
		if (RemainingTime > 120.0f || bud->buildSpeed > 0)	{
			TCommand TCS(tc,"capture CActions");
			tc.ID(CMD_REPAIR);
			tc.Push(unit);
			tc.created = G->cb->GetCurrentFrame();
			tc.unit = uid;
			return G->OrderRouter->GiveOrder(tc,true);
		}else{
			return false;
		}
	}else{
		return false;
	}
}

bool CActions::MoveToStrike(int unit,float3 pos){
	NLOG("CActions::MoveToStrike");
	if(unit < 1) return false;
	if(G->Map->CheckFloat3(pos)==false) return false;
	float3 npos = G->Map->distfrom(G->GetUnitPos(unit),pos,G->cb->GetUnitMaxRange(unit)*0.95f);
	npos.y = G->cb->GetElevation(npos.x,npos.z);
	return Move(unit,npos);
}
bool CActions::Move(int unit,float3 pos){
	NLOG("CActions::Move");
	if(unit < 1) return false;
	if(G->Map->CheckFloat3(pos)==false) return false;
	TCommand TCS(tc,"move CActions");
	tc.ID(CMD_MOVE);
	tc.PushFloat3(pos);
	tc.created = G->cb->GetCurrentFrame();
	tc.unit = unit;
	return G->OrderRouter->GiveOrder(tc,true);
}

bool CActions::Guard(int unit,int guarded){
	NLOG("CActions::Guard");
	if(unit < 1) return false;
	if(guarded < 1) return false;
	TCommand TCS(tc,"Guard CActions");
	tc.ID(CMD_GUARD);
	tc.Push(guarded);
	tc.created = G->cb->GetCurrentFrame();
	tc.unit = unit;
	return G->OrderRouter->GiveOrder(tc,true);
}

bool CActions::ReclaimNearby(int uid){
	NLOG("CActions::ReclaimNearby");
	float3 upos = G->GetUnitPos(uid);
	if(G->Map->CheckFloat3(upos)){
		int* f = new int[100];
		int fc = G->cb->GetFeatures(f,99,upos,700);
		if(fc >0){
			float smallest = 800;
			int fid = 0;
			for(int i = 0; i < fc; i++){
				float3 fpos = G->cb->GetFeaturePos(f[i]);
				float tdist = fpos.distance2D(upos);
				if(tdist < smallest){
					smallest = tdist;
					fid = f[i];
				}
			}
			delete [] f;
			if(fid >0){
				// do stuff
				return Reclaim(uid,fid);
			}
		}else{
			delete [] f;
			return false;
		}
	}
	return false;
}
bool CActions::RessurectNearby(int uid){
	NLOG("CActions::RessurectNearby");
	float3 upos = G->GetUnitPos(uid);
	if(G->Map->CheckFloat3(upos)){
		int* f = new int[1000];
		int fc = G->cb->GetFeatures(f,99,upos,700);
		delete [] f;
		if(fc >0){
			TCommand TCS(tc,"ressurect nearby CActions");
			tc.ID(CMD_RESURRECT);
			tc.PushFloat3(upos);
			tc.Push(700);
			tc.created = G->cb->GetCurrentFrame();
			tc.unit = uid;
			return G->OrderRouter->GiveOrder(tc,true);
		}else{
			return false;
		}
	}
	return false;
}
bool CActions::AttackNearest(int unit){ // Attack nearby enemies...
	const UnitDef* ud = G->GetUnitDef(unit);
	if(ud == 0) return false;
	if(G->positions.empty()==false){
		float3 p = G->GetUnitPos(unit);
		if(G->Map->CheckFloat3(p)==false){
			return false;
		}
		// iterate through them all generating a score, if the score is the highest encountered
		float highest_score=0.01f;
		int target=0;
		float tscore=0;
		float3 tpos=UpVector;
		int frame= G->GetCurrentFrame()-10;
		bool mobile = false;
		for(map<int,Global::temp_pos>::const_iterator i = G->positions.begin(); i != G->positions.end(); ++i){
			if(i->second.enemy == false){
				continue;
			}
			if(i->second.last_update < frame){
				continue;
			}
			float3 pos = i->second.pos;
			float d = pos.distance2D(p);
			const UnitDef* ude = G->GetUnitDef(i->first);
			if(ude != 0){
				float eff = G->GetTargettingWeight(ud->name,ude->name);
				
				tscore = eff/max(d,0.1f);
				if(tscore > highest_score){
					if(ud->highTrajectoryType == 2){
						if((ude->movedata != 0)||(ude->canfly)){
							if(G->info->hardtarget == false) tscore = tscore/4;
							mobile = true;
						}else{
							mobile = false;
						}
					}
					target = i->first;
					tpos =pos;
					highest_score=tscore;
					tscore=0;
					continue;
				}
			}
		}
		//
		
		if(target > 0){
			if(ud->highTrajectoryType == 2){
				Trajectory(unit,(int)mobile);
			}
			return MoveToStrike(unit,tpos);
		}
	}
	return false;
}
bool CActions::AttackNear(int unit,float LOSmultiplier){
	NLOG("CActions::AttackNear");
	const UnitDef* ud = G->GetUnitDef(unit);
	if(ud == 0) return false;
	int* en = new int[5000];
	int e = G->GetEnemyUnits(en,G->GetUnitPos(unit),max(G->cb->GetUnitMaxRange(unit),ud->losRadius)*LOSmultiplier);
	if(e>0){
		float best_score = 0;
		int best_target = 0;
		float tempscore = 0;
		bool mobile = true;
		for(int i = 0; i < e; i++){
			if(en[i] < 1) continue;
			const UnitDef* endi = G->GetEnemyDef(en[i]);
			if(endi == 0){
				continue;
			}else{
				bool tmobile = false;
				tempscore = G->GetTargettingWeight(ud->name,endi->name);
				if((endi->movedata != 0)||(endi->canfly)){
					if(G->info->hardtarget == false) tempscore = tempscore/4;
					tmobile = true;
				}else if(endi->weapons.empty()==false){
					tempscore /= 2;
				}
				if(tempscore > best_score){
					best_score = tempscore;
					best_target = en[i];
					mobile = tmobile;
				}
				tempscore = 0;
			}
		}
		if(ud->highTrajectoryType == 2){
			Trajectory(unit,(int)mobile);
		}
		if(best_target > 0){
			delete [] en;
			return Attack(unit,best_target,mobile);
		}
		delete [] en;
		return true;
	}else{
		delete [] en;
		return false;
	}
}

void CActions::ScheduleIdle(int unit){
	NLOG("CActions::ScheduleIdle");
	if(unit < 1){
		return;
	}else{
		TCommand TCS(tc,"schedule idle unit");
		tc.unit = unit;
		tc.created = G->cb->GetCurrentFrame();
		tc.ID(CMD_IDLE);
		if(G->OrderRouter->GiveOrder(tc,false)== false){
			G->L.print("GiveOrder on scedule idle failed!!");
		}
	}
}
bool CActions::DGun(int uid,int enemy){
	NLOG("CActions::DGun");
	TCommand TCS(tc,"cmd_attack chaser::update every 4 secs");
	tc.ID(CMD_DGUN);
	tc.Push(enemy);
	tc.c.timeOut = 3 SECONDS+(G->cb->GetCurrentFrame()%300) + G->cb->GetCurrentFrame(); // dont let this command get in the way of the commanders taskqueue with a never ending vendetta that can never be fulfilled

	tc.created = G->cb->GetCurrentFrame();
	tc.unit = uid;
	if(G->OrderRouter->GiveOrder(tc,true)==false){
		return false; // GiveOrder returned an error
	}else{
		G->Ch->dgunning.insert(uid); 
		// Command successful, This unit is now dgunning, so add it to this container so that next time it
		// dguns we can prevent it from doing so because it's already dgunning
		return true;
	}
}
bool CActions::Reclaim(int uid,int enemy){
	NLOG("CActions::Reclaim");
	TCommand TCS(tc,"cmd_recl CActions::Reclaim");
	tc.ID(CMD_RECLAIM);
	tc.Push(enemy);
	tc.created = G->cb->GetCurrentFrame();
	tc.unit = uid;
	return G->OrderRouter->GiveOrder(tc,true);
}

bool CActions::DGunNearby(int uid){
	NLOG("CActions::DGunNearby");
	
	if(G->Ch->dgunning.find(uid) != G->Ch->dgunning.end()){
		G->L.print("Dgunning failed, dgunning.find() != dgunning.end()");
		// This unit is ALREADY dgunning, and is in the middle of doing so, without this check the commander would get confused and
		// not dgun anything because it'd be switching between different targets to quickly
		return false;
	}
	const UnitDef* ud = G->GetUnitDef(uid);
	if(ud == 0 ){
		G->L.print("Dgunning failed, ud == 0");
		return false;
	}
	if(G->CanDGun(uid)==false){
		G->L.print("Dgunning failed, canDGun == false");
		return false;
	}
	
	int* en = new int[5000];
	int e = G->GetEnemyUnits(en,G->GetUnitPos(uid),G->cb->GetUnitMaxRange(uid)*1.3f); // get all enemy units within weapons range atm
	if(e>0){
		float best_score = 0;
		int best_target = 0;
		float tempscore = 0;
		float3 compos = G->GetUnitPos(uid); // get units position
		if(G->Map->CheckFloat3(compos)==false) return false;
		bool  capture=ud->canCapture;
		bool commander = false;
		for(int i = 0; i < e; i++){
			if(en[i] < 1) continue;
			const UnitDef* endi = G->GetEnemyDef(en[i]);
			if(endi == 0){
				continue;
			}else{
				//if(endi->isCommander == true) continue; // no dgunning enemy commanders!
				// no dgunning enemy commanders is commented out because now if the enemy is a commander the CMD_DGUN is
				// changed to CMD_RECLAIM, allowing the enemy commander to be eliminated without causing a wild goose chase of
				// building dgunned building dgunned, commander in the way of building dgunned, BANG, both commanders bye bye
				if(endi->canfly == true) continue; // attempting to dgun an aircraft without predictive dgunning leads to a goose chase
				tempscore = G->GetTargettingWeight(ud->name,endi->name);
				float3 enpos = G->GetUnitPos(en[i]);
				if(G->Map->CheckFloat3(enpos)) continue;
				if(endi->weapons.empty() == false){
					capture = false;
				}
				if(endi->canKamikaze == true){
					capture = false;
				}
				tempscore = tempscore / compos.distance2D(enpos);
				if(tempscore > best_score){
					best_score = tempscore;
					best_target = en[i];
					commander = endi->isCommander;
				}
				tempscore = 0;
			}
		}
		delete [] en;
		if(best_target < 1){
			G->L.print("Dgunning failed, best_target < 1");
			return false;
		}
		if(commander==true){
			if(G->cb->GetUnitHealth(uid)/ud->buildSpeed > G->cb->GetUnitHealth(best_target)/ud->buildSpeed){
				return Reclaim(uid,best_target);
			}else{
				return Retreat(uid);
			}
		}else{
			if(capture == false){
				return DGun(uid,best_target);
			}else{
				return Capture(uid,best_target);
			}
		}
		//G->Manufacturer->UnitDamaged(*aa,99,99,UpVector,true);*/
	}
	delete [] en;
	return false;
}


bool CActions::RepairNearby(int uid,float radius){
	NLOG("CActions::RepairNearby");
	const UnitDef* udi = G->cb->GetUnitDef(uid);
	if(udi == 0) return false;
	float3 pos = G->GetUnitPos(uid);
	int* hn = new int[5000];
	int h = G->cb->GetFriendlyUnits(hn,pos,radius);
	if( h>0){
		for( int i = 0; i<h; i++){
			if(((G->cb->UnitBeingBuilt(hn[i]) == true)||(G->cb->GetUnitMaxHealth(hn[i])*0.6f > G->cb->GetUnitHealth(hn[i])))&&(G->cb->GetUnitTeam(hn[i]) == G->Cached->team)){
				const UnitDef* bud = G->GetUnitDef(hn[i]);
				float Remainder = 1.0f - G->cb->GetUnitHealth(hn[i]) / G->cb->GetUnitMaxHealth(hn[i]);
				float RemainingTime = bud->buildTime / udi->buildSpeed * Remainder;
				if (RemainingTime < 30.0f) continue;
				float EPerS = bud->energyCost / (bud->buildTime / udi->buildSpeed);
				float MPerS = bud->metalCost / (bud->buildTime / udi->buildSpeed);
				if (	G->cb->GetMetal() + (G->Pl->GetMetalIncome() - MPerS) * RemainingTime > 0	&& G->cb->GetEnergy() + (G->Pl->GetEnergyIncome() - EPerS) * RemainingTime > 0	){
					if (RemainingTime > 120.0f || bud->buildSpeed > 0)	{
						TCommand TCS(tc,"Task::execute :: repair cammand unfinished units");
						tc.unit = uid;
						tc.clear = false;
						tc.delay = 0;
						tc.Priority = tc_pseudoinstant;
						tc.ID(CMD_REPAIR);
						tc.Push(hn[i]);
						delete [] hn;
						return G->OrderRouter->GiveOrder(tc);
					}
				}
			}
		}
	}else{
		h = G->cb->GetFriendlyUnits(hn);
		if(h <1) return false;
		for( int i = 0; i<h; i++){
			if(((G->cb->UnitBeingBuilt(hn[i]) == true)||(G->cb->GetUnitMaxHealth(hn[i])*0.6f > G->cb->GetUnitHealth(hn[i])))&&(G->cb->GetUnitTeam(hn[i]) == G->Cached->team)){
				const UnitDef* bud = G->GetUnitDef(hn[i]);
				float Remainder = 1.0f - G->cb->GetUnitHealth(hn[i]) / G->cb->GetUnitMaxHealth(hn[i]);
				float RemainingTime = bud->buildTime / udi->buildSpeed * Remainder;
				if (RemainingTime < 30.0f) continue;
				float EPerS = bud->energyCost / (bud->buildTime / udi->buildSpeed);
				float MPerS = bud->metalCost / (bud->buildTime / udi->buildSpeed);
				if (	G->cb->GetMetal() + (G->Pl->GetMetalIncome() - MPerS) * RemainingTime > 0	&& G->cb->GetEnergy() + (G->Pl->GetEnergyIncome() - EPerS) * RemainingTime > 0	){
					if (RemainingTime > 120.0f || bud->buildSpeed > 0)	{
						TCommand TCS(tc,"Task::execute :: repair cammand damaged units");
						tc.unit = uid;
						tc.clear = false;
						tc.Priority = tc_pseudoinstant;
						tc.ID(CMD_REPAIR);
						tc.Push(hn[i]);
						delete [] hn;
						return G->OrderRouter->GiveOrder(tc);
					}
				}
			}
		}
	}
	delete [] hn;
	return false;
}

bool CActions::RandomSpiral(int unit,bool water){
	NLOG("CActions::RandomSpiral");
	G->L.print("issuing random move");
	const UnitDef* ud = G->GetUnitDef(unit);
	if(ud == 0) return false;
	float3 upos = G->GetUnitPos(unit);
	if(G->Map->CheckFloat3(upos) == false){
		G->L.print("issuing random move failed (upos given as UpVector)");
		return false;
	}else{
		float angle = float(rand()%320);
		float3 epos = upos;
		epos.x -= ud->losRadius*1.5f;
		upos = G->Map->Rotate(epos,angle,upos);
		if(upos.x > G->cb->GetMapWidth() * SQUARE_SIZE){
			upos.x = (G->cb->GetMapWidth() * SQUARE_SIZE)-(upos.x-(G->cb->GetMapWidth() * SQUARE_SIZE));
			upos.x -= 500;
		}
		if(upos.z > G->cb->GetMapHeight() * SQUARE_SIZE){
			upos.z = (G->cb->GetMapHeight() * SQUARE_SIZE)-(upos.z-(G->cb->GetMapHeight() * SQUARE_SIZE));
			upos.z -= 500;
		}
		if(upos.x < 0){
			upos.x *= (-1);
			upos.x += 500;
		}
		if(upos.z <0){
			upos.z *= (-1);
			upos.z += 500;
		}
		return Move(unit,upos);
	}
}
bool CActions::SeekOutLastAssault(int unit){
	NLOG("CActions::SeekOutLastAssault");
	if(G->Map->CheckFloat3(last_attack) ==false){
		return false;
	}else{
		return Move(unit,last_attack);
	}
}
bool CActions::SeekOutNearestInterest(int unit){
	NLOG("CActions::SeekOutNearestInterest");
	if(unit < 1) return false;
	if(points.empty() == true) return false;
	float3 pos = G->GetUnitPos(unit);
	if(G->Map->CheckFloat3(pos) == false) return false;
	if(points.empty() == false){
		float3 destination = UpVector;
		float shortest = 2000000.0f;
		for(vector<float3>::iterator i = points.begin(); i != points.end(); ++i){
			if(pos.distance2D(*i) < shortest){
				destination = *i;
			}
		}
		if(G->Map->CheckFloat3(destination)==true){
			return Move(unit,destination);
		}
	}
	return false;
}
bool CActions::SeekOutInterest(int unit, float range){
	NLOG("CActions::SeekOutInterest");
	if(unit < 1) return false;
	if(range < 200.0f) return false; // cant have a range that'll get us absolutely no results...
	if(points.empty() == true) return false;
	float3 pos = G->GetUnitPos(unit);
	if(G->Map->CheckFloat3(pos) == false) return false;
	vector<float3> in_range;
	for(vector<float3>::iterator i = points.begin(); i != points.end(); ++i){
		if(pos.distance2D(*i) < range){
			in_range.push_back(*i);
		}
	}
	if(in_range.empty() == false){
		//
		float3 destination = UpVector;
		float shortest = 2000000.0f;
		for(vector<float3>::iterator i = in_range.begin(); i != in_range.end(); ++i){
			if(pos.distance2D(*i) < shortest){
				destination = *i;
			}
		}
		if(G->Map->CheckFloat3(destination) == false){
			return false;
		}
		return Move(unit,destination);
	}else{
		return false;
	}
} 

bool CActions::AddPoint(float3 pos){
	NLOG("CActions::AddPoint");
	if(G->Map->CheckFloat3(pos) == false) return false;
	points.push_back(pos);
	return true;
}

void CActions::Update(){
	NLOG("CActions::Update");
	if(EVERY_((15 FRAMES))){
		if(points.empty()==false){
			for(vector<float3>::iterator i = points.begin(); i!=points.end(); ++i){
				if(G->InLOS(*i)==true){
					points.erase(i);
					break;
				}
			}
		}
	}
	if(G->GetCurrentFrame()== (20 SECONDS)){
		if(G->M->m->NumSpotsFound > 0){
			vector<float3>* spots = G->M->getHotSpots();
			if(spots!=0){
				if(spots->empty() == false){
					for(vector<float3>::iterator i = spots->begin(); i != spots->end(); ++i){
						float3 q = *i;
						q.y=G->cb->GetElevation(q.x,q.z);
						AddPoint(q);
					}
				}
			}
		}
	}
}

bool CActions::Retreat(int uid){
	float3 pos = G->GetUnitPos(uid);
	if(G->Map->CheckFloat3(pos)==true){
		float3 bpos =G->Map->nbasepos(pos);
		if(bpos.distance2D(pos) < 900){
			return false;
		}
		pos = G->Map->distfrom(pos,bpos,900);
		return Move(uid,pos);
	}else{
		return false;
	}
}

bool CActions::CopyAttack(int unit,set<int> tocopy){
	float3 p = G->GetUnitPos(unit);
	if(G->Map->CheckFloat3(p)==false){
		return false;
	}
	int* a = new int[10000];
	int n = G->cb->GetFriendlyUnits(a,p,900.0f);
	if(n > 1){
		//
		for(int i =0; i < n; i++){
			if(i == unit) continue;
			if(tocopy.find(i) == tocopy.end()) continue;
			const deque<Command>* vc = G->cb->GetCurrentUnitCommands(i);
			if(vc == 0) continue;
			if(vc->empty()) continue;
			for(deque<Command>::const_iterator q = vc->begin(); q != vc->end(); ++q){
				//
				if(q->id == CMD_ATTACK){
					delete [] a;
					int eu = (int)q->params.front();
					return Attack(unit,eu,false);
				}
			}
		}
	}
	delete [] a;
	return false;
}
bool CActions::CopyMove(int unit,set<int> tocopy){
	float3 p = G->GetUnitPos(unit);
	if(G->Map->CheckFloat3(p)==false){
		return false;
	}
	 int* a = new int[10000];
	int n = G->cb->GetFriendlyUnits(a,p,1300.0f);
	if(n > 1){
		//
		for(int i =0; i < n; i++){
			if(i == unit) continue;
			if(tocopy.find(i) == tocopy.end()) continue;
			
			const deque<Command>* vc = G->cb->GetCurrentUnitCommands(i);
			if(vc == 0) continue;
			if(vc->empty()) continue;
			for(deque<Command>::const_iterator q = vc->begin(); q != vc->end(); ++q){
				if(q->id == CMD_MOVE){
					float3 rpos = float3(q->params.at(0),q->params.at(1),q->params.at(2));
					if(rpos.distance2D(p) < 200.0f) continue;
					delete [] a;
					return Move(unit,rpos);
				}
			}
		}
	}
	delete [] a;
	return false;
}


//
