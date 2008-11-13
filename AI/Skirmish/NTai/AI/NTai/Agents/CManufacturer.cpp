/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

// CManufacturer
#include "../Core/include.h"


namespace ntai {
	map<int,deque<CBPlan* >* > alliedplans;
	uint plancounter=1;


	uint CManufacturer::getplans(){
		return plancounter;
	}

	void CManufacturer::AddPlan(){
		plancounter++;
	}

	void CManufacturer::RemovePlan(){
		plancounter--;
	}

	float3 CManufacturer::GetBuildPos(int builder, const UnitDef* target, const UnitDef* builderdef, float3 unitpos){
		NLOG("CManufacturer::GetBuildPos");
		// Find the least congested sector
		// skew the new position around the centre of the sector
		// issue a new call to closestbuildsite()
		// push the unitpos out so that the commander does not try to build on top of itself leading to stall (especially important for something like nanoblobz)
		if(G->Map->CheckFloat3(unitpos) == false){
			G->L.print("CManufacturer::GetBuildPos bad position");
			return unitpos;
		}

		if(!ValidUnitID(builder)) return unitpos;
		if (target == 0) return unitpos;
		if (builderdef == 0) return unitpos;

		float3 epos = unitpos;
		epos.z -= min(max(float(builderdef->zsize*8+target->zsize*10),float(builderdef->xsize*8+target->xsize*10)),builderdef->buildDistance*0.7f);
		//epos.z -= target->buildDistance*0.6f;
		float angle = float(G->mrand()%320);
		unitpos = G->Map->Rotate(epos,angle,unitpos);

		// move the new position if it's off map
		if(unitpos.x > G->cb->GetMapWidth() * SQUARE_SIZE){
			unitpos.x = (G->cb->GetMapWidth() * SQUARE_SIZE)-(unitpos.x-(G->cb->GetMapWidth() * SQUARE_SIZE));
			unitpos.x -= 500;
		}
		if(unitpos.z > G->cb->GetMapHeight() * SQUARE_SIZE){
			unitpos.z = (G->cb->GetMapHeight() * SQUARE_SIZE)-(unitpos.z-(G->cb->GetMapHeight() * SQUARE_SIZE));
			unitpos.z -= 500;
		}
		if(unitpos.x < 0){
			unitpos.x *= (-1);
			unitpos.x += 500;
		}
		if(unitpos.z <0){
			unitpos.z *= (-1);
			unitpos.z += 500;
		}

		int wc = G->Map->WhichCorner(unitpos);
		if(wc == t_NE){
			unitpos.z += 40;
			unitpos.x -= 40;
		}
		if(wc == t_NW){
			unitpos.z += 40;
			unitpos.x += 40;
		}
		if(wc == t_SE){
			unitpos.z -= 40;
			unitpos.x -= 40;
		}
		if(wc == t_SW){
			unitpos.z -= 40;
			unitpos.x += 40;
		}
		return unitpos;
	}

	/*bool CManufacturer::TaskCycle(CBuilder* i){
		NLOG("CManufacturer::TaskCycle");
		if(i->tasks.empty()){
			if(i->GetRepeat()){
				//G->L.print("CManufacturer::TaskCycle, buildtreecomeptled and now being reloaded!");
				if(!LoadTaskList(i)){
					G->L.print("failed loading of task list for " + i->GetUnitDef()->name);
					return false;
				}
			}else{
				//G->L.print("CManufacturer::TaskCycle, cycle has no tasks!!!");
				return false;
			}
		}
		if(i->tasks.empty()){ // STILL EMPTY!!!
			return false;
		}
		// ok first task
		vector< boost::shared_ptr<ITask> >::iterator k = i->tasks.begin();
		boost::shared_ptr<ITask> t = *k;
		//G->L.print("CManufacturer::TaskCycle executing task :: " + GetTaskName(t->type));
		if(t->execute(i->GetID())==false){
			//G->L.print("CManufacturer::TaskCycle Task returned false");
			t->SetValid(false);
			i->tasks.erase(k);
			return false;
		}else{
			//G->L.print("CManufacturer::TaskCycle Task executed");
			btype j = t->GetType();

			if(j == B_RULE_EXTREME_CARRY){
				boost::shared_ptr<ITask> t2 = boost::shared_ptr<ITask>((ITask*)new Task(G, B_RULE_EXTREME_CARRY));
				if(t2->IsValid()){
					i->tasks.insert(i->tasks.begin(),t2);
				}
			}
			if(j == B_GUARDIAN){
				boost::shared_ptr<ITask> t2 = boost::shared_ptr<ITask>((ITask*)new Task(G, B_GUARDIAN));
				//
				if(t2->IsValid()){
					i->tasks.insert(i->tasks.begin(),t2);
				}
			}
			i->tasks.erase(k);
			return true;
		}
	}*/
	/*creg::Class *UnitDef::GetClass(void){
		return NULL;
	}
	UnitDef::~UnitDef(void){
		//
	}*/

