#include "BuildUp.h"


CBuildUp::CBuildUp(AIClasses* ai) {
	this->ai = ai;

	// these are used to determine how many update
	// cycles should pass before building or unit of
	// this type is (re-)considered for construction
	factoryTimer = 0;
	builderTimer = 1;
	storageTimer = 0;
	nukeSiloTimer = 0;
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

		if (b1 && b2 && b3 && builderTimer > 0 && !(rand() % 3) && frame > 3600) {
			builderTimer--;
		}

		if (storageTimer > 0)
			storageTimer--;

		if (nukeSiloTimer > 0)
			nukeSiloTimer--;
	}
}




void CBuildUp::Buildup() {
	float mIncome = ai->cb->GetMetalIncome();
	float eIncome = ai->cb->GetEnergyIncome();
	float mLevel = ai->cb->GetMetal();
	float eLevel = ai->cb->GetEnergy();
	float mStorage = ai->cb->GetMetalStorage();
	float eStorage = ai->cb->GetEnergyStorage();
	float mUsage = ai->cb->GetMetalUsage();
	float eUsage = ai->cb->GetEnergyUsage();
	bool makersOn = ai->uh->metalMaker->AllAreOn();		// are all our metal makers active?

	bool mLevel50 = (mLevel) < (mStorage * 0.5f);		// is our current metal level less than 50% of our current metal storage capacity?
	bool eLevel50 = (eLevel) > (eStorage * 0.5f);		// is our current energy level more than 50% of our current energy storage capacity?
	bool eLevel80 = (eLevel) > (eStorage * 0.8f);		// is our current energy level more than 80% of our current energy storage capacity?

	bool mStall = (mIncome) < (mUsage * 1.3f);			// are we currently producing less metal than we are currently expending * 1.3?
	bool eStall = (eIncome) < (eUsage * 1.6f);			// are we currently producing less energy than we are currently expending * 1.6?

	// KLOOTNOTE: <MAX_NUKE_SILOS> nuke silos ought to be enough for
	// everybody (assuming we can build them at all in current mod)
	// TODO: use actual metal and energy drain of nuke weapon here
	bool buildNukeSilo =
		(mIncome > 100.0f && eIncome > 6000.0f && mUsage < mIncome && eUsage < eIncome &&
		ai->ut->nuke_silos->size() > 0 && ai->uh->NukeSilos.size() < MAX_NUKE_SILOS);

	if (ai->uh->NumIdleUnits(CAT_BUILDER)) {
		// get first idle (mobile) builder every Update() cycle
		int builder = ai->uh->GetIU(CAT_BUILDER);
		const UnitDef* builderDef = ai->cb->GetUnitDef(builder);
		const UnitDef* factoryDef = ai->ut->GetUnitByScore(builder, CAT_FACTORY);

		// KLOOTNOTE: check unit-limit before building something
		// int factoriesOfTypeDef = ((ai->uh)->AllUnitsByType[factoryDef->id])->size();
		// int factoriesOfTypeMax = factoryDef->maxThisUnit;

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
				builderTimer = 0;
				return;
			}

			else if (buildNukeSilo && nukeSiloTimer <= 0) {
				if (!ai->uh->BuildTaskAddBuilder(builder, CAT_NUKE)) {
					// always favor building one silo at a time rather than
					// many in parallel to prevent a sudden massive resource
					// drain when silos are finished since nukes get queued
					// right away (note that this will result in ALL builders
					// trying to build nothing but silos eventually until the
					// defined limit is reached)
					const UnitDef* building = ai->ut->GetUnitByScore(builder, CAT_NUKE);
					bool r = false;
	
					if (building) {
						r = ai->MyUnits[builder]->Build_ClosestSite(building, ai->cb->GetUnitPos(builder));
					} else {
						FallbackBuild(builder, CAT_NUKE);
					}
	
					if (r)
						nukeSiloTimer += 300;
				}
			}

			else if ((eLevel50 && makersOn) && (mLevel50 || (((RANDINT % 3) == 0) && mStall && eLevel80) || (!factFeasM && factoryTimer <= 0))) {
				if (!ai->MyUnits[builder]->ReclaimBest(1)) {
					const UnitDef* mex = ai->ut->GetUnitByScore(builder, CAT_MEX);
					float3 mexpos = ai->mm->GetNearestMetalSpot(builder, mex);

					bool eOverflow = (eStorage / (eIncome + 0.01) < STORAGETIME);
					bool eExcess = (eIncome > (eUsage * 1.5));
					// number of buildings supported by mod, not how many currently built
					int buildableEStorage = ai->ut->energy_storages->size();
					int buildableMMakers = ai->ut->metal_makers->size();

					if (mexpos != ERRORVECTOR) {
						if (!ai->uh->BuildTaskAddBuilder(builder, CAT_MEX)) {
							// build metal extractor
							ai->MyUnits[builder]->Build(mexpos, mex, -1);
						}
					}
					else if (eOverflow && buildableEStorage > 0 && storageTimer <= 0) {
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
								storageTimer += 90;
						}
					}
					else if (buildableMMakers > 0 && eExcess && ((RANDINT % 10) == 0)) {
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
				bool mOverflow = (mStorage / (mIncome + 0.01)) < (STORAGETIME * 2);
				bool numMStorage = ai->ut->metal_storages->size();
				int numDefenses = ai->uh->AllUnitsByCat[CAT_DEFENCE]->size();
				int numFactories = ai->uh->AllUnitsByCat[CAT_FACTORY]->size();

				// do we have more factories than defense?
				if (numFactories > (numDefenses / DEFENSEFACTORYRATIO)) {
					if (mOverflow && numMStorage > 0 && storageTimer <= 0 && (numFactories > 0)) {
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
								storageTimer += 90;
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
							bool r = false;

							if (factoryDef) {
								r = ai->MyUnits[builder]->Build_ClosestSite(factoryDef, ai->cb->GetUnitPos(builder));
							} else {
								FallbackBuild(builder, CAT_FACTORY);
							}
						}
					}
				}
			}
		}
	}


	bool b1 = ((eLevel > (eStorage * 0.8f)) || (eIncome > 6000.0f && eUsage < eIncome));
	bool b2 = ((mLevel > (mStorage * 0.2f)) || (mIncome > 100.0f && mUsage < mIncome));

	if (b1 && b2)
		FactoryCycle();

	if (buildNukeSilo)
		NukeSiloCycle();
}



