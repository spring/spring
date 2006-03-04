#include "helper.h"
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

using namespace std;

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
map<string,float> efficiency;
bool loaded;
bool saved;
int a;

Global::Global(IGlobalAICallback* callback){
	G = this;
	encache = new int[5000];
	enemy_number = 0;
	lastcacheupdate = 0;
	abstract = false;
	for(int i = 0; i< 5000; i++){
		encache[i] = 0;
	}
	badcount = 0;
	gcb = callback;
	cb = gcb->GetAICallback();
	ccb = 0;
	team = cb->GetMyTeam();
	L.G = this;
	L.Open(true);
	if(L.FirstInstance() == true){
        loaded = false;
		saved = false;
	}
	M = new CMetalHandler(cb);
	M->loadState();
	if(M->hotspot.empty() == false){
		for(vector<float3>::iterator hs = M->hotspot.begin(); hs != M->hotspot.end(); ++hs){
			float3 tpos = *hs;
			tpos.y = cb->GetElevation(tpos.x,tpos.z);
			ctri triangle = Tri(tpos);
			triangles.push_back(triangle);
		}
	}
	if(L.FirstInstance() == true){
        L << " :: Found " << M->m->NumSpotsFound << " Metal Spots" << endline;
	}

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
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

int Global::GiveOrder(TCommand c){
	NLOG("Global::GiveOrder");
	if(c.unit < 1) return -1;
	if(c.clear == true) return -1;
	if(G->cb->GetCurrentFrame() > c.c.timeOut) return -1;
	if(InstantCache.empty() == false){
		for(list<TCommand>::iterator i = InstantCache.begin(); i != InstantCache.end();++i){
			if(i->unit == c.unit){
				i->clear = true;
				badcount++;
			}
		}
	}
	if(PseudoCache.empty() == false){
		for(list<TCommand>::iterator i = PseudoCache.begin(); i != PseudoCache.end();++i){
			if(i->unit == c.unit){
				i->clear = true;
				badcount++;
			}
		}
	}
	if(AssignCache.empty() == false){
		for(list<TCommand>::iterator i = AssignCache.begin(); i != AssignCache.end();++i){
			if(i->unit == c.unit){
				i->clear = true;
				badcount++;
			}
		}
	}
	if(LowCache.empty() == false){
		for(list<TCommand>::iterator i = LowCache.begin(); i != LowCache.end();++i){
			if(i->unit == c.unit){
				i->clear = true;
				badcount++;
			}
		}
	}
	if(c.Priority == tc_instant){
		InstantCache.push_back(c);
	}else if(c.Priority == tc_pseudoinstant){
		PseudoCache.push_back(c);
	}else if(c.Priority == tc_assignment){
		AssignCache.push_back(c);
	}else if(c.Priority == tc_low){
		LowCache.push_back(c);
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
	if(EVERY_(3 FRAMES)){
		if(badcount > 20){
			G->L.print("badcount too high, erasing all clear values");
			int bcount = badcount;
			for(int i = 0; i < bcount ; i++){
				if(InstantCache.empty() == false){
					for(list<TCommand>::iterator i = InstantCache.begin(); i != InstantCache.end();++i){
						if(i->clear == true){
							InstantCache.erase(i);
							--badcount;
							break;
						}
					}
				}
				if(PseudoCache.empty() == false){
					for(list<TCommand>::iterator i = PseudoCache.begin(); i != PseudoCache.end();++i){
						if(i->clear == true){
							PseudoCache.erase(i);
							--badcount;
							break;
						}
					}
				}
				if(AssignCache.empty() == false){
					for(list<TCommand>::iterator i = AssignCache.begin(); i != AssignCache.end();++i){
						if(i->clear == true){
							AssignCache.erase(i);
							--badcount;
							break;
						}
					}
				}
				if(LowCache.empty() == false){
					for(list<TCommand>::iterator i = LowCache.begin(); i != LowCache.end();++i){
						if(i->clear == true){
							LowCache.erase(i);
							--badcount;
							break;
						}
					}
				}
			}
		}
		if(L.FirstInstance() == true){
			a=0;
		}
		if((InstantCache.empty() == false)&&(a != BUFFERMAX)){
			NLOG("(InstantCache.empty() == false)&&(a != BUFFERMAX)");
			for(list<TCommand>::iterator i = InstantCache.begin();i != InstantCache.end();++i){
				if(i->clear == true) continue;
				if(i->c.timeOut < cb->GetCurrentFrame()){
					i->clear = true;
					badcount++;
					continue;
				}
                if(cb->GetCurrentFrame() > i->created + i->delay){
					Command gc = i->c;
					if(cb->GiveOrder(i->unit,&gc) == -1){
						L.print("hmm failed task update()");
						i->clear = true;
						badcount++;
						UnitIdle(i->unit);
						break;
					}else{
						i->clear = true;
						badcount++;
						a++;
						if(a == BUFFERMAX){
							break;
						}
					}
				}
			}
		}
		if((PseudoCache.empty() == false)&&(a != BUFFERMAX)){
			NLOG("(PseudoCache.empty() == false)&&(a != BUFFERMAX)");
			for(list<TCommand>::iterator i = PseudoCache.begin();i != PseudoCache.end();++i){
				if(i->clear == true) continue;
				if(i->c.timeOut < cb->GetCurrentFrame()){
					i->clear = true;
					badcount++;
					continue;
				}
				if(cb->GetCurrentFrame() > (i->created + i->delay)){
					Command gc = i->c;
					int u = i->unit;
					if(cb->GiveOrder(u,&gc) == -1){
						L.print("hmm failed task update()");
						i->clear = true;
						badcount++;
						UnitIdle(i->unit);
						break;
					}else{
						i->clear = true;
						badcount++;
						a++;
						if(a == BUFFERMAX){
							break;
						}
					}
				}
			}
		}
		if((AssignCache.empty() == false)&&(a != BUFFERMAX)){
			NLOG("(AssignCache.empty() == false)&&(a != BUFFERMAX)");
			for(list<TCommand>::iterator i = AssignCache.begin(); i != AssignCache.end();++i){
				if(i->clear == true) continue;
				if(i->c.timeOut < cb->GetCurrentFrame()){
					i->clear = true;
					badcount++;
					continue;
				}
				if(cb->GetCurrentFrame() > i->created + i->delay){
					Command gc = i->c;
					if(cb->GiveOrder(i->unit,&gc) == -1){
						L.print("hmm failed task update()");
						i->clear = true;
						badcount++;
						UnitIdle(i->unit);
						break;
					}else{
						i->clear = true;
						badcount++;
						a++;
						if(a == BUFFERMAX){
							break;
						}
					}
				}
			}
		}
		if((LowCache.empty() == false)&&(a != BUFFERMAX)){
			NLOG("(LowCache.empty() == false)&&(a != BUFFERMAX)");
			for(list<TCommand>::iterator i = LowCache.begin(); i != LowCache.end();++i){
				if(i->clear == true) continue;
				if(i->c.timeOut < cb->GetCurrentFrame()){
					i->clear = true;
					badcount++;
					continue;
				}
				if(cb->GetCurrentFrame() > i->created + i->delay){
					Command gc = i->c;
					if(cb->GiveOrder(i->unit,&gc) == -1){
						L.print("hmm failed task update()");
						i->clear = true;
						badcount++;
						UnitIdle(i->unit);
						break;
					}else{
						i->clear = true;
						a++;
						badcount++;
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
	if(InstantCache.empty() == false){
		NLOG("InstantCache.empty() == false");
		for(list<TCommand>::iterator i = InstantCache.begin(); i != InstantCache.end();++i){
			if(i->clear == true){
				InstantCache.erase(i);
				badcount--;
				break;
			}
		}
	}
	if(PseudoCache.empty() == false){
		NLOG("PseudoCache.empty() == false");
		for(list<TCommand>::iterator i = PseudoCache.begin(); i != PseudoCache.end();++i){
			if(i->clear == true){
				PseudoCache.erase(i);
				badcount--;
				break;
			}
		}
	}
	if(AssignCache.empty() == false){
		NLOG("AssignCache.empty() == false");
		for(list<TCommand>::iterator i = AssignCache.begin(); i != AssignCache.end();++i){
			if(i->clear == true){
				AssignCache.erase(i);
				badcount--;
				break;
			}
		}
	}
	if(LowCache.empty() == false){
		NLOG("LowCache.empty() == false");
		for(list<TCommand>::iterator i = LowCache.begin(); i != LowCache.end();++i){
			if(i->clear == true){
				LowCache.erase(i);
				badcount--;
				break;
			}
		}
	}
	if(triangles.empty() == false){
		NLOG("triangles.empty() == false");
		if((L.FirstInstance() == true)&&(EVERY_( 3 FRAMES))){
			int tricount = 0;
            for(vector<ctri>::iterator ti = triangles.begin(); ti != triangles.end(); ++ti){
				if(ti->bad = true){
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
	NLOG("Global::Update :: done");
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::EnemyDestroyed(int enemy,int attacker){
	const UnitDef* uda = cb->GetUnitDef(attacker);
	if(uda != 0){
		if(efficiency.find(uda->name) != efficiency.end()){
			efficiency[uda->name] += 1;
		}else{
			efficiency[uda->name] = 1;
		}
	}
	Ch->EnemyDestroyed(enemy);
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitFinished(int unit){
	As->UnitFinished(unit);
	Fa->UnitFinished(unit);
	Sc->UnitFinished(unit);
	Ch->UnitFinished(unit);
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
	L.header("\n :: The user has initiated a crash, terminating NTAI \n");
	L.Close();
	// Create an exception forcing spring to close
	vector<string> cv;
	string n = cv.back();
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitDamaged(int damaged,int attacker,float damage,float3 dir){
	Fa->UnitDamaged(damaged,attacker,damage,dir);
	Ch->UnitDamaged(damaged,attacker,damage,dir);
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::GotChatMsg(const char* msg,int player){
	L.Message(msg,player);
	string tmsg = msg;
	string verb = ".verbose";
	if(tmsg == verb){
		if(L.Verbose()){
			L.iprint("Verbose turned on");
		} else L.iprint("Verbose turned off");
	}
	verb = ".crash";
	if(tmsg == verb){
		Crash();
	}
	verb = ".cheat";
	if(tmsg == verb){
		ccb = gcb->GetCheatInterface();
	}
	verb = ".break";
	if(tmsg == verb){
		if(L.FirstInstance() == true){
			L.print("The user initiated debugger break");
		}
	}
	
	verb = ".isfirst";
	if(tmsg == verb){
		if(L.FirstInstance() == true){
			L.iprint(" info :: This is the first NTAI instance");
		}
	}
	verb = ".save";
	if(tmsg == verb){
		if(L.FirstInstance() == true){
			SaveUnitData();
		}
	}
	verb = ".reload";
	if(tmsg == verb){
		if(L.FirstInstance() == true){
			LoadUnitData();
		}
	}
	verb = ".flush";
	if(tmsg == verb){
		L.Flush();
	}
	verb = ".speed";
	if(tmsg == verb){
		const UnitDef* ud = G->cb->GetUnitDef("CORFAST");
		char q[20];
		sprintf(q,"%f",ud->speed);
		string s = "CORFAST:: ";
		s += q;
		G->L.iprint(s);
		const UnitDef* uf = G->cb->GetUnitDef("CORTHUD");
		char fg[20];
		sprintf(fg,"%f",uf->speed);
		s = "CORTHUD:: ";
		s += fg;
		G->L.iprint(s);
	}
	verb = ".threat";
	if(tmsg == verb){
		Ch->MakeTGA();
	}
	verb = ".flushcommand";
	if(tmsg == verb){
		for(int gh = 0; gh < badcount; ++gh){
			if(InstantCache.empty() == false){
				for(list<TCommand>::iterator i = InstantCache.begin(); i != InstantCache.end();++i){
					if(i->clear == true){
						InstantCache.erase(i);
						++gh;
						break;
					}
				}
			}
			if(PseudoCache.empty() == false){
				for(list<TCommand>::iterator i = PseudoCache.begin(); i != PseudoCache.end();++i){
					if(i->clear == true){
						PseudoCache.erase(i);
						++gh;
						break;
					}
				}
			}
			if(AssignCache.empty() == false){
				for(list<TCommand>::iterator i = AssignCache.begin(); i != AssignCache.end();++i){
					if(i->clear == true){
						AssignCache.erase(i);
						++gh;
						break;
					}
				}
			}
			if(LowCache.empty() == false){
				for(list<TCommand>::iterator i = LowCache.begin(); i != LowCache.end();++i){
					if(i->clear == true){
						LowCache.erase(i);
						++gh;
						break;
					}
				}
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::UnitDestroyed(int unit, int attacker){
	const UnitDef* uda = cb->GetUnitDef(attacker);
	if(uda != 0){
		if(efficiency.find(uda->name) != efficiency.end()){
			efficiency[uda->name] += 1;
		}else{
			efficiency[uda->name] = 1;
		}
	}
	As->UnitDestroyed(unit);
    Fa->UnitDestroyed(unit);
	Sc->UnitDestroyed(unit);
	Ch->UnitDestroyed(unit);
	if(InstantCache.empty() == false){
		for(list<TCommand>::iterator i = InstantCache.begin(); i != InstantCache.end();++i){
			if(i->unit == unit){
				i->clear = true;
				badcount++;
			}
		}
	}
	if(PseudoCache.empty() == false){
		for(list<TCommand>::iterator i = PseudoCache.begin(); i != PseudoCache.end();++i){
			if(i->unit == unit){
				i->clear = true;
				badcount++;
			}
		}
	}
	if(AssignCache.empty() == false){
		for(list<TCommand>::iterator i = AssignCache.begin(); i != AssignCache.end();++i){
			if(i->unit == unit){
				i->clear = true;
				badcount++;
			}
		}
	}
	if(LowCache.empty() == false){
		for(list<TCommand>::iterator i = LowCache.begin(); i != LowCache.end();++i){
			if(i->unit == unit){
				i->clear = true;
				badcount++;
			}
		}
	}
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Global::InitAI(IAICallback* callback, int team){
	L << " :: InitAI" << endline;
	string filename;
	ifstream fp;
	filename = "NTAI/";
	filename += cb->GetModName();
	filename += "/mod.tdf";
	fp.open(filename.c_str(), ios::in);
	if(fp.is_open() == true){
		char buffer [5000];
		int bsize = 0;
		char in_char;
		while(fp.get(in_char)){
			buffer[bsize] = in_char;
			bsize++;
			if(bsize == 4999) break;
		}
		CSunParser cq(this);
		cq.LoadBuffer(buffer,bsize);
		cq.GetDef(abstract, "0", "AI\\abstract");
		if(abstract == true){
			L.print("abstract!!!");
		}else{
			L.print("not abstract");
		}
	} else {
		abstract = true;
		L.print("abstract!!! (mod.tdf dont exist?)");
	}
	As->InitAI(this);
	Pl->InitAI(this);
	Fa->InitAI(this);
	Sc->InitAI(this);
	Ch->InitAI(this);
	if(loaded == false){
		LoadUnitData();
	}
	L << " :: InitAI finished" << endline;
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
	float3 spos;
	spos.x = pos.x/(int)Ch->xSize;
	spos.y = 0;
	spos.z = pos.z/(int)Ch->ySize;
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
	NLOG("Global::GetEnemyUnits :: A");
	if(cb->GetCurrentFrame() > lastcacheupdate + 5){
		enemy_number = cb->GetEnemyUnits(encache);
		lastcacheupdate = cb->GetCurrentFrame();
	}
	return cb->GetEnemyUnits(units,pos,radius);
}


int Global::GetEnemyUnits(int* units){
	NLOG("Global::GetEnemyUnits :: B");
	if(cb->GetCurrentFrame() > lastcacheupdate + 5){
		enemy_number = cb->GetEnemyUnits(encache);
		lastcacheupdate = cb->GetCurrentFrame();
	}
	for(int h = 0; h < enemy_number; h++){
		units[h] = encache[h];
	}
	return enemy_number;
}


map<int,float3> Global::GetEnemys(){
	NLOG("Global::GetEnemyUnits :: C");
	return enemy_cache;
}


map<int,float3> Global::GetEnemys(float3 pos, float radius){
	NLOG("Global::GetEnemyUnits :: D");
	return enemy_cache;
}

float Global::GetEfficiency(string s){
	return efficiency[s];
}

bool Global::LoadUnitData(){
	if(L.FirstInstance() == true){
		ifstream io;
		string filename;
		filename = "NTAI/learn/";
		filename += cb->GetModName();
		filename += ".tdf";
		io.open(filename.c_str(), ios::in);
		if(io.is_open() == true){
			char buffer [10000];
			int buffer_size = 0;
			char in_char;
			while(io.get(in_char)){
				buffer[buffer_size] = in_char;
				buffer_size++;
				if(buffer_size == 9999){
					G->L.print("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n error tdf file too large, please notify AF that the buffer needs enlarging in the next release of NTAI \n |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||");
					break;
				}
			}
			CSunParser cq(this);
			cq.LoadBuffer(buffer, buffer_size);
			for(map<string,float>::iterator i = efficiency.begin(); i != efficiency.end(); ++i){
				i->second = atof(cq.SGetValueDef("1", "AI\\"+ i->first).c_str());
			}
			loaded = true;
			return true;
		} else{
			int unum = cb->GetNumUnitDefs();
			const UnitDef** list = new const UnitDef*[unum];
			cb->GetUnitDefList(list);
			for(int i = 0; i < unum; i++){
				efficiency[list[i]->name] = 1;
			}
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
		filename = "NTAI/learn/";
		filename += cb->GetModName();
		filename += ".tdf";
		off.open(filename.c_str());
		if(off.is_open() == true){
			off << "[AI]" << endl << "{" << endl << "    // NTAI AF :: unit efficiency cache file" << endl << endl;
			// put stuff in here;
			for(map<string,float>::const_iterator i = efficiency.begin(); i != efficiency.end(); ++i){
				off << "    "<< i->first << "=" << i->second << ";" << endl;
			}
			off << "}" << endl;
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