	/*float3 CManufacturer::GetBuildPlacement(int unit,float3 unitpos,const UnitDef* builder, const UnitDef* ud, int spacing){
		NLOG("CManufacturer::GetBuildPlacement");
		//if(G->UnitDefHelper->IsFactory(builder)){
		//	return unitpos;
		//}
		float3 pos = unitpos;
		//if(G->UnitDefHelper->IsHub(builder)){
		//    unitpos.z+= 100;
		//}
		if(ud->type == string("MetalExtractor")){
			pos = G->M->getNearestPatch(unitpos,0.7f,ud->extractsMetal,ud);
			if(G->Map->CheckFloat3(pos) == false){
				//G->L.print("zero mex co-ordinates intercepted");
				return UpVector;
			}
			int* iunits = new int[10000];
			int itemp = G->GetEnemyUnits(iunits,pos,(float)max(ud->ysize,ud->xsize)*8);
			delete [] iunits;
			if(itemp>0){
				return UpVector;
			}
			if(!BPlans->empty()){
				// find any plans for mexes very close to here.... If so change position to match if the same, if its a better mex then cancel existing plan and continue as normal.
				for(deque<CBPlan>::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
					//
					if(G->UnitDefHelper->IsMex(i->ud)){
						if(i->pos.distance2D(pos) < (i->ud->extractRange+ud->extractRange)*0.75f){
							if(i->ud->extractsMetal > ud->extractsMetal){
								//
								if(!i->started){
									if(i->builders.empty()==false){
										for(set<int>::iterator j = i->builders.begin(); j != i->builders.end(); ++j){
											G->Actions->ScheduleIdle(*j);
											WipePlansForBuilder(*j);
										}
										i->builders.erase(i->builders.begin(),i->builders.end());
										i->builders.clear();
									}
									BPlans->erase(i);
									return pos;
								}else{
									return UpVector;
								}
							}else{
								if(i->started){
									G->Actions->Repair(unit,i->subject);
									i->builders.insert(unit);
								}
								return UpVector;
							}
						}
					}
				}
			}
			return pos;
		} else if(ud->isFeature==true){ // dragon teeth for dragon teeth rings
			pos = G->DTHandler->GetDTBuildSite(unit);
			if(G->Map->CheckFloat3(pos) == false){
				G->L.print(string("zero DT co-ordinates intercepted :: ")+ to_string(pos.x) + string(",")+to_string(pos.y)+string(",")+to_string(pos.z));
				return UpVector;
			}
			return pos;
		} else if((ud->type == string("Building"))&&(ud->builder == false)&&(ud->weapons.empty() == true)&&(ud->radarRadius > 100)){ // Radar!
			//pos = G->DTHandler->GetDTBuildSite(unit);
			if(G->UnitDefHelper->IsHub(builder)){
				pos = G->RadarHandler->NextSite(unit,ud,(int)builder->buildDistance);
			}else{
				pos = G->RadarHandler->NextSite(unit,ud,1200);
			}
			if(G->Map->CheckFloat3(pos) == false){
				G->L.print(string("zero radar placement co-ordinates intercepted  :: ")+ to_string(pos.x) + string(",")+to_string(pos.y)+string(",")+to_string(pos.z));
				return UpVector;
			}
			return pos;
		}else{
			/**if(G->UnitDefHelper->IsHub(builder)){
				pos = GetBuildPos(unit,ud,builder,pos);
			}**/
			//UnitDef* uj = new UnitDef();
			//*uj = *ud;
			//uj->xsize = ud->xsize + (spacing*2); // add the spacing for either side
			//uj->ysize = ud->ysize + (spacing*2);
			/*if(G->UnitDefHelper->IsHub(builder)){
				pos = G->cb->ClosestBuildSite(ud,unitpos,builder->buildDistance-5,spacing);
			}else{
				pos = G->cb->ClosestBuildSite(ud,unitpos,max(builder->buildDistance,400.0f)+1000.0f,spacing);
			}*/
			/*pos = G->BuildingPlacer->GetBuildPos(unitpos,builder,ud,(float)spacing);
			//pos = G->cb->ClosestBuildSite(ud,unitpos,2000,spacing);
			//pos = G->Map->ClosestBuildSite(ud,pos,float((4000+(G->cb->GetCurrentFrame()%940))),spacing);
			if(G->Map->CheckFloat3(pos) == false){
				//pos = GetBuildPos(unit,ud,unitpos);
				/*if(G->UnitDefHelper->IsHub(builder)==false){
					pos = G->cb->ClosestBuildSite(ud,pos,max(builder->buildDistance,400.0f)+2000.0f,spacing);
				}

				//pos = G->Map->ClosestBuildSite(ud,pos,9000.0f,spacing);
				if(G->Map->CheckFloat3(pos) == false){*//*
					G->L.print(string("bad vector returned for ")+ to_string(pos.x) + string(",")+to_string(pos.y)+string(",")+to_string(pos.z));
					return pos;
				//}
			}
			//delete uj;
			/*int* en = new int[100];
			int e = G->cb->GetFriendlyUnits(en,pos,10.0f);
			bool  badg = false;
			if(e >0){
				const UnitDef* ed = G->GetUnitDef(en[0]);
				if(ed != 0){
					if(G->UnitDefHelper->IsMobile(ed)==false){
						badg =true;
					}
				}
			}
			delete [] en;
			if(badg) return UpVector;*/
			/*int* iunits = new int[10000];
			int itemp = G->GetEnemyUnits(iunits,pos,(float)max(ud->ysize,ud->xsize)*8);
			delete [] iunits;
			if(itemp>0){
				G->L.print(string("discarding, enemy units found at  :: ")+ to_string(pos.x) + string(",")+to_string(pos.y)+string(",")+to_string(pos.z));
				return UpVector;
			}
			// Check single build plans.......
			string lname = ud->name;
			tolowercase(lname);
			if(G->Cached->singlebuilds.find(lname) != G->Cached->singlebuilds.end()){
				if(G->Cached->singlebuilds[lname]==false){
					// check BPlans, and if we find a plan for this, go help build it....
					if(!BPlans->empty()){
						for(deque<CBPlan>::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
							if(i->ud->name == ud->name){
								if(i->started){
									if(G->Actions->Repair(unit,i->subject)){
										G->L.print("discarding, repairing nearby plan instead");
										return UpVector;
									}
								}else{
									return i->pos;
								}
							}
						}
					}
				}
			}
			// Check solo build plans.......
			if(G->Cached->solobuilds.find(lname) == G->Cached->solobuilds.end()){
				bool found = false;//tolowercase(ud->name)
				if(G->Cached->solobuild.empty()==false){
					for(vector<string>::iterator j = G->Cached->solobuild.begin(); j != G->Cached->solobuild.end(); ++j){
						string j2 =*j;
						tolowercase(j2);
						if(j2==lname){
							found=true;
							break;
						}
					}
				}
				if(found){
					// check BPlans, and if we find a plan for this, go help build it....
					if(!BPlans->empty()){
						for(deque<CBPlan>::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
							if(i->ud->name == ud->name){
								if(i->started){
									if(G->Actions->Repair(unit,i->subject)){
										G->L.print("discarding, repairing nearby plan instead(2)");
										return UpVector;
									}
								}else{
									return i->pos;
								}
							}
						}
					}
				}
			}
			string j = ud->name;
			trim(j);
			tolowercase(j);
			float rmax = r_ranges[j];
			if(rmax > 10){
				if(!BPlans->empty()){
					for(deque<CBPlan>::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
						if(i->ud->name == ud->name){
							if(unitpos.distance2D(i->pos) < rmax){
								if(i->started){
									if(G->Actions->Repair(unit,i->subject)){
										G->L.print("discarding, repairing nearby plan instead(3)");
										return UpVector;
									}
								}else{
									return i->pos;
								}
							}
						}
					}
				}
			}
			if(OverlappingPlans(pos,ud) != BPlans->end()){
				G->L.print("discarding, OverlappingPlans(pos,ud) != BPlans->end()");
				return UpVector;
			}
			//
			if((G->UnitDefHelper->IsFactory(builder)	&&(!G->UnitDefHelper->IsHub(builder))&&(G->UnitDefHelper->IsMobile(ud))) == false){
				if(G->cb->CanBuildAt(ud,pos)==false){
					G->L.print("bad result for CanBuildAt returned for " + ud->name);
					return UpVector;
				}
			}

			return pos;
		}
		return pos;
	}*/

