#include "../Core/include.h"

namespace ntai {

	CKeywordConstructionTask::CKeywordConstructionTask(Global* GL, int unit, btype type){
		valid=true;
		G = GL;
		this->unit=unit;
		this->type = type;
		this->utd = G->UnitDefLoader->GetUnitTypeDataByUnitId(unit);
		if(utd == 0){
			valid = false;
		}
	}

	void CKeywordConstructionTask::RecieveMessage(CMessage &message){
		NLOG("CKeywordConstructionTask::RecieveMessage");
		if (!valid) return;

		if(message.GetType() == string("unitidle")){
			if(message.GetParameter(0) == unit){
				if((type == B_GUARDIAN)
					||(type == B_GUARDIAN_MOBILES)
					||(type == B_RULE_EXTREME_CARRY)){
						Init();
				}else{
					End();
					return;
				}
			}
		}else if(message.GetType() == string("type?")){
			message.SetType(" keywordtask: "+G->Manufacturer->GetTaskName(this->type));
		}else	if(message.GetType() == string("unitdestroyed")){
			if(message.GetParameter(0) == unit){
				End();
				return;
			}
		}else	if(message.GetType() == string("buildposition")){
			// continue construction
			//

			TCommand tc(unit,"CBuild");
			tc.ID(-building->GetUnitDef()->id);
			float3 pos = message.GetFloat3();

			if(building->IsMobile()==false){
				if(pos==UpVector){
					G->L.print("CKeywordConstructionTask::RecieveMessage BuildPlacement returned UpVector or some other nasty position");
					End();
					return;
				}else if (G->cb->CanBuildAt(building->GetUnitDef(),pos,0)==false){
					G->L.print("CKeywordConstructionTask::RecieveMessage BuildPlacement returned a position that cant eb built upon");
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
				}else*/ if ((*qi)->utd == building){
				G->L.print("CKeywordConstructionTask::RecieveMessage overlapping plans that're the same item but not started, moving pos to make it build quicker");
				pos = (*qi)->pos;
				}
			}

			tc.PushFloat3(pos);
			tc.Push(0);
			tc.c.timeOut = int(building->GetUnitDef()->buildTime/5) + G->cb->GetCurrentFrame();
			tc.created = G->cb->GetCurrentFrame();
			tc.delay=0;

			if(!G->OrderRouter->GiveOrder(tc)){
				G->L.print("CKeywordConstructionTask::RecieveMessage Failed Order G->OrderRouter->GiveOrder(tc)== false:: " + building->GetUnitDef()->name);
				End();
				return;

			}else{
				// create a plan
				G->L.print("CKeywordConstructionTask::RecieveMessage G->OrderRouter->GiveOrder(tc)== true :: " + building->GetUnitDef()->name);

				if(qi == G->Manufacturer->BPlans->end()){
					NLOG("CKeywordConstructionTask::RecieveMessage :: WipePlansForBuilder");
					G->L.print("CKeywordConstructionTask::RecieveMessage wiping and creaiing the plan :: " + building->GetUnitDef()->name);


					CBPlan* Bplan = new CBPlan();
					Bplan->started = false;
					Bplan->AddBuilder(unit);
					Bplan->subject = -1;
					Bplan->pos = pos;
					Bplan->utd=building;
					G->Manufacturer->AddPlan();
					Bplan->id = G->Manufacturer->getplans();
					Bplan->radius = (float)max(building->GetUnitDef()->xsize,building->GetUnitDef()->ysize)*8.0f;
					Bplan->inFactory = building->IsFactory();

					G->Manufacturer->BPlans->push_back(Bplan);
					G->BuildingPlacer->Block(pos,building);

					//G->BuildingPlacer->UnBlock(G->GetUnitPos(uid),ud);
					//builders[unit].curplan = plancounter;
					//builders[unit].doingplan = true;
				}
				return;
			}
		}
	}

