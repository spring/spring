#include "BuildUp.h"

int factoryBuildupTime;
int econBuildupTime;
int getBestFactoryThatCanBeBuiltTime;
int defenceBuildupTime;
int defenceBuildupCBC_Time;
CBuildUp::CBuildUp(AIClasses* ai)
{
	this->ai = ai;
	factorycounter = 0;
	buildercounter = -3;
	storagecounter = 0;

	factoryBuildupTime = ai->math->GetNewTimerGroupNumber("FactoryBuildupTime");
	econBuildupTime = ai->math->GetNewTimerGroupNumber("EconBuildupTime");
	getBestFactoryThatCanBeBuiltTime = ai->math->GetNewTimerGroupNumber("GetBestFactoryThatCanBeBuiltTime");
	defenceBuildupTime = ai->math->GetNewTimerGroupNumber("DefenceBuildupTime");
	defenceBuildupCBC_Time = ai->math->GetNewTimerGroupNumber("Defence CBC time");
}
CBuildUp::~CBuildUp()
{

}
void CBuildUp::Update()
{
	int frame=ai->cb->GetCurrentFrame();
	if(frame % 15 == 0){
		//L("Frame: " << frame);
		//ai->tm->Create();
		MexUpgraders.unique();
		Buildup();
		if(ai->uh->NumIdleUnits(CAT_FACTORY) && frame % 150 == 0){				
			for(list<Factory>::iterator i = ai->uh->Factories.begin(); i != ai->uh->Factories.end();i++){
				for(list<int>::iterator b = ai->uh->IdleUnits[CAT_FACTORY]->begin(); b != ai->uh->IdleUnits[CAT_FACTORY]->end();b++){
					if(*b == i->id && i->supportBuilderTrackers.size()){
						ai->MyUnits[i->supportBuilderTrackers.front()->builderID]->Stop();
						ai->cb->SendTextMsg("Builder Removed from factory!",0);
					}
				}
			}
		}
		if(ai->cb->GetMetal() > ai->cb->GetMetalStorage() * 0.9 && ai->cb->GetEnergyIncome() > ai->cb->GetEnergyUsage() * 1.3 
			&& ai->cb->GetMetalIncome() > ai->cb->GetMetalUsage() * 1.3
			&& buildercounter > 0
			&& !(rand()% 3)
			&& frame > 3600){
			buildercounter--;
			}
		if(storagecounter > 0)
			storagecounter--;
		//L("Storage Counter : " << storagecounter);
	}
	if(frame%59){
		totalbuildingcosts = 0;
		totaldefensecosts = 0;
		for(int i = 1; i <= ai->cb->GetNumUnitDefs(); i++){
			if(ai->ut->unittypearray[i].def->speed == 0){
				if(ai->ut->unittypearray[i].category == CAT_DEFENCE){					
					totaldefensecosts += ai->dc->MyUnitsAll[i].Cost;
				}
				else{
					totalbuildingcosts += ai->dc->MyUnitsAll[i].Cost;
				}
			}
		}
	}
}

