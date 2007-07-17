// implementation of the logic in a metal maker class
//////////////////////////////////////////////////////////////////////

#include "Include.h"
//#include "StdAfx.h"
#include "MetalMaker.h"
//#include "ExternalAI/IGroupAiCallback.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include <vector>

CR_BIND(CMetalMaker ,(NULL))

CR_REG_METADATA(CMetalMaker,(
				CR_MEMBER(myUnits),
				CR_MEMBER(lastEnergy),
				CR_MEMBER(ai),
				CR_MEMBER(listIndex),
				CR_MEMBER(addedDelay),
				CR_RESERVED(64)
				));

CR_BIND(CMetalMaker::UnitInfo, )

CR_REG_METADATA_SUB(CMetalMaker,UnitInfo,(
					CR_MEMBER(id),
					CR_MEMBER(energyUse),
					CR_MEMBER(metalPerEnergy),
					CR_MEMBER(turnedOn),
					CR_RESERVED(16)
					));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//namespace std{
//	void _xlen(){};
//}

CMetalMaker::CMetalMaker(AIClasses* ai)
{
//	lastUpdate=0;
	listIndex=0;
	lastEnergy=0;
	addedDelay=0;
	this->ai=ai;
}

CMetalMaker::~CMetalMaker()
{
//	for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
//		delete ui->second;
	myUnits.clear();
}

/*void CMetalMaker::Init()
{
//	this->callback=callback;
	this->aicb=aicb;
}*/

bool CMetalMaker::Add(int unit)
{
	const UnitDef* ud=ai->cb->GetUnitDef(unit);
	if(!(ud->energyUpkeep>0.0f && ud->makesMetal>0.0f)){ // || !ud->extractsMetal>0.0f) {
//		ai->cb->SendTextMsg("Can only use metal makers",0);
		return false;
	}
	//analyse the command and stuff on or off into the info thingy
	UnitInfo info;//=new UnitInfo;
	const std::vector<CommandDescription>* cd=ai->cb->GetUnitCommands(unit);
	for(std::vector<CommandDescription>::const_iterator cdi=cd->begin();cdi!=cd->end();++cdi){
		if(cdi->id==CMD_ONOFF){
			int on=atoi(cdi->params[0].c_str());
			if(on)
				info.turnedOn=true;
			else
				info.turnedOn=false;
			break;
		}
	}
	info.id = unit;
	info.energyUse=ud->energyUpkeep;
	info.metalPerEnergy = ud->makesMetal/ud->energyUpkeep;
	//myUnits[unit]=info;
	
	//myUnits.push_back(info);

	//insert sorted by metalPerEnergy
	bool inserted = false;
	int counter = 0;
	vector<UnitInfo>::iterator theIterator = myUnits.begin();
	while(theIterator != myUnits.end() && !inserted) {
		//insert it where it should be in order for the list to remain sorted (insertion sort)
		if (info.metalPerEnergy > theIterator->metalPerEnergy 
			//in addition, provide a sexy ordering ability ;P
				|| (info.metalPerEnergy == theIterator->metalPerEnergy
					&& (ai->cb->GetUnitPos(info.id).x == ai->cb->GetUnitPos(theIterator->id).x
					&& ai->cb->GetUnitPos(info.id).z > ai->cb->GetUnitPos(theIterator->id).z)
				|| (info.metalPerEnergy == theIterator->metalPerEnergy
					&& ai->cb->GetUnitPos(info.id).x > ai->cb->GetUnitPos(theIterator->id).x))
			) {
			myUnits.insert(theIterator, info);
			inserted = true;
			break;
		}
		theIterator++;
		counter++;
	}
	if(!inserted) {
		myUnits.push_back(info);
	}
	if (counter < listIndex) {
		//make sure its on and increase index
		if(!myUnits[counter].turnedOn)
		{
			Command c;
			c.id=CMD_ONOFF;
			c.params.push_back(1);
			ai->cb->GiveOrder(myUnits[counter].id,&c);
			myUnits[counter].turnedOn=true;
		}
		listIndex++;

	} else {
		//make sure its off
		if(myUnits[counter].turnedOn)
		{
			Command c;
			c.id=CMD_ONOFF;
			c.params.push_back(0);
			ai->cb->GiveOrder(myUnits[counter].id,&c);
			myUnits[counter].turnedOn=false;
		}
	}
//	char text[500];
//	sprintf(text, "CFirenuMM: metal maker added, id=%i, index=%i, num=%i, energyUpkeep=%f, makesMetal=%f", unit, counter, (int)myUnits.size(), ud->energyUpkeep, ud->makesMetal);
//	ai->cb->SendTextMsg(text, 0);
//	for(int i = 0; i < (int)myUnits.size(); i++) {
//		sprintf(text, "-%f", myUnits[i].metalPerEnergy);
//		ai->cb->SendTextMsg(t, 0);
//	}
	//ai->cb->SendTextMsg(t, 0);
	
	

	return true;
}