	void CKeywordConstructionTask::Build(){
		G->L.print("CKeywordConstructionTask::Build() :: " + building->GetUnitDef()->name);


		if(G->Pl->AlwaysAntiStall.empty() == false){ // Sort out the stuff that's setup to ALWAYS be under the antistall algorithm
			NLOG("CKeywordConstructionTask::Build G->Pl->AlwaysAntiStall.empty() == false");
			for(vector<string>::iterator i = G->Pl->AlwaysAntiStall.begin(); i != G->Pl->AlwaysAntiStall.end(); ++i){
				if(*i == building->GetName()){
					NLOG("CKeywordConstructionTask::Build *i == name :: "+building->GetUnitDef()->name);
					if(!G->Pl->feasable(building,utd)){
						G->L.print("unfeasable " + building->GetUnitDef()->name);
						End();
						return;
					}else{
						NLOG("CKeywordConstructionTask::Build  "+building->GetUnitDef()->name+" is feasable");
						break;
					}
				}
			}
		}


		NLOG("CKeywordConstructionTask::Build  Resource\\MaxEnergy\\");
		float emax=1000000000;

		string key = "Resource\\MaxEnergy\\";
		key += building->GetName();
		G->Get_mod_tdf()->GetDef(emax,"3000000",key);// +300k energy per tick by default/**/

		if(emax ==0){
			emax = 30000000;
		}

		if(G->Pl->GetEnergyIncome() > emax){
			G->L.print("CKeywordConstructionTask::Build  emax " + building->GetUnitDef()->name);
			End();
			return;
		}

		NLOG("CKeywordConstructionTask::Build  Resource\\MinEnergy\\");
		float emin=0;

		key = "Resource\\MinEnergy\\";
		key += building->GetName();

		G->Get_mod_tdf()->GetDef(emin,"0",key);// +0k energy per tick by default/**/

		if(G->Pl->GetEnergyIncome() < emin){
			G->L.print("CKeywordConstructionTask::Build  emin " + building->GetUnitDef()->name);
			End();
			return;
		}


		// Now sort out stuff that can only be built one at a time

		if(building->GetSoloBuildActive()){
			NLOG("CKeywordConstructionTask::Build  G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()");
			End();
			return;
		}

		// Now sort out if it's one of those things that can only be built once
		if(building->GetSingleBuildActive()){
			G->L.print("CKeywordConstructionTask::Build  singlebuild " + building->GetUnitDef()->name);
			End();
			return;
		}

		if(G->info->antistall>1){
			NLOG("CKeywordConstructionTask::Build  G->info->antistall>1");

			bool fk = G->Pl->feasable(building,utd);

			NLOG("CKeywordConstructionTask::Build  feasable called");

			if(!fk){
				G->L.print("CKeywordConstructionTask::Build  unfeasable " + building->GetUnitDef()->name);
				End();
				return;
			}
		}

		float3 unitpos = G->GetUnitPos(unit);
		float3 pos=unitpos;

		if(G->Map->CheckFloat3(unitpos) == false){
			NLOG("CKeywordConstructionTask::Build  mark 2# bad float exit");
			End();
			return;
		}
		


		if(building->GetDeferRepairRange() > 10){
			NLOG("CKeywordConstructionTask::Build  rmax > 10");

			int* funits = new int[10000];
			int fnum = G->cb->GetFriendlyUnits(funits,unitpos,building->GetDeferRepairRange());

			if(fnum > 1){
				//
				for(int i = 0; i < fnum; i++){
					CUnitTypeData* ufdt = G->UnitDefLoader->GetUnitTypeDataByUnitId(funits[i]);

					if(ufdt == building){
						NLOG("CKeywordConstructionTask::Build  mark 2b#");

						if(G->GetUnitPos(funits[i]).distance2D(unitpos) < building->GetDeferRepairRange()){
							if(G->cb->UnitBeingBuilt(funits[i])==true){
								delete [] funits;
								NLOG("CKeywordConstructionTask::Build  exit on repair");
								G->Actions->Repair(unit,funits[i]);
								End();
								return;
							}
						}
					}

				}
			}

			delete [] funits;
		}

		NLOG("CKeywordConstructionTask::Build  mark 3#");
		////////

		if(building->GetExclusionRange() > 10){

			int* funits = new int[10000];

			int fnum = G->cb->GetFriendlyUnits(funits,unitpos,(float)building->GetExclusionRange());

			if(fnum > 1){
				//
				for(int i = 0; i < fnum; i++){
					CUnitTypeData* ufdt = G->UnitDefLoader->GetUnitTypeDataByUnitId(funits[i]);

					if(ufdt == building){

						NLOG("CKeywordConstructionTask::Build  mark 3a#");

						if(G->GetUnitPos(funits[i]).distance2D(unitpos) < building->GetExclusionRange()){
							int kj = funits[i];
							delete [] funits;

							if(G->cb->UnitBeingBuilt(kj)==true){
								NLOG("CKeywordConstructionTask::Build  exit on repair");
								if(!G->Actions->Repair(unit,kj)){
									End();
								}
								return;
							}else{
								NLOG("CKeywordConstructionTask::Build  return false");
								End();
								return;
							}

						}

					}
				}
			}
			delete [] funits;
		}

		NLOG("CKeywordConstructionTask::Build  mark 4");

		G->BuildingPlacer->GetBuildPosMessage(this,unit,unitpos,utd,building,building->GetSpacing()*1.4f);

		NLOG("CKeywordConstructionTask::Build  mark 5");
	}

