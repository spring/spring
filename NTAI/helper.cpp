#include "helper.h"
#include "IGlobalAICallback.h"

using namespace std;
IAICallback* hcallback;
THelper* khelper;

THelper::~THelper(){}

void THelper::AddUnit(int unit){
	//print("THelper::AddUnit");
	TUnit* tu = new TUnit(unit);
	const UnitDef* ud =hcallback->GetUnitDef(unit);
	tu->ownerteam = hcallback->GetUnitAllyTeam(unit);
	tu->Taken = false;
	tu->ud = ud;
}

THelper::THelper(IAICallback* callback){
	khelper = this;
	::hcallback = callback;
}

void THelper::Init(){
	//print("THelper::Init");
	int uf[10000];
	hcallback->GetFriendlyUnits(uf);
	const UnitDef* ud = hcallback->GetUnitDef(uf[0]);
	if(ud->isCommander){
		commander = uf[0];
		if(ud->name.c_str() == "ARMCOM"){
			race = RACE_ARM;
		} else if(ud->name.c_str() == "CORCOM"){
			race = RACE_CORE;
		}
	}
	float3 StartPos = hcallback->GetUnitPos(uf[0]);
	if(!ud->isCommander) hcallback->SendTextMsg("commander not found",1);
}

void THelper::msg(D_Message msg){
	string message = msg.source;
	message += " :: ";
	message += msg.type;
	message += " :: ";
	message += msg.description;
	print(message);
}


void THelper::print(string message){
	hcallback->SendTextMsg(message.c_str(),1);
}

void THelper::post(string usource, string utype, string udescription){
	D_Message error(usource,utype);
	error.description = udescription;
	msg(error);
}

string THelper::GameTime(){
	char min[2];
	char sec[2];
	int Time = hcallback->GetCurrentFrame();
	Time = Time/30;
	int Seconds = Time%60;
	int Minutes = Time/60;
	sprintf(min,"%i",Minutes);
	sprintf(sec,"%i",Seconds);
	string stime;
	if(Minutes<10){
		stime += "0";
	}
	stime += min;
	stime += ":";
	if(Seconds <10){
		stime += "0";
	}
	stime += sec;
	return stime.c_str();
}
 	
bool THelper::Verbose(){
	if(verbose){
		verbose = false;
	}else{
		verbose = true;
	}
	return verbose;	
}

void THelper::AddMatrix(int gridsetnum, int matrix, int defacto){
	//print("THelper::AddMatrix");
	if(defacto!=1){
		matrixes[matrix]=defacto;
	} else{
		matrixes[matrix]=defacto+1;
	}
}

bool THelper::MatrixPresent(int gridsetnum, int matrix){
	//print("THelper::MatrixPresent");
	if(matrixes[matrix] == 0) return false;
	return true;
}

void THelper::MatrixPercentage(int gridsetnum, int matrix, int percentage){
	//print("THelper::MatrixPercentage");
}