void CBuildUp::FactoryCycle(void) {
	int numIdleFactories = ai->uh->NumIdleUnits(CAT_FACTORY);

	for (int i = 0; i < numIdleFactories; i++) {
		// pick the first idle factory we have
		// (note: if we don't give a build order
		// or the order fails somehow then this
		// will keep picking the same one each
		// iteration)
		int producedCat = 0;
		int factoryUnitID = ai->uh->GetIU(CAT_FACTORY);
		bool isHub = (ai->MyUnits[factoryUnitID]->isHub());

		if (isHub) {
			// if we are a hub then we can only construct
			// factories (and some other types of buildings)
			producedCat = CAT_FACTORY;
			builderTimer = 0;
		}
		else {
			if ((builderTimer > 0) || (ai->uh->NumIdleUnits(CAT_BUILDER) > 2)) {
				// if we have more than two idle builders
				// then compensate with an offensive unit
				producedCat = CAT_G_ATTACK;

				if (builderTimer > 0)
					builderTimer--;
			}

			else {
				const UnitDef* leastBuiltBuilder = GetLeastBuiltBuilder();
				const UnitDef* builderUnit = ai->ut->GetUnitByScore(factoryUnitID, CAT_BUILDER);

				if (builderUnit && builderUnit == leastBuiltBuilder) {
					// if this factory makes the builder that we are short of
					producedCat = CAT_BUILDER;
					builderTimer += 4;
				}
				else {
					// build some offensive unit
					producedCat = CAT_G_ATTACK;

					if (builderTimer > 0)
						builderTimer--;
				}
			}
		}

		// get a unit of the category we want this factory to produce
		const UnitDef* udef = ai->ut->GetUnitByScore(factoryUnitID, producedCat);

		if (udef) {
			if (isHub) {
				(ai->MyUnits[factoryUnitID])->HubBuild(udef);
			} else {
				(ai->MyUnits[factoryUnitID])->FactoryBuild(udef);
			}
		}
	}
}


