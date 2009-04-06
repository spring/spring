// Actions
#include "../../Core/include.h"

using namespace ntai;

CActions::CActions(Global* GL){
    G = GL;
    last_attack = UpVector;
	temp = new int[10000];
}

CActions::~CActions(){
	delete[] temp;
}

bool CActions::Attack(int uid, float3 pos){
    NLOG("CActions::Attack Position");
	
	if(!ValidUnitID(uid)){
		return false;
	}
	
	if(G->Map->CheckFloat3(pos)==false){
		return false;
	}

    TCommand tc(uid, "Attack Pos CActions");
    tc.ID(CMD_ATTACK);
    tc.PushFloat3(pos);
    tc.created = G->cb->GetCurrentFrame();
    
	return G->OrderRouter->GiveOrder(tc, true);
}

bool CActions::Trajectory(int unit, int traj){
    NLOG("CActions::Trajectory");

    TCommand tc(unit, "Trajectory CAction");
    tc.c.timeOut = 400+(G->cb->GetCurrentFrame()%600) + G->cb->GetCurrentFrame();
    tc.Priority = tc_low;
    tc.ID(CMD_TRAJECTORY);
    tc.Push(traj);

    return G->OrderRouter->GiveOrder(tc);
}

void CActions::UnitDamaged(int damaged, int attacker, float damage, float3 dir){
    NLOG("CActions::UnitDamaged");

    float3 pos = G->GetUnitPos(attacker);

    if(G->Map->CheckFloat3(pos)==true){
        last_attack=pos;
    }else{
        pos = G->GetUnitPos(damaged);

        if(G->Map->CheckFloat3(pos)){
            last_attack=pos;
        }
    }
}

bool CActions::Attack(int unit, int target, bool mobile){
    NLOG("CActions::Attack Target");

	if(!ValidUnitID(unit)){
		return false;
	}

	if(!ValidUnitID(target)){
		return false;
	}

    float3 pos = G->GetUnitPos(target);

    if(G->Map->CheckFloat3(pos)){
        last_attack=pos;
    }else{
        pos = G->GetUnitPos(unit);
        if(G->Map->CheckFloat3(pos)){
            last_attack=pos;
        }
    }

    TCommand tc(unit, "attack target CActions");
    tc.ID(CMD_ATTACK);
    tc.Push(target);
    tc.created = G->cb->GetCurrentFrame();
    return G->OrderRouter->GiveOrder(tc, true);
}

bool CActions::Capture(int uid, int enemy){
    NLOG("CActions::Capture");

	if(!ValidUnitID(uid)){
		return false;
	}

	if(!ValidUnitID(enemy)){
		return false;
	}

    TCommand tc(uid, "capture CActions");
    tc.ID(CMD_CAPTURE);
    tc.PushUnit(enemy);
    tc.created = G->GetCurrentFrame();

    return G->OrderRouter->GiveOrder(tc, true);
}