	deque<CBPlan* >::iterator CManufacturer::OverlappingPlans(float3 pos,const UnitDef* ud){
		NLOG("CManufacturer::OverlappingPlans");
		if(!BPlans->empty()){
			for(deque<CBPlan* >::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
				if((*i)->utd->GetUnitDef()->name != ud->name){
					if(pos.distance2D((*i)->pos) < (*i)->radius+(max(ud->zsize,ud->xsize)*8)){
						return i;
					}
				}
			}
		}
		return BPlans->end();
	}
	/*
	bool CManufacturer::CBuild(string name, int unit, int spacing){
		NLOG("CManufacturer::CBuild");
		G->L.print("CBuild() :: " + name);
		const UnitDef* uda = G->GetUnitDef(unit);
		const UnitDef* ud = G->GetUnitDef(name);

		/////////////

		if(ud == 0){
			G->L.print("Factor::CBuild(3) error: a problem occurred loading this units unit definition : " + name);
			return false;
		} else if(uda == 0){
			G->L.print("Factor::CBuild(3) error: a problem occurred loading uda ");
			return false;
		}
		if(G->Pl->AlwaysAntiStall.empty() == false){ // Sort out the stuff that's setup to ALWAYS be under the antistall algorithm
			NLOG("CManufacturer::CBuild G->Pl->AlwaysAntiStall.empty() == false");
			for(vector<string>::iterator i = G->Pl->AlwaysAntiStall.begin(); i != G->Pl->AlwaysAntiStall.end(); ++i){
				if(*i == name){
					NLOG("CManufacturer::CBuild *i == name :: "+name);
					if(G->Pl->feasable(ud,uda) == false){
						G->L.print("Factor::CBuild  unfeasable " + name);
						return false;
					}else{
						NLOG("CManufacturer::CBuild  "+name+" is feasable");
						break;
					}
				}
			}
		}
		NLOG("CManufacturer::CBuild  Resource\\MaxEnergy\\");
		float emax=1000000000;
		string key = "Resource\\MaxEnergy\\";
		key += name;
		G->Get_mod_tdf()->GetDef(emax,"3000000",key);// +300k energy per tick by default
		if(emax ==0) emax = 30000000;
		if(G->Pl->GetEnergyIncome() > emax){
			G->L.print("Factor::CBuild  emax " + name);
			return false;
		}
		NLOG("CManufacturer::CBuild  Resource\\MinEnergy\\");
		float emin=0;
		key = "Resource\\MinEnergy\\";
		key += name;
		G->Get_mod_tdf()->GetDef(emin,"0",key);// +0k energy per tick by default
		if(G->Pl->GetEnergyIncome() < emin){
			G->L.print("Factor::CBuild  emin " + name);
			return false;
		}
		// Now sort out stuff that can only be built one at a time

		if(G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()){
			NLOG("CManufacturer::CBuild  G->Cached->solobuilds.find(name)!= G->Cached->solobuilds.end()");
			G->L.print("Factor::CBuild  solobuild " + name);
			return G->Actions->Repair(unit,G->Cached->solobuilds[name]);// One is already being built, change to a repair order to go help it!
		}
		// Now sort out if it's one of those things that can only be built once
		if(G->Cached->singlebuilds.find(name) != G->Cached->singlebuilds.end()){
			NLOG("CManufacturer::CBuild  G->Cached->singlebuilds.find(name) != G->Cached->singlebuilds.end()");
			if(G->Cached->singlebuilds[name] == true){
				G->L.print("Factor::CBuild  singlebuild " + name);
				return false;
			}
		}
		//if(G->Pl->feasable(name,unit)==false){
		//	return false;
		//}
		// If Antistall == 2/3 then we always do this

		if(G->info->antistall>1){
			NLOG("CManufacturer::CBuild  G->info->antistall>1");
			bool fk = G->Pl->feasable(ud,uda);
			NLOG("CManufacturer::CBuild  feasable called");
			if(fk == false){
				G->L.print("Factor::CBuild  unfeasable " + name);
				return false;
			}
		}

		NLOG("CManufacturer::CBuild  mark 1#");

		TCommand tc(unit,"CBuild");
		tc.ID(-ud->id);
		NLOG("CManufacturer::CBuild  mark 2#");
		float3 unitpos = G->GetUnitPos(unit);
		float3 pos=unitpos;
		if(G->Map->CheckFloat3(unitpos) == false){
			NLOG("CManufacturer::CBuild  mark 2# bad float exit");
			return false;
		}
		string hj = name;
		trim(hj);
		tolowercase(hj);
		float rmax = r_ranges[hj];
		if(rmax > 10){
			NLOG("CManufacturer::CBuild  rmax > 10");
			int* funits = new int[10000];
			int fnum = G->cb->GetFriendlyUnits(funits,unitpos,rmax);
			if(fnum > 1){
				//
				for(int i = 0; i < fnum; i++){
					const UnitDef* udf = G->GetUnitDef(funits[i]);
					if(udf == 0) continue;
					if(udf->name == name){
						NLOG("CManufacturer::CBuild  mark 2b#");
						if(G->GetUnitPos(funits[i]).distance2D(unitpos) < rmax){
							if(G->cb->UnitBeingBuilt(funits[i])==true){
								delete [] funits;
								NLOG("CManufacturer::CBuild  exit on repair");
								return G->Actions->Repair(unit,funits[i]);
							}
						}
					}
				}
			}
			delete [] funits;
		}
		NLOG("CManufacturer::CBuild  mark 3#");
		////////
		float exrange = exclusion_ranges[hj];
		if(exrange > 10){
			int* funits = new int[10000];
			int fnum = G->cb->GetFriendlyUnits(funits,unitpos,rmax);
			if(fnum > 1){
				//
				for(int i = 0; i < fnum; i++){
					const UnitDef* udf = G->GetUnitDef(funits[i]);
					if(udf == 0) continue;
					if(udf->name == name){
						NLOG("CManufacturer::CBuild  mark 3a#");
						if(G->GetUnitPos(funits[i]).distance2D(unitpos) < exrange){
							int kj = funits[i];
							delete [] funits;
							if(G->cb->UnitBeingBuilt(kj)==true){
								NLOG("CManufacturer::CBuild  exit on repair");
								return G->Actions->Repair(unit,kj);
							}else{
								NLOG("CManufacturer::CBuild  return false");
								return false;
							}
						}
					}
				}
			}
			delete [] funits;
		}
		//if(ud->type != string("MetalExtractor")){
		//	unitpos = GetBuildPos(unit,ud,unitpos);
		//}
		NLOG("CManufacturer::CBuild  mark 4#");
	//	pos = GetBuildPlacement(unit,unitpos,uda,ud,spacing);
		//if(G->Map->CheckFloat3(pos)==false){
		if(pos==UpVector){
			G->L.print("GetBuildPlacement returned UpVector or some other nasty position");
			return false;
		}
		pos.y = G->cb->GetElevation(pos.x,pos.z);
		deque<CBPlan>::iterator qi = OverlappingPlans(pos,ud);
		if(qi != BPlans->end()){
			NLOG("vector<CBPlan>::iterator qi = OverlappingPlans(pos,ud); :: WipePlansForBuilder");
			if(qi->started){
				G->L.print("::Cbuild overlapping plans issuing repair instead");
				if(G->Actions->Repair(unit,qi->subject)){
					WipePlansForBuilder(unit);
					qi->builders.insert(unit);
					return false;
				}
			}else if (qi->ud == ud){
				G->L.print("::Cbuild overlapping plans that're the same item but not started, moving pos to make it build quicker");
				pos = qi->pos;
			}else{
				G->L.print("::Cbuild overlapping plans that are not the same item, no alternative action, cancelling task");
				return false;
			}
		}
		tc.PushFloat3(pos);
		tc.Push(0);
		tc.c.timeOut = int(ud->buildTime/5) + G->cb->GetCurrentFrame();
		tc.created = G->cb->GetCurrentFrame();
		tc.delay=0;
		if(G->OrderRouter->GiveOrder(tc)== false){
			G->L.print("Error::Cbuild Failed Order G->OrderRouter->GiveOrder(tc)== false:: " + name);
			return false;
		}else{
			// create a plan
			G->L.print("Error::Cbuild G->OrderRouter->GiveOrder(tc)== true :: " + name);
			if(qi == BPlans->end()){
				NLOG("CBuild :: WipePlansForBuilder");
				G->L.print("Error::Cbuild wiping and creaiing the plan :: " + name);
				WipePlansForBuilder(unit);
				CBPlan Bplan;
				Bplan.started = false;
				Bplan.builders.insert(unit);
				Bplan.subject = -1;
				Bplan.pos = pos;
				Bplan.ud=ud;
				plancounter++;
				Bplan.id = plancounter;
				Bplan.radius = (float)max(ud->xsize,ud->ysize)*8.0f;
				Bplan.inFactory = G->UnitDefHelper->IsFactory(uda);
				BPlans->push_back(Bplan);
				G->BuildingPlacer->Block(pos,ud);
				//G->BuildingPlacer->UnBlock(G->GetUnitPos(uid),ud);
				//builders[unit].curplan = plancounter;
				//builders[unit].doingplan = true;
			}
			return true;
		}
	}
	*/

