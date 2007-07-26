#include "../../Core/helper.h"

CBuildingPlacer::CBuildingPlacer(Global* GL){
    G=GL;
    valid=true;
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
            if(G->GetCurrentFrame()%35 == 0){
                map<int, boost::shared_ptr<CGridCell> > n = blockingmap.GetGrid();
                if(!n.empty()){
                    for(map<int, boost::shared_ptr<CGridCell> >::iterator i = n.begin(); i != n.end(); ++i){
                        boost::shared_ptr<CGridCell> c = i->second;
                        int j = c->GetIndex();
                        float3 pos = blockingmap.GridtoMap(blockingmap.IndextoGrid(j));
                        pos.y = G->cb->GetElevation(pos.x, pos.z);
                        //pos.y -= 20; // move it down into the ground so it doesnt obscure low lying units
                        if(G->cb->PosInCamera(pos, 1000)){
                            if(G->GetUnitDef("lttank")!= 0){
                                G->cb->DrawUnit("lttank", pos, 1, 35, 0, true, false, 0);
                            }else if(G->GetUnitDef("armpw")!= 0){
                                G->cb->DrawUnit("armpw", pos, 1, 35, 0, true, false, 0);
                            }else if(G->GetUnitDef("arm_peewee")!= 0){
                                G->cb->DrawUnit("arm_peewee", pos, 1, 35, 0, true, false, 0);
                            }else if(G->GetUnitDef("urcspiderp2")!= 0){
                                G->cb->DrawUnit("urcspiderp2", pos, 1, 35, 0, true, false, 0);
                            }
                            //G->cb->DrawUnit("armpw",pos,1,35,0,false,false,0);
                            //G->cb->DrawUnit("arm_peewee",pos,1,35,0,false,false,0);
                        }
                        //int k = G->cb->CreateLineFigure(pos,pos+float3(16,50,0),10,1,10,0);
                        //G->cb->SetFigureColor(k,1,0,0,0.5);
                    }
                }
            }
        }
    }
}

bool CBuildingPlacer::Init(boost::shared_ptr<IModule> me){
    this->me = me;
    const float* slope = G->cb->GetSlopeMap();
    float3 mapdim= float3((float)G->cb->GetMapWidth()*SQUARE_SIZE*2, 0, (float)G->cb->GetMapHeight()*SQUARE_SIZE*2);
    
    int smaxW = G->cb->GetMapWidth()/16;
    int smaxH = G->cb->GetMapHeight()/16;
    
    /*slopemap.Initialize(mapdim,float3(32,0,32),true);
     slopemap.SetDefaultGridValue(0);
     slopemap.SetMinimumValue(0);
     slopemap.UseArray(slope,smaxW*smaxH,smaxH,smaxW);*/
    
    /*	geomap.Initialize(mapdim,float3(124,0,124),true);
     geomap.SetDefaultGridValue(0);
     geomap.SetMinimumValue(0);*/
    
    const float* heightmaparray = G->cb->GetHeightMap();
    lowheightmap.Initialize(mapdim, float3(32, 0, 32), true);
    lowheightmap.SetDefaultGridValue(0);
    lowheightmap.SetMinimumValue(0);
    lowheightmap.UseArrayLowValues(heightmaparray, smaxW*smaxH*2, smaxH*2, smaxW*2);
    
    highheightmap.Initialize(mapdim, float3(32, 0, 32), true);
    highheightmap.SetDefaultGridValue(0);
    highheightmap.SetMinimumValue(0);
    highheightmap.UseArrayHighValues(heightmaparray, smaxW*smaxH*2, smaxH*2, smaxW*2);
    
    blockingmap.Initialize(mapdim, float3(32, 0, 32), true);
    blockingmap.SetDefaultGridValue(0);
    blockingmap.SetMinimumValue(1);
    
    G->RegisterMessageHandler("unitcreated", me);
    G->RegisterMessageHandler("unitdestroyed", me);
    G->RegisterMessageHandler("unitidle", me);
    G->RegisterMessageHandler("update", me);
    return true;
}

float3 CBuildingPlacer::GetBuildPos(float3 builderpos, const UnitDef* builder, const UnitDef* building, float freespace){
    if(G->UnitDefHelper->IsFactory(builder)&&(!G->UnitDefHelper->IsHub(builder))){
        if(G->UnitDefHelper->IsMobile(building)){
            return builderpos;
        }
    }
    return findfreespace(building, builderpos, freespace, 1000);
}