	bool CKeywordConstructionTask::Init(){
		
		if(!valid){
			return false;
		}

		NLOG(("CKeywordConstructionTask::Init"+G->Manufacturer->GetTaskName(type)));
		G->L.print("CKeywordConstructionTask::Init "+G->Manufacturer->GetTaskName(type));

		//G->L.print("CKeywordConstructionTask::Init");

		NLOG("1");
		NLOG((string("tasktype: ")+G->Manufacturer->GetTaskName(type)));

		// register this modules listeners
		//G->RegisterMessageHandler("unitidle",me);
		//G->RegisterMessageHandler("unitdestroyed",me);

		if(type == B_RANDMOVE){
			if(G->Actions->RandomSpiral(unit)==false){
				End();
				return false;
			}else{
				return true;
			}
		} else if(type == B_RETREAT){
			if(!G->Actions->Retreat(unit)){
				End();
				return false;
			}else{
				return true;
			}
		} else if(type == B_GUARDIAN){
			if(!G->Actions->RepairNearby(unit,1200)){
				End();
				return false;
			}else{
				return true;
			}
		} else if(type == B_GUARDIAN_MOBILES){
			if(!G->Actions->RepairNearbyUnfinishedMobileUnits(unit,1200)){
				End();
				return false;
			}else{
				return true;
			}
		} else if(type == B_RESURECT){
			if(!G->Actions->RessurectNearby(unit)){
				End();
				return false;
			}else{
				return true;
			}
		}else if((type == B_RULE)||(type == B_RULE_EXTREME)||(type == B_RULE_EXTREME_NOFACT)||(type == B_RULE_EXTREME_CARRY)){//////////////////////////////

			type = G->Economy->Get(!(type == B_RULE),!(B_RULE_EXTREME_NOFACT == type));
			if(type == B_NA){
				valid = false;
				End();
				return false;
			}

			CUBuild b;
			b.Init(G,utd,unit);
			string targ = b(type);
			G->L.print("gotten targ value of " + targ + " for RULE");
			if (targ != string("")){

				building = G->UnitDefLoader->GetUnitTypeDataByName(targ);
				Build();
				return true;
			}else{
				G->L.print("B_RULE_EXTREME skipped bad return");
				End();
				return false;
			}
		}else if(type  == B_OFFENSIVE_REPAIR_RETREAT){
			if(G->Actions->OffensiveRepairRetreat(unit,800)==false){
				End();
				return false;
			}else{
				return true;
			}
		}else if(type  == B_REPAIR){
			if(G->Actions->RepairNearby(unit,500)==false){
				if(G->Actions->RepairNearby(unit,1000)==false){
					if(G->Actions->RepairNearby(unit,2600)==false){
						End();
						return false;
					}else{
						return true;
					}
				}else{
					return true;
				}
			}else{
				return true;
			}
			return valid;
		}else if(type  == B_RECLAIM){
			if(!G->Actions->ReclaimNearby(unit,400)){
				End();
				return false;
			}else{
				return true;
			}
		}else if(type  == B_GUARD_FACTORY){
			float3 upos = G->GetUnitPos(unit);
			if(G->Map->CheckFloat3(upos)==false){
				End();
				return false;
			}
			int* funits = new int[6000];
			int fnum = G->cb->GetFriendlyUnits(funits,upos,2000);
			if(fnum > 0){
				int closest=0;
				float d = 2500;
				for(int  i = 0; i < fnum; i++){
					if(funits[i] == unit) continue;
					float3 rpos = G->GetUnitPos(funits[i]);
					if(G->Map->CheckFloat3(rpos)==false){
						continue;
					}else{
						float q = upos.distance2D(rpos);
						if(q < d){
							const UnitDef* rd = G->GetUnitDef(funits[i]);
							if(rd == 0){
								continue;
							}else{
								if(rd->builder && (rd->movedata == 0) && (rd->buildOptions.empty()==false)){
									d = q;
									closest = funits[i];
									continue;
								}else{
									continue;
								}
							}
						}
					}
				}
				if(ValidUnitID(closest)){
					delete [] funits;
					if(!G->Actions->Guard(unit,closest)){
						End();
						return false;
					}else{
						return true;
					}
				}else{
					delete [] funits;
					End();
					return false;
				}
			}
			delete [] funits;
			End();
			return false;
		}else if(type  == B_GUARD_LIKE_CON){
			float3 upos = G->GetUnitPos(unit);
			if(G->Map->CheckFloat3(upos)==false){
				End();
				return false;
			}
			int* funits = new int[5005];
			int fnum = G->cb->GetFriendlyUnits(funits,upos,2000);
			if(fnum > 0){
				int closest=-1;
				float d = 2500;
				for(int  i = 0; i < fnum; i++){
					if(funits[i] == unit) continue;
					float3 rpos = G->GetUnitPos(funits[i]);
					if(G->Map->CheckFloat3(rpos)==false){
						continue;
					}else{
						float q = upos.distance2D(rpos);
						if(q < d){
							CUnitTypeData* rd = G->UnitDefLoader->GetUnitTypeDataByUnitId(funits[i]);
							if(rd == building){
								d = q;
								closest = funits[i];
							}
							continue;
						}
					}
				}
				if(ValidUnitID(closest)){
					delete [] funits;
					if(!G->Actions->Guard(unit,closest)){
						End();
						return false;
					}else{
						return true;
					}
				}else{
					delete [] funits;
					End();
					return false;
				}
			}
			delete [] funits;
		}else{// if(type != B_NA){
			CUBuild b;
			b.Init(G,utd,unit);
			b.SetWater(G->info->spacemod);
			//if(b.ud->floater==true){
			//	b.SetWater(true);
			//}
			string targ = b(type);
			if(targ == string("")){
				G->L << "if(targ == string(\"\")) for "<< G->Manufacturer->GetTaskName(type)<< endline;
				End();
				return false;
			}else{
				CUnitTypeData* udk = G->UnitDefLoader->GetUnitTypeDataByName(targ);
				building = udk;
				Build();
				return true;
			}
		}
		End();
		return false;
	}

	void CKeywordConstructionTask::End(){
		NLOG("CKeywordConstructionTask::End");
		valid = false;
		//CMessage message(string("taskfinished"));
		//FireEventListener(message);
	//	G->RemoveHandler(me);
	}
}
