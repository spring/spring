#include "../Core/include.h"

/*
AF 2004+
LGPL 2
*/

namespace ntai {

	CUnitConstructionTask::CUnitConstructionTask(Global* GL, int unit,CUnitTypeData* builder, CUnitTypeData* building){
		valid=ValidUnitID(unit);
		if(!valid){
			End();
			return;
		}

		G = GL;
		this->unit=unit;
		this->builder = builder;
		this->building = building;

		if( builder == 0){
			valid = false;
		}else if(building == 0){
			valid = false;
		}

		G->L.print("CUnitConstructionTask::CUnitConstructionTask object created | params: building :: "+this->building->GetUnitDef()->name+" using builder::"+ this->builder->GetUnitDef()->name);
	}

	CUnitConstructionTask::~CUnitConstructionTask(){
		G->L.print("CUnitConstructionTask::~CUnitConstructionTask :: "+building->GetUnitDef()->name);
	}

	void CUnitConstructionTask::RecieveMessage(CMessage &message){
		if(!valid){
			return;// false;
		}

		NLOG("CUnitConstructionTask::RecieveMessage");
		if(message.GetType() == string("unitidle")){
			if(message.GetParameter(0) == unit){
				End();
				return;
			}
		} else if(message.GetType() == string("unitdestroyed")){
			if(message.GetParameter(0) == unit){
				End();
				return;
			}
		}else if(message.GetType() == string("type?")){
			message.SetType(" build:"+building->GetUnitDef()->name);
		}else if(message.GetType() == string("buildposition")){
			// continue construction
			//
			TCommand tc(unit,"CBuild");
			tc.ID(-building->GetUnitDef()->id);
			float3 pos = message.GetFloat3();
			if(!building->IsMobile()){
				if(pos==UpVector){
					G->L.print("BuildPlacement returned UpVector or some other nasty position, a build location wasn't found!");
					End();
					return;
				} else if (G->cb->CanBuildAt(building->GetUnitDef(),pos,0)==false){
					G->L.print("CUnitConstructionTask::RecieveMessage BuildPlacement returned a position that cant be built upon");
					End();
					return;
				}
			}
			pos.y = G->cb->GetElevation(pos.x,pos.z);

			deque<CBPlan* >::iterator qi = G->Manufacturer->OverlappingPlans(pos,building->GetUnitDef());
			if(qi != G->Manufacturer->BPlans->end()){
				NLOG("vector<CBPlan>::iterator qi = OverlappingPlans(pos,ud); :: WipePlansForBuilder");
				/*if(qi->started){
					G->L.print("::Cbuild overlapping plans issuing repair instead");
					if(G->Actions->Repair(unit,qi->subject)){
						WipePlansForBuilder(unit);
						qi->builders.insert(unit);
						return false;
					}
				}else*/
				if ((*qi)->utd == building){
					G->L.print("CUnitConstructionTask::RecieveMessage overlapping plans that're the same item but not started, moving pos to make it build quicker");
					pos = (*qi)->pos;
				}
				/*else{
					G->L.print("::Cbuild overlapping plans that are not the same item, no alternative action, cancelling task");
					CMessage message(string("taskfinished"));
					FireEventListener(message);
					return;
				}*/
			}
			tc.PushFloat3(pos);
			tc.Push(0);
			tc.c.timeOut = int(building->GetUnitDef()->buildTime/5) + G->cb->GetCurrentFrame();
			tc.created = G->cb->GetCurrentFrame();
			tc.delay=0;

			if(G->OrderRouter->GiveOrder(tc)== false){
				G->L.print("CUnitConstructionTask::RecieveMessage Failed Order G->OrderRouter->GiveOrder(tc)== false:: " + builder->GetUnitDef()->name);
				End();
				return;
			}else{

				// create a plan
				G->L.print("CUnitConstructionTask::RecieveMessage G->OrderRouter->GiveOrder(tc)== true :: " + builder->GetUnitDef()->name);
				if(qi == G->Manufacturer->BPlans->end()){
					NLOG("CUnitConstructionTask::RecieveMessage :: WipePlansForBuilder");
					G->L.print("CUnitConstructionTask::RecieveMessage wiping and creaiing the plan :: " + builder->GetUnitDef()->name);

					CBPlan* Bplan = new CBPlan();
					Bplan->started = false;
					Bplan->AddBuilder(unit);
					Bplan->subject = -1;
					Bplan->pos = pos;
					Bplan->utd=building;
					G->Manufacturer->AddPlan();

					Bplan->id = G->Manufacturer->getplans();
					Bplan->radius = (float)max(building->GetUnitDef()->xsize,building->GetUnitDef()->ysize)*8.0f;
					Bplan->inFactory = builder->IsFactory();
					G->Manufacturer->BPlans->push_back(Bplan);
					if((builder->IsFactory()&&building->IsMobile())==false){
						G->BuildingPlacer->Block(pos,building);
					}

				}
				return;
			}
		}
	}

