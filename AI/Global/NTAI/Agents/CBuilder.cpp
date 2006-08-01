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
		return;
	}
	Build.Init(GL,ud,uid);
}

CBuilder::~CBuilder(){}


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
	Task t (G, type);
	if(t.valid == true){
		tasks.push_back(t);
		return true;
	}else{
		return false;
	}
}
int CBuilder::GetID(){
	//
	return uid;
}
bool CBuilder::AddTask(string buildname, bool meta, int spacing){
	NLOG("CBuilder::AddTask");
	Task t(G,meta,buildname,spacing);
	if(t.valid == false){
		return false;
	} else {
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
