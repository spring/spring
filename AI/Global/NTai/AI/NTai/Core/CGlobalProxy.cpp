#include "include.h"

namespace ntai {

	CGlobalProxy::CGlobalProxy(Global* GL){
		//
		G = GL;
	}

	CGlobalProxy::~CGlobalProxy(){
		//
	}


	// Interface functions

	// initialize the AI
	bool CGlobalProxy::Init(){
		//
		return true;
	}

	void CGlobalProxy::RecieveMessage(CMessage &message){
		//

		if(message.IsType("update")){
			//
			if(message.GetFrame() == (1 SECOND)){
				NLOG("STARTUP BANNER IN Global::Update()");

				if(G->L.FirstInstance()){
					
					string s = string(":: ") + AI_NAME + string(" by Tom J. Nowell");
					
					G->cb->SendTextMsg(s.c_str(), 0);
					G->cb->SendTextMsg(":: Copyright (C) 2005-2008 AF Tom J. Nowell", 0);
					
					string q = string(" :: ") + G->Get_mod_tdf()->SGetValueMSG("AI\\message");
					
					if(q != string("")){
						G->cb->SendTextMsg(q.c_str(), 0);
					}

					G->cb->SendTextMsg("Please check www.darkstars.co.uk for updates", 0);
				}

				int* ax = new int[10000];
				int anum = G->cb->GetFriendlyUnits(ax);
				
				G->ComName = string("");
		        
				if(anum !=0){
					for(int a = 0; a<anum; a++){
						if(G->cb->GetUnitTeam(ax[a]) == G->cb->GetMyTeam()){
							const UnitDef* ud = G->GetUnitDef(ax[a]);
							if(ud!=0){
								//
								G->ComName = ud->name;
								G->Cached->comID=ax[a];
								break;
							}
						}
					}
				}
				delete[] ax;
			}

			G->OrderRouter->Update();

			if(message.GetFrame()%(2 MINUTES) == 0){
				G->SaveUnitData();
			}

			G->Actions->Update();
				
			G->Ch->Update();
		}
	}

}
