#include "Include.h"
#include "creg/STL_List.h"
#include "creg/STL_Map.h"
#include "EconomyTracker.h"

CR_BIND(CEconomyTracker , (NULL))

CR_REG_METADATA(CEconomyTracker,(
	CR_MEMBER(allTheBuildingTrackers),
	CR_MEMBER(deadEconomyUnitTrackers),
	CR_MEMBER(newEconomyUnitTrackers),
	CR_MEMBER(activeEconomyUnitTrackers),
	CR_MEMBER(underConstructionEconomyUnitTrackers),
	
	CR_MEMBER(ai),
	
	CR_MEMBER(trackerOff),
	
	CR_MEMBER(oldEnergy),
	CR_MEMBER(oldMetal),
	
	CR_MEMBER(myCalcEnergy),
	CR_MEMBER(myCalcMetal),
	
	CR_MEMBER(constructionEnergy),
	CR_MEMBER(constructionMetal),

	CR_MEMBER(constructionEnergySum),
	CR_MEMBER(constructionMetalSum),
	CR_RESERVED(128)
				));

CR_BIND(BuildingTracker ,)

CR_REG_METADATA(BuildingTracker,(
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
				CR_RESERVED(64)
				));

CR_BIND(EconomyUnitTracker ,)

CR_REG_METADATA(EconomyUnitTracker,(
				CR_MEMBER(ai),
				CR_MEMBER(economyUnitId),
				CR_MEMBER(createFrame),
				CR_MEMBER(buildingTracker),
				CR_MEMBER(alive),
				CR_MEMBER(unitDefName),
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
				CR_RESERVED(64),
				CR_POSTLOAD(PostLoad)
				));

void EconomyUnitTracker::PostLoad()
{
	unitDef = ai->cb->GetUnitDef(unitDefName.c_str());
}

CEconomyTracker::CEconomyTracker(AIClasses* ai)
{
	this->ai=ai;
	allTheBuildingTrackers.resize(LASTCATEGORY);
	oldEnergy = ai?ai->cb->GetEnergy():0;
	oldMetal = ai?ai->cb->GetMetal():0;
	constructionEnergySum = 0;
	constructionMetalSum = 0;
	constructionEnergy = 0;
	constructionMetal = 0;
	myCalcEnergy = ai?ai->cb->GetEnergy():0;
	myCalcMetal = ai?ai->cb->GetMetal():0;
//	for(int i = 0; i < LASTCATEGORY; i++){
//		allTheBuildingTrackers[i] = new list<BuildingTracker>;
//		allTheBuildingTrackers[i]->clear();
//	}
	//deadEconomyUnitTrackers.reserve(10000);
	//newEconomyUnitTrackers.reserve(50);
	//activeEconomyUnitTrackers.reserve(10000);
	//underConstructionEconomyUnitTrackers.reserve(100);
	
	trackerOff = true;
	
}

CEconomyTracker::~CEconomyTracker()
{
//	for(int i = 0; i < LASTCATEGORY; i++){
//		delete allTheBuildingTrackers[i];
//	}
	for(list<EconomyUnitTracker*>::iterator i = deadEconomyUnitTrackers.begin(); i != deadEconomyUnitTrackers.end(); i++){
		delete *i;
	}
	for(list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++){
		delete *i;
	}
	for(list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++){
		delete *i;
	}
	for(list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++){
		delete *i;
	}
}

