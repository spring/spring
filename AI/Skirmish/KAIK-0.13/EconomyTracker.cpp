#include "EconomyTracker.h"



CR_BIND(BuildingTracker, )
CR_REG_METADATA(BuildingTracker, (
	CR_MEMBER(unitUnderConstruction),
	CR_MEMBER(category),
	CR_MEMBER(hpLastFrame),
	CR_MEMBER(damage),
	CR_MEMBER(hpSomeTimeAgo),
	CR_MEMBER(damageSomeTimeAgo),
	CR_MEMBER(startedRealBuildingFrame),
	CR_MEMBER(etaFrame),
	CR_MEMBER(maxTotalBuildPower),
	CR_MEMBER(assignedTotalBuildPower),
	CR_MEMBER(energyUsage),
	CR_MEMBER(metalUsage),

	CR_MEMBER(buildTask),
	CR_MEMBER(factory),
	CR_MEMBER(economyUnitTracker),
	CR_RESERVED(16)
));

CR_BIND(EconomyUnitTracker, )
CR_REG_METADATA(EconomyUnitTracker, (
	CR_MEMBER(economyUnitId),
	CR_MEMBER(createFrame),
	CR_MEMBER(buildingTracker),
	CR_MEMBER(alive),
	CR_MEMBER(dieFrame),
	CR_MEMBER(category),
	CR_MEMBER(totalEnergyMake),
	CR_MEMBER(totalMetalMake),
	CR_MEMBER(totalEnergyUsage),
	CR_MEMBER(totalMetalUsage),
	CR_MEMBER(lastUpdateEnergyMake),
	CR_MEMBER(lastUpdateMetalMake),
	CR_MEMBER(lastUpdateEnergyUsage),
	CR_MEMBER(lastUpdateMetalUsage),
	CR_MEMBER(dynamicChangingUsage),
	CR_MEMBER(nonEconomicUnit),
	CR_MEMBER(estimateEnergyChangeFromDefWhileOn),
	CR_MEMBER(estimateMetalChangeFromDefWhileOn),
	CR_MEMBER(estimateEnergyChangeFromDefWhileOff),
	CR_MEMBER(estimateMetalChangeFromDefWhileOff),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(CEconomyTracker, (NULL));
CR_REG_METADATA(CEconomyTracker, (
	CR_MEMBER(allTheBuildingTrackers),
	CR_MEMBER(deadEconomyUnitTrackers),
	CR_MEMBER(newEconomyUnitTrackers),
	CR_MEMBER(activeEconomyUnitTrackers),
	CR_MEMBER(underConstructionEconomyUnitTrackers),

	CR_MEMBER(ai),

	CR_MEMBER(trackerOff),

	CR_MEMBER(constructionEnergy),
	CR_MEMBER(constructionMetal),

	CR_MEMBER(constructionEnergySum),
	CR_MEMBER(constructionMetalSum),
	CR_RESERVED(16)
));



CEconomyTracker::CEconomyTracker(AIClasses* ai) {
	this->ai = ai;
	allTheBuildingTrackers.resize(LASTCATEGORY);

	if (ai) {
		oldEnergy = ai->cb->GetEnergy();
		oldMetal = ai->cb->GetMetal();
	}

	constructionEnergySum = 0;
	constructionMetalSum = 0;
	constructionEnergy = 0;
	constructionMetal = 0;

	for (int i = 0; i < LASTCATEGORY; i++) {
		allTheBuildingTrackers[i].clear();
	}

	trackerOff = true;
}

CEconomyTracker::~CEconomyTracker() {
	for (list<EconomyUnitTracker*>::iterator i = deadEconomyUnitTrackers.begin(); i != deadEconomyUnitTrackers.end(); i++) {
		delete *i;
	}
	for (list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++) {
		delete *i;
	}
	for (list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++) {
		delete *i;
	}
	for (list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++) {
		delete *i;
	}
}



void CEconomyTracker::frameUpdate(int frame) {
	if (trackerOff) {
		return;
	}

/*
	// iterate over all the BuildTasks
	for (int category = 0; category < LASTCATEGORY; category++) {
		for (list<BuildTask>::iterator i = ai->uh->BuildTasks[category]->begin(); i != ai->uh->BuildTasks[category]->end(); i++) {
			BuildTask bt = *i;
			updateUnitUnderConstruction(&bt);
		}
	}
*/

	for (int category = 0; category < LASTCATEGORY; category++) {
		for (list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++) {
			BuildingTracker* bt = &(*i);
			updateUnitUnderConstruction(bt);
		}
	}

	constructionEnergySum += constructionEnergy;
	constructionMetalSum += constructionMetal;



	// move the new EconomyUnitTrackers
	list<EconomyUnitTracker*> removeList;

	for (list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++) {
		EconomyUnitTracker* bt = *i;
		assert(frame - bt->createFrame <= 16);

		if (frame - bt->createFrame == 16) {
			// move it to the active list
			assert(bt->alive);
			activeEconomyUnitTrackers.push_back(bt);
			removeList.push_back(bt);
		}
	}

	// remove them from newEconomyUnitTrackers
	for (list<EconomyUnitTracker*>::iterator i = removeList.begin(); i != removeList.end(); i++) {
		newEconomyUnitTrackers.remove(*i);
	}

	// update the units in activeEconomyUnitTrackers, add their production/usage to total
	float energyProduction = 0.0f;
	float metalProduction = 0.0f;
 	// these are exclusive of what is used by builders
	float energyUsage = 0.0f;
	float metalUsage = 0.0f;

	if (frame % 16 == 0) {
		for (list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker* bt = *i;
			assert(bt->alive);

			// if the unit is a builder then we track its resource usage already
			// if it's using more than its weapon fire or upkeep
			UnitResourceInfo resourceInfo;
			bool isAlive = ai->cb->GetUnitResourceInfo(bt->economyUnitId, &resourceInfo);
			assert(isAlive);

			// add the change from last update
			bt->totalEnergyMake += bt->lastUpdateEnergyMake = resourceInfo.energyMake - bt->lastUpdateEnergyMake;
			bt->totalMetalMake += bt->lastUpdateMetalMake = resourceInfo.metalMake - bt->lastUpdateMetalMake;
			bt->totalEnergyUsage += bt->lastUpdateEnergyUsage = resourceInfo.energyUse - bt->lastUpdateEnergyUsage;
			bt->totalMetalUsage += bt->lastUpdateMetalUsage = resourceInfo.metalUse - bt->lastUpdateMetalUsage;
			energyProduction += bt->lastUpdateEnergyMake;
			metalProduction += bt->lastUpdateMetalMake;

			if (!bt->unitDef->builder) {
				energyUsage += bt->lastUpdateEnergyUsage;
				metalUsage += bt->lastUpdateMetalUsage;
			}
		}
	}

	float energy = ai->cb->GetEnergy();
	float metal = ai->cb->GetMetal();
	// float deltaEnergy = energy - oldEnergy + constructionEnergy;
	// float deltaMetal = metal - oldMetal + constructionMetal;

	if (frame % 16 == 0) {
		makePrediction(frame + 320 + 160);
	}

	oldEnergy = energy;
	oldMetal = metal;
	constructionEnergy = 0;
	constructionMetal = 0;
}


TotalEconomyState CEconomyTracker::makePrediction(int targetFrame) {
	int frame = ai->cb->GetCurrentFrame();

	// find current state and make copy
	TotalEconomyState state;
	state.frame = frame;
	state.madeInFrame = frame;
	state.energyMake = ai->cb->GetEnergyIncome();
	state.energyStored = ai->cb->GetEnergy();
	state.energyUsage = ai->cb->GetEnergyUsage();
	state.energyStorageSize = ai->cb->GetEnergyStorage();

	state.metalMake = ai->cb->GetMetalIncome();
	state.metalStored = ai->cb->GetMetal();
	state.metalUsage = ai->cb->GetMetalUsage();
	state.metalStorageSize = ai->cb->GetMetalStorage();

	TotalEconomyState state2 = state;

	// HACK: just iterate over all the non-dead EconomyUnitTrackers and
	// add the usage from the BuildingTrackers too, until the stuff they
	// make hits their ETA

	for (int preFrame = frame; preFrame <= targetFrame; preFrame += 16) {
		// do the BuildingTrackers
		float constructionEnergy = 0;
		float constructionMetal = 0;

		for (int category = 0; category < LASTCATEGORY; category++ ) {
			for (list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++) {
				BuildingTracker* bt = &*i;

				// using the "semi-useless" GetCurrentFrame() stats only
				if (bt->etaFrame >= preFrame) {
					// Its not done yet
					constructionEnergy += bt->energyUsage;
					constructionMetal += bt->metalUsage;
				}
			}
		}

		float unitEnergy = 0;
		float unitMetal = 0;

		// do the EconomyUnitTrackers in activeEconomyUnitTrackers, it needs no changes (metalmakers == bad)
		for (list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker* eut = *i;

			// we guess it's on ATM
			unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
			unitMetal += eut->estimateMetalChangeFromDefWhileOn;
		}

		// do the EconomyUnitTrackers in newEconomyUnitTrackers, it needs no changes (metalmakers == bad)
		for (list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker* eut = *i;

			// we guess it's on ATM
			unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
			unitMetal += eut->estimateMetalChangeFromDefWhileOn; 
		}

		// do the EconomyUnitTrackers in newEconomyUnitTrackers, it needs to test the ETA first (metalmakers == bad,  nanostall == bad)
		for (list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker* eut = *i;

			if (eut->createFrame +16 < preFrame) {
				// we guess it's on ATM
				unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
				unitMetal += eut->estimateMetalChangeFromDefWhileOn; 
			}
		}

		// make the new bank balance
		state.energyMake = unitEnergy;
		state.energyStored += unitEnergy - constructionEnergy;
		state.energyUsage = constructionEnergy; // HACK
		state.metalMake = unitMetal;
		state.metalStored += unitMetal - constructionMetal;
		state.metalUsage = constructionMetal; // HACK
		bool staling = false;

		if (state.energyStored <= 0) {
			staling = true;
			state.energyStored = 0;
		}
		if (state.metalStored <= 0) {
			staling = true;
			state.metalStored = 0;
		}
		if (state.energyStored > ai->cb->GetEnergyStorage()) {
			state.energyStored = ai->cb->GetEnergyStorage();
		}
		if (state.metalStored > ai->cb->GetMetalStorage()) {
			state.metalStored = ai->cb->GetMetalStorage();
		}
		if (staling) {
			// must turn stuff off (metalmakers), and cut back on construction (and fix the global ETA)
			// int timeToStall = (preFrame - frame) / 30;
		}
		state.frame = preFrame;
	}


	{
		// try #2: add all the activeEconomyUnitTrackers in one go at the start
		float unitEnergy = 0;
		float unitMetal = 0;

		// do the EconomyUnitTrackers in activeEconomyUnitTrackers, it needs no changes (metalmakers == bad)
		for (list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker* eut = *i;

			// we guess it's on ATM
			unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
			unitMetal += eut->estimateMetalChangeFromDefWhileOn;
		}

		state2.energyMake = unitEnergy;
		state2.metalMake = unitMetal;


		// make a copy of the EconomyUnitTracker lists
		list<EconomyUnitTracker*> allFutureEconomyUnitTrackers;
		for (list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker* eut = *i;
			allFutureEconomyUnitTrackers.push_back(eut);
		}

		for (list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker* eut = *i;
			allFutureEconomyUnitTrackers.push_back(eut);
		}

		// run the new prediction loop
		for (int preFrame = frame; preFrame <= targetFrame; preFrame += 16) {
			// do the EconomyUnitTrackers in newEconomyUnitTrackers, it needs to test the ETA first (metalmakers == bad,  nanostall == bad)
			unitEnergy = 0;
			unitMetal = 0;

			for (list<EconomyUnitTracker*>::iterator i = allFutureEconomyUnitTrackers.begin(); i != allFutureEconomyUnitTrackers.end(); i++) {
				EconomyUnitTracker* eut = *i;

				if (eut->createFrame +16 < preFrame && eut->createFrame + 32 >= preFrame) {
					// we guess it's on ATM
					unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
					unitMetal += eut->estimateMetalChangeFromDefWhileOn;
					// update the storage change
					state2.energyStorageSize += eut->unitDef->energyStorage;
					state2.metalStorageSize += eut->unitDef->metalStorage;
				}
			}
			// add in the new production
			state2.energyMake += unitEnergy;
			state2.metalMake += unitMetal;

			// do the BuildingTrackers
			float constructionEnergy = 0;
			float constructionMetal = 0;

			for (int category = 0; category < LASTCATEGORY; category++) {
				for (list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++) {
					BuildingTracker* bt = &*i;

					// HACK: using the "semi-useless" GetCurrentFrame() stats only
					if (bt->etaFrame >= preFrame) {
						// it's not done yet
						constructionEnergy += bt->energyUsage;
						constructionMetal += bt->metalUsage;
					}
				}
			}

			// make the new bank balance:
			state2.energyStored += state2.energyMake - constructionEnergy;
			state2.energyUsage = constructionEnergy;
			state2.metalStored += state2.metalMake - constructionMetal;
			state2.metalUsage = constructionMetal;
			bool staling = false;

			if (state2.energyStored <= 0) {
				staling = true;
				state2.energyStored = 0;
			}
			if (state2.metalStored <= 0) {
				staling = true;
				state2.metalStored = 0;
			}
			if (state2.energyStored > state2.energyStorageSize) {
				state2.energyStored = state2.energyStorageSize;
			}
			if (state2.metalStored > state2.metalStorageSize) {
				state2.metalStored = state2.metalStorageSize;
			}
		}
	}

/*
	sprintf(c,"1 %is: e: %i, %3.1f, m: %i, %3.1f",
		time, (int)state.energyStored, (state.energyMake - state.energyUsage) * 2, (int) state.metalStored, (state.metalMake - state.metalUsage) * 2);
	sprintf(c,"2 %is: e: %i, %3.1f, m: %i, %3.1f",
		time, (int)state2.energyStored, (state2.energyMake - state2.energyUsage) * 2, (int) state2.metalStored, (state2.metalMake - state2.metalUsage) * 2);
*/

	return state;
}


void CEconomyTracker::updateUnitUnderConstruction(BuildingTracker* bt) {
	// find out how much resources have been used on this unit
	// just using its HP wont work (it might be damaged), so must
	// sum up what the builders spend
	// find the builders that work on this unit, and sum up their spending

	int unitUnderConstruction = bt->unitUnderConstruction;
	const UnitDef* unitDef = ai->cb->GetUnitDef(unitUnderConstruction);
	assert(unitDef != NULL);
	int frame = ai->cb->GetCurrentFrame();
	bt->economyUnitTracker->buildingTracker = bt;
	// make the builder list
	list<int>* builderList = 0;

	if (bt->buildTask) {
		bool found = false;
		for (list<BuildTask>::iterator i = ai->uh->BuildTasks[bt->category].begin(); i != ai->uh->BuildTasks[bt->category].end(); i++) {
			if (i->id == unitUnderConstruction) {
				builderList = &i->builders;
				found = true;
				break;
			}
		}

		assert(found);
	} else {
		bool found = false;
		for (list<Factory>::iterator i = ai->uh->Factories.begin(); i != ai->uh->Factories.end(); i++) {
			if (i->id == bt->factory) {
				builderList = &i->supportbuilders;
				found = true;
				break;
			}
		}

		if (!found) {
			return;
		}
	}

	assert(builderList != NULL);

	float metalUsage = 0;
	float energyUsage = 0;
	float maxBuildPower = 0;
	float maxAssignedBuildPower = 0;

	for (list<int>::iterator i = builderList->begin(); i != builderList->end(); i++) {
		int builder = *i;
		UnitResourceInfo resourceInfo;
		bool isAlive = ai->cb->GetUnitResourceInfo(builder, &resourceInfo);
		// TODO: uncomment this
		// assert(isAlive);
		if (!isAlive)
			continue;

		metalUsage += resourceInfo.metalUse;
		energyUsage += resourceInfo.energyUse;
		maxAssignedBuildPower += ai->cb->GetUnitDef(builder)->buildSpeed;
		const UnitDef * unitDefBuilder = ai->cb->GetUnitDef(builder);

		if (resourceInfo.metalUse > unitDefBuilder->metalUpkeep) {
			// is this needed?
			maxBuildPower += ai->cb->GetUnitDef(builder)->buildSpeed;
		}
	}

	if (!bt->buildTask) {
		assert(ai->cb->GetUnitDef(bt->factory) != NULL); // This will fail
		maxAssignedBuildPower += ai->cb->GetUnitDef(bt->factory)->buildSpeed;
		maxBuildPower += ai->cb->GetUnitDef(bt->factory)->buildSpeed;
	}

	bt->assignedTotalBuildPower = maxAssignedBuildPower;
	bt->maxTotalBuildPower = maxBuildPower;
	// remove the factory from factory->supportbuilders (if it was added)

	// find how much of the unitUnderConstruction is done
	float hp = ai->cb->GetUnitHealth(unitUnderConstruction);
	float oldHP = bt->hpLastFrame;
	float endHp = unitDef->health;
	float eNeed = unitDef->energyCost;
	float mNeed = unitDef->metalCost;
	float buildTime = unitDef->buildTime;
	// NOTE: the damage part needs testing
	float builtRatio = (hp + bt->damage) / endHp;
//	float usedE = builtRatio * eNeed;
//	float usedM = builtRatio * mNeed;
//	float currentBuildPower = energyUsage / currentMaxE_usage;
//	float currentBuildPower2 = metalUsage / currentMaxM_usage
//
//	if(currentBuildPower == 0)
//		currentBuildPower = 0.00000000000000001;
//
//	float eta = (((1.0 - builtRatio) * buildTime) / maxBuildPower ) / currentBuildPower;

	// builders and factories are different (factories
	// start building the first frame, with max speed)
	if (hp <= 0.10000001 || (bt->etaFrame == -1 && !bt->buildTask)) {
		// unit was made this frame
		assert(maxAssignedBuildPower > 0);
//		float minTimeNeeded =  buildTime / maxAssignedBuildPower;
//		oldHP = hp;
	}

	if (hp > 0.11 && maxBuildPower > 0) {
		// building has started
		float minTimeNeeded =  buildTime / maxBuildPower;
		assert(minTimeNeeded > 0);
//		float currentMaxE_usage = eNeed / minTimeNeeded;
//		float currentMaxM_usage = mNeed / minTimeNeeded;

		// find the delta HP from last frame, note that the first
		// 0.1 HP is a free loan that is payed at the end
		float deltaHP = hp - oldHP;

		if (hp == endHp)
			deltaHP += 0.1;

		if (deltaHP <= 0.0f) {
			// nanostalling
			deltaHP = 0.0001;
		}
		if (bt->etaFrame == -1 && !bt->buildTask)
			deltaHP -= 0.1;

		assert(deltaHP > 0);
		float eta2 = endHp * (1.0 - builtRatio) / deltaHP;

		if (bt->etaFrame == -1) {
			// started to make the unit now
			bt->startedRealBuildingFrame = frame;
			bt->etaFrame = -2;
			bt->hpSomeTimeAgo = hp;
			// deltaHP -= 0.1;

			if (!bt->buildTask) {
				bt->etaFrame = int(eta2 + frame);
			}
		}

//		assert(eNeed > 0);
//		assert(mNeed > 0);
//		assert(currentMaxE_usage > 0);

		float usedE_ThisFrame = deltaHP / endHp * eNeed;
		float usedM_ThisFrame = deltaHP / endHp * mNeed;
//		float calcUsedE_ThisFrame = currentMaxE_usage / 30;
//		float currentBuildPower =  32.0 * maxBuildPower * (usedE_ThisFrame / currentMaxE_usage);
		constructionEnergy += usedE_ThisFrame;
		constructionMetal += usedM_ThisFrame;
		oldHP = hp;
		bt->hpLastFrame = hp;

		if ((frame - bt->startedRealBuildingFrame) % 16 == 0 && (bt->startedRealBuildingFrame +16) <= frame) {
			// the damage part needs testing
			float longDeltaHP = (hp + bt->damage) - bt->hpSomeTimeAgo;

			if (longDeltaHP <= 0) {
				// builder must have been pushed away and Spring says it still builds
				longDeltaHP = 0.000001;
			}

			// assert(longDeltaHP > 0);
			float longEta = endHp * (1.0 - builtRatio) / (longDeltaHP / 16.0);
			bt->etaFrame = int(longEta + frame + 1);
			bt->hpSomeTimeAgo = hp;
			bt->energyUsage =  longDeltaHP / endHp * eNeed;
			bt->metalUsage =  longDeltaHP / endHp * mNeed;
			// bt->damageSomeTimeAgo = bt->damage;
		}

		// HACK
		bt->economyUnitTracker->createFrame = bt->etaFrame;
	} else {
		// not started yet (unit moving the nano-emitter thing)
		// NOTE: the builder(s) might have been killed too...
		bt->hpLastFrame = hp;
	}
	

}


void CEconomyTracker::UnitCreated(int unit) {
	if(trackerOff)
		return;

	int frame = ai->cb->GetCurrentFrame();

	if (frame == 0) {
		// ignore the commander
		return;
	}

	EconomyUnitTracker* economyUnitTracker = new EconomyUnitTracker;
	economyUnitTracker->clear();
	economyUnitTracker->economyUnitId = unit;
	economyUnitTracker->createFrame = -frame;
	economyUnitTracker->alive = true;
	economyUnitTracker->category = GCAT(unit);
	economyUnitTracker->unitDef = ai->cb->GetUnitDef(unit);
	SetUnitDefDataInTracker(economyUnitTracker);
	underConstructionEconomyUnitTrackers.push_back(economyUnitTracker);


	// find it (slow++)
	bool found = false;
	for (int category = 0; category < LASTCATEGORY; category++ ) {
		for (list<BuildTask>::iterator i = ai->uh->BuildTasks[category].begin(); i != ai->uh->BuildTasks[category].end(); i++) {
			BuildTask bt = *i;

			if (bt.id == unit) {
				// add this new unit to the list
				BuildingTracker tracker;
				tracker.clear();
				tracker.economyUnitTracker = economyUnitTracker;
				tracker.buildTask = true;
				tracker.category = category;
				tracker.unitUnderConstruction = unit;
				allTheBuildingTrackers[category].push_front(tracker);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		// it is made by a factory
		float3 unitPos = ai->cb->GetUnitPos(unit);
		int category = GCAT(unit);

		for (list<Factory>::iterator i = ai->uh->Factories.begin(); i != ai->uh->Factories.end(); i++) {
			Factory factory = *i;
			int factoryId = factory.id;
			// bad, no easy way to get the factory of the unit
			float3 factoryPos = ai->cb->GetUnitPos(factoryId);
			float distance = factoryPos.distance2D(unitPos);

			if (distance < 100) {
				BuildingTracker tracker;
				tracker.clear();
				tracker.economyUnitTracker = economyUnitTracker;
				tracker.category = category;
				tracker.unitUnderConstruction = unit;
				tracker.factory = factoryId;
				allTheBuildingTrackers[category].push_front(tracker);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		// unit constructor not found?!
	}
}



void CEconomyTracker::SetUnitDefDataInTracker(EconomyUnitTracker* economyUnitTracker) {
	economyUnitTracker->unitDef = ai->cb->GetUnitDef(economyUnitTracker->economyUnitId);
	float energyProduction = 0;
	float metalProduction = 0;
	energyProduction += economyUnitTracker->unitDef->energyMake;
	metalProduction += economyUnitTracker->unitDef->metalMake;

	if (economyUnitTracker->unitDef->windGenerator > 0) {
		// it makes (some) power from wind
		int minWind = int(ai->cb->GetMinWind());
		int maxWind = int(ai->cb->GetMaxWind());
		energyProduction += ((minWind + maxWind) / 2.0);
	}

	// tidalGenerator?
	if (economyUnitTracker->unitDef->tidalGenerator > 0) {
		// NOTE: is this the power or a bool ?  and can it be negative ?
		// it makes (some) power from water
		energyProduction += ai->cb->GetTidalStrength();
	}

	economyUnitTracker->estimateEnergyChangeFromDefWhileOff = energyProduction / 2;
	economyUnitTracker->estimateMetalChangeFromDefWhileOff = metalProduction / 2;
	// add the ON state change
	energyProduction -= economyUnitTracker->unitDef->energyUpkeep;
	metalProduction -= economyUnitTracker->unitDef->metalUpkeep;

	if (economyUnitTracker->unitDef->isMetalMaker) {
		metalProduction += economyUnitTracker->unitDef->makesMetal;
	}

	if (economyUnitTracker->unitDef->extractsMetal) {
		// it's a mex: must find out what it will make later on (look at the metalMap spot data?)
		vector<float3> spots = ai->mm->VectoredSpots;
		float3 thisPos = ai->cb->GetUnitPos(economyUnitTracker->economyUnitId);
		bool foundMexSpot = false;

		for (vector<float3>::iterator i = spots.begin(); i != spots.end(); i++) {
			float distance = i->distance2D(thisPos);
			if (distance < 48) {
				// HACK
				float metalMakeFromThisSpot = i->y;
				metalMakeFromThisSpot *= economyUnitTracker->unitDef->extractsMetal;
				metalProduction += metalMakeFromThisSpot;
				foundMexSpot = true;
				break;
			}
		}
		assert(foundMexSpot);
		// metalProduction += conomyUnitTracker->def->makesMetal;
	}

	economyUnitTracker->estimateEnergyChangeFromDefWhileOn = energyProduction / 2;;
	economyUnitTracker->estimateMetalChangeFromDefWhileOn = metalProduction / 2;;
}



void CEconomyTracker::UnitFinished(int unit) {
	if (trackerOff)
		return;

	int frame = ai->cb->GetCurrentFrame();
	
	if (frame == 0) {
		// add the commander to a EconomyUnitTracker
		EconomyUnitTracker * economyUnitTracker = new EconomyUnitTracker;
		economyUnitTracker->clear();
		economyUnitTracker->economyUnitId = unit;
		economyUnitTracker->createFrame = frame;
		economyUnitTracker->alive = true;
		economyUnitTracker->category = GCAT(unit);
		economyUnitTracker->unitDef = ai->cb->GetUnitDef(unit);
		SetUnitDefDataInTracker(economyUnitTracker);
		newEconomyUnitTrackers.push_back(economyUnitTracker);
		return;
	}

	// move the new EconomyUnitTrackers
	bool found = false;
	list<EconomyUnitTracker*> removeList;
	for (list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++) {
		EconomyUnitTracker *bt = *i;
		if (bt->economyUnitId == unit) {
			 bt->createFrame = frame;
			assert(bt->alive);
			newEconomyUnitTrackers.push_back(bt);
			removeList.push_back(bt);
			found = true;
			break;
		}
	}

	assert(found);
	// remove them from underConstructionEconomyUnitTrackers
	for (list<EconomyUnitTracker*>::iterator i = removeList.begin(); i != removeList.end(); i++) {
		underConstructionEconomyUnitTrackers.remove(*i);
	}

	int category = ai->ut->GetCategory(unit);
	found = false;

	if (category != -1) {
		for (list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++) {
			BuildingTracker* bt = &*i;
			if (bt->unitUnderConstruction == unit) {
				updateUnitUnderConstruction(bt);
				found = true;
				allTheBuildingTrackers[category].erase(i);
				break;
			}
		}
		if (!found) {
			// unit not in a BuildingTracker?
		}
	}
}




void CEconomyTracker::UnitDestroyed(int unit) {
	if (trackerOff)
		return;

	assert(ai->cb->GetUnitDef(unit) != NULL);
	int frame = ai->cb->GetCurrentFrame();

	// move the dead EconomyUnitTracker
	bool found = false;
	for (list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++) {
		EconomyUnitTracker *bt = *i;
		if (bt->economyUnitId == unit) {
			assert(bt->alive);
			bt->alive = false;
			bt->dieFrame = frame;
			deadEconomyUnitTrackers.push_back(bt);
			activeEconomyUnitTrackers.remove(bt);
			// was alive for (frame - bt->createFrame) frames
			found = true;
			break;
		}
	}
	if (!found) {
		for (list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker *bt = *i;
			if (bt->economyUnitId == unit) {
				assert(bt->alive);
				bt->alive = false;
				bt->dieFrame = frame;
				deadEconomyUnitTrackers.push_back(bt);
				underConstructionEconomyUnitTrackers.remove(bt);
				// was still under construction
				found = true;
				break;
			}
		}
	}
	if (!found) {
		for (list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++) {
			EconomyUnitTracker *bt = *i;
			if (bt->economyUnitId == unit) {
				assert(bt->alive);
				bt->alive = false;
				bt->dieFrame = frame;
				deadEconomyUnitTrackers.push_back(bt);
				newEconomyUnitTrackers.remove(bt);
				// was alive for (frame - bt->createFrame) frames and never managed to do anything
				found = true;
				break;
			}
		}
	}

	// if unit was being built, remove it
	if (ai->cb->UnitBeingBuilt(unit)) {
		int category = ai->ut->GetCategory(unit);
		bool found = false;

		if (category != -1) {
			for (list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++) {
				BuildingTracker *bt = &*i;
				if (bt->unitUnderConstruction == unit) {
					// hp will be negative if this re-enabled
					// updateUnitUnderConstruction(bt);
					found = true;
					allTheBuildingTrackers[category].erase(i);
					break;
				}
			}
			if (!found) {
				// unit not in a BuildingTracker?
			}
		}
	}
}



void CEconomyTracker::UnitDamaged(int unit, float damage) {
	if (trackerOff)
		return;

	if (ai->cb->UnitBeingBuilt(unit)) {
		int category = ai->ut->GetCategory(unit);
		bool found = false;

		if (category != -1) {
			for (list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++) {
				BuildingTracker *bt = &*i;
				if (bt->unitUnderConstruction == unit) {
					bt->damage += damage;
					bt->hpLastFrame -= damage;
					found = true;
					break;
				}
			}
			if (!found) {
				// unit not in a BuildingTracker?
			}
		}
	}
}
