#include "../Core/helper.h"

int* garbage = 0;
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Chaser::Chaser(){
    G = 0;
    lock = false;
    if(garbage == 0){
        garbage = new int[10000];
    }
}

void Chaser::InitAI(Global* GLI){
    G = GLI;
    NLOG("Chaser::InitAI");
    enemynum=0;
    G->Get_mod_tdf()->GetDef(threshold, "5", "AI\\initial_threat_value");
    G->Get_mod_tdf()->GetDef(thresh_increase, "1", "AI\\increase_threshold_value");
    G->Get_mod_tdf()->GetDef(thresh_percentage_incr, "1", "AI\\increase_threshold_percentage");
    G->Get_mod_tdf()->GetDef(max_threshold, "1", "AI\\maximum_attack_group_size");
    string contents = G->Get_mod_tdf()->SGetValueMSG("AI\\kamikaze");
    trim(contents);
    tolowercase(contents);
    if(contents.empty() == false){
        vector<string> v;
		CTokenizer<CIsComma>::Tokenize(v, contents, CIsComma());
        //v = bds::set_cont(v, contents);
        if(v.empty() == false){
            for(vector<string>::iterator vi = v.begin(); vi != v.end(); ++vi){
                string h = *vi;
                trim(h);
                tolowercase(h);
                if(h != string("")){
                    sd_proxim[h] = true;
                }
            }
        }
    }

    float3 mapdim= float3((float)G->cb->GetMapWidth()*SQUARE_SIZE, 0, (float)G->cb->GetMapHeight()*SQUARE_SIZE);
    Grid.Initialize(mapdim, float3(1024, 0, 1024), true);
    Grid.ApplyModifierOnUpdate(0.9f, (15 SECONDS));
    Grid.SetDefaultGridValue(5.0f);
    Grid.SetMinimumValue(10.0f);

	CTokenizer<CIsComma>::Tokenize(fire_at_will, G->Get_mod_tdf()->SGetValueMSG("AI\\fire_at_will"), CIsComma());
	CTokenizer<CIsComma>::Tokenize(return_fire, G->Get_mod_tdf()->SGetValueMSG("AI\\return_fire"), CIsComma());
	CTokenizer<CIsComma>::Tokenize(hold_fire, G->Get_mod_tdf()->SGetValueMSG("AI\\hold_fire"), CIsComma());
	CTokenizer<CIsComma>::Tokenize(roam, G->Get_mod_tdf()->SGetValueMSG("AI\\roam"), CIsComma());
	CTokenizer<CIsComma>::Tokenize(maneouvre, G->Get_mod_tdf()->SGetValueMSG("AI\\maneouvre"), CIsComma());
	CTokenizer<CIsComma>::Tokenize(hold_pos, G->Get_mod_tdf()->SGetValueMSG("AI\\hold_pos"), CIsComma());


    //fire_at_will = bds::set_cont(fire_at_will, G->Get_mod_tdf()->SGetValueMSG("AI\\fire_at_will"));
    //return_fire = bds::set_cont(return_fire, G->Get_mod_tdf()->SGetValueMSG("AI\\return_fire"));
    //hold_fire = bds::set_cont(hold_fire, G->Get_mod_tdf()->SGetValueMSG("AI\\hold_fire"));
    //roam = bds::set_cont(roam, G->Get_mod_tdf()->SGetValueMSG("AI\\roam"));
    //maneouvre = bds::set_cont(maneouvre, G->Get_mod_tdf()->SGetValueMSG("AI\\maneouvre"));
    //hold_pos = bds::set_cont(hold_pos, G->Get_mod_tdf()->SGetValueMSG("AI\\hold_pos"));
    /*if(G->Sc->start_pos.empty() == false){
        for(list<float3>::iterator i = G->Sc->start_pos.begin(); i != G->Sc->start_pos.end(); ++i){
            float3 tpos = *i;
            Grid.AddValueatMapPos(tpos, 60000.0f);
            G->Actions->AddPoint(tpos);
        }
    }*/
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::Add(int unit, bool aircraft){
    NO_GAIA(NA)
    G->L.print("Chaser::Add()");
    Attackers.insert(unit);
    //	attack_units.insert(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDestroyed(int unit, int attacker){
    NLOG("Chaser::UnitDestroyed");
    NO_GAIA(NA)
    dgunning.erase(unit);
    defences.erase(unit);
    dgunners.erase(unit);
    engaged.erase(unit);
    walking.erase(unit);
    //	attack_units.erase(unit);
    sweap.erase(unit);
    kamikaze_units.erase(unit);
    Attackers.erase(unit);
    if(!attack_groups.empty()){
        // remove from atatck groups
        for(vector< set<int> >::iterator i = attack_groups.begin(); i != attack_groups.end(); ++i){
            if(i->find(unit)!= i->end()){
                i->erase(unit);
                if(i->empty()) break;
                if((int)i->size() < threshold/5){
                    // this group is now too small
                    // break it apart and put the units into the temporary unit group and retreat.
                    this->temp_attack_units.insert(i->begin(), i->end());
                    i->clear();
                    attack_groups.erase(i);
                }
                break;
            }
        }
    }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitDamaged(int damaged, int attacker, float damage, float3 dir){
    NLOG("Chaser::UnitDamaged");

    NO_GAIA(NA)
    START_EXCEPTION_HANDLING
    float3 dpos = G->GetUnitPos(damaged);
    if(G->Map->CheckFloat3(dpos) == false) return;
    if(attacker<0) return;

    if(G->cb->GetFriendlyUnits(garbage,dpos,400) > 4){
        float mhealth = G->cb->GetUnitMaxHealth(damaged);
        float chealth = G->cb->GetUnitHealth(damaged);
        if((mhealth != 0)&&(chealth != 0)){
            if((chealth < mhealth*0.45f)||(damage > mhealth*0.7f)){
                // ahk ahk unit is low on health run away! run away!
                // Or unit is being attacked by a weapon that's going to blow it up very quickly
                // Units will want to stay alive!!!
                G->Actions->Retreat(damaged);
            }
        }
    }

    /*int ateam = G->chcb->GetUnitAllyTeam(attacker);
    if(allyteamGrudges.empty()==false){
        if(allyteamGrudges.find(ateam) == allyteamGrudges.end()){
            allyteamGrudges[ateam] = damage;
        }else{
            allyteamGrudges[ateam] += damage;
        }
    }else{
        allyteamGrudges[ateam] = damage;
    }*/
    if(dgunners.find(damaged) != dgunners.end()){
        //
        if(dgunning.find(damaged) == dgunning.end()){
            if(ValidUnitID(attacker)){
                //
                if(damaged > G->cb->GetUnitHealth(damaged)/3){
                    // escape
                    if(G->Actions->Move(damaged, dpos+(dir*300))){
                        //
                        dgunning.insert(damaged);
                    }
                }else{
                    // move to dgun
                    G->Actions->DGun(damaged, attacker);
                }
            }
        }
    }
    END_EXCEPTION_HANDLING("Chaser::UnitDamaged running away routine!!!!!!")
    //EXCEPTION_HANDLER(G->Actions->DGunNearby(damaged),"G->Actions->DGunNearby INSIDE Chaser::UnitDamaged",NA) // make the unit attack nearby enemies with the dgun, the attacker being the one most likely to be the nearest....
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitFinished(int unit){
    NLOG("Chaser::UnitFinished");
    NO_GAIA(NA)
    const UnitDef* ud = G->GetUnitDef(unit);
    if(ud == 0){
        G->L.print("UnitDef call filled, was this unit blown up as soon as it was created?");
        return;
    }
    NLOG("stockpile/dgun");
    if(ud->canDGun){
        dgunners.insert(unit);
    }
    if(ud->weapons.empty() == false){
        for(vector<UnitDef::UnitDefWeapon>::const_iterator i = ud->weapons.begin(); i!= ud->weapons.end(); ++i){
            if(i->def->stockpile == true){
                TCommand tc(unit, "chaser::unitfinished stockpile");
                tc.c.timeOut = G->cb->GetCurrentFrame() + (20 MINUTES);
                tc.ID(CMD_STOCKPILE);
                for(int i = 0;i<110;i++){
                    G->OrderRouter->GiveOrder(tc, false);
                }
                sweap.insert(unit);
            }
        }
    }
    if((ud->movedata == 0) && (ud->canfly == false)&&(sweap.find(unit) == sweap.end()) && (ud->weapons.empty() == false)){
        defences.insert(unit);
        return;
    }
    if(maneouvre.empty() == false){
        for(vector<string>::iterator i = maneouvre.begin(); i != maneouvre.end(); ++i){
            //
            if(*i == ud->name){
                TCommand tc(unit, "setting firing state/move state");
                tc.ID(CMD_MOVE_STATE);
                tc.Push(1);
                G->OrderRouter->GiveOrder(tc, false);
                break;
            }
        }
    }
    if(hold_pos.empty() == false){
        for(vector<string>::iterator i = hold_pos.begin(); i != hold_pos.end(); ++i){
            //
            if(*i == ud->name){
                TCommand tc(unit, "setting firing state/movestate");
                tc.ID(CMD_MOVE_STATE);
                tc.Push(0);
                G->OrderRouter->GiveOrder(tc, false);
                break;
            }
        }
    }
    if(roam.empty() == false){
        for(vector<string>::iterator i = roam.begin(); i != roam.end(); ++i){
            //
            if(*i == ud->name){
                TCommand tc(unit, "setting firing state/movestate");
                tc.ID(CMD_MOVE_STATE);
                tc.Push(2);
                G->OrderRouter->GiveOrder(tc, false);
                break;
            }
        }
    }
    if(fire_at_will.empty() == false){
        for(vector<string>::iterator i = fire_at_will.begin(); i != fire_at_will.end(); ++i){
            //
            if(*i == ud->name){
                TCommand tc(unit, "setting firing state/movestate");
                tc.ID(CMD_FIRE_STATE);
                tc.Push(2);
                G->OrderRouter->GiveOrder(tc, false);
                break;
            }
        }
    }
    if(return_fire.empty() == false){
        for(vector<string>::iterator i = return_fire.begin(); i != return_fire.end(); ++i){
            //
            if(*i == ud->name){
                TCommand tc(unit, "setting firing state/movestate");
                tc.ID(CMD_FIRE_STATE);
                tc.Push(1);
                G->OrderRouter->GiveOrder(tc, false);
                break;
            }
        }
    }
    if(hold_fire.empty() == false){
        for(vector<string>::iterator i = hold_fire.begin(); i != hold_fire.end(); ++i){
            //
            if(*i == ud->name){
                TCommand tc(unit, "setting firing state/movestate");
                tc.ID(CMD_FIRE_STATE);
                tc.Push(0);
                G->OrderRouter->GiveOrder(tc, false);
                break;
            }
        }
    }

    if(G->UnitDefHelper->IsAttacker(ud)){
        unit_to_initialize.insert(unit);
    }
    NLOG("kamikaze");
    if(sd_proxim.empty() == false){
        string sh = ud->name;
        trim(sh);
        tolowercase(sh);
        if(sd_proxim.find(sh) != sd_proxim.end()){
            kamikaze_units.insert(unit);
        }
    }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Chaser::~Chaser(){
    delete[] garbage;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::EnemyDestroyed(int enemy, int attacker){
    NLOG("Chaser::EnemyDestroyed");
    NO_GAIA(NA)
    if(enemy < 1) return;
    engaged.erase(attacker);
    float3 dir = G->GetUnitPos(enemy);
    if(G->Map->CheckFloat3(dir) == true){
        Grid.ApplyModifierAtMapPos(dir, 0.7f);
        const UnitDef* def = G->GetEnemyDef(enemy);
        if(def){
            if(def->extractsMetal > 0) G->M->EnemyExtractorDead(dir, enemy);
        }
    }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

bool Chaser::FindTarget(set<int> atkgroup, bool upthresh){
    //NLOG("Chaser::FindTarget");
    float3 final_dest = UpVector;

    int Index = Grid.GetHighestIndex();
    if(Grid.ValidIndex(Index)==false){
        return false;
    }
    final_dest = Grid.GridtoMap(Grid.IndextoGrid(Index));
    if(Grid.ValidMapPos(final_dest)){
        if(atkgroup.empty() == false){
            //check target position has enemies
            /*int enenum = G->chcb->GetEnemyUnits(garbage,final_dest,1000);
            if(enenum < 5){
                return false;
            }*/

            if(G->L.IsVerbose()){
                AIHCAddMapPoint ac;
                ac.label=new char[11];
                ac.label="attack pos";
                ac.pos = final_dest;
                G->cb->HandleCommand(AIHCAddMapPointId,&ac);
            }
            //Grid.ApplyModifierAtMapPos(final_dest, 1.6f);
            G->L.print("attack position targetted sending units");
            for(set<int>::iterator aik = atkgroup.begin();aik != atkgroup.end();++aik){
                float3 upos = G->GetUnitPos(*aik);
                if(Grid.ValidMapPos(upos)){
                    float3 npos;
                    if(upos.distance2D(final_dest) < G->cb->GetUnitMaxRange(*aik)){
                        continue;
                    }else{
                        npos = final_dest;//G->Map->distfrom(upos,final_dest,G->cb->GetUnitMaxRange(*aik)*0.95f);
                    }
                    //if(upos.distance2D(npos) > G->cb->GetUnitMaxRange(*aik)){
                    npos.y = 0;
                    //AIHCAddMapPoint ac2;
                    //ac2.label=new char[12];
                    //ac2.label="bad atk pos";

                    if(!G->Actions->MoveToStrike(*aik, npos, true)){//ToStrike
                        //	ac2.pos = npos;
                        //}else{
                        //	ac2.pos = upos;
                        //	G->cb->HandleCommand(AIHCAddMapPointId,&ac2);
                    }

                    //}
                }
            }
        }
        if(upthresh){
            threshold = int(threshold * thresh_percentage_incr);
            threshold = int(threshold + thresh_increase);
            threshold = min(threshold, max_threshold);
            G->L << "new threshold :: "<< threshold<<"\n";
        }
    }else{
        return false;
    }

    return true;
}


// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitMoveFailed(int unit){
    NLOG("Chaser::UnitMoveFailed");
    NO_GAIA(NA)

    this->UnitIdle(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Chaser::UnitIdle(int unit){
    NLOG("Chaser::UnitIdle");
    NO_GAIA(NA)
    if(this->unit_to_initialize.find(unit) != this->unit_to_initialize.end()){
        unit_to_initialize.erase(unit);
        const UnitDef* ud = G->GetUnitDef(unit);
        if(ud == 0) return;
        Add(unit);
        bool target = false;
        if(G->UnitDefHelper->IsAirCraft(ud)){
            temp_air_attack_units.insert(unit);
            if((int)temp_air_attack_units.size()>threshold){
                target = true;
            }
        }else{
            temp_attack_units.insert(unit);
            if((int)temp_attack_units.size()>threshold){
                target = true;
            }
        }
        G->L.print(string("new attacker added :: ") + ud->name + string(" target?:")+to_string(target)+string(" threshold:")+to_string(threshold));
        if(target){
            set<int> temp;
            if(G->UnitDefHelper->IsAirCraft(ud)){
                FindTarget(temp_air_attack_units, true);
                temp.insert(temp_air_attack_units.begin(), temp_air_attack_units.end());
                temp_air_attack_units.erase(temp_air_attack_units.begin(), temp_air_attack_units.end());
                temp_air_attack_units.clear();
            }else{

                FindTarget(temp_attack_units, true);
                temp.insert(temp_attack_units.begin(), temp_attack_units.end());
                temp_attack_units.erase(temp_attack_units.begin(), temp_attack_units.end());
                temp_attack_units.clear();
            }
            /*threshold = int(threshold * thresh_percentage_incr);
            threshold = int(threshold + thresh_increase);
            threshold = min(threshold, max_threshold);*/

            this->attack_groups.push_back(temp);
        } else {
            DoUnitStuff(unit);
        }
        return;
    }

    if(dgunning.empty() == false){
        dgunning.erase(unit);
    }
    engaged.erase(unit);
    walking.erase(unit);

    if(this->Attackers.find(unit) != Attackers.end()){
        G->Actions->Retreat(unit);
        //DoUnitStuff(unit);
    }
    return;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// Fire units in the sweap array, such as nuke silos etc.

void Chaser::FireSilos(){
    NLOG("Chaser::FireSilos");
    NO_GAIA(NA)
    int fnum = G->cb->GetCurrentFrame(); // Get frame
    if((fnum%200 == 0)&&(sweap.empty() == false)){ // Every 30 seconds
        for(set<int>::iterator si = sweap.begin(); si != sweap.end();++si){
            float maxrange = G->cb->GetUnitMaxRange(*si);
            float3 WPos = G->GetUnitPos(*si);
            if(!Grid.ValidMapPos(WPos)) continue;
            int Index = Grid.GetHighestindexInRadius(WPos, maxrange);
            if(Grid.ValidIndex(Index)){
                float3 swtarget = Grid.GridtoMap(Grid.IndextoGrid(Index));
                if(Grid.ValidMapPos(swtarget)){
                    G->Actions->Attack(*si, swtarget);
                }
            }
        }
    }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#define TGA_RES 8

void ThreatTGA(Global* G){
    NLOG("ThreatTGA");
    // 	// open file
    // 	char buffer[1000];
    // 	char c [4];
    // 	sprintf(c,"%i",G->Cached->team);
    // 	string filename = G->info->datapath + string("/threatdebug/threatmatrix") + c + string(".tga");
    //
    // 	strcpy(buffer, filename.c_str());
    // 	G->cb->GetValue(AIVAL_LOCATE_FILE_W, buffer);
    //
    // 	FILE *fp=fopen(buffer, "wb");
    // 	//#ifdef _MSC_VER
    //  	//	if(!fp)	_endthread();
    // 	//#else
    // 		if(!fp) return;
    // 	//#endif
    // 	map< int, map<int,agrid> > sec = G->Ch->sector;
    //
    // 	// fill & write header
    // 	char targaheader[18];
    // 	memset(targaheader, 0, sizeof(targaheader));
    // 	float biggestebuild = 1;
    // 	float biggestheight = 1;
    // 	float smallestheight = 0;
    // 	float p;
    // 	for(int xa = 0; xa < G->Ch->xSectors; xa++){
    // 		for(int ya = 0; ya < G->Ch->ySectors; ya++){
    // 			if(G->Ch->threat_matrix[sec[xa][ya].index] / (sec[xa][ya].centre.distance(G->Map->nbasepos(sec[xa][ya].centre))/4) > biggestebuild)	biggestebuild = G->Ch->threat_matrix[sec[xa][ya].index] / (sec[xa][ya].centre.distance(G->Map->nbasepos(sec[xa][ya].centre))/4);
    // 			float g = G->cb->GetElevation(sec[xa][ya].centre.x,sec[xa][ya].centre.z);
    // 			if(g > biggestheight) biggestheight = g;
    // 			if(g < smallestheight) smallestheight = g;
    // 		}
    // 	}
    // 	if(smallestheight < 0.0f) smallestheight *= -1;
    // 	p = (smallestheight + biggestheight)/255;
    // 	if( p == 0.0f) p = 0.01f;
    //
    // 	targaheader[2] = 2;		/* image type = uncompressed gray-scale */
    // 	targaheader[12] = (char) (G->Ch->xSectors*TGA_RES & 0xFF);
    // 	targaheader[13] = (char) (G->Ch->xSectors*TGA_RES >> 8);
    // 	targaheader[14] = (char) (G->Ch->ySectors*TGA_RES & 0xFF);
    // 	targaheader[15] = (char) (G->Ch->ySectors*TGA_RES >> 8);
    // 	targaheader[16] = 24; /* 24 bit color */
    // 	targaheader[17] = 0x20;	/* Top-down, non-interlaced */
    //
    // 	fwrite(targaheader, 18, 1, fp);
    //
    // 	// write file
    // 	uchar out[3];
    // 	float ebuildp = (biggestebuild)/255;
    // 	if(ebuildp <= 0.0f) ebuildp = 1;
    // 	for (int z=0;z<G->Ch->ySectors*TGA_RES;z++){
    // 		for (int x=0;x<G->Ch->xSectors*TGA_RES;x++){
    // 			const int div=4;
    // 			int x1 = x;
    // 			int z1 = z;
    // 			if(z%TGA_RES != 0) z1 = z-(z%TGA_RES); z1 = z1/TGA_RES;
    // 			if(x%TGA_RES != 0) x1 = x-(x%TGA_RES); x1 = x1/TGA_RES;
    // 			out[0]=(uchar)/*min(255, (int)*/max(1.0f,(sec[x1][z1].centre.y+smallestheight)/p);//);//Blue Heightmap
    // 			//out[1]=(uchar)/*min(255,(int)*/max(1.0f,(sec[x1][z1].index] / (sec[x1][z1].centre.distance(G->Map->nbasepos(sec[x1][z1].centre))/4)/groundp) / div);//);//Green Ground threat
    // 			out[2]=(uchar)/*min(255,(int)*/max(1.0f,(G->Ch->threat_matrix[sec[x1][z1].index] / (sec[x1][z1].centre.distance(G->Map->nbasepos(sec[x1][z1].centre))/4)/ebuildp)/div);//);//Red Building threat
    //
    // 			fwrite (out, 3, 1, fp);
    // 		}
    // 	}
    // 	fclose(fp);
    // 	#ifdef _MSC_VER
    // 	system(filename.c_str());
    // 	//_endthread();
    // 	#endif
    // 	return;
}
void Chaser::MakeTGA(){
    NLOG("Chaser::MakeTGA");
    int Index = Grid.GetHighestIndex();
    G->L.Verbose();
    G->L << "HighestIndex : "<< Index << " GridDimensions: "<< Grid.GetGridDimensions();
    if(Grid.ValidIndex(Index)){
        G->L << "value: " << Grid.GetValue(Index)<< endline;
        AIHCAddMapPoint ac;
        ac.label=new char[11];
        ac.label="attack pos";
        ac.pos = Grid.GridtoMap(Grid.IndextoGrid(Index));
        G->cb->HandleCommand(AIHCAddMapPointId, &ac);
    }else{
        G->L << "value: bad Index"<< endline;
    }
    G->L.Verbose();
    //#ifdef _MSC_VER
    //_beginthread((void (__cdecl *)(void *))ThreatTGA, 0, (void*)G);
    //#else
    ThreatTGA(G);
    //#endif
}

void Chaser::UpdateMatrixFriendlyUnits(){
    NLOG("Chaser::Update :: threat matrix update friendly units");
    bool dofriend =false;
    G->Get_mod_tdf()->GetDef(dofriend, "0", "AI\\depreciateMatrixForAllyUnits");
    if(dofriend){
        int* funits = new int[6000];
        int fu = G->cb->GetFriendlyUnits(funits);
        if(fu > 0){
            for(int f = 0; f <fu; f++){
                float3 fupos = G->GetUnitPos(funits[f]);
                Grid.ApplyModifierAtMapPos(fupos, 0.99f);
            }
        }
        delete [] funits;
    }
}
//
void Chaser::CheckKamikaze(){
    if((kamikaze_units.empty() == false)&&(G->Cached->enemies.empty() == false)){
        for(set<int>::iterator i = kamikaze_units.begin(); i != kamikaze_units.end(); ++i){
            int* funits = new int [G->Cached->enemies.size()];
            int fu = G->GetEnemyUnits(funits, G->GetUnitPos(*i), G->cb->GetUnitMaxRange(*i));
            if(fu > 0 ){
                TCommand tc(*i, "chaser::update selfd");
                tc.ID(CMD_SELFD);
                G->OrderRouter->GiveOrder(tc);
            }
            delete [] funits;
        }
    }
}
//
//
void Chaser::UpdateSites(){
    //if(!gridmaintainer) return;
    /*for(vector<float3>::iterator i = G->Actions->points.begin(); i != G->Actions->points.end(); ++i){
     Grid->AddValueatMapPos(*i,500.0f);
     }*/
}

float Chaser::ApplyGrudge(int unit, float efficiency){
    return efficiency;
    /*
     if(allyteamGrudges.empty()) return efficiency;
     float e = efficiency;
     int ateam = G->chcb->GetUnitAllyTeam(unit);
     if(allyteamGrudges.find(ateam) == allyteamGrudges.end()) allyteamGrudges[ateam]=1;
     // calculate the average grudge value
     int n=0;
     float total=0;
     float average = 1;
     if(!allyteamGrudges.empty()){
     for(map<int,float>::iterator i = allyteamGrudges.begin(); i != allyteamGrudges.end();++i){
     n++;
     total+=i->second;
     }
     if(n == 0) return e;
     average = total/n;
     float percentage = allyteamGrudges[ateam]/min(average,1.0f);
     return e*percentage;
     }
     return e;*/
}

void Chaser::EnemyDamaged(int damaged, int attacker, float damage, float3 dir){
    NLOG("Chaser::UnitDamaged");
    NO_GAIA(NA)
    //START_EXCEPTION_HANDLING
    /*int ateam = G->chcb->GetUnitAllyTeam(attacker);
    if(ateam != G->Cached->unitallyteam) allyteamGrudges[ateam] -= damage;*/
    //END_EXCEPTION_HANDLING("Chaser::UnitDamaged running away routine!!!!!!")
}

map<int, float> balances;
float average=1;
void Chaser::UpdateMatrixEnemies(){
    NLOG("Chaser::Update :: update threat matrix enemy targets");
    float3 pos = UpVector;
    // get all enemies in los
    int* en = new int[5000];
    int unum = G->chcb->GetEnemyUnits(en);//GetEnemyUnits(en);

    //map<int,float> balances2;
    // go through the list
    if(unum > 0){
        for(int i = 0; i < unum; i++){
            if(ValidUnitID(en[i])){
                //if(G->cb->GetUnitAllyTeam(en[i]) == G->Cached->unitallyteam) continue;
                pos = G->GetUnitPos(en[i]);

                if(Grid.ValidMapPos(pos) == false) continue;
                //Grid.AddValueatMapPos(pos,30000);
                const UnitDef* def = G->GetUnitDef(en[i]);
                if(def){
                    //G->cb->DrawUnit(def->name.c_str(),pos,0,40,2,true,true,0);
                    if(def->extractsMetal > 0.0f) G->M->EnemyExtractor(pos, en[i]);

                    float e = G->GetEfficiency(def->name);//def->power;
                    //float3 nbpos =

                    // Apply grudge modifiers
                    //e = ApplyGrudge(en[i],e);

                    // manipulate the value to make threats bigger as they get closer to the AI base.
                    e /= (pos.distance2D(G->Map->nbasepos(pos))/2);
                    //e /= 10;
                    //int team = G->chcb->GetUnitTeam(en[i]);
                    //if(balances.empty()==false) e*= balances[team]/max(average,1.0f);

                    //e *=5;
                    //e *= 10;
                    // Help maintain consistency by inflating the value of this location
                    Grid.AddValueatMapPos(pos, e);

                    //balances2[team] += e;
                }
                //G->Actions->AddPoint(pos);
            }
        }
    }
    /*if(balances2.empty()==false){
     float total=1;
     int n=0;
     for(map<int,float>::iterator i = balances2.begin(); i != balances2.end(); ++i){
     total += i->second;
     n++;
     }
     average=total/n;
     balances.erase(balances.begin(),balances.end());
     balances.clear();
     balances.insert(balances2.begin(),balances2.end());
     }*/
    delete [] en;
}
//

void Chaser::Update(){
    NLOG("Chaser::Update");
    NO_GAIA(NA)
    // decrease threat values of all sectors..
    /*START_EXCEPTION_HANDLING
     if(EVERY_(67 FRAMES)){
     if(gridmaintainer){
     UpdateMatrixFriendlyUnits();
     }
     }
     END_EXCEPTION_HANDLING("Chaser::UpdateMatrixFriendlyUnits()")*/
    // Run the code that handles stockpiled weapons.
    START_EXCEPTION_HANDLING
    if(EVERY_((10 SECONDS))){
        if((sweap.empty() == false)&&(G->Cached->enemies.empty() == false)){
            NLOG("Chaser::Update :: firing silos");
            FireSilos();
        }
    }
    END_EXCEPTION_HANDLING("Chaser::Update :: firing silos")

    START_EXCEPTION_HANDLING
    if(EVERY_((3 SECONDS))){
        CheckKamikaze();
    }
    END_EXCEPTION_HANDLING("Chaser::CheckKamikaze()")

    START_EXCEPTION_HANDLING
    if((EVERY_((11 FRAMES)))&&(G->Actions->points.empty() == false)){
        UpdateSites();
    }
    END_EXCEPTION_HANDLING("Chaser::UpdateSites()")

    START_EXCEPTION_HANDLING
    Grid.Update(G->GetCurrentFrame());
    END_EXCEPTION_HANDLING("CGridManager::Update() in Chaser class")

    START_EXCEPTION_HANDLING
    if(EVERY_((8 FRAMES))){
        UpdateMatrixEnemies();
    }
    END_EXCEPTION_HANDLING("Chaser::UpdateMatrixEnemies()")


    // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    // re-evaluate the target of an attack force
    //if(EVERY_(919 FRAMES)){
    //	EXCEPTION_HANDLER(EvaluateTargets(),"Chaser::EvaluateTargets()",NA)
    //}
    START_EXCEPTION_HANDLING
    if(EVERY_(120 FRAMES)/*&&(G->enemies.empty() == false)*/){
        FireWeaponsNearby();
    }
    END_EXCEPTION_HANDLING("Chaser::FireWeaponsNearby()")

    START_EXCEPTION_HANDLING
    if(EVERY_((2 MINUTES))){
        for(vector< set<int> >::iterator k = this->attack_groups.begin(); k != attack_groups.end(); ++k){
            if((int)k->size()>threshold){
                FindTarget(*k, false);
            }
        }
    }
    END_EXCEPTION_HANDLING("Chaser::Update :: re-assigning targets")

    START_EXCEPTION_HANDLING
    if(EVERY_(120 FRAMES)&&(G->Cached->enemies.empty() == false)){
        FireDefences();
    }
    END_EXCEPTION_HANDLING("Chaser::FireDefences()")

    START_EXCEPTION_HANDLING
    if(EVERY_(33 FRAMES)){
        FireDgunsNearby();
    }
    END_EXCEPTION_HANDLING("Chaser::FireDgunsNearby()")

    NLOG("Chaser::Update :: DONE");
}

void Chaser::Beacon(float3 pos, float radius){
    if(walking.empty() == false){
        set<int> rejects;
        for(set<int>::iterator i = walking.begin(); i != walking.end(); ++i){
            float3 rpos = G->GetUnitPos(*i);
            if(G->Map->CheckFloat3(rpos)==true){
                if (rpos.distance2D(pos)<radius){
                    G->Actions->MoveToStrike(*i, pos);
                }
            }
        }
    }
}

void Chaser::DoUnitStuff(int aa){
    if(engaged.find(aa) != engaged.end()) return;
    float3 pos = G->GetUnitPos(aa);
    if(G->Map->CheckFloat3(pos)==false) return;
    //if(G->Actions->CopyAttack(aa,Attackers)==false){
    if(G->Actions->AttackNear(aa, 3.5f) == false){
        //if(G->Actions->CopyMove(aa,Attackers)==false){
        /*if(walking.find(aa) == walking.end()){
         //if(G->Actions->SeekOutInterest(aa,900.0f)==false){
         //if(G->Actions->SeekOutLastAssault(aa)==false){
         if(G->Actions->RandomSpiral(aa)==false){
         //	G->Actions->ScheduleIdle(*aa);
         }else{
         walking.insert(aa);
         }
         //}else{
         //	walking.insert(aa);
         //}
         //}else{
         //	walking.insert(aa);
         //}
         }else{
         return;
         }*/
        //}else{
        //	walking.insert(aa);
        //}
    }else{
        engaged.insert(aa);
        Beacon(pos, 500.0f);
    }
    /*}else{
     engaged.insert(aa);
     Beacon(pos,1000);
     }*/
}

void Chaser::FireWeaponsNearby(){
    NLOG("Chaser::Update :: make attackers attack nearby targets");
    if(Attackers.empty() == false){
        for(set<int>::iterator aa = Attackers.begin(); aa != Attackers.end();++aa){
            DoUnitStuff(*aa);
        }
    }
}

void Chaser::FireDgunsNearby(){
    NLOG("Chaser::Update :: make units dgun nearby enemies nearby targets");
    if(dgunners.empty() == false){
        for(set<int>::iterator aa = dgunners.begin(); aa != dgunners.end();++aa){
            G->Actions->DGunNearby(*aa);
        }
    }
}

void Chaser::FireDefences(){
    NLOG("Chaser::Update :: make defences attack nearby targets");
    if(defences.empty() == false){
        for(set<int>::iterator aa = defences.begin(); aa != defences.end();++aa){
            G->Actions->AttackNear(*aa, 1.0f);
        }
    }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

