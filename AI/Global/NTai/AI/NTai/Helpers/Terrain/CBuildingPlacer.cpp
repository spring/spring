/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../../Core/include.h"

namespace ntai {

	ThreadPool* pool=0;
	extern ThreadPool* pool;

	CBuildingPlacer::CBuildingPlacer(Global* GL){
		G=GL;
		valid=true;

		if(pool ==0){
			pool= new ThreadPool(8,200);
		}
	}

	CBuildingPlacer::~CBuildingPlacer(){
		//
	}

	void CBuildingPlacer::RecieveMessage(CMessage &message){
		if(message.GetType()==string("unitcreated")){
			int uid = (int)message.GetParameter(0);
			float3 p = G->GetUnitPos(uid);
			if(this->tempgeo.empty()==false){
				for(map<int, float3>::iterator i = tempgeo.begin(); i != tempgeo.end(); ++i){
					if(i->second.distance2D(p) < 50){
						tempgeo.erase(i);
						break;
					}
				}
			}
		}else if(message.GetType()==string("unitdestroyed")){
			int uid = (int)message.GetParameter(0);
			tempgeo.erase(uid);
		}else if(message.GetType()==string("unitidle")){
			int uid = (int)message.GetParameter(0);
			tempgeo.erase(uid);
		}
		if(message.GetType()==string("update")){
			if(G->L.IsVerbose()){
				if(message.GetFrame()%35 == 0){
					map<int, boost::shared_ptr<CGridCell> > n = blockingmap.GetGrid();
					if(!n.empty()){
						for(map<int, boost::shared_ptr<CGridCell> >::iterator i = n.begin(); i != n.end(); ++i){
							boost::shared_ptr<CGridCell> c = i->second;
							int j = c->GetIndex();
							float3 pos = blockingmap.GridtoMap(blockingmap.IndextoGrid(j));
							pos.y = G->cb->GetElevation(pos.x, pos.z);
							//pos.y -= 20; // move it down into the ground so it doesnt obscure low lying units
							if(G->cb->PosInCamera(pos, 1000)){
								if(G->UnitDefLoader->HasUnit("lttank")!= 0){
									G->cb->DrawUnit("lttank", pos, 1, 35, 0, true, false, 0);
								}else if(G->UnitDefLoader->HasUnit("armpw")!= 0){
									G->cb->DrawUnit("armpw", pos, 1, 35, 0, true, false, 0);
								}else if(G->UnitDefLoader->HasUnit("arm_peewee")!= 0){
									G->cb->DrawUnit("arm_peewee", pos, 1, 35, 0, true, false, 0);
								}else if(G->UnitDefLoader->HasUnit("urcspiderp2")!= 0){
									G->cb->DrawUnit("urcspiderp2", pos, 1, 35, 0, true, false, 0);
								}else if(G->UnitDefLoader->HasUnit("bit")!= 0){
									G->cb->DrawUnit("bit", pos, 1, 35, 0, true, false, 0);
								}
							}
						}
					}
				}
			}
		}
	}

	bool CBuildingPlacer::Init(){
		const float* slope = G->cb->GetSlopeMap();
		float3 mapdim= float3((float)G->cb->GetMapWidth()*SQUARE_SIZE*2, 0, (float)G->cb->GetMapHeight()*SQUARE_SIZE*2);

		int smaxW = G->cb->GetMapWidth()/16;
		int smaxH = G->cb->GetMapHeight()/16;

		int* f = new int[10000];
		int fnum = 0;
		fnum = G->cb->GetFeatures(f, 9999);
		if(fnum > 0){
			//
			for(int i = 0; i < fnum; ++i){
				//
				const FeatureDef* fd = G->cb->GetFeatureDef(f[i]);
				if(fd != 0){
					if(fd->geoThermal){
						float3 fpos = G->cb->GetFeaturePos(f[i]);
						if(G->Map->CheckFloat3(fpos)){
							geolist.push_back(fpos);
						}
					}
				}
			}
		}

		blockingmap.Initialize(mapdim, float3(32, 0, 32), true);
		blockingmap.SetDefaultGridValue(0);
		blockingmap.SetMinimumValue(1);

		return true;
	}


	CBuildAlgorithm::CBuildAlgorithm(CBuildingPlacer* buildalgorithm, IModule* reciever, float3 builderpos, CUnitTypeData* wbuilder, CUnitTypeData* wbuilding, float freespace, CGridManager* blockingmap, const float* heightmap, float3 mapdim, Global* G):
		buildalgorithm(buildalgorithm),
		reciever(reciever),
		builderpos(builderpos),
		builder(wbuilder),
		building(wbuilding),
		freespace(freespace),
		blockingmap(blockingmap),
		heightmap(heightmap),
		mapdim(mapdim),
		valid(true),
		G(G)
	{
		//
	}

