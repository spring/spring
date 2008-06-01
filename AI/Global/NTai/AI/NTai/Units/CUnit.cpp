
#include "../Core/include.h"

namespace ntai {

	CUnit::CUnit(Global* GL, int uid){
		G = GL;
		
		NLOG("CUnit::CUnit");
		
		valid = true;
		
		if(!ValidUnitID(uid)){
			valid = false;
		}
		
		this->uid = uid;
		
		utd = G->UnitDefLoader->GetUnitTypeDataByUnitId(uid);

		if(utd==0){
			//
			G->L.eprint("ERROR IN CUNIT :: UTD == 0, ntai failed to retrieve the units unitdef object, something has gone wrong somewhere.");
			valid = false;
		}
		
		if(!valid){
			return;
		}

		if(!utd->IsMobile()){
			G->BuildingPlacer->Block(G->GetUnitPos(uid),utd);
		}
		currentTask = boost::shared_ptr<IModule>();
		doingplan=false;
		curplan=0;
		birth = G->GetCurrentFrame();
		//nolist=false;
		under_construction = true;
	}

	CUnit::~CUnit(){
		//
	}

	bool CUnit::Init(){

		NLOG("CUnit::Init");

		if((G->GetCurrentFrame() > 32)&&utd->IsMobile()){
			currentTask = boost::shared_ptr<IModule>(new CLeaveBuildSpotTask(G,uid,utd));
			currentTask->Init();
			G->RegisterMessageHandler(currentTask);
		}
		return true;
	}

	void CUnit::RecieveMessage(CMessage &message){
		NLOG("CUnit::RecieveMessage");
		if(!IsValid()){
			return;
		}
		if(message.GetType() == string("update")){
			if(under_construction){
				return;
			}

			if(EVERY_((GetAge()%16+17))){
				if(currentTask.get() != 0){
					if(currentTask->IsValid()==false){
						//
						taskManager->TaskFinished();
						if(taskManager->HasTasks()){
							currentTask = taskManager->GetNextTask();
							currentTask->Init();
							G->RegisterMessageHandler(currentTask);
						}else{
							currentTask = boost::shared_ptr<IModule>();
						}
					}
				}else{
					//
					if(taskManager->HasTasks()){
						currentTask = taskManager->GetNextTask();
						currentTask->Init();
						G->RegisterMessageHandler(currentTask);
					}
				}

			}
		}else if(message.GetType() == string("")){
			return;
		}else if(message.GetType() == string("unitfinished")){
			if(message.GetParameter(0) == this->uid){
				under_construction = false;
				LoadBehaviours();
			}
		}else if(message.GetType() == string("unitdestroyed")){
			if(message.GetParameter(0) == uid){

				if(!utd->IsMobile()){
					G->BuildingPlacer->UnBlock(G->GetUnitPos(uid),utd);
				}

				G->RemoveHandler(currentTask);
				taskManager->RemoveAllTasks();
				G->RemoveHandler(taskManager);

				if(!behaviours.empty()){
					behaviours.erase(behaviours.begin(),behaviours.end());
					behaviours.clear();
				}
				this->End();
				return;
			}
		}
		if(under_construction) return;
		/*if(!nolist){
			if(tasks.empty()){
				if(LoadTaskList()){
					boost::shared_ptr<IModule> t = tasks.front();
					t->Init();
					G->RegisterMessageHandler(t);
				}
				//executenext = !tasks.empty();
			}
		}*/
	}

	int CUnit::GetAge(){
		NLOG("CUnit::GetAge");
		return G->GetCurrentFrame()-birth;
	}

	CUnitTypeData* CUnit::GetUnitDataType(){
		NLOG("CBuilder::GetUnitDataType");
		return utd;
	}

	bool CUnit::operator==(int unit){
		NLOG("CUnit::operator==");
		if(valid == false) return false;
		if(unit == uid){
			return true;
		}else{
			return false;
		}
	}

	int CUnit::GetID(){
		NLOG("CUnit::GetID");
		return uid;
	}

