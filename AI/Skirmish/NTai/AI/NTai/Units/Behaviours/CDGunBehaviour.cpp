/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../../Core/include.h"

namespace ntai {
	CDGunBehaviour::CDGunBehaviour(Global* GL, int uid){
		//
		G = GL;
		this->uid = uid;
		active = false;
	}

	CDGunBehaviour::~CDGunBehaviour(){
		//
	}

	bool CDGunBehaviour::Init(){
		return true;
	}

	void CDGunBehaviour::RecieveMessage(CMessage &message){
		if(message.GetType() == string("update")){
			if(message.GetFrame() % (64) == 0){
				if(!active){
					active = G->Actions->DGunNearby(uid);
				}
			}
		}else if(message.GetType() == string("unitdestroyed")){
			if(message.GetParameter(0)== uid){
				End();
				return;
			}
		}else if(message.GetType() == string("unitidle")){
			if(message.GetParameter(0)== uid){
				active=false;
			}
		}
	}
}