void CEconomyTracker::frameUpdate()
{
	if(trackerOff)
		return;
	int frame=ai->cb->GetCurrentFrame();
	L(frame);
	// Iterate over all the BuildTasks:
	/*
	for(int category = 0; category < LASTCATEGORY; category++ )
	{
		for(list<BuildTask>::iterator i = ai->uh->BuildTasks[category]->begin(); i != ai->uh->BuildTasks[category]->end(); i++){
			BuildTask bt = *i;
			updateUnitUnderConstruction(&bt);
		}
	}*/
	for(int category = 0; category < LASTCATEGORY; category++ )
	{
		for(list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++){
			BuildingTracker *bt = &*i; // This is bad, but needed
			updateUnitUnderConstruction(bt);
		}
	}
	constructionEnergySum += constructionEnergy;
	constructionMetalSum += constructionMetal;
	// Iterate over all the factories:
	
	
	//	bool IsUnitActivated (int unitid); 
	
	// Move the new EconomyUnitTrackers
	list<EconomyUnitTracker*> removeList;
	for(list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++){
		EconomyUnitTracker *bt = *i; // This is bad, but needed
		assert(frame - bt->createFrame <= 16);
		if(frame - bt->createFrame == 16)
		{
			// Move it to the active list
			assert(bt->alive);
			activeEconomyUnitTrackers.push_back(bt);
			removeList.push_back(bt);
			L("Moved "  << bt->unitDef->humanName << " to activeEconomyUnitTrackers");
		}
	}
	// Remove them from newEconomyUnitTrackers: 
	for(list<EconomyUnitTracker*>::iterator i = removeList.begin(); i != removeList.end(); i++){
		newEconomyUnitTrackers.remove(*i);
	}
	// Update the units in activeEconomyUnitTrackers, add their production/usage to total
	float energyProduction = 0;
	float metalProduction = 0;
	float energyUsage = 0; // This is without what is used by builders
	float metalUsage = 0; // This is without what is used by builders
	if(frame % 16 == 0)
	{
		// Update
		for(list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *bt = *i; // This is bad, but needed
			assert(bt->alive);
			//L("Adding "  << bt->unitDef->humanName << " UnitResourceInfo....");
			// Hmmm
			// If the unit is a builder then we track its resource usage already
			// if its useing more then its weapon fire or upkeep
			UnitResourceInfo resourceInfo;
			bool isAlive = ai->cb->GetUnitResourceInfo(bt->economyUnitId, &resourceInfo);
			assert(isAlive);
			
			// Add the change from last update
			bt->totalEnergyMake += bt->lastUpdateEnergyMake = resourceInfo.energyMake - bt->lastUpdateEnergyMake;
			bt->totalMetalMake += bt->lastUpdateMetalMake = resourceInfo.metalMake - bt->lastUpdateMetalMake;
			bt->totalEnergyUsage += bt->lastUpdateEnergyUsage = resourceInfo.energyUse - bt->lastUpdateEnergyUsage;
			bt->totalMetalUsage += bt->lastUpdateMetalUsage = resourceInfo.metalUse - bt->lastUpdateMetalUsage;
			energyProduction += bt->lastUpdateEnergyMake;
			metalProduction += bt->lastUpdateMetalMake;
			if(!bt->unitDef->builder)
			{
				// Hmmmm
				energyUsage += bt->lastUpdateEnergyUsage;
				metalUsage += bt->lastUpdateMetalUsage;
			} 
		}
	}
	
	float energy = ai->cb->GetEnergy();
	float metal = ai->cb->GetMetal();
	float deltaEnergy = energy - oldEnergy + constructionEnergy;
	float deltaMetal = metal - oldMetal + constructionMetal;
	myCalcEnergy -= constructionEnergy;
	myCalcMetal -= constructionMetal;
	if(frame % 16 == 0)
	{
		// Add the activeEconomyUnitTrackers change now
		myCalcEnergy += energyProduction - energyUsage;
		myCalcMetal += metalProduction - metalUsage;
		if(myCalcEnergy > ai->cb->GetEnergyStorage())
			myCalcEnergy = ai->cb->GetEnergyStorage();
		if(myCalcMetal > ai->cb->GetMetalStorage())
			myCalcMetal = ai->cb->GetMetalStorage();
		L("energy:         " << ai->cb->GetEnergy() << ", metal:     " << ai->cb->GetMetal());
		L("myCalcEnergy: " << myCalcEnergy <<        ", myCalcMetal: " << myCalcMetal);
		L("energyProduction: " << energyProduction << ", energyUsage: " << energyUsage);
		L("metalProduction: " << metalProduction << ", metalUsage: " << metalUsage);
		
		makePrediction(frame + 320 + 160);
		
	}
	//if(frame % 30 == 0)
	//	L("energy:         " << ai->cb->GetEnergy() << ", metal:     " << ai->cb->GetMetal());
	//L("myCalcEnergy: " << myCalcEnergy <<        ", myCalcMetal: " << myCalcMetal);
	if(fabs(deltaEnergy) > 0.001 || fabs(deltaMetal) > 0.001)
		L("deltaEnergy:  " << deltaEnergy <<         ", deltaMetal:  " << deltaMetal);
	if(frame % 32 == 0)
		L("constructionEnergy: " << constructionEnergy << ", constructionMetal: " << constructionMetal);
	if(frame % 32 == 0)
	{
		L("constructionEnergySum: " << constructionEnergySum << ", constructionMetalSum: " << constructionMetalSum);
	}
	oldEnergy = energy;
	oldMetal = metal;
	constructionEnergy = 0;
	constructionMetal = 0;
	
	L(frame << ", end");
	L("");
}


