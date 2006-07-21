#include "../Core/helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Planning::InitAI(){
	NLOG("Planning::InitAI");
	NLOG("filling antistall array");
	NoAntiStall = bds::set_cont(NoAntiStall,G->Get_mod_tdf()->SGetValueMSG("AI\\NoAntiStall"));
	NLOG("filling antistall control values");
	G->Get_mod_tdf()->GetDef(a, "1.3", "AI\\STALL_MULTIPLIERS\\metalIncome");
	G->Get_mod_tdf()->GetDef(b, "1.3", "AI\\STALL_MULTIPLIERS\\metalstored");
	G->Get_mod_tdf()->GetDef(c, "1.3", "AI\\STALL_MULTIPLIERS\\energyIncome");
	G->Get_mod_tdf()->GetDef(d, "1.3", "AI\\STALL_MULTIPLIERS\\energyStored");
	NLOG("Planning::Init complete");
}

Planning::Planning(Global* GLI){
	G = GLI;
	fnum = 1;
}
void Planning::Update(){
	NLOG("Planning::Update");
	/*if(EVERY_((1 MINUTE))){
		int* fx = new int[6000];
		fnum = G->cb->GetFriendlyUnits(fx);
		delete [] fx;
	}
	if(EVERY_(10)&&(fnum < 50)){
		if(G->Ch->enemynum <1){
			return;
		}else if(G->Cached->enemies.empty() ==true){
			return;
		}
		for(set<int>::iterator i = G->Cached->enemies.begin(); i != G->Cached->enemies.end(); ++i){
			//
			if(enemy_vectors.find(*i) == enemy_vectors.end()){
				envec e;
				float3 tpos = G->GetUnitPos(*i);
				if(G->Map->CheckFloat3(tpos)==true){
					e.now = tpos;
					e.last = e.now;
					e.last_frame = G->cb->GetCurrentFrame();
					enemy_vectors[*i] = e;
				}
			}else{
				float3 pos = G->GetUnitPos(*i);
				if(G->Map->CheckFloat3(pos)==true){
					enemy_vectors[*i].last = enemy_vectors[*i].now;
					enemy_vectors[*i].now = pos;
					enemy_vectors[*i].last_frame = G->cb->GetCurrentFrame();
				}
			}
		}
	}*/
}