bool CActions::Repair(int uid, int unit){
    NLOG("CActions::Repair");

	if(!ValidUnitID(uid)){
		return false;
	}

	if(!ValidUnitID(unit)){
		return false;
	}

    const UnitDef* udi = G->GetUnitDef(uid);
    if(udi == 0) return false;

    const UnitDef* bud = G->GetUnitDef(unit);
    if(bud == 0) return false;

    if(!udi->builder) return false;

    float uhealth, umhealth;
    uhealth = G->cb->GetUnitHealth(unit);
    umhealth = G->cb->GetUnitMaxHealth(unit);

    if(umhealth == 0) return false;

    float Remainder = 1.0f - uhealth / umhealth;
    float RemainingTime = udi->buildTime / max(bud->buildSpeed, 0.1f) * Remainder;

    if (RemainingTime < 30.0f) return false;

    float EPerS = bud->energyCost / (bud->buildTime / udi->buildSpeed);
    float MPerS = bud->metalCost / (bud->buildTime / udi->buildSpeed);

    if ( G->cb->GetMetal() + (G->Pl->GetMetalIncome() - MPerS) * RemainingTime > 0	&& G->cb->GetEnergy() + (G->Pl->GetEnergyIncome() - EPerS) * RemainingTime > 0	){
        if (RemainingTime > 120.0f || bud->buildSpeed > 0)	{
            TCommand tc(uid, "repair CActions");
            tc.ID(CMD_REPAIR);
            tc.PushUnit(unit);
            tc.created = G->cb->GetCurrentFrame();

            if(G->OrderRouter->GiveOrder(tc, true)){
                if(G->cb->UnitBeingBuilt(unit)){

                    // add this builder to the appropriate plan
                    if(!G->Manufacturer->BPlans->empty()){
                        for(deque<CBPlan* >::iterator i = G->Manufacturer->BPlans->begin(); i != G->Manufacturer->BPlans->end(); ++i){
                            if((*i)->subject == unit){
								(*i)->AddBuilder(uid);
                                break;
                            }
                        }
                    }

                }
                return true;
            }else{
                return false;
            }
        }else{
            return false;
        }
    }else{
        return false;
    }
}

bool CActions::MoveToStrike(int unit, float3 pos, bool overwrite){
    NLOG("CActions::MoveToStrike");

    if(!ValidUnitID(unit)){
		return false;
	}

    if(G->Map->CheckFloat3(pos)==false) return false;
    float3 npos = G->Map->distfrom(G->GetUnitPos(unit), pos, G->cb->GetUnitMaxRange(unit)*0.95f);
    npos.y = 0;
    return Move(unit, npos, overwrite);
}

bool CActions::Move(int unit, float3 pos, bool overwrite){
    NLOG("CActions::Move");

    if(!ValidUnitID(unit)){
		return false;
	}

    if(G->Map->CheckFloat3(pos)==false) return false;
    TCommand tc(unit, "move CActions");
    tc.ID(CMD_MOVE);
    tc.PushFloat3(pos);
    tc.created = G->cb->GetCurrentFrame();
    return G->OrderRouter->GiveOrder(tc, overwrite);
}

bool CActions::Guard(int unit, int guarded){
    NLOG("CActions::Guard");

    if(!ValidUnitID(unit)){
		return false;
	}

    if(!ValidUnitID(guarded)){
		return false;
	}

    TCommand tc(unit, "Guard CActions");
    tc.ID(CMD_GUARD);
    tc.Push(guarded);
    tc.created = G->cb->GetCurrentFrame();

    return G->OrderRouter->GiveOrder(tc, true);
}

bool CActions::ReclaimNearby(int uid, float radius){
    NLOG("CActions::ReclaimNearby");
	
	if(!ValidUnitID(uid)){
		return false;
	}
    
	float3 upos = G->GetUnitPos(uid);

    if(G->Map->CheckFloat3(upos)){
        return AreaReclaim(uid, upos, (int)radius);
    }
    return false;
}