TotalEconomyState CEconomyTracker::makePrediction(int targetFrame)
{
	int frame=ai->cb->GetCurrentFrame();
	// Find the current state.
	
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
	
	TotalEconomyState state2 = state; // Make a copy
	
	// Make the prediction loop....
	// Just iterate over all the non dead EconomyUnitTrackers... (super hack)
	// Add the usage from the BuildingTrackers too, until the stuff they make hits their eta.
	bool printedStalingMessage = false;
	for(int preFrame = frame; preFrame <= targetFrame; preFrame += 16) // int preFrame = frame +16 ?
	{
		// Do the BuildingTrackers
		float constructionEnergy = 0;
		float constructionMetal = 0;
		for(int category = 0; category < LASTCATEGORY; category++ )
		{
			for(list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++){
				BuildingTracker *bt = &*i; // This is bad, but needed
				// Hack, useing the "semi useless" GetCurrentFrame() stats only:
				if(bt->etaFrame >= preFrame)
				{
					// Its not done yet
					constructionEnergy += bt->energyUsage;
					constructionMetal += bt->metalUsage;
				}
			}
		}
		float unitEnergy = 0;
		float unitMetal = 0;
		// Do the EconomyUnitTrackers in activeEconomyUnitTrackers, it needs no changes (metalmakers == bad)
		for(list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *eut = *i; // This is bad, but needed
			
			// We guess its on atm...
			unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
			unitMetal += eut->estimateMetalChangeFromDefWhileOn; 
		}
		// Do the EconomyUnitTrackers in newEconomyUnitTrackers, it needs no changes (metalmakers == bad)
		for(list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *eut = *i; // This is bad, but needed
			
			// We guess its on atm...
			unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
			unitMetal += eut->estimateMetalChangeFromDefWhileOn; 
		}
		// Do the EconomyUnitTrackers in newEconomyUnitTrackers, it needs to test the eta first (metalmakers == bad,  nanostall == bad)
		for(list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *eut = *i; // This is bad, but needed
			
			if(eut->createFrame +16 < preFrame)
			{
				// We guess its on atm...
				unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
				unitMetal += eut->estimateMetalChangeFromDefWhileOn; 
			}
		}
		// Make the new bank balance:
		state.energyMake = unitEnergy;
		state.energyStored += unitEnergy - constructionEnergy;
		state.energyUsage = constructionEnergy; // HACK...
		state.metalMake = unitMetal;
		state.metalStored += unitMetal - constructionMetal;
		state.metalUsage = constructionMetal; // HACK...
		bool staling = false;
		if(state.energyStored <= 0)
		{
			staling = true;
			state.energyStored = 0;
		}
		if(state.metalStored <= 0)
		{
			staling = true;
			state.metalStored = 0;
		}
		if(state.energyStored > ai->cb->GetEnergyStorage())
		{
			state.energyStored = ai->cb->GetEnergyStorage();
		}
		if(state.metalStored > ai->cb->GetMetalStorage())
		{
			state.metalStored = ai->cb->GetMetalStorage();
		}
		if(staling)
		{
			// Now its bad... must turn stuff off (metalmakers), and cun back on construction (and fix the global eta)
			if(!printedStalingMessage)
			{
				char c[500];
				int time = (preFrame - frame) / 30;
				sprintf(c,"Staling detected in : %i seconds", time);
				L(c);
				ai->cb->SendTextMsg(c,0);
				printedStalingMessage = true;
				//break;
			}
		}
		state.frame = preFrame;
	}
	
	{
		// Try nr.2:
		// Add all the activeEconomyUnitTrackers in one go at the start:
		float unitEnergy = 0;
		float unitMetal = 0;
		// Do the EconomyUnitTrackers in activeEconomyUnitTrackers, it needs no changes (metalmakers == bad)
		for(list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *eut = *i; // This is bad, but needed
			
			// We guess its on atm...
			unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
			unitMetal += eut->estimateMetalChangeFromDefWhileOn;
		}
		// Now this part is done
		state2.energyMake = unitEnergy;
		state2.metalMake = unitMetal;
		
		
		// Make a copy of the EconomyUnitTracker lists
		list<EconomyUnitTracker*> allFutureEconomyUnitTrackers;
		for(list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *eut = *i; // This is bad, but needed
			allFutureEconomyUnitTrackers.push_back(eut);
		}
		for(list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *eut = *i; // This is bad, but needed
			allFutureEconomyUnitTrackers.push_back(eut);
		}
		
		// Start the new loop:
		for(int preFrame = frame; preFrame <= targetFrame; preFrame += 16) // int preFrame = frame +16 ?
		{
			// Do the EconomyUnitTrackers in newEconomyUnitTrackers, it needs to test the eta first (metalmakers == bad,  nanostall == bad)
			unitEnergy = 0;
			unitMetal = 0;
			for(list<EconomyUnitTracker*>::iterator i = allFutureEconomyUnitTrackers.begin(); i != allFutureEconomyUnitTrackers.end(); i++){
				EconomyUnitTracker *eut = *i; // This is bad, but needed
				
				if(eut->createFrame +16 < preFrame && eut->createFrame + 32 >= preFrame)
				{
					// We guess its on atm...
					unitEnergy += eut->estimateEnergyChangeFromDefWhileOn;
					unitMetal += eut->estimateMetalChangeFromDefWhileOn;
					state2.energyStorageSize += eut->unitDef->energyStorage; // Update the storage change 
					state2.metalStorageSize += eut->unitDef->metalStorage;
				}
			}
			// Add in the new production:
			state2.energyMake += unitEnergy;
			state2.metalMake += unitMetal;
			
			// Do the BuildingTrackers:
			float constructionEnergy = 0;
			float constructionMetal = 0;
			for(int category = 0; category < LASTCATEGORY; category++ )
			{
				for(list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++){
					BuildingTracker *bt = &*i; // This is bad, but needed
					// Hack, useing the "semi useless" GetCurrentFrame() stats only:
					if(bt->etaFrame >= preFrame)
					{
						// Its not done yet (hack)
						constructionEnergy += bt->energyUsage;
						constructionMetal += bt->metalUsage;
					}
				}
			}
			// Make the new bank balance:
			state2.energyStored += state2.energyMake - constructionEnergy;
			state2.energyUsage = constructionEnergy; // HACK...
			state2.metalStored += state2.metalMake - constructionMetal;
			state2.metalUsage = constructionMetal; // HACK...
			bool staling = false;
			if(state2.energyStored <= 0)
			{
				staling = true;
				state2.energyStored = 0;
			}
			if(state2.metalStored <= 0)
			{
				staling = true;
				state2.metalStored = 0;
			}
			if(state2.energyStored > state2.energyStorageSize)
			{
				state2.energyStored = state2.energyStorageSize;
			}
			if(state2.metalStored > state2.metalStorageSize)
			{
				state2.metalStored = state2.metalStorageSize;
			}
			
		}
		
	}
	
	
	char c[500];
	int time = (targetFrame - frame) / 30;
	sprintf(c,"1 %is: e: %i, %3.1f, m: %i, %3.1f", time, (int)state.energyStored, (state.energyMake - state.energyUsage)*2, (int)state.metalStored, (state.metalMake - state.metalUsage)*2);
	L(c);
	ai->cb->SendTextMsg(c,0);
	
	sprintf(c,"2 %is: e: %i, %3.1f, m: %i, %3.1f", time, (int)state2.energyStored, (state2.energyMake - state2.energyUsage)*2, (int)state2.metalStored, (state2.metalMake - state2.metalUsage)*2);
	L(c);
	ai->cb->SendTextMsg(c,0);
	
	
	
	return state;
}


