#include "../Core/helper.h"
// Tasks
#include "../Tasks/CKeywordConstructionTask.h"


CKeywordConstructionTask::CKeywordConstructionTask(Global* GL, int unit, btype type){
	valid=true;
	G = GL;
	this->unit=unit;
	this->type = type;
}

void CKeywordConstructionTask::RecieveMessage(CMessage &message){
	NLOG("CKeywordConstructionTask::RecieveMessage");
	if (!valid) return;

	if(message.GetType() == string("unitidle")){
		if(message.GetParameter(0) == unit){
			if((type == B_GUARDIAN)
				||(type == B_GUARDIAN_MOBILES)
				||(type == B_RULE_EXTREME_CARRY)){
					Init(me);
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
		tc.ID(-building->id);
		float3 pos = message.GetFloat3();
		if(G->UnitDefHelper->IsMobile(building)==false){
			if(pos==UpVector){
				G->L.print("CKeywordConstructionTask::RecieveMessage BuildPlacement returned UpVector or some other nasty position");
				End();
				return;
			}else if (G->cb->CanBuildAt(building,pos,0)==false){
				G->L.print("CKeywordConstructionTask::RecieveMessage BuildPlacement returned a position that cant eb built upon");
				End();
				return;
			}
		}
		pos.y = G->cb->GetElevation(pos.x,pos.z);

		deque<CBPlan>::iterator qi = G->Manufacturer->OverlappingPlans(pos,building);
		if(qi != G->Manufacturer->BPlans->end()){
			NLOG("vector<CBPlan>::iterator qi = OverlappingPlans(pos,ud); :: WipePlansForBuilder");
			/*if(qi->started){
			G->L.print("::Cbuild overlapping plans issuing repair instead");
			if(G->Actions->Repair(unit,qi->subject)){
			WipePlansForBuilder(unit);
			qi->builders.insert(unit);
			return false;
			}
			}else*/ if (qi->ud == building){
			G->L.print("CKeywordConstructionTask::RecieveMessage overlapping plans that're the same item but not started, moving pos to make it build quicker");
			pos = qi->pos;
			}/*else{
				G->L.print("::Cbuild overlapping plans that are not the same item, no alternative action, cancelling task");
				CMessage message(string("taskfinished"));
				FireEventListener(message);
				return;
			}*/
		}
		tc.PushFloat3(pos);
		tc.Push(0);
		tc.c.timeOut = int(building->buildTime/5) + G->cb->GetCurrentFrame();
		tc.created = G->cb->GetCurrentFrame();
		tc.delay=0;
		if(G->OrderRouter->GiveOrder(tc)== false){
			G->L.print("CKeywordConstructionTask::RecieveMessage Failed Order G->OrderRouter->GiveOrder(tc)== false:: " + building->name);
			End();
			return;
		}else{
			// create a plan
			G->L.print("CKeywordConstructionTask::RecieveMessage G->OrderRouter->GiveOrder(tc)== true :: " + building->name);
			if(qi == G->Manufacturer->BPlans->end()){
				NLOG("CKeywordConstructionTask::RecieveMessage :: WipePlansForBuilder");
				G->L.print("CKeywordConstructionTask::RecieveMessage wiping and creaiing the plan :: " + building->name);
				G->Manufacturer->WipePlansForBuilder(unit);
				CBPlan Bplan;
				Bplan.started = false;
				Bplan.builders.insert(unit);
				Bplan.subject = -1;
				Bplan.pos = pos;
				Bplan.ud=building;
				G->Manufacturer->AddPlan();
				Bplan.id = G->Manufacturer->getplans();
				Bplan.radius = (float)max(building->xsize,building->ysize)*8.0f;
				Bplan.inFactory = G->UnitDefHelper->IsFactory(building);
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
	G->L.print("CKeywordConstructionTask::Build() :: " + building->name);
	const UnitDef* builder = G->GetUnitDef(unit);

	/////////////

	if(builder == 0){
		G->L.print("error: a problem occurred loading this units unit definition unitid= "+to_string(unit)+" building=" +building->name);
		End();
		return;
	}
	if(G->Pl->AlwaysAntiStall.empty() == false){ // Sort out the stuff that's setup to ALWAYS be under the antistall algorithm
		NLOG("CKeywordConstructionTask::Build G->Pl->AlwaysAntiStall.empty() == false");
		for(vector<string>::iterator i = G->Pl->AlwaysAntiStall.begin(); i != G->Pl->AlwaysAntiStall.end(); ++i){
			if(*i == building->name){
				NLOG("CKeywordConstructionTask::Build *i == name :: "+building->name);
				if(G->Pl->feasable(building,builder) == false){
					G->L.print("unfeasable " + building->name);
					End();
					return;
				}else{
					NLOG("CKeywordConstructionTask::Build  "+building->name+" is feasable");
					break;
				}
			}
		}
	}
	string t = "";
	NLOG("CKeywordConstructionTask::Build  Resource\\MaxEnergy\\");
	float emax=1000000000;
	string key = "Resource\\MaxEnergy\\";
	t = building->name;
	trim(t);
	tolowercase(t);
	key += t;
	G->Get_mod_tdf()->GetDef(emax,"3000000",key);// +300k energy per tick by default/**/
	if(emax ==0) emax = 30000000;
	if(G->Pl->GetEnergyIncome() > emax){
		G->L.print("CKeywordConstructionTask::Build  emax " + building->name);
		End();
		return;
	}

	NLOG("CKeywordConstructionTask::Build  Resource\\MinEnergy\\");
	float emin=0;
	key = "Resource\\MinEnergy\\";
	t = building->name;
	trim(t);
	tolowercase(t);
	key += t;
	G->Get_mod_tdf()->GetDef(emin,"0",key);// +0k energy per tick by default/**/
	if(G->Pl->GetEnergyIncome() < emin){
		G->L.print("CKeywordConstructionTask::Build  emin " + building->name);
		End();
		return;
	}


	// Now sort out stuff that can only be built one at a time
	t = building->name;
	trim(t);
	tolowercase(t);
	if(G->Cached->solobuilds.find(t)!= G->Cached->solobuilds.end()){
		NLOG("CKeywordConstructionTask::Build  G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()");
		G->L.print("CKeywordConstructionTask::Build  solobuild " + building->name);
		if(G->Actions->Repair(unit,G->Cached->solobuilds[t])){// One is already being built, change to a repair order to go help it!
			End();
			return;
		}
	}

	// Now sort out if it's one of those things that can only be built once
	if(G->Cached->singlebuilds.find(t) != G->Cached->singlebuilds.end()){
		NLOG("CKeywordConstructionTask::Build  G->Cached->singlebuilds.find(name) != G->Cached->singlebuilds.end()");
		if(G->Cached->singlebuilds[t] == true){
			G->L.print("CKeywordConstructionTask::Build  singlebuild " + building->name);
			End();
			return;
		}
	}
	//if(G->Pl->feasable(name,unit)==false){
	//	return false;
	//}
	// If Antistall == 2/3 then we always do this

	if(G->info->antistall>1){
		NLOG("CKeywordConstructionTask::Build  G->info->antistall>1");
		bool fk = G->Pl->feasable(building,builder);
		NLOG("CKeywordConstructionTask::Build  feasable called");
		if(fk == false){
			G->L.print("CKeywordConstructionTask::Build  unfeasable " + building->name);
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
	string hj = building->name;
	trim(hj);
	tolowercase(hj);
	float rmax = G->Manufacturer->getRranges(hj);
	if(rmax > 10){
		NLOG("CKeywordConstructionTask::Build  rmax > 10");
		int* funits = new int[5000];
		int fnum = G->cb->GetFriendlyUnits(funits,unitpos,rmax);
		if(fnum > 1){
			//
			for(int i = 0; i < fnum; i++){
				const UnitDef* udf = G->GetUnitDef(funits[i]);
				if(udf == 0) continue;
				if(udf == building){
					NLOG("CKeywordConstructionTask::Build  mark 2b#");
					if(G->GetUnitPos(funits[i]).distance2D(unitpos) < rmax){
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
	float exrange = G->Manufacturer->getexclusion(hj);
	if(exrange > 10){
		int* funits = new int[5000];
		int fnum = G->cb->GetFriendlyUnits(funits,unitpos,rmax);
		if(fnum > 1){
			//
			for(int i = 0; i < fnum; i++){
				const UnitDef* udf = G->GetUnitDef(funits[i]);
				if(udf == 0) continue;
				if(udf == building){
					NLOG("CKeywordConstructionTask::Build  mark 3a#");
					if(G->GetUnitPos(funits[i]).distance2D(unitpos) < exrange){
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
	G->BuildingPlacer->GetBuildPosMessage(me,unit,unitpos,builder,building,G->Manufacturer->GetSpacing(building)*1.4f);
        NLOG("CKeywordConstructionTask::Build  mark 5");
}

bool CKeywordConstructionTask::Init(boost::shared_ptr<IModule> me){
	NLOG(("CKeywordConstructionTask::Init"+G->Manufacturer->GetTaskName(type)));
	G->L.print("CKeywordConstructionTask::Init "+G->Manufacturer->GetTaskName(type));
	this->me = me;
	//G->L.print("CKeywordConstructionTask::Init");
	const UnitDef* ud2= G->GetUnitDef(unit);
	if(ud2 == 0){
		End();
		return false;
	}
    NLOG("1");
    NLOG((string("tasktype: ")+G->Manufacturer->GetTaskName(type)));

	// register this modules listeners
	G->RegisterMessageHandler("unitidle",me);
	G->RegisterMessageHandler("unitdestroyed",me);

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
        b.Init(G,ud2,unit);
		string targ = b(type);
		G->L.print("gotten targ value of " + targ + " for RULE");
		if (targ != string("")){
			const UnitDef* udk = G->GetUnitDef(targ);
			if(udk == 0){
				End();
				return false;
			}

			building = udk;
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
		if(!G->Actions->ReclaimNearby(unit,1000)){
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
						const UnitDef* rd = G->GetUnitDef(funits[i]);
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
		b.Init(G,ud2,unit);
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
			const UnitDef* udk = G->GetUnitDef(targ);
			if(udk != 0){
				building = udk;
				Build();
				return true;
			}
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
