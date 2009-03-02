/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/
#include "../../Core/include.h"

namespace ntai {
	CMetalMakerBehaviour::CMetalMakerBehaviour(Global* GL, int uid){
		//
		G = GL;
		
		this->uid = uid;
		turnedOn = false;
	}

	CMetalMakerBehaviour::~CMetalMakerBehaviour(){
		//
	}

	bool CMetalMakerBehaviour::Init(){

		CUnit* unit = G->GetUnit(uid);
		turnedOn = G->cb->IsUnitActivated(uid);
		energyUse=min(unit->GetUnitDataType()->GetUnitDef()->energyUpkeep,1.0f);
		return true;
	}

	void CMetalMakerBehaviour::RecieveMessage(CMessage &message){
		
		if(message.GetType() == string("update")){
			if(EVERY_((1 SECONDS))){
				float energy=G->cb->GetEnergy();
				float estore=G->cb->GetEnergyStorage();
				if(energy<estore*0.3f){
					if(turnedOn){
						TCommand tc(uid,"assigner:: turnoff");
						tc.ID(CMD_ONOFF);
						tc.Push(0);
						G->OrderRouter->GiveOrder(tc);
						turnedOn=false;
					}
				} else if(energy>estore*0.6f){
					if(!turnedOn){
						TCommand tc(uid,"assigner:: turnon");
						tc.ID(CMD_ONOFF);
						tc.Push(1);
						G->OrderRouter->GiveOrder(tc);
						turnedOn=true;
					}
				}
			}
		}else if(message.GetType() == string("unitdestroyed")){
			if(message.GetParameter(0)== uid){
				End();
			}
		}
	}

}
