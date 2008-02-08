#include "../../Core/include.h"

CDGunBehaviour::CDGunBehaviour(Global* GL, int uid){
	//
	G = GL;
	unit = G->GetUnit(uid);
	uid = ((CUnit*)unit.get())->GetID();
	active = false;
}

CDGunBehaviour::~CDGunBehaviour(){
	//
}

bool CDGunBehaviour::Init(){
	return true;
}

void CDGunBehaviour::RecieveMessage(CMessage &message){
	if(message.IsType("update")){
		if(message.GetFrame() % (64) == 0){
			if(!active){
				active = G->Actions->DGunNearby(uid);
			}
		}
	}else if(message.IsType("unitdestroyed")){
		if(message.GetParameter(0)== uid){
			End();
			return;
		}
	}else if(message.IsType("unitidle")){
		if(message.GetParameter(0)== uid){
			active=false;
		}
	}
}
