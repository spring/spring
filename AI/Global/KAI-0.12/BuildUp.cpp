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
	int frame = ai->cb->GetCurrentFrame();

	if (frame % 15 == 0) {
		// update current threat map
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

		// KLOOTNOTE: check unit-limit before building something
		// int factoriesOfTypeDef = ((ai->uh)->AllUnitsByType[factoryDef->id])->size();
		// int factoriesOfTypeMax = factoryDef->maxThisUnit;

		float mIncome = ai->cb->GetMetalIncome();
		float eIncome = ai->cb->GetEnergyIncome();
		bool makersOn = ai->uh->metalMaker->AllAreOn();									// are all our metal makers active?

		bool mLevel50 = (ai->cb->GetMetal()) < (ai->cb->GetMetalStorage() * 0.5);		// is our current metal level less than 50% of our current metal storage capacity?
		bool eLevel50 = (ai->cb->GetEnergy()) > (ai->cb->GetEnergyStorage() * 0.5);		// is our current energy level more than 50% of our current energy storage capacity?
		bool eLevel80 = (ai->cb->GetEnergy()) > (ai->cb->GetEnergyStorage() * 0.8);		// is our current energy level more than 80% of our current energy storage capacity?

		bool mStall = (mIncome) < (ai->cb->GetMetalUsage() * 1.3);						// are we currently producing less metal than we are currently expending * 1.3?
		bool eStall = (eIncome) < (ai->cb->GetEnergyUsage() * 1.6);						// are we currently producing less energy than we are currently expending * 1.6?

		// KLOOTNOTE: prevent NPE if this builder cannot build any factories
		bool factFeasM = (factoryDef? ai->math->MFeasibleConstruction(builderDef, factoryDef): true);
		bool factFeasE = (factoryDef? ai->math->EFeasibleConstruction(builderDef, factoryDef): true);


		if (builderDef == NULL) {
			ai->uh->UnitDestroyed(builder);
		}

		else {
			if (builderDef->isCommander && builderDef->canDGun && ai->dgunController->isBusy()) {
				// don't start building solars etc. while dgun-controller is doing stuff
				return;
			}

			else if ((mIncome > 20.0f && eIncome > 250.0f) && builderDef->isCommander && ai->uh->FactoryBuilderAdd(builder)) {
				// add commander to factory so it doesn't wander around too much (works best if
				// AI given bonus, otherwise initial expansion still mostly done by commander)
				factoryCounter += 2;
				builderCounter = 0;
				return;
			}

			else if ((eLevel50 && makersOn) && (mLevel50 || (((RANDINT % 3) == 0) && mStall && eLevel80) || (!factFeasM && factoryCounter <= 0))) {
				if (!ai->MyUnits[builder]->ReclaimBest(1)) {
					const UnitDef* mex = ai->ut->GetUnitByScore(builder, CAT_MEX);
					float3 mexpos = ai->mm->GetNearestMetalSpot(builder, mex);

					bool eOverflow = (ai->cb->GetEnergyStorage() / (eIncome + 0.01) < STORAGETIME);
					int eStorage = ai->ut->energy_storages->size();
					int mStorage = ai->ut->metal_makers->size();
					bool eExcess = (eIncome) > (ai->cb->GetEnergyUsage() * 1.5);

					if (mexpos != ERRORVECTOR) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_MEX)) {
							// build metal extractor
							ai->MyUnits[builder]->Build(mexpos, mex, -1);
						}
					}
					else if (eOverflow && eStorage > 0 && storageCounter <= 0) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_ESTOR)) {
							// build energy storage
							const UnitDef* building = ai->ut->GetUnitByScore(builder, CAT_ESTOR);
							bool r = false;

							if (building) {
								r = ai->MyUnits[builder]->Build_ClosestSite(building, ai->cb->GetUnitPos(builder));
							} else {
								FallbackBuild(builder, CAT_ESTOR);
							}

							if (r)
								storageCounter += 90;
						}
					}
					else if (mStorage > 0 && eExcess && ((RANDINT % 10) == 0)) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_MMAKER)) {
							// build metal maker
							const UnitDef* building = ai->ut->GetUnitByScore(builder, CAT_MMAKER);
							bool r = false;

							if (building) {
								r = ai->MyUnits[builder]->Build_ClosestSite(building, ai->cb->GetUnitPos(builder));
							} else {
								FallbackBuild(builder, CAT_MMAKER);
							}
						}
					}
				}
			}


			else if (eStall || !factFeasE) {
				if (!ai->uh->BuildTaskAddBuilder(builder, CAT_ENERGY)) {
					// build energy generator
					const UnitDef* building = ai->ut->GetUnitByScore(builder, CAT_ENERGY);
					bool r = false;

					if (building) {
						r = ai->MyUnits[builder]->Build_ClosestSite(building, ai->cb->GetUnitPos(builder));
					} else {
						FallbackBuild(builder, CAT_ENERGY);
					}
				}
			}

			else {
				bool mOverflow = (ai->cb->GetMetalStorage() / (mIncome + 0.01)) < (STORAGETIME * 2);
				bool numMStorage = ai->ut->metal_storages->size();
				int numDefenses = ai->uh->AllUnitsByCat[CAT_DEFENCE]->size();
				int numFactories = ai->uh->AllUnitsByCat[CAT_FACTORY]->size();

				// do we have more factories than defense?
				if (numFactories > (numDefenses / DEFENSEFACTORYRATIO)) {
					if (mOverflow && numMStorage > 0 && storageCounter <= 0 && (numFactories > 0)) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_MSTOR)) {
							// build metal storage
							const UnitDef* building = ai->ut->GetUnitByScore(builder, CAT_MSTOR);
							bool r = false;

							if (building) {
								r = ai->MyUnits[builder]->Build_ClosestSite(building, ai->MyUnits[builder]->pos());
							} else {
								FallbackBuild(builder, CAT_MSTOR);
							}

							if (r)
								storageCounter += 90;
						}
					}
					else {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_DEFENCE)) {
							// if we can't add this builder to some defense
							// task then build something in CAT_DEFENCE
							const UnitDef* building = ai->ut->GetUnitByScore(builder, CAT_DEFENCE);
							float3 buildPos = ai->dm->GetDefensePos(building, ai->MyUnits[builder]->pos());
							bool r = false;

							if (building) {
								r = ai->MyUnits[builder]->Build_ClosestSite(building, buildPos, 2);
							} else {
								FallbackBuild(builder, CAT_DEFENCE);
							}
						}
					}
				}

				// no, build more factories
				else {
					if (!ai->uh->BuildTaskAddBuilder(builder, CAT_FACTORY)) {
						// if we can't add this builder to some other buildtask
						if (!ai->uh->FactoryBuilderAdd(builder)) {
							// if we can't add this builder to some
							// other factory then construct new one
						//	const UnitDef* building = ai->ut->GetUnitByScore(builder, CAT_FACTORY);
							bool r = false;

						//	if (building) {
						//		r = ai->MyUnits[builder]->Build_ClosestSite(building, ai->cb->GetUnitPos(builder));
							if (factoryDef) {
								r = ai->MyUnits[builder]->Build_ClosestSite(factoryDef, ai->cb->GetUnitPos(builder));
							} else {
								FallbackBuild(builder, CAT_FACTORY);
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




void CBuildUp::FactoryCycle(int numIdleFactories) {
	for (int i = 0; i < numIdleFactories; i++) {
		int producedCat;
		int factoryUnitID = ai->uh->GetIU(CAT_FACTORY);

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
				// get factory unitID
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
			const UnitDef* builderUnit = ai->ut->GetUnitByScore(factoryUnitID, CAT_BUILDER);

			if (builderUnit == leastBuiltBuilder) {
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

		if ((ai->MyUnits[factoryUnitID])->isHub()) {
			(ai->MyUnits[factoryUnitID])->HubBuild(ai->ut->GetUnitByScore(factoryUnitID, producedCat));
		} else {
			(ai->MyUnits[factoryUnitID])->FactoryBuild(ai->ut->GetUnitByScore(factoryUnitID, producedCat));
		}
	}
}




void CBuildUp::FallbackBuild(int builder, int failedCat) {
	// called if an idle builder was selected to construct
	// some category of unit, but builder not capable of
	// constructing anything of that category

	bool b1 = ai->uh->BuildTaskAddBuilder(builder, CAT_MEX);
	bool b2 = false;
	bool b3 = false;
	bool b4 = false;
	float3 builderPos = ai->cb->GetUnitPos(builder);
	const UnitDef* udef1 = ai->ut->GetUnitByScore(builder, CAT_MEX);
	const UnitDef* udef2 = ai->ut->GetUnitByScore(builder, CAT_ENERGY);
	const UnitDef* udef3 = ai->ut->GetUnitByScore(builder, CAT_DEFENCE);

	if (!b1) { b2 = ai->uh->BuildTaskAddBuilder(builder, CAT_ENERGY); }
	if (!b2) { b3 = ai->uh->BuildTaskAddBuilder(builder, CAT_DEFENCE); }
	if (!b3) { b4 = ai->uh->BuildTaskAddBuilder(builder, CAT_FACTORY); }
/*
	if (!b2 && !b3 && !b4) {
		if (udef2 && failedCat != CAT_ENERGY) {
			ai->MyUnits[builder]->Build_ClosestSite(udef2, builderPos);
			return;
		}
		if (udef3 && failedCat != CAT_DEFENCE) {
			float3 pos = ai->dm->GetDefensePos(udef3, builderPos);
			ai->MyUnits[builder]->Build_ClosestSite(udef3, pos);
			return;
		}
		if (udef1 && failedCat != CAT_MEX) {
			float3 pos = ai->mm->GetNearestMetalSpot(builder, udef1);
			if (pos != ERRORVECTOR)
				ai->MyUnits[builder]->Build(pos, udef1, -1);
			return;
		}
	}
*/
	// unable to assist and unable to build, just patrol
	if (!b1 && !b2 && !b3 && !b4)
		ai->MyUnits[builder]->Patrol(builderPos);
}
