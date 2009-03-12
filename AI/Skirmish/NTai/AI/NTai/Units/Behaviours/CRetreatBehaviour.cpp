/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/
#include "../../Core/include.h"

namespace ntai {
	CRetreatBehaviour::CRetreatBehaviour(Global* GL, int uid){
		//
		G = GL;
		this->uid = uid;
		active = false;
	}

	CRetreatBehaviour::~CRetreatBehaviour(){
		//
	}

	bool CRetreatBehaviour::Init(){
		return true;
	}

	void CRetreatBehaviour::RecieveMessage(CMessage &message){
		if(message.GetType() == string("unitdamaged")){
			if(message.GetParameter(0) == uid){
				damage += message.GetParameter(2);
			}
		}else if(message.GetType() == string("update")){
			if(G->GetCurrentFrame() % (64) == 0){
				float d = damage;
				damage = 0;

				if(!active){
					float3 dpos = G->GetUnitPos(uid);
					if(G->Map->CheckFloat3(dpos) == false){
						return;
					}

					int* garbage = new int[6];
					int n =G->cb->GetFriendlyUnits(garbage,dpos,5);
					delete[] garbage;

					if(n > 4){
						float mhealth = G->cb->GetUnitMaxHealth(uid);
						float chealth = G->cb->GetUnitHealth(uid);
						if((mhealth != 0)&&(chealth != 0)){
							if((chealth < mhealth*0.45f)||(d > mhealth*0.7f)){
								// ahk ahk unit is low on health run away! run away!
								// Or unit is being attacked by a weapon that's going to blow it up very quickly
								// Units will want to stay alive!!!
								active = G->Actions->Retreat(uid);
							}
						}
					}
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
