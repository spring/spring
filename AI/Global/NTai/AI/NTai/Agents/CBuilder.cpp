// CBuilder class
#include "../Core/helper.h"


CBuilder::CBuilder(Global* GL, int uid){
	G = GL;
	valid = true;
	if(uid < 1) valid = false;
	this->uid = uid;
	ud = G->GetUnitDef(uid);
	if(ud == 0){
		valid = false;
	}
	doingplan=false;
	curplan=0;
	Build.Init(GL,ud,uid);
	birth = G->GetCurrentFrame();
}

CBuilder::~CBuilder(){}

int CBuilder::GetAge(){
	return G->GetCurrentFrame()-birth;
}
const UnitDef* CBuilder::GetUnitDef(){
	NLOG("CBuilder::GetUnitDef");
	return ud;
}

bool CBuilder::operator==(int unit){
	if(valid == false) return false;
	if(unit == uid){
		return true;
	}else{
		return false;
	}
}

bool CBuilder::AddTask(btype type){
	NLOG("CBuilder::AddTask");
	boost::shared_ptr<ITask> t = boost::shared_ptr<ITask>((ITask*)new Task(G,type));
	if(t->IsValid()){
		tasks.push_back(t);
		return true;
	}else{
		return false;
	}
}

int CBuilder::GetID(){
	return uid;
}

bool CBuilder::AddTask(string buildname, bool meta, int spacing){
	NLOG("CBuilder::AddTask");
	boost::shared_ptr<ITask> t = boost::shared_ptr<ITask>((ITask*)new Task(G,meta,buildname,spacing));
	if(t->IsValid()){
		tasks.push_back(t);
		return true;
	}
	return false;
}

bool CBuilder::IsValid(){
	return valid;
}

unit_role CBuilder::GetRole(){
	return role;
}

void CBuilder::SetRole(unit_role new_role){
	NLOG("CBuilder::SetRole");
	role = new_role;
}
