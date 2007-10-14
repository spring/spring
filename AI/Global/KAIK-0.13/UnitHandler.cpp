#include "UnitHandler.h"
#include "MetalMaker.h"


CR_BIND(CUnitHandler, (NULL));
CR_REG_METADATA(CUnitHandler, (
	CR_MEMBER(IdleUnits),
	CR_MEMBER(BuildTasks),
	CR_MEMBER(TaskPlans),
	CR_MEMBER(AllUnitsByCat),
	CR_MEMBER(AllUnitsByType),

	CR_MEMBER(Factories),
	CR_MEMBER(NukeSilos),
	CR_MEMBER(MetalExtractors),

	CR_MEMBER(Limbo),
	CR_MEMBER(BuilderTrackers),

	CR_MEMBER(metalMaker),

	CR_MEMBER(ai),
	CR_MEMBER(taskPlanCounter),
	CR_RESERVED(16)
));


CUnitHandler::CUnitHandler(AIClasses* ai) {
	this->ai = ai;
	taskPlanCounter = 1;
	IdleUnits.resize(LASTCATEGORY);
	BuildTasks.resize(LASTCATEGORY);
	TaskPlans.resize(LASTCATEGORY);
	AllUnitsByCat.resize(LASTCATEGORY);

	if (ai) {
		AllUnitsByType.resize(ai->cb->GetNumUnitDefs() + 1);
		metalMaker = new CMetalMaker(ai);
	}
}

CUnitHandler::~CUnitHandler() {
	for (list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
		delete *i;
	}
}



void CUnitHandler::IdleUnitUpdate(int frame) {
	list<integer2> limboremoveunits;

	for (list<integer2>::iterator i = Limbo.begin(); i != Limbo.end(); i++) {
		if (i->y > 0) {
			i->y--;
		}
		else {
			if (ai->cb->GetUnitDef(i->x) == NULL) {
				// ignore dead unit
			} else {
				IdleUnits[ai->ut->GetCategory(i->x)].push_back(i->x);
			}

			limboremoveunits.push_back(*i);
		}
	}

	if (limboremoveunits.size()) {
		for (list<integer2>::iterator i = limboremoveunits.begin(); i != limboremoveunits.end(); i++) {
			Limbo.remove(*i);
		}
	}

	// make sure that all the builders are in action (hack?)
	if (frame % 15 == 0) {
		for (list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
			// the new test
			if ((*i)->idleStartFrame != -2) {
				// the brand new builders must be filtered still
				bool ans = VerifyOrder(*i);
				const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands((*i)->builderID);
				Command c;

				if (mycommands->size() > 0)
					c = mycommands->front();

				// two sec delay is ok
				if (((*i)->commandOrderPushFrame + LAG_ACCEPTANCE) < frame) {
					if (!ans) {
						char text[512];
						float3 pos = ai->cb->GetUnitPos((*i)->builderID);
						sprintf(text, "builder %i VerifyOrder failed ", (*i)->builderID);

						ClearOrder(*i, false);

						if (!mycommands->empty())
							DecodeOrder(*i, true);
						else
							IdleUnitAdd((*i)->builderID, frame);
					}
				}
			}
		}
	}
}




void CUnitHandler::UnitMoveFailed(int unit) {
	unit = unit;
}

// called when unit nanoframe first created
// (CEconomyTracker deals with UnitFinished())
void CUnitHandler::UnitCreated(int unit) {
	int category = ai->ut->GetCategory(unit);
	const UnitDef* newUnitDef = ai->cb->GetUnitDef(unit);

	if (category != -1) {
		AllUnitsByCat[category].push_back(unit);
		AllUnitsByType[newUnitDef->id].push_back(unit);

		if (category == CAT_FACTORY) {
			FactoryAdd(unit);
		}

		BuildTaskCreate(unit);

		if (category == CAT_BUILDER) {
			// add the new builder
			BuilderTracker* builderTracker = new BuilderTracker;
			builderTracker->builderID = unit;
			builderTracker->buildTaskId = 0;
			builderTracker->taskPlanId = 0;
			builderTracker->factoryId = 0;
			builderTracker->stuckCount = 0;
			builderTracker->customOrderId = 0;
			// under construction
			builderTracker->commandOrderPushFrame = -2;
			builderTracker->categoryMaker = -1;
			// wait for the first idle call, as this unit might be under construction
			builderTracker->idleStartFrame = -2;
			BuilderTrackers.push_back(builderTracker);
		}

		if (category == CAT_MMAKER) {
			MMakerAdd(unit);
		}
		if (category == CAT_MEX) {
			MetalExtractorAdd(unit);
		}

		if (category == CAT_NUKE) {
			NukeSiloAdd(unit);
		}
	}
}