	/*bool CUnit::AddTask(boost::shared_ptr<IModule> &t){
		NLOG("CBuilder::AddTask");
		//if(t->IsValid()){
			//t->AddListener(me);
			tasks.push_back(t);
			return true;
		//}
		//return false;
	}

	bool CUnit::LoadTaskList(){
		NLOG("CUnit::LoadTaskList");


		vector<string> vl;
		string sl;
		if(G->Cached->cheating){
			sl= G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\CHEAT\\")+utd->GetName());
		}else{
			sl = G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\NORMAL\\")+utd->GetName());
		}

		tolowercase(sl);
		trim(sl);
		string u = utd->GetName();
		if(sl != string("")){
			CTokenizer<CIsComma>::Tokenize(vl, sl, CIsComma());
			//vl = bds::set_cont(vl,sl.c_str());
			if(vl.empty() == false){
				int randnum = G->mrand()%vl.size();
				u = vl.at(min(randnum,max(int(vl.size()-1),1)));
			}
		}

		string s = G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\LISTS\\")+u);

		if(s.empty() == true){
			G->L.print(" error loading tasklist for unit :: \"" + u + "\" :: buffer empty, most likely because of an empty list");
			nolist=true;
			return false;
		}

		tolowercase(s);
		trim(s);

		vector<string> v;

		CTokenizer<CIsComma>::Tokenize(v, s, CIsComma());
		//v = bds::set_cont(v,s.c_str());

		if(v.empty() == false){
			G->L.print("loading contents of  tasklist :: " + u + " :: filling tasklist with #" + to_string(v.size()) + " items");
			bool polate=false;
			bool polation = G->info->rule_extreme_interpolate;
			btype bt = G->Manufacturer->GetTaskType(G->Get_mod_tdf()->SGetValueDef("b_na","AI\\interpolate_tag"));
			if(utd->IsFactory()){
				polation = false;
			}
			if(bt == B_NA){
				polation = false;
			}


			// TASKS LOADING
			
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
				if(polation){
					if(polate){
						boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,uid,bt));
						AddTask(t);
					}
					polate = !polate;
				}
				string q = *vi;
				trim(q);
				tolowercase(q);
				CUnitTypeData* b = G->UnitDefLoader->GetUnitTypeDataByName(q);
				if(b != 0){
					boost::shared_ptr<IModule> t(new CUnitConstructionTask(G,uid,utd,b));
					AddTask(t);
				}else if(q == string("")){
					continue;
				}else if(q == string("b_na")){
					continue;
				} else if(q == string("no_rule_interpolation")){
					polation=false;
				} else if(q == string("rule_interpolate")){
					polation=true;
				}else if(q == string("base_pos")){
					G->Map->base_positions.push_back(G->GetUnitPos(GetID()));
				} else if(q == string("gaia")){
					G->info->gaia = true;
				} else if(q == string("not_gaia")){
					G->info->gaia = false;
				} else if(q == string("switch_gaia")){
					G->info->gaia = !G->info->gaia;
				} else if(q == string("b_factory")){
					boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,uid,B_FACTORY));
					AddTask(t);
				} else if(q == string("b_power")){
					boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,uid,B_POWER));
					AddTask(t);
				} else if(q == string("b_defence")){
					boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,uid,B_DEFENCE));
					AddTask(t);
				}  else if(q == string("b_mex")){
					boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,uid,B_MEX));
					AddTask(t);
				} else{
					btype x = G->Manufacturer->GetTaskType(q);
					if( x != B_NA){
						boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,this->uid,x));
						AddTask(t);
					}else{
						G->L.print("error :: a value :: " + *vi +" :: was parsed in :: "+u + " :: this does not have a valid UnitDef according to the engine, and is not a Task keyword such as repair or b_mex");
					}
				}

			}
			if(utd->GetUnitDef()->isCommander){
				G->Map->basepos = G->GetUnitPos(GetID());
			}

			G->L.print("loaded contents of  tasklist :: " + u + " :: loaded tasklist at " + to_string(tasks.size()) + " items");
			return !tasks.empty();
		} else{
			G->L.print(" error loading contents of  tasklist :: " + u + " :: buffer empty, most likely because of an empty tasklist");
			return false;
		}
	}*/

