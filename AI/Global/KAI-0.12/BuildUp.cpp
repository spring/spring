#include "BuildUp.h"


CBuildUp::CBuildUp(AIClasses* ai) {
	this -> ai = ai;
	factorycounter = 0;
	buildercounter = 1;
	storagecounter = 0;
}
CBuildUp::~CBuildUp() {
}


void CBuildUp::Update() {
//	std::cout << "CBuildUp::Update()" << std::endl;
	int frame = ai -> cb -> GetCurrentFrame();

	if (frame % 15 == 0) {
		// L("Frame: " << frame);
		ai -> tm -> Create();
		Buildup();

		// L("Idle builders: " << ai -> uh -> NumIdleUnits(CAT_BUILDER) << " factorycounter: " << factorycounter << " Builder counter: " << buildercounter);
		bool b1 = (ai -> cb -> GetMetal()) > (ai -> cb -> GetMetalStorage() * 0.9);
		bool b2 = (ai -> cb -> GetEnergyIncome()) > (ai -> cb -> GetEnergyUsage() * 1.3);
		bool b3 = (ai -> cb -> GetMetalIncome()) > (ai -> cb -> GetMetalUsage() * 1.3);

		if (b1 && b2 && b3 && buildercounter > 0 && !(rand() % 3) && frame > 3600) {
			buildercounter--;
		}

		if (storagecounter > 0)
			storagecounter--;

		// L("Storage Counter : " << storagecounter);
	}
}



