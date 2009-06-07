#include "IncCREG.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

CR_BIND(CMetalMaker, (NULL))
CR_REG_METADATA(CMetalMaker, (
	CR_MEMBER(myUnits),
	CR_MEMBER(lastEnergy),
	CR_MEMBER(ai),
	CR_MEMBER(listIndex),
	CR_MEMBER(addedDelay),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(CMetalMaker::UnitInfo, )
CR_REG_METADATA_SUB(CMetalMaker, UnitInfo, (
	CR_MEMBER(id),
	CR_MEMBER(energyUse),
	CR_MEMBER(metalPerEnergy),
	CR_MEMBER(turnedOn),
	CR_RESERVED(8)
));



CMetalMaker::CMetalMaker(AIClasses* aic) {
	listIndex  = 0;
	lastEnergy = 0;
	addedDelay = 0;
	ai         = aic;
}

CMetalMaker::~CMetalMaker() {
//	for (map<int, UnitInfo*>::iterator ui = myUnits.begin(); ui != myUnits.end(); ++ui)
//		delete ui->second;
	myUnits.clear();
}

void CMetalMaker::PostLoad() {
}



bool CMetalMaker::Add(int unit) {
	const UnitDef* ud = ai->cb->GetUnitDef(unit);

	if (!(ud->energyUpkeep > 0.0f && ud->makesMetal > 0.0f)) {
		// can only use metal makers
		return false;
	}

	// analyse the command
	UnitInfo info;
	const std::vector<CommandDescription>* cd = ai->cb->GetUnitCommands(unit);

	for (std::vector<CommandDescription>::const_iterator cdi = cd->begin(); cdi != cd->end(); ++cdi) {
		if (cdi->id == CMD_ONOFF) {
			int on = atoi(cdi->params[0].c_str());
			if (on)
				info.turnedOn = true;
			else
				info.turnedOn = false;
			break;
		}
	}

	info.id = unit;
	info.energyUse = ud->energyUpkeep;
	info.metalPerEnergy = ud->makesMetal / ud->energyUpkeep;

	// myUnits[unit] = info;
	// myUnits.push_back(info);

	// insert sorted by metalPerEnergy
	bool inserted = false;
	int counter = 0;
	std::vector<UnitInfo>::iterator theIterator = myUnits.begin();

	while (theIterator != myUnits.end() && !inserted) {
		// insert it where it should be in order for the list to remain sorted (insertion sort)
		if (info.metalPerEnergy > theIterator->metalPerEnergy ||
			((info.metalPerEnergy == theIterator->metalPerEnergy &&
			(ai->cb->GetUnitPos(info.id).x == ai->cb->GetUnitPos(theIterator->id).x &&
			ai->cb->GetUnitPos(info.id).z > ai->cb->GetUnitPos(theIterator->id).z)) ||
			(info.metalPerEnergy == theIterator->metalPerEnergy &&
			ai->cb->GetUnitPos(info.id).x > ai->cb->GetUnitPos(theIterator->id).x))
		) {
			myUnits.insert(theIterator, info);
			inserted = true;
			break;
		}

		theIterator++;
		counter++;
	}

	if (!inserted) {
		myUnits.push_back(info);
	}
	if (counter < listIndex) {
		// make sure it's on and increase index
		if (!myUnits[counter].turnedOn) {
			Command c;
			c.id = CMD_ONOFF;
			c.params.push_back(1);
			ai->ct->GiveOrder(myUnits[counter].id, &c);
			myUnits[counter].turnedOn = true;
		}

		listIndex++;
	} else {
		// make sure it's off
		if (myUnits[counter].turnedOn) {
			Command c;
			c.id = CMD_ONOFF;
			c.params.push_back(0);
			ai->ct->GiveOrder(myUnits[counter].id, &c);
			myUnits[counter].turnedOn = false;
		}
	}

	return true;
}


bool CMetalMaker::Remove(int unit) {
	bool found = false;
	int counter = 0;
	std::vector<UnitInfo>::iterator it;

	for (it = myUnits.begin(); it != myUnits.end(); it++) {
		if (it->id == unit) {
			found = true;
			break;
		}
		counter++;
	}

	if (found)
		myUnits.erase(it);

	// is this index below listIndex?
	if (counter < listIndex && listIndex > 0) {
		listIndex--;
	}

	return found;
}


bool CMetalMaker::AllAreOn() {
	return (this->listIndex == ((int) this->myUnits.size() - 1) || myUnits.size() == 0);
}


void CMetalMaker::Update(int frameNum) {
	const int updateSpread = 33;
	int numUnits = (int) myUnits.size();

	if (frameNum % updateSpread == 0 && numUnits > 0 && addedDelay-- <= 0) {
		float energy     = ai->cb->GetEnergy();
		float estore     = ai->cb->GetEnergyStorage();
		float difference = (energy - lastEnergy) * 0.25f;

		lastEnergy = energy;

		// turn something off
		if (energy < estore * 0.6) {
			while (difference < 0 && listIndex > 0) {
				// listIndex points at the first offline one,
				// after decrement should be the last online one
				listIndex--;

				if (myUnits[listIndex].turnedOn) {
					Command c;
					c.id = CMD_ONOFF;
					c.params.push_back(0);
					ai->ct->GiveOrder(myUnits[listIndex].id, &c);

					myUnits[listIndex].turnedOn = false;
					difference += myUnits[listIndex].energyUse;
				}
			}
			addedDelay = 4;
		// turn something on
		} else if (energy > estore * 0.9 && listIndex < numUnits) {
			if (!myUnits[listIndex].turnedOn) {
				Command c;
				c.id = CMD_ONOFF;
				c.params.push_back(1);
				ai->ct->GiveOrder(myUnits[listIndex].id, &c);
				myUnits[listIndex].turnedOn = true;

				if (difference < myUnits[listIndex].energyUse)
					addedDelay = 4;
			}

			//set it to point to the next one, which should be offline
			listIndex++;
		}


/*
		// turn something off
		if (energy < estore * 0.8) {
			// how much energy we need to save to turn positive
			float needed =- dif + 4;

			for (int i = 0; i < (int) myUnits.size(); i++) {
				// find makers to turn off
				if (needed < 0)
					break;

				if (myUnits[i].turnedOn) {
					needed -= myUnits[i].energyUse;
					Command c;
					c.id = CMD_ONOFF;
					c.params.push_back(0);
					ai->ct->GiveOrder(myUnits[i].id, &c);
					myUnits[i].turnedOn = false;
				}
			}
		// turn something on
		} else if (energy > estore * 0.9) {
			// how much energy we need to start using to turn negative
			float needed = dif + 4;

			for (int i = 0; i < (int) myUnits.size(); i++) {
				// find makers to turn on
				if (needed < 0)
					break;

				if (!myUnits[i].turnedOn) {
					needed -= myUnits[i].energyUse;
					Command c;
					c.id = CMD_ONOFF;
					c.params.push_back(1);
					ai->ct->GiveOrder(myUnits[i].id, &c);
					myUnits[i].turnedOn = true;
				}
			}

			// ensure we don't waste any E (using too
			// much is slightly more acceptable)
			// lastUpdate = frameNum - updateSpread;
		}
*/
	}

	if ((frameNum % 1800) == 0) {
		// HACK: once a minute, turn everything off and reset
		for (int i = 0; i < (int) myUnits.size(); i++) {
			Command c;
			c.id = CMD_ONOFF;
			c.params.push_back(0);
			ai->ct->GiveOrder(myUnits[i].id, &c);
			myUnits[i].turnedOn = false;
		}

		this->listIndex = 0;
		this->addedDelay = 0;
	}
}
