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
bool Planning::MetalForConstr(const UnitDef* ud, int workertime){
	float metal = (ud->buildTime/(float)workertime) * (GetMetalIncome()-(G->cb->GetMetalUsage()) + G->cb->GetMetal());
	float total_cost = ud->metalCost;
	if(metal > total_cost)
		return true;

	return false;
}
bool Planning::EnergyForConstr(const UnitDef* ud, int workertime){
	// check energy
	float energy = (ud->buildTime/(float)workertime) * (GetEnergyIncome()-(G->cb->GetEnergyUsage()) + G->cb->GetEnergy());
	float total_cost = ud->energyCost;
	if(energy>total_cost)
		return true;
	return true;
	//return (energy > bt->unitList[unit-1]->energyCost);
}
Planning::Planning(Global* GLI){
	G = GLI;
	fnum = 1;
//	lEupdate= lMupdate = 0;
}

float Planning::GetEnergyIncome(){
	return G->cb->GetEnergyIncome();
// 	if(G->GetCurrentFrame() - lEupdate > 32){
// 		e_cache = G->cb->GetEnergyIncome();
// 		lEupdate = G->GetCurrentFrame();
// 	}
// 	return e_cache;
}
float Planning::GetMetalIncome(){
	return G->cb->GetMetalIncome();
// 	if(G->GetCurrentFrame() - lMupdate > 8){
// 		m_cache = G->cb->GetMetalIncome();
// 		lMupdate = G->GetCurrentFrame();
// 	}
// 	return m_cache;
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
float3 Planning::GetDirVector(int enemy,float3 unit, const WeaponDef* def){
	NLOG("Planning::GetDirVector");
	return UpVector;
	/*const UnitDef* ed = G->GetUnitDef(enemy);
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
	}*/
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Planning::feasable(string s, int builder){
	const UnitDef* uud = G->GetUnitDef(s);
	if(uud == 0){
		G->L.print("Given the go ahead :: "+s);
		return true;
	}
	const UnitDef* pud = G->GetUnitDef(builder);
	if(pud == 0){
		G->L.print("Given the go ahead :: "+s);
		return true;
	}
	return feasable(uud,pud);
}
float Planning::FramesTillZeroRes(float in, float out, float starting, float maxres, float mtime, float minRes){
	//
	float r = starting;
	float t = 0;
	float diff = in-out;
	while(t<mtime){
		t++;
		r += diff;
		if(r > maxres) r= maxres;
		if( r < minRes) return t;
	}
	return mtime;
}

bool Planning::equalsIgnoreCase(string a ,string b){
	return G->lowercase(a)== G->lowercase(b);
}
bool Planning::feasable(const UnitDef* uud, const UnitDef* pud){
	NLOG("Planning::feasable");
	if(NoAntiStall.empty() == false){
		string n2 = G->lowercase(uud->name);
		for(vector<string>::iterator i = NoAntiStall.begin(); i != NoAntiStall.end(); ++i){
			string i2 = G->lowercase(*i);
			if(i2 == n2){
				G->L.print("Given the go ahead :: "+uud->name);
				return true;
			}
		}
	}
	if(G->GetCurrentFrame() < 43 SECONDS){
		G->L.print("Given the go ahead :: "+uud->name);
		return true;
	}
	if(G->info->antistall==7){ // SIMULATIVE ANTISTALL ALGORITHM V7
		float ti = uud->buildTime/pud->buildSpeed - (G->info->Max_Stall_Time*(32/30));
		// Energy Cost/Second
		float e_usage = G->cb->GetEnergyUsage()+( uud->energyCost/ti);
		float t;
		t = FramesTillZeroRes(GetEnergyIncome(),e_usage,G->cb->GetEnergy(),G->cb->GetEnergyStorage(),ti,200.0f);
		if(t < ti){
			G->L << "(7) insufficient energy to build " << uud->name << ", a stall is anticipated if construction is started" << endline;
			return false;
		}
		float m_usage = G->cb->GetMetalUsage()+( uud->metalCost/t);;
		ti = FramesTillZeroRes(GetMetalIncome(),m_usage,G->cb->GetMetal(),G->cb->GetMetalStorage(),ti,150.0f);
		if(t < ti){
			G->L << "(7) insufficient metal to build " << uud->name << ", a stall is anticipated if construction is started" << endline;
			return false;
		}
	}
	if(G->info->antistall==2){// XE8 ANTISTALL ALGORITHM V2
		
		float BuildTime = uud->buildTime / pud->buildSpeed;
		// Energy Cost/Second
		float EPerS = (uud->energyCost*0.9f) / BuildTime;
		if (G->cb->GetEnergy() + BuildTime * (G->cb->GetEnergyIncome() - (G->cb->GetEnergyUsage() + EPerS)) < 0.0f){
			//insufficient energy
			G->L << "(2) insufficient energy to build " << uud->name << ", a stall is anticipated if construction is started" << endline;
			return false;
		}
		float MPerS = (uud->metalCost*0.9f) / BuildTime;
		if (G->cb->GetMetal() + BuildTime * (G->cb->GetMetalIncome() - G->cb->GetMetalUsage() - MPerS) < 0.0f){
			// insufficient metal
			G->L << "(2) insufficient metal to build " << uud->name << ", a stall is anticipated if construction is started" << endline;
			return false;
		}
	}
	if((G->info->antistall == 3)||(G->info->antistall == 5)){ // ANTISTALL ALGORITHM V3+5
		float t = uud->buildTime/pud->buildSpeed - (G->info->Max_Stall_Time*(32/30));
		if(uud->energyCost > 100.0f){
			if(G->Economy->BuildPower(true)&&(G->UnitDefHelper->IsEnergy(uud)==false)){
				return false;
			}
			float e_usage = G->cb->GetEnergyUsage()+( uud->energyCost/t);
			if(GetEnergyIncome() < e_usage){
				if(G->info->antistall == 5){
					//ENERGY STALL
					G->L << "(5) insufficient energy to build " << uud->name << ", a stall is anticipated if construction is started" << endline;
					return false;
				}
			
				if((G->cb->GetEnergy()+ (t*GetEnergyIncome()))< (30/32)*t*e_usage){
					//ENERGY STALL
					G->L << "(5) insufficient energy to build " << uud->name << ", a stall is anticipated if construction is started" << endline;
					return false;
				}
			}
		}
		if(uud->metalCost>50){
			if(G->Economy->BuildMex(true)&& (G->UnitDefHelper->IsMex(uud)==false)){
				return false;
			}
			float m_usage = G->cb->GetMetalUsage()+( uud->metalCost/t);//G->cb->GetMetalUsage()+( (uud->metalCost*GetRealValue(pud->buildSpeed))/uud->buildTime);

			if(2*GetMetalIncome()<m_usage){
				if(G->info->antistall == 5){
					//METAL STALL
					G->L << "(3) insufficient metal to build " << uud->name << ", a stall is anticipated if construction is started" << endline;
					return false;
				}

				if(G->cb->GetMetal() +(t*GetMetalIncome())< (30/32)*t*m_usage){
					//METAL STALL
					G->L << "(3) insufficient metal to build " << uud->name << ", a stall is anticipated if construction is started" << endline;
					return false;
				}
			}
		}
		G->L.print("Given the go ahead :: "+uud->name);
		return true;
	}
	G->L.print("Given the go ahead :: "+uud->name);
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