void CBuildUp::Buildup()
{
	int NumberofidleBuilders = ai->uh->NumIdleUnits(CAT_BUILDER);
	if(NumberofidleBuilders > 1)
		L("Number of idle builders: " << NumberofidleBuilders);
	if(NumberofidleBuilders){
		int builder = ai->uh->GetIU(CAT_BUILDER);
		L("Starting buildup with builder: " << builder);
		const UnitDef* builderDef = ai->cb->GetUnitDef(builder);
		////assert(false);
		//const UnitDef* factory = ai->ut->GetUnitByScore(builder,CAT_FACTORY); // This must be globaly selected soon, and the right builder must start it
		ai->math->StartTimer(getBestFactoryThatCanBeBuiltTime);
		const UnitDef* factory = GetBestFactoryThatCanBeBuilt(ai->cb->GetUnitPos(builder)); // This is the global selection. TEST IT!!!
		ai->math->StopTimer(getBestFactoryThatCanBeBuiltTime);
		if(factory == NULL)
		{
			// This builder dont have any factory to make...
			L("This builder dont have any factory to make: " << builderDef->humanName);
			// Hack:
			////assert(false);
		}
		
		ai->math->StartTimer(econBuildupTime);
		bool builderIsTaken = EconBuildup(builder, factory, false);
		ai->math->StopTimer(econBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = MakeStorage(builder, false);
		ai->math->StartTimer(defenceBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = DefenceBuildup(builder, false);
		ai->math->StopTimer(defenceBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = AddBuilderToFactory(builder, factory, false);
		if(!builderIsTaken)
		{
			// This can happen now...
			////assert(builderIsTaken);
			// Try one more economy run:
			ai->math->StartTimer(econBuildupTime);
			EconBuildup(builder, factory, true); // The "true" part is still TODO
			ai->math->StopTimer(econBuildupTime);
		}
		
		if(builderDef == NULL)
		{
			// Its dead..
			L(" Its dead... ");
			//assert(false); // Useless now i think, better keep it for some time
			//ai->uh->UnitDestroyed(builder);
		} 
	}
	ai->math->StartTimer(factoryBuildupTime);
	FactoryBuildup();
	ai->math->StopTimer(factoryBuildupTime);
}

void CBuildUp::FactoryBuildup()
{
	//L("starting Factory Cycle");
	int NumberofidleFactories = ai->uh->NumIdleUnits(CAT_FACTORY);
	if(NumberofidleFactories  && ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.8 && ai->cb->GetMetal() > ai->cb->GetMetalStorage() * 0.2){
		L("CBuildUp::FactoryBuildup(): starting Factory Cycle");
		for(int i = 0; i < NumberofidleFactories; i++){
			int producedcat;
			L("buildercounter: " << buildercounter);
			L("ai->uh->NumIdleUnits(CAT_BUILDER): " << ai->uh->NumIdleUnits(CAT_BUILDER));
			int factory = ai->uh->GetIU(CAT_FACTORY);
			if(buildercounter > 0 || ai->uh->NumIdleUnits(CAT_BUILDER) > 2){
				producedcat = CAT_G_ATTACK;
				if(buildercounter > 0)
					buildercounter--;
			}
			else{
				// Look at all factorys, and find the best builder they have.
				// Then find the builder that there are least of.
				int factoryCount = ai->uh->AllUnitsByCat[CAT_FACTORY]->size();
				const UnitDef* leastBuiltBuilder = NULL;
				int leastBuiltBuilderCount = 50000;
				//assert(factoryCount > 0);
				for(list<int>::iterator i = ai->uh->AllUnitsByCat[CAT_FACTORY]->begin(); i != ai->uh->AllUnitsByCat[CAT_FACTORY]->end(); i++)
				{
					int factoryToLookAt = *i;
					if(!ai->cb->UnitBeingBuilt(factoryToLookAt))
					{
						const UnitDef* bestBuilder = ai->ut->GetUnitByScore(factoryToLookAt,CAT_BUILDER);
						if(bestBuilder == NULL)
						{
							// This is bad, the factory dont have any builders.
							// Ignore this factory
							L("BuildUp::FactoryBuildup - This is bad, the factory dont have any builders.");
							continue;
						}
						int bestBuilderCount =  ai->uh->AllUnitsByType[bestBuilder->id]->size();
						if(bestBuilderCount < leastBuiltBuilderCount)
						{
							leastBuiltBuilderCount = bestBuilderCount;
							leastBuiltBuilder = bestBuilder;
						}
					}
				}
				if(leastBuiltBuilder != NULL)
				{
					L("leastBuiltBuilder: " << leastBuiltBuilder->humanName << ": " << leastBuiltBuilderCount);
					
					// Find the the builder type this factory makes:
					const UnitDef* builder_unit = ai->ut->GetUnitByScore(factory,CAT_BUILDER);
					// See if it is the least built builder, if it is then make one.
					if(builder_unit == leastBuiltBuilder)
					{
						L("Can build it");
						producedcat = CAT_BUILDER;
						buildercounter += ATTACKERSPERBUILDER;
					}
					else
					{
						producedcat = CAT_G_ATTACK;
						if(buildercounter > 0)
							buildercounter--;
					}
				}
				else
				{
					// Cant make any builders with the factories we have!
					// Make CAT_G_ATTACK then.
					L("Cant make any builders with the factories we have!");
					 producedcat = CAT_G_ATTACK;
					 if(buildercounter > 0)
							buildercounter--;
				}
			}
			
			L("Trying to build unit: " << producedcat);
			const UnitDef* unitToBuild = ai->ut->GetUnitByScore(factory,producedcat);
			if(unitToBuild != NULL)
				ai->MyUnits[factory]->FactoryBuild(unitToBuild);
			else
			{
				// This is bad, the factory can make any units of that type.
				L("This is bad, the factory can make any units of that type: " << producedcat);
			}
		}
	}
}

/*
Returns the factory that is needed most (globaly).
*/
const UnitDef* CBuildUp::GetBestFactoryThatCanBeBuilt(float3 builderPos)
{
	// Look at all factories, and find the factories we can make
	list<const UnitDef*> canMakeList;
	for(int side = 0; side < ai->ut->numOfSides; side++) {
		for(unsigned i = 0; i < ai->ut->ground_factories[side].size(); i++)
		{
			const UnitDef* factoryDef = ai->ut->unittypearray[ai->ut->ground_factories[side][i]].def;
			// Now test if the factory can be built by our construction units:
			for(unsigned builtBy = 0; builtBy < ai->ut->unittypearray[factoryDef->id].builtByList.size(); builtBy++)
			{
				if(ai->uh->AllUnitsByType[ai->ut->unittypearray[factoryDef->id].builtByList[builtBy]]->size() > 0)
				{
					// Ok, we have a builder that can make it.
					int num = ai->ut->unittypearray[factoryDef->id].builtByList[builtBy];
					const UnitDef* builder = ai->ut->unittypearray[num].def;
					L(builder->humanName << " can make " << factoryDef->humanName);
					canMakeList.push_back(factoryDef);
					break;
				}
			}
		}
	}
	// Now the list of factories is ready, use getScore to select the best:
	float bestScore = -FLT_MAX;
	const UnitDef* bestFactoryDef = NULL;
	for(list<const UnitDef*>::iterator i = canMakeList.begin(); i != canMakeList.end(); i++)
	{
		float score = ai->ut->GetScore(*i);
		//L((*i)->humanName << "'s score: " << score);
		if(score > bestScore)
		{
			bestScore = score;
			bestFactoryDef = *i;
		}
	}
	if(bestFactoryDef != NULL)
	{
		L("Best factory: " << bestFactoryDef->humanName << ", possible num: " << canMakeList.size());
	}
	else
	{
		L("We cant make any factories!");
	}
	return bestFactoryDef;
}


const UnitDef* CBuildUp::GetBestMexThatCanBeBuilt()
{
	// Look at all factories, and find the factories we can make
	list<const UnitDef*> canMakeList;
	for(int side = 0; side < ai->ut->numOfSides; side++) {
		for(unsigned i = 0; i < ai->ut->metal_extractors[side].size(); i++)
		{
			const UnitDef* factoryDef = ai->ut->unittypearray[ai->ut->metal_extractors[side][i]].def;
			// Now test if the factory can be built by our construction units:
			for(unsigned builtBy = 0; builtBy < ai->ut->unittypearray[factoryDef->id].builtByList.size(); builtBy++)
			{
				if(ai->uh->AllUnitsByType[ai->ut->unittypearray[factoryDef->id].builtByList[builtBy]]->size() > 0)
				{
					// Ok, we have a builder that can make it.
					int num = ai->ut->unittypearray[factoryDef->id].builtByList[builtBy];
					const UnitDef* builder = ai->ut->unittypearray[num].def;
					L(builder->humanName << " can make " << factoryDef->humanName);
					canMakeList.push_back(factoryDef);
					break;
				}
			}
		}
	}
	// Now the list of factories is ready, use getScore to select the best:
	float bestScore = -FLT_MAX;
	const UnitDef* bestFactoryDef = NULL;
	for(list<const UnitDef*>::iterator i = canMakeList.begin(); i != canMakeList.end(); i++)
	{
		float score = ai->ut->GetScore(*i);
		//L((*i)->humanName << "'s score: " << score);
		if(score > bestScore)
		{
			bestScore = score;
			bestFactoryDef = *i;
		}
	}
	if(bestFactoryDef != NULL)
	{
		L("Best factory: " << bestFactoryDef->humanName << ", possible num: " << canMakeList.size());
	}
	else
	{
		L("We cant make any factories!");
	}
	return bestFactoryDef;
}

/*
Returns true if the builder was assigned an order.
factory is the current target factory thats needed (globaly).
*/
bool CBuildUp::EconBuildup(int builder, const UnitDef* factory, bool forceUseBuilder)
{
	bool mexupgrader = false;
	for(list<int>::iterator i = MexUpgraders.begin(); i != MexUpgraders.end();i++){
		L("Mex upgrader number: " << *i);
		if(builder == *i){
			mexupgrader = true;
		}
	}
	bool builderIsUsed = false;
	const UnitDef* mex = ai->ut->GetUnitByScore(builder,CAT_MEX);
	float3 mexpos = ai->mm->GetNearestMetalSpot(builder,mex);
	if(mexpos != ERRORVECTOR && mex != NULL && mexupgrader){ //if the unit just reclaimed a mex you have to build one and nothing else
		L("trying to build upgraded mex at: " << mexpos.x << ","<< mexpos.z);
		ai->MyUnits[builder]->Build(mexpos,mex);					
		MexUpgraders.remove(builder);
		builderIsUsed = true;
	}
	else if(ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.5
	//&&  ai->uh->metalMaker->AllAreOn() )  // Dont make metal stuff without energy
	&& (ai->cb->GetMetal() < ai->cb->GetMetalStorage() * 0.5 // More than 40% metal, and we are down to a chance only ?
	|| (RANDINT%3 == 0 && ai->cb->GetMetalIncome() < ai->cb->GetMetalUsage() * 1.3) // Only more metal if 110% of the metal usage is bigger than the income ?
	|| (!ai->math->MFeasibleConstruction(ai->cb->GetUnitDef(builder),factory) && !factorycounter))){	
		if(!ai->MyUnits[builder]->ReclaimBest(1)){
			if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MEX) && mex != NULL){								
				int upgradespot = ai->mm->FindMetalSpotUpgrade(builder,mex);		
				if(upgradespot != -1){
					L("reclaiming unit number: " << upgradespot << " of type " << ai->cb->GetUnitDef(upgradespot)->humanName);
					ai->MyUnits[builder]->Reclaim(upgradespot);
					MexUpgraders.push_back(builder);
					//ai->uh->TaskPlanCreate(builder,ai->cb->GetUnitPos(upgradespot),mex);
					builderIsUsed = true;
				}			
				else if(mexpos != ERRORVECTOR && mex != NULL){
					L("trying to build mex at: " << mexpos.x << ","<< mexpos.z);
					ai->MyUnits[builder]->Build(mexpos,mex);					
					MexUpgraders.remove(builder);
					builderIsUsed = true;
				}
				else if(ai->mm->NumSpotsFound == 0 && ai->mm->AverageMetal > 5 && mex != NULL){
					if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MEX))					
						ai->MyUnits[builder]->Build_ClosestSite(mex,ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
					builderIsUsed = true;
				}
				else if (ai->ut->metal_makers->size() && ai->cb->GetEnergyIncome() > ai->cb->GetEnergyUsage() * 1.5 && RANDINT%10==0){
					if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MMAKER)){
						L("Trying to build CAT_MMAKER");
						const UnitDef* def = ai->ut->GetUnitByScore(builder,CAT_MMAKER);
						if(def == NULL)
							return false;
						ai->MyUnits[builder]->Build_ClosestSite(def,ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
					}
					builderIsUsed = true;
				}
			}
			else
				builderIsUsed = true;
		}
		else
			builderIsUsed = true;
	}
	else
		if(ai->cb->GetEnergyIncome() < ai->cb->GetEnergyUsage() * 1.6 
			//|| ai->cb->GetEnergy() < ai->cb->GetEnergyStorage() * 0.7 // Better make sure the bank is full too
			|| !ai->math->EFeasibleConstruction(ai->cb->GetUnitDef(builder),factory)){	
			if(!ai->uh->BuildTaskAddBuilder(builder,CAT_ENERGY)){
				L("Trying to build CAT_ENERGY");
				const UnitDef* def = ai->ut->GetUnitByScore(builder,CAT_ENERGY);
				if(def == NULL)
					return false;
				//Find a safe position to build this at
				ai->MyUnits[builder]->Build_ClosestSite(def,ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
			}
		builderIsUsed = true;
	}
	
	return builderIsUsed;
}