	CManufacturer::CManufacturer(Global* GL){
		G = GL;
		initialized = false;
		valid = true;
		//G->RegisterMessageHandler("update",this);
	}

	CManufacturer::~CManufacturer(){
		G = 0;
	}

	void CManufacturer::RecieveMessage(CMessage &message){
		//G->L.iprint("RecievedMessage");
		if(message.GetType() == string("update")){
			Update();
		}else if(message.GetType() == string("unitidle")){
			//G->L.iprint("RecievedupdateMessage");
			UnitIdle((int)message.GetParameter(0));
		}
	}

	bool CManufacturer::Init(){
		NLOG("CManufacturer::Init");
		NLOG("Loading MetaTags");

		BPlans = new deque<CBPlan* >();

		NLOG("Registering TaskTypes");
		RegisterTaskTypes();


		initialized = true;
		NLOG("Manufacturer Init Done");
		return true;

	}

	void CManufacturer::UnitCreated(int uid){
		NLOG("CManufacturer::UnitCreated");
		// sort out the plans
		if(BPlans->empty() == false){
			//
			float3 upos = G->GetUnitPos(uid);
			if(upos != UpVector){
				for(deque<CBPlan* >::iterator k = BPlans->begin(); k != BPlans->end();++k){
					CBPlan* i = *k;
					if(i->pos.distance2D(upos)<i->radius*1.5f){
						const UnitDef* ud = G->GetUnitDef(uid);
						if(ud==i->utd->GetUnitDef()){
							i->pos = upos;
							i->subject = uid;
							i->started = true;
							break;
						}
					}
				}
			}
		}
		G->RadarHandler->Change(uid,true);
	}

	void CManufacturer::UnitFinished(int uid){
		NLOG("CManufacturer::UnitFinished");
		if(initialized == false){
			G->L.print("CManufacturer::UnitFinished Initialized == false");
			return;
		}

		if(BPlans->empty() == false){
			for(deque<CBPlan* >::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
				if((*i)->subject == uid){
					if(!(*i)->inFactory){
						G->Actions->ScheduleIdle(uid);
					}
					delete (*i);
					BPlans->erase(i);
					break;
				}
			}
		}
		// create a new CBuilder struct
		/*CBuilder b(G,uid);
		if(b.IsValid() == true){
			const UnitDef* ud = b.GetUnitDef();
			if(ud){
				//
				if(ud->builder == true){
					bool setback = false;
					b.SetRepeat(true);
					if(G->UnitDefHelper->IsFactory(ud)){
						b.SetRole(R_FACTORY); // Factory
						setback = true;
					}else if(G->UnitDefHelper->IsHub(ud)){
						b.SetRole(R_BUILDER); // Hub
						setback = true;
					}else{
						b.SetRole(R_BUILDER); // builder!
						setback = true;
					}
					G->L.print("CManufacturer::UnitFinished role set");
					LoadTaskList(&b);
					G->L.print("CManufacturer::UnitFinished buildtree loaded");
					builders[uid]=b;
					G->L.print("CManufacturer::UnitFinished builder pushed back");
					//	if (setback == true){
					if(G->cb->GetCurrentFrame() > 10) G->Actions->ScheduleIdle(uid);
					//	}
				}
			}
		}else{
			G->L.print("cbuilder->valid() == false");
		}*/
	}