void CUnitHandler::UnitDestroyed(int unit) {
	int category = ai->ut->GetCategory(unit);
	const UnitDef* unitDef = ai->cb->GetUnitDef(unit);

	if (category != -1) {
		AllUnitsByType[unitDef->id].remove(unit);
		AllUnitsByCat[category].remove(unit);
		IdleUnitRemove(unit);
		BuildTaskRemove(unit);

		if (category == CAT_DEFENCE) {
			ai->dm->RemoveDefense(ai->cb->GetUnitPos(unit), unitDef);
		}
		if (category == CAT_MMAKER) {
			MMakerRemove(unit);
		}
		if (category == CAT_FACTORY) {
			FactoryRemove(unit);
		}

		if (category == CAT_BUILDER) {
			// remove the builder
			for (list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
				if ((*i)->builderID == unit) {
					if ((*i)->buildTaskId)
						BuildTaskRemove(*i);
					if ((*i)->taskPlanId)
						TaskPlanRemove(*i);
					if ((*i)->factoryId)
						FactoryBuilderRemove(*i);

					BuilderTracker* builderTracker = *i;
					BuilderTrackers.erase(i);
					delete builderTracker;
					break;
				}
			}
		}

		if (category == CAT_MEX) {
			MetalExtractorRemove(unit);
		}
		if (category == CAT_NUKE) {
			NukeSiloRemove(unit);
		}
	}
}


void CUnitHandler::IdleUnitAdd(int unit, int frame) {
	int category = ai->ut->GetCategory(unit);

	if (category != -1) {
		const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(unit);

		if (mycommands->empty()) {
			if (category == CAT_BUILDER) {
				BuilderTracker* builderTracker = GetBuilderTracker(unit);
				// add clear here
				ClearOrder(builderTracker, true);

				if (builderTracker->idleStartFrame == -2) {
					// it was in the idle list already?
					IdleUnitRemove(builderTracker->builderID);
				}

				builderTracker->idleStartFrame = -2;

				if (builderTracker->commandOrderPushFrame == -2) {
					// make sure that if the unit was just built
					// it will have some time to leave the factory
					builderTracker->commandOrderPushFrame = frame + 30 * 3;
				}
			}

			integer2 myunit(unit, LIMBOTIME);
			Limbo.remove(myunit);
			Limbo.push_back(myunit);
		}
		else {
			// the unit has orders still
			if (category == CAT_BUILDER) {
				if (false) {
					// KLOOTNOTE: somehow we are now reaching this branch
					// on initialization when USE_CREG is not defined,
					// mycommands->size() returns garbage?
					BuilderTracker* builderTracker = GetBuilderTracker(unit);
					DecodeOrder(builderTracker, true);
				}
			}
		}
	}
}





bool CUnitHandler::VerifyOrder(BuilderTracker* builderTracker) {
	// if it's without orders then try to find the lost command (TODO)
	const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);
	bool commandFound = false;
	bool hit = false;

	if (mycommands->size() > 0) {
		// it has orders
		const Command* c = &mycommands->front();

		if (mycommands->size() == 2) {
			// it might have a reclaim order, or terrain change order
			// take command nr. 2
			c = &mycommands->back();
		}

		if (builderTracker->buildTaskId != 0) {
			hit = true;
			// test that this builder is on repair on this unit
			BuildTask* buildTask = GetBuildTask(builderTracker->buildTaskId);

			if (c->id == CMD_REPAIR && c->params[0] == builderTracker->buildTaskId
				|| (c->id == -buildTask->def->id && c->params[0] == buildTask->pos.x && c->params[2] == buildTask->pos.z))
				commandFound = true;
			else
				return false;
		}

		if (builderTracker->taskPlanId != 0) {
			assert(!hit);
			hit = true;
			TaskPlan* taskPlan = GetTaskPlan(builderTracker->taskPlanId);
			
			if (c->id == -taskPlan->def->id && c->params[0] == taskPlan->pos.x && c->params[2] == taskPlan->pos.z)
				commandFound = true;
			else
				return false;
		}

		if (builderTracker->factoryId != 0) {
			assert(!hit);
			hit = true;
			if (c->id == CMD_GUARD && c->params[0] == builderTracker->factoryId)
				commandFound = true;
			else
				return false;
		}

		if (builderTracker->customOrderId != 0) {
			assert(!hit);
			hit = true;
			// CMD_MOVE is for human control stuff, CMD_REPAIR is for repairs of just made stuff
			// CMD_GUARD... ?
			if (c->id == CMD_RECLAIM || c->id == CMD_MOVE || c->id == CMD_REPAIR)
				commandFound = true;
			else {
				return false;
			}
		}

		if (hit && commandFound) {
			// it's on the right job
			return true;
		}
	}

	else  {
		if (builderTracker->idleStartFrame == -2) {
			return true;
		}
	}

	return false;
}