bool CActions::RessurectNearby(int uid){
    NLOG("CActions::RessurectNearby");
    float3 upos = G->GetUnitPos(uid);
    if(G->Map->CheckFloat3(upos)){
        int* f = new int[1000];
        int fc = G->cb->GetFeatures(f, 99, upos, 700);
        delete [] f;
        if(fc >0){
            TCommand tc(uid, "ressurect nearby CActions");
            tc.ID(CMD_RESURRECT);
            tc.PushFloat3(upos);
            tc.Push(700);
            tc.created = G->cb->GetCurrentFrame();
            return G->OrderRouter->GiveOrder(tc, true);
        }else{
            return false;
        }
    }
    return false;
}
bool CActions::AttackNearest(int unit){ // Attack nearby enemies...
    NLOG("CActions::AttackNearest");
    /*const UnitDef* ud = G->GetUnitDef(unit);
     if(ud == 0) return false;
     //if(G->positions.empty()==false){
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
     }*/
    //}
    return false;
}
bool CActions::AttackNear(int unit, float LOSmultiplier){
    NLOG("CActions::AttackNear");

    const UnitDef* ud = G->GetUnitDef(unit);
    if(ud == 0) return false;

	CUnitTypeData* utd = G->UnitDefLoader->GetUnitTypeDataById(ud->id);

    int* en = new int[10000];
    int e = G->GetEnemyUnits(en, G->GetUnitPos(unit), max(G->cb->GetUnitMaxRange(unit), ud->losRadius)*LOSmultiplier);

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
				CUnitTypeData* etd = G->UnitDefLoader->GetUnitTypeDataById(endi->id);

                bool tmobile = false;

				tempscore = G->efficiency->GetEfficiency(endi->name);

                //tempscore = G->Ch->ApplyGrudge(en[i], tempscore);

                if(etd->IsMobile()){
					if(!G->info->hardtarget){
						tempscore = tempscore/4;
					}
                    tmobile = true;
                }else if(!endi->weapons.empty()){
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
            Trajectory(unit, (int)mobile);
        }

        if(best_target > 0){
            delete [] en;
            return Attack(unit, best_target, mobile);
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
    if(ValidUnitID(unit)){
        TCommand tc(unit, "schedule idle unit");
        tc.created = G->cb->GetCurrentFrame();
        tc.ID(CMD_IDLE);
        if(G->OrderRouter->GiveOrder(tc, false)== false){
            G->L.print("GiveOrder on scedule idle failed!!");
        }
    }
}
bool CActions::DGun(int uid, int enemy){
    NLOG("CActions::DGun");

    TCommand tc(uid, "cmd_attack chaser::update every 4 secs");
    tc.ID(CMD_DGUN);
    tc.Push(enemy);
    tc.c.timeOut = 3 SECONDS+(G->cb->GetCurrentFrame()%300) + G->cb->GetCurrentFrame(); // dont let this command get in the way of the commanders taskqueue with a never ending vendetta that can never be fulfilled

    tc.created = G->cb->GetCurrentFrame();
    if(G->OrderRouter->GiveOrder(tc, true)==false){
		// GiveOrder returned an error
        return false; 
    }else{
        // Command successful, This unit is now dgunning
        return true;
    }
}
bool CActions::Reclaim(int uid, int enemy){
    NLOG("CActions::Reclaim");
    TCommand tc(uid, "cmd_recl CActions::Reclaim");
    tc.ID(CMD_RECLAIM);
    tc.Push(enemy);
    tc.created = G->cb->GetCurrentFrame();
    return G->OrderRouter->GiveOrder(tc, true);
}

bool CActions::AreaReclaim(int uid, float3 pos, int radius){
    NLOG("CActions::AreaReclaim");
    TCommand tc(uid, "cmd_recl CActions::AreaReclaim");
    tc.ID(CMD_RECLAIM);
    tc.PushFloat3(pos);
    tc.Push(radius);
    tc.created = G->cb->GetCurrentFrame();
    return G->OrderRouter->GiveOrder(tc, true);
}

bool CActions::DGunNearby(int uid){
    NLOG("CActions::DGunNearby");
	
	CUnitTypeData* utd = G->UnitDefLoader->GetUnitTypeDataByUnitId(uid);
    
	if(utd == 0 ){
        G->L.print("Dgunning failed, utd == 0");
        return false;
    }
	
	if(!utd->CanDGun()){
        return false;
    }

    float3 compos = G->GetUnitPos(uid);
    if(G->Map->CheckFloat3(compos)==false){
        return false;
    }
    
	int* en = new int[10000];

    int e = G->GetEnemyUnits(en, compos, G->cb->GetUnitMaxRange(uid)*1.3f); // get all enemy units within weapons range atm
    
	if(e>0){
        for(int i = 0; i < e; i++){
            
			if(ValidUnitID(en[i])==false){
				continue;
			}

			CUnitTypeData* edt = G->UnitDefLoader->GetUnitTypeDataByUnitId(en[i]);

            //if(endi->isCommander == true) continue; // no dgunning enemy commanders!

            // no dgunning enemy commanders is commented out because now if the enemy is a commander the CMD_DGUN is
            // changed to CMD_RECLAIM, allowing the enemy commander to be eliminated without causing a wild goose chase of
            // building dgunned building dgunned, commander in the way of building dgunned, BANG, both commanders bye bye
			// that and the line needs fiddling anyway due to the CUnitTypeData refactor

			if(edt->IsAirCraft()&&(edt->GetUnitDef()->speed<3)){
				continue;// attempting to dgun an aircraft without predictive dgunning leads to a goose chase
			}

            int k = en[i];
            delete [] en;

            if(edt->GetUnitDef()->canDGun){

                int r = (int)G->Pl->ReclaimTime(utd->GetUnitDef(), edt->GetUnitDef(), G->chcb->GetUnitHealth(k));
                int c = (int)G->Pl->CaptureTime(utd->GetUnitDef(), edt->GetUnitDef(), G->chcb->GetUnitHealth(k));

                if (r<(4 SECONDS) && r < c){
                    return Reclaim(uid, k);
                } else {

                    if (c<(4 SECONDS)){
                        return Capture(uid, k);
                    } else {
                        NLOG("CActions::IfNobodyNearMoveToNearest :: WipePlansForBuilder");
                        return MoveToStrike(uid, G->Map->nbasepos(compos), false);
                    }
                }

            } else {

                if (utd->GetDGunCost()<G->cb->GetEnergy()){
                    return DGun(uid, k);
                } else {

                    int r = (int)G->Pl->ReclaimTime(utd->GetUnitDef(), edt->GetUnitDef(), G->chcb->GetUnitHealth(k));
                    int c = (int)G->Pl->CaptureTime(utd->GetUnitDef(), edt->GetUnitDef(), G->chcb->GetUnitHealth(k));

                    if(r<(4 SECONDS) && r < c){
                        return Reclaim(uid, k);
                    } else {
                        if (c<(4 SECONDS)){
							return Capture(uid, k);
						} else {
							return MoveToStrike(uid, G->Map->nbasepos(compos), false);
                        }

                    }

                }
            }
        }
    }else{

        // we aint got enemies within immediate range, however this doesnt mean we're safe.
        // check fi there are nearby enemy groups
        // check the surrounding units and repair them if necessary.
        // if none need repairing then retreat, this may be an enemy base or an incoming army

        e = G->GetEnemyUnits(en, compos, G->cb->GetUnitMaxRange(uid)*2.5f); // get all enemy units within weapons range atm

        if(e > 0){
            float total_enemy_power = 0;
            float total_allied_power = 0;

            for(int i = 0; i < e; i++){
				if(!ValidUnitID(en[i])){
					continue;
				}

                const UnitDef* endi = G->GetUnitDef(en[i]);

                if(endi){
                    total_enemy_power += endi->power;
                }
            }

            e = G->cb->GetFriendlyUnits(en, compos, G->cb->GetUnitMaxRange(uid)*2.5f);

            for(int i = 0; i < e; i++){
				if(!ValidUnitID(en[i])){
					continue; // filter out bad unit id's
				}

                const UnitDef* endi = G->GetUnitDef(en[i]);
                if(endi){
                    total_allied_power += endi->power;
                }
            }

            float ratio = total_allied_power/total_enemy_power;

            if((ratio<0.9f)&&(ratio > 0.4f)){
                // repair nearby units
                int r_uid = -1;
                float best_r = 0;

                e = G->cb->GetFriendlyUnits(en, compos, G->cb->GetUnitMaxRange(uid)*2.5f);

                for(int i = 0; i < e; i++){
					if(!ValidUnitID(en[i])){
						continue; // filter out bad unit id's
					}

					if(en[i] == uid){
						continue; // we cant repair ourself!!
					}
                    
					const UnitDef* endi = G->GetUnitDef(en[i]);
                    if(endi){
                        float ratio = G->cb->GetUnitHealth(en[i])/G->cb->GetUnitMaxHealth(en[i]);
                        ratio = endi->power*ratio;

                        if( ratio > best_r){
                            best_r = ratio;
                            r_uid = en[i];
                        }
                    }
                }

                delete[] en;

                if(ValidUnitID(r_uid)){
                    return Repair(uid, r_uid);
                }

            }else{
                delete[] en;

                // runaway!!!!!!!!!!!!!!!!!!
				return MoveToStrike(uid, G->Map->nbasepos(compos), true);
            }
        }
    }

    delete [] en;
    return false;
}

bool CActions::OffensiveRepairRetreat(int uid, float radius){
    NLOG("CActions::OffensiveRepairRetreat");

    const UnitDef* ud = G->cb->GetUnitDef(uid);
    if(ud == 0 ){
        G->L.print("OffensiveRepairRetreat: ud == 0");
        return false;
    }

    float3 compos = G->GetUnitPos(uid);
    if(G->Map->CheckFloat3(compos)==false){
        return false;
    }

    int* en = new int[10000];
    int e = G->GetEnemyUnits(en, compos, radius); // get all enemy units within weapons range atm

    if(e>0){
        float total_enemy_power = 0;
        float total_allied_power = 0;

        for(int i = 0; i < e; i++){
            if(::ValidUnitID(en[i])==false) continue;
            const UnitDef* endi = G->GetUnitDef(en[i]);
            if(endi){
                total_enemy_power += endi->power;
            }
        }

        e = G->cb->GetFriendlyUnits(en, compos, G->cb->GetUnitMaxRange(uid)*2.5f);

        for(int i = 0; i < e; i++){
            if(ValidUnitID(en[i])==false) continue; // filter out bad unit id's
            const UnitDef* endi = G->GetUnitDef(en[i]);
            if(endi){
                total_allied_power += endi->power;
            }
        }

        float ratio = total_allied_power/total_enemy_power;

        if((ratio<0.9f)&&(ratio > 0.4f)){
            // repair nearby units
            int r_uid = -1;
            float best_r = 0;
            e = G->cb->GetFriendlyUnits(en, compos, G->cb->GetUnitMaxRange(uid)*2.5f);

            for(int i = 0; i < e; i++){
                if(ValidUnitID(en[i])==false) continue; // filter out bad unit id's
                if(en[i] == uid) continue; // we cant repair ourself!!
                const UnitDef* endi = G->GetUnitDef(en[i]);
                if(endi){
                    float ratio = G->cb->GetUnitHealth(en[i])/G->cb->GetUnitMaxHealth(en[i]);
                    ratio = endi->power*ratio;
                    if( ratio > best_r){
                        best_r = ratio;
                        r_uid = en[i];
                    }
                }
            }

            delete[] en;

            if(ValidUnitID(r_uid)){
                return Repair(uid, r_uid);
            }

        }else{
            delete[] en;

            // runaway!!!!!!!!!!!!!!!!!!
			return MoveToStrike(uid, G->Map->nbasepos(compos), false);
        }
    }

    delete [] en;
    return false;
}
bool CActions::RepairNearby(int uid, float radius){
    NLOG("CActions::RepairNearby");
    const UnitDef* udi = G->cb->GetUnitDef(uid);
	if(udi == 0){
		return false;
	}

    float3 pos = G->GetUnitPos(uid);

    int* hn = new int[10000];

    int h = G->cb->GetFriendlyUnits(hn, pos, radius);

    if(h>0){

        for( int i = 0; i<h; i++){
            if(((G->cb->UnitBeingBuilt(hn[i]) == true)||(G->cb->GetUnitMaxHealth(hn[i])*0.6f > G->cb->GetUnitHealth(hn[i])))&&(G->cb->GetUnitTeam(hn[i]) == G->Cached->team)){
                const UnitDef* bud = G->GetUnitDef(hn[i]);
                float Remainder = 1.0f - G->cb->GetUnitHealth(hn[i]) / G->cb->GetUnitMaxHealth(hn[i]);
                float RemainingTime = bud->buildTime / udi->buildSpeed * Remainder;

				if (RemainingTime < 30.0f){
					continue;
				}

                float EPerS = bud->energyCost / (bud->buildTime / udi->buildSpeed);
                float MPerS = bud->metalCost / (bud->buildTime / udi->buildSpeed);

                if (G->cb->GetMetal() + (G->Pl->GetMetalIncome() - MPerS) * RemainingTime > 0	&& G->cb->GetEnergy() + (G->Pl->GetEnergyIncome() - EPerS) * RemainingTime > 0	){
                    if (RemainingTime > 120.0f || bud->buildSpeed > 0){
                        int target = hn[i];

                        delete [] hn;

                        return Repair(uid, target);
                    }
                }
            }
        }
    }else{
        h = G->cb->GetFriendlyUnits(hn);

		if(h <1){
			return false;
		}

        for( int i = 0; i<h; i++){
            if(((G->cb->UnitBeingBuilt(hn[i]) == true)||(G->cb->GetUnitMaxHealth(hn[i])*0.6f > G->cb->GetUnitHealth(hn[i])))&&(G->cb->GetUnitTeam(hn[i]) == G->Cached->team)){
                const UnitDef* bud = G->GetUnitDef(hn[i]);

                float Remainder = 1.0f - G->cb->GetUnitHealth(hn[i]) / G->cb->GetUnitMaxHealth(hn[i]);
                float RemainingTime = bud->buildTime / udi->buildSpeed * Remainder;

				if (RemainingTime < 30.0f){
					continue;
				}

                float EPerS = bud->energyCost / (bud->buildTime / udi->buildSpeed);
                float MPerS = bud->metalCost / (bud->buildTime / udi->buildSpeed);

                if (G->cb->GetMetal() + (G->Pl->GetMetalIncome() - MPerS) * RemainingTime > 0	&& G->cb->GetEnergy() + (G->Pl->GetEnergyIncome() - EPerS) * RemainingTime > 0	){
                    if (RemainingTime > 120.0f || bud->buildSpeed > 0)	{
                        int target = hn[i];
                        delete [] hn;
                        return Repair(uid, target);
                    }
                }
            }
        }
    }

    delete [] hn;
    return false;
}

bool CActions::RepairNearbyUnfinishedMobileUnits(int uid, float radius){
    NLOG("CActions::RepairNearbyUnfinishedMobileUnits");
    const UnitDef* udi = G->cb->GetUnitDef(uid);
    if(udi == 0) return false;
    float3 pos = G->GetUnitPos(uid);
    int* hn = new int[10000];
    int h = G->cb->GetFriendlyUnits(hn, pos, radius);
    if( h>0){
        for( int i = 0; i<h; i++){

            if(G->cb->UnitBeingBuilt(hn[i]) == true){
				CUnitTypeData* btd = G->UnitDefLoader->GetUnitTypeDataByUnitId(hn[i]);

				if(!btd->IsMobile()){
					continue;
				}

                float Remainder = 1.0f - G->cb->GetUnitHealth(hn[i]) / G->cb->GetUnitMaxHealth(hn[i]);
                float RemainingTime = btd->GetUnitDef()->buildTime / udi->buildSpeed * Remainder;

				if (RemainingTime < 30.0f){
					continue;
				}

				float EPerS = btd->GetUnitDef()->energyCost / (btd->GetUnitDef()->buildTime / udi->buildSpeed);
                float MPerS = btd->GetUnitDef()->metalCost / (btd->GetUnitDef()->buildTime / udi->buildSpeed);

                if (G->cb->GetMetal() + (G->Pl->GetMetalIncome() - MPerS) * RemainingTime > 0 && G->cb->GetEnergy() + (G->Pl->GetEnergyIncome() - EPerS) * RemainingTime > 0){
					
					if (RemainingTime > 120.0f || btd->GetUnitDef()->buildSpeed > 0){
                        int target = hn[i];
                        delete [] hn;
                        return Repair(uid, target);
                    }

                }

            }
        }

    }else{
        h = G->cb->GetFriendlyUnits(hn);

		if(h <1){
			return false;
		}

        for( int i = 0; i<h; i++){

            if((G->cb->UnitBeingBuilt(hn[i])||(G->cb->GetUnitMaxHealth(hn[i])*0.6f > G->cb->GetUnitHealth(hn[i])))&&(G->cb->GetUnitTeam(hn[i]) == G->Cached->team)){
                CUnitTypeData* btd = G->UnitDefLoader->GetUnitTypeDataByUnitId(hn[i]);
                
				float Remainder = 1.0f - G->cb->GetUnitHealth(hn[i]) / G->cb->GetUnitMaxHealth(hn[i]);
                float RemainingTime = btd->GetUnitDef()->buildTime / udi->buildSpeed * Remainder;
				
				if (RemainingTime < 30.0f){
					continue;
				}

                float EPerS = btd->GetUnitDef()->energyCost / (btd->GetUnitDef()->buildTime / udi->buildSpeed);
                float MPerS = btd->GetUnitDef()->metalCost / (btd->GetUnitDef()->buildTime / udi->buildSpeed);

                if (G->cb->GetMetal() + (G->Pl->GetMetalIncome() - MPerS) * RemainingTime > 0	&& G->cb->GetEnergy() + (G->Pl->GetEnergyIncome() - EPerS) * RemainingTime > 0	){
                    if (RemainingTime > 120.0f || btd->GetUnitDef()->buildSpeed > 0)	{
                        int target = hn[i];
                        delete [] hn;
                        return Repair(uid, target);
                    }
                }

            }
        }
    }
    delete [] hn;
    return false;
}

bool CActions::RandomSpiral(int unit, bool water){
    
	NLOG("CActions::RandomSpiral");
    
	G->L.print("issuing random move");
    
	const UnitDef* ud = G->GetUnitDef(unit);
	
	if(ud == 0){
		return false;
	}

    float3 upos = G->GetUnitPos(unit);

    if(G->Map->CheckFloat3(upos) == false){
		G->L.print("issuing random move failed (upos given as UpVector)");
        return false;
    } else {

        float angle = float(rand()%320);
        float3 epos = upos;
        epos.x -= ud->losRadius*1.5f;
        upos = G->Map->Rotate(epos, angle, upos);

        if(upos.x > G->cb->GetMapWidth() * SQUARE_SIZE){
            upos.x = (G->cb->GetMapWidth() * SQUARE_SIZE)-(upos.x-(G->cb->GetMapWidth() * SQUARE_SIZE));
            upos.x -= 900;
        }

        if(upos.z > G->cb->GetMapHeight() * SQUARE_SIZE){
            upos.z = (G->cb->GetMapHeight() * SQUARE_SIZE)-(upos.z-(G->cb->GetMapHeight() * SQUARE_SIZE));
            upos.z -= 900;
        }

        if(upos.x < 0){
            upos.x *= (-1);
            upos.x += 900;
        }

        if(upos.z <0){
            upos.z *= (-1);
            upos.z += 500;
        }

        return Move(unit, upos);
    }
}

bool CActions::SeekOutLastAssault(int unit){
    NLOG("CActions::SeekOutLastAssault");

    if(G->Map->CheckFloat3(last_attack) ==false){
        return false;
    }else{
        return Move(unit, last_attack);
    }
}

bool CActions::SeekOutNearestInterest(int unit){
    NLOG("CActions::SeekOutNearestInterest");

	if(unit < 1){
		return false;
	}

	if(points.empty()){
		return false;
	}

    float3 pos = G->GetUnitPos(unit);

	if(!G->Map->CheckFloat3(pos)){
		return false;
	}

    if(!points.empty()){

        float3 destination = UpVector;
        float shortest = 2000000.0f;

        for(vector<float3>::iterator i = points.begin(); i != points.end(); ++i){
            if(pos.distance2D(*i) < shortest){
                destination = *i;
            }

        }

        if(G->Map->CheckFloat3(destination)==true){
            return Move(unit, destination);
        }

    }

    return false;
}

bool CActions::SeekOutInterest(int unit, float range){
    NLOG("CActions::SeekOutInterest");

	if(!ValidUnitID(unit)){
		return false;
	}
    
	if(range < 200.0f){
		// cant have a range that'll get us absolutely no results...
		return false;
	}
    
	if(points.empty()){
		return false;
	}
    
	float3 pos = G->GetUnitPos(unit);
	
	if(!G->Map->CheckFloat3(pos)){
		return false;
	}

    vector<float3> in_range;

    for(vector<float3>::iterator i = points.begin(); i != points.end(); ++i){
        if(pos.distance2D(*i) < range){
            in_range.push_back(*i);

        }

    }

    if(!in_range.empty()){
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

        return Move(unit, destination);
    }else{
        return false;
    }

}

bool CActions::AddPoint(float3 pos){
    NLOG("CActions::AddPoint");

	if(G->Map->CheckFloat3(pos) == false){
		return false;
	}

    points.push_back(pos);
    
	return true;
}

void CActions::Update(){
    NLOG("CActions::Update");

    if(EVERY_((16 FRAMES))){

        if(!points.empty()){
            NLOG("CActions::Update points");

            for(vector<float3>::iterator i = points.begin(); i!=points.end(); ++i){
                if(G->InLOS(*i)==true){
                    int ecount = G->GetEnemyUnits(temp, *i, 300);
                    if(ecount < 1){
                        points.erase(i);
                        break;
                    }
                }
            }


        }
    }

    NLOG("CActions::Update points done");

}

bool CActions::Retreat(int uid){
    NLOG("CActions::Retreat");

    float3 pos = G->GetUnitPos(uid);

    if(G->Map->CheckFloat3(pos)==true){
        float3 bpos =G->Map->nbasepos(pos);

        if(bpos.distance2D(pos) < 900){
            return false;
        }

        pos = G->Map->distfrom(pos, bpos, 900);
        return Move(uid, pos);
    }else{

        return false;
    }

}

bool CActions::CopyAttack(int unit, set<int> tocopy){
    NLOG("CActions::CopyAttack");
    /*float3 p = G->GetUnitPos(unit);
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
     const CCommandQueue* vc = G->cb->GetCurrentUnitCommands(i);
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
     delete [] a;*/
    return false;
}

bool CActions::CopyMove(int unit, set<int> tocopy){
    NLOG("CActions::CopyMove");
    /*float3 p = G->GetUnitPos(unit);
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
     delete [] a;*/
    return false;
}

bool CActions::IfNobodyNearMoveToNearest(int uid, set<int> units){
    NLOG("CActions::IfNobodyNearMoveToNearest");
    if(units.empty()) return false;
    float3 pos = G->GetUnitPos(uid);
    if(G->Map->CheckFloat3(pos)==false){
        return false;
    }
    int* nu = new int[2000];
    int n = G->cb->GetFriendlyUnits(nu, pos, 600.0f);
    if(n > 0){
        for(int i = 0; i < n; i++){
            if(nu[i]==uid) continue;
            if(units.find(nu[i])!=units.end()){
                delete [] nu;
                return false;
            }
        }
    }
    delete [] nu;
    float3 best=UpVector;
    float d= 900000.0f;
    for(set<int>::iterator i = units.begin(); i != units.end(); ++i){
        if(*i == uid) continue;
        float3 tpos = G->GetUnitPos(*i);
        if(G->Map->CheckFloat3(tpos)==false) continue;
        float td = tpos.distance2D(pos);
        if(td < d){
            d = td;
            best = tpos;
        }
    }
    if(G->Map->CheckFloat3(best)){
        return Move(uid, best);
    }
    return false;
}

//