void CBuildUp::Buildup() {
	int NumberofidleBuilders = ai -> uh -> NumIdleUnits(CAT_BUILDER);
//	std::cout << "CBuildUp::BuildUp(), NumberOfIdleBuilders: " << NumberofidleBuilders << std::endl;

	if (NumberofidleBuilders) {
		int builder = ai -> uh -> GetIU(CAT_BUILDER);

		const UnitDef* factory = ai -> ut -> GetUnitByScore(builder, CAT_FACTORY);

		bool b02  = ai -> uh -> metalMaker -> AllAreOn();										// are all our metal makers active?
		bool b04  = (RANDINT % 3) == 0;

		bool b03  = (ai -> cb -> GetMetal()) < (ai -> cb -> GetMetalStorage() * 0.5);			// is our current metal level less than 50% of our current metal storage capacity?
//		bool b03b = (ai -> cb -> GetMetal()) < (ai -> cb -> GetMetalStorage() * 0.4);			// is our current metal level less than 40% of our current metal storage capacity?
		bool b01  = (ai -> cb -> GetEnergy()) > (ai -> cb -> GetEnergyStorage() * 0.5);			// is our current energy level more than 50% of our current energy storage capacity?
		bool b06  = (ai -> cb -> GetEnergy()) > (ai -> cb -> GetEnergyStorage() * 0.8);			// is our current energy level more than 80% of our current energy storage capacity?
//		bool b10  = (ai -> cb -> GetEnergy()) < (ai -> cb -> GetEnergyStorage() * 0.7);			// is our current energy level less than 70% of our current energy storage capacity?

		bool b05  = (ai -> cb -> GetMetalIncome()) < (ai -> cb -> GetMetalUsage() * 1.3);		// are we currently producing less metal than we are currently expending * 1.3?
//		bool b05b = (ai -> cb -> GetMetalIncome()) < (ai -> cb -> GetMetalUsage() * 1.1);		// are we currently producing less metal than we are currently expending * 1.1?
		bool b09  = (ai -> cb -> GetEnergyIncome()) < (ai -> cb -> GetEnergyUsage() * 1.6);		// are we currently producing less energy than we are currently expending * 1.6?

		bool b07  = !ai -> math -> MFeasibleConstruction(ai -> cb -> GetUnitDef(builder), factory);		// is what we want to build feasible metal-wise?
		bool b11  = !ai -> math -> EFeasibleConstruction(ai -> cb -> GetUnitDef(builder), factory);		// is what we want to build feasible energy-wise?
		bool b08  = !factorycounter;

//		bool b12  = ai -> cb -> GetUnitDef(builder) -> isCommander;
//		bool b13  = (ai -> cb -> GetCurrentFrame() > (30 * 60 * 15));
//		bool b14  = ai -> uh -> FactoryBuilderAdd(builder);

		if (ai -> cb -> GetUnitDef(builder) == NULL) {
			// L(" It's dead Jim... ");
			ai -> uh -> UnitDestroyed(builder);
		}

		// else if (b12 && b13 && b14) {
			// Add the commander to help the factory, so that it doesn't wander off
			// (do this at a late stage, just in case)
			// factorycounter += 2; // don't start a new factory in 2 sec or so
			// buildercounter = 0; // make sure it makes one more builder
			// L("Added the commander to help the factory");
		// }

//		else if ((                b03b || (b04 && b05b && b06) || (b11 && b08))) {
		else if ((b01 && b02) && (b03  || (b04 && b05 && b06)  || (b07 && b08))) {
			if (!ai -> MyUnits[builder] -> ReclaimBest(1)) {
				const UnitDef* mex = ai -> ut -> GetUnitByScore(builder, CAT_MEX);
				float3 mexpos = ai -> mm -> GetNearestMetalSpot(builder, mex);

				bool bb1 = (ai -> cb -> GetEnergyStorage() / (ai -> cb -> GetEnergyIncome() + 0.01) < STORAGETIME);
				bool bb2 = ai -> ut -> energy_storages -> size();
				bool bb3 = ai -> ut -> metal_makers -> size();
				bool bb4 = (ai -> cb -> GetEnergyIncome()) > (ai -> cb -> GetEnergyUsage() * 1.5);
				bool bb5 = (RANDINT % 10) == 0;

				if (mexpos != ERRORVECTOR) {
					// L("trying to build mex at: " << mexpos.x << mexpos.z);
					if (!ai -> uh -> BuildTaskAddBuilder(builder, CAT_MEX))
						ai -> MyUnits[builder] -> Build(mexpos, mex);
				}
				else if (bb1 && bb2 && !storagecounter) {
					if (!ai -> uh -> BuildTaskAddBuilder(builder, CAT_ESTOR)) {
						// L("Trying to build Estorage");
						ai -> MyUnits[builder] -> Build_ClosestSite(ai -> ut -> GetUnitByScore(builder, CAT_ESTOR), ai -> cb -> GetUnitPos(builder));
						storagecounter += 90;
					}
				}
				else if (bb3 && bb4 && bb5) {
					if (!ai -> uh -> BuildTaskAddBuilder(builder, CAT_MMAKER))
						// L("Trying to build CAT_MMAKER");
						ai -> MyUnits[builder] -> Build_ClosestSite(ai -> ut -> GetUnitByScore(builder, CAT_MMAKER), ai -> cb -> GetUnitPos(builder));
				}
			}
		}


//		else if (b09 || b10 || b11) {
		else if (b09 || b11) {
			if (!ai -> uh -> BuildTaskAddBuilder(builder, CAT_ENERGY)) {
				// L("Trying to build CAT_ENERGY");
				// Find a safe position to build this at
				ai -> MyUnits[builder] -> Build_ClosestSite(ai -> ut -> GetUnitByScore(builder, CAT_ENERGY), ai -> cb -> GetUnitPos(builder));
			}
		}

		else {
			bool bb6 = (ai -> cb -> GetMetalStorage() / (ai -> cb -> GetMetalIncome() + 0.01)) < (STORAGETIME * 2);
			bool bb7 = ai -> ut -> metal_storages -> size();
			bool bb8 = ai -> uh -> AllUnitsByCat[CAT_FACTORY] -> size();

			if (ai -> uh -> AllUnitsByCat[CAT_FACTORY] -> size() > ai -> uh -> AllUnitsByCat[CAT_DEFENCE] -> size() / DEFENSEFACTORYRATIO) {
				if (bb6 && bb7 && !storagecounter && bb8) {
					if (!ai -> uh -> BuildTaskAddBuilder(builder, CAT_MSTOR)) {
						// L("Trying to build CAT_MSTOR");
						ai -> MyUnits[builder] -> Build_ClosestSite(ai -> ut -> GetUnitByScore(builder, CAT_MSTOR),ai -> MyUnits[builder] -> pos());
						storagecounter += 90;
					}
				}
				else {
					if (!ai -> uh -> BuildTaskAddBuilder(builder, CAT_DEFENCE)) {
						const UnitDef* Defense = ai -> ut -> GetUnitByScore(builder, CAT_DEFENCE);
						// L("trying to build def " << Defense -> humanName);
						// L("Trying to build CAT_DEFENCE");
						if (ai -> MyUnits[builder] -> Build_ClosestSite(Defense, ai -> dm -> GetDefensePos(Defense, ai -> MyUnits[builder] -> pos()), 2)) {
						}
					}
				}
			}
			else {
				if (!ai -> uh -> BuildTaskAddBuilder(builder, CAT_FACTORY)) {
					if (!ai -> uh -> FactoryBuilderAdd(builder)) {
						// L("trying to build Factory: " << factory -> humanName);
						if (ai -> MyUnits[builder] -> Build_ClosestSite(factory, ai -> cb -> GetUnitPos(builder))) {
						}
					}
					else
						factorycounter++;
				}
			}
		}
	}



	bool b1 = (ai -> cb -> GetEnergy()) > (ai -> cb -> GetEnergyStorage() * 0.8);
	bool b2 = (ai -> cb -> GetMetal()) > (ai -> cb -> GetMetalStorage() * 0.2);
	int NumberofidleFactories = ai -> uh -> NumIdleUnits(CAT_FACTORY);

	if (NumberofidleFactories && b1 && b2) {
		// L("starting Factory Cycle");
		for (int i = 0; i < NumberofidleFactories; i++) {
			int producedcat;
			// L("buildercounter: " << buildercounter);
			// L("ai -> uh -> NumIdleUnits(CAT_BUILDER): " << ai -> uh -> NumIdleUnits(CAT_BUILDER));
			int factory = ai -> uh -> GetIU(CAT_FACTORY);
			if ((buildercounter > 0) || (ai -> uh -> NumIdleUnits(CAT_BUILDER) > 2)) {
				producedcat = CAT_G_ATTACK;

				if (buildercounter > 0)
					buildercounter--;
			}

			else {
				// Look at all factorys, and find the best builder they have.
				// Then find the builder that there are least of.
				int factoryCount = ai -> uh -> AllUnitsByCat[CAT_FACTORY] -> size();
				const UnitDef* leastBuiltBuilder;
				int leastBuiltBuilderCount = 50000;
				assert(factoryCount > 0);

				for (list<int>::iterator i = ai -> uh -> AllUnitsByCat[CAT_FACTORY] -> begin(); i != ai -> uh -> AllUnitsByCat[CAT_FACTORY] -> end(); i++) {
					int factoryToLookAt = *i;

					if (!ai -> cb -> UnitBeingBuilt(factoryToLookAt)) {
						const UnitDef* bestBuilder = ai -> ut -> GetUnitByScore(factoryToLookAt, CAT_BUILDER);
						int bestBuilderCount =  ai -> uh -> AllUnitsByType[bestBuilder -> id] -> size();

						if (bestBuilderCount < leastBuiltBuilderCount) {
							leastBuiltBuilderCount = bestBuilderCount;
							leastBuiltBuilder = bestBuilder;
						}
					}
				}

				// L("leastBuiltBuilder: " << leastBuiltBuilder -> humanName << ": " << leastBuiltBuilderCount);
				
				// Find the the builder type this factory makes
				const UnitDef* builder_unit = ai -> ut -> GetUnitByScore(factory, CAT_BUILDER);

				// See if it is the least built builder, if it is then make one.
				if (builder_unit == leastBuiltBuilder) {
					// L("Can build it");
					producedcat = CAT_BUILDER;
					buildercounter += 4;
				}
				else {
					producedcat = CAT_G_ATTACK;

					if (buildercounter > 0)
						buildercounter--;
				}
			}
			
			// L("Trying to build unit: " << producedcat);
			ai -> MyUnits[factory] -> FactoryBuild(ai -> ut -> GetUnitByScore(factory, producedcat));
		}
	}
}



void CBuildUp::EconBuildup(int builder, const UnitDef* factory) {
	builder = builder;
	factory = factory;
}