void CEconomyTracker::updateUnitUnderConstruction(BuildingTracker * bt)
{
	// Need to find out how much resourses that have been used on this unit
	// Just useing its hp wont work (it might be damaged)
	// Must sum up what the builders spends...
	// Find the builders that work on this unit, and sum up their spending:
	int unitUnderConstruction = bt->unitUnderConstruction;
	const UnitDef * unitDef = ai->cb->GetUnitDef(unitUnderConstruction);
	assert(unitDef != NULL);
	int frame=ai->cb->GetCurrentFrame();
	bt->economyUnitTracker->buildingTracker = bt;
	// Make the builder list
	//list<int> * builderList;
	list<BuilderTracker*> * builderTrackerList;
	if(bt->buildTask)
	{
		bool found = false;
		for(list<BuildTask*>::iterator i = ai->uh->BuildTasks[bt->category].begin(); i != ai->uh->BuildTasks[bt->category].end(); i++){
			//BuildTask buildTask = *i;
			if((*i)->id == unitUnderConstruction)
			{
				//builderList = &i->builders;
				builderTrackerList = &(*i)->builderTrackers;
				found = true;
				//L("Found the builderList");
				break;
			}
		}
		if(!found)
		{
			// This is needed.... ()
			L("Didnt find the buildGroup of unit " << unitDef->humanName << ", id: " << bt->unitUnderConstruction);
			//return;
		}
		assert(found);
	} else
	{
		bool found = false;
		for(list<Factory>::iterator i = ai->uh->Factories.begin(); i != ai->uh->Factories.end(); i++){
			//Factory factory = *i;
			//int factoryId = factory.id;
			if(i->id == bt->factory)
			{
				//builderList = &i->supportbuilders;
				builderTrackerList = &i->supportBuilderTrackers;
				found = true;
				break;
			}
		}
		if(!found)
		{
			// This is needed.... ()
			L("Didnt find the factory of unit " << unitDef->humanName << ", id: " << bt->unitUnderConstruction << "... factory id: " << bt->factory);
			return;
		}
		//assert(found);
	}
	//L("builderList: " << builderList);
	//assert(builderList != NULL);
	assert(builderTrackerList != NULL);
	//int numBuilders = builderList->size();
	int numBuilders = builderTrackerList->size();
	if(frame % 30 == 2)
	{
		L("Looking at " << unitDef->humanName << ", it have " << numBuilders << " builders");
	}
	
	
	float metalUsage = 0;
	float energyUsage = 0;
	float maxBuildPower = 0;
	float maxAssignedBuildPower = 0;
	for(list<BuilderTracker*>::iterator i = builderTrackerList->begin(); i != builderTrackerList->end(); i++)
	//for(list<int>::iterator i = builderList->begin(); i != builderList->end(); i++)
	{
		//int builder = *i;
		BuilderTracker* builderTracker = *i;
		UnitResourceInfo resourceInfo;
		//bool isAlive = ai->cb->GetUnitResourceInfo(builder, &resourceInfo);
		bool isAlive = ai->cb->GetUnitResourceInfo(builderTracker->builderID, &resourceInfo);
		assert(isAlive); // TODO comment this back in
		if(!isAlive)
			continue;
		metalUsage += resourceInfo.metalUse;
		energyUsage += resourceInfo.energyUse;
		const UnitDef * unitDefBuilder = ai->cb->GetUnitDef(builderTracker->builderID);
		maxAssignedBuildPower += unitDefBuilder->buildSpeed;
		if(resourceInfo.metalUse > unitDefBuilder->metalUpkeep) // Is this needed ???
			maxBuildPower += unitDefBuilder->buildSpeed;
	}
	if(!bt->buildTask)
	{
		assert(ai->cb->GetUnitDef(bt->factory) != NULL); // This will fail
		maxAssignedBuildPower += ai->cb->GetUnitDef(bt->factory)->buildSpeed;
		maxBuildPower += ai->cb->GetUnitDef(bt->factory)->buildSpeed;
		//L("maxAssignedBuildPower: " << maxAssignedBuildPower);
		//L("maxBuildPower: " << maxBuildPower);
	}
	//L("maxAssignedBuildPower: " << maxAssignedBuildPower);
	//L("maxBuildPower: " << maxBuildPower);
	bt->assignedTotalBuildPower = maxAssignedBuildPower;
	bt->maxTotalBuildPower = maxBuildPower;
	// Remove the factory from factory->supportbuilders (if it was added)
	
	// Find how much of the unitUnderConstruction that is made:
	float hp = ai->cb->GetUnitHealth(unitUnderConstruction);
	float oldHP = bt->hpLastFrame;
	float endHp = unitDef->health;
	float eNeed = unitDef->energyCost;
	float mNeed = unitDef->metalCost;
	float buildTime = unitDef->buildTime;
	float builtRatio = (hp + bt->damage) / endHp; // The damage part needs testing
	float usedE = builtRatio * eNeed;
	float usedM = builtRatio * mNeed;
	//float currentBuildPower = energyUsage / currentMaxE_usage;
	//float currentBuildPower2 = metalUsage / currentMaxM_usage
	/*
	if(currentBuildPower != currentBuildPower2)
	{
		L("currentBuildPower: " << currentBuildPower << ", currentBuildPower2: " << currentBuildPower2);
	}*/
	
	//if(currentBuildPower == 0)
	//	currentBuildPower = 0.00000000000000001;
	
	//float eta = (((1.0 - builtRatio) * buildTime) / maxBuildPower ) / currentBuildPower;
	
	// Builders and factories are diffrent.
	// Factories start building the first frame, with max speed
	if(hp <= 0.10000001 || (bt->etaFrame == -1 && !bt->buildTask))
	{
		// The Unit was made this frame
		assert(maxAssignedBuildPower > 0);
		float minTimeNeeded =  buildTime / maxAssignedBuildPower;    //seconds
		L("Looking at " << unitDef->humanName << ", it have " << numBuilders << " builders");
		L("The Unit was made this frame");
		L("hp: " << hp << ", minTimeNeeded: " << minTimeNeeded << ", maxAssignedBuildPower: " << maxAssignedBuildPower);
		L("endHp: " << endHp << ", buildTime: " << buildTime);
		//oldHP = hp;
	}
	
	if(hp > 0.11 && maxBuildPower > 0)
	{
		// Building have started (temp)
		//L("hp > 0.105     : " << hp);
		float minTimeNeeded =  buildTime / maxBuildPower;    //seconds
		assert(minTimeNeeded > 0);
		float currentMaxE_usage = eNeed / minTimeNeeded;
		
		// Find the delta hp from last frame, note that the first 0.1 hp is a free loan that is payed at the end
		float deltaHP = hp - oldHP;
		if(hp == endHp)
			deltaHP += 0.1;
			
		if(deltaHP <= 0) // Nanostaling
			deltaHP = 0.0001;
		if(bt->etaFrame == -1 && !bt->buildTask)
			deltaHP -= 0.1;
		assert(deltaHP > 0);
		float eta2 = endHp * (1.0 - builtRatio) / deltaHP;
		if(bt->etaFrame == -1)
		{
			// Started the to make the unit now
			bt->startedRealBuildingFrame = frame;
			bt->etaFrame = -2;
			bt->hpSomeTimeAgo = hp;
			//deltaHP -= 0.1;
			if(!bt->buildTask)
			{
				bt->etaFrame = int(eta2 + frame);
				L("First eta for (factory made) " << unitDef->humanName << ": " << bt->etaFrame);
			}
			
			//bt->etaFrame = eta2;
			//L("First eta for " << unitDef->humanName << ": " << bt->etaFrame);
		}
		assert(eNeed > 0);
		assert(mNeed > 0);
		assert(currentMaxE_usage > 0);
		float usedE_ThisFrame = deltaHP / endHp * eNeed;
		float usedM_ThisFrame = deltaHP / endHp * mNeed;

		float currentBuildPower =  32.0 * maxBuildPower * (usedE_ThisFrame / currentMaxE_usage);
		constructionEnergy += usedE_ThisFrame;
		constructionMetal += usedM_ThisFrame;
		oldHP = hp;
		bt->hpLastFrame = hp;
		
		if((frame - bt->startedRealBuildingFrame) % 16 == 0 && (bt->startedRealBuildingFrame +16) <= frame)
		{
			float longDeltaHP = (hp + bt->damage) - bt->hpSomeTimeAgo; // The damage part needs testing
			if(longDeltaHP <= 0)
				longDeltaHP = 0.000001; // The builder must have been pushed away and spring says it still builds
			//assert(longDeltaHP > 0);
			float longEta = endHp * (1.0 - builtRatio) / (longDeltaHP / 16.0);
			bt->etaFrame = int(longEta + frame + 1);
			if(bt->startedRealBuildingFrame +16 == frame)
			{
				L("First eta for " << unitDef->humanName << ": " << bt->etaFrame);
			}
			L("bt->startedRealBuildingFrame: " << bt->startedRealBuildingFrame);
			L("longDeltaHP at frame: " << frame << ", eta: " << bt->etaFrame);
			bt->hpSomeTimeAgo = hp;
			bt->energyUsage =  longDeltaHP / endHp * eNeed;
			bt->metalUsage =  longDeltaHP / endHp * mNeed;
			
			//bt->damageSomeTimeAgo = bt->damage;
		}
		
		// HACK:
		bt->economyUnitTracker->createFrame = bt->etaFrame;
		
		if(frame % 30 == 2)
		{
			//L("maxAssignedBuildPower: " << maxAssignedBuildPower);
			//L("maxBuildPower: " << maxBuildPower);
			L("maxBuildPower: " << maxBuildPower << ", maxAssignedBuildPower: " << maxAssignedBuildPower  << ", currentBuildPower: " << currentBuildPower);
			L("usedM: " << usedM << ", usedE: " << usedE);
			L("deltaHP: " << deltaHP << ", usedE_ThisFrame: " << usedE_ThisFrame << ", usedM_ThisFrame: " << usedM_ThisFrame);
			L("spring metalUsage: " << metalUsage << ", energyUsage: " << energyUsage << ", builtRatio: " << builtRatio << ", hp: " << hp);
			L("eta2: " << (eta2+ frame) << ", bt->etaFrame: " << bt->etaFrame << ", time: " << ((eta2+ frame) / 30));
		}
	} else
	{
		// Not started yet (unit moveing the nano thing)
		bt->hpLastFrame = hp;
		// The builder(s) might have been killed too...
	}
	

}

