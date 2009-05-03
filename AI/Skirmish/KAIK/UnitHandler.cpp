#include <sstream>

#include "IncCREG.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

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
	lastCapturedUnitFrame = -1;
	lastCapturedUnitID = -1;

	IdleUnits.resize(CAT_LAST);
	BuildTasks.resize(CAT_LAST);
	TaskPlans.resize(CAT_LAST);
	AllUnitsByCat.resize(CAT_LAST);

	if (ai) {
		AllUnitsByType.resize(ai->cb->GetNumUnitDefs() + 1);
		metalMaker = new CMetalMaker(ai);
	}
}

CUnitHandler::~CUnitHandler() {
	for (std::list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
		delete *i;
	}
}



void CUnitHandler::IdleUnitUpdate(int frame) {
	std::list<integer2> limboremoveunits;

	for (std::list<integer2>::iterator i = Limbo.begin(); i != Limbo.end(); i++) {
		if (i->y > 0) {
			i->y--;
		} else {
			if (ai->cb->GetUnitDef(i->x) == NULL) {
				// ignore dead unit
			} else {
				IdleUnits[ai->ut->GetCategory(i->x)].push_back(i->x);
			}

			limboremoveunits.push_back(*i);
		}
	}

	if (limboremoveunits.size()) {
		for (std::list<integer2>::iterator i = limboremoveunits.begin(); i != limboremoveunits.end(); i++) {
			Limbo.remove(*i);
		}
	}

	// make sure that all the builders are in action (hack?)
	if (frame % 15 == 0) {
		for (std::list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
			if ((*i)->idleStartFrame != -2) {
				// the brand new builders must be filtered still
				bool ans = VerifyOrder(*i);
				const int builderID = (*i)->builderID;
				const CCommandQueue* myCommands = ai->cb->GetCurrentUnitCommands(builderID);
				Command c;

				if (myCommands->size() > 0) {
					c = myCommands->front();
				}

				// two sec delay is ok
				if (((*i)->commandOrderPushFrame + LAG_ACCEPTANCE) < frame) {
					if (!ans) {
						float3 pos = ai->cb->GetUnitPos(builderID);

						std::stringstream msg;
							msg << "[CUnitHandler::IdleUnitUpdate()] frame " << frame << "\n";
							msg << "\tfailed to verify order for builder " << builderID;
							msg << " with " << (myCommands->size()) << " remaining commands\n";
						L(ai, msg.str());

						ClearOrder(*i, false);

						if (!myCommands->empty()) {
							DecodeOrder(*i, true);
						} else {
							IdleUnitAdd(builderID, frame);
						}
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
void CUnitHandler::UnitCreated(int unitID) {
	UnitCategory ucat = ai->ut->GetCategory(unitID);
	const UnitDef* udef = ai->cb->GetUnitDef(unitID);

	if (ucat != CAT_LAST) {
		AllUnitsByCat[ucat].push_back(unitID);
		AllUnitsByType[udef->id].push_back(unitID);

		if (ucat == CAT_FACTORY) {
			FactoryAdd(unitID);
		}

		BuildTaskCreate(unitID);

		if (ucat == CAT_BUILDER) {
			// add the new builder
			BuilderTracker* builderTracker = new BuilderTracker();
			builderTracker->builderID      = unitID;
			builderTracker->buildTaskId    = 0;
			builderTracker->taskPlanId     = 0;
			builderTracker->factoryId      = 0;
			builderTracker->stuckCount     = 0;
			builderTracker->customOrderId  = 0;
			// under construction
			builderTracker->commandOrderPushFrame = -2;
			builderTracker->categoryMaker = CAT_LAST;
			// wait for the first idle call, as this unit might be under construction
			builderTracker->idleStartFrame = -2;
			BuilderTrackers.push_back(builderTracker);
		}

		if (ucat == CAT_MMAKER) {
			MMakerAdd(unitID);
		}
		if (ucat == CAT_MEX) {
			MetalExtractorAdd(unitID);
		}

		if (ucat == CAT_NUKE) {
			NukeSiloAdd(unitID);
		}
	}

	if (udef->isCommander && udef->canDGun) {
		ai->dgunConHandler->AddController(unitID);
	} else {
		ai->MyUnits[unitID]->SetFireState(2);
	}
}

void CUnitHandler::UnitDestroyed(int unit) {
	UnitCategory category = ai->ut->GetCategory(unit);
	const UnitDef* unitDef = ai->cb->GetUnitDef(unit);

	if (category != CAT_LAST) {
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
			for (std::list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
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
	UnitCategory category = ai->ut->GetCategory(unit);

	if (category != CAT_LAST) {
		const CCommandQueue* myCommands = ai->cb->GetCurrentUnitCommands(unit);

		if (myCommands->empty()) {
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
		} else {
			// the unit has orders still, so should not be idle
			if (category == CAT_BUILDER) {
				if (false) {
					// KLOOTNOTE: somehow we are reaching this branch
					// on initialization when USE_CREG is not defined,
					// mycommands->size() returns garbage?
					//
					// BuilderTracker* builderTracker = GetBuilderTracker(unit);
					// DecodeOrder(builderTracker, true);
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

			commandFound =
				((c->id        == CMD_REPAIR                 ) &&
				 (c->params[0] == builderTracker->buildTaskId)) ||

				((c->id       == -buildTask->def->id) &&
				 (c->params[0] == buildTask->pos.x  ) &&
				 (c->params[2] == buildTask->pos.z  ));

			if (!commandFound) {
				return false;
			}
		}

		if (builderTracker->taskPlanId != 0) {
			assert(!hit);
			hit = true;
			TaskPlan* taskPlan = GetTaskPlan(builderTracker->taskPlanId);

			if ((c->id == -taskPlan->def->id)
					&& (c->params[0] == taskPlan->pos.x)
					&& (c->params[2] == taskPlan->pos.z))
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

	const int frame        = ai->cb->GetCurrentFrame();
	const int builderID    = builderTracker->builderID;
	const int buildTaskID  = builderTracker->buildTaskId;
	const int factoryID    = builderTracker->factoryId;
	const CCommandQueue* q = ai->cb->GetCurrentUnitCommands(builderID);

	assert(q->empty() || !reportError);

	if (buildTaskID != 0) {
		hit = true;
		BuildTask* buildTask = GetBuildTask(buildTaskID);

		std::stringstream msg;
			msg << "[CUnitHandler::ClearOrder()] frame " << frame << "\n";
			msg << "\tbuilder " << builderID << " is reported idle but";
			msg << " still has a build-task with ID " << (buildTaskID) << "\n";
		L(ai, msg.str());

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

		const TaskPlan*    taskPlan = GetTaskPlan(builderTracker->taskPlanId);
		const std::string& taskName = taskPlan->def->humanName;

		std::stringstream msg;
			msg << "[CUnitHandler::ClearOrder()] frame " << frame << "\n";
			msg << "\tbuilder " << builderID << " is reported idle but";
			msg << " still has a task-plan named \"" << (taskName) << "\"\n";
		L(ai, msg.str());

		// mask this build-spot as bad
		ai->dm->MaskBadBuildSpot(taskPlan->pos);

		// TODO: remove all builders from this plan
		if (reportError) {
			std::list<BuilderTracker*> builderTrackers = taskPlan->builderTrackers;
			for (std::list<BuilderTracker*>::iterator i = builderTrackers.begin(); i != builderTrackers.end(); i++) {
				TaskPlanRemove(*i);
				ai->MyUnits[(*i)->builderID]->Stop();
			}
		} else {
			TaskPlanRemove(builderTracker);
		}
	}

	if (factoryID != 0) {
		assert(!hit);
		hit = true;

		std::stringstream msg;
			msg << "[CUnitHandler::ClearOrder()] frame " << frame << "\n";
			msg << "\tbuilder " << builderID << " is reported idle but";
			msg << " still has a factory ID of " << factoryID << "\n";
		L(ai, msg.str());

		// remove the builder from its job
		FactoryBuilderRemove(builderTracker);
	}

	if (builderTracker->customOrderId != 0) {
		assert(!hit);
		hit = true;
		// why is this builder idle?
		// no tracking of custom orders yet
		//SNPRINTF(logMsg, logMsg_maxSize,
		//		"builder %i: was idle, but it is on customOrderId: %i (removing the builder from the job)",
		//		unit, builderTracker->customOrderId);
		//PRINTF("%s", logMsg);
		builderTracker->customOrderId = 0;
	}

	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
}




void CUnitHandler::DecodeOrder(BuilderTracker* builderTracker, bool reportError) {
	const int            frame     = ai->cb->GetCurrentFrame();
	const int            builderID = builderTracker->builderID;
	const CCommandQueue* builderQ  = ai->cb->GetCurrentUnitCommands(builderID);

	if (builderQ->size() > 0) {
		// builder has orders
		const Command* c   = &builderQ->front();
		const int      n   = c->params.size();
		const int      cID = c->id;

		if (builderQ->size() == 2 && cID == CMD_MOVE) {
			// it might have a move order before the real order,
			// take command nr. 2 if nr. 1 is a move order
			c = &builderQ->back();
		}

		if (reportError) {
			std::stringstream msg;
				msg << "[CUnitHandler::DecodeOrder()] frame " << frame << "\n";
				msg << "\tbuilder " << builderID << " claimed idle, but has";
				msg << " command " << cID << " with " << n << " parameters";
				msg << " (params[0]: " << ((n > 0)? c->params[0]: -1) << ")\n";
			L(ai, msg.str());
		}

		if (cID < 0) {
			assert(n >= 3);

			// it's building a unit
			float3 newUnitPos;
			newUnitPos.x = c->params[0];
			newUnitPos.y = c->params[1];
			newUnitPos.z = c->params[2];

			const UnitDef* newUnitDef = ai->ut->unitTypes[-cID].def;
			// make sure that no BuildTasks exists there
			BuildTask* buildTask = BuildTaskExist(newUnitPos, newUnitDef);

			if (buildTask) {
				BuildTaskAddBuilder(buildTask, builderTracker);
			} else {
				// make a new TaskPlan (or join an existing one)
				TaskPlanCreate(builderID, newUnitPos, newUnitDef);
			}
		}

		if (cID == CMD_REPAIR) {
			assert(n >= 1);

			// it's repairing, find the unit being repaired
			int guardingID = int(c->params[0]);
			bool found = false;

			UnitCategory cat = ai->ut->GetCategory(guardingID);
			std::list<BuildTask>::iterator i;

			if (cat == CAT_LAST) {
				return;
			}

			for (i = BuildTasks[cat].begin(); i != BuildTasks[cat].end(); i++) {
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
	} else {
		// error: this function needs a builder with orders
		// should not be possible because IdleUnitUpdate()
		// calls us only if a unit's command-queue is NOT
		// empty?
		// assert(false);
		std::stringstream msg;
			msg << "[CUnitHandler::DecodeOrder()] frame " << frame << "\n";
			msg << "\tbuilder " << builderID << " should not have an empty queue!\n";
		L(ai, msg.str());
	}
}



void CUnitHandler::IdleUnitRemove(int unit) {
	UnitCategory category = ai->ut->GetCategory(unit);

	if (category != CAT_LAST) {
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

		std::list<integer2>::iterator tempunit;
		bool found = false;

		for (std::list<integer2>::iterator i = Limbo.begin(); i != Limbo.end(); i++) {
			if (i->x == unit) {
				tempunit = i;
				found = true;
			}
		}

		if (found) {
			Limbo.erase(tempunit);
		}
	}
}


int CUnitHandler::GetIU(UnitCategory category) {
	assert(IdleUnits[category].size() > 0);
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

int CUnitHandler::NumIdleUnits(UnitCategory category) {
	assert(category < CAT_LAST);
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
	UnitCategory category = ai->ut->GetCategory(id);
	float3 pos = ai->cb->GetUnitPos(id);

	if ((!newUnitDef->movedata || category == CAT_DEFENCE) && !newUnitDef->canfly && category != CAT_LAST) {
		// this needs to change, so that it can make more stuff
		if (category >= CAT_LAST) {
			return;
		}

		BuildTask bt;
		bt.id = -1;

		std::list<TaskPlan>::iterator i;
		for (i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++) {
			if (pos.distance2D(i->pos) < 1.0f && newUnitDef == i->def) {
				 // there can not be more than one found TaskPlan
				assert(bt.id == -1);

				bt.category = category;
				bt.id       = id;
				bt.pos      = i->pos;
				bt.def      = newUnitDef;

				std::list<BuilderTracker*> moveList;

				for (std::list<BuilderTracker*>::iterator builder = i->builderTrackers.begin(); builder != i->builderTrackers.end(); builder++) {
					moveList.push_back(*builder);
				}

				for (std::list<BuilderTracker*>::iterator builder = moveList.begin(); builder != moveList.end(); builder++) {
					TaskPlanRemove(*builder);
					BuildTaskAddBuilder(&bt, *builder);
				}

				i = TaskPlans[category].begin();
			}
		}

		if (bt.id == -1) {
			// buildtask creation error (can happen if builder manages
			// to restart a dead building, or a human has taken control),
			// make one anyway
			std::stringstream msg;
				msg << "[CUnitHandler::BuildTaskCreate()] frame " << (ai->cb->GetCurrentFrame()) << "\n";
				msg << "\tBuildTask Creation Error for task with ID " << id << "\n";
			L(ai, msg.str());

			if (category == CAT_DEFENCE) {
				ai->dm->AddDefense(pos, newUnitDef);
			}

			bt.category = category;
			bt.id = id;
			bt.pos = pos;
			bt.def = newUnitDef;

			// if we have any friendly builders
			for (std::list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
				BuilderTracker* builderTracker = *i;

				// check what builder is doing
				const CCommandQueue* cq = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);

				if (!cq->empty()) {
					Command c = cq->front();

					const bool b0 = (c.id == -newUnitDef->id && c.params[0] == pos.x && c.params[2] == pos.z); // at this pos
					const bool b1 = (c.id == CMD_REPAIR && c.params[0] == id); // at this unit (id)
					const bool b2 = (c.id == CMD_GUARD  && c.params[0] == id); // at this unit (id)
					const bool b3 = b0 || b1 || b2;

					if (b3) {
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
						if (builderTracker->idleStartFrame == -2) {
							IdleUnitRemove(builderTracker->builderID);
						}

						// add it to this task
						BuildTaskAddBuilder(&bt, builderTracker);

						msg.str("");
							msg << "\tadded builder " << builderTracker->builderID << " to";
							msg << " build-task with ID " << builderTracker->buildTaskId << "\n";
						L(ai, msg.str());
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
	UnitCategory category = ai->ut->GetCategory(id);

	if (category < CAT_LAST) {
		std::list<BuildTask>::iterator killtask;
		bool found = false;

		for (std::list<BuildTask>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
			if (i->id == id) {
				killtask = i;
				assert(!found);
				found = true;
			}
		}
		if (found) {
			// remove the builders from this BuildTask
			std::list<BuilderTracker*> removeList;
			for (std::list<BuilderTracker*>::iterator builder = killtask->builderTrackers.begin(); builder != killtask->builderTrackers.end(); builder++) {
				removeList.push_back(*builder);
			}
			for (std::list<BuilderTracker*>::iterator builder = removeList.begin(); builder != removeList.end(); builder++) {
				BuildTaskRemove(*builder);
			}

			BuildTasks[category].erase(killtask);
		}
	}
}


BuilderTracker* CUnitHandler::GetBuilderTracker(int builder) {
	for (std::list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++) {
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

	UnitCategory category = ai->ut->GetCategory(builderTracker->buildTaskId);

	// TODO: Hack fix
	if (category == CAT_LAST) {
		return;
	}

	assert(category >= 0);
	assert(category < CAT_LAST);
	assert(builderTracker->buildTaskId != 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	// std::list<BuildTask>::iterator killtask;
	bool found = false;
	bool found2 = false;

	for (std::list<BuildTask>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
		if (i->id == builderTracker->buildTaskId){
			// killtask = i;
			assert(!found);
			for (std::list<int>::iterator builder = i->builders.begin(); builder != i->builders.end(); builder++) {
				if (builderTracker->builderID == *builder) {
					assert(!found2);
					i->builders.erase(builder);
					builderTracker->buildTaskId = 0;
					found2 = true;
					break;
				}
			}
			for (std::list<BuilderTracker*>::iterator builder = i->builderTrackers.begin(); builder != i->builderTrackers.end(); builder++) {
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
	for (int k = 0; k < CAT_LAST; k++) {
		for (std::list<BuildTask>::iterator i = BuildTasks[k].begin(); i != BuildTasks[k].end(); i++) {
			if (i->id == buildTaskId)
				return  &*i;
		}
	}

	// this better not happen
	assert(false);
	return 0;
}



BuildTask* CUnitHandler::BuildTaskExist(float3 pos, const UnitDef* builtdef) {
	UnitCategory category = ai->ut->GetCategory(builtdef);

	if (category >= CAT_LAST) {
		return NULL;
	}

	for (std::list<BuildTask>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
		if (i->pos.distance2D(pos) < 1 && ai->ut->GetCategory(i->def) == category) {
			return &*i;
		}
	}

	return NULL;
}



bool CUnitHandler::BuildTaskAddBuilder(int builderID, UnitCategory category) {
	assert(category < CAT_LAST);
	assert(builderID >= 0);
	assert(ai->MyUnits[builderID] != NULL);

	CUNIT* u = ai->MyUnits[builderID];
	BuilderTracker* builderTracker = GetBuilderTracker(builderID);
	const UnitDef* builderDef = ai->cb->GetUnitDef(builderID);
	const int frame = ai->cb->GetCurrentFrame();

	// make sure this builder is free
	const bool b1 = (builderTracker->taskPlanId == 0);
	const bool b2 = (builderTracker->buildTaskId == 0);
	const bool b3 = (builderTracker->factoryId == 0);
	const bool b4 = (builderTracker->customOrderId == 0);
	const bool b5 = builderDef->canAssist;
	const bool b6 = (category == CAT_FACTORY && frame >= 18000);

	if (!b1 || !b2 || !b3 || !b4 || !b5) {
		if (b6) {
			// note that FactoryBuilderAdd() asserts b1 through b4
			// immediately after BuildTaskAddBuilder() is tried and
			// fails in BuildUp(), so at least those must be true
			// (and so should b5 in most of the *A mods)

			std::stringstream msg;
				msg << "[CUnitHandler::BuildTaskAddBuilder()] frame " << frame << "\n";
				msg << "\tbuilder " << builderID << " not able to be added to CAT_FACTORY build-task\n";
				msg << "\tb1: " << b1 << ", b2: " << b2 << ", b3: " << b3;
				msg << ", b4: " << b4 << ", b5: " << b5 << ", b6: " << b6;
			L(ai, msg.str());
		}
		return false;
	}

	// see if there are any BuildTasks that it can join
	if (BuildTasks[category].size() > 0) {
		float largestTime = 0.0f;
		std::list<BuildTask>::iterator task;
		std::list<BuildTask>::iterator bestTask;

		for (task = BuildTasks[category].begin(); task != BuildTasks[category].end(); task++) {
			float buildTime = ai->math->ETT(*task) - ai->math->ETA(builderID, ai->cb->GetUnitPos(task->id));

			if (buildTime > largestTime) {
				largestTime = buildTime;
				bestTask = task;
			}
		}

		if (largestTime > 0.0f) {
			BuildTaskAddBuilder(&*bestTask, builderTracker);
			u->Repair(bestTask->id);
			return true;
		}
	}

	// see if there any joinable TaskPlans
	if (TaskPlans[category].size() > 0) {
		float largestTime = 0.0f;
		std::list<TaskPlan>::iterator plan;
		std::list<TaskPlan>::iterator bestPlan;

		for (plan = TaskPlans[category].begin(); plan != TaskPlans[category].end(); plan++) {
			float buildTime = (plan->def->buildTime / plan->currentBuildPower) - ai->math->ETA(builderID, plan->pos);

			// must test if this builder can make this unit/building too
			if (buildTime > largestTime) {
				const std::vector<int>* canBuildList = &ai->ut->unitTypes[builderDef->id].canBuildList;
				const int buildListSize = canBuildList->size();

				for (int j = 0; j < buildListSize; j++) {
					if (canBuildList->at(j) == plan->def->id) {
						largestTime = buildTime;
						bestPlan = plan;
						break;
					}
				}
			}
		}

		if (largestTime > 10.0f) {
			assert(builderID >= 0);

			// bad, CUNIT::Build() uses TaskPlanCreate()
			// should we really give build orders here?
			// return u->Build(bestPlan->pos, bestPlan->def, -1);
			// TaskPlanCreate(builderID, bestPlan->pos, bestPlan->def);
			return true;
		}
	}

	if (b6) {
		std::stringstream msg;
			msg << "[CUnitHandler::BuildTaskAddBuilder()] frame " << frame << "\n";
			msg << "\tno joinable CAT_FACTORY build-tasks or task-plans for builder " << builderID;
		L(ai, msg.str());
	}

	return false;
}




void CUnitHandler::TaskPlanCreate(int builder, float3 pos, const UnitDef* builtdef) {
	UnitCategory category = ai->ut->GetCategory(builtdef);

	// HACK
	if (category >= CAT_LAST) {
		return;
	}

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


	bool existingTP = false;
	for (std::list<TaskPlan>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++) {
		if (pos.distance2D(i->pos) < 20.0f && builtdef == i->def) {
			// make sure there are no other TaskPlans
			if (!existingTP) {
				existingTP = true;
				TaskPlanAdd(&*i, builderTracker);
			} else {
				std::stringstream msg;
					msg << "[CUnitHandler::TaskPlanCreate()] frame " << ai->cb->GetCurrentFrame() << "\n";
					msg << "\ttask-plan for \"" << builtdef->humanName << "\" already present";
					msg << " at position <" << pos.x << ", " << pos.y << ", " << pos.z << ">\n";
				L(ai, msg.str());
			}
		}
	}
	if (!existingTP) {
		TaskPlan tp;
		tp.pos = pos;
		tp.def = builtdef;
		tp.defName = builtdef->name;
		tp.currentBuildPower = 0;
		tp.id = taskPlanCounter++;
		TaskPlanAdd(&tp, builderTracker);

		if (category == CAT_DEFENCE)
			ai->dm->AddDefense(pos, builtdef);

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
	std::list<TaskPlan>::iterator killplan;
	std::list<int>::iterator killBuilder;
	// make sure this builder is in a TaskPlan
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId != 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	builderTracker->taskPlanId = 0;
	int builder = builderTracker->builderID;
	bool found = false;
	bool found2 = false;

	for (int k = 0; k < CAT_LAST;k++) {
		for (std::list<TaskPlan>::iterator i = TaskPlans[k].begin(); i != TaskPlans[k].end(); i++) {
			for (std::list<int>::iterator j = i->builders.begin(); j != i->builders.end(); j++) {
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
			for (std::list<BuilderTracker*>::iterator i = killplan->builderTrackers.begin(); i != killplan->builderTrackers.end(); i++) {
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
	for (int k = 0; k < CAT_LAST;k++) {
		for (std::list<TaskPlan>::iterator i = TaskPlans[k].begin(); i != TaskPlans[k].end(); i++) {
			if (i->id == taskPlanId)
				return  &*i;
		}
	}

	// this better not happen
	assert(false);
	return 0;
}



bool CUnitHandler::TaskPlanExist(float3 pos, const UnitDef* builtdef) {
	UnitCategory category = ai->ut->GetCategory(builtdef);

	if (category >= CAT_LAST) {
		return false;
	}

	for (std::list<TaskPlan>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++) {
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
	for (std::vector<MetalExtractor>::iterator i = MetalExtractors.begin(); i != MetalExtractors.end(); i++) {
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
	for (std::list<NukeSilo>::iterator i = NukeSilos.begin(); i != NukeSilos.end(); i++) {
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
	std::list<Factory>::iterator iter;
	bool factoryFound = false;

	for (std::list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++) {
		if (i->id == id) {
			iter = i;
			factoryFound = true;
			break;
		}
	}
	if (factoryFound) {
		// remove all builders from this plan
		std::list<BuilderTracker*> builderTrackers = iter->supportBuilderTrackers;

		for (std::list<BuilderTracker*>::iterator i = builderTrackers.begin(); i != builderTrackers.end(); i++) {
			FactoryBuilderRemove(*i);
		}

		Factories.erase(iter);
	}
}



bool CUnitHandler::FactoryBuilderAdd(int builderID) {
	bool b = (ai->MyUnits[builderID]->def())->canAssist;
	BuilderTracker* builderTracker = GetBuilderTracker(builderID);
	return (b && FactoryBuilderAdd(builderTracker));
}



bool CUnitHandler::FactoryBuilderAdd(BuilderTracker* builderTracker) {
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);

	for (std::list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++) {
		CUNIT* u = ai->MyUnits[i->id];

		// don't assist hubs (or factories that cannot be assisted)
		if ((u->def())->canBeAssisted && !u->isHub()) {
			float totalBuilderCost = 0.0f;

			// HACK: get the sum of the heuristic costs of every
			// builder that is already assisting this factory
			for (std::list<int>::iterator j = i->supportbuilders.begin(); j != i->supportbuilders.end(); j++) {
				if ((ai->MyUnits[*j]->def())->isCommander) {
					continue;
				}

				totalBuilderCost += ai->math->GetUnitCost(*j);
			}

			// if this sum is less than the heuristic cost of the
			// factory itself, add the builder to this factory
			//
			// this is based purely on the metal and energy costs
			// of all involved parties, and silently expects that
			// building _another_ factory would always be better
			// than assisting it further
			if (totalBuilderCost < (ai->math->GetUnitCost(i->id) * BUILDERFACTORYCOSTRATIO * 2.5f)) {
				builderTracker->factoryId = i->id;
				i->supportbuilders.push_back(builderTracker->builderID);
				i->supportBuilderTrackers.push_back(builderTracker);
				ai->MyUnits[builderTracker->builderID]->Guard(i->id);
				return true;
			}
		}
	}

	return false;
}

void CUnitHandler::FactoryBuilderRemove(BuilderTracker* builderTracker) {
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId != 0);
	assert(builderTracker->customOrderId == 0);
	std::list<Factory>::iterator killfactory;

	for (std::list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++) {
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






UpgradeTask* CUnitHandler::CreateUpgradeTask(int oldBuildingID, const float3& oldBuildingPos, const UnitDef* newBuildingDef) {
	assert(FindUpgradeTask(oldBuildingID) == NULL);

	// UpdateTasks() handles the deletion
	UpgradeTask* task = new UpgradeTask(oldBuildingID, ai->cb->GetCurrentFrame(), oldBuildingPos, newBuildingDef);
	upgradeTasks[oldBuildingID] = task;

	return task;
}

UpgradeTask* CUnitHandler::FindUpgradeTask(int oldBuildingID) {
	std::map<int, UpgradeTask*>::iterator it = upgradeTasks.find(oldBuildingID);

	if (it != upgradeTasks.end()) {
		return (it->second);
	}

	return NULL;
}

void CUnitHandler::RemoveUpgradeTask(UpgradeTask* task) {
	assert(task != NULL);
	assert(FindUpgradeTask(task->oldBuildingID) != NULL);

	upgradeTasks.erase(task->oldBuildingID);
	delete task;
}

bool CUnitHandler::AddUpgradeTaskBuilder(UpgradeTask* task, int builderID) {
	std::set<int>::iterator it = task->builderIDs.find(builderID);

	if (it == task->builderIDs.end()) {
		task->builderIDs.insert(builderID);
		return true;
	}

	return false;
}

void CUnitHandler::UpdateUpgradeTasks(int frame) {
	std::map<int, UpgradeTask*>::iterator upgradeTaskIt;

	std::list<UpgradeTask*> deadTasks;
	std::list<UpgradeTask*>::iterator deadTasksIt;

	for (upgradeTaskIt = upgradeTasks.begin(); upgradeTaskIt != upgradeTasks.end(); upgradeTaskIt++) {
		UpgradeTask* task = upgradeTaskIt->second;

		const int oldBuildingID = task->oldBuildingID;
		const bool oldBuildingDead =
			(ai->cb->GetUnitDef(oldBuildingID) == NULL) ||
			(ai->cb->GetUnitHealth(oldBuildingID) < 0.0f);

		std::set<int>::iterator builderIDsIt;
		std::list<int> deadBuilderIDs;
		std::list<int>::iterator deadBuilderIDsIt;


		for (builderIDsIt = task->builderIDs.begin(); builderIDsIt != task->builderIDs.end(); builderIDsIt++) {
			const int builderID = *builderIDsIt;
			const bool builderDead =
				(ai->cb->GetUnitDef(builderID) == NULL) ||
				(ai->cb->GetUnitHealth(builderID) <= 0.0f);

			if (builderDead) {
				deadBuilderIDs.push_back(builderID);
			} else {
				const CCommandQueue* cq = ai->cb->GetCurrentUnitCommands(builderID);

				if (oldBuildingDead) {
					if (!task->reclaimStatus) {
						task->creationFrame = frame;
						task->reclaimStatus = true;
					}

					// give a build order for the replacement structure
					if (cq->size() == 0 || ((cq->front()).id != -task->newBuildingDef->id)) {
						std::stringstream msg;
							msg << "[CUnitHandler::UpdateUpgradeTasks()] frame " << frame << "\n";
							msg << "\tgiving build order for \"" << task->newBuildingDef->humanName;
							msg << "\" to builder " << builderID << "\n";
						L(ai, msg.str());

						ai->MyUnits[builderID]->Build_ClosestSite(task->newBuildingDef, task->oldBuildingPos);
					}
				} else {
					// give a reclaim order for the original structure
					if (cq->size() == 0 || ((cq->front()).id != CMD_RECLAIM)) {
						std::stringstream msg;
							msg << "[CUnitHandler::UpdateUpgradeTasks()] frame " << frame << "\n";
							msg << "\tgiving reclaim order for \"" << ai->cb->GetUnitDef(oldBuildingID)->humanName;
							msg << "\" to builder " << builderID << "\n";
						L(ai, msg.str());

						ai->MyUnits[builderID]->Reclaim(oldBuildingID);
					}
				}
			}
		}


		// filter any dead builders from the upgrade task
		// (probably should tie this into UnitDestroyed())
		for (deadBuilderIDsIt = deadBuilderIDs.begin(); deadBuilderIDsIt != deadBuilderIDs.end(); deadBuilderIDsIt++) {
			task->builderIDs.erase(*deadBuilderIDsIt);
		}

		if (oldBuildingDead) {
			// all builders have been given a build order
			// for the replacement structure at this point,
			// so the task itself is no longer needed
			deadTasks.push_back(task);
		}
	}

	for (deadTasksIt = deadTasks.begin(); deadTasksIt != deadTasks.end(); deadTasksIt++) {
		RemoveUpgradeTask(*deadTasksIt);
	}
}
