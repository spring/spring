#include "helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Task::Task(Global* GL){
	G=GL;
	construction = false;
	executions = 0;
	valid = true;
	targ = "";
	c = new Command;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Task::Task(Global* GL,string uname){
	G = GL;
	const UnitDef* u = G->cb->GetUnitDef(uname.c_str());
	if(u == 0){
		G->L->eprint("unitdef failed to load Task::Task");
		valid = false;
	}else{
		c = new Command;
		targ = uname;
		c->id = -u->id;
		construction = true;
		executions = 0;
		valid = true;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Task::execute(int uid){
	if(this->valid == true){
		if(construction == true){
			const UnitDef* udi = G->cb->GetUnitDef(uid);
			string typ = "Factory";
			if(udi->type == typ){
				if(G->Fa->FBuild(targ,uid,1)==true){
					executions++;
					return true;
				}else{
					return false;
				}
			} else{
				if(G->Fa->CBuild(targ,uid) == true){
					executions++;
					return true;
				}else{
					return false;
				}
			}
		} else{
			if(G->cb->GiveOrder(uid,c) != -1){
				executions++;
				return true;
			}else{
				//valid = false;
				return false;
			}
		}
	} else{
		return false;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Tunit::Tunit(Global* GLI, int id){
	G = GLI;
	//p = new cpersona;
	uid = id;
	ud = G->cb->GetUnitDef(id);
	if(ud == 0){
		G->L->eprint("Tunit constructor error, loading unitdefinition");
		bad = true;
	}else{
		bad = false;
	}
	idle = true;
	creation = G->cb->GetCurrentFrame();
	frame = 1;
	guarding = false;
	factory = false;
	waiting = false;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Tunit::~Tunit(){
	//delete p;
	if(tasks.empty() == false){
		for(list<Task*>::iterator ti = tasks.begin();ti != tasks.end();++ti){
			Task* t =*ti;
			delete t;
		}
		tasks.clear();
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Tunit::AddTask(string buildname){
	if((buildname.c_str() == C_METAL)||(buildname.c_str() == C_RADAR)||(buildname.c_str() == C_ENERGY)||(buildname.c_str() == C_WISHLIST)){
		// figure out the best appropriate build item dynamically.
	}else{
		const UnitDef* ua = G->cb->GetUnitDef(buildname.c_str());
		if(ua == 0){
			string emesg = "unitdef loading failed Tunit::AddTask";
			emesg += buildname.c_str();
			exprint(emesg);
			return;
		} else{
			Task* t = new Task(G,buildname);
			if(t->valid == false){
				delete t;
				return;
			}
			tasks.push_front(t);
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Tunit::execute(Task* t){
	if(t != 0){
		return t->execute(uid);
	}else{
		return false;
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

