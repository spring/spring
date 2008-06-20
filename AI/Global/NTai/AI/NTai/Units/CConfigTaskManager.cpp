/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/
#include "../Core/include.h"

namespace ntai {
	

	CConfigTaskManager::CConfigTaskManager(Global* G, boost::shared_ptr<IModule> unit){
		//
		this->G = G;
		nolist = false;
		repeat=true;
		this->unit = unit;
		LoadTaskList();
		
	}

	bool CConfigTaskManager::Init(){
		//
		return true;
	}

	void CConfigTaskManager::RecieveMessage(CMessage &message){
		//
	}

	void CConfigTaskManager::TaskFinished(){
		//
		if(HasTasks()){
			tasks.erase(tasks.begin());
		}
		if(!HasTasks()){
			LoadTaskList();
		}
	}
	
	bool CConfigTaskManager::HasTasks(){
		//
		return !tasks.empty();
	}
	
	boost::shared_ptr<IModule> CConfigTaskManager::GetNextTask(){
		//
		assert(HasTasks());

		return tasks.front();
	}
	
	void CConfigTaskManager::RemoveAllTasks(){
		//
		assert(HasTasks());

		tasks.erase(tasks.begin(),tasks.end());
	}

	bool CConfigTaskManager::LoadTaskList(){
		NLOG("CConfigTaskManager::LoadTaskList");

		IModule* im = unit.get();
		CUnit* u = dynamic_cast<CUnit* >(im);

		CUnitTypeData* utd = u->GetUnitDataType();

		vector<string> vl;
		string sl;
		if(G->Cached->cheating){
			sl= G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\CHEAT\\")+utd->GetName());
		}else{
			sl = G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\NORMAL\\")+utd->GetName());
		}

		tolowercase(sl);
		trim(sl);
		string us = utd->GetName();
		if(sl != string("")){
			CTokenizer<CIsComma>::Tokenize(vl, sl, CIsComma());
			//vl = bds::set_cont(vl,sl.c_str());
			if(vl.empty() == false){
				int randnum = G->mrand()%vl.size();
				us = vl.at(min(randnum,max(int(vl.size()-1),1)));
			}
		}

		string s = G->Get_mod_tdf()->SGetValueMSG(string("TASKLISTS\\LISTS\\")+us);

		if(s.empty() == true){
			G->L.print(" error loading tasklist for unit :: \"" + us + "\" :: buffer empty, most likely because of an empty list");
			nolist=true;
			return false;
		}

		tolowercase(s);
		trim(s);

		vector<string> v;

		CTokenizer<CIsComma>::Tokenize(v, s, CIsComma());
		//v = bds::set_cont(v,s.c_str());

		if(v.empty() == false){
			G->L.print("loading contents of  tasklist :: " + us + " :: filling tasklist with #" + to_string(v.size()) + " items");
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
						boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,u->GetID(),bt));
						tasks.push_back(t);
					}
					polate = !polate;
				}
				string q = *vi;

				trim(q);
				tolowercase(q);

				CUnitTypeData* b = G->UnitDefLoader->GetUnitTypeDataByName(q);
				if(b != 0){
					boost::shared_ptr<IModule> t(new CUnitConstructionTask(G,u->GetID(),utd,b));
					tasks.push_back(t);
				}else if(q == string("")){
					continue;
				}else if(q == string("b_na")){
					continue;
				} else if(q == string("no_rule_interpolation")){
					polation=false;
				} else if(q == string("rule_interpolate")){
					polation=true;
				}else if(q == string("base_pos")){
					G->Map->base_positions.push_back(G->GetUnitPos(u->GetID()));
				} else if(q == string("gaia")){
					G->info->gaia = true;
				} else if(q == string("not_gaia")){
					G->info->gaia = false;
				} else if(q == string("switch_gaia")){
					G->info->gaia = !G->info->gaia;
				} else if(q == string("b_factory")){
					boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,u->GetID(),B_FACTORY));
					tasks.push_back(t);
				} else if(q == string("b_power")){
					boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,u->GetID(),B_POWER));
					tasks.push_back(t);
				} else if(q == string("b_defence")){
					boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,u->GetID(),B_DEFENCE));
					tasks.push_back(t);
				}  else if(q == string("b_mex")){
					boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,u->GetID(),B_MEX));
					tasks.push_back(t);
				} else{
					btype x = G->Manufacturer->GetTaskType(q);
					if( x != B_NA){
						boost::shared_ptr<IModule> t(new CKeywordConstructionTask(G,u->GetID(),x));
						tasks.push_back(t);
					}else{
						G->L.print("error :: a value :: " + *vi +" :: was parsed in :: "+us + " :: this does not have a valid UnitDef according to the engine, and is not a Task keyword such as repair or b_mex");
					}
				}

			}
			if(utd->GetUnitDef()->isCommander){
				G->Map->basepos = G->GetUnitPos(u->GetID());
			}

			G->L.print("loaded contents of  tasklist :: " + us + " :: loaded tasklist at " + to_string(tasks.size()) + " items");
			return !tasks.empty();
		} else{
			G->L.print(" error loading contents of  tasklist :: " + us + " :: buffer empty, most likely because of an empty tasklist");
			return false;
		}
	}

}
