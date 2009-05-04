#include <sstream>

#include "IncCREG.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

CR_BIND(CBuildUp, (NULL));
CR_REG_METADATA(CBuildUp, (
	CR_MEMBER(ai),
	CR_MEMBER(factoryTimer),
	CR_MEMBER(builderTimer),
	CR_MEMBER(storageTimer),
	CR_MEMBER(nukeSiloTimer),
	CR_RESERVED(16)
));

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


void CBuildUp::Update(int frame) {
	if (frame % 15 == 0) {
		// update current threat map
		ai->tm->Create();
		ai->uh->UpdateUpgradeTasks(frame);

		GetEconState(&econState);
		Buildup(frame);

		// KLOOTNOTE: b1 will be false if we have
		// large amounts of metal storage, so the
		// multiplier <m> must not be a constant
		// (more or less assumes starting storage
		// capacity of 1000)
		float m = 900.0f / (ai->cb->GetMetalStorage());
		bool b1 = (ai->cb->GetMetal()) > (ai->cb->GetMetalStorage() * m);
		bool b2 = (ai->cb->GetEnergyIncome()) > (ai->cb->GetEnergyUsage() * 1.3f);
		bool b3 = (ai->cb->GetMetalIncome()) > (ai->cb->GetMetalUsage() * 1.3f);

		if ((b1 && b2 && b3) && builderTimer > 0 && !(rand() % 3) && frame > 3600) {
			// decrease builderTime iif we have more metal
			// than 90% of our metal storage capacity and
			// we are generating more than 130% the amount
			// of M and E used (meaning we have excess M
			// and are over-producing M and E)
			builderTimer--;
			factoryTimer--;
		}

		if (storageTimer > 0) { storageTimer--; }
		if (nukeSiloTimer > 0) { nukeSiloTimer--; }
	}
}



void CBuildUp::GetEconState(EconState* es) const {
	es->nIdleBuilders = ai->uh->NumIdleUnits(CAT_BUILDER);
	// get the first idle (mobile) builder
	es->builderID  = (es->nIdleBuilders > 0)? ai->uh->GetIU(CAT_BUILDER): -1;
	es->builderDef = (es->nIdleBuilders > 0)? ai->cb->GetUnitDef(es->builderID): NULL;
	es->factoryDef = (es->nIdleBuilders > 0)? ai->ut->GetUnitByScore(es->builderID, CAT_FACTORY): NULL;

	// TODO: emphasize mexes and energy more during
	// the first two minutes (important for eg. PURE)
	es->mIncome  = ai->cb->GetMetalIncome();
	es->eIncome  = ai->cb->GetEnergyIncome();
	es->mLevel   = ai->cb->GetMetal();
	es->eLevel   = ai->cb->GetEnergy();
	es->mStorage = ai->cb->GetMetalStorage();
	es->eStorage = ai->cb->GetEnergyStorage();
	es->mUsage   = ai->cb->GetMetalUsage();
	es->eUsage   = ai->cb->GetEnergyUsage();
	es->makersOn = ai->uh->metalMaker->AllAreOn();

	es->m1 = 500.0f / es->mStorage;				// 0.5f IIF starting with 1000E
	es->m2 = 200.0f / es->mStorage;				// 0.2f IIF starting with 1000E
	es->e1 = 500.0f / es->eStorage;				// 0.5f IIF starting with 1000M
	es->e2 = 800.0f / es->eStorage;				// 0.8f IIF starting with 1000M
	// es->m1 = es->mStorage * 0.5f;			// bad: as M-storage cap. increases the needed M-level to build also rises
	// es->m2 = es->mStorage * 0.2f;
	// es->e1 = es->eStorage * 0.5f;			// bad: as E-storage cap. increases the needed E-level to build also rises
	// es->e2 = es->eStorage * 0.8f;
	es->mLevel50 = (es->mLevel < (es->mStorage * es->m1));
	es->eLevel50 = (es->eLevel > (es->eStorage * es->e1));
	es->eLevel80 = (es->eLevel > (es->eStorage * es->e2));

	// fake a resource crisis during the first
	// minute to get our economy going quicker
	// KLOOTNOTE: reverted, has opposite effect
	es->mStall = (/*(frame < 1800) ||*/ (es->mIncome < (es->mUsage * 1.3f)));
	es->eStall = (/*(frame <  900) ||*/ (es->eIncome < (es->eUsage * 1.6f)));
	es->mOverflow = (es->mStorage / (es->mIncome + 0.01)) < (STORAGETIME * 2);

	es->eLevelMed = (es->eLevel50 && es->makersOn);
	es->mLevelLow =
		es->mLevel50 ||
		(es->mStall && es->eLevel80) ||
		(!es->factFeasM && factoryTimer <= 0);

	es->factFeasM =
		(es->factoryDef != NULL)?
		ai->math->MFeasibleConstruction(es->builderDef, es->factoryDef):
		true;
	es->factFeasE =
		(es->factoryDef != NULL)?
		ai->math->EFeasibleConstruction(es->builderDef, es->factoryDef):
		true;
	es->factFeas =
		((es->factoryDef != NULL) && es->factFeasM && es->factFeasE);

	// these determine if we can tell our idle
	// factories to start building something
	// M- and E-levels can never exceed the
	// storage capacity, so we need to make
	// sure that e2 and m2 are less than 1
	es->b1 = ((es->eLevel > (es->eStorage * es->e2)) || (es->eIncome > 6000.0f && es->eUsage < es->eIncome));
	es->b2 = ((es->mLevel > (es->mStorage * es->m2)) || (es->mIncome >  100.0f && es->mUsage < es->mIncome));
	es->b3 = (es->m2 >= 1.0f || es->e2 >= 1.0f);

	// KLOOTNOTE: <MAX_NUKE_SILOS> nuke silos ought to be enough for
	// everybody (assuming we can build them at all in current mod)
	// TODO: use actual metal and energy drain of nuke weapon here
	es->buildNukeSilo =
		(es->builderDef != NULL) && (ai->uh->NukeSilos.size() < MAX_NUKE_SILOS) &&
		(es->mIncome > 100.0f && es->eIncome > 6000.0f) &&
		(es->mUsage < es->mIncome && es->eUsage < es->eIncome);

	es->numM = ai->uh->AllUnitsByCat[CAT_MEX].size();
	es->numE = ai->uh->AllUnitsByCat[CAT_ENERGY].size();

	es->numDefenses  = ai->uh->AllUnitsByCat[CAT_DEFENCE].size();
	es->numFactories = ai->uh->AllUnitsByCat[CAT_FACTORY].size();
}

