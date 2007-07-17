#include "BuildUp.h"


CBuildUp::CBuildUp(AIClasses* ai) {
	this->ai = ai;
	factoryCounter = 0;
	builderCounter = 1;
	storageCounter = 0;
}
CBuildUp::~CBuildUp() {
}


void CBuildUp::Update() {
//	std::cout << "CBuildUp::Update()" << std::endl;
	int frame = ai->cb->GetCurrentFrame();

	if (frame % 15 == 0) {
		ai->tm->Create();
		Buildup();

		bool b1 = (ai->cb->GetMetal()) > (ai->cb->GetMetalStorage() * 0.9);
		bool b2 = (ai->cb->GetEnergyIncome()) > (ai->cb->GetEnergyUsage() * 1.3);
		bool b3 = (ai->cb->GetMetalIncome()) > (ai->cb->GetMetalUsage() * 1.3);

		if (b1 && b2 && b3 && builderCounter > 0 && !(rand() % 3) && frame > 3600) {
			builderCounter--;
		}

		if (storageCounter > 0)
			storageCounter--;
	}
}




void CBuildUp::Buildup() {
	if (ai->uh->NumIdleUnits(CAT_BUILDER)) {
		// get idle (mobile) builder
		int builder = ai->uh->GetIU(CAT_BUILDER);

		const UnitDef* builderDef = ai->cb->GetUnitDef(builder);
		const UnitDef* factoryDef = ai->ut->GetUnitByScore(builder, CAT_FACTORY);

		// KLOOTNOTE: TODO
	//	int factoriesOfTypeDef = ((ai->uh)->AllUnitsByType[factoryDef->id])->size();
	//	int factoriesOfTypeMax = factoryDef->maxThisUnit;

		bool makersOn  = ai->uh->metalMaker->AllAreOn();								// are all our metal makers active?

		bool mLevel50  = (ai->cb->GetMetal()) < (ai->cb->GetMetalStorage() * 0.5);		// is our current metal level less than 50% of our current metal storage capacity?
		bool eLevel50  = (ai->cb->GetEnergy()) > (ai->cb->GetEnergyStorage() * 0.5);	// is our current energy level more than 50% of our current energy storage capacity?
		bool eLevel80  = (ai->cb->GetEnergy()) > (ai->cb->GetEnergyStorage() * 0.8);	// is our current energy level more than 80% of our current energy storage capacity?

		bool mStall  = (ai->cb->GetMetalIncome()) < (ai->cb->GetMetalUsage() * 1.3);	// are we currently producing less metal than we are currently expending * 1.3?
		bool eStall  = (ai->cb->GetEnergyIncome()) < (ai->cb->GetEnergyUsage() * 1.6);	// are we currently producing less energy than we are currently expending * 1.6?

		bool factFeasM = !ai->math->MFeasibleConstruction(builderDef, factoryDef);		// is factory we want to build feasible metal-wise?
		bool factFeasE  = !ai->math->EFeasibleConstruction(builderDef, factoryDef);		// is factory we want to build feasible energy-wise?


		if (builderDef == NULL) {
			// L(" It's dead Jim... ");
			ai->uh->UnitDestroyed(builder);
		}


		else {
			if (builderDef->isCommander && builderDef->canDGun && ai->dgunController->isBusy()) {
				// don't start building solars etc. while dgun-controller is doing stuff
				return;
			}

			/*
			else if (builderDef->isCommander && (ai->cb->GetCurrentFrame() < (30 * 60 * 15)) && ai->uh->FactoryBuilderAdd(builder)) {
				// add commander to factory early in game so it doesn't wander around
				// (FIXME: FactoryBuilderAdd() is too strict here, this never happens)
				factoryCounter += 2;
				builderCounter = 0;
				return;
			}
			*/

			else if ((eLevel50 && makersOn) && (mLevel50 || (((RANDINT % 3) == 0) && mStall && eLevel80) || (factFeasM && factoryCounter <= 0))) {
				if (!ai->MyUnits[builder]->ReclaimBest(1)) {
					const UnitDef* mex = ai->ut->GetUnitByScore(builder, CAT_MEX);
					float3 mexpos = ai->mm->GetNearestMetalSpot(builder, mex);

					bool eOverflow = (ai->cb->GetEnergyStorage() / (ai->cb->GetEnergyIncome() + 0.01) < STORAGETIME);
					int eStorage = ai->ut->energy_storages->size();
					int mStorage = ai->ut->metal_makers->size();
					bool eExcess = (ai->cb->GetEnergyIncome()) > (ai->cb->GetEnergyUsage() * 1.5);

					if (mexpos != ERRORVECTOR) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_MEX))
							// build metal extractor
							ai->MyUnits[builder]->Build(mexpos, mex);
					}
					else if (eOverflow && eStorage > 0 && storageCounter <= 0) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_ESTOR)) {
							// build energy storage
							ai->MyUnits[builder]->Build_ClosestSite(ai->ut->GetUnitByScore(builder, CAT_ESTOR), ai->cb->GetUnitPos(builder));
							storageCounter += 90;
						}
					}
					else if (mStorage > 0 && eExcess && ((RANDINT % 10) == 0)) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_MMAKER))
							// build metal maker
							ai->MyUnits[builder]->Build_ClosestSite(ai->ut->GetUnitByScore(builder, CAT_MMAKER), ai->cb->GetUnitPos(builder));
					}
				}
			}


			else if (eStall || factFeasE) {
				if (!ai->uh->BuildTaskAddBuilder(builder, CAT_ENERGY)) {
					// build energy generator
					ai->MyUnits[builder]->Build_ClosestSite(ai->ut->GetUnitByScore(builder, CAT_ENERGY), ai->cb->GetUnitPos(builder));
				}
			}

			else {
				bool mOverflow = (ai->cb->GetMetalStorage() / (ai->cb->GetMetalIncome() + 0.01)) < (STORAGETIME * 2);
				bool numMStorage = ai->ut->metal_storages->size();
				int numDefenses = ai->uh->AllUnitsByCat[CAT_DEFENCE]->size();
				int numFactories = ai->uh->AllUnitsByCat[CAT_FACTORY]->size();

				// do we have more factories than defense?
				if (numFactories > (numDefenses / DEFENSEFACTORYRATIO)) {
					if (mOverflow && numMStorage > 0 && storageCounter <= 0 && (numFactories > 0)) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_MSTOR)) {
							// build metal storage
							ai->MyUnits[builder]->Build_ClosestSite(ai->ut->GetUnitByScore(builder, CAT_MSTOR),ai->MyUnits[builder]->pos());
							storageCounter += 90;
						}
					}
					else {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_DEFENCE)) {
							// if we can't add this builder to some defense task
							const UnitDef* defenseDef = ai->ut->GetUnitByScore(builder, CAT_DEFENCE);

							// build something in CAT_DEFENCE
							if (ai->MyUnits[builder]->Build_ClosestSite(defenseDef, ai->dm->GetDefensePos(defenseDef, ai->MyUnits[builder]->pos()), 2)) {
							}
						}
					}
				}

				// no, build more factories
				else {
					if (!ai->uh->BuildTaskAddBuilder(builder, CAT_FACTORY)) {
						// if we can't add this builder to some other buildtask
						if (!ai->uh->FactoryBuilderAdd(builder)) {
							// if we can't add this builder to some other factory then construct one with it
							if (ai->MyUnits[builder]->Build_ClosestSite(factoryDef, ai->cb->GetUnitPos(builder))) {
							}
						}
						else
							factoryCounter++;
					}
				}
			}
		}
	}



	bool b1 = (ai->cb->GetEnergy()) > (ai->cb->GetEnergyStorage() * 0.8);
	bool b2 = (ai->cb->GetMetal()) > (ai->cb->GetMetalStorage() * 0.2);
	int numIdleFactories = ai->uh->NumIdleUnits(CAT_FACTORY);

	if (b1 && b2 && numIdleFactories > 0)
		this->FactoryCycle(numIdleFactories);
}




