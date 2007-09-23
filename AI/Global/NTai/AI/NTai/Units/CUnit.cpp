// CBuilder class
#include "../Core/helper.h"
// Tasks
#include "../Tasks/CConsoleTask.h"
#include "../Tasks/CUnitConstructionTask.h"
#include "../Tasks/CKeywordConstructionTask.h"
#include "../Tasks/CLeaveBuildSpotTask.h"

CUnit::CUnit(Global* GL, int uid){
	G = GL;
	NLOG("CUnit::CUnit");
	valid = true;
	if(!ValidUnitID(uid)) valid = false;
	this->uid = uid;
	ud = G->GetUnitDef(uid);
	if(ud == 0){
		valid = false;
		return;
	}
	if(G->UnitDefHelper->IsMobile(ud)==false){
		G->BuildingPlacer->Block(G->GetUnitPos(uid),ud);
	}
	doingplan=false;
	curplan=0;
	birth = G->GetCurrentFrame();
	nolist=false;
	under_construction = true;
}

CUnit::~CUnit(){
	/*if(G->UnitDefHelper->IsMobile(ud)==false){
		G->BuildingPlacer->UnBlock(G->GetUnitPos(uid),ud);
	}*/
	//G->RemoveHandler(me);
}

bool CUnit::Init(boost::shared_ptr<IModule> me){
    NLOG("CUnit::Init");
	this->me = &me;
	//if(ud->builder){
	G->RegisterMessageHandler(me);
		/*G->RegisterMessageHandler("unitidle",me);
		G->RegisterMessageHandler("unitcreated",me);
		G->RegisterMessageHandler("unitfinished",me);
		G->RegisterMessageHandler("unitdestroyed",me);
		G->RegisterMessageHandler("update",me);*/
	//}

	if((G->GetCurrentFrame() > 32)&&(G->UnitDefHelper->IsMobile(ud))){
		boost::shared_ptr<IModule> t(new CLeaveBuildSpotTask(G,this->uid,ud));
		AddTask(t);
		t->Init(t);
	}
	return true;
}

void CUnit::RecieveMessage(CMessage &message){
	NLOG("CUnit::RecieveMessage");
	if(message.GetType() == string("update")){
		if(under_construction) return;
		if(EVERY_((GetAge()%16+17))){
			if(!tasks.empty()){
				if(tasks.front()->IsValid()==false){
					G->L.print("next task?");
					tasks.erase(tasks.begin());
					if(tasks.empty()==false){
						boost::shared_ptr<IModule> t = tasks.front();
						t->Init(t);
					}
				}
			}
		}
		/*if(EVERY_(32)){
			float3 p = G->GetUnitPos(uid);
			G->cb->CreateLineFigure(p,p+float3(30,40,30),5,0,32,0);
			float3 p2 = G->cb->GetMousePos();
			if(p2.distance2D(p)<30){
				CMessage m("type?");
				tasks.front()->RecieveMessage(m);
				G->cb->SendTextMsg(m.GetType().c_str(),1);
			}
		}*/
	}else if(message.GetType() == string("")){
		return;
	}else if(message.GetType() == string("unitfinished")){
		if(message.GetParameter(0) == this->uid){
			this->under_construction = false;
			//
		}
	}else if(message.GetType() == string("unitdestroyed")){
		if(message.GetParameter(0) == this->uid){
			if(G->UnitDefHelper->IsMobile(ud)==false){
				G->BuildingPlacer->UnBlock(G->GetUnitPos(uid),ud);
			}
			if(!tasks.empty()){
				tasks.erase(tasks.begin(),tasks.end());
				tasks.clear();
			}
			if(!behaviours.empty()){
				behaviours.erase(behaviours.begin(),behaviours.end());
				behaviours.clear();
			}
			this->End();
			return;
		}
	}
	if(under_construction) return;
	if(!nolist){
		if(tasks.empty()){
			if(LoadTaskList()){
				boost::shared_ptr<IModule> t = tasks.front();
				t->Init(t);
			}
			//executenext = !tasks.empty();
		}
	}
}

int CUnit::GetAge(){
    NLOG("CUnit::GetAge");
	return G->GetCurrentFrame()-birth;
}

const UnitDef* CUnit::GetUnitDef(){
	NLOG("CBuilder::GetUnitDef");
	return ud;
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

bool CUnit::AddTask(boost::shared_ptr<IModule> &t){
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
		sl= G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\CHEAT\\")+ud->name);
	}else{
		sl = G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\NORMAL\\")+ud->name);
	}
	tolowercase(sl);
	trim(sl);
	string u = ud->name;
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
		G->L.print(" error loading tasklist :: " + u + " :: buffer empty, most likely because of an empty list");
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
		if(G->UnitDefHelper->IsFactory(ud)){
			polation = false;
		}
		if(bt == B_NA){
			polation = false;
		}

		// TASKS LOADING

		tasks.reserve(v.size());
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
			const UnitDef* building = G->GetUnitDef(q);
			if(building != 0){
				boost::shared_ptr<IModule> t(new CUnitConstructionTask(G,uid,this->ud,building));
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
		if(ud->isCommander == true)	 G->Map->basepos = G->GetUnitPos(GetID());
		//if((ud->isCommander== true)||(ui->GetRole() == R_FACTORY))	G->Map->base_positions.push_back(G->GetUnitPos(GetID()));
		G->L.print("loaded contents of  tasklist :: " + u + " :: loaded tasklist at " + to_string(tasks.size()) + " items");
		return !tasks.empty();
		//G->Actions->ScheduleIdle(GetID());
	} else{
		G->L.print(" error loading contents of  tasklist :: " + u + " :: buffer empty, most likely because of an empty tasklist");
		return false;
	}
}

bool CUnit::LoadBehaviours(){
	string d = G->Get_mod_tdf()->SGetValueDef("auto","AI\\behaviours\\"+ud->name);
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
				boost::shared_ptr<IBehaviour> t = boost::shared_ptr<IBehaviour>(new CMetalMakerBehaviour(G, *me));
				t->Init(t);
				behaviours.push_back(t);
			} else if(s == "attacker"){
				boost::shared_ptr<IBehaviour> t = boost::shared_ptr<IBehaviour>(new CAttackBehaviour(G, *me));
				t->Init(t);
				behaviours.push_back(t);
			} else if(s == "auto"){
				if(G->UnitDefHelper->IsAttacker(ud)){
					boost::shared_ptr<IBehaviour> t = boost::shared_ptr<IBehaviour>(new CAttackBehaviour(G, *me));
					t->Init(t);
					behaviours.push_back(t);
				}
				if(G->UnitDefHelper->IsMetalMaker(ud)||(G->UnitDefHelper->IsMex(ud) && ud->onoffable ) ){
					boost::shared_ptr<IBehaviour> t = boost::shared_ptr<IBehaviour>(new CMetalMakerBehaviour(G, *me));
					t->Init(t);
					behaviours.push_back(t);
				}
			}
		}
	}
	return true;
}
