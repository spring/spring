#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::InitAI(Global* GLI){
	G = GLI;
	G->L->print("Scouter Init");
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitFinished(int unit){
	if(G->cb->GetUnitDef(unit) == 0) return;
	bool naset = false;
	vector<string> na= bds::make_cont(na,"CORFAST, CORFINK, CORSFIG, CORFAV, CORPT, CORSH, TLLBUG, ARMFAST, TLLGLADIUS, ARMSPY");
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	for(vector<string>::iterator nai = na.begin(); nai != na.end();++nai){
		if(*nai == ud->name){
			naset = true;
			break;
		}
	}
	if(naset){
		vector<float3>::iterator ic = G->M->getHotSpots()->begin();
		cp[unit] = ic;
		G->L->print("scouter found");
	 }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitMoveFailed(int unit){
	if(G->cb->GetUnitDef(unit) == 0) return;
    bool naset = false;
	vector<string> na= bds::make_cont(na,"CORFAST, CORFINK, CORSFIG, CORFAV, CORPT, CORSH, TLLBUG, ARMFAST, TLLGLADIUS, ARMSPY");
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	for(vector<string>::iterator nai = na.begin(); nai != na.end();++nai){
		if(*nai == ud->name){
			naset = true;
			break;
		}
	}
	if(naset){
		Command* c;
		c = new Command;
		vector<float3>::iterator ic = cp[unit];
		if(ic == G->M->getHotSpots()->end()){
			ic = G->M->getHotSpots()->begin();
		} else {
			ic++;
		}
		cp[unit] = ic;
		c->params.push_back(cp[unit]->x);
		c->params.push_back(G->cb->GetElevation(cp[unit]->x,cp[unit]->z));
		c->params.push_back(cp[unit]->z);
		c->id=CMD_MOVE;
		G->L->print("scouter given new position");
		G->cb->GiveOrder(unit,c);
	 }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::UnitIdle(int unit){
	if(G->cb->GetUnitDef(unit) == 0) return;
	bool naset = false;
	const UnitDef* ud = G->cb->GetUnitDef(unit);
	vector<string> na= bds::make_cont(na,"CORFAST, CORFINK, CORSFIG, CORFAV, CORPT, CORSH, TLLBUG, ARMFAST, TLLGLADIUS, ARMSPY");
	for(vector<string>::iterator nai = na.begin(); nai != na.end();++nai){
		if(*nai == ud->name){
			naset = true;
			break;
		}
	}
	if(naset){
		Command* c;
		c = new Command;
		vector<float3>::iterator ic = cp[unit];
		if(ic == G->M->getHotSpots()->end()){
			ic = G->M->getHotSpots()->begin();
		} else {
			ic++;
		}
		cp[unit] = ic;
		c->params.push_back(cp[unit]->x);
		c->params.push_back(G->cb->GetElevation(cp[unit]->x,cp[unit]->z));
		c->params.push_back(cp[unit]->z);
		c->id=CMD_MOVE;
		G->cb->GiveOrder(unit,c);
	 }
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Scouter::Update(){}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void Scouter::UnitDestroyed(int unit){
	if(cp.empty() == false){
		for(map<int,vector<float3>::iterator>::iterator q = cp.begin(); q != cp.end();++q){
			if(q->first == unit){
				cp.erase(q);
				break;
			}
		}
	}
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

