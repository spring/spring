#include "helper.h"
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

using namespace std;

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
map<string,float> efficiency;
map<string,string> unit_names; //unitname -> human name
bool loaded;
bool firstload;
bool saved;
bool send_to_web;
bool get_from_web;
bool update_NTAI;
int iterations;
int a;



CSunParser* Global::Get_mod_tdf(){
	return mod_tdf;
};
Global::Global(IGlobalAICallback* callback){
	G = this;

	randadd = 0;
	encache = new int[5000];
	enemy_number = 0;
	lastcacheupdate = 0;
	team = 567;

	for(int i = 0; i< 5000; i++){
		encache[i] = 0;
	}
	
	gcb = callback;
	cb = gcb->GetAICallback();
	team = cb->GetMyTeam();
	L.G = this;
	L.Open(true);
	mod_tdf = new CSunParser(this);
	if(L.FirstInstance() == true){
        loaded = false;
		firstload = false;
		iterations = 0;
		saved = false;
		send_to_web = true;
		get_from_web = true;
		update_NTAI = true;
	}
	abstract = true;
	gaia = false;
	spacemod = false;
	dynamic_selection = true;
	use_modabstracts = false;
	absent_abstract = false;
	fire_state_commanders = 0;
	move_state_commanders = 0;
	scout_speed = 60;
	M = new CMetalHandler(this);
	M->loadState();
	/*if(M->hotspot.empty() == false){
		for(vector<float3>::iterator hs = M->hotspot.begin(); hs != M->hotspot.end(); ++hs){
			float3 tpos = *hs;
			tpos.y = cb->GetElevation(tpos.x,tpos.z);
			ctri triangle = Tri(tpos);
			triangles.push_back(triangle);
		}
	}*/
	//if(L.FirstInstance() == true){
        L << " :: Found " << M->m->NumSpotsFound << " Metal Spots" << endline;
	//}

	Pl = new Planning;
	As = new Assigner;
	Sc = new Scouter;
	Fa = new Factor;
	Ch = new Chaser;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Global::~Global(){
	SaveUnitData();
	delete [] encache;
	L.Close();
	delete Fa;
	delete Ch;
	delete Sc;
	delete As;
	delete Pl;
	delete M;
	delete this->mod_tdf;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

int Global::GiveOrder(TCommand c, bool newer){
	NLOG("Global::GiveOrder");
	if(c.unit < 1) return -1;
	if(c.clear == true) return -1;
	if(G->cb->GetCurrentFrame() > c.c.timeOut) return -1;
	if(c.Priority == tc_na){
		L.print("tc_na found");
	}
	if(c.c.params.empty() == true){
		if ((c.c.id != CMD_SELFD)&&(c.c.id != CMD_STOP)&&(c.c.id != CMD_STOCKPILE)&&(c.c.id != CMD_WAIT)){
#ifdef TC_SOURCE
			L.print(_T("empty params?!?!?!? :: ") + c.source);
#endif
			return -1;
		}
	}
	if(newer == true){
		if(CommandCache.empty() == false){
			for(list<TCommand>::iterator i = CommandCache.begin(); i != CommandCache.end();++i){
				if(i->clear == true) continue;
				if(i->unit == c.unit){
					i->clear = true;
				}
			}
		}
	}
	if(c.Priority != tc_na){
		CommandCache.push_back(c);
		//CommandCache.sort();
	}else{
		L << "error :: a command was sent to cache that had no priority =s" << endline;
	}
	return 0;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

int Global::WhichCorner(float3 pos){
	NLOG("Global::WhichCorner");
	// 1= NW, 2=NE,  3= SW, 4=SE
	if ((pos.x<(cb->GetMapWidth() / 2))&&(pos.z<(cb->GetMapHeight()/ 2))) return t_NW;
	if ((pos.x>(cb->GetMapWidth() / 2))&&(pos.z<(cb->GetMapHeight() / 2))) return t_NE;
	if ((pos.x<(cb->GetMapWidth() / 2))&&(pos.z>(cb->GetMapHeight() / 2))) return t_SW;
	if ((pos.x>(cb->GetMapWidth() / 2))&&(pos.z>(cb->GetMapHeight() / 2))) return t_SE;
	return 0;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::Update(){
	if(cb->GetCurrentFrame() == 1 SECOND){
		if(L.FirstInstance() == true){
			cb->SendTextMsg(":: NTAI XE7.5 by AF",0);
			cb->SendTextMsg(":: Learning NTAI V0.39",0);
			cb->SendTextMsg(":: Copyright (C) 2006 AF",0);
			cb->SendTextMsg(":: Metal class Krogothe/Cain",0);
			string s = mod_tdf->SGetValueMSG("AI\\message");
			if(s != string("")){
                cb->SendTextMsg(s.c_str(),0);
			}
		}
	}
	if(EVERY_(2 MINUTES)){
		SaveUnitData();
	}
	if(EVERY_(8 FRAMES)){
		int hg = CommandCache.size();
		if( hg >20){
			for(int j = 2;  j <hg ; j++){
				for(list<TCommand>::iterator i = CommandCache.begin(); i != CommandCache.end();++i){
					if(i->clear == true){
						CommandCache.erase(i);
						break;
					}
				}
			}
		}
	}
	if(EVERY_(3 FRAMES /*+ team*/)){
		if(L.FirstInstance() == true){
			a=0;
		}else if (a == BUFFERMAX){
			a -= 2;
		}
		if((CommandCache.empty() == false)&&(a != BUFFERMAX)){
			NLOG("(InstantCache.empty() == false)&&(a != BUFFERMAX)");
			for(list<TCommand>::iterator i = CommandCache.begin();i != CommandCache.end();++i){
				if(i->clear == true) continue;
				if(i->c.timeOut < cb->GetCurrentFrame()){
					i->clear = true;
					continue;
				}
                if(cb->GetCurrentFrame() > i->created + i->delay){
					Command gc = i->c;
					if(cb->GiveOrder(i->unit,&gc) == -1){
						L.print("hmm failed task update()");
						i->clear = true;
						UnitIdle(i->unit);
						break;
					}else{
						i->clear = true;
						a++;
						if(a == BUFFERMAX){
							break;
						}
					}
				}
			}
		}
	}
	As->Update();
	Fa->Update();
	Ch->Update();
/*	if(EVERY_(1 SECOND)){
		if(triangles.empty() == false){
			NLOG(_T("triangles.empty() == false"));
			if((L.FirstInstance() == true)&&(EVERY_( 3 FRAMES))){
				int tricount = 0;
				for(vector<ctri>::iterator ti = triangles.begin(); ti != triangles.end(); ++ti){
					if(ti->bad == true){
						tricount++;
						continue;
					}
					Increment(ti, G->cb->GetCurrentFrame());
					if((ti->lifetime+ti->creation+1)>cb->GetCurrentFrame()){
						Draw(*ti);
					}else{
						ti->bad = true;
					}
				}
				for(int gk = 0; gk < tricount; gk++){
					for(vector<ctri>::iterator ti = triangles.begin(); ti != triangles.end(); ++ti){
						if(ti->bad == true){
							triangles.erase(ti);
							break;
						}
					}
				}
			}
		}
	}*/
	NLOG(_T("Global::Update :: done"));
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::EnemyDestroyed(int enemy,int attacker){
	enemies.erase(enemy);
	if(attacker > 0){
		const UnitDef* uda = cb->GetUnitDef(attacker);
		if(uda != 0){
			if(efficiency.find(uda->name) != efficiency.end()){
				efficiency[uda->name] += 20/uda->metalCost;
			}else{
				efficiency[uda->name] = 20/uda->metalCost;
			}
		}
		const UnitDef* ude = cb->GetUnitDef(attacker);
		if(uda != 0){
			if(efficiency.find(ude->name) != efficiency.end()){
				efficiency[ude->name] += 10/ude->metalCost;
			}else{
				efficiency[ude->name] = 10/ude->metalCost;
			}
		}
	}
	Ch->EnemyDestroyed(enemy);
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitFinished(int unit){
	Ch->UnitFinished(unit);
	As->UnitFinished(unit);
	Fa->UnitFinished(unit);
	Sc->UnitFinished(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitMoveFailed(int unit){
	Sc->UnitMoveFailed(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitIdle(int unit){
	Fa->UnitIdle(unit);
	Sc->UnitIdle(unit);
	Ch->UnitIdle(unit);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Global::Crash(){
	// close the logfile
	SaveUnitData();
	L.header(_T("\n :: The user has initiated a crash, terminating NTAI \n"));
	L.Close();
#ifdef _DEBUG
	L.print(_T("breaking"));
#endif
#ifndef _DEBUG
	// Create an exception forcing spring to close
	vector<string> cv;
	string n = cv.back();
#endif
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	if(attacker > 0){
		const UnitDef* uda = cb->GetUnitDef(attacker);
		if(uda != 0){
			if(efficiency.find(uda->name) != efficiency.end()){
				efficiency[uda->name] += 1/uda->metalCost;
			}else{
				efficiency[uda->name] = 1/uda->metalCost;
			}
		}
	}
	Fa->UnitDamaged(damaged,attacker,damage,dir);
	Ch->UnitDamaged(damaged,attacker,damage,dir);
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/* returns the base position nearest to the given float3*/
float3 Global::nbasepos(float3 pos){ 
	if(base_positions.empty() == false){
		float3 best;
		float best_score = 20000000;
		for(vector<float3>::iterator i = base_positions.begin(); i != base_positions.end(); ++i){
			if( i->distance2D(pos) < best_score){
				best = *i;
				best_score = i->distance2D(pos);
			}
		}
		return best;
	}else{
		return basepos;
	}
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::GotChatMsg(const char* msg,int player){
	L.Message(msg,player);
	string tmsg = msg;
	if(tmsg == string(_T(".verbose"))){
		if(L.Verbose()){
			L.iprint(_T("Verbose turned on"));
		} else L.iprint(_T("Verbose turned off"));
	}else if(tmsg == string(_T(".crash"))){
		Crash();
	}else if(tmsg == string(_T(".break"))){
		if(L.FirstInstance() == true)L.print(_T("The user initiated debugger break"));
	}else if(tmsg == string(_T(".isfirst"))){
		if(L.FirstInstance() == true) L.iprint(_T(" info :: This is the first NTAI instance"));
	}else if(tmsg == string(_T(".save"))){
		if(L.FirstInstance() == true) SaveUnitData();
	}else if(tmsg == string(_T(".reload"))){
		if(L.FirstInstance() == true)LoadUnitData();
	}else if(tmsg == string(_T(".flush"))){
		L.Flush();
	}else if(tmsg == string(_T(".threat"))){
		Ch->MakeTGA();
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitDestroyed(int unit, int attacker){
	if(attacker >0){
		const UnitDef* uda = cb->GetUnitDef(attacker);
		const UnitDef* udu = cb->GetUnitDef(unit);
		if((uda != 0)&&(udu != 0)){
			if(efficiency.find(uda->name) != efficiency.end()){
				efficiency[uda->name] += 20/udu->metalCost;
			}else{
				efficiency[uda->name] = 20/udu->metalCost;
			}
			if(efficiency.find(udu->name) != efficiency.end()){
				efficiency[udu->name] -= 10/uda->metalCost;
			}else{
				efficiency[udu->name] = 10/uda->metalCost;
			}
		}
	}
	As->UnitDestroyed(unit);
    Fa->UnitDestroyed(unit);
	Sc->UnitDestroyed(unit);
	Ch->UnitDestroyed(unit,attacker);
	if(CommandCache.empty() == false){
		for(list<TCommand>::iterator i = CommandCache.begin(); i != CommandCache.end();++i){
			if(i->unit == unit){
				i->clear = true;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::InitAI(IAICallback* callback, int team){
	CSunParser sf(G);
	sf.LoadFile("modinfo.tdf");
	tdfpath =  sf.SGetValueDef(string(cb->GetModName()),"MOD\\NTAI\\tdfpath");
	string filename = string(_T("aidll/globalai/NTAI")) + slash + tdfpath + slash + string(_T("mod.tdf"));
	ifstream fp;
	fp.open(filename.c_str(), ios::in);
	if(fp.is_open() == true){
		char buffer [10000];
		int bsize = 0;
		char in_char;
		while(fp.get(in_char)){
			buffer[bsize] = in_char;
			bsize++;
			if(bsize == 9999) break;
		}
		mod_tdf->LoadBuffer(buffer,bsize);
	} else {/////////////////
		abstract = true;
		L.header(_T(" :: mod.tdf failed to load, assuming default values"));
	}
	//load all the mod.tdf settings!
	gaia = false;
	mod_tdf->GetDef(abstract, "1", "AI\\abstract");
	mod_tdf->GetDef(gaia, "0", "AI\\GAIA");
	mod_tdf->GetDef(spacemod, "0", "AI\\spacemod");
	mod_tdf->GetDef(mexfirst, "0", "AI\\first_attack_mexraid");
	mod_tdf->GetDef(hardtarget, "0", "AI\\hard_target");
	mod_tdf->GetDef(mexscouting, "1", "AI\\mexscouting");
	mod_tdf->GetDef(dynamic_selection, "1", "AI\\dynamic_selection");
	mod_tdf->GetDef(use_modabstracts, "0", "AI\\use_mod_default");
	mod_tdf->GetDef(absent_abstract, "1", "AI\\use_mod_default_if_absent");
	//mod_tdf->GetDef(send_to_web, "1", "AI\\web_contribute");
	//mod_tdf->GetDef(get_from_web, "1", "AI\\web_recieve");
	//mod_tdf->GetDef(update_NTAI, "1", "AI\\NTAI_update");
	fire_state_commanders = atoi(mod_tdf->SGetValueDef("0", "AI\\fire_state_commanders").c_str());
	move_state_commanders = atoi(mod_tdf->SGetValueDef("0", "AI\\move_state_commanders").c_str());
	scout_speed = atof(mod_tdf->SGetValueDef("50", "AI\\scout_speed").c_str());
	if(abstract == true) dynamic_selection = true;
	if(use_modabstracts == true) abstract = false;

	if(abstract == true){
		L.header(_T(" :: Using abstract buildtree"));
		L.header(endline);
	}
	if(gaia == true){
		L.header(_T(" :: GAIA AI On"));
		L.header(endline);
	}
	L.header(endline);
	if(loaded == false){
		LoadUnitData();
	}

	As->InitAI(this);
	Pl->InitAI(this);
	Fa->InitAI(this);
	Sc->InitAI(this);
	Ch->InitAI(this);
	
}

string Global::lowercase(string s){
	string ss = s;
	int (*pf)(int)=tolower;
	transform(ss.begin(), ss.end(), ss.begin(), pf);
	return ss;
}

bool Global::ReadFile(string filename, string* buffer){
	int ebsize= 0;
	ifstream fp;
	fp.open(filename.c_str(), ios::in);
	if(fp.is_open() == false){
		L.print(string(_T("error loading file :: ")) + filename);
		return false;
	}else{
		*buffer = _T("");
		char in_char;
		while(fp.get(in_char)){
			buffer->push_back(in_char);
			ebsize++;
		}
		if(ebsize == 0){
			L.print(string(_T("error loading contents of file :: ")) + filename);
			return false;
		}else{
            return true;
		}
	}
}
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||


vector<float3> Global::GetSurroundings(float3 pos){
	vector<float3> s;
	float3 ss = WhichSector(pos);
	s.push_back(ss);
	ss.z = ss.z - 1;
	s.push_back(ss);
	ss.z = ss.z +2;
	s.push_back(ss);
	ss.z = ss.z -1;
	ss.x = ss.x - 1;
	s.push_back(ss);
	ss.z = ss.z - 1;
	s.push_back(ss);
	ss.z = ss.z +2;
	s.push_back(ss);
	ss.z = ss.z - 1;
	ss.x = ss.x + 2;
	s.push_back(ss);
	ss.z = ss.z - 1;
	s.push_back(ss);
	ss.z = ss.z +2;
	s.push_back(ss);
	return s;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

float3 Global::WhichSector(float3 pos){
	if((pos == ZeroVector)||(pos == UpVector)){
		return UpVector;
	}
	if(pos.x < 0) return UpVector;
	if(pos.z < 0) return UpVector;
	if(pos.x > cb->GetMapWidth()*8) return UpVector;
	if(pos.z > cb->GetMapHeight()*8) return UpVector;
	float3 spos;
	//x = int(pos.x/xSize);
	//y = int(pos.z/ySize);
	spos.x = pos.x/Ch->xSize;
	spos.y = pos.y;
	spos.z = pos.z/Ch->ySize;
	return spos;
}

float3 Global::Rotate(float3 pos, float angle, float3 origin){
    float3 newpos;
    newpos.x = (pos.x-origin.x)*cos(angle) - (pos.z-origin.z)*sin(angle);
	newpos.z = (pos.x-origin.x)*sin(angle) + (pos.z-origin.z)*cos(angle);
	newpos.x += origin.x;
	newpos.z += origin.z;
	newpos.y = pos.y;
    return newpos;
}

void Global::Draw(ctri triangle){
	int t1 = G->cb->CreateLineFigure(triangle.a,triangle.b,4,0,triangle.fade,0);
	int t2 = G->cb->CreateLineFigure(triangle.b,triangle.c,4,0,triangle.fade,0);
	int t3 = G->cb->CreateLineFigure(triangle.c,triangle.a,4,0,triangle.fade,0);
	G->cb->SetFigureColor(t1,triangle.colour.red,triangle.colour.green,triangle.colour.blue,triangle.alpha);
	G->cb->SetFigureColor(t2,triangle.colour.red,triangle.colour.green,triangle.colour.blue,triangle.alpha);
	G->cb->SetFigureColor(t3,triangle.colour.red,triangle.colour.green,triangle.colour.blue,triangle.alpha);
}

ctri Global::Tri(float3 pos, float size, float speed, int lifetime, int fade, int creation){
	ctri Triangle;
	Triangle.d = size;
	Triangle.position = pos;
	Triangle.position.y = cb->GetElevation(pos.x,pos.y) + 30;
	Triangle.speed = speed;
	Triangle.flashy = false;
	Triangle.a = Triangle.position;
	Triangle.a.z = Triangle.a.z - size;
	Triangle.b = Triangle.c = Triangle.a;
	Triangle.b = Rotate(Triangle.a,120 DEGREES,Triangle.position);
	Triangle.c = Rotate(Triangle.b,120 DEGREES,Triangle.position);
	Triangle.lifetime = lifetime;
	Triangle.creation = creation;
	Triangle.fade = fade;
	Triangle.alpha = Triangle.colour.alpha = 0.6f;
	Triangle.colour.red = 0.7f;
	Triangle.colour.green = 0.9f;
	Triangle.colour.blue =  0.01f;
	Triangle.bad = false;
	return Triangle;
}

void Global::Increment(vector<ctri>::iterator triangle, int frame){
	triangle->a = Rotate(triangle->a,triangle->speed DEGREES,triangle->position);
	triangle->b = Rotate(triangle->b,triangle->speed DEGREES,triangle->position);
	triangle->c = Rotate(triangle->c,triangle->speed DEGREES,triangle->position);
}


int Global::GetEnemyUnits(int* units, const float3 &pos, float radius){
	NLOG(_T("Global::GetEnemyUnits :: A"));
	if(cb->GetCurrentFrame() > lastcacheupdate + (1 SECOND)){
		enemy_number = cb->GetEnemyUnits(encache);
		lastcacheupdate = cb->GetCurrentFrame();
	}
	return cb->GetEnemyUnits(units,pos,radius);
}


int Global::GetEnemyUnits(int* units){
	NLOG(_T("Global::GetEnemyUnits :: B"));
	if(cb->GetCurrentFrame() > lastcacheupdate + (1 SECOND)){
		enemy_number = cb->GetEnemyUnits(encache);
		lastcacheupdate = cb->GetCurrentFrame();
	}
	for(int h = 0; h < enemy_number; h++){
		units[h] = encache[h];
	}
	return enemy_number;
}


map<int,float3> Global::GetEnemys(){
	NLOG(_T("Global::GetEnemyUnits :: C"));
	return enemy_cache;
}


map<int,float3> Global::GetEnemys(float3 pos, float radius){
	NLOG(_T("Global::GetEnemyUnits :: D"));
	return enemy_cache;
}

float Global::GetEfficiency(string s){
	if(efficiency.find(s) != efficiency.end()){
		float x = efficiency[s];
		if(x <20){
			L.iprint(_T("error ::  efficiency of ") + s + _T(" is less than 20"));
			return 100;
		}else{
			return x;
		}
	}else{
		L.iprint(_T("error ::   ") + s + _T(" is missing from the efficiency array"));
		return 100;
	}
}

bool Global::LoadUnitData(){
	if(L.FirstInstance() == true){
		int unum = cb->GetNumUnitDefs();
		const UnitDef** ulist = new const UnitDef*[unum];
		cb->GetUnitDefList(ulist);
		for(int i = 0; i < unum; i++){
			float ts = ulist[i]->health+ulist[i]->energyMake + ulist[i]->metalMake + ulist[i]->extractsMetal*50+ulist[i]->tidalGenerator*30 + ulist[i]->windGenerator*30;
			if(ts > 20){
				efficiency[ulist[i]->name] = ts;
			}else{
				efficiency[ulist[i]->name] = 1;
			}
			unit_names[ulist[i]->name] = ulist[i]->humanName;
		}
		ifstream io;
		string filename;
		filename = _T("aidll/globalai/NTAI");
		filename += slash;
		filename += _T("learn");
		filename += slash;
		filename += cb->GetModName();
		filename += _T(".tdf");
		io.open(filename.c_str(), ios::in);
		if(io.is_open() == true){
			char buffer [20000];
			int buffer_size = 0;
			char in_char;
			while(io.get(in_char)){
				buffer[buffer_size] = in_char;
				buffer_size++;
				if(buffer_size == 19999){
					G->L.print(_T("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n error tdf file too large, please notify AF that the buffer needs enlarging in the next release of NTAI \n |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"));
					break;
				}
			}
			CSunParser cq(this);
			cq.LoadBuffer(buffer, buffer_size);
			for(map<string,float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
				string s = "AI\\";
				s += i->first;
				float ank = atoi(cq.SGetValueDef("10", s.c_str()).c_str());
				if(ank > i->second) i->second = ank;
			}
			iterations = atof(cq.SGetValueDef(_T("1"), _T("AI\\iterations")).c_str());
			iterations++;
			cq.GetDef(firstload, _T("1"), _T("AI\\firstload"));
			if(firstload == true){
				L.iprint(_T(" This is the first time this mod ahs been loaded, up. Take this first game to train NTAI up, and be careful fo throwing the same units at it over and over again"));
				firstload = false;
				for(map<string,float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
					const UnitDef* uda = cb->GetUnitDef(i->first.c_str());
					if(uda !=0){
						i->second += uda->health;
					}
				}
			}
			loaded = true;
			return true;
		} else{
			SaveUnitData();
			return false;
		}
	}
	return false;
}

bool Global::SaveUnitData(){
	if(L.FirstInstance() == true){
		ofstream off;
		string filename;
		filename = _T("aidll/globalai/NTAI");
		filename += slash;
		filename += _T("learn");
		filename += slash;
		filename += cb->GetModName();
		filename += _T(".tdf");
		off.open(filename.c_str());
		if(off.is_open() == true){
			off << _T("[AI]") << endl << _T("{") << endl << _T("    // NTAI XE7.5 AF :: unit efficiency cache file") << endl << endl;
			// put stuff in here;
			int first = firstload;
			off << _T("    version=XE7.5;") << endl;
			off << _T("    firstload=") << first << _T(";") << endl;
			off << _T("    modname=") << G->cb->GetModName() << _T(";") << endl;
			off << _T("    iterations=") << iterations << _T(";") << endl;
			off << endl;
			for(map<string,float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
				off << _T("    ")<< i->first << _T("=") << i->second << _T(";    // ") << unit_names[i->first] <<endl;
			}
			off << _T("}") << endl;
			off.close();
			saved = true;
			return true;
		}else{
			off.close();
			return false;
		}
	}
	return false;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/** unitdef->type
	MetalExtractor
	Transport
	Builder
	Factory
	Bomber
	Fighter
	GroundUnit
	Building
**/