/*
Returns true if the builder was assigned an order.
*/
bool CBuildUp::DefenceBuildup(int builder, bool forceUseBuilder)
{
	bool builderIsUsed = false;	
	//L("Checking defense building: totaldefensecosts = " <<  totaldefensecosts << " totalbuildingcosts = " << totalbuildingcosts " ai->uh->AllUnitsByCat[CAT_DEFENCE]->size() = " << ai->uh->AllUnitsByCat[CAT_DEFENCE]->size() << " ai->uh->AllUnitsByCat[CAT_FACTORY]->size() = " << ai->uh->AllUnitsByCat[CAT_FACTORY]->size());
	if(ai->uh->AllUnitsByCat[CAT_FACTORY]->size() > ai->uh->AllUnitsByCat[CAT_DEFENCE]->size() / DEFENSEFACTORYRATIO){
	//if(totalbuildingcosts > totaldefensecosts * DEFENSEFACTORYRATIO && ai->uh->AllUnitsByCat[CAT_FACTORY]->size()){
		if(!ai->uh->BuildTaskAddBuilder(builder,CAT_DEFENCE)){
			const UnitDef* Defense = ai->ut->GetUnitByScore(builder,CAT_DEFENCE);
			L("Trying to build CAT_DEFENCE");
			if(Defense == NULL)
				return false;
			L("trying to build def " << Defense->humanName);
			float3 defPos = ai->dm->GetDefensePos(Defense,ai->MyUnits[builder]->pos());
			
			
			ai->math->StartTimer(defenceBuildupCBC_Time);
			ai->math->TimerStart();
			if(ai->MyUnits[builder]->Build_ClosestSite(Defense,defPos,2, 150)){ // No point in looking far away, as thats the job of GetDefensePos()
				builderIsUsed = true;
			}
			else
			{
				builderIsUsed = false;
			}
			ai->math->StopTimer(defenceBuildupCBC_Time);
			L("Build_ClosestSite time:" << ai->math->TimerSecs());
		}
		else
			builderIsUsed = true;
	}
	
	return builderIsUsed;
}