	bool CUnitConstructionTask::Init(){


		G->L.print("CUnitConstructionTask::Init :: "+building->GetUnitDef()->name);

		/////////////
		// Sort out the stuff that's setup to ALWAYS be under the antistall algorithm
		if(!G->Pl->AlwaysAntiStall.empty()){

			NLOG("CUnitConstructionTask::Init G->Pl->AlwaysAntiStall.empty() == false");
			for(vector<string>::iterator i = G->Pl->AlwaysAntiStall.begin(); i != G->Pl->AlwaysAntiStall.end(); ++i){

				if(*i == building->GetUnitDef()->name){

					NLOG("CUnitConstructionTask::Init *i == name :: "+building->GetUnitDef()->name);

					if(G->Pl->feasable(building,builder) == false){
						
						G->L.print("CUnitConstructionTask::Init  unfeasable " + building->GetUnitDef()->name);
						
						End();
						return false;
					}else{

						NLOG("CUnitConstructionTask::Init  "+building->GetUnitDef()->name+" is feasable");
						break;
					}

				}
			}
		}

		string t = "";
		
		NLOG("CUnitConstructionTask::Init  Resource\\MaxEnergy\\");
		
		float emax=1000000000;
		string key = "Resource\\MaxEnergy\\";
		key += building->GetName();
		G->Get_mod_tdf()->GetDef(emax,"3000000",key);// +300k energy per tick by default/**/
		
		if(emax ==0){
			emax = 30000000;
		}

		if(G->Pl->GetEnergyIncome() > emax){
			G->L.print("CUnitConstructionTask::Init  emax " + building->GetUnitDef()->name);
			End();
			return false;
		}

		NLOG("CUnitConstructionTask::Init  Resource\\MinEnergy\\");
		
		float emin=0;
		key = "Resource\\MinEnergy\\";
		key += building->GetName();

		G->Get_mod_tdf()->GetDef(emin,"0",key);// +0k energy per tick by default/**/

		if(G->Pl->GetEnergyIncome() < emin){
			G->L.print("CUnitConstructionTask::Init  emin " + building->GetUnitDef()->name);
			End();
			return false;
		}



		// Now sort out stuff that can only be built one at a time
		if(building->GetSoloBuildActive()){
			NLOG("CUnitConstructionTask::Build  G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()");
			End();
			return false;
		}

		// Now sort out if it's one of those things that can only be built once
		if(building->GetSingleBuildActive()){
			G->L.print("CUnitConstructionTask::Build  singlebuild " + building->GetUnitDef()->name);
			End();
			return true;
		}


		//if(G->Pl->feasable(name,unit)==false){
		//	return false;
		//}
		// If Antistall == 2/3 then we always do this

		if(G->info->antistall>1){
			NLOG("CUnitConstructionTask::Init  G->info->antistall>1");
			bool fk = G->Pl->feasable(building,builder);
			NLOG("CUnitConstructionTask::Init  feasable called");
			if(fk == false){
				G->L.print("CUnitConstructionTask::Init  unfeasable " + building->GetUnitDef()->name);
				End();
				return false;
			}
		}

		float3 unitpos = G->GetUnitPos(unit);
		float3 pos=unitpos;
		if(G->Map->CheckFloat3(unitpos) == false){
			NLOG("CUnitConstructionTask::Init  mark 2# bad float exit");
			End();
			return false;
		}

		if(building->GetDeferRepairRange() > 10){
			NLOG("CUnitConstructionTask::Init  rmax > 10");
			int* funits = new int[10000];
			int fnum = G->cb->GetFriendlyUnits(funits,unitpos,building->GetDeferRepairRange());
			if(fnum > 1){
				//
				for(int i = 0; i < fnum; i++){
					const UnitDef* udf = G->GetUnitDef(funits[i]);
					if(udf == 0) continue;
					if(udf == building->GetUnitDef()){
						NLOG("CUnitConstructionTask::Init  mark 2b#");
						if(G->GetUnitPos(funits[i]).distance2D(unitpos) < building->GetDeferRepairRange()){
							if(G->cb->UnitBeingBuilt(funits[i])){
								delete [] funits;
								NLOG("CUnitConstructionTask::Init  exit on repair");
								if(!G->Actions->Repair(unit,funits[i])){
									End();
									return false;
								}else{
									End();
									return true;
								}
							}
						}
					}
				}
			}
			delete [] funits;
		}

		NLOG("CUnitConstructionTask::Init  mark 3#");
		////////

		if(building->GetExclusionRange() > 10){
			int* funits = new int[10000];
			int fnum = G->cb->GetFriendlyUnits(funits,unitpos,building->GetDeferRepairRange());
			if(fnum > 1){
				//
				for(int i = 0; i < fnum; i++){
					const UnitDef* udf = G->GetUnitDef(funits[i]);
					if(udf == 0){
						continue;
					}
					if(udf == building->GetUnitDef()){
						NLOG("CUnitConstructionTask::Init  mark 3a#");
						if(G->GetUnitPos(funits[i]).distance2D(unitpos) < building->GetExclusionRange()){
							int kj = funits[i];
							delete [] funits;
							if(G->cb->UnitBeingBuilt(kj)==true){
								NLOG("CUnitConstructionTask::Init  exit on repair");
								if(!G->Actions->Repair(unit,kj)){
									End();
									return true;
								}else{
									End();
									return true;
								}
							}else{
								NLOG("CUnitConstructionTask::Init  return false");
								End();
								return false;
							}
						}
					}
				}
			}
			delete [] funits;
		}

		G->BuildingPlacer->GetBuildPosMessage(this,unit,unitpos,builder,building,building->GetSpacing()*1.4f);
		return true;
	}

	void CUnitConstructionTask::End(){
		NLOG("CUnitConstructionTask::End");
		valid = false;
	}

}