bool CMetalMaker::Remove(int unit)
{
	bool found = false;
	vector<UnitInfo>::iterator it;
	int counter = 0;
	for (it = myUnits.begin(); it != myUnits.end(); it++) {
		if (it->id == unit) {
			found = true;
			break;
		}
		counter++;
	}
	if (found) myUnits.erase(it);
	//is this index below listIndex
	if (counter < listIndex && listIndex > 0) {
		listIndex--;
	}
	return found;
}

bool CMetalMaker::AllAreOn() {
	return this->listIndex == ((int)this->myUnits.size()-1) || myUnits.size() == 0;
}

void CMetalMaker::Update()
{
	int frameNum=ai->cb->GetCurrentFrame();
	const int updateSpread = 33;
	int numUnits = (int)myUnits.size();
	if(frameNum % updateSpread == 0 && numUnits > 0 && addedDelay-- <= 0){
		//lastUpdate=frameNum;
		float energy=ai->cb->GetEnergy();
		float estore=ai->cb->GetEnergyStorage();

		float difference=energy-lastEnergy;
		difference /= 4.0f;
		lastEnergy=energy;

		//turn something off
		if(energy<estore*0.6) {// && listIndex > 0){
			while (difference < 0 && listIndex > 0) {
				//listIndex points at the first offline one
				listIndex--; //now it should be the last online one
//				char t[500];
//				sprintf(t, "CFirenuMM: %i-->off %f", listIndex, energy/estore);
//				ai->cb->SendTextMsg(t, 0);
				if(myUnits[listIndex].turnedOn)
				{
					Command c;
					c.id=CMD_ONOFF;
					c.params.push_back(0);
					ai->cb->GiveOrder(myUnits[listIndex].id,&c);
					myUnits[listIndex].turnedOn=false;
					//break; //moo
					difference += myUnits[listIndex].energyUse;
				}
			}
			addedDelay = 4;
		//turn something on
		} else if(energy>estore*0.9 && listIndex < numUnits){
//			char t[500];
//			sprintf(t, "CFirenuMM: %i--->on %f", listIndex, energy/estore);
//			ai->cb->SendTextMsg(t, 0);
			if(!myUnits[listIndex].turnedOn)
			{
				Command c;
				c.id=CMD_ONOFF;
				c.params.push_back(1);
				ai->cb->GiveOrder(myUnits[listIndex].id,&c);
				myUnits[listIndex].turnedOn=true;
				//break; //moo
				if (difference < myUnits[listIndex].energyUse) addedDelay = 4;
			}
			listIndex++;//set it to point to the next one, which should be offline
		}
		

/*
		float dif=energy-lastEnergy;
		lastEnergy=energy;
		//turn something off
		if(energy<estore*0.8){
			float needed=-dif+4;		//how much energy we need to save to turn positive
			for(int i = 0; i < (int)myUnits.size(); i++) {		//find makers to turn off
				if(needed<0)
					break;
				if(myUnits[i].turnedOn){
					needed-=myUnits[i].energyUse;
					Command c;
					c.id=CMD_ONOFF;
					c.params.push_back(0);
					ai->cb->GiveOrder(myUnits[i].id,&c);
					myUnits[i].turnedOn=false;
					//break; //moo
				}
			}
		//turn something on
		} else if(energy>estore*0.9){
			float needed=dif+4;		//how much energy we need to start using to turn negative
			for(int i = 0; i < (int)myUnits.size(); i++) {		//find makers to turn on
				if(needed<0)
					break;
				if(!myUnits[i].turnedOn){
					needed-=myUnits[i].energyUse;
					Command c;
					c.id=CMD_ONOFF;
					c.params.push_back(1);
					ai->cb->GiveOrder(myUnits[i].id,&c);
					myUnits[i].turnedOn=true;
					//break; //moo
				}
			}
			//lastUpdate=frameNum-updateSpread; //want to be sure i dont waste any E, while using too much E is slightly more acceptable
		}
		*/
	}
	if ((ai->cb->GetCurrentFrame() % 1800) == 0) {
		//TODO: super evil hack. once a minute, turn everything off and reset.
		for (int i = 0; i < (int)myUnits.size(); i++) {
			Command c;
			c.id=CMD_ONOFF;
			c.params.push_back(0);
			ai->cb->GiveOrder(myUnits[i].id,&c);
			myUnits[i].turnedOn = false;
		}
		this->listIndex = 0;
		this->addedDelay = 0;
//		ai->cb->SendTextMsg("MM: turned all off.", 0);
	}
}