/*
Returns true if the builder was assigned an order to build a factory or
the builder was added to a factory (assist) or
it helps to make a factory.
*/
bool CBuildUp::AddBuilderToFactory(int builder, const UnitDef* factory, bool forceUseBuilder)
{
	if(!ai->uh->BuildTaskAddBuilder(builder,CAT_FACTORY)){
		if(!ai->uh->FactoryBuilderAdd(builder)){
			L("trying to build Factory.");
			if(factory == NULL)
				return false;
			L("Factory: " << factory->humanName);
			// Test if this factory is made by this builder:
			int builderId = ai->cb->GetUnitDef(builder)->id;
			bool canBuildIt = false;
			if(!ai->uh->NumIdleUnits(CAT_FACTORY)){	
				for(unsigned builtBy = 0; builtBy < ai->ut->unittypearray[factory->id].builtByList.size(); builtBy++)
				{
					if(ai->ut->unittypearray[factory->id].builtByList[builtBy] == builderId)
					{
						// Ok, the builder can make it.
						canBuildIt = true;
						break;
					}
				}
				if(!canBuildIt)
				{
					L("This builder cant make this factory. Builder: " << ai->cb->GetUnitDef(builder)->humanName);
					return false;
				}			
				else
					ai->MyUnits[builder]->Build_ClosestSite(factory,ai->cb->GetUnitPos(builder));//MyUnits[builder].pos()))
			}
		}
		else
			factorycounter++;
	}
	return true;
}