	bool CUnit::LoadBehaviours(){
		string d = G->Get_mod_tdf()->SGetValueDef("auto","AI\\behaviours\\"+utd->GetName());

		vector<string> v;
		CTokenizer<CIsComma>::Tokenize(v, d, CIsComma());

		if(!v.empty()){
			for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){

				string s = *vi;

				trim(s);
				tolowercase(s);

				if(s == "none"){
					return true;
				} else if(s == "metalmaker"){
					CMetalMakerBehaviour* m = new CMetalMakerBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(m);
					
					behaviours.push_back(t);
					t->Init();

					G->RegisterMessageHandler(t);
				} else if(s == "attacker"){
					CAttackBehaviour* a = new CAttackBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(a);
					
					behaviours.push_back(t);
					t->Init();

					G->RegisterMessageHandler(t);
				} else if(s == "dgun"){
					CDGunBehaviour* a = new CDGunBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(a);
					
					behaviours.push_back(t);
					t->Init();

					G->RegisterMessageHandler(t);
				} else if(s == "retreat"){
					CRetreatBehaviour* a = new CRetreatBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(a);
					
					behaviours.push_back(t);
					t->Init();

					G->RegisterMessageHandler(t);
				} else if(s == "kamikaze"){
					CKamikazeBehaviour* a = new CKamikazeBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(a);
					
					behaviours.push_back(t);
					t->Init();

					G->RegisterMessageHandler(t);
				} else if(s == "staticdefence"){
					CStaticDefenceBehaviour* a = new CStaticDefenceBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(a);
					
					behaviours.push_back(t);
					t->Init();

					G->RegisterMessageHandler(t);
				} else if(s == "movefailreclaim"){
					CMoveFailReclaimBehaviour* a = new CMoveFailReclaimBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(a);
					
					behaviours.push_back(t);
					t->Init();

					G->RegisterMessageHandler(t);
				} else if(s == "auto"){
					// we have to decide what this units behaviours should be automatically
					// check each type of unit for pre-requisites and then assign the behaviour
					// accordingly.

					if(utd->IsAttacker()){
						CAttackBehaviour* a = new CAttackBehaviour(G, GetID());
						boost::shared_ptr<IBehaviour> t(a);

						behaviours.push_back(t);
						t->Init();

						G->RegisterMessageHandler(t);
					}

					if(utd->IsMetalMaker()||(utd->IsMex() && utd->GetUnitDef()->onoffable ) ){
						CMetalMakerBehaviour* m = new CMetalMakerBehaviour(G, GetID());
						boost::shared_ptr<IBehaviour> t(m);

						behaviours.push_back(t);
						t->Init();

						G->RegisterMessageHandler(t);
						
					}
					
					if(utd->CanDGun()){
						CDGunBehaviour* a = new CDGunBehaviour(G, GetID());
						boost::shared_ptr<IBehaviour> t(a);
						
						behaviours.push_back(t);
						t->Init();

						G->RegisterMessageHandler(t);
					}

					

					if(utd->GetUnitDef()->canmove || utd->GetUnitDef()->canfly){
						CRetreatBehaviour* a = new CRetreatBehaviour(G, GetID());
						boost::shared_ptr<IBehaviour> t(a);
						
						behaviours.push_back(t);
						t->Init();

						G->RegisterMessageHandler(t);

						if(utd->GetUnitDef()->canReclaim){
							CMoveFailReclaimBehaviour* a = new CMoveFailReclaimBehaviour(G, GetID());
							boost::shared_ptr<IBehaviour> t(a);
							
							behaviours.push_back(t);
							t->Init();

							G->RegisterMessageHandler(t);
						}

					}else{
						/* this unit can't move, if it can fire a weapon though give it
						   the static defence behaviour*/

						if(utd->GetUnitDef()->weapons.empty()==false){
							CStaticDefenceBehaviour* a = new CStaticDefenceBehaviour(G, GetID());
							boost::shared_ptr<IBehaviour> t(a);
							
							behaviours.push_back(t);
							t->Init();

							G->RegisterMessageHandler(t);
						}
					}

					/*At the moment I can't think of a viable way of testing for the kamakaze
					  behaviour. It's usually a specialized behaviour though so a modder is
					  likely to mark it out in toolkit as a kamikaze unit
					*/

				}
			}
		}
		return true;
	}
	
	void CUnit::SetTaskManager(boost::shared_ptr<ITaskManager> taskManager){
		//
		this->taskManager = taskManager;
	}

	boost::shared_ptr<ITaskManager> CUnit::GetTaskManager(){
		//
		return taskManager;
	}
}
