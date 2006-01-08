#include "StdAfx.h"
#include "AirBaseHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/COB/CobInstance.h"
#include "mmgr.h"

CAirBaseHandler* airBaseHandler=0;

CAirBaseHandler::CAirBaseHandler(void)
{
}

CAirBaseHandler::~CAirBaseHandler(void)
{
	//shouldnt be any bases left here...
	for(int a=0;a<gs->activeAllyTeams;++a){
		for(std::list<AirBase*>::iterator bi=bases[a].begin();bi!=bases[a].end();++bi){
			for(std::list<LandingPad*>::iterator pi=(*bi)->pads.begin();pi!=(*bi)->pads.end();++pi){
				delete *pi;
			}
			delete *bi;
		}
	}
}

void CAirBaseHandler::RegisterAirBase(CUnit* base)
{
	AirBase* ab=new AirBase;
	ab->unit=base;

	std::vector<long> args;
	args.push_back(0);
	args.push_back(0);

	base->cob->Call("QueryLandingPad",args);

	LandingPad* pad=new LandingPad;
	
	pad->unit=base;
	pad->piece=args[0];
	pad->base=ab;

	ab->pads.push_back(pad);
	ab->freePads.push_back(pad);

	if(args[0]!=args[1]){	//if we get two different pieces we assume the unit has two pads, not sure what is the correct way
		LandingPad* pad=new LandingPad;
		
		pad->unit=base;
		pad->piece=args[1];
		pad->base=ab;

		ab->pads.push_back(pad);
		ab->freePads.push_back(pad);
	}

	freeBases[base->allyteam].push_back(ab);
	bases[base->allyteam].push_back(ab);
}

void CAirBaseHandler::DeregisterAirBase(CUnit* base)
{
	for(std::list<AirBase*>::iterator bi=freeBases[base->allyteam].begin();bi!=freeBases[base->allyteam].end();++bi){
		if((*bi)->unit==base){
			freeBases[base->allyteam].erase(bi);
			break;
		}
	}
	for(std::list<AirBase*>::iterator bi=bases[base->allyteam].begin();bi!=bases[base->allyteam].end();++bi){
		if((*bi)->unit==base){
			for(std::list<LandingPad*>::iterator pi=(*bi)->pads.begin();pi!=(*bi)->pads.end();++pi){
				delete *pi;		//its the unit that has reserved a pads responsibility to see if the pad is gone so just delete it
			}
			delete *bi;
			bases[base->allyteam].erase(bi);
			break;
		}
	}
}

//Try to find an airbase and reserve it if one can be found
//caller must call LeaveLandingPad if it gets one and is finished with it or dies
//its the callers responsibility to detect if the base dies while its reserved
CAirBaseHandler::LandingPad* CAirBaseHandler::FindAirBase(CUnit* unit, float maxDist)
{
	for(std::list<AirBase*>::iterator bi=freeBases[unit->allyteam].begin();bi!=freeBases[unit->allyteam].end();++bi){
		if((*bi)->unit->pos.distance(unit->pos)>maxDist)
			continue;
		for(std::list<LandingPad*>::iterator pi=(*bi)->freePads.begin();pi!=(*bi)->freePads.end();++pi){
			LandingPad* lp=*pi;
			(*bi)->freePads.erase(pi);
			return lp;
		}
	}
	return 0;
}

void CAirBaseHandler::LeaveLandingPad(LandingPad* pad)
{
	pad->base->freePads.push_back(pad);
}