	CBPlan::CBPlan(){
		//plan_mutex = new boost::mutex();
		bcount = 0;
	}

	CBPlan::~CBPlan(){
		//delete plan_mutex;
	}

	void CBPlan::AddBuilder(int i){
		//boost::mutex::scoped_lock lock(plan_mutex);
		bcount++;
	}

	bool CBPlan::HasBuilders(){
		//boost::mutex::scoped_lock lock(plan_mutex);
		return bcount >0;
	}

	void CBPlan::RemoveBuilder(int i){
		//boost::mutex::scoped_lock lock(plan_mutex);
		bcount--;
	}

	void CBPlan::RemoveAllBuilders(){
		//boost::mutex::scoped_lock lock(plan_mutex);
		bcount = 0;
	}

	int CBPlan::GetBuilderCount(){
		//boost::mutex::scoped_lock lock(plan_mutex);
		return bcount;
	}

	void CManufacturer::UnitIdle(int uid){
		NLOG("CManufacturer::UnitIdle");
		if(!ValidUnitID(uid)) return;
		//if(builders.empty() == false){
			/*bool idle = false;
			for(map<int,CBuilder>::iterator i = builders.begin(); i != builders.end(); ++i){
				if(i->second.GetID() == uid){
					/*if(G->Actions->DGunNearby(uid)==true){
					idle = false;
					break;
					}*/
					//G->L.print("CManufacturer::TaskCycle has found the unitID, issuing TaskCycle");
					/*bool result = TaskCycle(&i->second);
					idle = (!result);
					break;
				}
			}
			if(idle){
				G->Actions->ScheduleIdle(uid);
			}
			return;
		}*/
	}

	void CManufacturer::UnitDestroyed(int uid){
		NLOG("CManufacturer::UnitDestroyed");
		//get rid of CBuilder, destroy plans needing this unit
		if(BPlans->empty() == false){
			for(deque<CBPlan* >::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
				if((*i)->subject == uid){
					delete (*i);
					BPlans->erase(i);
					break;
				}
			}
		}
		//builders.erase(uid);
		G->RadarHandler->Change(uid,false);
	}

	void CManufacturer::Update(){
		NLOG("CManufacturer::Update");
		/*if(G->cb->GetCurrentFrame() == 30){ // kick start the commander and any other starting units
			if(builders.empty() == false){
				for(map<int,CBuilder>::iterator i = builders.begin();i != builders.end(); ++i){
					UnitIdle(i->second.GetID());
				}
			}
		}*/


		if(!G->L.IsVerbose()==false){
			return;
		}
		if EVERY_SECOND{
			// iterate through all the plans and print out their positions and helping builders to verify that the plan system is actually working
			// and keeping track of construction....
			if(!BPlans->empty() && G->L.IsVerbose()){
				for(deque<CBPlan* >::iterator i =  BPlans->begin(); i != BPlans->end(); ++i){
					if((*i)->HasBuilders()){
						//for(set<int>::iterator j = i->builders.begin(); j != i->builders.end(); ++j){
							float3 bpos = (*i)->pos;//G->GetUnitPos(*j);
							float3 upos = bpos + float3(0,100,0);
							int q = G->cb->CreateLineFigure(upos,(*i)->pos,15,1,30,0);
							string s = "GAME\\TEAM" + to_string(G->Cached->team) + "\\RGBColor";
							//char ck[60];
							//sprintf(ck,"GAME\\TEAM%i\\RGBColor",G->Cached->team);
							float3 r = G->L.startupscript->GetFloat3(ZeroVector,s.c_str());
							G->cb->SetFigureColor(q,r.x,r.y,r.z,0.3f);
							G->cb->DrawUnit((*i)->utd->GetUnitDef()->name.c_str(),(*i)->pos,0,30,G->Cached->team,true,true);
							int w = (*i)->GetBuilderCount();
							SkyWrite k(G->cb);
							float3 jpos = (*i)->pos;
							jpos.x+= (*i)->radius/2 +10;
							string v = to_string(w)+"::"+to_string((*i)->started);
							k.Write(v,jpos,15,10,30,r.x,r.y,r.z);
						//}
					}
				}
			}
		}


	}

	//af CManufacturer::RegisterTaskPair
	void CManufacturer::RegisterTaskPair(string name, btype type){
		types[name] = type;
		typenames[type] = name;
	}

