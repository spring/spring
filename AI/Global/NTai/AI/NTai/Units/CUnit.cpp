/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../Core/include.h"

namespace ntai {

	CUnit::CUnit(){
		G = 0;
		valid=false;
	}

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

			if(EVERY_((GetAge()%32+20))){
				if(currentTask.get() != 0){
					if(!currentTask->IsValid()){
						//
						taskManager->TaskFinished();
						currentTask = taskManager->GetNextTask();
						if(currentTask.get() != 0){
							currentTask->Init();
							G->RegisterMessageHandler(currentTask);
						}else{
							currentTask = boost::shared_ptr<IModule>();
						}
					}
				}else{
					//
					currentTask = taskManager->GetNextTask();
					if(currentTask.get() != 0){
						currentTask->Init();
						G->RegisterMessageHandler(currentTask);
					}
				}

			}

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
		if(under_construction){
			return;
		}

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
		if(valid == false){
			return false;
		}
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
