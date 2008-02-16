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
		} else if (message.IsType("unitdamaged")){
			int damaged = message.GetParameter(0);
			int attacker = message.GetParameter(1);
			
			const UnitDef* uda = G->GetUnitDef(damaged);

			if(uda != 0){
				float e = G->GetEfficiency(uda->name, uda->power);
				e -= 10000/uda->metalCost;
				G->SetEfficiency(uda->name, e);

				
				const UnitDef* udb = G->GetUnitDef(attacker);

				if(udb != 0){
					float e = G->GetEfficiency(udb->name, udb->power);
					e += 10000/uda->metalCost;
					G->SetEfficiency(udb->name, e);

				}
			}

			int damage = message.GetParameter(2);
			
			float3 dir;

			dir.x = message.GetParameter(3);
			dir.y = message.GetParameter(4);
			dir.z = message.GetParameter(5);

			G->Actions->UnitDamaged(damaged, attacker, damage, dir);


			G->Ch->UnitDamaged(damaged, attacker, damage, dir);

		} else if (message.IsType("unitfinished")){
			//
			int unit = message.GetParameter(0);

			CUnitTypeData* u = G->UnitDefLoader->GetUnitTypeDataByUnitId(unit);
			if(u ==0){
				return;
			}

			const UnitDef* ud = u->GetUnitDef();

			if(ud!=0){
				if(ud->isCommander){
					G->Cached->comID = unit;
				}

				G->max_energy_use += ud->energyUpkeep;

				// prepare unitname
				string t = u->GetName();

				// solo build cleanup

				// Regardless of wether the unit is subject to this behaviour the value of
				// solobuildactive will always be false, so why bother running a check?
				u->SetSoloBuildActive(false);

				if(ud->movedata == 0){
					if(!ud->canfly){
						if(!ud->builder){
							float3 upos = G->GetUnitPos(unit);
							if(upos != UpVector){
								G->DTHandler->AddRing(upos, 500.0f, float(PI_2) / 6.0f);
								G->DTHandler->AddRing(upos, 700.0f, float(-PI_2) / 6.0f);
							}
						}
					}
				}
			}

			G->Manufacturer->UnitFinished(unit);

			G->Ch->UnitFinished(unit);

		} else if(message.IsType("##.verbose")){
			if(G->L.Verbose()){
				G->L.iprint("Verbose turned on");
			} else {
				G->L.iprint("Verbose turned off");
			}

		}else if(message.IsType("##.crash")){
			G->Crash();

		}else if(message.IsType("##.end")){
			exit(0);

		}else if(message.IsType("##.break")){
			if(G->L.FirstInstance()){
				G->L.print("The user initiated debugger break");
			}

		}else if(message.IsType("##.isfirst")){
			if(G->L.FirstInstance()){
				G->L.iprint(" info :: This is the first NTai instance");
			}

		}else if(message.IsType("##.save")){
			if(G->L.FirstInstance()){
				G->SaveUnitData();
			}

		}else if(message.IsType("##.reload")){
			if(G->L.FirstInstance()){
				G->LoadUnitData();
			}

		}else if(message.IsType("##.flush")){
			G->L.Flush();

		}else if(message.IsType("##.threat")){
			G->Ch->MakeTGA();

		}else if(message.IsType("##.aicheat")){

			 //chcb = G->gcb->GetCheatInterface();
			 if(G->Cached->cheating== false){

				 G->Cached->cheating = true;
				 G->chcb->SetMyHandicap(1000.0f);

				 if(G->L.FirstInstance()){
					 G->L.iprint("Make sure you've typed .cheat for full cheating!");
				 }
				 
				 // Spawn 4 commanders around the starting position
				 CUnitTypeData* ud = G->UnitDefLoader->GetUnitTypeDataByName(G->ComName);
				 if(ud != 0){

					 float3 pos = G->Map->basepos;
					 pos = G->cb->ClosestBuildSite(ud->GetUnitDef(), pos, 1000.0f, 0);
					 
					 int ij = G->chcb->CreateUnit(G->ComName.c_str(), pos);
					 
					 if(ij != 0){
						 G->Actions->RandomSpiral(ij);
					 }

					 float3 epos = pos;
					 epos.z -= 1300.0f;
					 float angle = float(G->mrand()%320);
					 pos = G->Map->Rotate(epos, angle, pos);
					 pos = G->cb->ClosestBuildSite(ud->GetUnitDef(), pos, 1000, 300, 1);

					 ij = G->chcb->CreateUnit(G->ComName.c_str(), pos);

					 if(ij != 0){
						 G->Actions->RandomSpiral(ij);
					 }

					 epos = pos;
					 epos.z -= 900.0f;

					 angle = float(G->mrand()%320);

					 pos = G->Map->Rotate(epos, angle, pos);
					 pos = G->cb->ClosestBuildSite(ud->GetUnitDef(), pos, 1000, 300, 0);
					 ///
					 ij = G->chcb->CreateUnit(G->ComName.c_str(), pos);

					 if(ij != 0){
						 G->Actions->RandomSpiral(ij);
					 }
				 }

			 } else if(G->Cached->cheating){
				 G->Cached->cheating  = false;

				 if(G->L.FirstInstance()){
					 G->L.iprint("cheating is now disabled therefore NTai will no longer cheat");
				 }
			 }
		 }

	}

}