/*
Returns true if the builder was assigned an order.
Makes either metal or energy storage
*/
bool CBuildUp::MakeStorage(int builder, bool forceUseBuilder)
{
	if (ai->cb->GetEnergyStorage() / (ai->cb->GetEnergyIncome() + 0.01) < STORAGETIME && ai->ut->energy_storages->size() && !storagecounter && ai->uh->AllUnitsByCat[CAT_FACTORY]->size()){
		if(!ai->uh->BuildTaskAddBuilder(builder,CAT_ESTOR)){
			L("Trying to build Estorage, storagecounter: " << storagecounter);
			const UnitDef* def = ai->ut->GetUnitByScore(builder,CAT_ESTOR);
			if(def == NULL)
				return false;
			
			ai->MyUnits[builder]->Build_ClosestSite(def,ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
			storagecounter += 90;
		}
		return true;
	}
	
	if (ai->cb->GetMetalStorage() / (ai->cb->GetMetalIncome() + 0.01) < STORAGETIME * 2 && ai->ut->metal_storages->size() && !storagecounter && ai->uh->AllUnitsByCat[CAT_FACTORY]->size()){
		if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MSTOR)){
			L("Trying to build CAT_MSTOR, storagecounter: " << storagecounter);
			const UnitDef* def = ai->ut->GetUnitByScore(builder,CAT_MSTOR);
			if(def == NULL)
				return false;
			ai->MyUnits[builder]->Build_ClosestSite(def,ai->MyUnits[builder]->pos());
			storagecounter += 90;
		}
		return true;
	}
	return false;
}