// queue up nukes if we have any silos (note that this
// doesn't cause a resource drain if silo still under
// construction, missiles won't start building until
// silo done)
void CBuildUp::NukeSiloCycle(void) {
	for (std::list<NukeSilo>::iterator i = ai->uh->NukeSilos.begin(); i != ai->uh->NukeSilos.end(); i++) {
		NukeSilo* silo = &*i;
		ai->cb->GetProperty(silo->id, AIVAL_STOCKPILED, &(silo->numNukesReady));
		ai->cb->GetProperty(silo->id, AIVAL_STOCKPILE_QUED, &(silo->numNukesQueued));

		// always keep at least 5 nukes in queue for a rainy day
		if (silo->numNukesQueued < 5)
			ai->MyUnits[silo->id]->NukeSiloBuild();
	}
}




void CBuildUp::FallbackBuild(int builder, int failedCat) {
	// called if an idle builder was selected to construct
	// some category of unit, but builder not capable of
	// constructing anything of that category (note that
	// if AI is swimming in resources then most L1 builders
	// will be used in assisting roles)

	bool b1 = ai->uh->BuildTaskAddBuilder(builder, CAT_MEX);
	bool b2 = false;
	bool b3 = false;
	bool b4 = false;
	float3 builderPos = ai->cb->GetUnitPos(builder);

	if (!b1              ) { b2 = ai->uh->BuildTaskAddBuilder(builder, CAT_ENERGY); }
	if (!b1 && !b2       ) { b3 = ai->uh->BuildTaskAddBuilder(builder, CAT_DEFENCE); }
	if (!b1 && !b2 && !b3) { b4 = ai->uh->BuildTaskAddBuilder(builder, CAT_FACTORY); }

	if (!b1 && !b2 && !b3 && !b4) {
		// failed to add builder to any task, try building something
		const UnitDef* udef1 = ai->ut->GetUnitByScore(builder, CAT_MEX);
		const UnitDef* udef2 = ai->ut->GetUnitByScore(builder, CAT_ENERGY);
		const UnitDef* udef3 = ai->ut->GetUnitByScore(builder, CAT_DEFENCE);
		const UnitDef* udef4 = ai->ut->GetUnitByScore(builder, CAT_FACTORY);

		if (udef2 && failedCat != CAT_ENERGY) {
			ai->MyUnits[builder]->Build_ClosestSite(udef2, builderPos);
			return;
		}
		if (udef3 && failedCat != CAT_DEFENCE) {
			float3 pos = ai->dm->GetDefensePos(udef3, builderPos);
			ai->MyUnits[builder]->Build_ClosestSite(udef3, pos);
			return;
		}
		if (udef4 && failedCat != CAT_FACTORY) {
			ai->MyUnits[builder]->Build_ClosestSite(udef4, builderPos);
			return;
		}
		if (udef1 && failedCat != CAT_MEX) {
			float3 pos = ai->mm->GetNearestMetalSpot(builder, udef1);
			if (pos != ERRORVECTOR)
				ai->MyUnits[builder]->Build(pos, udef1, -1);
			return;
		}
	}

	// unable to assist and unable to build, just patrol
	if (!b1 && !b2 && !b3 && !b4)
		ai->MyUnits[builder]->Patrol(builderPos);
}



// look at all online factories and their best builders,
// then find the best builder that there are least of
const UnitDef* CBuildUp::GetLeastBuiltBuilder(void) {
	int factoryCount = ai->uh->AllUnitsByCat[CAT_FACTORY]->size();
	const UnitDef* leastBuiltBuilder;
	int leastBuiltBuilderCount = 65536;
	assert(factoryCount > 0);

	for (list<int>::iterator j = ai->uh->AllUnitsByCat[CAT_FACTORY]->begin(); j != ai->uh->AllUnitsByCat[CAT_FACTORY]->end(); j++) {
		// get factory unitID
		int factoryToLookAt = *j;

		if (!ai->cb->UnitBeingBuilt(factoryToLookAt)) {
			// if factory isn't still under construction
			const UnitDef* bestBuilder = ai->ut->GetUnitByScore(factoryToLookAt, CAT_BUILDER);

			if (bestBuilder) {
				int bestBuilderCount = ai->uh->AllUnitsByType[bestBuilder->id]->size();

				if (bestBuilderCount < leastBuiltBuilderCount) {
					leastBuiltBuilderCount = bestBuilderCount;
					leastBuiltBuilder = bestBuilder;
				}
			}
		}
	}

	return leastBuiltBuilder;
}