	void CManufacturer::RegisterTaskTypes(){
		NLOG("CManufacturer::RegisterTaskTypes");
		if(types.empty()){

			RegisterTaskPair("b_metatag_failed", B_METAFAILED);
			RegisterTaskPair("b_solar", B_POWER);
			RegisterTaskPair("b_power",B_POWER);
			RegisterTaskPair("b_mex", B_MEX);
			RegisterTaskPair("b_rand_assault", B_RAND_ASSAULT);
			RegisterTaskPair("b_assault", B_ASSAULT);
			RegisterTaskPair("b_factory_cheap", B_FACTORY_CHEAP);
			RegisterTaskPair("b_factory", B_FACTORY);
			RegisterTaskPair("b_builder", B_BUILDER);
			RegisterTaskPair("b_geo", B_GEO);
			RegisterTaskPair("b_scout", B_SCOUT);
			RegisterTaskPair("b_random", B_RANDOM);
			RegisterTaskPair("b_defence", B_DEFENCE);
			RegisterTaskPair("b_defense", B_DEFENCE);
			RegisterTaskPair("b_radar", B_RADAR);
			RegisterTaskPair("b_estore", B_ESTORE);
			RegisterTaskPair("b_targ", B_TARG);
			RegisterTaskPair("b_mstore", B_MSTORE);
			RegisterTaskPair("b_silo", B_SILO);
			RegisterTaskPair("b_jammer", B_JAMMER);
			RegisterTaskPair("b_sonar", B_SONAR);
			RegisterTaskPair("b_antimissile", B_ANTIMISSILE);
			RegisterTaskPair("b_artillery", B_ARTILLERY);
			RegisterTaskPair("b_focal_mine", B_FOCAL_MINE);
			RegisterTaskPair("b_sub", B_SUB);
			RegisterTaskPair("b_amphib", B_AMPHIB);
			RegisterTaskPair("b_mine", B_MINE);
			RegisterTaskPair("b_carrier", B_CARRIER);
			RegisterTaskPair("b_metal_maker", B_METAL_MAKER);
			RegisterTaskPair("b_fortification", B_FORTIFICATION);
			RegisterTaskPair("b_dgun", B_DGUN);
			RegisterTaskPair("b_repair", B_REPAIR);
			RegisterTaskPair("repair", B_REPAIR);
			RegisterTaskPair("reclaim", B_RECLAIM);
			RegisterTaskPair("retreat", B_RETREAT);
			RegisterTaskPair("b_idle", B_IDLE);
			RegisterTaskPair("b_cmd", B_CMD);
			RegisterTaskPair("b_randmove", B_RANDMOVE);
			RegisterTaskPair("b_resurrect", B_RESURECT);
			RegisterTaskPair("b_ressurect", B_RESURECT);
			RegisterTaskPair("b_resurect", B_RESURECT);
			RegisterTaskPair("b_global", B_RULE);
			RegisterTaskPair("b_rule", B_RULE);
			RegisterTaskPair("b_rule_extreme", B_RULE_EXTREME);
			RegisterTaskPair("b_rule_extreme_nofact", B_RULE_EXTREME_NOFACT);
			RegisterTaskPair("b_rule_extreme_carry", B_RULE_EXTREME_CARRY);
			RegisterTaskPair("b_rule_carry", B_RULE_EXTREME_CARRY);
			RegisterTaskPair("b_retreat", B_RETREAT);
			RegisterTaskPair("b_guardian", B_GUARDIAN);
			RegisterTaskPair("b_bomber", B_BOMBER);
			RegisterTaskPair("b_gunship", B_GUNSHIP);
			RegisterTaskPair("b_plasma_repulsor", B_SHIELD);
			RegisterTaskPair("b_plasma_repulser", B_SHIELD);
			RegisterTaskPair("b_shield", B_SHIELD);
			RegisterTaskPair("b_missile_unit", B_MISSILE_UNIT);
			RegisterTaskPair("b_na", B_NA);
			RegisterTaskPair("b_fighter", B_FIGHTER);
			RegisterTaskPair("b_guard_factory", B_GUARD_FACTORY);
			RegisterTaskPair("b_guard_like_con", B_GUARD_LIKE_CON);
			RegisterTaskPair("b_reclaim", B_RECLAIM);
			RegisterTaskPair("b_wait", B_WAIT);
			RegisterTaskPair("b_hub", B_HUB);
			RegisterTaskPair("b_airsupport", B_AIRSUPPORT);
			RegisterTaskPair("b_offensive_repair_retreat", B_OFFENSIVE_REPAIR_RETREAT);
			RegisterTaskPair("b_guardian_mobiles", B_GUARDIAN_MOBILES);
		}
	}

	btype CManufacturer::GetTaskType(string s){
		NLOG("CManufacturer::GetTaskType");
		trim(s);
		tolowercase(s);

		if(types.empty()==false){
			if(types.find(s) != types.end()){
				return types[s];
			}
		}
		return B_NA;
	}

	string CManufacturer::GetTaskName(btype type){
		NLOG("CManufacturer::GetTaskName");
		if(typenames.empty()==false){
			if(typenames.find(type) != typenames.end()){
				return typenames[type];
			}
		}
		return "";
	}

	bool CManufacturer::CanBuild(int uid, const UnitDef* ud, string name){
		NLOG("CManufacturer::CanBuild");
		string n = name;
		tolowercase(n);
		trim(n);
		if(ud == 0){
			G->L <<"CManufacturer::CanBuild(" <<uid << ",const UnitDef* ud," <<  name << ") gave false because ud == 0" << endline;
			return false;
		}
		if(ud->buildOptions.empty()){
			G->L << "CManufacturer::CanBuild(" <<uid << ",const UnitDef* ud," <<  name << ") gave false because ud->buildOptions.empty() == true" << endline;
			return false;
		}else{
			for(map<int,string>::const_iterator i = ud->buildOptions.begin(); i !=ud->buildOptions.end(); ++i){
				string k = i->second;
				tolowercase(k);
				trim(k);
				if(k==n){
					return true;
				}
			}
			return false;
		}
	}

	/*bool CManufacturer::LoadTaskList(CBuilder* ui){
		NLOG("CManufacturer::LoadTaskList");
		if((ui->GetUnitDef()->canfly ||ui->GetUnitDef()->movedata) &&  (ui->GetAge() < (5 SECONDS))){
			G->Actions->ScheduleIdle(ui->GetID());
			return true;
		}
		const UnitDef* ud = ui->GetUnitDef();
		G->L.print("loading buildtree for "+ud->name);
		// get the list of filenames
		vector<string> vl;
		string sl;
		if(G->Cached->cheating){
			sl= G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\CHEAT\\")+ud->name);
		}else{
			sl = G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\NORMAL\\")+ud->name);
		}
		tolowercase(sl);
		trim(sl);
		string u = ud->name;
		if(sl != string("")){
			vl = bds::set_cont(vl,sl.c_str());
			if(vl.empty() == false){
				int randnum = G->mrand()%vl.size();
				u = vl.at(min(randnum,max(int(vl.size()-1),1)));
			}
		}
		string s = G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\LISTS\\")+u);
		vector<string> v;
		//string s = *buffer;
		if(s.empty() == true){
			G->L.print(" error loading tasklist :: " + u + " :: buffer empty, most likely because of an empty list");
			return false;
		}

		tolowercase(s);
		trim(s);
		v = bds::set_cont(v,s.c_str());

		if(v.empty() == false){
			G->L.print("loading contents of  tasklist :: " + u + " :: filling tasklist with #" + to_string(v.size()) + " items");
			bool polate=false;
			bool polation = G->info->rule_extreme_interpolate;
			btype bt = GetTaskType(G->Get_mod_tdf()->SGetValueDef("b_rule_extreme_nofact","AI\\interpolate_tag"));
			if(ui->GetRole() == R_FACTORY){
				polation = false;
			}
			if(bt == B_NA){
				polation = false;
			}
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				if(polation==true){
					if(polate==true){
						ui->AddTask(bt);
						polate = false;
					}else{
						polate=true;
					}
				}
				string q = *vi;
				trim(q);
				tolowercase(q);
				if(metatags.empty() ==false){
					if (metatags.find(q) != metatags.end()){
						string s = "";
						s = GetBuild(ui->GetID(),q,true);
						if((s != q)&&(s != string(""))&&(s != string("b_metatag_failed"))){
							ui->AddTask(s,false);
						}else{
							G->L.print(" error with metatag!!!!!!! ::  "+ s + " :: " + *vi);
						}
						continue;
					}
				}
				const UnitDef* uj = G->GetUnitDef(*vi);
				if(uj != 0){
					ui->AddTask(q,false);
					if(v.size() == 1) ui->AddTask(q,false);
				}else if(q == string("")){
					continue;
				} else if(q == string("is_factory")){
					ui->SetRole(R_FACTORY);
				} else if(q == string("is_builder")){
					ui->SetRole(R_BUILDER);
				} else if(q == string("repeat")){
					ui->SetRepeat(true);
				} else if(q == string("no_rule_interpolation")){
					polation=false;
				} else if(q == string("rule_interpolate")){
					polation=true;
				} else if(q == string("dont_repeat")){
					ui->SetRepeat(false);
				}else if(q == string("base_pos")){
					G->Map->base_positions.push_back(G->GetUnitPos(ui->GetID()));
				} else if(q == string("gaia")){
					G->info->gaia = true;
				} else if(q == string("not_gaia")){
					G->info->gaia = false;
				} else if(q == string("switch_gaia")){
					G->info->gaia = !G->info->gaia;
				} else{
					btype x = GetTaskType(q);
					if( x != B_NA){
						ui->AddTask(x);
					}else{
						G->L.iprint("error :: a value :: " + *vi +" :: was parsed in :: "+u + " :: this does not have a valid UnitDef according to the engine, and is not a Task keyword such as repair or b_mex");
					}
				}
			}
			if(ud->isCommander == true)	G->Map->basepos = G->GetUnitPos(ui->GetID());
			if((ud->isCommander== true)||(ui->GetRole() == R_FACTORY))	G->Map->base_positions.push_back(G->GetUnitPos(ui->GetID()));
			G->L.print("loaded contents of  tasklist :: " + u + " :: loaded tasklist at " + to_string(ui->tasks.size()) + " items");
			return true;
			//G->Actions->ScheduleIdle(ui->GetID());
		} else{
			G->L.print(" error loading contents of  tasklist :: " + u + " :: buffer empty, most likely because of an empty tasklist");
			return false;
		}
	}*/

