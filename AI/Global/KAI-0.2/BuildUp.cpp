#include "BuildUp.h"
#include "CoverageHandler.h"
#include "creg/STL_List.h"

CR_BIND(CBuildUp ,(NULL))

CR_REG_METADATA(CBuildUp,(
				CR_MEMBER(MexUpgraders),
				CR_MEMBER(totalbuildingcosts),
				CR_MEMBER(totaldefensecosts),

				CR_MEMBER(factorycounter),
				CR_MEMBER(buildercounter),
				CR_MEMBER(storagecounter),

				CR_MEMBER(ai),
				CR_RESERVED(256),
				CR_POSTLOAD(PostLoad)
				));

int factoryBuildupTime;
int econBuildupTime;
int getBestFactoryThatCanBeBuiltTime;
int defenceBuildupTime;
int defenceBuildupCBC_Time;
CBuildUp::CBuildUp(AIClasses* ai)
{
	this->ai = ai;
	factorycounter = -10;
	buildercounter = -3;
	storagecounter = 0;

	if (ai) {
		factoryBuildupTime = ai->math->GetNewTimerGroupNumber("FactoryBuildupTime");
		econBuildupTime = ai->math->GetNewTimerGroupNumber("EconBuildupTime");
		getBestFactoryThatCanBeBuiltTime = ai->math->GetNewTimerGroupNumber("GetBestFactoryThatCanBeBuiltTime");
		defenceBuildupTime = ai->math->GetNewTimerGroupNumber("DefenceBuildupTime");
		defenceBuildupCBC_Time = ai->math->GetNewTimerGroupNumber("Defence CBC time");
	}
}
CBuildUp::~CBuildUp()
{

}