// use this only if the unit does not have any orders at the moment
void CUnitHandler::ClearOrder(BuilderTracker* builderTracker, bool reportError) {
	bool hit = false;
	const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);
	assert(mycommands->empty() || !reportError);

	if (builderTracker->buildTaskId != 0) {
		// why is this builder idle?
		hit = true;
		BuildTask* buildTask = GetBuildTask(builderTracker->buildTaskId);
		char text[512];
		sprintf(text, "builder %i: was idle, but it is on buildTaskId: %i  (stuck?)", builderTracker->builderID, builderTracker->buildTaskId);

		if (buildTask->builderTrackers.size() > 1) {
			BuildTaskRemove(builderTracker);
		} else {
			// only builder of this thing, and now idle
			BuildTaskRemove(builderTracker);
		}
	}

	if (builderTracker->taskPlanId != 0) {
		assert(!hit);
		hit = true;
		// why is this builder idle?
		TaskPlan* taskPlan = GetTaskPlan(builderTracker->taskPlanId);
		char text[512];
		sprintf(text, "builder %i: was idle, but it is on taskPlanId: %s (masking this spot)", builderTracker->builderID, taskPlan->def->humanName.c_str());

		ai->dm->MaskBadBuildSpot(taskPlan->pos);
		// TODO: fix this, remove all builders from this plan

		if (reportError) {
			list<BuilderTracker*> builderTrackers = taskPlan->builderTrackers;
			for (list<BuilderTracker*>::iterator i = builderTrackers.begin(); i != builderTrackers.end(); i++) {
				TaskPlanRemove(*i);
				ai->MyUnits[(*i)->builderID]->Stop();
			}
		} else {
			TaskPlanRemove(builderTracker);
		}
	}

	if (builderTracker->factoryId != 0) {
		assert(!hit);
		hit = true;
		// why is this builder idle?
		char text[512];
		sprintf(text, "builder %i: was idle, but it is on factoryId: %i (removing the builder from the job)", builderTracker->builderID, builderTracker->factoryId);

		FactoryBuilderRemove(builderTracker);
	}

	if (builderTracker->customOrderId != 0) {
		assert(!hit);
		hit = true;
		// why is this builder idle?
		// no tracking of custom orders yet
		// char text[512];
		// sprintf(text, "builder %i: was idle, but it is on customOrderId: %i (removing the builder from the job)", unit, builderTracker->customOrderId);
		builderTracker->customOrderId = 0;
	}

	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
}