	void CBuildAlgorithm::RecieveMessage(CMessage &message){
		//
	}

	bool CBuildAlgorithm::Init(){
		return true;
	}

	void CBuildAlgorithm::operator()(){
		//boost::mutex::scoped_lock lock(io_mutex[building->id]);
		if(!reciever->IsValid()) return;
		float3 bestPosition = UpVector;
		float bestDistance = 500000.0f;
		if(G->L.IsVerbose()){
			AIHCAddMapPoint ac;
			ac.label=new char[11];
			ac.label="build algo";
			ac.pos = builderpos;
			G->cb->HandleCommand(AIHCAddMapPointId,&ac);
		}

		float search_radius = freespace+3000;
		if(builder->IsHub()){
			search_radius = builder->GetUnitDef()->buildDistance;
		}
		CUBuild b;
		b.Init(G, builder, 0);
		int e=0;
		vector<float3> cells = blockingmap->GetCellsInRadius(builderpos, search_radius, e);

		if(!cells.empty()){

			bestDistance = freespace+2001;

			for(vector<float3>::iterator i = cells.begin(); i != cells.end(); ++i){// for each cell

				if(!valid){
					bestPosition = UpVector;
					break;
				}

				// check the reciever is still valid
				// The reciever may have died during this threads running so we need to stop if its
				// invalid to prevent running overtime
				if(reciever->IsValid()==false){
					break;
				}

				float3 gpos = *i;

				if(blockingmap->ValidGridPos(gpos)==false){
					continue;
				}

				float3 mpos = blockingmap->GridtoMap(gpos);

				if(blockingmap->ValidMapPos(mpos)==false){
					continue;
				}

				float distance = mpos.distance2D(builderpos);

				if(distance < bestDistance){
					bool good = true;

					// check our blocking map
					vector<float3> cells2 = blockingmap->GetCellsInRadius(mpos, freespace);

					if(!cells2.empty()){

						//for each cell
						for(vector<float3>::iterator i2 = cells2.begin(); i2 != cells2.end(); ++i2){

							if (blockingmap->GetValuebyGrid(*i2) == 3){

								good = false;
								break;
							}
						}

					}else{

						good = false;//continue;
					}

					// if good == false then the previous check gave a negative result so exit
					if(!good){
						continue;
					}

					// Check if the engine says we can build here
					// This incorporates checking the terrain
					if(G->cb->CanBuildAt(building->GetUnitDef(), mpos)==false){
						continue;
					}

					// update best position/distance
					bestPosition = gpos;
					bestDistance = distance;
				}else{
					continue;
				}
			}

			if(!b.OkBuildSelection(building->GetName())){
				bestPosition = UpVector;
			}

		}else{

			// no surrounding cells?! assign an error value
			string es = "no cells: "+to_string(e);
			G->L.iprint(es);

			bestPosition= UpVector;
		}

		e = blockingmap->ValidGridPosE(bestPosition);

		// check if the grid position is valid
		if(e==0){
			// its valid so convert to a map position
			bestPosition = blockingmap->GridtoMap(bestPosition);
		}else{
			string es = "bad pos(1)"+to_string(e);
			G->L.iprint(es);

			// its invalid so assign an error value
			bestPosition = UpVector;
		}

		CMessage m("buildposition");
		m.AddParameter(bestPosition);

		reciever->RecieveMessage(m);

	}

