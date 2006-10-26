#include "BuildUp.h"


CBuildUp::CBuildUp(AIClasses* ai)
{
	this->ai = ai;
	factorycounter = 0;
	buildercounter = 1;
	storagecounter = 0;
}
CBuildUp::~CBuildUp()
{

}
void CBuildUp::Update()
{
	int frame=ai->cb->GetCurrentFrame();
	if(frame % 15 == 0){
		////L("Frame: " << frame);
		ai->tm->Create();
		Buildup();			
		////L("Idle builders: " << ai->uh->NumIdleUnits(CAT_BUILDER) << " factorycounter: " << factorycounter << " Builder counter: " << buildercounter);
		if(ai->cb->GetMetal() > ai->cb->GetMetalStorage() * 0.9 && ai->cb->GetEnergyIncome() > ai->cb->GetEnergyUsage() * 1.3 
			&& ai->cb->GetMetalIncome() > ai->cb->GetMetalUsage() * 1.3
			&& buildercounter > 0
			&& !(rand()% 3)
			&& frame > 3600){
			buildercounter--;
			}
		if(storagecounter > 0)
			storagecounter--;
		////L("Storage Counter : " << storagecounter);
	}
}

void CBuildUp::Buildup()
{
	int NumberofidleBuilders = ai->uh->NumIdleUnits(CAT_BUILDER);
	if(NumberofidleBuilders > 1)
		//L("Number of idle builders: " << NumberofidleBuilders);
	if(NumberofidleBuilders){
	//	for(int i = 0; i < NumberofidleBuilders; i++){
			int builder = ai->uh->GetIU(CAT_BUILDER);
			
			
			const UnitDef* builderDef = ai->cb->GetUnitDef(builder);
			//assert(false);
			const UnitDef* factory = ai->ut->GetUnitByScore(builder,CAT_FACTORY);
			if(ai->cb->GetUnitDef(builder) == NULL)
			{
				// Its dead..
				//L(" Its dead... ");
				ai->uh->UnitDestroyed(builder);
			} 
			//else if(ai->cb->GetUnitDef(builder)->isCommander && (ai->cb->GetCurrentFrame() > 30*60*15) && ai->uh->FactoryBuilderAdd(builder))
			//{
				// Add the commander to help the factory, so that it dont wander off. (do this at a late stage, just in case)
				// And... speed the building of stuff, like new builders
				//int frame = ai->cb->GetCurrentFrame();
				//assert(false);
				//factorycounter += 2; // Dont start a new factory in 2 sec or so
				//buildercounter = 0; // Make sure it makes one more builder
				////L("Added the commander to help the factory, so that it dont wander off");
			//}
			else
//			if(ai->cb->GetMetal() < ai->cb->GetMetalStorage() * 0.4 
//			|| (RANDINT%3 == 0 && ai->cb->GetMetalIncome() < ai->cb->GetMetalUsage() * 1.1
//			&& ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.8) 
//			|| (!ai->math->MFeasibleConstruction(ai->cb->GetUnitDef(builder),factory) && !factorycounter)){
			if( (ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.5
				&&  ai->uh->metalMaker->AllAreOn() ) &&  // Dont make metal stuff without energy
					(ai->cb->GetMetal() < ai->cb->GetMetalStorage() * 0.5 // More than 40% metal, and we are down to a chance only ?
					|| (RANDINT%3 == 0 && ai->cb->GetMetalIncome() < ai->cb->GetMetalUsage() * 1.3 // Only more metal if 110% of the metal usage is bigger than the income ?
					&& ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.8)  // Dont make metal stuff without energy (80% full)
					|| (!ai->math->MFeasibleConstruction(ai->cb->GetUnitDef(builder),factory) && !factorycounter))){				
				if(!ai->MyUnits[builder]->ReclaimBest(1)){
					const UnitDef* mex = ai->ut->GetUnitByScore(builder,CAT_MEX);
					float3 mexpos = ai->mm->GetNearestMetalSpot(builder,mex);
					if(mexpos != ERRORVECTOR){
						//L("trying to build mex at: " << mexpos.x << mexpos.z);
						if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MEX))
							ai->MyUnits[builder]->Build(mexpos,mex);
					}
					else if (ai->cb->GetEnergyStorage() / (ai->cb->GetEnergyIncome() + 0.01) < STORAGETIME && ai->ut->energy_storages->size() && !storagecounter){
						if(!ai->uh->BuildTaskAddBuilder(builder,CAT_ESTOR)){
							//L("Trying to build Estorage");
							ai->MyUnits[builder]->Build_ClosestSite(ai->ut->GetUnitByScore(builder,CAT_ESTOR),ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
							storagecounter += 90;
						}
					}
					else if (ai->ut->metal_makers->size() && ai->cb->GetEnergyIncome() > ai->cb->GetEnergyUsage() * 1.5 && RANDINT%10==0){
						if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MMAKER))
							//L("Trying to build CAT_MMAKER");
							ai->MyUnits[builder]->Build_ClosestSite(ai->ut->GetUnitByScore(builder,CAT_MMAKER),ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
					}
				}
			}
			else if(ai->cb->GetEnergyIncome() < ai->cb->GetEnergyUsage() * 1.6 
					//|| ai->cb->GetEnergy() < ai->cb->GetEnergyStorage() * 0.7 // Better make sure the bank is full too
					|| !ai->math->EFeasibleConstruction(ai->cb->GetUnitDef(builder),factory)){	
				if(!ai->uh->BuildTaskAddBuilder(builder,CAT_ENERGY)){
					//L("Trying to build CAT_ENERGY");
					//Find a safe position to build this at
					ai->MyUnits[builder]->Build_ClosestSite(ai->ut->GetUnitByScore(builder,CAT_ENERGY),ai->cb->GetUnitPos(builder));//MyUnits[builder].pos());
				}
			}
			else{
				int separation = DEFCBS_SEPARATION;
				float radius = DEFCBS_RADIUS;
				if(ai->uh->AllUnitsByCat[CAT_FACTORY]->size() > ai->uh->AllUnitsByCat[CAT_DEFENCE]->size() / DEFENSEFACTORYRATIO){
					if (ai->cb->GetMetalStorage() / (ai->cb->GetMetalIncome() + 0.01) < STORAGETIME * 2 && ai->ut->metal_storages->size() && !storagecounter && ai->uh->AllUnitsByCat[CAT_FACTORY]->size()){
						if(!ai->uh->BuildTaskAddBuilder(builder,CAT_MSTOR)){
							//L("Trying to build CAT_MSTOR");
							ai->MyUnits[builder]->Build_ClosestSite(ai->ut->GetUnitByScore(builder,CAT_MSTOR),ai->MyUnits[builder]->pos());
							storagecounter += 90;
						}
					}
					else{						
						if(!ai->uh->BuildTaskAddBuilder(builder,CAT_DEFENCE)){
							const UnitDef* Defense = ai->ut->GetUnitByScore(builder,CAT_DEFENCE);
							//L("trying to build def " << Defense->humanName);
							//L("Trying to build CAT_DEFENCE");
							if(ai->MyUnits[builder]->Build_ClosestSite(Defense,ai->dm->GetDefensePos(Defense,ai->MyUnits[builder]->pos()),2)){
							}
						}
					}
				}
				else{
					
					if(!ai->uh->BuildTaskAddBuilder(builder,CAT_FACTORY)){
						if(!ai->uh->FactoryBuilderAdd(builder)){
							//L("trying to build Factory: " << factory->humanName);
							if(ai->MyUnits[builder]->Build_ClosestSite(factory,ai->cb->GetUnitPos(builder))){//MyUnits[builder].pos())){
							}
						}
						else
							factorycounter++;
					}
				}
			}
		}	
	//}
	////L("starting Factory Cycle");
	int NumberofidleFactories = ai->uh->NumIdleUnits(CAT_FACTORY);
	if(NumberofidleFactories  && ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.8 && ai->cb->GetMetal() > ai->cb->GetMetalStorage() * 0.2){
		//L("starting Factory Cycle");
		for(int i = 0; i < NumberofidleFactories; i++){
			int producedcat;
			//L("buildercounter: " << buildercounter);
			//L("ai->uh->NumIdleUnits(CAT_BUILDER): " << ai->uh->NumIdleUnits(CAT_BUILDER));
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
				const UnitDef* leastBuiltBuilder;
				int leastBuiltBuilderCount = 50000;
				assert(factoryCount > 0);
				for(list<int>::iterator i = ai->uh->AllUnitsByCat[CAT_FACTORY]->begin(); i != ai->uh->AllUnitsByCat[CAT_FACTORY]->end(); i++)
				{
					int factoryToLookAt = *i;
					if(!ai->cb->UnitBeingBuilt(factoryToLookAt))
					{
						const UnitDef* bestBuilder = ai->ut->GetUnitByScore(factoryToLookAt,CAT_BUILDER);
						int bestBuilderCount =  ai->uh->AllUnitsByType[bestBuilder->id]->size();
						if(bestBuilderCount < leastBuiltBuilderCount)
						{
							leastBuiltBuilderCount = bestBuilderCount;
							leastBuiltBuilder = bestBuilder;
						}
					}
				}
				//L("leastBuiltBuilder: " << leastBuiltBuilder->humanName << ": " << leastBuiltBuilderCount);
				
				// Find the the builder type this factory makes:
				const UnitDef* builder_unit = ai->ut->GetUnitByScore(factory,CAT_BUILDER);
				// See if it is the least built builder, if it is then make one.
				if(builder_unit == leastBuiltBuilder)
				{
					//L("Can build it");
					producedcat = CAT_BUILDER;
					buildercounter +=4;
				}
				else
				{
					producedcat = CAT_G_ATTACK;
					if(buildercounter > 0)
						buildercounter--;
				}
			}
			
			//L("Trying to build unit: " << producedcat);
			ai->MyUnits[factory]->FactoryBuild(ai->ut->GetUnitByScore(factory,producedcat));
		}
	}
}

void CBuildUp::EconBuildup(int builder, const UnitDef* factory)
{

}
