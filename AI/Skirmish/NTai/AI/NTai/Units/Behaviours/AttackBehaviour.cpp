/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../../Core/include.h"

namespace ntai {
	CAttackBehaviour::CAttackBehaviour(Global* GL, int uid){
		//
		G = GL;
		engaged = false;
		this->uid = uid;
	}

	CAttackBehaviour::~CAttackBehaviour(){
		//
	}

	bool CAttackBehaviour::Init(){
		return true;
	}

	void CAttackBehaviour::RecieveMessage(CMessage &message){
		if(message.GetType() == string("update")){
			if(engaged){
				return;
			}
			if(EVERY_(120 FRAMES)){
				
				float3 pos = G->GetUnitPos(uid);
				if(G->Map->CheckFloat3(pos)==false){
					return;
				}

				engaged = G->Actions->AttackNear(uid, 3.5f);
			}
		}else if(message.GetType() == string("unitdestroyed")){
			if(message.GetParameter(0)== uid){
				End();
			}
		}else if(message.GetType() == string("unitidle")){
			if(message.GetParameter(0)== uid){
				engaged=false;
			}
		}
	}

}