BuildState CBuildUp::GetBuildState(int frame, const EconState* es) const {
	if (
		(es->numM < 2 && es->numE <= 2) ||
		(es->numE < 2 && es->numM <= 2) ||
		(es->numFactories < 1)
	) {
		return BUILD_INIT;
	}

	if (es->buildNukeSilo && nukeSiloTimer <= 0) {
		return BUILD_NUKE;
	}

	if (/*es->eLevelMed &&*/ es->mLevelLow) {
		return BUILD_M_STALL;
	}

	if (es->eStall || !es->factFeasE) {
		return BUILD_E_STALL;
	}

	if (
		es->eIncome > 2000.0f && es->eUsage < (es->eIncome - 1000.0f) &&
		es->mStall && es->mLevel < 100.0f
	) {
		return BUILD_E_EXCESS;
	}
	/*
	// if we never build defenses, this will always be true
	if (es->numFactories > (es->numDefenses / DEFENSEFACTORYRATIO) && frame > 18000) {
		return BUILD_DEFENSE;
	}
	*/

	return BUILD_FACTORY;
}




void CBuildUp::Buildup(int frame) {
	if (econState.nIdleBuilders > 0) {
		const BuildState buildState = GetBuildState(frame, &econState);
		const bool buildInterrupted =
			(ai->dgunConHandler->GetController(econState.builderID) != NULL) &&
			(ai->dgunConHandler->GetController(econState.builderID)->IsBusy());

		if (econState.builderDef == NULL) {
			ai->uh->UnitDestroyed(econState.builderID);
		} else {
			switch (buildState) {
				case BUILD_INIT: {
					if (!buildInterrupted) {
						// note: in E&E metal processors belong to CAT_ENERGY,
						// so numM never reaches 3 and we need to allow overlap
						// note: this probably breaks mods with static commanders
						if (econState.numM < 2 && econState.numE <= 2) {
							BuildUpgradeExtractor(econState.builderID); return;
						}
						if (econState.numE < 2 && econState.numM <= 2) {
							BuildUpgradeReactor(econState.builderID); return;
						}
						if (econState.numFactories < 1 && econState.factFeas) {
							BuildNow(econState.builderID, CAT_FACTORY, econState.factoryDef); return;
						}

						if (ai->uh->FactoryBuilderAdd(econState.builderID)) {
							// add commander to factory so it doesn't wander
							builderTimer = 0;
						}
					}
				} break;

				case BUILD_NUKE: {
					if (!ai->uh->BuildTaskAddBuilder(econState.builderID, CAT_NUKE)) {
						// always favor building one silo at a time rather than
						// many in parallel to prevent a sudden massive resource
						// drain when silos are finished
						if (BuildNow(econState.builderID, CAT_NUKE, NULL)) {
							nukeSiloTimer += 300;
						}
					}
				} break;

				case BUILD_M_STALL: {
					// only reclaim features during odd frames so we don't
					// spend the entire game just chasing after rocks etc.
					// (problem on Cooper Hill and similar maps)
					// FIXME: not happening often enough during res. stalls
					//
					const bool reclaimFeature =
						((frame & 1) && ai->MyUnits[econState.builderID]->ReclaimBestFeature(true, 4096));

					if (!reclaimFeature) {
						const bool haveNewMex = BuildUpgradeExtractor(econState.builderID);
						const bool eOverflow  = (econState.eStorage / (econState.eIncome + 0.01) < STORAGETIME);
						const bool eExcess    = (econState.eIncome > (econState.eUsage * 1.5));

						// if we couldn't build or upgrade an extractor
						if (!haveNewMex && eOverflow && storageTimer <= 0) {
							if (!ai->uh->BuildTaskAddBuilder(econState.builderID, CAT_ESTOR)) {
								// build energy storage
								if (BuildNow(econState.builderID, CAT_ESTOR, NULL)) {
									storageTimer += 90;
								}
							}
						}
						else if (!haveNewMex && eExcess) {
							// build metal maker
							if (!ai->uh->BuildTaskAddBuilder(econState.builderID, CAT_MMAKER)) {
								BuildNow(econState.builderID, CAT_MMAKER, NULL);
							}
						}
					}
				} break;

				case BUILD_E_STALL: {
					BuildUpgradeReactor(econState.builderID);
				} break;

				case BUILD_E_EXCESS: {
					if (!ai->uh->BuildTaskAddBuilder(econState.builderID, CAT_MMAKER)) {
						BuildNow(econState.builderID, CAT_MMAKER, NULL);
					}
				} break;

				case BUILD_DEFENSE: {
					// do we have more factories than defenses (and have at least 10 minutes passed)?
					if (econState.numFactories > (econState.numDefenses / DEFENSEFACTORYRATIO) && frame > 18000) {
						if (econState.mOverflow && (storageTimer <= 0) && (econState.numFactories > 0)) {
							if (!ai->uh->BuildTaskAddBuilder(econState.builderID, CAT_MSTOR)) {
								// build metal storage
								if (BuildNow(econState.builderID, CAT_MSTOR, NULL)) {
									storageTimer += 90;
								}
							}
						} else {
							/*
							if (!ai->uh->BuildTaskAddBuilder(econState.builderID, CAT_DEFENCE)) {
								// if we can't add this builder to some defense
								// task then build something in CAT_DEFENCE
								const UnitDef* building = ai->ut->GetUnitByScore(econState.builderID, CAT_DEFENCE);
								bool r = false;

								if (building) {
									float3 buildPos = ai->dm->GetDefensePos(building, ai->MyUnits[econState.builderID]->pos());
									r = ai->MyUnits[econState.builderID]->Build_ClosestSite(building, buildPos, 2);
								} else {
									FallbackBuild(econState.builderID, CAT_DEFENCE);
								}
							}
							*/
						}
					}
				} break;

				case BUILD_FACTORY: {
					// no, build more factories
					if (!ai->uh->BuildTaskAddBuilder(econState.builderID, CAT_FACTORY)) {
						// if we can't add this builder to some other buildtask
						if (!ai->uh->FactoryBuilderAdd(econState.builderID)) {
							// if we can't add this builder to some
							// other factory then construct new one
							// (but restrict the number of factories
							// to one for the first 10 minutes)
							if (ai->uh->AllUnitsByCat[CAT_FACTORY].size() < 1 || frame > 13500) {
								BuildNow(econState.builderID, CAT_FACTORY, econState.factoryDef);
							} else {
								std::stringstream msg;
									msg << "[CBuildUp::BuildUp()] frame " << frame << "\n";
									msg << "\tbuilder " << econState.builderID << " is currently in limbo";
									msg << " (total number of idle builders: " << econState.nIdleBuilders << ")\n";
								L(ai, msg.str());
							}
						}
					}
				} break;
			}
		}
	}

	if ((econState.b1 && econState.b2) || econState.b3) {
		FactoryCycle(frame);
	}

	if (!ai->uh->AllUnitsByCat[CAT_NUKE].empty()) {
		NukeSiloCycle();
	}
}