void CBuildUp::PostLoad()
{
	factoryBuildupTime = ai->math->GetNewTimerGroupNumber("FactoryBuildupTime");
	econBuildupTime = ai->math->GetNewTimerGroupNumber("EconBuildupTime");
	getBestFactoryThatCanBeBuiltTime = ai->math->GetNewTimerGroupNumber("GetBestFactoryThatCanBeBuiltTime");
	defenceBuildupTime = ai->math->GetNewTimerGroupNumber("DefenceBuildupTime");
	defenceBuildupCBC_Time = ai->math->GetNewTimerGroupNumber("Defence CBC time");
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
				for(list<int>::iterator b = ai->uh->IdleUnits[CAT_FACTORY].begin(); b != ai->uh->IdleUnits[CAT_FACTORY].end();b++){
					if(*b == i->id && i->supportBuilderTrackers.size()){
						BuilderTracker *builder = i->supportBuilderTrackers.front();
						ai->MyUnits[builder->builderID]->Stop();
//						ai->cb->SendTextMsg("Builder Removed from factory!",0);
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
#define GROUND_WATER_SUBCAT (builderpos.y>2?0:builderpos.y<-1?1:(RANDINT%2))
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
		int side = ai->ut->GetSide(builder);
		int factype = -1;
		float3 builderpos = ai->cb->GetUnitPos(builder);
		const UnitDef* factory;
		for (int a=0;a<6;a++) {
			int r=ai->math->RandInt()%100;
			if (builderpos.y<2) {
				//water
				if (r>=  0 && r< 20) factype=0;
				if (r>= 20 && r< 50) factype=1;
				if (r>= 50 && r<100) factype=2;
			} else {
				//ground
				if (r>=  0 && r< 50) factype=0;
				if (r>= 50 && r< 90) factype=1;
				if (r>= 90 && r<100) factype=2;
			}
			if (!ai->uh->Factories.size() && factype==1) factype=0;
			factory = ai->ut->GetUnitByScore(builder,CAT_FACTORY,factype);
			if (factory) break;
		}
//		if (!ai->ut->ground_factories[side].empty()) {
//			int factoryidx = RANDINT%ai->ut->ground_factories[side].size();
//			factory = ai->ut->unittypearray[ai->ut->ground_factories[side][factoryidx]].def;
//		}
//		const UnitDef* factory = GetBestFactoryThatCanBeBuilt(ai->cb->GetUnitPos(builder)); // This is the global selection. TEST IT!!!
		ai->math->StopTimer(getBestFactoryThatCanBeBuiltTime);
		if(factory == NULL)
		{
			// This builder dont have any factory to make...
			L("This builder dont have any factory to make: " << builderDef->humanName);
			// Hack:
			////assert(false);
		}
		bool builderIsTaken = false;
		ai->math->StartTimer(defenceBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = DefenceBuildup(builder, false, true);
		ai->math->StopTimer(defenceBuildupTime);

		ai->math->StartTimer(econBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = EconBuildup(builder, factory, false);
		ai->math->StopTimer(econBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = MakeStorage(builder, false);
		ai->math->StartTimer(defenceBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = DefenceBuildup(builder, false, false);
		ai->math->StopTimer(defenceBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = AddBuilderToFactory(builder, factory, false);

		ai->math->StartTimer(defenceBuildupTime);
		if(!builderIsTaken)
			builderIsTaken = DefenceBuildup(builder, true, false);
		ai->math->StopTimer(defenceBuildupTime);
		ai->math->StartTimer(econBuildupTime);
		if(!builderIsTaken)
		{
			// This can happen now...
			////assert(builderIsTaken);
			// Try one more economy run:
			EconBuildup(builder, factory, true); // The "true" part is still TODO
		}
		if (!builderIsTaken && !ai->uh->IdleUnits[CAT_BUILDER].empty()) {
			//Move builder to idle queue end
			ai->uh->IdleUnits[CAT_BUILDER].push_back(ai->uh->IdleUnits[CAT_BUILDER].front());
			ai->uh->IdleUnits[CAT_BUILDER].pop_front();
		}
		ai->math->StopTimer(econBuildupTime);
		
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
	if(NumberofidleFactories  && ((ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.4 && ai->cb->GetMetal() > ai->cb->GetMetalStorage() * 0.2)||(RANDINT%1000<5))){
		if (factorycounter>0) factorycounter--;
		L("CBuildUp::FactoryBuildup(): starting Factory Cycle");
		for(int i = 0; i < NumberofidleFactories; i++){
			int producedcat;
			const UnitDef* unitToBuild;
			L("buildercounter: " << buildercounter);
			L("ai->uh->NumIdleUnits(CAT_BUILDER): " << ai->uh->NumIdleUnits(CAT_BUILDER));
			int factory = ai->uh->GetIU(CAT_FACTORY);
			if(buildercounter > 0 || ai->uh->NumIdleUnits(CAT_BUILDER) > 2){
				for (int a=0;a<6;a++) {
					producedcat = CAT_ATTACK;
					int r=ai->math->RandInt()%100;
					if (r>=  0 && r< 40) producedcat=CAT_ATTACK;
					if (r>= 40 && r< 75) producedcat=CAT_ARTILLERY;
					if (r>= 75 && r<100) producedcat=CAT_ASSAULT;
//					if (r>= 80 && r<100) producedcat=CAT_A_ATTACK;
					unitToBuild = ai->ut->GetUnitByScore(factory,producedcat);
					if (unitToBuild) break;
				}
				if (!unitToBuild || ai->math->RandInt()%100<5) {
					producedcat = CAT_A_ATTACK;
					unitToBuild = ai->ut->GetUnitByScore(factory,producedcat);
				}
				if(buildercounter > 0)
					buildercounter--;
				if ((ai->cb->GetEnergy() > ai->cb->GetEnergyStorage()*0.9 && ai->cb->GetMetal() > ai->cb->GetMetalStorage()*0.9)) buildercounter=0;
			}
			else {
				producedcat = CAT_BUILDER;
				unitToBuild = ai->ut->GetUnitByScore(factory,producedcat);
				buildercounter += ATTACKERSPERBUILDER;
/*				// Look at all factorys, and find the best builder they have.
				// Then find the builder that there are least of.
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
*/			
			}
			L("Trying to build unit: " << producedcat);
			if(unitToBuild != NULL)
				ai->MyUnits[factory]->FactoryBuild(unitToBuild);
			else
			{
				// This is bad, the factory can make any units of that type.
				L("This is bad, the factory can make any units of that type: " << producedcat);
			}
		}
	} else {
		L("CBuildUp::FactoryBuildup(): no resources");
	}
}

/*
Returns the factory that is needed most (globaly).
*/
const UnitDef* CBuildUp::GetBestFactoryThatCanBeBuilt(float3 builderPos) {
	builderPos = builderPos;

	// Look at all factories, and find the factories we can make
	list<const UnitDef*> canMakeList;
	for(int side = 0; side < ai->ut->numOfSides; side++) {
		for(unsigned i = 0; i < ai->ut->ground_factories[side].size(); i++)
		{
			const UnitDef* factoryDef = ai->ut->unittypearray[ai->ut->ground_factories[side][i]].def;
			// Now test if the factory can be built by our construction units:
			for(unsigned builtBy = 0; builtBy < ai->ut->unittypearray[factoryDef->id].builtByList.size(); builtBy++)
			{
				if(ai->uh->AllUnitsByType[ai->ut->unittypearray[factoryDef->id].builtByList[builtBy]].size() > 0)
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
				if(ai->uh->AllUnitsByType[ai->ut->unittypearray[factoryDef->id].builtByList[builtBy]].size() > 0)
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
*/
/*
Returns true if the builder was assigned an order.
factory is the current target factory thats needed (globaly).
*/
bool CBuildUp::EconBuildup(int builder, const UnitDef* factory, bool forceUseBuilder) {
	forceUseBuilder = forceUseBuilder;
	float3 builderpos = ai->cb->GetUnitPos(builder);
	bool mexupgrader = false;
	for(list<int>::iterator i = MexUpgraders.begin(); i != MexUpgraders.end();i++){
		L("Mex upgrader number: " << *i);
		if(builder == *i){
			mexupgrader = true;
		}
	}
	bool builderIsUsed = false;
	const UnitDef* mex = ai->ut->GetUnitByScore(builder,CAT_MEX,GROUND_WATER_SUBCAT);
	float3 mexpos = ai->mm->GetNearestMetalSpot(builder,mex);
	if(mexpos != ERRORVECTOR && mex != NULL && mexupgrader){ //if the unit just reclaimed a mex you have to build one and nothing else
		L("trying to build upgraded mex at: " << mexpos.x << ","<< mexpos.z);
		ai->MyUnits[builder]->Build(mexpos,mex);					
		MexUpgraders.remove(builder);
		builderIsUsed = true;
	}
	else if(ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.5
	//&&  ai->uh->metalMaker->AllAreOn() )  // Dont make metal stuff without energy
	&& (((ai->cb->GetMetal() < ai->cb->GetMetalStorage() * 0.8) // More than 40% metal, and we are down to a chance only ?
	&& ((ai->cb->GetMetalIncome() < ai->cb->GetMetalUsage())
	|| (RANDINT%3 == 0 && ai->cb->GetMetalIncome() < ai->cb->GetMetalUsage() * 2.0) // Only more metal if 200% of the metal usage is bigger than the income ?
	|| (!ai->math->MFeasibleConstruction(ai->cb->GetUnitDef(builder),factory) && !ai->uh->Factories.size())))
	|| (ai->cb->GetMetalIncome()<4.0))){	
		if(!ai->uh->Factories.size() || !ai->MyUnits[builder]->ReclaimBest(1)){
			if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MEX))if (mex != NULL){								
				int upgradespot = ai->mm->FindMetalSpotUpgrade(builder,mex);		
				if(upgradespot != -1){
					L("reclaiming unit number: " << upgradespot << " of type " << ai->cb->GetUnitDef(upgradespot)->humanName);
					ai->MyUnits[builder]->Reclaim(upgradespot);
					MexUpgraders.push_back(builder);
					//ai->uh->TaskPlanCreate(builder,ai->cb->GetUnitPos(upgradespot),mex);
					builderIsUsed = true;
				}			
				else if(mexpos != ERRORVECTOR && mex != NULL && (ai->uh->Distance2DToNearestFactory(mexpos.x,mexpos.z)<DEFCBS_RADIUS || mexpos.distance2D(builderpos)<DEFCBS_RADIUS/2 || RANDINT%100<75)){
					L("trying to build mex at: " << mexpos.x << ","<< mexpos.z);
					if (ai->MyUnits[builder]->Build(mexpos,mex)) {
						MexUpgraders.remove(builder);
						builderIsUsed = true;
					}
				}
				else if(ai->mm->NumSpotsFound == 0 && ai->mm->AverageMetal > 5 && mex != NULL){
					if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MEX) || (ai->MyUnits[builder]->Build_ClosestSite(mex,ai->cb->GetUnitPos(builder))))
					//MyUnits[builder].pos());
						builderIsUsed = true;
				}
				else if ((ai->ut->ground_metal_makers->size()||ai->ut->water_metal_makers->size()) && ai->cb->GetEnergyIncome() > ai->cb->GetEnergyUsage() * 1.5 && RANDINT%100<50){
					if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MMAKER)){
						L("Trying to build CAT_MMAKER");
						const UnitDef* def = ai->ut->GetUnitByScore(builder,CAT_MMAKER,GROUND_WATER_SUBCAT);
						if(def == NULL)
							return false;
						ai->MyUnits[builder]->Build_ClosestSite(def,ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
					}
					builderIsUsed = true;
				}
			}else;
			else
				builderIsUsed = true;
		}
		else
			builderIsUsed = true;
	}
	else
		if(ai->cb->GetEnergyIncome() < ai->cb->GetEnergyUsage() * 1.6 
			//|| ai->cb->GetEnergy() < ai->cb->GetEnergyStorage() * 0.7 // Better make sure the bank is full too
			|| !ai->math->EFeasibleConstruction(ai->cb->GetUnitDef(builder),factory)
			|| ai->cb->GetEnergyIncome()<100){	
			if(!ai->uh->BuildTaskAddBuilder(builder,CAT_ENERGY)){
				L("Trying to build CAT_ENERGY");
				const UnitDef* def = ai->ut->GetUnitByScore(builder,CAT_ENERGY,GROUND_WATER_SUBCAT);
				if(def == NULL)
					return false;
				//Find a safe position to build this at
				if (!ai->MyUnits[builder]->Build_ClosestSite(def,ai->cb->GetUnitPos(builder))) return false;//MyUnits[builder].pos());
			}
		builderIsUsed = true;
	}
	
	return builderIsUsed;
}

/*
Returns true if the builder was assigned an order.
*/
bool CBuildUp::DefenceBuildup(int builder, bool forceUseBuilder, bool firstPass) {
	bool builderIsUsed = false;
	if (firstPass && RANDINT%100>25) return false;

	if (!firstPass && !builderIsUsed && (forceUseBuilder || RANDINT%100<25)) {
		const int cats[4]={CAT_RADAR, CAT_SONAR, CAT_R_JAMMER, CAT_S_JAMMER};
		const UnitDef* Unit;
		int cat;
		for (int a=0;a<8;a++) {
			int catidx=RANDINT%4;
			cat=cats[catidx];
			Unit = ai->ut->GetUnitByScore(builder,cat);
			if (Unit) break;
		}
		if (Unit) {
			float heightK=0;
			switch (cat) {
				case CAT_RADAR:heightK= 0.5;break;
				case CAT_SONAR:heightK= 0.1;break;
				case CAT_R_JAMMER:heightK= 0.2;break;
				case CAT_S_JAMMER:heightK= 0.1;break;
			}
			CCoverageHandler* ch=0;
			switch (cat) {
				case CAT_RADAR:ch=ai->radarCoverage;break;
				case CAT_SONAR:ch=ai->sonarCoverage;break;
				case CAT_R_JAMMER:ch=ai->rjammerCoverage;break;
				case CAT_S_JAMMER:ch=ai->sjammerCoverage;break;
			}
			float3 Pos = ai->dm->GetDefensePos(Unit,ai->MyUnits[builder]->pos(),heightK,ch);
			ai->math->StartTimer(defenceBuildupCBC_Time);
			ai->math->TimerStart();
			if(ai->MyUnits[builder]->Build_ClosestSite(Unit,Pos,2, 150)){ // No point in looking far away, as thats the job of GetDefensePos()
				builderIsUsed = true;
			}
			else
			{
				builderIsUsed = false;
			}
			ai->math->StopTimer(defenceBuildupCBC_Time);
			L("Build_ClosestSite time:" << ai->math->TimerSecs());
		}
	}

	if(!builderIsUsed && ai->antinukeCoverage->GetCoverage(ai->cb->GetUnitPos(builder))==0 && (forceUseBuilder || RANDINT%100<20)){
		if (!ai->uh->BuildTaskAddBuilder(builder,CAT_ANTINUKE)) {
			const UnitDef* AntiNuke = ai->ut->GetUnitByScore(builder,CAT_ANTINUKE);
			if(AntiNuke == NULL)
				return false;
			L("trying to build antinuke " << AntiNuke->humanName);
			if (ai->MyUnits[builder]->Build_ClosestSite(AntiNuke,ai->cb->GetUnitPos(builder)))
				builderIsUsed=true;
		} else builderIsUsed = true;
	}

	if(!builderIsUsed && ai->shieldCoverage->GetCoverage(ai->cb->GetUnitPos(builder))==0 && (forceUseBuilder || RANDINT%100<40)){
		if (!ai->uh->BuildTaskAddBuilder(builder,CAT_SHIELD)) {
			const UnitDef* Shield = ai->ut->GetUnitByScore(builder,CAT_SHIELD);
			if(Shield == NULL)
				return false;
			L("trying to build shield " << Shield->humanName);
			if (ai->MyUnits[builder]->Build_ClosestSite(Shield,ai->cb->GetUnitPos(builder)))
				builderIsUsed=true;
		} else builderIsUsed = true;
	}

	int DefenseFactoryRatio = DEFENSEFACTORYRATIO;
	if (firstPass) DefenseFactoryRatio = MINIMUMDEFENSEFACTORYRATIO;
	if (forceUseBuilder) DefenseFactoryRatio = MAXIMUMDEFENSEFACTORYRATIO;
	
	//L("Checking defense building: totaldefensecosts = " <<  totaldefensecosts << " totalbuildingcosts = " << totalbuildingcosts " ai->uh->AllUnitsByCat[CAT_DEFENCE]->size() = " << ai->uh->AllUnitsByCat[CAT_DEFENCE]->size() << " ai->uh->AllUnitsByCat[CAT_FACTORY]->size() = " << ai->uh->AllUnitsByCat[CAT_FACTORY]->size());
	if(!builderIsUsed && ai->uh->AllUnitsByCat[CAT_FACTORY].size() > ai->uh->AllUnitsByCat[CAT_DEFENCE].size() / DefenseFactoryRatio){
	//if(totalbuildingcosts > totaldefensecosts * DEFENSEFACTORYRATIO && ai->uh->AllUnitsByCat[CAT_FACTORY]->size()){
		if(RANDINT%100<50 || !ai->uh->BuildTaskAddBuilder(builder,CAT_DEFENCE)){
			float3 builderpos = ai->cb->GetUnitPos(builder);
			const UnitDef* Defense;
			int defencecat;
			for (int a=0;a<6;a++) {
				int r=ai->math->RandInt()%100;
				defencecat = -1;
				if (builderpos.y<2) {
					//water
					if (r>=  0 && r< 50) defencecat=0;
					if (r>= 50 && r< 70) defencecat=1;
					if (r>= 70 && r<100) defencecat=2;
				} else {
					//ground
					if (r>=  0 && r< 70) defencecat=0;
					if (r>= 70 && r<100) defencecat=1;
				}
				Defense = ai->ut->GetUnitByScore(builder,CAT_DEFENCE,defencecat);
				if (Defense) break;
			}
			L("Trying to build CAT_DEFENCE " << defencecat);
			if(Defense == NULL)
				return false;
			L("trying to build def " << Defense->humanName);
			float heightK = 1.0;
			switch (defencecat) {
				case 0:heightK= 1.0;break;
				case 1:heightK= 0.2;break;
				case 2:heightK=-0.1;break;
			}
			float3 defPos = ai->dm->GetDefensePos(Defense,ai->MyUnits[builder]->pos(),heightK,0);
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

	if(!builderIsUsed && ai->uh->AllUnitsByCat[CAT_NUKE].size()<5 && RANDINT%100<10 
		&& (ai->cb->GetMetal()>=ai->cb->GetMetalStorage()*0.8)
		&& (ai->cb->GetEnergy()>=ai->cb->GetEnergyStorage()*0.8)){
		if (!ai->uh->BuildTaskAddBuilder(builder,CAT_NUKE)) {
			const UnitDef* Nuke = ai->ut->GetUnitByScore(builder,CAT_NUKE);
			if(Nuke == NULL)
				return false;
			L("trying to build nuke " << Nuke->humanName);
			if (ai->MyUnits[builder]->Build_ClosestSite(Nuke,ai->cb->GetUnitPos(builder)))
				builderIsUsed=true;
		} else builderIsUsed = true;
	}
	return builderIsUsed;
}

/*
Returns true if the builder was assigned an order to build a factory or
the builder was added to a factory (assist) or
it helps to make a factory.
*/
bool CBuildUp::AddBuilderToFactory(int builder, const UnitDef* factory, bool forceUseBuilder) {
	if(!ai->uh->BuildTaskAddBuilder(builder,CAT_FACTORY)){
		if((factorycounter<=0) || (ai->uh->GetFactoryHelpersCount()>ai->uh->BuilderTrackers.size()/2) || 
			(ai->uh->NumIdleUnits(CAT_FACTORY)<2 && RANDINT%100<25) || 
			(factory && ai->uh->AllUnitsByType[factory->id].size()==0) || 
			!ai->uh->FactoryBuilderAdd(builder)){
			L("trying to build Factory.");
			if(factory == NULL)
				return false;
			L("Factory: " << factory->humanName);
			// Test if this factory is made by this builder:
			int builderId = ai->cb->GetUnitDef(builder)->id;
			bool canBuildIt = false;
			if(ai->uh->NumIdleUnits(CAT_FACTORY)<4){	
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
				else {
					L("building Factory.");
					ai->MyUnits[builder]->Build_ClosestSite(factory,ai->cb->GetUnitPos(builder));//MyUnits[builder].pos()))
					factorycounter+=FACTORYBUILDTIMEOUT;
					if (factorycounter>20) factorycounter=20;
				}
			}
		}
	}
	return true;
}


/*
Returns true if the builder was assigned an order.
Makes either metal or energy storage
*/
bool CBuildUp::MakeStorage(int builder, bool forceUseBuilder) {
	forceUseBuilder = forceUseBuilder;;

	if (ai->cb->GetEnergyStorage() / (ai->cb->GetEnergyIncome() + 0.01) < STORAGETIME && ai->ut->energy_storages->size() && !storagecounter && ai->uh->AllUnitsByCat[CAT_FACTORY].size()){
		if(!ai->uh->BuildTaskAddBuilder(builder,CAT_ESTOR)){
			L("Trying to build Estorage, storagecounter: " << storagecounter);
			const UnitDef* def = ai->ut->GetUnitByScore(builder,CAT_ESTOR);
			if(def == NULL)
				return false;
			
			ai->MyUnits[builder]->Build_ClosestSite(def,ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
			storagecounter += 10;
		}
		return true;
	}
	
	if (ai->cb->GetMetalStorage() / (ai->cb->GetMetalIncome() + 0.01) < STORAGETIME * 2 && ai->ut->metal_storages->size() && !storagecounter && ai->uh->AllUnitsByCat[CAT_FACTORY].size()){
		if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MSTOR)){
			L("Trying to build CAT_MSTOR, storagecounter: " << storagecounter);
			const UnitDef* def = ai->ut->GetUnitByScore(builder,CAT_MSTOR);
			if(def == NULL)
				return false;
			ai->MyUnits[builder]->Build_ClosestSite(def,ai->MyUnits[builder]->pos());
			storagecounter += 10;
		}
		return true;
	}
	return false;
}