	void CBuildingPlacer::GetBuildPosMessage(IModule* reciever, int builderID, float3 builderpos, CUnitTypeData* builder, CUnitTypeData* building, float freespace){
		/*if(G->UnitDefHelper->IsFactory(builder)&&(!G->UnitDefHelper->IsHub(builder))){
		 if(G->UnitDefHelper->IsMobile(building)){
		 CMessage m("buildposition");
		 m.AddParameter(builderpos);
		 //if(reciever->IsValid()){
		 reciever->RecieveMessage(m);
		 //}
		 return;
		 }
		 }*/
		if(reciever->IsValid()==false){
			// oh noes! invalid module
			CMessage m("buildposition");
			m.AddParameter(UpVector);
			reciever->RecieveMessage(m);
			return;
		}




		float3 q = UpVector;
		if(building->IsMex()){
			q = G->M->getNearestPatch(builderpos, 0.7f, building->GetUnitDef()->extractsMetal, building->GetUnitDef());
			if((G->Map->CheckFloat3(q) == false)||(builder->IsHub()&&(builder->GetUnitDef()->buildDistance < q.distance2D(builderpos)))){
				//G->L.print("zero mex co-ordinates intercepted");

				CMessage m("buildposition");
				m.AddParameter(UpVector);
				//if(reciever->IsValid()){
				reciever->RecieveMessage(m);
				return;
			}

			int* iunits = new int[10000];
			int itemp = G->GetEnemyUnits(iunits, q, (float)max(building->GetUnitDef()->ysize, building->GetUnitDef()->xsize)*8);
			delete [] iunits;

			if(itemp>0){
				q =  UpVector;
				CMessage m("buildposition");
				m.AddParameter(q);
				//if(reciever->IsValid()){
				reciever->RecieveMessage(m);
				return;
			}
			if(!G->Manufacturer->BPlans->empty()){
				// find any plans for mexes very close to here.... If so change position to match if the same, if its a better mex then cancel existing plan and continue as normal.
				for(deque<CBPlan* >::iterator k = G->Manufacturer->BPlans->begin(); k != G->Manufacturer->BPlans->end(); ++k){
					//
					CBPlan* i = (*k);
					if(i->utd->IsMex()){
						if(i->pos.distance2D(q) < (i->utd->GetUnitDef()->extractRange+building->GetUnitDef()->extractRange)*0.75f){
							if(i->utd->GetUnitDef()->extractsMetal > building->GetUnitDef()->extractsMetal){
								//
								if(!i->started){

									i->RemoveAllBuilders();
									delete i;

									G->Manufacturer->BPlans->erase(k);
									CMessage m("buildposition");
									m.AddParameter(q);
									reciever->RecieveMessage(m);
									return;
								}else{
									q = UpVector;
									CMessage m("buildposition");
									m.AddParameter(q);
									//if(reciever->IsValid()){
									reciever->RecieveMessage(m);
									return;
								}
							}else{
								q = UpVector;
								CMessage m("buildposition");
								m.AddParameter(q);
								//if(reciever->IsValid()){
								reciever->RecieveMessage(m);
								return;
							}
						}
					}
				}
			}
			CMessage m("buildposition");
			m.AddParameter(q);
			//if(reciever->IsValid()){
			reciever->RecieveMessage(m);
			return;
		} else if(G->DTHandler->IsDragonsTeeth(building->GetName())){ // dragon teeth for dragon teeth rings

			if(G->DTHandler->DTNeeded()){
				q = G->DTHandler->GetDTBuildSite(builderpos);
				if(G->Map->CheckFloat3(q) == false){
					G->L.print(string("zero DT co-ordinates intercepted :: ")+ to_string(q.x) + string(",")+to_string(q.y)+string(",")+to_string(q.z));
					q = UpVector;
				}else if(builder->IsHub()&&(builder->GetUnitDef()->buildDistance < q.distance2D(builderpos))){
					q = UpVector;
				}
			}else{
				q = UpVector;
			}

			CMessage m("buildposition");
			m.AddParameter(q);
			reciever->RecieveMessage(m);

			return;
		} else if((building->GetUnitDef()->type == string("Building"))&&(!building->GetUnitDef()->builder)&&building->GetUnitDef()->weapons.empty()&&(building->GetUnitDef()->radarRadius > 100)){ // Radar!
	        
			if(builder->IsHub()){
				q = G->RadarHandler->NextSite(builderpos, building->GetUnitDef(), (int)builder->GetUnitDef()->buildDistance);
			} else {
				q = G->RadarHandler->NextSite(builderpos, building->GetUnitDef(), 1200);
			}

			if(G->Map->CheckFloat3(q) == false){
				G->L.print(string("zero radar placement co-ordinates intercepted  :: ")+ to_string(q.x) + string(",")+to_string(q.y)+string(",")+to_string(q.z));
	            
				q = UpVector;

				CMessage m("buildposition");
				m.AddParameter(q);
				reciever->RecieveMessage(m);

				return;
			}

		}else if(building->GetUnitDef()->needGeo){

			NLOG("CBuildingPlacer::GetBuildPosMessage geo");
			int* f = new int[20000];
			int fnum = 0;

			if(builder->IsHub()){
				fnum = G->cb->GetFeatures(f, 19999, builderpos, builder->GetUnitDef()->buildDistance);
			}else{
				fnum = G->cb->GetFeatures(f, 19999);
			}

			float3 result = UpVector;

			if(!geolist.empty()){

				float gsearchdistance;
				float genemydist;

				G->Get_mod_tdf()->GetDef(gsearchdistance, "3000", "AI\\geotherm\\searchdistance");
				G->Get_mod_tdf()->GetDef(genemydist, "600", "AI\\geotherm\\noenemiesdistance");

				float nearest_dist = 10000000;
				int* a = new int[20000];

				for(vector<float3>::iterator it = geolist.begin(); it != geolist.end(); ++it){
					float d = it->distance2D(builderpos);
					if(d < nearest_dist){
						if(tempgeo.empty()==false){
							for(map<int, float3>::iterator i = tempgeo.begin(); i != tempgeo.end(); ++i){
								if(i->second.distance2D((*it)) < 100){
									continue;
								}
							}
						}

						if(G->cb->GetFriendlyUnits(a,*it,100)> 0){
							continue;
						}

						if(it->distance2D(builderpos) < gsearchdistance){

							if(G->cb->CanBuildAt(building->GetUnitDef(), *it)&&(G->chcb->GetEnemyUnits(a, *it, genemydist)<1)){
								nearest_dist = d;
								result = *it;
							}

						}
					}
				}
	            
				delete[] a;

				NLOG("CBuildingPlacer::GetBuildPosMessage geomark 5#");
			}

			CMessage m("buildposition");
			m.AddParameter(result);
			reciever->RecieveMessage(m);
			tempgeo[builderID] = result;
			delete[] f;

			return;
		}

		if(!builder->IsHub()){

			if(!builder->IsMobile()){
				CMessage m("buildposition");
				m.AddParameter(builderpos);
				reciever->RecieveMessage(m);
				return;
			}
		}

	    
		CBuildAlgorithm cb (this, reciever, builderpos, builder, building, freespace, &blockingmap, G->cb->GetHeightMap(), float3((float)G->cb->GetMapWidth(), 0, (float)G->cb->GetMapHeight()), G);

		pool->invoke(cb);
	}

