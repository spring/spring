/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../../Core/include.h"

namespace ntai {
	CMoveFailReclaimBehaviour::CMoveFailReclaimBehaviour(Global* GL, int uid){
		//
		G = GL;
		this->uid = uid;
	}

	CMoveFailReclaimBehaviour::~CMoveFailReclaimBehaviour(){
		//
	}

	bool CMoveFailReclaimBehaviour::Init(){
		return true;
	}

	void CMoveFailReclaimBehaviour::RecieveMessage(CMessage &message){
		if(message.GetType() == string("unitmovefailed")){
			if(message.GetParameter(0) == uid){
				int* f = new int[2];
				int i = G->cb->GetFeatures(f,1,G->GetUnitPos(uid),100.0f);
				delete [] f;
				if(i > 0){
					G->Actions->AreaReclaim(uid,G->GetUnitPos(uid),100);
				}
			}
		}else if(message.GetType() == string("unitdestroyed")){
			if(message.GetParameter(0)== uid){
				End();
			}
		}
	}
}
