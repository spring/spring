#include "../Core/include.h"

namespace ntai {


	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	void Planning::InitAI(){
		NLOG("Planning::InitAI");
		NLOG("filling antistall array");

		CTokenizer<CIsComma>::Tokenize(NoAntiStall, G->Get_mod_tdf()->SGetValueMSG("AI\\NoAntiStall"), CIsComma());

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
	}

	Planning::Planning(Global* GLI){
		G = GLI;
		fnum = 1;
	}

	float Planning::GetEnergyIncome(){
		return G->cb->GetEnergyIncome();
	}

	float Planning::GetMetalIncome(){
		return G->cb->GetMetalIncome();
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	bool Planning::feasable(string s, int builder){

		CUnitTypeData* wbuilder = G->UnitDefLoader->GetUnitTypeDataByUnitId(builder);
		CUnitTypeData* wbuilding = G->UnitDefLoader->GetUnitTypeDataByName(s);
		return feasable(wbuilding,wbuilder);
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
		string c = a;
		string d = b;
		trim(c);
		trim(d);
		tolowercase(c);
		tolowercase(d);
		return (c == d);
	}

	bool Planning::feasable(CUnitTypeData* building, CUnitTypeData* builder){
		NLOG("Planning::feasable");


		if(NoAntiStall.empty() == false){
			for(vector<string>::iterator i = NoAntiStall.begin(); i != NoAntiStall.end(); ++i){
				string ti = *i;
				if(ti == building->GetName()){
					G->L.print("Feasable:: "+building->GetName());
					return true;
				}
			}
		}

		float Max_Stall_Time = G->info->Max_Stall_TimeIMMobile;

		if(building->IsMobile()){
			Max_Stall_Time = G->info->Max_Stall_TimeMobile;
		}

		int t = 43;
		G->Get_mod_tdf()->GetDef(t,"43","AI\\antistallwindow");
		if(G->GetCurrentFrame() < ( t SECONDS)){
			//G->L.print("Given the go ahead :: "+uud->name);
			return true;
		}

		if(G->info->antistall==7){ // SIMULATIVE ANTISTALL ALGORITHM V7
			float ti = (building->GetUnitDef()->buildTime/max(builder->GetUnitDef()->buildSpeed,0.1f) + Max_Stall_Time)*(32/30);

			// Energy Cost/Second
			float e_usage = G->cb->GetEnergyUsage()+( building->GetUnitDef()->energyCost/ti);
			float t;
			t = FramesTillZeroRes(GetEnergyIncome(),e_usage,G->cb->GetEnergy(),G->cb->GetEnergyStorage(),ti,200.0f);
			
			if(t < ti){
				G->L << "(7) insufficient energy to build " << building->GetUnitDef()->name << ", a stall is anticipated if construction is started" << endline;
				return false;
			}

			float m_usage = G->cb->GetMetalUsage()+( building->GetUnitDef()->metalCost/t);;
			ti = FramesTillZeroRes(GetMetalIncome(),m_usage,G->cb->GetMetal(),G->cb->GetMetalStorage(),ti,150.0f);
			
			if(t < ti){
				G->L << "(7) insufficient metal to build " << building->GetUnitDef()->name << ", a stall is anticipated if construction is started" << endline;
				return false;
			}
		}

		if(G->info->antistall==2){// XE8 ANTISTALL ALGORITHM V2

			float BuildTime = building->GetUnitDef()->buildTime / max(builder->GetUnitDef()->buildSpeed,0.1f);

			// Energy Cost/Second
			float EPerS = (building->GetUnitDef()->energyCost*0.9f) / BuildTime;

			if (G->cb->GetEnergy() + BuildTime * (G->cb->GetEnergyIncome() - (G->cb->GetEnergyUsage() + EPerS)) < 0.0f){
				//insufficient energy
				G->L << "(2) insufficient energy to build " << building->GetUnitDef()->name << ", a stall is anticipated if construction is started" << endline;
				return false;
			}

			float MPerS = (building->GetUnitDef()->metalCost*0.9f) / BuildTime;

			if (G->cb->GetMetal() + BuildTime * (G->cb->GetMetalIncome() - G->cb->GetMetalUsage() - MPerS) < 0.0f){
				// insufficient metal
				G->L << "(2) insufficient metal to build " << building->GetUnitDef()->name << ", a stall is anticipated if construction is started" << endline;
				return false;
			}

		}

		if((G->info->antistall == 3)||(G->info->antistall == 5)){ // ANTISTALL ALGORITHM V3+5
			NLOG("ANTISTALL ALGORITHM V3+5");

			float t = (building->GetUnitDef()->buildTime/max(builder->GetUnitDef()->buildSpeed,0.1f) + Max_Stall_Time)*(32/30);
			t = max(t,0.1f);

			if(building->GetUnitDef()->energyCost > 100.0f){

				if(G->Economy->BuildPower(true)&&(!building->IsEnergy())){
					return false;
				}

				NLOG("ANTISTALL mk2");
				float e_usage = G->cb->GetEnergyUsage()+(building->GetUnitDef()->energyCost/t);

				if(GetEnergyIncome() < e_usage){
					if(G->info->antistall == 5){
						//ENERGY STALL
						G->L << "(5) insufficient energy to build " << building->GetUnitDef()->name << ", a stall is anticipated if construction is started" << endline;
						return false;
					}
				}

				NLOG("ANTISTALL mk3");

				if((G->cb->GetEnergy()+ (t*GetEnergyIncome()))< (30/32)*t*e_usage){
					//ENERGY STALL
					G->L << "(5) insufficient energy to build " << building->GetUnitDef()->name << ", a stall is anticipated if construction is started" << endline;
					return false;
				}

				NLOG("ANTISTALL mk4");
			}

			if(building->GetUnitDef()->metalCost>50){
				NLOG("ANTISTALL mk5");
				
				if(G->Economy->BuildMex(true)&& (!building->IsMex())){
					return false;
				}

				NLOG("ANTISTALL mk6");
				
				float m_usage = G->cb->GetMetalUsage()+( building->GetUnitDef()->metalCost/t);
				//G->cb->GetMetalUsage()+( (uud->metalCost*GetRealValue(pud->buildSpeed))/uud->buildTime);
				
				NLOG("ANTISTALL mk7");
				
				if(GetMetalIncome()<m_usage){
					if(G->info->antistall == 5){
						//METAL STALL
						G->L << "(3) insufficient metal to build " << building->GetUnitDef()->name << ", a stall is anticipated if construction is started" << endline;
						return false;
					}
				}

				NLOG("ANTISTALL mk8");

				if(G->cb->GetMetal() +(t*GetMetalIncome())< (30/32)*t*m_usage){
					//METAL STALL
					G->L << "(3) insufficient metal to build " << building->GetUnitDef()->name << ", a stall is anticipated if construction is started" << endline;
					
					return false;
				}

				NLOG("ANTISTALL mk9");
			}

			G->L.print("Given the go ahead :: "+building->GetUnitDef()->name);
			return true;
		}
		G->L.print("Feasable:: "+building->GetUnitDef()->name);
		return true;
	}

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	float Planning::BuildMetalPerSecond(const UnitDef* builder,const UnitDef* built){
		if(builder->buildSpeed){
			return (built->metalCost / BuildTime(builder,built));
		}
		return 0;
	}

	float Planning::BuildEnergyPerSecond(const UnitDef* builder,const UnitDef* built){
		if(builder->buildSpeed){
			return (built->energyCost / BuildTime(builder,built));
		}
		return 0;
	}

	float Planning::BuildTime(const UnitDef* builder,const UnitDef* built){
		if(builder->buildSpeed){
			return built->buildTime / builder->buildSpeed;
		}
		return 1000000000;
	}

	float Planning::ReclaimTime(const UnitDef* builder,const UnitDef* built,float health){
		if(!builder->canReclaim){
			return 100000000;
		}
		if(!built->reclaimable){
			return 100000000;
		}
		if(builder->reclaimSpeed){
			return ( (built->buildTime / builder->reclaimSpeed)/built->health)*health;
		}
		return 1000000000;
	}

	float Planning::CaptureTime(const UnitDef* builder,const UnitDef* built,float health){
		if(!builder->canCapture){
			return 100000000;
		}

		if(builder->captureSpeed){
			return ( (built->buildTime / builder->captureSpeed)/built->health)*health;
		}
		return 1000000000;
	}

	bool Planning::FeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinMpc, float MinEpc){
		if(builder->buildSpeed){
			float buildtime = BuildTime(builder,built);
			float Echange = GetEnergyIncome()*ECONRATIO - G->cb->GetEnergyUsage() - built->energyCost / buildtime;
			float ResultingRatio = (G->cb->GetEnergy()+(Echange*buildtime)) / G->cb->GetEnergyStorage();
			if(ResultingRatio > MinEpc){
				float Mchange =GetMetalIncome()*ECONRATIO- G->cb->GetMetalUsage() - built->metalCost / buildtime;
				ResultingRatio = (G->cb->GetMetal()+(Mchange*buildtime)) / G->cb->GetMetalStorage();
				return (ResultingRatio > MinMpc);
			}
		}
		return false;
	}

	bool Planning::MFeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinMpc){
		if(builder->buildSpeed){
			float buildtime = BuildTime(builder,built);
			float Mchange = GetMetalIncome()*ECONRATIO- G->cb->GetMetalUsage() - built->metalCost / buildtime;
			float ResultingRatio = (G->cb->GetMetal()+(Mchange*buildtime)) / G->cb->GetMetalStorage();
			return (ResultingRatio > MinMpc);
		}
		return false;
	}

	bool Planning::EFeasibleConstruction(const UnitDef* builder,const UnitDef* built,float MinEpc){
		if(builder->buildSpeed){
			float buildtime = BuildTime(builder,built);
			float Echange = GetEnergyIncome()*ECONRATIO - G->cb->GetEnergyUsage() - built->energyCost / buildtime;
			float ResultingRatio = (G->cb->GetEnergy()+(Echange*buildtime)) / G->cb->GetEnergyStorage();
			return (ResultingRatio > MinEpc);
		}
		return false;
	}
}