	/*float3 CBuildingPlacer::findfreespace(const UnitDef* building, float3 MapPos, float buildingradius, float searchradius){
		NLOG("CBuildingPlacer::findfreespace");
		float bestDistance = searchradius+1;
		float3 bestPosition = UpVector;
		vector<float3> cells = this->blockingmap.GetCellsInRadius(MapPos, searchradius);
		if(!cells.empty()){
			NLOG("if(!cells.empty()){");
			for(vector<float3>::iterator i = cells.begin(); i != cells.end(); ++i){// for each cell
				float3 gpos= *i;
				float3 mpos = blockingmap.GridtoMap(gpos);
				float distance = mpos.distance2D(MapPos);
				if(distance < bestDistance){
					vector<float3> cells2 = this->blockingmap.GetCellsInRadius(mpos, buildingradius);

					if(!cells2.empty()){
						bool cnt = false;
						for(vector<float3>::iterator i2 = cells2.begin(); i2 != cells2.end(); ++i2){//for each cell
							int o = blockingmap.GetIndex(*i2);
							if (blockingmap.CellExists(o)){
								cnt = true;
								break;
							}
						}
						if(cnt) continue;
					}else{
						continue;
					}
					//int h = blockingmap.GetHighestindexInRadius(mpos,buildingradius);
					//if(h!= -1) continue;
					/*
					 if(blockingmap.ValidIndex(h)){
					 float j = blockingmap.GetValue(h);
					 if(j ==1){
					 continue;
					 }
					 }else{
					 continue;
					 }*//*
					float fh = highheightmap.GetValuebyMap(mpos);
					float fl = lowheightmap.GetValuebyMap(mpos);
					if(min(fl, fh) <0){
						//
						if(building->minWaterDepth>fl) continue;
						if(building->maxWaterDepth<fh) continue;
					}
					float dh = fh-fl;
					if(dh > building->maxHeightDif){
						//
						continue;
					}

					if(!blockingmap.ValidGridPos(gpos)){
						continue;
					}else{
						bestDistance = distance;
						bestPosition = gpos;
					}
				}
			}
		}else{
			return UpVector;
		}

		float3 fipos = blockingmap.GridtoMap(bestPosition);
		/*if(!G->UnitDefHelper->IsMobile(building)){
		 AIHCAddMapPoint ac;
		 int l = building->name.size()+1;
		 ac.label=new char[l];
		 memcpy(ac.label,building->name.c_str(),l);
		 ac.pos = fipos;
		 G->cb->HandleCommand(AIHCAddMapPointId,&ac);
		 }*//*
		return fipos;
	}*/

	void CBuildingPlacer::Block(float3 pos, CUnitTypeData* utd){
		Block(pos, utd->GetSpacing());
	}

	void CBuildingPlacer::Block(float3 pos){
		Block(pos, 1);
	}

	void CBuildingPlacer::Block(float3 pos, float radius){
		blockingmap.SetCellsInRadius(pos, radius, 3);
	}

	void CBuildingPlacer::UnBlock(float3 pos, CUnitTypeData* utd){
		UnBlock(pos, utd->GetSpacing());
	}

	void CBuildingPlacer::UnBlock(float3 pos){
		UnBlock(pos, (float)0);
	}

	void CBuildingPlacer::UnBlock(float3 pos, float radius){
		blockingmap.SetCellsInRadius(pos, radius, 0);
	}
}