/*TOffset THelper::getMetalPatch(TOffset pos, float radius, TUnit* tu){
	//print("THelper::getMetalPatch");
	vector<float3>* mexfloats = MHandler->getMetalPatch(pos,0,radius,tu->ud->extractsMetal);
	vector<Offset*> mexoffs;
	int mc = 0;
	for(vector<float3>::iterator fi =mexfloats->begin(); fi !=mexfloats->end();++fi){
		mexoffs[mc] = this->TransCord(0,*fi);
		mexoffs[mc]->x *= 8;
		mexoffs[mc]->y *= 8;
		mexoffs[mc]->z *= 8;
		mc++;
	}
	return TOffset(mexoffs);
}
float3 THelper::getNearestPatch(float3 pos, const UnitDef* ud){
	//print("THelper::getNearestPatch");
	float3 npos = MHandler->getNearestPatch(pos,0,ud->extractsMetal);
	npos.x *= 8;
	npos.y *= 8;
	npos.z *= 8;
	return npos;
}
void THelper::markpatch(TUnit* tu){
	//print("THelper::markpatch");
	MHandler->addExtractor(tu->id);
}
void THelper::unmarkpatch(TUnit* tu){
	//print("THelper::unmarkpatch");
	MHandler->removeExtractor(tu->id);
}*/
Gridset::Gridset(int x,int y,int gridset){
	this->gdata->SectorHeight =x;
	gdata->SectorWidth = y;
	gdata->gridnumber = gridset;
	gdata->x = hcallback->GetMapWidth()/x;
	gdata->y = hcallback->GetMapWidth()/y;
	for (int i=0; i!=gdata->x; ++i){
		this->Grid[i] = new TOffset;
		for(int iy = 0; iy!=gdata->y; ++iy){
			Grid[i]->offsets.push_back(new Offset(i,iy));
		}
	}
}
Offset* Gridset::TransCord(float3 position){
	Offset* cord = new Offset(int(position.x/gdata->SectorWidth),int(position.y/gdata->SectorHeight));
	Offset* offit;
	for(vector<Offset*>::iterator ogi = Grid[int(cord->x)]->offsets.begin(); ogi != Grid[int(cord->x)]->offsets.end();++ogi){
		offit = *ogi;
		if(offit->y == cord->y){
			delete cord;
			return offit;
		}
	}
	delete cord;
	return 0;
}

float3 Gridset::TransOff(Offset* o){
	float3 position;
	position.x = o->x*gdata->SectorWidth;
	position.y = o->y*gdata->SectorHeight;
	return position;
}

void THelper::AddGridset(int gridsetnum, int sectwidth, int sectheight){
	Grids[gridsetnum]=new Gridset(sectwidth,sectheight,gridsetnum);
}

//// Offset class

Offset::Offset(float3 position){
	this->x = position.x;
	this->y = position.y;
	this->z = position.z;;
}

Offset::Offset(int x, int y, int z){
	this->x = x;
	this->y = y;
	this->z = z;
}

Offset::Offset(int x, int y){
	this->x = x;
	this->y = y;
}


void TUnit::attack(TOffset o){
	
}

void TUnit::move(TOffset o){
	Command* c = new Command;
	c->id = CMD_MOVE;
	c->params.push_back(o.x);
	hcallback->GiveOrder(id,c);
}

void TUnit::guard(TUnit* u){
	Command* c = new Command;
	c->id = CMD_GUARD;
	c->params.push_back(u->id);
	hcallback->GiveOrder(id,c);
}
void TUnit::destruct(){
	
}
void TUnit::patrol(TOffset offsets){
	
}
void TUnit::execute(Command* c){
	hcallback->GiveOrder(id,c);
}

void TUnit::execute(vector<Command*> cv){
	Command* c;
	for(vector<Command*>::iterator ci = cv.begin();ci!=cv.end();++ci){
		c=*ci;
		hcallback->GiveOrder(id,c);
	}
}
TOffset::TOffset(float3 position){
	Offset* o = khelper->TransCord(0,position);
	offsets.push_back(o);
}
int TOffset::get(int value){
	int freq =0;
	int width=0;
	Offset* oi;
	for(vector<Offset*>::iterator i = offsets.begin();i!=offsets.end();++i){
		oi = *i;
		freq += oi->get(value);
		width++;
	}
	return(freq/(width+1));
}
void TOffset::set(int matrix, int value){
	Offset* oi;
	for(vector<Offset*>::iterator i = offsets.begin();i!=offsets.end();++i){
		oi = *i;
		oi->set(matrix,value);
	}
}
void TOffset::raise(int matrix, int value){
	Offset* oi;
	for(vector<Offset*>::iterator i = offsets.begin();i!=offsets.end();++i){
		oi = *i;
		oi->raise(matrix,value);
	}
}

void TOffset::lower(int matrix, int value){
	Offset* oi;
	for(vector<Offset*>::iterator i = offsets.begin();i!=offsets.end();++i){
		oi = *i;
		oi->lower(matrix,value);
	}
}

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