void CBuildUp::FactoryCycle(int frame) {
	int numIdleFactories = ai->uh->NumIdleUnits(CAT_FACTORY);

	for (int i = 0; i < numIdleFactories; i++) {
		// pick the i-th idle factory we have
		UnitCategory producedCat = CAT_LAST;
		const int factoryUnitID  = ai->uh->GetIU(CAT_FACTORY);
		const CUNIT* u           = ai->MyUnits[factoryUnitID];
		const bool isHub         = u->isHub();
		const UnitDef* factDef   = u->def();

		// assume that factories with tech-level TL > 0 are
		// useful to keep active and building for (TL * 30)
		// minutes, but depreciate rapidly after that point
		// TODO: don't reduce factory build frequency, but
		// focus more on mobile constructors instead?
		const int  tchLvl   = ai->ut->unitTypes[factDef->id].techLevel;
		const bool obsolete = ((tchLvl > 0)? ((tchLvl * 30) > (frame / 1800)): false);
		const bool mayBuild = ((obsolete)? (frame % 1800 == 0): true);

		if (mayBuild) {
			if (isHub) {
				// if we are a hub then assume we can only construct
				// factories and some other types of static buildings
				// note: not always true, in Evolution the "commander"
				// (starting factory unit) can construct mobile units
				if (factDef->isCommander) {
					producedCat = CAT_BUILDER;
					builderTimer = 0;
				} else {
					producedCat = (econState.eStall)? CAT_ENERGY: CAT_FACTORY;
					factoryTimer = 0;
				}
			} else {
				if ((builderTimer > 0) || (ai->uh->NumIdleUnits(CAT_BUILDER) > 2)) {
					// if we have more than two idle builders
					// then compensate with an offensive unit
					producedCat = CAT_G_ATTACK;
					builderTimer = std::max(0, builderTimer - 1);
				}

				else {
					const UnitDef* leastBuiltBuilderDef = GetLeastBuiltBuilder();
					const UnitDef* builderUnitDef = ai->ut->GetUnitByScore(factoryUnitID, CAT_BUILDER);

					if (builderUnitDef && builderUnitDef == leastBuiltBuilderDef) {
						// if this factory makes the builder that we are short of
						producedCat = CAT_BUILDER;
						builderTimer += 4;
					} else {
						// build some offensive unit
						producedCat = CAT_G_ATTACK;
						builderTimer = std::max(0, builderTimer - 1);
					}
				}
			}

			// get a unit of the category we want this factory to produce
			const UnitDef* udef = ai->ut->GetUnitByScore(factoryUnitID, producedCat);

			if (udef) {
				if (isHub) {
					const bool factFeasM = ai->math->MFeasibleConstruction(factDef, udef);
					const bool factFeasE = ai->math->EFeasibleConstruction(factDef, udef);

					// cap the number of assistable factories of type <udef>
					const bool b0 = (producedCat == CAT_FACTORY && udef->canBeAssisted);
					const bool b1 = ((ai->uh->AllUnitsByType[udef->id]).size() < 1);

					if (factFeasM && factFeasE) {
						if (!b0 || b1) {
							u->HubBuild(udef);
						} else {
							u->Patrol(u->pos());
						}
					}
				} else {
					u->FactoryBuild(udef);
				}
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




void CBuildUp::FallbackBuild(int builderID, int failedCat) {
	// called if an idle builder was selected to construct
	// some category of unit, but the builder was incapable
	// of constructing anything of that category (note that
	// if AI is swimming in resources then most L1 builders
	// will be used in assisting roles)
	bool b1 = ai->uh->BuildTaskAddBuilder(builderID, CAT_MEX);
	bool b2 = false;
	bool b3 = false;
	bool b4 = false;
	float3 builderPos = ai->cb->GetUnitPos(builderID);

	if (!b1              ) { b2 = ai->uh->BuildTaskAddBuilder(builderID, CAT_ENERGY); }
	if (!b1 && !b2       ) { b3 = ai->uh->BuildTaskAddBuilder(builderID, CAT_DEFENCE); }
	if (!b1 && !b2 && !b3) { b4 = ai->uh->BuildTaskAddBuilder(builderID, CAT_FACTORY); }

/*
	if (!b1 && !b2 && !b3 && !b4) {
		// failed to add builder to any task, try building something
		const UnitDef* udef1 = ai->ut->GetUnitByScore(builderID, CAT_MEX);
		const UnitDef* udef2 = ai->ut->GetUnitByScore(builderID, CAT_ENERGY);
		const UnitDef* udef3 = ai->ut->GetUnitByScore(builderID, CAT_DEFENCE);
		const UnitDef* udef4 = ai->ut->GetUnitByScore(builderID, CAT_FACTORY);

		if (udef2 && failedCat != CAT_ENERGY) {
			ai->MyUnits[builderID]->Build_ClosestSite(udef2, builderPos);
			return;
		}
		if (udef3 && failedCat != CAT_DEFENCE) {
			float3 pos = ai->dm->GetDefensePos(udef3, builderPos);
			ai->MyUnits[builderID]->Build_ClosestSite(udef3, pos);
			return;
		}
		if (udef4 && failedCat != CAT_FACTORY) {
			ai->MyUnits[builderID]->Build_ClosestSite(udef4, builderPos);
			return;
		}
		if (udef1 && failedCat != CAT_MEX) {
			float3 pos = ai->mm->GetNearestMetalSpot(builderID, udef1);
			if (pos != ERRORVECTOR)
				ai->MyUnits[builderID]->Build(pos, udef1, -1);
			return;
		}
	}
*/

	// unable to assist and unable to build, just patrol
	if (!b1 && !b2 && !b3 && !b4) {
		ai->MyUnits[builderID]->Patrol(builderPos);
	}
}



// look at all online factories and their best builders,
// then find the best builder that there are least of
const UnitDef* CBuildUp::GetLeastBuiltBuilder(void) {
	const UnitDef* leastBuiltBuilder = 0;
	int leastBuiltBuilderCount = 65536;

	std::list<int>::iterator j;
	for (j = ai->uh->AllUnitsByCat[CAT_FACTORY].begin(); j != ai->uh->AllUnitsByCat[CAT_FACTORY].end(); j++) {
		// get factory unitID
		int factoryToLookAt = *j;

		if (!ai->cb->UnitBeingBuilt(factoryToLookAt)) {
			// if factory isn't still under construction
			const UnitDef* bestBuilder = ai->ut->GetUnitByScore(factoryToLookAt, CAT_BUILDER);

			if (bestBuilder) {
				int bestBuilderCount = ai->uh->AllUnitsByType[bestBuilder->id].size();

				if (bestBuilderCount < leastBuiltBuilderCount) {
					leastBuiltBuilderCount = bestBuilderCount;
					leastBuiltBuilder = bestBuilder;
				}
			}
		}
	}

	return leastBuiltBuilder;
}



bool CBuildUp::BuildNow(int builderID, UnitCategory cat, const UnitDef* udef) {
	bool r = false;

	if (udef == NULL) {
		udef = ai->ut->GetUnitByScore(builderID, cat);
	}

	if (udef != NULL) {
		// cap the number of assistable factories of type <building>
		const bool b0 = (cat == CAT_FACTORY && udef->canBeAssisted);
		const bool b1 = ((ai->uh->AllUnitsByType[udef->id]).size() < 1);

		if (!b0 || b1) {
			r = ai->MyUnits[builderID]->Build_ClosestSite(udef, ai->cb->GetUnitPos(builderID));
		}
	} else {
		FallbackBuild(builderID, cat);
	}

	return r;
}



bool CBuildUp::BuildUpgradeExtractor(int builderID) {
	const UnitDef* newMexDef = ai->ut->GetUnitByScore(builderID, CAT_MEX);

	if (newMexDef != NULL) {
		const float3 builderPos = ai->MyUnits[builderID]->pos();
		const float3 newMexPos  = ai->mm->GetNearestMetalSpot(builderID, newMexDef);
		// const float  oldMexDist = newMexPos.distance2D(builderPos);

		if (newMexPos != ERRORVECTOR) {
			if (!ai->uh->BuildTaskAddBuilder(builderID, CAT_MEX)) {
				// build a new metal extractor
				return (ai->MyUnits[builderID]->Build(newMexPos, newMexDef, -1));
			}
		} else {
			// upgrade an existing extractor (NOTE: GetNearestMetalSpot()
			// very rarely returns an error-vector, so we should give this
			// some more incentives)
			const int      oldMexID  = ai->uh->GetOldestMetalExtractor();
			const float3&  oldMexPos = ai->cb->GetUnitPos(oldMexID);
			const UnitDef* oldMexDef = ai->cb->GetUnitDef(oldMexID);

			if (oldMexDef != NULL) {
				if (ai->cb->GetUnitHealth(oldMexID) >= ai->cb->GetUnitMaxHealth(oldMexID)) {
					if ((newMexDef->extractsMetal / oldMexDef->extractsMetal) >= 1.5f) {
						UpgradeTask* task = ai->uh->FindUpgradeTask(oldMexID);

						if (task == NULL) {
							task = ai->uh->CreateUpgradeTask(oldMexID, oldMexPos, newMexDef);
						}

						ai->uh->AddUpgradeTaskBuilder(task, builderID);
						return true;
					}
				}
			}
		}
	}

	// can't build or upgrade
	return false;
}



bool CBuildUp::BuildUpgradeReactor(int builderID) {
	bool ret = true;

	if (!ai->uh->BuildTaskAddBuilder(builderID, CAT_ENERGY)) {
		const UnitDef* newReactorDef = ai->ut->GetUnitByScore(builderID, CAT_ENERGY);

		if (newReactorDef) {
			const float3 builderPos   = ai->cb->GetUnitPos(builderID);
			float        netEnergy    = newReactorDef->energyMake - newReactorDef->energyUpkeep;
			float        closestDstSq = MY_FLT_MAX;

			int             oldReactorID  = -1;
			float3          oldReactorPos = ZEROVECTOR;
			const  UnitDef* oldReactorDef = NULL;

			std::list<int> lst = ai->uh->AllUnitsByCat[CAT_ENERGY];

			for (std::list<int>::iterator it = lst.begin(); it != lst.end(); it++) {
				const int      itReactorID  = *it;
				const float3   itReactorPos = ai->cb->GetUnitPos(itReactorID);
				const UnitDef* itReactorDef = ai->cb->GetUnitDef(itReactorID);

				if (ai->cb->GetUnitHealth(itReactorID) >= ai->cb->GetUnitMaxHealth(itReactorID)) {
					if (itReactorDef->energyMake <= 0.0f || itReactorDef->windGenerator > 0.0f) {
						// only look at solars which produce energy through negative
						// upkeep (ud.energyUpkeep = udTable.GetFloat("energyUse"))
						// rather than positive energyMake, and at windmills
						float       itUpgradeRatio = 0.0f;
						const float itNetEnergy    = itReactorDef->energyMake - itReactorDef->energyUpkeep;
						const float itReactorDstSq = (itReactorPos - builderPos).SqLength();

						if (itNetEnergy > 0.0f) {
							itUpgradeRatio = netEnergy / itNetEnergy;
						} else {
							if (itReactorDef->windGenerator > 0.0f) {
								const float minWind = ai->cb->GetMinWind();
								const float maxWind = ai->cb->GetMaxWind();
								const float avgWind = (minWind + maxWind) * 0.5f;
								const float maxMake = std::min(maxWind, itReactorDef->windGenerator);
								const float avgEffi = std::min(avgWind / itReactorDef->windGenerator, 1.0f);

								if (maxWind > 0.0f) {
									itUpgradeRatio = (netEnergy * 0.5f) / (maxMake * avgEffi);
								}
							}
						}

						// find the closest CAT_ENERGY structure
						// FIXME: find the closest and oldest?
						if (itReactorDstSq < closestDstSq) {
							if (itUpgradeRatio > 2.0f) {
								oldReactorID  = itReactorID;
								oldReactorPos = itReactorPos;
								oldReactorDef = itReactorDef;
								closestDstSq  = itReactorDstSq;
							}
						}
					}
				}
			}

			if (oldReactorID != -1) {
				UpgradeTask* task = ai->uh->FindUpgradeTask(oldReactorID);

				if (task == NULL) {
					task = ai->uh->CreateUpgradeTask(oldReactorID, oldReactorPos, newReactorDef);
				}

				ai->uh->AddUpgradeTaskBuilder(task, builderID);
				ret = true;
			} else {
				ret = BuildNow(builderID, CAT_ENERGY, newReactorDef);
			}
		} else {
			ret = false;
		}
	}

	// can't build or upgrade
	return ret;
}