float Planning::GetRealValue(float value){
	return value * 0.937207123f;
}
float Planning::UndoRealValue(float value){
	return value/0.937207123f;
}
float3 Planning::GetDirVector(int enemy,float3 unit, const WeaponDef* def){
	NLOG("Planning::GetDirVector");
	const UnitDef* ed = G->GetUnitDef(enemy);
	if(ed == 0){
		return UpVector;
	}
	if(enemy < 1){
		// units cant have ID's smaller than 1! error!!!!!!
		G->L.print("Planning::GetDirVector unitID smaller than 1 given for enemy");
		return UpVector;
	}
	if(enemy_vectors.find(enemy) == enemy_vectors.end()){
		G->L.print("Planning::GetDirVector enemies vector not found!!!!");
		return UpVector;
	}else{
		float3 newpos =UpVector;
		envec e = enemy_vectors[enemy];
		//if(G->cb->GetCurrentFrame()- (3 SECONDS) > e.last_frame){
		//	G->L.print("data out of date!!!!");
		//	return UpVector;
		//}
		float weapon_velocity = (def->startvelocity+def->maxvelocity)/2;
		float3 dpos = e.now - e.last;
		dpos.x = dpos.x/10;
		dpos.z = dpos.z/10;
		dpos.y = dpos.y/10;
		float starting_velocity = e.last.distance(e.now);
		if(starting_velocity  == 0 ){
			return e.now;
		}
		//check that there is a solution and that the projectile can actually hit.
		//do calculations
		int t=0;
		float dist = e.now.distance2D(unit);
		float maxtime = (def->maxvelocity-def->startvelocity)/(def->range*1.3f);
		float last_gap = dist;
		float dimensions = float(max(ed->xsize*8,ed->ysize*8));
		if(G->cb->GetCurrentFrame() - e.last_frame < 5){
			t = -20;
		}else{
			t = 0 - (G->cb->GetCurrentFrame() - e.last_frame);
		}
		weapon_velocity = (def->startvelocity + def->maxvelocity)/2;
		float3 apos = e.now;
		while(t < maxtime){
			apos = e.now;//last_pos;
			apos.x += (dpos.x*t);
			apos.z += (dpos.z*t);
			//weapon_velocity = min(def->startvelocity+def->weaponacceleration*t,def->maxvelocity);
			float current_gap = unit.distance2D(apos) - (weapon_velocity*t);
			if(current_gap < 0){
				current_gap *= -1;
			}
			if(current_gap < dimensions){
				return apos;
			}else{
				if(current_gap > last_gap){
					return apos-dpos;
				}else{
					last_gap = current_gap;
				}
			}
			t++;
		}
		float3 tpos = e.now;
		tpos += (apos-e.now)*0.5;
		return tpos;
	}
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Planning::feasable(string s, int builder){
	NLOG("Planning::feasable");
	if(NoAntiStall.empty() == false){
		for(vector<string>::iterator i = NoAntiStall.begin(); i != NoAntiStall.end(); ++i){
			if(*i == s){
				return true;
			}
		}
	}
	//if(G->info->antistall == 3){ // ANTISTALL ALGORITHM V3
		const UnitDef* uud = G->GetUnitDef(s);
		const UnitDef* pud = G->GetUnitDef(builder);
		if(uud == 0){
			G->L.print("uud == 0 , could not get unitdef for :: " + s );
			return false;
		}
		if(pud== 0){
			G->L.print("pud == 0 could not get unitdef for builder");
			return false;
		}
		float t = uud->buildTime/pud->buildSpeed;
		//t *=1.875;
		//char c2[20];
		//sprintf( c2,"%f",t);
		///G->L.iprint(string("time to build :: ") + c2 + string(" :: ") + s);
		//char c3[20];
		//sprintf( c3,"%f",uud->buildTime);
		//G->L.iprint(string("uud->buildTime :: ") + c3 + string(" :: ") + s);
		//char c4[20];
		//sprintf( c4,"%f",pud->buildSpeed);
		//G->L.iprint(string("pud->buildSpeed :: ") + c4 + string(" :: ") + s);
		if(uud->energyCost > 0){
			float e_usage = G->cb->GetEnergyUsage()+( uud->energyCost/t);
			float e = G->cb->GetEnergyIncome()-e_usage;
 			//char c[20];
 			//sprintf( c,"%f",( uud->energyCost/t));
 			//G->L.iprint(string("energy out :: ") + c + string(" :: ") + s);
			if(G->cb->GetEnergy() < e_usage){
				if(G->info->antistall == 5){
					//ENERGY STALL
					//G->L.print("ENERGY STALL anticipated :: "+s);
					return false;
				}
				//char c5[20];
				//sprintf( c5,"%f",(uud->energyCost/t));
				//G->L.iprint(string("energy cost :: ") + c5 + string(" :: ") + s);
				//float eres = Se -(t*e)
				
				if(G->cb->GetEnergy()+ t*G->cb->GetEnergyIncome()< t*e_usage){
					//ENERGY STALL
					//G->L.print("ENERGY STALL anticipated :: "+s);
					return false;
				}
			}
		}
		if(uud->metalCost>0){
			float m_usage = G->cb->GetMetalUsage()+( uud->metalCost/t);//G->cb->GetMetalUsage()+( (uud->metalCost*GetRealValue(pud->buildSpeed))/uud->buildTime);
			//
			
			
		//	float m = -;
			//char c[20];
			//sprintf( c,"%f",( uud->metalCost/t));
			//G->L.iprint(string("metal out  for:: ") + c+ string(" :: ") + s);
			if(G->cb->GetMetalIncome()<m_usage){
				if(G->info->antistall == 5){
					//METAL STALL
					//G->L.print("METAL STALL anticipated :: "+ s);
					return false;
				}
				//
				if(G->cb->GetMetal() +t*G->cb->GetMetalIncome()< t*m_usage){
					//METAL STALL
					//G->L.print("METAL STALL anticipated :: "+ s);
					return false;
				}
			}
		}
		//G->L.print("Given the go ahead :: "+s);
		return true;
//	}

	///
	/*
	if(G->cb->GetMetal() < G->cb->GetMetalStorage()*0.2f){
		return false;
	}
	if(G->cb->GetMetal() < 200.0f){
		return false;
	}
	if(G->cb->GetEnergy() < G->cb->GetEnergyStorage()*0.2f){
		return false;
	}
	if(G->cb->GetEnergy() < 200.0f){
		return false;
	}
	const UnitDef* uud = G->GetUnitDef(s);
	const UnitDef* pud = G->GetUnitDef(builder);
	if(uud == 0) return true;
	if(pud== 0) return true;
	float BuildTime = uud->buildTime / pud->buildSpeed;
// 	if(G->info->abstract == true){
// 		
// 		
// 		// Energy Cost/Second
// 		float EPerS = (uud->energyCost*0.9f) / BuildTime;
// 		if (G->cb->GetEnergy() + BuildTime * (G->cb->GetEnergyIncome() - (G->cb->GetEnergyUsage() + EPerS)) < 0.0f){
// 			//insufficient energy
// 			G->L << "insufficient energy to build " << s << ", a stall is anticipated if construction is started" << endline;
// 			return false;
// 		}
// 		float MPerS = (uud->metalCost*0.9f) / BuildTime;
// 		if (G->cb->GetMetal() + BuildTime * (G->cb->GetMetalIncome() - (G->cb->GetMetalUsage() + MPerS)) < 0.0f){
// 			// insufficient metal
// 			G->L << "insufficient metal to build " << s << ", a stall is anticipated if construction is started" << endline;
// 			return false;
// 		}
// 	}else{
		// This anti stall algorithm should work but it works better as the amount you have stored goes up, so build storage buildings!
	if(G->cb->GetMetalUsage() > G->cb->GetMetalIncome()*a){
		//check if we'll run out at this rate if( timexout > stored
		float over = (BuildTime*G->cb->GetMetalUsage()) - (G->cb->GetMetal()*b);
	//	if( over > (2 SECONDS)){
		if( (BuildTime*G->cb->GetMetalUsage()) > (G->cb->GetMetal()*b)){
			return false;
		}
	}
	if(G->cb->GetEnergyUsage() > G->cb->GetEnergyIncome()*c){
		//check if we'll run out at this rate if( timexout > stored
		float over = (BuildTime*G->cb->GetEnergyUsage()) - (G->cb->GetEnergy()*d);
		if( over > (2 SECONDS)){
			return false;
		}
	}
	//}*/
	return true;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/*int Planning::DrawLine(ARGB colours, float3 A, float3 B, int LifeTime, float Width, bool Arrow){
	// Get New Group
	/*int DrawGroup = G->cb->CreateLineFigure(float3(0,0,0), float3(1,1,1), 1.0f, 0, LifeTime, 0);
	G->cb->SetFigureColor(DrawGroup, colours.red, colours.green, colours.blue,colours.alpha);
	G->cb->CreateLineFigure(A, B, Width, Arrow ? 1 : 0, LifeTime, DrawGroup);
	return DrawGroup;
	return 0;
}*/

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/*int Planning::DrawLine(float3 A, float3 B, int LifeTime, float Width, bool Arrow){
	// Get New Group
	/*int DrawGroup = G->cb->CreateLineFigure(float3(0,0,0), float3(1,1,1), 1.0f, 0, LifeTime, 0);
	switch(G->cb->GetMyAllyTeam()){
		case 0:
            G->cb->SetFigureColor(DrawGroup, 0.0f, 0.0f, 1.0f, 1.0f);
            break;
        case 1:
            G->cb->SetFigureColor(DrawGroup, 0.0f, 1.0f, 0.0f, 1.0f);
            break;
        case 2:
            G->cb->SetFigureColor(DrawGroup, 1.0f, 1.0f, 1.0f, 1.0f);
            break;
        case 3:
            G->cb->SetFigureColor(DrawGroup, 0.0f, 1.0f, 0.0f, 1.0f);
            break;
        case 4:
			G->cb->SetFigureColor(DrawGroup, 0.0f, 0.0f, 0.75f, 1.0f);
            break;
        case 5:
            G->cb->SetFigureColor(DrawGroup, 1.0f, 0.0f, 1.0f, 1.0f);
            break;
        case 6:
            G->cb->SetFigureColor(DrawGroup, 1.0f, 1.0f, 0.0f, 1.0f);
            break;
        case 7:
			G->cb->SetFigureColor(DrawGroup, 0.0f, 0.0f, 0.0f, 1.0f);
            break;
        case 8:
            G->cb->SetFigureColor(DrawGroup, 0.0f, 1.0f, 1.0f, 1.0f);
            break;
        case 9:
            G->cb->SetFigureColor(DrawGroup, 0.625f, 0.625f, 0.0f, 1.0f);
            break;
	}
    G->cb->CreateLineFigure(A, B, Width, Arrow ? 1 : 0, LifeTime, DrawGroup);
	return DrawGroup;
	return 0;
}*/

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