void CUnitHandler::DecodeOrder(BuilderTracker* builderTracker, bool reportError) {
	// take a look and see what it's doing
	const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);

	if (mycommands->size() > 0) {
		// builder has orders
		const Command* c = &mycommands->front();
		if (mycommands->size() == 2 && c->id == CMD_MOVE) { //&& (c->id == CMD_MOVE || c->id == CMD_RECLAIM))
			// it might have a move order before the real order,
			// take command nr. 2 if nr. 1 is a move order
			c = &mycommands->back();
		}

		if (reportError) {
			char text[512];
			sprintf(text, "builder %i: claimed idle, but has command c->id: %i, c->params[0]: %f", builderTracker->builderID, c->id, c->params[0]);
		}

		if (c->id < 0) {
			// it's building a unit
			float3 newUnitPos;
			newUnitPos.x = c->params[0];
			newUnitPos.y = c->params[1];
			newUnitPos.z = c->params[2];
			// c.id == -newUnitDef->id
			const UnitDef* newUnitDef = ai->ut->unittypearray[-c->id].def;
			// make sure that no BuildTasks exists there
			BuildTask* buildTask = BuildTaskExist(newUnitPos, newUnitDef);

			if (buildTask) {
				BuildTaskAddBuilder(buildTask, builderTracker);
			} else {
				// make a new TaskPlan (or join an existing one)
				TaskPlanCreate(builderTracker->builderID, newUnitPos, newUnitDef);
			}
		}

		if (c->id == CMD_REPAIR) {
			// it's repairing
			int guardingID = int(c->params[0]);
			// find the unit being repaired
			int category = ai->ut->GetCategory(guardingID);
			bool found = false;

			if (category == -1)
				return;

			for (list<BuildTask>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
				if (i->id == guardingID) {
					// whatever the old order was, update it now
					bool hit = false;
					if (builderTracker->buildTaskId != 0) {
						hit = true;
						// why is this builder idle?
						BuildTask* buildTask = GetBuildTask(builderTracker->buildTaskId);

						if (buildTask->builderTrackers.size() > 1) {
							BuildTaskRemove(builderTracker);
						} else {
							// only builder of this thing, and now idle?
							BuildTaskRemove(builderTracker);
						}
					}
					if (builderTracker->taskPlanId != 0) {
						assert(!hit);
						hit = true;
						TaskPlanRemove(builderTracker);
					}
					if (builderTracker->factoryId != 0) {
						assert(!hit);
						hit = true;
						FactoryBuilderRemove(builderTracker);
					}
					if (builderTracker->customOrderId != 0) {
						assert(!hit);
						hit = true;
						builderTracker->customOrderId = 0;
					}
					BuildTask* bt = &*i;
					BuildTaskAddBuilder(bt, builderTracker);
					found = true;
				}
			}
			if (!found) {
				// not found, just make a custom order
				builderTracker->customOrderId = taskPlanCounter++;
				builderTracker->idleStartFrame = -1;
			}
		}
	}
	else {
		// error: this function needs a builder with orders
		assert(false);
	}
}



void CUnitHandler::IdleUnitRemove(int unit) {
	int category = ai->ut->GetCategory(unit);

	if (category != -1) {
		IdleUnits[category].remove(unit);

		if (category == CAT_BUILDER) {
			BuilderTracker* builderTracker = GetBuilderTracker(unit);
			builderTracker->idleStartFrame = -1;

			if (builderTracker->commandOrderPushFrame == -2) {
				// bad
			}

			// update the order start frame
			builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame();
			// assert(builderTracker->buildTaskId == 0);
			// assert(builderTracker->taskPlanId == 0);
			// assert(builderTracker->factoryId == 0);
		}

		list<integer2>::iterator tempunit;
		bool foundit = false;

		for (list<integer2>::iterator i = Limbo.begin(); i != Limbo.end(); i++) {
			if (i->x == unit) {
				tempunit = i;
				foundit = true;
			}
		}

		if (foundit)
			Limbo.erase(tempunit);
	}
}


int CUnitHandler::GetIU(int category) {
	assert(IdleUnits[category]->size() > 0);
	int unitID = IdleUnits[category].front();

	// move the returned unitID to the back
	// of the list, so CBuildUp::FactoryCycle()
	// doesn't keep examining the same factory
	// if a build order fails and we have more
	// than one idle factory
	IdleUnits[category].pop_front();
	IdleUnits[category].push_back(unitID);

	return unitID;
}

int CUnitHandler::NumIdleUnits(int category) {
	assert(category >= 0 && category < LASTCATEGORY);
	IdleUnits[category].sort();
	IdleUnits[category].unique();
	return IdleUnits[category].size();
}




void CUnitHandler::MMakerAdd(int unit) {
	metalMaker->Add(unit);
}
void CUnitHandler::MMakerRemove(int unit) {
	metalMaker->Remove(unit);
}

void CUnitHandler::MMakerUpdate(int frame) {
	metalMaker->Update(frame);
}




