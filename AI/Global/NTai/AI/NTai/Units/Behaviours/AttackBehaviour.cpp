#include "../../Core/include.h"

CAttackBehaviour::CAttackBehaviour(Global* GL, int uid){
	G = GL;
	engaged = false;
	unit = G->GetUnit(uid);
	uid = ((CUnit*)unit.get())->GetID();
}

CAttackBehaviour::~CAttackBehaviour(){
	//
}

bool CAttackBehaviour::Init(){
	return true;
}

void CAttackBehaviour::RecieveMessage(CMessage &message){

	if(message.IsType("update")){
		
		if(engaged){
			return;
		}

		if(message.GetFrame()%(120 FRAMES) == 0){
			
			float3 pos = G->GetUnitPos(uid);

			if(!G->Map->CheckFloat3(pos)){
				return;
			}

			engaged = G->Actions->AttackNear(uid, 3.5f);

		}
	}else if(message.IsType("unitdestroyed")){
		if(message.GetParameter(0) == uid){
			End();
		}
	}else if(message.IsType("unitidle")){
		if(message.GetParameter(0) == uid){
			engaged=false;
		}
	}
}

