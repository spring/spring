/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../Core/include.h"

namespace ntai {
	int a;

	COrderRouter::COrderRouter(Global* GL){
		G = GL;
	}

	bool COrderRouter::GiveOrder(TCommand c, bool newer){
		NLOG("Global::GiveOrder");
		if(!ValidUnitID(c.unit)){
			G->L.print("ValidUnitID(c.unit)==false :: c.unit = " + to_string((c.unit))+" MAXUNITS = "+to_string(MAX_UNITS));
			return false;
		}
		if(c.clear == true){
			G->L.print("c.clear == true");
			return false;
		}
		//if(c.c.timeOut > 1){
		//	if(G->cb->GetCurrentFrame() > c.c.timeOut)return -1;
		//}
		//G->idlenextframe.erase(c.unit);
		if(c.type == B_CMD){
			if((c.c.params.empty()==true) || (c.c.params.size()==0)){
				if ((c.c.id == CMD_ATTACK)||(c.c.id == CMD_MOVE)||(c.c.id == CMD_DGUN)||(c.c.id == CMD_ONOFF)||(c.c.id == CMD_RECLAIM)||(c.c.id == CMD_GUARD)||(c.c.id == CMD_MOVE_STATE)||(c.c.id == CMD_FIRE_STATE)||(c.c.id == CMD_REPAIR)){
	#ifdef TC_SOURCE
					G->L.print("empty params?!?!?!? :: " + c.source);
	#else
	G->L.print("empty params?!?!?!? :: ");
	#endif
	return false;
				}
			}else if(((c.c.params.size()==3)||(c.c.params.size()>3))&&(c.c.id > 0)){
				float3 p(c.c.params.at(0), c.c.params.at(1), c.c.params.at(2));
				if(G->Map->CheckFloat3(p)==false){
					G->L.print("intercepted erroneous float3 in a command");
					return false;
				}
			}else if ((c.c.params.size()==1)&&(c.c.id==CMD_DGUN)){
				if(c.c.params.at(0)<0){
					G->L.print("c.c.params.at(0)<0 in CMD_DGUN");
					return false;
				}
			}
	        
		}
		if(newer == true){
			if(CommandCache.empty() == false){
				for(vector<TCommand>::iterator i = CommandCache.begin(); i != CommandCache.end();++i){
					if(i->clear == true) continue;
					if(i->unit == c.unit){
						i->clear = true;
					}
				}
			}
		}
		if (c.type == B_IDLE){
			if(!ValidUnitID(c.unit)){
				G->L.print("!ValidUnitID(c.unit) :: c.unit = " + to_string((c.unit))+" MAXUNITS = "+to_string(MAX_UNITS));
				return false;
			}else{
				G->idlenextframe.insert(c.unit);
				return true;
			}
		}else if(c.type == B_CMD){
			G->idlenextframe.erase(c.unit);
			CommandCache.push_back(c);
			return true;
		}else{
			G->L.print("bad command type");
			return false;
		}
	}

	void COrderRouter::CleanUpOrders(){
		NLOG("COrderRouter::CleanUpOrders()");
		if(CommandCache.empty() == false){
			int hg = CommandCache.size();
			if( hg >20){
				for(int j = 2;  j <(hg/2) ; j++){
					if(CommandCache.empty() == false){
						for(vector<TCommand>::iterator i = CommandCache.begin(); i != CommandCache.end();++i){
							if(i->clear == true){
								CommandCache.erase(i);
								break;
							}
						}
					}
				}
			}
		}
	}

	void COrderRouter::IssueOrders(){
		NLOG("COrderRouter::IssueOrders()");
		if(G->L.FirstInstance() == true){
			a=0;
		}else if (a == BUFFERMAX){
			a -= 4;
		}
		if(CommandCache.empty() == false){
			set<int> timed_out_units;
			for(vector<TCommand>::iterator i = CommandCache.begin();i != CommandCache.end();++i){
				if(i->clear == true) continue;
				switch(i->type)	{
					case B_CMD :{
						if(a == BUFFERMAX){
							continue;
						}
						G->L.print("issuing command in update()");
						// print out the command!
						G->L.print(i->toString());
						if(G->cb->GiveOrder(i->unit, &i->c) == -1){
							G->L.print("hmm failed task update()");
							i->clear = true;
							timed_out_units.insert(i->unit);
							continue;
						}else{
							G->L.print("issuing command in update() succeeded");
							timed_out_units.erase(i->unit);
							G->idlenextframe.erase(i->unit);
							i->clear = true;
							a++;
							continue;
						}
					}case B_IDLE : {
						G->L.print("idle task issuing");
						timed_out_units.erase(i->unit);
						i->clear = true;
						//UnitIdle(i->unit);
						continue;
					}case B_NA :{
						G->L.print("hmm failed task update() B_NA found");
						i->clear = true;
						timed_out_units.insert(i->unit);
						continue;
					}default:{
						G->L.print("hmm failed task update() default????");
						i->clear = true;
						timed_out_units.insert(i->unit);
						continue;
					}
				}
			}
			if(timed_out_units.empty()== false){
				for(set<int>::iterator i = timed_out_units.begin(); i != timed_out_units.end(); i++){
					G->Actions->ScheduleIdle(*i);
				}
			}
		}
	}

	void COrderRouter::UnitDestroyed(int uid){
		if((-1<uid)&&(uid<MAX_UNITS-1)){
			if(CommandCache.empty() == false){
				for(vector<TCommand>::iterator i = CommandCache.begin(); i != CommandCache.end();++i){
					if(i->unit == uid){
						i->clear = true;
					}else if(SubjectOf(*i, uid)){
						i->clear = true;
					}
				}
			}
		}
	}

	bool COrderRouter::SubjectiveCommand(int cmd){
		bool rv = false;
		switch(cmd){
			case CMD_ATTACK: {
				rv = true;
				break;
			}case CMD_GUARD :{
				rv = true;
				break;
			}case CMD_REPAIR :{
				rv = true;
				break;
			}case CMD_RECLAIM : {
				rv = true;
				break;
			}case CMD_DGUN :{
				rv = true;
				break;
			}default:{
				rv = false;
				break;
			}
		}
		return rv;
	}

	bool COrderRouter::SubjectiveCommand(TCommand& cmd){
		if(cmd.type = B_CMD){
			if(cmd.c.params.empty()){
				return false;
			}else{
				if(cmd.c.params.size()==1){
					return SubjectiveCommand(cmd.c.id);
				}else{
					return false;
				}
			}
		}else{
			return false;
		}
	}

	bool COrderRouter::SubjectOf(TCommand c, int unit){
		if (SubjectiveCommand(c)){
			if(c.c.params.at(0)==unit){
				return true;
			}
		}
		return false;
	}

	void COrderRouter::Update(){
		//
		if(EVERY_((5 FRAMES))){
			CleanUpOrders();
		}
		if(EVERY_((3 FRAMES) /*+ team*/)){
			IssueOrders();
		}
	}
}