// added by Kloot
void CBuildUp::FactoryCycle(int numIdleFactories) {
	for (int i = 0; i < numIdleFactories; i++) {
		int producedCat;
		int factory = ai->uh->GetIU(CAT_FACTORY);

		if ((builderCounter > 0) || (ai->uh->NumIdleUnits(CAT_BUILDER) > 2)) {
			producedCat = CAT_G_ATTACK;

			if (builderCounter > 0)
				builderCounter--;
		}

		else {
			// look at all factories, and find the best builder they have
			// then find the builder that there are least of
			int factoryCount = ai->uh->AllUnitsByCat[CAT_FACTORY]->size();
			const UnitDef* leastBuiltBuilder;
			int leastBuiltBuilderCount = 50000;
			assert(factoryCount > 0);

			for (list<int>::iterator i = ai->uh->AllUnitsByCat[CAT_FACTORY]->begin(); i != ai->uh->AllUnitsByCat[CAT_FACTORY]->end(); i++) {
				int factoryToLookAt = *i;

				if (!ai->cb->UnitBeingBuilt(factoryToLookAt)) {
					const UnitDef* bestBuilder = ai->ut->GetUnitByScore(factoryToLookAt, CAT_BUILDER);
					int bestBuilderCount =  ai->uh->AllUnitsByType[bestBuilder->id]->size();

					if (bestBuilderCount < leastBuiltBuilderCount) {
						leastBuiltBuilderCount = bestBuilderCount;
						leastBuiltBuilder = bestBuilder;
					}
				}
			}

			// find the builder type this factory makes
			const UnitDef* builder_unit = ai->ut->GetUnitByScore(factory, CAT_BUILDER);

			if (builder_unit == leastBuiltBuilder) {
				// see if it is the least built builder, if so then make one
				producedCat = CAT_BUILDER;
				builderCounter += 4;
			}
			else {
				// build some offensive unit
				producedCat = CAT_G_ATTACK;

				if (builderCounter > 0)
					builderCounter--;
			}
		}


		if ((ai->MyUnits[i])->isHub)
			(ai->MyUnits[factory])->HubBuild(ai->ut->GetUnitByScore(factory, producedCat));
		else
			(ai->MyUnits[factory])->FactoryBuild(ai->ut->GetUnitByScore(factory, producedCat));
	}
}




void CBuildUp::EconBuildup(int builder, const UnitDef* factory) {
	builder = builder;
	factory = factory;
}