void CUnitHandler::BuildTaskCreate(int id) {
	const UnitDef* newUnitDef = ai->cb->GetUnitDef(id);
	int category = ai->ut->GetCategory(id);
	float3 pos = ai->cb->GetUnitPos(id);

	if ((!newUnitDef->movedata || category == CAT_DEFENCE) && !newUnitDef->canfly && category != -1) {
		// this needs to change, so that it can make more stuff
		if (category == -1)
			return;

		assert(category >= 0);
		assert(category < LASTCATEGORY);

		BuildTask bt;
		bt.id = -1;

		redo:
		for (list<TaskPlan>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++) {
			if(pos.distance2D(i->pos) < 1 && newUnitDef == i->def){
				assert(bt.id == -1); // There can not be more than one TaskPlan that is found;
				bt.category = category;
				bt.id = id;
				bt.pos = i->pos;
				bt.def = newUnitDef;
				list<BuilderTracker*> moveList;

				for (list<BuilderTracker*>::iterator builder = i->builderTrackers.begin(); builder != i->builderTrackers.end(); builder++) {
					moveList.push_back(*builder);
				}

				for (list<BuilderTracker*>::iterator builder = moveList.begin(); builder != moveList.end(); builder++) {
					TaskPlanRemove(*builder);
					BuildTaskAddBuilder(&bt, *builder);
				}

				goto redo;
			}
		}

		if (bt.id == -1) {
			// buildtask creation error (can happen if builder manages
			// to restart a dead building, or a human has taken control),
			// make one anyway
			bt.category = category;
			bt.id = id;

			if (category == CAT_DEFENCE)
				ai->dm->AddDefense(pos,newUnitDef);

			bt.pos = pos;
			bt.def = newUnitDef;
			char text[512];
			sprintf(text, "BuildTask Creation Error: %i", id);
			int num = BuilderTrackers.size();

			if (num == 0) {
				// no friendly builders found
			} else {
				// iterate over the list and find the builders
				for (list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
					BuilderTracker* builderTracker = *i;

					// check what builder is doing
					const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);
					if (mycommands->size() > 0) {
						Command c = mycommands->front();

						if ((c.id == -newUnitDef->id && c.params[0] == pos.x && c.params[2] == pos.z) // at this pos
							|| (c.id == CMD_REPAIR  && c.params[0] == id)  // at this unit (id)
							|| (c.id == CMD_GUARD  && c.params[0] == id)) // at this unit (id)
						{
							if (builderTracker->buildTaskId != 0) {
								BuildTask* buildTask = GetBuildTask(builderTracker->buildTaskId);

								if (buildTask->builderTrackers.size() > 1) {
									BuildTaskRemove(builderTracker);
								} else {
									// only builder of this thing, and now idle
									BuildTaskRemove(builderTracker);
								}
							}

							if (builderTracker->taskPlanId != 0) {
								GetTaskPlan(builderTracker->taskPlanId);
								TaskPlanRemove(builderTracker);
							}
							if (builderTracker->factoryId != 0) {
								FactoryBuilderRemove(builderTracker);
							}
							if (builderTracker->customOrderId != 0) {
								builderTracker->customOrderId = 0;
							}

							// this builder is now free
							if (builderTracker->idleStartFrame == -2)
								IdleUnitRemove(builderTracker->builderID);

							// add it to this task
							BuildTaskAddBuilder(&bt, builderTracker);
							sprintf(text, "Added builder %i to buildTaskId %i (human order?)", builderTracker->builderID, builderTracker->buildTaskId);
						}
					}
				}
			}

			// add the task anyway
			BuildTasks[category].push_back(bt);
		}
		else {
			if (category == CAT_DEFENCE)
				ai->dm->AddDefense(pos,newUnitDef);

			BuildTasks[category].push_back(bt);
		}
	}
}


void CUnitHandler::BuildTaskRemove(int id) {
	int category = ai->ut->GetCategory(id);

	if (category == -1)
		return;

	assert(category >= 0);
	assert(category < LASTCATEGORY);
	
	if (category != -1) {
		list<BuildTask>::iterator killtask;
		bool found = false;

		for (list<BuildTask>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
			if (i->id == id) {
				killtask = i;
				assert(!found);
				found = true;
			}
		}
		if (found) {
			// remove the builders from this BuildTask
			list<BuilderTracker*> removeList;
			for (list<BuilderTracker*>::iterator builder = killtask->builderTrackers.begin(); builder != killtask->builderTrackers.end(); builder++) {
				removeList.push_back(*builder);
			}
			for (list<BuilderTracker*>::iterator builder = removeList.begin(); builder != removeList.end(); builder++) {
				BuildTaskRemove(*builder);
			}

			BuildTasks[category].erase(killtask);
		}
	}
}


BuilderTracker* CUnitHandler::GetBuilderTracker(int builder) {
	for (list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
		if ((*i)->builderID == builder) {
			return (*i);
		}
	}

	// this better not happen
	assert(false);
	return 0;
}