class CBuildAlgorithm{
    public:
        CBuildAlgorithm(boost::shared_ptr<IModule> buildalgorithm, boost::shared_ptr<IModule> reciever, float3 builderpos, const UnitDef* builder, const UnitDef* building, float freespace, CGridManager* blockingmap, const float* heightmap, float3 mapdim, Global* G):
            buildalgorithm(buildalgorithm),
            reciever(reciever),
            builderpos(builderpos),
            builder(builder),
            building(building),
            freespace(freespace),
            blockingmap(blockingmap),
            heightmap(heightmap),
            mapdim(mapdim),
            valid(true),
            G(G){ }
            
            void operator()(){
                //boost::mutex::scoped_lock lock(io_mutex[building->id]);
                if(!reciever->IsValid()) return;
                float3 bestPosition = UpVector;
                float bestDistance = 500000.0f;
                //float3 nbpos = ZeroVector;
                //nbpos.x = (building->xsize*4);
                //nbpos.z = (building->ysize*4);
                
                //float builder_radius = (builder->xsize+builder->ysize)*4;//G->Manufacturer->GetSpacing(builder);
                /*bool goodbpos = true;
                 if(G->UnitDefHelper->IsMobile(builder)&&(G->UnitDefHelper->IsHub(builder)==false)){
                 vector<float3> cells1 = blockingmap->GetCellsInRadius(builderpos,freespace);
                 if(!cells1.empty()){
                 for(vector<float3>::iterator i2 = cells1.begin(); i2 != cells1.end(); ++i2){//for each cell
                 if (blockingmap->GetValuebyGrid(*i2) == 3){
                 goodbpos = false;
                 break;
                 }
                 }
                 if(goodbpos){
                 if(G->cb->CanBuildAt(building,builderpos)==false){
                 goodbpos = false;
                 }
                 }
                 }else{
                 goodbpos = false;
                 }
                 }else{
                 goodbpos = false;
                 }
                
                
                
                 if(goodbpos){//+nbpos
                 bestPosition = builderpos;//+nbpos;
                 }else{*/
                float search_radius = freespace+3000;
                if(G->UnitDefHelper->IsHub(builder)){
                    search_radius = builder->buildDistance;
                }
                CUBuild b;
                b.Init(G, builder, 0);
                int e=0;
                vector<float3> cells = blockingmap->GetCellsInRadius(builderpos, search_radius, e);//+nbpos
                if(!cells.empty()){
                    bestDistance = freespace+2001;
                    for(vector<float3>::iterator i = cells.begin(); i != cells.end(); ++i){// for each cell
                        /*if(!b.OkBuildSelection(building->name)){
                         bestPosition = UpVector;
                         break;
                         }*/
                        if(!valid){
                            bestPosition = UpVector;
                            break;
                        }
                        // check the reciever is still valid
                        // The reciever may have died during this threads running so we need to stop if its
                        // invalid to prevent running overtime
                        if(reciever->IsValid()==false) break;
                        float3 gpos = *i;
                        if(blockingmap->ValidGridPos(gpos)==false) continue;
                        float3 mpos = blockingmap->GridtoMap(gpos);
                        if(blockingmap->ValidMapPos(mpos)==false) continue;
                        float distance = mpos.distance2D(builderpos);
                        //if((distance < bestDistance)&&(distance > builder_radius)){
                        if(distance < bestDistance){
                            bool good = true;
                            
                            // check our blocking map
                            vector<float3> cells2 = blockingmap->GetCellsInRadius(mpos, freespace);
                            if(!cells2.empty()){
                                for(vector<float3>::iterator i2 = cells2.begin(); i2 != cells2.end(); ++i2){//for each cell
                                    if (blockingmap->GetValuebyGrid(*i2) == 3){
                                        good = false;
                                        break;
                                    }
                                }
                            }else{
                                good = false;//continue;
                            }
                            
                            // if good == false then the previous check gave a negative result so exit
                            if(!good) continue;
                            
                            // Check if the engine says we can build here
                            // This incorporates checking the terrain
                            if(G->cb->CanBuildAt(building, mpos)==false){
                                continue;
                            }
                            
                            // update best position/distance
                            bestPosition = gpos;
                            bestDistance = distance;
                        }else{
                            continue;
                        }
                    }
                    if(!b.OkBuildSelection(building->name)){
                        bestPosition = UpVector;
                    }
                }else{
                    // no surrounding cells?! assign an error value
                    string es = "no cells: "+to_string(e);
                    G->L.iprint(es);
                    /*			AIHCAddMapPoint ac;
                     ac.label=new char[11];
                     ac.label=(char*)es.c_str();//"no cells";
                     ac.pos = builderpos;
                     G->cb->HandleCommand(AIHCAddMapPointId,&ac);*/
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
                    /*			AIHCAddMapPoint ac;
                     ac.label=new char[11];
                     ac.label=(char*)es.c_str();//"no cells";
                     ac.pos = builderpos;
                     G->cb->HandleCommand(AIHCAddMapPointId,&ac);*/
                    // its invalid so assign an error value
                    bestPosition = UpVector;
                }
                //}
                CMessage m("buildposition");
                m.AddParameter(bestPosition);
                //if(reciever->IsValid()){
                reciever->RecieveMessage(m);
                //}
            }
            //bool figgy(){
            //	return true;
            //}
            Global* G;
            const float* heightmap;
            float bestDistance;
            float3 mapdim;
            /*bool ValidGridPos(float3 gpos){
             //
             if(!blockingmap->ValidGridPos(gpos)){
             return false;
             }
             float3 mpos = blockingmap->GridtoMap(gpos);
             return ValidMapPos(mpos);
             }
            
             bool ValidMapPos(float3 mpos){
             float distance = mpos.distance2D(builderpos);
             if(distance < bestDistance){
             vector<float3> cells2 = blockingmap->GetCellsInRadius(mpos,freespace);
             if(!cells2.empty()){
             for(vector<float3>::iterator i2 = cells2.begin(); i2 != cells2.end(); ++i2){//for each cell
             if (blockingmap->GetValuebyGrid(*i2) == 2){
             return false;
             }
             }
             }else{
             return false;//continue;
             }
            
             if(buildalgorithm->G->cb->CanBuildAt(building,mpos)==false){
             return false;
             }
            
             bestDistance = distance;
             return true;
             }
             return false;
             }*/
            