void CEconomyTracker::UnitCreated(int unit)
{
	if(trackerOff)
		return;
	int frame=ai->cb->GetCurrentFrame();
	if(frame == 0)
	{
		// Ignore the commander
		return;
	}
		
	EconomyUnitTracker * economyUnitTracker = new EconomyUnitTracker;
	economyUnitTracker->clear();
	economyUnitTracker->ai = ai;
	economyUnitTracker->economyUnitId = unit;
	economyUnitTracker->createFrame = -frame;
	economyUnitTracker->alive = true;
	economyUnitTracker->category = GCAT(unit);
	economyUnitTracker->unitDef = ai->cb->GetUnitDef(unit);
	economyUnitTracker->unitDefName = economyUnitTracker->unitDef->name;
	SetUnitDefDataInTracker(economyUnitTracker);
	underConstructionEconomyUnitTrackers.push_back(economyUnitTracker);
	
	
	L("Started to make an " << ai->cb->GetUnitDef(unit)->humanName);
	// Find it (slow++)
	bool found = false;
	for(int category = 0; category < LASTCATEGORY; category++ )
	{
		for(list<BuildTask*>::iterator i = ai->uh->BuildTasks[category].begin(); i != ai->uh->BuildTasks[category].end(); i++){
			BuildTask* bt = *i;
			if(bt->id == unit)
			{
				// Add this new unit to the list
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
	if(!found)
	{
		// It is made by a factory
		float3 unitPos = ai->cb->GetUnitPos(unit);
		int category = GCAT(unit);
		for(list<Factory>::iterator i = ai->uh->Factories.begin(); i != ai->uh->Factories.end(); i++){
			Factory factory = *i;
			int factoryId = factory.id;
			// This is bad, no easy way to get the factory of the unit...
			float3 factoryPos = ai->cb->GetUnitPos(factoryId);
			float distance = factoryPos.distance2D(unitPos);
			L("factory distance: " << distance);
			if(distance < 100)
			{
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
	if(!found)
		L("Unit constructor not found !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	
	/*
	float EProductionNeed = unit->energyCost - ai->cb->GetEnergy();//ai->cb->GetEnergy() - unit->energyCost;
	float EProduction = ai->cb->GetEnergyIncome() - ai->cb->GetEnergyUsage();
	if(EProduction < 1)
		EProduction = 1;
	//L("EProductionNeed: " << EProductionNeed);
	//L("EProduction: " << EProduction);
	float MProductionNeed = unit->metalCost - ai->cb->GetMetal();//ai->cb->GetMetal() - unit->metalCost;
	float MProduction = ai->cb->GetMetalIncome() - ai->cb->GetMetalUsage();
	if(MProduction < 1)
		MProduction = 1;
	//L("MProductionNeed: " << MProductionNeed);
	//L("MProduction: " << MProduction);
	float timeToBuild = max (EProductionNeed / EProduction, MProductionNeed / MProduction);
	if(timeToBuild < 1)
		timeToBuild = 1; // It can be built right now...  Make something more costly instead?
	//L("timeToBuild: " << timeToBuild);
	//float METAL2ENERGY_current = ai->cb->GetEnergy() (ai->cb->GetEnergyIncome()*0.9 - ai->cb->GetEnergyUsage())
	//float buildtime = //unit->buildTime / builder->buildSpeed;
	// Take the buildtime, and use it smart.
	// Make a ratio of unit->buildTime / timeToBuild ???:
	// This realy needs to know its creators build speed.
	float buildTimeRatio =  unit->buildTime / timeToBuild;
	//L("buildTimeRatio: " << buildTimeRatio);
	float buildTimeFix = unit->buildTime / 100;
	*/
}

void CEconomyTracker::SetUnitDefDataInTracker(EconomyUnitTracker * economyUnitTracker)
{
	economyUnitTracker->unitDef = ai->cb->GetUnitDef(economyUnitTracker->economyUnitId);
	economyUnitTracker->unitDefName = economyUnitTracker->unitDef->name;
	// Energy
	float energyProduction = 0;
	float metalProduction = 0;
	energyProduction += economyUnitTracker->unitDef->energyMake;
	metalProduction += economyUnitTracker->unitDef->metalMake;
	if(economyUnitTracker->unitDef->windGenerator > 0)
	{
		// It makes (some) power from wind
		int minWind = int(ai->cb->GetMinWind());
		int maxWind = int(ai->cb->GetMaxWind());
		energyProduction += ((minWind + maxWind) / 2.0);
	}
	// tidalGenerator ?
	if(economyUnitTracker->unitDef->tidalGenerator > 0) // Is this the power or a bool ?  and can it be negative ?
	{
		// It makes (some) power from water
		energyProduction += ai->cb->GetTidalStrength();
	}
	economyUnitTracker->estimateEnergyChangeFromDefWhileOff = energyProduction / 2;
	economyUnitTracker->estimateMetalChangeFromDefWhileOff = metalProduction / 2;
	// Add the on state change:
	energyProduction -= economyUnitTracker->unitDef->energyUpkeep;
	metalProduction -= economyUnitTracker->unitDef->metalUpkeep;
	if(economyUnitTracker->unitDef->isMetalMaker)
	{
		metalProduction += economyUnitTracker->unitDef->makesMetal;
	}
	if(economyUnitTracker->unitDef->extractsMetal)
	{
		// Its a mex, what now ???
		// Must find out what it will make later on, or something...  (look at the metalMap spot data)
		vector<float3> spots = ai->mm->VectoredSpots;
		float3 thisPos = ai->cb->GetUnitPos(economyUnitTracker->economyUnitId);
		bool foundMexSpot = false;
		for(vector<float3>::iterator i = spots.begin(); i != spots.end(); i++)
		{
			float distance = i->distance2D(thisPos);
			if(distance < 48) // TODO:  hack
			{
				float metalMakeFromThisSpot = i->y;
				metalMakeFromThisSpot *= economyUnitTracker->unitDef->extractsMetal;
				//L("i->y: " << (i->y) << ", metalMakeFromThisSpot: " << metalMakeFromThisSpot);
				metalProduction += metalMakeFromThisSpot;
				foundMexSpot = true;
				break;
			}
		}
		assert(foundMexSpot);
		
		//metalProduction += conomyUnitTracker->def->makesMetal;
	}
	economyUnitTracker->estimateEnergyChangeFromDefWhileOn = energyProduction / 2;;
	economyUnitTracker->estimateMetalChangeFromDefWhileOn = metalProduction / 2;;
	L("off energy: " << economyUnitTracker->estimateEnergyChangeFromDefWhileOff << ", on " << economyUnitTracker->estimateEnergyChangeFromDefWhileOn);
	L("off metal: " << economyUnitTracker->estimateMetalChangeFromDefWhileOff << ", on " << economyUnitTracker->estimateMetalChangeFromDefWhileOn);
	
	
}

void CEconomyTracker::UnitFinished(int unit)
{
	if(trackerOff)
		return;
	int frame=ai->cb->GetCurrentFrame();
	
	if(frame == 0)
	{
		// Add the commander to a EconomyUnitTracker
		EconomyUnitTracker * economyUnitTracker = new EconomyUnitTracker;
		economyUnitTracker->clear();
		economyUnitTracker->ai = ai;
		economyUnitTracker->economyUnitId = unit;
		economyUnitTracker->createFrame = frame;
		economyUnitTracker->alive = true;
		economyUnitTracker->category = GCAT(unit);
		economyUnitTracker->unitDef = ai->cb->GetUnitDef(unit);
		economyUnitTracker->unitDefName = economyUnitTracker->unitDef->name;
		SetUnitDefDataInTracker(economyUnitTracker);
		newEconomyUnitTrackers.push_back(economyUnitTracker);
		return;
	}	
	
	// Move the new EconomyUnitTrackers
	bool found = false;
	list<EconomyUnitTracker*> removeList;
	for(list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++){
		EconomyUnitTracker *bt = *i; // This is bad, but needed
		if(bt->economyUnitId == unit)
		{
			 bt->createFrame = frame;
			// Move it to the new list
			assert(bt->alive);
			newEconomyUnitTrackers.push_back(bt);
			removeList.push_back(bt);
			L("Moved "  << bt->unitDef->humanName << " to newEconomyUnitTrackers");
			found = true;
			break;
		}
	}
	assert(found);
	// Remove them from underConstructionEconomyUnitTrackers: 
	for(list<EconomyUnitTracker*>::iterator i = removeList.begin(); i != removeList.end(); i++){
		underConstructionEconomyUnitTrackers.remove(*i);
	}
	
	L("Finished a " << ai->cb->GetUnitDef(unit)->humanName);
	int category = ai->ut->GetCategory(unit);
	found = false;
	if(category != -1)
		for(list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++){
			BuildingTracker *bt = &*i;
			if(bt->unitUnderConstruction == unit)
			{
				updateUnitUnderConstruction(bt);
				found = true;
				allTheBuildingTrackers[category].erase(i);
				break;
			}
		}
	if(!found)
		L("This unit was not in a BuildingTracker!!!!!!!!!");
}

void CEconomyTracker::UnitDestroyed(int unit)
{
	if(trackerOff)
		return;
	assert(ai->cb->GetUnitDef(unit) != NULL);
	int frame=ai->cb->GetCurrentFrame();
	// Move the dead EconomyUnitTracker
	bool found = false;
	for(list<EconomyUnitTracker*>::iterator i = activeEconomyUnitTrackers.begin(); i != activeEconomyUnitTrackers.end(); i++){
		EconomyUnitTracker *bt = *i; // This is bad, but needed
		if(bt->economyUnitId == unit)
		{
			// Move it to the dead list
			assert(bt->alive);
			bt->alive = false;
			bt->dieFrame = frame;
			deadEconomyUnitTrackers.push_back(bt);
			activeEconomyUnitTrackers.remove(bt);
			L("Moved "  << bt->unitDef->humanName << " to deadEconomyUnitTrackers");
			L("It was alive for " << (frame - bt->createFrame) << " frames");
			found = true;
			break;
		}
	}
	if(!found)
		for(list<EconomyUnitTracker*>::iterator i = underConstructionEconomyUnitTrackers.begin(); i != underConstructionEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *bt = *i; // This is bad, but needed
			if(bt->economyUnitId == unit)
			{
				// Move it to the dead list
				assert(bt->alive);
				bt->alive = false;
				bt->dieFrame = frame;
				deadEconomyUnitTrackers.push_back(bt);
				underConstructionEconomyUnitTrackers.remove(bt);
				L("Moved "  << bt->unitDef->humanName << " to deadEconomyUnitTrackers");
				L("It was still under construction");
				found = true;
				break;
			}
		}
	if(!found)
		for(list<EconomyUnitTracker*>::iterator i = newEconomyUnitTrackers.begin(); i != newEconomyUnitTrackers.end(); i++){
			EconomyUnitTracker *bt = *i; // This is bad, but needed
			if(bt->economyUnitId == unit)
			{
				// Move it to the dead list
				assert(bt->alive);
				bt->alive = false;
				bt->dieFrame = frame;
				deadEconomyUnitTrackers.push_back(bt);
				newEconomyUnitTrackers.remove(bt);
				L("Moved "  << bt->unitDef->humanName << " to deadEconomyUnitTrackers");
				L("It was alive for " << (frame - bt->createFrame) << " frames, and never managed to do anything....");
				found = true;
				break;
			}
		}
	
	// If the unit was being built, remove it
	if(ai->cb->UnitBeingBuilt(unit))
	{
		L("Lost a " << ai->cb->GetUnitDef(unit)->humanName);
		int category = ai->ut->GetCategory(unit);
		bool found = false;
		if(category != -1)
			for(list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++){
				BuildingTracker *bt = &*i;
				if(bt->unitUnderConstruction == unit)
				{
					//updateUnitUnderConstruction(bt); // Add this back in ???   then the hp will be negative, and it needs to be fixed...
					found = true;
					allTheBuildingTrackers[category].erase(i);
					break;
				}
			}
		if(!found)
			L("This unit was not in a BuildingTracker!!!!!!!!!");
	}
}

void CEconomyTracker::UnitDamaged(int unit, float damage)
{
	if(trackerOff)
		return;
	if(ai->cb->UnitBeingBuilt(unit))
	{
		L("Damage to " << ai->cb->GetUnitDef(unit)->humanName);
		int category = ai->ut->GetCategory(unit);
		bool found = false;
		if(category != -1)
			for(list<BuildingTracker>::iterator i = allTheBuildingTrackers[category].begin(); i != allTheBuildingTrackers[category].end(); i++){
				BuildingTracker *bt = &*i;
				if(bt->unitUnderConstruction == unit)
				{
					bt->damage += damage;
					bt->hpLastFrame -= damage;
					found = true;
					break;
				}
			}
		if(!found)
			L("This unit was not in a BuildingTracker!!!!!!!!!");
	}
}