	/*int CManufacturer::WhatIsUnitBuilding(int builder){
		NLOG("WhatIsUnitBuilding");
		if(builder < 0) return -1;
		if (BPlans->empty()) return -1;

		for(deque<CBPlan* >::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
			if((*i)->HasBuilder(builder)){
				return (*i)->subject;
			}
		}
		return -1;
	}

	bool CManufacturer::UnitTargetStartedBuilding(int builder){
		NLOG("UnitTargetStartedBuilding");
		if(builder < 0) return false;
		if (BPlans->empty()) return false;
		for(deque<CBPlan* >::iterator i = BPlans->begin(); i != BPlans->end(); ++i){
			if((*i)->HasBuilder(builder)){
				return (*i)->started;
			}
		}
		return false;
	}*/

	SkyWrite::SkyWrite(IAICallback *_GS):
	GS(_GS){
	}
	// TODO: HIKLMNOPQRSTUVWXYZ,/&%!_-+=
	SkyWrite::~SkyWrite(){
	}
	void SkyWrite::Write(string Text, float3 loc, float Height, float Width, int Duration, float Red, float Green, float Blue, float Alpha){
		int Group = 0;
		int bGroup = 0;
		float pos = 0;
		// loc = bottom, left hand corner
		const char* c = Text.c_str();
		for(int n=0; n < (int)strlen(c); n++)
			switch(Text[n])	{
				case 'A':
					{
						// Upward, downward, center
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z),
							float3(loc.x + pos + Width/2, loc.y, loc.z+Height),
							3, 0, Duration, Group);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z),
							float3(loc.x + pos + Width/2, loc.y-10, loc.z+Height),
							5, 0, Duration, bGroup);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos + Width/2, loc.y, loc.z+Height),
							float3(loc.x + pos + Width, loc.y, loc.z),
							3, 0, Duration, Group);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos + Width/2, loc.y-10, loc.z+Height),
							float3(loc.x + pos + Width, loc.y-10, loc.z),
							5, 0, Duration, bGroup);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos + Width/4, loc.y, loc.z+Height/2),
							float3(loc.x + pos + Width*3/4, loc.y, loc.z+Height/2),
							3, 0, Duration, Group);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos + Width/4, loc.y-10, loc.z+Height/2),
							float3(loc.x + pos + Width*3/4, loc.y-10, loc.z+Height/2),
							5, 0, Duration, bGroup);
						//
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						pos += Width * 0.7f;
						break;
					}
				case 'B':
					{
						// Left Side, Top Bubble, Bottom Bubble
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z),
							float3(loc.x + pos, loc.y, loc.z+Height),
							3, 0, Duration, Group);
						Group = GS->CreateSplineFigure(
							float3(loc.x + pos, loc.y+Height, loc.z),
							float3(loc.x + pos + Width, loc.y, loc.z+Height),
							float3(loc.x + pos + Width, loc.y, loc.z+Height/2),
							float3(loc.x + pos, loc.y, loc.z+Height/2),
							3, 0, Duration, Group);
						Group = GS->CreateSplineFigure(
							float3(loc.x + pos, loc.y, loc.z+Height/2),
							float3(loc.x + pos + Width, loc.y, loc.z+Height/2),
							float3(loc.x + pos + Width, loc.y, loc.z),
							float3(loc.x + pos, loc.y, loc.z),
							3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z),
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							5, 0, Duration, bGroup);
						bGroup = GS->CreateSplineFigure(
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							float3(loc.x + pos + Width, loc.y-10, loc.z+Height),
							float3(loc.x + pos + Width, loc.y-10, loc.z+Height/2),
							float3(loc.x + pos, loc.y-10, loc.z+Height/2),
							5, 0, Duration, bGroup);
						bGroup = GS->CreateSplineFigure(
							float3(loc.x + pos, loc.y-10, loc.z+Height/2),
							float3(loc.x + pos + Width, loc.y-10, loc.z+Height/2),
							float3(loc.x + pos + Width, loc.y-10, loc.z),
							float3(loc.x + pos, loc.y-10, loc.z),
							5, 0, Duration, bGroup);
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						pos += Width * 0.7f;
						break;
					}
				case 'C':
					{
						Group = GS->CreateSplineFigure(
							float3(loc.x + pos + Width, loc.y, loc.z+Height),
							float3(loc.x + pos, loc.y, loc.z+Height),
							float3(loc.x + pos, loc.y, loc.z),
							float3(loc.x + pos + Width, loc.y, loc.z),
							3, 0, Duration, Group);
						//
						bGroup = GS->CreateSplineFigure(
							float3(loc.x + pos + Width, loc.y-10, loc.z+Height),
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							float3(loc.x + pos, loc.y-10, loc.z),
							float3(loc.x + pos + Width, loc.y-10, loc.z),
							5, 0, Duration, bGroup);
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						pos += Width * 0.7f;
						break;
					}
				case 'D':
					{
						// Left Side, Bubble
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z),
							float3(loc.x + pos, loc.y, loc.z+Height),
							3, 0, Duration, Group);
						Group = GS->CreateSplineFigure(
							float3(loc.x + pos, loc.y, loc.z),
							float3(loc.x + pos + Width, loc.y, loc.z),
							float3(loc.x + pos + Width, loc.y, loc.z+Height),
							float3(loc.x + pos, loc.y, loc.z+Height),
							3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z),
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							5, 0, Duration, bGroup);
						bGroup = GS->CreateSplineFigure(
							float3(loc.x + pos, loc.y-10, loc.z),
							float3(loc.x + pos + Width, loc.y-10, loc.z),
							float3(loc.x + pos + Width, loc.y-10, loc.z+Height),
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case 'E':
					{
						// Left, Top, Middle, Bottom
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z),
							float3(loc.x + pos, loc.y, loc.z+Height),
							3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z+Height),
							float3(loc.x + pos + Width, loc.y, loc.z+Height),
							3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z+Height/2),
							float3(loc.x + pos + Width*3/4, loc.y, loc.z+Height/2),
							3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z),
							float3(loc.x + pos + Width, loc.y, loc.z),
							3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z),
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							float3(loc.x + pos + Width, loc.y-10, loc.z+Height),
							5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z+Height/2),
							float3(loc.x + pos + Width*3/4, loc.y-10, loc.z+Height/2),
							5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z),
							float3(loc.x + pos + Width, loc.y-10, loc.z),
							5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case 'F':
					{
						// Left, Top, Middle
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z),
							float3(loc.x + pos, loc.y, loc.z+Height),
							3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z+Height),
							float3(loc.x + pos + Width, loc.y, loc.z+Height),
							3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z+Height/2),
							float3(loc.x + pos + Width*3/4, loc.y, loc.z+Height/2),
							3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z),
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							float3(loc.x + pos + Width, loc.y-10, loc.z+Height),
							5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z+Height/2),
							float3(loc.x + pos + Width*3/4, loc.y-10, loc.z+Height/2),
							5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case 'G':
					{
						Group = GS->CreateSplineFigure(
							float3(loc.x + pos + Width, loc.y, loc.z+Height),
							float3(loc.x + pos, loc.y, loc.z+Height),
							float3(loc.x + pos + Width, loc.y, loc.z),
							float3(loc.x + pos + Width, loc.y , loc.z+ Height/2),
							3, 0, Duration, Group);
						//
						bGroup = GS->CreateSplineFigure(
							float3(loc.x + pos + Width, loc.y-10, loc.z+Height),
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							float3(loc.x + pos + Width, loc.y-10, loc.z),
							float3(loc.x + pos + Width, loc.y-10 , loc.z+ Height/2),
							5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case 'I':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y, loc.z+Height),
							float3(loc.x + pos,loc.y, loc.z), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos, loc.y-10, loc.z+Height),
							float3(loc.x + pos,loc.y-10, loc.z), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '1':
					{
						// Left, Top, Middle
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.5f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.5f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.5f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.5f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '2':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.1f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*1.0f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*1.0f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.3f),loc.y, loc.z+(Height*1.0f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.1f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*1.0f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*1.0f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.3f),loc.y-10, loc.z+(Height*1.0f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '3':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.1f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*1.0f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*1.0f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*1.0f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.1f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*1.0f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*1.0f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*1.0f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '4':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '5':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.1f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.1f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '6':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '7':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.1f)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.1f)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '8':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.1f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.1f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '9':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.1f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.1f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '0':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height*0.1f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height)),
							float3(loc.x + pos+(Width*0.7f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.7f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height*0.1f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height)),
							float3(loc.x + pos+(Width*0.7f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case ':':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.5f), loc.y, loc.z+(Height*0.4f)),
							float3(loc.x + pos+(Width*0.5f),loc.y, loc.z+(Height*0.5f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.5f), loc.y, loc.z+(Height*0.7f)),
							float3(loc.x + pos+(Width*0.5f),loc.y, loc.z+(Height*0.8f)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.5f), loc.y-10, loc.z+(Height*0.4f)),
							float3(loc.x + pos+(Width*0.5f),loc.y-10, loc.z+(Height*0.5f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.5f), loc.y-10, loc.z+(Height*0.7f)),
							float3(loc.x + pos+(Width*0.5f),loc.y-10, loc.z+(Height*0.8f)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '¬':// the lightning power symbol  FIXME ???
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.8f),loc.y, loc.z+(Height*0.1f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.8f), loc.y, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.6f),loc.y, loc.z+(Height*0.6f)), 3, 0, Duration, Group);
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.6f), loc.y, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.2f),loc.y, loc.z+(Height)), 3, 0, Duration, Group);
						//
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.8f),loc.y-10, loc.z+(Height*0.1f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.8f), loc.y-10, loc.z+(Height*0.1f)),
							float3(loc.x + pos+(Width*0.3f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.3f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.6f),loc.y-10, loc.z+(Height*0.6f)), 5, 0, Duration, bGroup);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.6f), loc.y-10, loc.z+(Height*0.6f)),
							float3(loc.x + pos+(Width*0.2f),loc.y-10, loc.z+(Height)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case '.':
					{
						Group = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.5f), loc.y, loc.z+(Height*0.9f)),
							float3(loc.x + pos+(Width*0.6f),loc.y, loc.z+(Height*0.9f)), 3, 0, Duration, Group);
						bGroup = GS->CreateLineFigure(
							float3(loc.x + pos+(Width*0.5f), loc.y-10, loc.z+(Height*0.9f)),
							float3(loc.x + pos+(Width*0.6f),loc.y-10, loc.z+(Height*0.9f)), 5, 0, Duration, bGroup);
						pos += Width * 0.7f;
						GS->SetFigureColor(Group,Red,Green,Blue,Alpha);
						GS->SetFigureColor(bGroup,0,0,0,0.5f);
						break;
					}
				case ' ':
					{
						pos += Width * 0.7f;
						break;
					}
				default:
					{
						pos += Width * 0.7f;
						break;
					}
			}
	}
}

/** unitdef->type
MetalExtractor
Transport
Builder
Factory
Bomber
Fighter
GroundUnit
Building
**/
