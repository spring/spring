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
	
	if(!ValidUnitID(uid)){
		valid = false;
	}
	
	this->uid = uid;
	
	
	
	weak_ptr<CUnitTypeData> wutd = G->UnitDefLoader->GetUnitTypeDataByUnitId(uid);
	
	if(wutd.px == 0){
		//
		G->L.eprint("ERROR IN CUNIT :: WUTD.PX == 0, this unit has been passed an invalid weak_ptr to its unit data type object. This unit will fail to load any behaviours or tasks.");
		valid = false;
	}

	if(wutd.expired()){
		//
		G->L.eprint("ERROR IN CUNIT :: WUTD.EXPLIRED() == TRUE, this unit has been passed an expired weak_ptr to its unit data type object. This unit will fail to load any behaviours or tasks.");
		valid = false;
	}

	utd = wutd.lock();

	if(utd->GetUnitDef()==0){
		//
		G->L.eprint("ERROR IN CUNIT :: UTD->GetUnitDef() == 0, ntai failed to retrieve the units unitdef object, something has gone wrong somewhere.");
		valid = false;
	}

	const UnitDef* ud = G->GetUnitDef(uid);
	
	if(ud == 0){
		G->L.eprint("ERROR IN CUNIT :: UD == 0, ntai failed to retrieve the units unitdef object, something has gone wrong somewhere.");
		valid = false;
	}
	
	if(!valid){
		return;
	}
	if(!utd->IsMobile()){
		G->BuildingPlacer->Block(G->GetUnitPos(uid),wutd);
	}
	
	doingplan=false;
	curplan=0;
	birth = G->GetCurrentFrame();
	nolist=false;
	under_construction = true;
}

CUnit::~CUnit(){
	//
}

bool CUnit::Init(){

    NLOG("CUnit::Init");

	if((G->GetCurrentFrame() > 32)&&utd->IsMobile()){
		boost::shared_ptr<IModule> t(new CLeaveBuildSpotTask(G,uid,utd));
		AddTask(t);
		t->Init();
		G->RegisterMessageHandler(t);
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
						t->Init();
						G->RegisterMessageHandler(t);
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
			under_construction = false;
			LoadBehaviours();
		}
	}else if(message.GetType() == string("unitdestroyed")){
		if(message.GetParameter(0) == uid){
			if(!utd->IsMobile()){
				G->BuildingPlacer->UnBlock(G->GetUnitPos(uid),utd);
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
				t->Init();
				G->RegisterMessageHandler(t);
			}
			//executenext = !tasks.empty();
		}
	}
}

int CUnit::GetAge(){
    NLOG("CUnit::GetAge");
	return G->GetCurrentFrame()-birth;
}

weak_ptr<CUnitTypeData> CUnit::GetUnitDataType(){
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
			const UnitDef* building = G->GetUnitDef(q);
			if(G->UnitDefLoader->HasUnit(q)){
				weak_ptr<CUnitTypeData> wb = G->UnitDefLoader->GetUnitTypeDataByName(building->name);
				boost::shared_ptr<IModule> t(new CUnitConstructionTask(G,uid,weak_ptr<CUnitTypeData>(utd),wb));
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
}

bool CUnit::LoadBehaviours(){
	string d = G->Get_mod_tdf()->SGetValueDef("auto","AI\\behaviours\\"+utd->GetName());
	vector<string> v;
	CTokenizer<CIsComma>::Tokenize(v, d, CIsComma());
	if(!v.empty()){
		///behaviours.reserve(v.size()+1);
		for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
			string s = *vi;
			trim(s);
			tolowercase(s);
			if(s == "none"){
				return true;
			} else if(s == "metalmaker"){
				CMetalMakerBehaviour* m = new CMetalMakerBehaviour(G, GetID());
				boost::shared_ptr<IBehaviour> t(m);
				G->RegisterMessageHandler(t);
				behaviours.push_back(t);
				t->Init();
			} else if(s == "attacker"){
				CAttackBehaviour* a = new CAttackBehaviour(G, GetID());
				boost::shared_ptr<IBehaviour> t(a);
				G->RegisterMessageHandler(t);
				behaviours.push_back(t);
				t->Init();
			} else if(s == "auto"){
				if(utd->IsAttacker()){
					CAttackBehaviour* a = new CAttackBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(a);
					behaviours.push_back(t);
					G->RegisterMessageHandler(t);
					t->Init();
				}
				if(utd->IsMetalMaker()||(utd->IsMex() && utd->GetUnitDef()->onoffable ) ){
					CMetalMakerBehaviour* m = new CMetalMakerBehaviour(G, GetID());
					boost::shared_ptr<IBehaviour> t(m);
					behaviours.push_back(t);
					G->RegisterMessageHandler(t);
					t->Init();
				}
			}
		}
	}
	return true;
}
