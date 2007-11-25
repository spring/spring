#include "../../Core/helper.h"

CDGunBehaviour::CDGunBehaviour(Global* GL, int uid){
	//
	G = GL;
	unit = G->GetUnit(uid);
	uid = ((CUnit*)unit.get())->GetID();
}

CDGunBehaviour::~CDGunBehaviour(){
	//
}

bool CDGunBehaviour::Init(){
	return true;
}

void CDGunBehaviour::RecieveMessage(CMessage &message){
	if(message.GetType() == string("update")){
	}else if(message.GetType() == string("unitdestroyed")){
		if(message.GetParameter(0)== uid){
			End();
		}
	}
}