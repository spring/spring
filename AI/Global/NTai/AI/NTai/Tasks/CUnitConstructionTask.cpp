#include "../Core/helper.h"
// Tasks
#include "../Tasks/CUnitConstructionTask.h"

CUnitConstructionTask::CUnitConstructionTask(Global* GL, int unit, const UnitDef* builder, const UnitDef* building){
	valid=ValidUnitID(unit);
	if(!valid){
		End();
		return;
	}
	if(builder == 0){
		End();
		return;
	}
	if(building == 0){
		End();
		return;
	}

	G = GL;
	this->unit=unit;
	this->builder = builder;
	this->building = building;
	G->L.print("CUnitConstructionTask::CUnitConstructionTask object created | params: building :: "+building->name+" using builder::"+ builder->name);
}

CUnitConstructionTask::~CUnitConstructionTask(){
	G->L.print("CUnitConstructionTask::~CUnitConstructionTask :: "+building->name);
}

void CUnitConstructionTask::RecieveMessage(CMessage &message){
	if (!valid) return;
	NLOG("CUnitConstructionTask::RecieveMessage");
	if(message.GetType() == string("unitidle")){
		if(message.GetParameter(0) == unit){
			End();
			return;
		}
	}else	if(message.GetType() == string("unitdestroyed")){
		if(message.GetParameter(0) == unit){
			End();
			return;
		}
	}else if(message.GetType() == string("type?")){
		message.SetType(" build:"+building->name);
	}else if(message.GetType() == string("buildposition")){
		// continue construction
		//
		TCommand tc(unit,"CBuild");
		tc.ID(-building->id);
		float3 pos = message.GetFloat3();
		if(G->UnitDefHelper->IsMobile(building)==false){
			if(pos==UpVector){
				G->L.print("BuildPlacement returned UpVector or some other nasty position, a build location wasn't found!");
				End();
				return;
			} else if (G->cb->CanBuildAt(building,pos,0)==false){
				G->L.print("CUnitConstructionTask::RecieveMessage BuildPlacement returned a position that cant be built upon");
				End();
				return;
			}
		}
		pos.y = G->cb->GetElevation(pos.x,pos.z);

		deque<CBPlan* >::iterator qi = G->Manufacturer->OverlappingPlans(pos,building);
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
			if ((*qi)->ud == building){
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
		tc.c.timeOut = int(building->buildTime/5) + G->cb->GetCurrentFrame();
		tc.created = G->cb->GetCurrentFrame();
		tc.delay=0;
		if(G->OrderRouter->GiveOrder(tc)== false){
			G->L.print("CUnitConstructionTask::RecieveMessage Failed Order G->OrderRouter->GiveOrder(tc)== false:: " + builder->name);
			End();
			return;
		}else{
			// create a plan
			G->L.print("CUnitConstructionTask::RecieveMessage G->OrderRouter->GiveOrder(tc)== true :: " + builder->name);
			if(qi == G->Manufacturer->BPlans->end()){
				NLOG("CUnitConstructionTask::RecieveMessage :: WipePlansForBuilder");
				G->L.print("CUnitConstructionTask::RecieveMessage wiping and creaiing the plan :: " + builder->name);
				G->Manufacturer->WipePlansForBuilder(unit);
				CBPlan* Bplan = new CBPlan();
				Bplan->started = false;
				Bplan->AddBuilder(unit);
				Bplan->subject = -1;
				Bplan->pos = pos;
				Bplan->ud=building;
				G->Manufacturer->AddPlan();
				Bplan->id = G->Manufacturer->getplans();
				Bplan->radius = (float)max(building->xsize,building->ysize)*8.0f;
				Bplan->inFactory = G->UnitDefHelper->IsFactory(builder);
				G->Manufacturer->BPlans->push_back(Bplan);
				if((G->UnitDefHelper->IsFactory(builder)&&G->UnitDefHelper->IsMobile(building))==false){
					G->BuildingPlacer->Block(pos,building);
				}
				//G->BuildingPlacer->UnBlock(G->GetUnitPos(uid),ud);
				//builders[unit].curplan = plancounter;
				//builders[unit].doingplan = true;
			}
			return;
		}
	}
}

bool CUnitConstructionTask::Init(){
	G->L.print("CUnitConstructionTask::Init :: "+building->name);

	// register this modules listeners
	//G->RegisterMessageHandler("unitidle",me);
	//G->RegisterMessageHandler("unitdestroyed",me);

	//builder = G->GetUnitDef(unit);

	/////////////
	//builder = G->GetUnitDef(unit);
	if(builder == 0){
		G->L.print("CUnitConstructionTask::Init error: a problem occurred loading unit definition: unitid="+to_string(unit)+" building="+building->name);
		End();
		return false;
	}
	if(G->Pl->AlwaysAntiStall.empty() == false){ // Sort out the stuff that's setup to ALWAYS be under the antistall algorithm
		NLOG("CUnitConstructionTask::Init G->Pl->AlwaysAntiStall.empty() == false");
		for(vector<string>::iterator i = G->Pl->AlwaysAntiStall.begin(); i != G->Pl->AlwaysAntiStall.end(); ++i){
			if(*i == building->name){
				NLOG("CUnitConstructionTask::Init *i == name :: "+building->name);
				if(G->Pl->feasable(building,builder) == false){
					G->L.print("CUnitConstructionTask::Init  unfeasable " + building->name);
					End();
					return false;
				}else{
					NLOG("CUnitConstructionTask::Init  "+building->name+" is feasable");
					break;
				}
			}
		}
	}
	string t = "";
	NLOG("CUnitConstructionTask::Init  Resource\\MaxEnergy\\");
	float emax=1000000000;
	string key = "Resource\\MaxEnergy\\";
	t = building->name;
	trim(t);
	tolowercase(t);
	key += t;
	G->Get_mod_tdf()->GetDef(emax,"3000000",key);// +300k energy per tick by default/**/
	if(emax ==0) emax = 30000000;
	if(G->Pl->GetEnergyIncome() > emax){
		G->L.print("CUnitConstructionTask::Init  emax " + building->name);
		End();
		return false;
	}

	NLOG("CUnitConstructionTask::Init  Resource\\MinEnergy\\");
	float emin=0;
	key = "Resource\\MinEnergy\\";
	t = building->name;
	trim(t);
	tolowercase(t);
	key += t;
	G->Get_mod_tdf()->GetDef(emin,"0",key);// +0k energy per tick by default/**/
	if(G->Pl->GetEnergyIncome() < emin){
		G->L.print("CUnitConstructionTask::Init  emin " + building->name);
		End();
		return false;
	}



	// Now sort out stuff that can only be built one at a time
	t = building->name;
	trim(t);
	tolowercase(t);
	if(G->Cached->solobuilds.find(t)!= G->Cached->solobuilds.end()){
		NLOG("CUnitConstructionTask::Build  G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()");
		G->L.print("CUnitConstructionTask::Build  solobuild " + building->name);
		if(G->Actions->Repair(unit,G->Cached->solobuilds[t])){// One is already being built, change to a repair order to go help it!
			End();
			return true;
		}
	}

	// Now sort out if it's one of those things that can only be built once
	if(G->Cached->singlebuilds.find(t) != G->Cached->singlebuilds.end()){
		NLOG("CUnitConstructionTask::Build  G->Cached->singlebuilds.find(name) != G->Cached->singlebuilds.end()");
		if(G->Cached->singlebuilds[t] == true){
			G->L.print("CUnitConstructionTask::Build  singlebuild " + building->name);
			End();
			return true;
		}
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
			G->L.print("CUnitConstructionTask::Init  unfeasable " + building->name);
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
	string hj = building->name;
	trim(hj);
	tolowercase(hj);
	float rmax = G->Manufacturer->getRranges(hj);
	if(rmax > 10){
		NLOG("CUnitConstructionTask::Init  rmax > 10");
		int* funits = new int[5000];
		int fnum = G->cb->GetFriendlyUnits(funits,unitpos,rmax);
		if(fnum > 1){
			//
			for(int i = 0; i < fnum; i++){
				const UnitDef* udf = G->GetUnitDef(funits[i]);
				if(udf == 0) continue;
				if(udf == building){
					NLOG("CUnitConstructionTask::Init  mark 2b#");
					if(G->GetUnitPos(funits[i]).distance2D(unitpos) < rmax){
						if(G->cb->UnitBeingBuilt(funits[i])==true){
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
					NLOG("CUnitConstructionTask::Init  mark 3a#");
					if(G->GetUnitPos(funits[i]).distance2D(unitpos) < exrange){
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

	G->BuildingPlacer->GetBuildPosMessage(this,unit,unitpos,builder,building,G->Manufacturer->GetSpacing(building)*1.4f);
	return true;
	/////////////
	/*if(G->Manufacturer->CBuild(ud->name,unit,G->Manufacturer->GetSpacing(ud))){
		G->L.print("CBuild worked in CUnitConstructionTask::Init at building :: "+ud->name);
		return true;
	}else{
		G->L.print("CBuild failed in CUnitConstructionTask::Init at building :: "+ud->name);
		CMessage message(string("taskfinished"));
		FireEventListener(message);
		return false;
	}*/
}
void CUnitConstructionTask::End(){
	NLOG("CUnitConstructionTask::End");
	//if(!valid) return;
	valid = false;
	//CMessage message(string("taskfinished"));
	//FireEventListener(message);
	//G->RemoveHandler(me);
}