void CUnitHandler::BuildTaskRemove(BuilderTracker* builderTracker) {
	if (builderTracker->buildTaskId == 0) {
		assert(false);
		return;
	}
	int category = ai->ut->GetCategory(builderTracker->buildTaskId);
	// TODO: Hack fix
	if (category == -1)
		return;

	assert(category >= 0);
	assert(category < LASTCATEGORY);
	assert(builderTracker->buildTaskId != 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	// list<BuildTask>::iterator killtask;
	bool found = false;
	bool found2 = false;

	for (list<BuildTask>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
		if (i->id == builderTracker->buildTaskId){
			// killtask = i;
			assert(!found);
			for (list<int>::iterator builder = i->builders.begin(); builder != i->builders.end(); builder++) {
				if (builderTracker->builderID == *builder) {
					assert(!found2);
					i->builders.erase(builder);
					builderTracker->buildTaskId = 0;
					found2 = true;
					break;
				}
			}
			for (list<BuilderTracker*>::iterator builder = i->builderTrackers.begin(); builder != i->builderTrackers.end(); builder++) {
				if (builderTracker == *builder) {
					assert(!found);
					i->builderTrackers.erase(builder);
					builderTracker->buildTaskId = 0;
 					// give it time to change command
					builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame();
					found = true;
					break;
				}
			}
			
		}
	}

	assert(found);
}



void CUnitHandler::BuildTaskAddBuilder(BuildTask* buildTask, BuilderTracker* builderTracker) {
	buildTask->builders.push_back(builderTracker->builderID);
	buildTask->builderTrackers.push_back(builderTracker);
	buildTask->currentBuildPower += ai->cb->GetUnitDef(builderTracker->builderID)->buildSpeed;
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	builderTracker->buildTaskId = buildTask->id;
}

BuildTask* CUnitHandler::GetBuildTask(int buildTaskId) {
	for (int k = 0; k < LASTCATEGORY; k++) {
		for (list<BuildTask>::iterator i = BuildTasks[k].begin(); i != BuildTasks[k].end(); i++) {
			if (i->id == buildTaskId)
				return  &*i;
		}
	}

	// this better not happen
	assert(false);
	return 0;
}



BuildTask* CUnitHandler::BuildTaskExist(float3 pos, const UnitDef* builtdef) {
	int category = ai->ut->GetCategory(builtdef);

	if (category == -1)
		return false;

	assert(category >= 0);
	assert(category < LASTCATEGORY);

	for (list<BuildTask>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
		if (i->pos.distance2D(pos) < 1 && ai->ut->GetCategory(i->def) == category) {
			return &*i;
		}
	}
	return NULL;
}



bool CUnitHandler::BuildTaskAddBuilder(int builder, int category) {
	assert(category >= 0);
	assert(category < LASTCATEGORY);
	assert(builder >= 0);
	assert(ai->MyUnits[builder] != NULL);
	BuilderTracker* builderTracker = GetBuilderTracker(builder);

	// make sure this builder is free
	// KLOOTNOTE: no longer use assertions
	// since new code for extractor upgrading
	// (in CBuildUp) seems to trigger them?
	bool b1 = (builderTracker->taskPlanId == 0);
	bool b2 = (builderTracker->buildTaskId == 0);
	bool b3 = (builderTracker->factoryId == 0);
	bool b4 = (builderTracker->customOrderId == 0);

	if (!b1 || !b2 || !b3 || !b4) {
		return false;
	}

	// see if there are any BuildTasks that it can join
	if (BuildTasks[category].size()) {
		float largestime = 0;
		list<BuildTask>::iterator besttask;

		for (list<BuildTask>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
			float timebuilding = ai->math->ETT(*i) - ai->math->ETA(builder, ai->cb->GetUnitPos(i->id));
			if (timebuilding > largestime) {
				largestime = timebuilding;
				besttask = i;
			}
		}

		if (largestime > 0) {
			BuildTaskAddBuilder(&*besttask, builderTracker);
			ai->MyUnits[builder]->Repair(besttask->id);
			return true;
		}
	}

	if (TaskPlans[category].size()) {
			float largestime = 0;
			list<TaskPlan>::iterator besttask;

			for (list<TaskPlan>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++) {
				float timebuilding = (i->def->buildTime / i->currentBuildPower) - ai->math->ETA(builder, i->pos);

				// must test if this builder can make this unit/building too
				if (timebuilding > largestime) {
					const UnitDef* builderDef = ai->cb->GetUnitDef(builder);
					vector<int>* canBuildList = &ai->ut->unittypearray[builderDef->id].canBuildList;
					int size = canBuildList->size();
					int thisBuildingID = i->def->id;

					for (int j = 0; j < size; j++) {
						if (canBuildList->at(j) == thisBuildingID) {
							largestime = timebuilding;
							besttask = i;
							break;
						}
					}
				}
			}

			if (largestime > 10) {
				assert(builder >= 0);
				assert(ai->MyUnits[builder] !=  NULL);
				// this is bad, as ai->MyUnits[builder]->Build uses TaskPlanCreate()
				ai->MyUnits[builder]->Build(besttask->pos, besttask->def, -1);
				return true;
		}
	}

	return false;
}




void CUnitHandler::TaskPlanCreate(int builder, float3 pos, const UnitDef* builtdef) {
	int category = ai->ut->GetCategory(builtdef);

	// HACK
	if (category == -1)
		return;

	assert(category >= 0);
	assert(category < LASTCATEGORY);

	// find this builder
	BuilderTracker* builderTracker = GetBuilderTracker(builder);

	// make sure this builder is free
	// KLOOTNOTE: no longer use assertions
	// since new code for extractor upgrading
	// (in CBuildUp) seems to trigger them?
	bool b1 = (builderTracker->taskPlanId == 0);
	bool b2 = (builderTracker->buildTaskId == 0);
	bool b3 = (builderTracker->factoryId == 0);
	bool b4 = (builderTracker->customOrderId == 0);

	if (!b1 || !b2 || !b3 || !b4) {
		return;
	}


	bool existingtp = false;
	for (list<TaskPlan>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++) {
		if (pos.distance2D(i->pos) < 20 && builtdef == i->def) {
			// make sure there are no other TaskPlans
			assert(!existingtp);
			existingtp = true;
			TaskPlanAdd(&*i, builderTracker);
		}
	}
	if (!existingtp) {
		TaskPlan tp;
		tp.pos = pos;
		tp.def = builtdef;
		tp.defName = builtdef->name;
		tp.currentBuildPower = 0;
		tp.id = taskPlanCounter++;
		TaskPlanAdd(&tp, builderTracker);

		if (category == CAT_DEFENCE)
			ai->dm->AddDefense(pos,builtdef);

		TaskPlans[category].push_back(tp);
	}
}

void CUnitHandler::TaskPlanAdd(TaskPlan* taskPlan, BuilderTracker* builderTracker) {
	taskPlan->builders.push_back(builderTracker->builderID);
	taskPlan->builderTrackers.push_back(builderTracker);
	taskPlan->currentBuildPower += ai->cb->GetUnitDef(builderTracker->builderID)->buildSpeed;
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	builderTracker->taskPlanId = taskPlan->id;
}

void CUnitHandler::TaskPlanRemove(BuilderTracker* builderTracker) {
	list<TaskPlan>::iterator killplan;
	list<int>::iterator killBuilder;
	// make sure this builder is in a TaskPlan
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId != 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	builderTracker->taskPlanId = 0;
	int builder = builderTracker->builderID;
	bool found = false;
	bool found2 = false;

	for (int k = 0; k < LASTCATEGORY;k++) {
		for (list<TaskPlan>::iterator i = TaskPlans[k].begin(); i != TaskPlans[k].end(); i++) {
			for (list<int>::iterator j = i->builders.begin(); j != i->builders.end(); j++) {
				if (*j == builder) {
					killplan = i;
					killBuilder = j;
					assert(!found);
					found = true;
					found2 = true;
				}
			}
		}
		if (found2) {
			for (list<BuilderTracker*>::iterator i = killplan->builderTrackers.begin(); i != killplan->builderTrackers.end(); i++) {
				if (builderTracker == *i) {
					// give it time to change command
					builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame();
					killplan->builderTrackers.erase(i);
					break;
				}
			}

			killplan->builders.erase(killBuilder);

			if (killplan->builders.size() == 0) {
				// TaskPlan lost all its workers, remove
				if (ai->ut->GetCategory(killplan->def) == CAT_DEFENCE)
					ai->dm->RemoveDefense(killplan->pos, killplan->def);

				TaskPlans[k].erase(killplan);
			}

			found2 = false;
		}
	}

	if (!found) {
		assert(false);
	}
}

TaskPlan* CUnitHandler::GetTaskPlan(int taskPlanId) {
	for (int k = 0; k < LASTCATEGORY;k++) {
		for (list<TaskPlan>::iterator i = TaskPlans[k].begin(); i != TaskPlans[k].end(); i++) {
			if (i->id == taskPlanId)
				return  &*i;
		}
	}

	// this better not happen
	assert(false);
	return 0;
}



// not used
bool CUnitHandler::TaskPlanExist(float3 pos, const UnitDef* builtdef) {
	int category = ai->ut->GetCategory(builtdef);

	if (category == -1)
		return false;

	assert(category >= 0);
	assert(category < LASTCATEGORY);

	for (list<TaskPlan>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++) {
		if (i->pos.distance2D(pos) < 100 && ai->ut->GetCategory(i->def) == category) {
			return true;
		}
	}
	return false;
}





void CUnitHandler::MetalExtractorAdd(int extractorID) {
	if (ai->ut->GetCategory(extractorID) == CAT_MEX) {
		MetalExtractor newMex;
		newMex.id = extractorID;
		newMex.buildFrame = ai->cb->GetCurrentFrame();
		MetalExtractors.push_back(newMex);
	} else {
		assert(false);
	}
}

void CUnitHandler::MetalExtractorRemove(int extractorID) {
	for (vector<MetalExtractor>::iterator i = MetalExtractors.begin(); i != MetalExtractors.end(); i++) {
		if (i->id == extractorID) {
			MetalExtractors.erase(i);
			break;
		}
	}
}



inline bool CompareExtractors(const MetalExtractor& l, const MetalExtractor& r) {
	// higher frame means more recently built
	return (l.buildFrame < r.buildFrame);
}

// returns the ID of the oldest (in
// frames) metal extractor built
int CUnitHandler::GetOldestMetalExtractor(void) {
	std::sort(MetalExtractors.begin(), MetalExtractors.end(), &CompareExtractors);

	return (MetalExtractors.size() > 0)? (MetalExtractors.begin())->id: -1;
}



void CUnitHandler::NukeSiloAdd(int siloID) {
	if (ai->ut->GetCategory(siloID) == CAT_NUKE) {
		NukeSilo newSilo;
		newSilo.id = siloID;
		newSilo.numNukesReady = 0;
		newSilo.numNukesQueued = 0;
		NukeSilos.push_back(newSilo);
	} else {
		assert(false);
	}
}

void CUnitHandler::NukeSiloRemove(int siloID) {
	for (list<NukeSilo>::iterator i = NukeSilos.begin(); i != NukeSilos.end(); i++) {
		if (i->id == siloID) {
			NukeSilos.erase(i);
			break;
		}
	}
}




// add a new factory
void CUnitHandler::FactoryAdd(int factory) {
	if (ai->ut->GetCategory(factory) == CAT_FACTORY) {
		Factory newFact;
		newFact.id = factory;
		Factories.push_back(newFact);
	} else {
		assert(false);
	}
}

void CUnitHandler::FactoryRemove(int id) {
	list<Factory>::iterator iter;
	bool factoryFound = false;

	for (list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++) {
		if (i->id == id) {
			iter = i;
			factoryFound = true;
			break;
		}
	}
	if (factoryFound) {
		// remove all builders from this plan
		list<BuilderTracker*> builderTrackers = iter->supportBuilderTrackers;

		for (list<BuilderTracker*>::iterator i = builderTrackers.begin(); i != builderTrackers.end(); i++) {
			FactoryBuilderRemove(*i);
		}

		Factories.erase(iter);
	}
}



bool CUnitHandler::FactoryBuilderAdd(int builder) {
	BuilderTracker* builderTracker = GetBuilderTracker(builder);
	return FactoryBuilderAdd(builderTracker);
}



bool CUnitHandler::FactoryBuilderAdd(BuilderTracker* builderTracker) {
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);

	for (list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++) {
		float totalbuildercost = 0.0f;

		// HACK
		for (list<int>::iterator j = i->supportbuilders.begin(); j != i->supportbuilders.end(); j++) {
			totalbuildercost += ai->math->GetUnitCost(*j);
		}

		if (totalbuildercost < ai->math->GetUnitCost(i->id) * BUILDERFACTORYCOSTRATIO) {
			builderTracker->factoryId = i->id;
			i->supportbuilders.push_back(builderTracker->builderID);
			i->supportBuilderTrackers.push_back(builderTracker);
			ai->MyUnits[builderTracker->builderID]->Guard(i->id);
			return true;
		}
	}

	return false;
}

void CUnitHandler::FactoryBuilderRemove(BuilderTracker* builderTracker) {
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId != 0);
	assert(builderTracker->customOrderId == 0);
	list<Factory>::iterator killfactory;

	for (list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++) {
		if (builderTracker->factoryId == i->id) {
			i->supportbuilders.remove(builderTracker->builderID);
			i->supportBuilderTrackers.remove(builderTracker);
			builderTracker->factoryId = 0;
			// give it time to change command
			builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame();
		}
	}
}





void CUnitHandler::BuilderReclaimOrder(int builderId, float3 pos) {
	pos = pos;
	BuilderTracker* builderTracker = GetBuilderTracker(builderId);
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	// Just use taskPlanCounter for the id.
	builderTracker->customOrderId = taskPlanCounter++;
}
