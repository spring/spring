/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../../Core/include.h"

namespace ntai {
	CKamikazeBehaviour::CKamikazeBehaviour(Global* GL, int uid){
		//
		G = GL;
		engaged = false;
		unit = G->GetUnit(uid);
		uid = ((CUnit*)unit.get())->GetID();
		maxrange = G->cb->GetUnitMaxRange(uid);
	}

	CKamikazeBehaviour::~CKamikazeBehaviour(){
		//
	}

	bool CKamikazeBehaviour::Init(){
		return true;
	}

	void CKamikazeBehaviour::RecieveMessage(CMessage &message){
		if(message.GetType() == string("update")){
			if(engaged){
				return;
			}
			if(EVERY_(120 FRAMES)){
				
				int* funits = new int [G->Cached->enemies.size()];
				int fu = G->GetEnemyUnits(funits, G->GetUnitPos(uid), maxrange);
				if(fu > 0 ){
					TCommand tc(uid, "CKamikazeBehaviour::RecieveMessage selfd");
					tc.ID(CMD_SELFD);
					engaged = G->OrderRouter->GiveOrder(tc);
				}
				delete [] funits;
			}
		}else if(message.GetType() == string("unitdestroyed")){
			if(message.GetParameter(0) == uid){
				End();
			}
		}else if(message.GetType() == string("unitidle")){
			if(message.GetParameter(0) == uid){
				engaged=false;
			}
		}
	}
}