            bool valid;
            CGridManager* blockingmap;
            boost::shared_ptr<IModule> buildalgorithm;
            boost::shared_ptr<IModule> reciever;
            float3 builderpos;
            const UnitDef* builder;
            const UnitDef* building;
            float freespace;
};

void CBuildingPlacer::GetBuildPosMessage(boost::shared_ptr<IModule> reciever, int builderID, float3 builderpos, const UnitDef* builder, const UnitDef* building, float freespace){
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
    if(G->UnitDefHelper->IsMex(building)){
        q = G->M->getNearestPatch(builderpos, 0.7f, building->extractsMetal, building);
        if((G->Map->CheckFloat3(q) == false)||(G->UnitDefHelper->IsHub(builder)&&(builder->buildDistance < q.distance2D(builderpos)))){
            //G->L.print("zero mex co-ordinates intercepted");
            CMessage m("buildposition");
            m.AddParameter(UpVector);
            //if(reciever->IsValid()){
            reciever->RecieveMessage(m);
            return;
        }
        int* iunits = new int[10000];
        int itemp = G->GetEnemyUnits(iunits, q, (float)max(building->ysize, building->xsize)*8);
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
            for(deque<CBPlan>::iterator i = G->Manufacturer->BPlans->begin(); i != G->Manufacturer->BPlans->end(); ++i){
                //
                if(G->UnitDefHelper->IsMex(i->ud)){
                    if(i->pos.distance2D(q) < (i->ud->extractRange+building->extractRange)*0.75f){
                        if(i->ud->extractsMetal > building->extractsMetal){
                            //
                            if(!i->started){
                                if(i->builders.empty()==false){
                                    for(set<int>::iterator j = i->builders.begin(); j != i->builders.end(); ++j){
                                        //G->Actions->ScheduleIdle(*j);
                                        G->Manufacturer->WipePlansForBuilder(*j);
                                    }
                                    i->builders.erase(i->builders.begin(), i->builders.end());
                                    i->builders.clear();
                                }
                                G->Manufacturer->BPlans->erase(i);
                                CMessage m("buildposition");
                                m.AddParameter(q);
                                //if(reciever->IsValid()){
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
                            /*if(i->started){
                             G->Actions->Repair(unit,i->subject);
                             i->builders.insert(unit);
                             }*/
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
    }else if(G->DTHandler->IsDragonsTeeth(building)){ // dragon teeth for dragon teeth rings
        if(G->DTHandler->DTNeeded()){
            q = G->DTHandler->GetDTBuildSite(builderpos);
            if(G->Map->CheckFloat3(q) == false){
                G->L.print(string("zero DT co-ordinates intercepted :: ")+ to_string(q.x) + string(",")+to_string(q.y)+string(",")+to_string(q.z));
                q = UpVector;
            }else if(G->UnitDefHelper->IsHub(builder)&&(builder->buildDistance < q.distance2D(builderpos))){
                q = UpVector;
            }
        }else{
            q = UpVector;
        }
        CMessage m("buildposition");
        m.AddParameter(q);
        reciever->RecieveMessage(m);
        return;
    } else if((building->type == string("Building"))&&(building->builder == false)&&(building->weapons.empty() == true)&&(building->radarRadius > 100)){ // Radar!
        if(G->UnitDefHelper->IsHub(builder)){
            q = G->RadarHandler->NextSite(builderpos, building, (int)builder->buildDistance);
        }else{
            q = G->RadarHandler->NextSite(builderpos, building, 1200);
        }
        if(G->Map->CheckFloat3(q) == false){
            G->L.print(string("zero radar placement co-ordinates intercepted  :: ")+ to_string(q.x) + string(",")+to_string(q.y)+string(",")+to_string(q.z));
            q = UpVector;
            CMessage m("buildposition");
            m.AddParameter(q);
            reciever->RecieveMessage(m);
            return;
        }
        
    }else if(building->needGeo){
        int* f = new int[20000];
        int fnum = 0;
        if(G->UnitDefHelper->IsHub(builder)){
            fnum = G->cb->GetFeatures(f, 20000, builderpos, builder->buildDistance);
        }else{
            fnum = G->cb->GetFeatures(f, 20000);
        }
        float3 result = UpVector;
        if(fnum >0){
            float gsearchdistance=3000;
            float genemydist=600;
            G->Get_mod_tdf()->GetDef(gsearchdistance, "3000", "AI\\geotherm\\searchdistance");
            G->Get_mod_tdf()->GetDef(genemydist, "600", "AI\\geotherm\\noenemiesdistance");
            float nearest_dist = 10000000;
            for(int i = 0; i < fnum; i++){
                FeatureDef* fd = G->cb->GetFeatureDef(f[i]);
                if(fd){
                    //
                    if(fd->geoThermal){
                        float3 fpos = G->cb->GetFeaturePos(f[i]);
                        float t = fpos.distance2D(builderpos);
                        if(t < nearest_dist){
                            if(tempgeo.empty()==false){
                                for(map<int, float3>::iterator i = tempgeo.begin(); i != tempgeo.end(); ++i){
                                    if(i->second.distance2D(fpos) < 50){
                                        continue;
                                    }
                                }
                            }
                            if(fpos.distance2D(builderpos) < gsearchdistance){
                                int* a = new int[5000];
                                if(G->cb->CanBuildAt(building, fpos)&&(G->chcb->GetEnemyUnits(a, fpos, genemydist)<1)&&(G->cb->GetFriendlyUnits(a, fpos, 50)<1)){
                                    nearest_dist = t;
                                    result = fpos;
                                }
                                delete[] a;
                            }
                        }
                    }
                }
            }
        }
        CMessage m("buildposition");
        m.AddParameter(result);
        reciever->RecieveMessage(m);
        tempgeo[builderID] = result;
        delete[] f;
        return;
    }
    
    
    if(!G->UnitDefHelper->IsHub(builder)){
        if(!G->UnitDefHelper->IsMobile(builder)){//G->UnitDefHelper->IsFactory(builder)){
            CMessage m("buildposition");
            m.AddParameter(builderpos);
            reciever->RecieveMessage(m);
            return;
        }
    }
    
    /*float3 bestPosition = builderpos;
     float bestDistance = 500000.0f;
     //float3 nbpos = ZeroVector;
     //nbpos.x = (building->xsize*4);
     //nbpos.z = (building->ysize*4);
     bool goodbpos = true;
     map<int,boost::shared_ptr<CGridCell> > n = blockingmap.GetGrid();
     if(!n.empty()){
     for(map<int,boost::shared_ptr<CGridCell> >::iterator i = n.begin(); i != n.end(); ++i){
     boost::shared_ptr<CGridCell> c = i->second;
     int j = c->GetIndex();
     if(blockingmap.GetValue(j,1) == 3){
     float3 pos = blockingmap.GridtoMap(blockingmap.IndextoGrid(j));
     if(pos.distance2D(builderpos) < freespace){
     goodbpos = false;
     break;
     }
     }
     }
     }
    
     if(G->cb->CanBuildAt(building,builderpos)==false){
     goodbpos = false;
     }
     if(goodbpos){//+nbpos
     bestPosition = builderpos;//+nbpos;
     }else{
     vector<float3> cells = blockingmap.GetCellsInRadius(builderpos,freespace+2000);//+nbpos
     if(!cells.empty()){
     bestDistance = freespace+2001;
     for(vector<float3>::iterator i = cells.begin(); i != cells.end(); ++i){// for each cell
     float3 gpos= *i;
     float3 mpos = blockingmap.GridtoMap(gpos);
     if(blockingmap.ValidMapPos(mpos)==false) continue;
     float distance = mpos.distance2D(builderpos);
     if(distance < bestDistance){
     bool good = true;
    
     map<int,boost::shared_ptr<CGridCell> > n = blockingmap.GetGrid();
     if(!n.empty()){
     for(map<int,boost::shared_ptr<CGridCell> >::iterator i = n.begin(); i != n.end(); ++i){
     boost::shared_ptr<CGridCell> c = i->second;
     int j = c->GetIndex();
     if(blockingmap.GetValue(j,1) == 3){
     float3 pos = blockingmap.GridtoMap(blockingmap.IndextoGrid(j));
     if(pos.distance2D(mpos) < freespace){
     good = false;
     break;
     }
     }
     }
     }
    
     if(!good) continue;
     if(G->cb->CanBuildAt(building,mpos)==false){
     continue;
     }
     bestPosition = gpos;//blockingmap->GridtoMap()
     bestDistance = distance;
     }else{
     continue;
     }
    
     }
     }else{
     bestPosition= UpVector;
     }
     if(blockingmap.ValidGridPos(bestPosition)){
     bestPosition = blockingmap.GridtoMap(bestPosition);
     }
     }
     CMessage m("buildposition");
     m.AddParameter(bestPosition);
     if(reciever->IsValid()){
     reciever->RecieveMessage(m);
     }*/
    CBuildAlgorithm b(me, reciever, builderpos, builder, building, freespace, &blockingmap, G->cb->GetHeightMap(), float3(G->cb->GetMapWidth(), 0, G->cb->GetMapHeight()), G);
    //
    b();
    
    //boost::thread thrd1(CBuildAlgorithm(me, reciever, builderpos,builder, building, freespace,&blockingmap,G->cb->GetHeightMap(),float3(G->cb->GetMapWidth(),0,G->cb->GetMapHeight()),G));
}

float3 CBuildingPlacer::findfreespace(const UnitDef* building, float3 MapPos, float buildingradius, float searchradius){
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
                 }*/
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
     }*/
    return fipos;
}

void CBuildingPlacer::Block(float3 pos, const UnitDef* ud){
    int r = G->Manufacturer->GetSpacing(ud);
    //pos.x += (ud->xsize*2);
    //pos.z += (ud->ysize*2);
    Block(pos, (float)r);
}

void CBuildingPlacer::Block(float3 pos){
    Block(pos, 1);
}

void CBuildingPlacer::Block(float3 pos, float radius){
    blockingmap.SetCellsInRadius(pos, radius, 3);
}

void CBuildingPlacer::UnBlock(float3 pos, const UnitDef* ud){
    int r = G->Manufacturer->GetSpacing(ud);
    //pos.x -= (ud->xsize*2);
    //pos.z -= (ud->ysize*2);
    UnBlock(pos, (float)r);
}

void CBuildingPlacer::UnBlock(float3 pos){
    UnBlock(pos, (float)0);
}

void CBuildingPlacer::UnBlock(float3 pos, float radius){
    blockingmap.SetCellsInRadius(pos, radius, 0);
}


