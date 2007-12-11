#include "UnitHandler.h"
#include "MetalMaker.h"
#include "creg/STL_List.h"

#undef assert
#define assert(a) if (!(a)) throw "Assertion " #a "faild";

CR_BIND(CUnitHandler ,(NULL))

CR_REG_METADATA(CUnitHandler, (
				CR_MEMBER(IdleUnits),

				CR_MEMBER(BuildTasks),
				CR_MEMBER(TaskPlans),
				CR_MEMBER(AllUnitsByCat),
				CR_MEMBER(AllUnitsByType),
				CR_MEMBER(Factories),

				CR_MEMBER(UncloakedBuildings),
				CR_MEMBER(UncloakedUnits),
				CR_MEMBER(CloakedBuildings),
				CR_MEMBER(CloakedUnits),

				CR_MEMBER(StockpileDefenceUnits),
				CR_MEMBER(StockpileAttackUnits),

				CR_MEMBER(Limbo),
				
				CR_MEMBER(BuilderTrackers),

				CR_MEMBER(metalMaker),
				CR_MEMBER(ai),
				CR_MEMBER(taskPlanCounter),
				CR_RESERVED(256)
				//CR_MEMBER(debugPoints)
				));

CUnitHandler::CUnitHandler(AIClasses* ai)
{
	this->ai=ai;
	IdleUnits.resize(LASTCATEGORY);
	BuildTasks.resize(LASTCATEGORY);
	TaskPlans.resize(LASTCATEGORY);
	AllUnitsByCat.resize(LASTCATEGORY);
	AllUnitsByType.resize(ai?ai->cb->GetNumUnitDefs()+1:0);
//	for(int i = 0; i <= ai->cb->GetNumUnitDefs(); i++){
//		AllUnitsByType[i] = new list<int>;
//	}
//	for(int i = 0; i < LASTCATEGORY; i++){
//		IdleUnits[i] = new list<int>;
//		BuildTasks[i] = new list<BuildTask*>;
//		TaskPlans[i] = new list<TaskPlan*>;
//		AllUnitsByCat[i] = new list<int>;
//	}
	taskPlanCounter = 1;
	if (ai) metalMaker = new CMetalMaker(ai);
	debugPoints = false;

}
CUnitHandler::~CUnitHandler()
{
	
	
	for(int i = 0; i < LASTCATEGORY; i++){
//		delete IdleUnits[i];
		for(list<BuildTask*>::iterator j = BuildTasks[i].begin(); j != BuildTasks[i].end(); j++){
			delete *j;
		}
//		delete BuildTasks[i];
		for(list<TaskPlan*>::iterator j = TaskPlans[i].begin(); j != TaskPlans[i].end(); j++){
			delete *j;
		}
//		delete TaskPlans[i];
//		delete AllUnitsByCat[i];
	}
//	for(int i = 0; i <= ai->ut->numOfUnits; i++){
//		delete AllUnitsByType[i];
//	}
	for(list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++){
		delete *i;
	}
	
}

void CUnitHandler::IdleUnitUpdate()
{
	list<integer2> limboremoveunits;
	for(list<integer2>::iterator i = Limbo.begin(); i != Limbo.end(); i++){
		if(i->y > 0){
			i->y = i->y - 1;
		}
		else{
			L("adding unit to idle units: " << i->x);
			if(ai->cb->GetUnitDef(i->x) == NULL)
			{
				L(" Removeing dead unit... ");
				
			}
			else {
				IdleUnits[ai->ut->GetCategory(i->x)].push_back(i->x);
				if(ai->ut->GetCategory(i->x) == CAT_BUILDER)
				{
					//GetBuilderTracker(i->x)->idleStartFrame = -1; // Its ok now, stop the force idle  (hack)
				}
			}
			
				
			limboremoveunits.push_back(*i);
		}
	}
	if(limboremoveunits.size()){
		for(list<integer2>::iterator i = limboremoveunits.begin(); i != limboremoveunits.end(); i++){
			Limbo.remove(*i);
		}
	}
	// Make shure that all the builders are in action:   (hack ?)
	if(ai->cb->GetCurrentFrame() % 15 == 0)
	{
		L("VerifyOrders");
		for(list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++){
			
			// The new test:
			if((*i)->idleStartFrame != -2) // The brand new builders must be filtered still
			{
				//L("VerifyOrder");
				assert((*i)->def == ai->cb->GetUnitDef((*i)->builderID) );
				bool ans = VerifyOrder(*i);
				const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands((*i)->builderID);
				const Command *c;
				if(mycommands->size() > 0)
					c = &mycommands->front();
				
				// Two sec delay is ok
				if(( (*i)->commandOrderPushFrame + LAG_ACCEPTANCE) < ai->cb->GetCurrentFrame())
				{
					//assert(ans);
					if(!ans)
					{
						if(debugPoints) {
							char text[512];
							float3 pos = ai->cb->GetUnitPos((*i)->builderID);
							sprintf(text, "builder %i VerifyOrder failed ", (*i)->builderID);
							AIHCAddMapPoint amp;
							amp.label = text;
							amp.pos = pos;
							ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
						}
						//ai->debug->MakeMapPoint(&pos, string("builder ") << (*i)->builderID << "VerifyOrder failed"));
						
						ClearOrder(*i, false);
						if(!mycommands->empty())
							DecodeOrder(*i, true);
						else // Its idle
							IdleUnitAdd((*i)->builderID);
							
					}
				}
			}
			
			/*
			if((*i)->buildTaskId == 0 && (*i)->taskPlanId == 0 && (*i)->factoryId == 0 && (*i)->customOrderId == 0 && (*i)->idleStartFrame != -2)
			{
				// Its without tasks and not in the idle list
				if((*i)->idleStartFrame == -1)
				{
					(*i)->idleStartFrame = ai->cb->GetCurrentFrame();
				}
				else
				{
					if(((*i)->idleStartFrame + 90) < ai->cb->GetCurrentFrame())
					{
						L("Idle unit command force: " << (*i)->builderID);
						char text[512];
						float3 pos = ai->cb->GetUnitPos((*i)->builderID);
						sprintf(text, "builder %i without tasks and not in the idle list (idle forced)", (*i)->builderID);
						AIHCAddMapPoint amp;
						amp.label = text;
						amp.pos = pos;
						ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
						IdleUnitAdd((*i)->builderID);
						(*i)->idleStartFrame =  ai->cb->GetCurrentFrame(); // Just to cut down on the spam, if its not idle
					}
				}
			} else if((*i)->idleStartFrame != -2) // This will only be ok if idleStartFrame is changed to -1 on unit UnitFinished
			{
			*/
			/*
				L("VerifyOrder");
				bool ans = VerifyOrder(*i);
				const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands((*i)->builderID);
				Command c;
				if(mycommands->size() > 0)
					c = mycommands->front();
				
				// Two sec delay is ok
				if(( (*i)->commandOrderPushFrame + 30 * 2) < ai->cb->GetCurrentFrame())
				{
					//assert(ans);
					if(!ans)
					{
						char text[512];
						float3 pos = ai->cb->GetUnitPos((*i)->builderID);
						sprintf(text, "builder %i VerifyOrder failed ", (*i)->builderID);
						AIHCAddMapPoint amp;
						amp.label = text;
						amp.pos = pos;
						ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
					}
				}
				* /
			}
			*/
		}
	}
}

void CUnitHandler::UnitMoveFailed(int unit)
{
}

void CUnitHandler::UnitCreated(int unit)
{
	int category = ai->ut->GetCategory(unit);
	const UnitDef* newUnitDef = ai->cb->GetUnitDef(unit);
	L("Unit " << unit << " created ( " << newUnitDef->humanName << " ), ID : " << newUnitDef->id << " Cat: " << category);
	if(category != -1){
		AllUnitsByCat[category].push_back(unit);
		AllUnitsByType[newUnitDef->id].push_back(unit);
		//L("push sucessful");
		if(category == CAT_FACTORY){
			if (AllUnitsByCat[CAT_FACTORY].size()<=1) ai->dgunController->startingPos = ai->cb->GetUnitPos(unit);
			FactoryAdd(unit);
		}
		BuildTaskCreate(unit);	
		if(category == CAT_BUILDER){
			// Add the new builder
			BuilderTracker* builderTracker = new BuilderTracker;
			builderTracker->builderID = unit;
			builderTracker->buildTaskId = 0;
			builderTracker->taskPlanId = 0;
			builderTracker->factoryId = 0;
			builderTracker->stuckCount = 0;
			builderTracker->customOrderId = 0;
			builderTracker->commandOrderPushFrame = -2; // Under construction
			builderTracker->categoryMaker = -1;
			builderTracker->idleStartFrame = -2; // Wait for the first idle call, as this unit might be under construction
			builderTracker->def = newUnitDef;
			BuilderTrackers.push_back(builderTracker);
		}
		
		if(category == CAT_MMAKER || category == CAT_MEX){
			MMakerAdd(unit);
		}

		if(category == CAT_NUKE){
			StockpileAttackUnits.push_back(unit);
			ai->MyUnits[unit]->SetFiringMode(2);
		}
		if(category == CAT_ANTINUKE){
			StockpileDefenceUnits.push_back(unit);
		}
	}
	if (newUnitDef->canCloak) {
//		L("New unit can cloak");
		if (newUnitDef->speed) {
			if (newUnitDef->startCloaked) CloakedUnits.push_back(unit);
			else UncloakedUnits.push_back(unit);
		} else {
			if (newUnitDef->startCloaked) ;//CloakedBuildings.push_back(unit);
			else UncloakedBuildings.push_back(unit);
		}
	}
}

void CUnitHandler::UnitDestroyed(int unit)
{
	int category = ai->ut->GetCategory(unit);
	const UnitDef* unitDef = ai->cb->GetUnitDef(unit);
	L("Unit " << unit << " destroyed ( " << unitDef->humanName << " ), ID : " << unitDef->id << " Cat: " << category);
	if(category != -1){
		AllUnitsByType[unitDef->id].remove(unit);
		AllUnitsByCat[category].remove(unit);
		IdleUnitRemove(unit);
		BuildTaskRemove(unit);
		if(category == CAT_DEFENCE){
			ai->dm->RemoveDefense(ai->cb->GetUnitPos(unit),unitDef);
		}
		if(category == CAT_MMAKER || category == CAT_MEX){
			MMakerRemove(unit);
		}
		if(category == CAT_FACTORY)
		{
			FactoryLost(unit);
		}
		
		if(category == CAT_BUILDER)
		{
			// Remove the builder
			L("Removeing builder (its dead): " << unit);
			for(list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++){
				if((*i)->builderID == unit)
				{
					if((*i)->buildTaskId)
						BuildTaskRemoved(*i);
					if((*i)->taskPlanId)
						TaskPlanRemoved(*i);
					if((*i)->factoryId)
						FactoryBuilderRemoved(*i);
					BuilderTracker* builderTracker = *i;
					BuilderTrackers.erase(i);
					delete builderTracker; // Test this
					break;
				}
			}
			// Verify that its gone:
			for(list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++){
				if((*i)->builderID == unit)
				{
					L("Fatal error: 'void CUnitHandler::UnitDestroyed(int unit)'   failed to remove builder: " << unit);
				}
			}
			
			
		}
		if(category == CAT_NUKE){
			for (vector<int>::iterator i=StockpileAttackUnits.begin();i!=StockpileAttackUnits.end();i++)
				if (*i==unit) {
					StockpileAttackUnits.erase(i);
					break;
				}
		}
		if(category == CAT_ANTINUKE){
			for (vector<int>::iterator i=StockpileDefenceUnits.begin();i!=StockpileDefenceUnits.end();i++)
				if (*i==unit) {
					StockpileDefenceUnits.erase(i);
					break;
				}
		}
	}
	for (vector<int>::iterator i=CloakedUnits.begin();i!=CloakedUnits.end();i++) 
		if (*i==unit) {
//			L("Cloaked unit deleted");
			CloakedUnits.erase(i);
			break;
		}
	for (vector<int>::iterator i=UncloakedUnits.begin();i!=UncloakedUnits.end();i++) 
		if (*i==unit) {
//			L("Uncloaked unit deleted");
			UncloakedUnits.erase(i);
			break;
		}
	for (vector<int>::iterator i=CloakedBuildings.begin();i!=CloakedBuildings.end();i++) 
		if (*i==unit) {
//			L("Cloaked building deleted");
			CloakedBuildings.erase(i);
			break;
		}
	for (vector<int>::iterator i=UncloakedBuildings.begin();i!=UncloakedBuildings.end();i++) 
		if (*i==unit) {
//			L("Uncloaked building deleted");
			UncloakedBuildings.erase(i);
			break;
		}
}

void CUnitHandler::IdleUnitAdd(int unit)
{	
	L("IdleUnitAdd: " << unit);
	int category = ai->ut->GetCategory(unit);
	if(category != -1){
		const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(unit);
		if(mycommands->empty()){
		
			if(category == CAT_BUILDER)
			{
				BuilderTracker* builderTracker = GetBuilderTracker(unit);
				L("it was a builder");
				
				// Add clear here
				ClearOrder(builderTracker, true);
				
				if(builderTracker->idleStartFrame == -2)
				{
					// It was in the idle list allready ?
					IdleUnitRemove(builderTracker->builderID);
				}
				builderTracker->idleStartFrame = -2; // Its in the idle list now
				if(builderTracker->commandOrderPushFrame == -2) // Make shure that if the unit was just built it will have some time to leave the factory
				{
					builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame() + 30*3;
				}
				//else if(builderTracker->commandOrderPushFrame == -)
				//	builderTracker->commandOrderPushFrame = ;
			}

			integer2 myunit(unit,LIMBOTIME);
			L("Adding unit : " << myunit.x << " To Limbo " << myunit.y);
			Limbo.remove(myunit);
			//IdleUnitRemove(unit);  // This might be a better idea, but its over the edge (possible assertion)
			Limbo.push_back(myunit);
		}
		else
		{
			// The unit have orders still
			if(category == CAT_BUILDER)
			{
				BuilderTracker* builderTracker = GetBuilderTracker(unit);
				assert(false);
				DecodeOrder(builderTracker, true);
			}
		}
	}
}

bool CUnitHandler::VerifyOrder(BuilderTracker* builderTracker)
{
	
	// If its without orders then try to find the lost command
	
	// TODO: All of it!!!!!!!!!!!!!!
	// Now take a look, and see what its doing:
	const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);
	bool commandFound = false;
	if(!mycommands->empty())
	{
		// It have orders
		const Command* c = &mycommands->front();
		if(mycommands->size() == 2)//&& (c->id == CMD_MOVE || c->id == CMD_RECLAIM))  
		{
			// Hmm, it might have a reclame order, or terrain change order
			// Take command nr. 2
			// Infact, take command nr. 2 anyway...
			
			c = &mycommands->back();
		}
		
		//L("idle builder: " << builderTracker->builderID);
		//L("c.id: " << c.id);
		//L("c.params[0]: " <<  c.params[0]);
		bool hit = false;
		if(builderTracker->buildTaskId != 0)
		{
			hit = true;
			// Test that this builder is on repair on this unit:
			BuildTask* buildTask = GetBuildTask(builderTracker->buildTaskId);
			if(c->id == CMD_REPAIR && c->params[0] == builderTracker->buildTaskId
				|| (c->id == -buildTask->def->id && c->params[0] == buildTask->pos.x && c->params[2] == buildTask->pos.z))
				commandFound = true; // Its ok
			else
				return false;

		}
		if(builderTracker->taskPlanId != 0)
		{
			assert(!hit);
			hit = true;
			TaskPlan* taskPlan = GetTaskPlan(builderTracker->taskPlanId);
			
			if(c->id == -taskPlan->def->id && c->params[0] == taskPlan->pos.x && c->params[2] == taskPlan->pos.z)
				commandFound = true; // Its ok
			else
				return false;
			
		}
		if(builderTracker->factoryId != 0)
		{
			assert(!hit);
			hit = true;
			if(c->id == CMD_GUARD && c->params[0] == builderTracker->factoryId)
				commandFound = true; // Its ok
			else
				return false;
		}
		if(builderTracker->customOrderId != 0)
		{
			assert(!hit);
			hit = true;
			// The CMD_MOVE is for human control stuff, CMD_REPAIR is for repairs of just made stuff
			// CMD_GUARD.... ?
			if(c->id == CMD_RECLAIM || c->id == CMD_MOVE || c->id == CMD_REPAIR) 
				commandFound = true; // Its ok
			else
			{
				//assert(false);
				return false;
			}
		}
		if(hit && commandFound)
		{
			// Its on the right job
			return true;
		}
	}
	else
	{
		if(builderTracker->idleStartFrame == -2)
		{
			return true;
		}
	}
	return false;
}

/*
Use this only if the unit dont have any orders at the moment
*/
void CUnitHandler::ClearOrder(BuilderTracker* builderTracker, bool reportError)
{
	bool hit = false;
	const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);
	L("ClearOrder, mycommands: " << mycommands->size());
	assert(mycommands->empty() || !reportError);
	if(builderTracker->buildTaskId != 0)
	{
		// Hmm, why is this builder idle ???
		hit = true;
		L("builder " << builderTracker->builderID << " was idle, but it is on buildTaskId : " << builderTracker->buildTaskId);
		BuildTask* buildTask = GetBuildTask(builderTracker->buildTaskId);
		if(debugPoints) {
			char text[512];
			sprintf(text, "builder %i: was idle, but it is on buildTaskId: %i  (stuck?)", builderTracker->builderID, builderTracker->buildTaskId);
			AIHCAddMapPoint amp;
			amp.label = text;
			amp.pos = buildTask->pos;
			ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
		}
		if(buildTask->builderTrackers.size() > 1)
		{
			BuildTaskRemoved(builderTracker);
		}
		else
		{
			// This is the only builder of this thing, and now its idle...
			BuildTaskRemoved(builderTracker); // IS this smart at all ???
		}
		//return;
	}
	if(builderTracker->taskPlanId != 0)
	{
		assert(!hit);
		hit = true;
		// Hmm, why is this builder idle ???
		// 
		TaskPlan* taskPlan = GetTaskPlan(builderTracker->taskPlanId);
		L("builder " << builderTracker->builderID << " was idle, but it is on taskPlanId : " << taskPlan->def->humanName << " (masking this spot)");
		if(debugPoints) {
			char text[512];
			sprintf(text, "builder %i: was idle, but it is on taskPlanId: %s (masking this spot)", builderTracker->builderID, taskPlan->def->humanName.c_str());
			AIHCAddMapPoint amp;
			amp.label = text;
			amp.pos = taskPlan->pos;
			ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
		}
//		ai->dm->MaskBadBuildSpot(taskPlan->pos);
		// TODO: fix this:  Remove all builders from this plan.
		if(reportError)
		{
			list<BuilderTracker*> builderTrackers = taskPlan->builderTrackers; // This is a copy of the list
			for(list<BuilderTracker*>::iterator i = builderTrackers.begin(); i != builderTrackers.end(); i++) {
				TaskPlanRemoved(*i);
				ai->MyUnits[(*i)->builderID]->Stop(); // Stop the units on the way to this plan
			}
		} else
		{
			TaskPlanRemoved(builderTracker);
		}
		//return;
	}
	if(builderTracker->factoryId != 0)
	{
		assert(!hit);
		hit = true;
		// Hmm, why is this builder idle ???
		L("builder " << builderTracker->builderID << " was idle, but it is on factoryId : " << builderTracker->factoryId) << " (removing the builder from the job)";
		if(debugPoints) {
			char text[512];
			sprintf(text, "builder %i: was idle, but it is on factoryId: %i (removing the builder from the job)", builderTracker->builderID, builderTracker->factoryId);
			AIHCAddMapPoint amp;
			amp.label = text;
			amp.pos = ai->cb->GetUnitPos(builderTracker->factoryId);
			ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
		}
		FactoryBuilderRemoved(builderTracker);
	}
	if(builderTracker->customOrderId != 0)
	{
		assert(!hit);
		hit = true;
		// Hmm, why is this builder idle ?
		// No tracking of custom orders yet... 
		L("builder " << builderTracker->builderID << " was idle, but it is on customOrderId : " << builderTracker->customOrderId << " (removing the builder from the job)");
		
		//char text[512];
		//sprintf(text, "builder %i: was idle, but it is on customOrderId: %i (removing the builder from the job)", unit, builderTracker->customOrderId);
		//AIHCAddMapPoint amp;
		//amp.label = text;
		//amp.pos = ai->cb->GetUnitPos(builderTracker->builderID);
		//ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
		builderTracker->customOrderId = 0;
	}
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
}

void CUnitHandler::DecodeOrder(BuilderTracker* builderTracker, bool reportError) {
	reportError = reportError;
	// If its without orders then try to find the lost command
	
	// TODO: All of it!!!!!!!!!!!!!!
	// Now take a look, and see what its doing:
	const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);
	if(!mycommands->empty())
	{
		// It have orders
		const Command* c = &mycommands->front();
		if(mycommands->size() == 2 && c->id == CMD_MOVE)//&& (c->id == CMD_MOVE || c->id == CMD_RECLAIM))  
		{
			// Hmm, it might have a move order before the real order
			// Take command nr. 2 if nr.1 is a move order
			c = &mycommands->back();
		}
		
		
		L("idle builder: " << builderTracker->builderID);
		L("c->id: " << c->id);
		L("c->params[0]: " <<  c->params[0]);
		if(debugPoints) {
			char text[512];
			sprintf(text, "builder %i: was clamed idle, but it have a command c->id: %i, c->params[0]: %g", builderTracker->builderID, c->id, c->params[0]);
			AIHCAddMapPoint amp;
			amp.label = text;
			amp.pos = ai->cb->GetUnitPos(builderTracker->builderID);
			ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
		}
		if(c->id < 0) // Its building a unit
		{
			float3 newUnitPos;
			newUnitPos.x = c->params[0];
			newUnitPos.y = c->params[1];
			newUnitPos.z = c->params[2];
			// c.id == -newUnitDef->id
			const UnitDef* newUnitDef = ai->ut->unittypearray[-c->id].def;
			// Now make shure that no BuildTasks exists there 
			BuildTask* buildTask = BuildTaskExist(newUnitPos, newUnitDef);
			if(buildTask)
			{
				BuildTaskAddBuilder(buildTask, builderTracker);
			}
			else // Make a new TaskPlan (or join an existing one)
				TaskPlanCreate(builderTracker->builderID, newUnitPos, newUnitDef);

		}
		if(c->id == CMD_REPAIR)  // Its repairing    ( || c.id == CMD_GUARD)
		{
			int guardingID = int(c->params[0]);
			// Find the unit its repairng
			int category = ai->ut->GetCategory(guardingID);
			if(category == -1)
				return; // This is bad....
			bool found = false;
			for(list<BuildTask*>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++){
				if((*i)->id == guardingID)
				{
					// Whatever the old order was, update it now...
					bool hit = false;
					if(builderTracker->buildTaskId != 0)
					{
						hit = true;
						// Hmm, why is this builder idle ???
						BuildTask* buildTask = GetBuildTask(builderTracker->buildTaskId);
						if(buildTask->builderTrackers.size() > 1)
						{
							BuildTaskRemoved(builderTracker);
						}
						else
						{
							// This is the only builder of this thing, and now its idle...
							BuildTaskRemoved(builderTracker); // IS this smart at all ???
						}
					}
					if(builderTracker->taskPlanId != 0)
					{
						assert(!hit);
						hit = true;
						TaskPlanRemoved(builderTracker);
						
						//return;
					}
					if(builderTracker->factoryId != 0)
					{
						assert(!hit);
						hit = true;
						FactoryBuilderRemoved(builderTracker);
					}
					if(builderTracker->customOrderId != 0)
					{
						assert(!hit);
						hit = true;
						builderTracker->customOrderId = 0;
					}
					BuildTask* bt = *i;
					L("Adding builder to BuildTask " << bt->id << ": " << bt->def->humanName);
					BuildTaskAddBuilder(bt, builderTracker);
					found = true;
				}
			}
			if(found == false)
			{
				// Not found, just make a custom order
				L("Not found, just make a custom order");
				builderTracker->customOrderId = taskPlanCounter++;
				builderTracker->idleStartFrame = -1; // Its in use now
			}
		}
	}
	else
	{
		// Error: this function needs a builder with orders
		L("Error: this function needs a builder with orders");
		assert(false);
	}
}

void CUnitHandler::IdleUnitRemove(int unit)
{
	int category = ai->ut->GetCategory(unit);
	if(category != -1){
		//L("removing unit");
		L("IdleUnitRemove(): " << unit);
		IdleUnits[category].remove(unit);
		if(category == CAT_BUILDER)
		{
			BuilderTracker* builderTracker = GetBuilderTracker(unit);
			builderTracker->idleStartFrame = -1; // Its not in the idle list now
			if(builderTracker->commandOrderPushFrame == -2)
			{
				// bad
				//L("bad");
			}
			
			builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame(); // Update the order start frame...
			//assert(builderTracker->buildTaskId == 0);
			//assert(builderTracker->taskPlanId == 0);
			//assert(builderTracker->factoryId == 0);
		}
		//L("removed from list");
		list<integer2>::iterator tempunit;
		bool foundit = false;
		for(list<integer2>::iterator i = Limbo.begin(); i != Limbo.end(); i++){
			if(i->x == unit){
				tempunit = i;
				foundit=true;
				L("foundit=true;");
			}
		}
		if(foundit)
			Limbo.erase(tempunit);
		
		// Make shure its gone:
		for(list<int>::iterator i = IdleUnits[category].begin(); i != IdleUnits[category].end(); i++){
			if(*i == unit)
			{
				foundit=true;
//				assert(false);
			}
		}
		foundit = false;
		for(list<integer2>::iterator i = Limbo.begin(); i != Limbo.end(); i++){
			if(i->x == unit){
				tempunit = i;
				foundit=true;
				L("foundit=true;");
			}
		}
		if(foundit){
//			assert(!foundit);
			std::string str;
			if (ai->cb->GetUnitDef(unit)) str = ai->cb->GetUnitDef(unit)->name;
			L("Idle unit not removed from idle list :"<<unit<<"("<<str<<")");
		}
		
		//L("removed from limbo");
	}
}
int CUnitHandler::GetIU(int category)
{
	assert(category >= 0 && category < LASTCATEGORY);
	assert(IdleUnits[category].size() > 0);
	L("GetIU(int category): " << IdleUnits[category].front());
	return IdleUnits[category].front();
}
int CUnitHandler::NumIdleUnits(int category)
{
	//for(list<int>::iterator i = IdleUnits[category]->begin(); i != IdleUnits[category]->end();i++)
		//L("Idle Unit: " << *i);
	assert(category >= 0 && category < LASTCATEGORY);
//	IdleUnits[category]->sort();
	IdleUnits[category].unique();
	return IdleUnits[category].size();
}
void CUnitHandler::MMakerAdd(int unit)
{
	metalMaker->Add(unit);
}
void CUnitHandler::MMakerRemove(int unit)
{
	metalMaker->Remove(unit);
}
void CUnitHandler::MMakerUpdate()
{
	metalMaker->Update();
}


void CUnitHandler::BuildTaskCreate(int id)
{
	const UnitDef* newUnitDef = ai->cb->GetUnitDef(id);
	int category = ai->ut->GetCategory(id);
	float3 pos = ai->cb->GetUnitPos(id);
	if((!newUnitDef->movedata  || category == CAT_DEFENCE) && !newUnitDef->canfly && category != -1){ // This thing need to change, so that it can make more stuff
		
		// TODO: Hack fix
		if(category == -1)
			return;
		assert(category >= 0);
		assert(category < LASTCATEGORY);
		
		BuildTask* bt = new BuildTask;
		bt->id = -1;
		//int killplan;
		//list<TaskPlan>::iterator killplan;
		redo:
		for(list<TaskPlan*>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++){
			if(pos.distance2D((*i)->pos) < 1 && newUnitDef == (*i)->def){
				assert(bt->id == -1); // There can not be more than one TaskPlan that is found;
				bt->category = category;
				bt->id = id;
				bt->pos = (*i)->pos;
				bt->def = newUnitDef;
				bt->currentBuildPower = 0;
				list<BuilderTracker*> moveList;
				for(list<BuilderTracker*>::iterator builder = (*i)->builderTrackers.begin(); builder != (*i)->builderTrackers.end(); builder++) {
					moveList.push_back(*builder);
					L("Marking builder " << (*builder)->builderID << " for removal, from plan " << (*i)->def->humanName);
				}
				for(list<BuilderTracker*>::iterator builder = moveList.begin(); builder != moveList.end(); builder++) {
					TaskPlanRemoved(*builder);
					BuildTaskAddBuilder(bt, *builder);
				}
				//bt.builders.push_back(i->builder);
				//killplan = i->builder;
				// This plan is gone now
				// Test it by redoing all:
				goto redo; // This is a temp
			}
		}
		if(bt->id == -1){
			L("*******BuildTask Creation Error!*********");
			// This can happen.
			// Either a builder manges to restart a dead building, or a human have taken control...
			// Make a BuildTask anyway
			bt->category = category;
			bt->id = id;
			if(category == CAT_DEFENCE)
				ai->dm->AddDefense(pos,newUnitDef);
			bt->pos = pos;
			bt->def = newUnitDef;
			bt->currentBuildPower = 0;
			if(debugPoints) {
				char text[512];
				sprintf(text, "BuildTask Creation Error: %i", id);
				AIHCAddMapPoint amp;
				amp.label = text;
				amp.pos = pos;
				ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
			}
			// Try to find workers that nearby:
			int num = BuilderTrackers.size();
			
			if(num == 0)
			{
				// Well what now ??? 
				L("Didnt find any friendly builders");
			} else
			{
				// Iterate over the list and find the builders
				for(list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++)
				{
					BuilderTracker* builderTracker = *i;
					// Now take a look, and see what its doing:
					const CCommandQueue* mycommands = ai->cb->GetCurrentUnitCommands(builderTracker->builderID);
					if(mycommands->size() > 0)
					{
						// It have orders
						Command c = mycommands->front();
						L("builder: " << builderTracker->builderID);
						L("c.id: " << c.id);
						L("c.params[0]: " <<  c.params[0]);
						if( (c.id == -newUnitDef->id && c.params[0] == pos.x && c.params[2] == pos.z) // Its at this pos
							|| (c.id == CMD_REPAIR  && c.params[0] == id)  // Its at this unit (id)
							|| (c.id == CMD_GUARD  && c.params[0] == id) ) // Its at this unit (id)
						{
							// Its making this unit
							// Remove the unit from its current job:
							bool hit = false;
							if(builderTracker->buildTaskId != 0)
							{
								// Hmm, why is this builder idle ???
								BuildTask* buildTask = GetBuildTask(builderTracker->buildTaskId);
								if (buildTask->builderTrackers.size() > 1) {
									BuildTaskRemoved(builderTracker);
								}
								else {
									// This is the only builder of this thing, and now its idle...
									BuildTaskRemoved(builderTracker); // IS this smart at all ???
								}
							}
							if (builderTracker->taskPlanId != 0) {
								assert(!hit);
								// Hmm, why is this builder idle ???
								TaskPlanRemoved(builderTracker);
							}
							if (builderTracker->factoryId != 0) {
								assert(!hit);
								FactoryBuilderRemoved(builderTracker);
							}
							if (builderTracker->customOrderId != 0) {
								assert(!hit);
								builderTracker->customOrderId = 0;
							}
							// This builder is now free.
							if (builderTracker->idleStartFrame == -2)
								IdleUnitRemove(builderTracker->builderID); // It was in the idle list
							// Add it to this task
							L("Added builder " << builderTracker->builderID << " to this new unit buildTask");
							BuildTaskAddBuilder(bt, builderTracker);
							if(debugPoints) {
								char text[512];
								sprintf(text, "Added builder %i: to buildTaskId: %i (human order?)", builderTracker->builderID, builderTracker->buildTaskId);
								AIHCAddMapPoint amp2;
								amp2.label = text;
								amp2.pos = ai->cb->GetUnitPos(builderTracker->builderID);
								ai->cb->HandleCommand(AIHCAddMapPointId, &amp2);
							}
						} else
						{
							// This builder have other orders.
						}
						
					} else
					{
						// This builder is without orders (idle)
					}
					
				}
			
			}
			// Add the task anyway
			BuildTasks[category].push_back(bt);
			
		}
		else{
			if(category == CAT_DEFENCE)
				ai->dm->AddDefense(pos,newUnitDef);
			BuildTasks[category].push_back(bt);
			//TaskPlanRemove(*killplan); // fix
		}
	}
}

// TODO: refactor this
void CUnitHandler::BuildTaskRemove(int id)
{
	L("BuildTaskRemove start");
	int category = ai->ut->GetCategory(id);
	// TODO: Hack fix
	if(category == -1)
		return;
	assert(category >= 0);
	assert(category < LASTCATEGORY);
	
	if(category != -1){
		list<BuildTask*>::iterator killtask;
		bool found = false;
		//list<list<BuildTask>::iterator> killList;
		for(list<BuildTask*>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++){
			if((*i)->id == id){
				killtask = i;
				assert(!found);
				//killList.push_front(i);
				found = true;
			}
		}
		if(found)
		{
			//for(list<list<BuildTask>::iterator>::iterator i = killList.begin(); i != killList.end(); i++){
			//	BuildTasks[category].erase(*i);
			//}
			
			// Remove the builders from this BuildTask:
			list<BuilderTracker*> removeList;
			for(list<BuilderTracker*>::iterator builder = (*killtask)->builderTrackers.begin(); builder != (*killtask)->builderTrackers.end(); builder++) {
				removeList.push_back(*builder);
				L("Marking builder " << (*builder)->builderID << " for removal, from task " << (*killtask)->id);
			}
			for(list<BuilderTracker*>::iterator builder = removeList.begin(); builder != removeList.end(); builder++) {
				BuildTaskRemoved(*builder);
			}
			//assert(false);
			delete *killtask; // Kill the BuildTask object.
			BuildTasks[category].erase(killtask);
		}
	}
	L("BuildTaskRemove end");
}

BuilderTracker* CUnitHandler::GetBuilderTracker(int builder)
{
	for(list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++){
		if((*i)->builderID == builder)
		{
			return *i;
		}
	}
	// This better not happen
	L("Fatal error: GetBuilderTracker failed to find builder. ID: " << builder << ", name: " << ai->cb->GetUnitDef(builder)->humanName);
	L("List of builders: ");
	for(list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++){
		L("ID: " << (*i)->builderID << ", name: " << ai->cb->GetUnitDef((*i)->builderID)->humanName);
		if(ai->cb->GetUnitDef((*i)->builderID) != (*i)->def)
			L("Unit def error. Internal def: " << (*i)->def);
	}
	
	assert(false);
	return 0;
}

// TODO: refactor this
void CUnitHandler::BuildTaskRemoved(BuilderTracker* builderTracker)
{
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
	bool found = false;

	for (list<BuildTask*>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++) {
		if ((*i)->id == builderTracker->buildTaskId) {
			//killtask = i;
			assert(!found);
			/*
			for(list<int>::iterator builder = i->builders.begin(); builder != i->builders.end(); builder++){
				if(builderTracker->builderID == *builder)
				{
					assert(!found2);
					i->builders.erase(builder);
					builderTracker->buildTaskId = 0;
					found2 = true;
					break;
				}
			}*/
			for(list<BuilderTracker*>::iterator builder = (*i)->builderTrackers.begin(); builder != (*i)->builderTrackers.end(); builder++){
				
				if(builderTracker == *builder)
				{
					assert(!found);
					(*i)->builderTrackers.erase(builder);
					builderTracker->buildTaskId = 0;
					(*i)->currentBuildPower -= builderTracker->def->buildSpeed;
					builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame(); // Give it time to change command
					found = true;
					break;
				}
			}
			
		}
	}
	assert(found);
	//if(found)
	//	for(list<list<BuildTask>::iterator>::iterator i = killList.begin(); i != killList.end(); i++){
	//		BuildTasks[category]->erase(*i);
	//	}
	//BuildTasks[category]->erase(killtask);
}

void CUnitHandler::BuildTaskAddBuilder(BuildTask* buildTask, BuilderTracker* builderTracker)
{
	//buildTask->builders.push_back(builderTracker->builderID);
	buildTask->builderTrackers.push_back(builderTracker);
	buildTask->currentBuildPower += builderTracker->def->buildSpeed;
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	builderTracker->buildTaskId = buildTask->id;
}

BuildTask* CUnitHandler::GetBuildTask(int buildTaskId)
{
	for(int k = 0; k < LASTCATEGORY;k++)
	{
		for(list<BuildTask*>::iterator i = BuildTasks[k].begin(); i != BuildTasks[k].end(); i++){
			if((*i)->id == buildTaskId)
				return  *i;
		}
	}
	// This better not happen
	assert(false);
	return 0;
}

BuildTask* CUnitHandler::BuildTaskExist(float3 pos,const UnitDef* builtdef)
{
	int category = ai->ut->GetCategory(builtdef);
	// TODO: Hack fix
	if(category == -1)
		return false;
	assert(category >= 0);
	assert(category < LASTCATEGORY);
		
	
	for(list<BuildTask*>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++){
		if((*i)->pos.distance2D(pos) < 1 && ai->ut->GetCategory((*i)->def) == category){
			return *i; // Hack
		}
	}
	return NULL;
}

// TODO: refactor this
bool CUnitHandler::BuildTaskAddBuilder (int builder, int category)
{
	L("BuildTaskAddBuilder: " << builder);
	assert(category >= 0);
	assert(category < LASTCATEGORY);
	assert(builder >= 0);
	assert(ai->MyUnits[builder] !=  NULL);
	BuilderTracker * builderTracker = GetBuilderTracker(builder);
	// Make shure this builder is free:
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->factoryId == 0);
	//assert(builderTracker->customOrderId == 0);
	
	// See if there are any BuildTasks that it can join
	if(BuildTasks[category].size()){
		float largestime = 0;
		list<BuildTask*>::iterator besttask;
		for(list<BuildTask*>::iterator i = BuildTasks[category].begin(); i != BuildTasks[category].end(); i++){
			float timebuilding = ai->math->ETT(*i) - (ai->math->ETA(builder,ai->cb->GetUnitPos((*i)->id)) * 1.5);
			if(timebuilding > largestime){
				largestime = timebuilding;
				besttask = i;
			}
		}
		if(largestime > 0){
			if (ai->MyUnits[builder]->Repair((*besttask)->id)) {
				BuildTaskAddBuilder(*besttask, builderTracker);
				return true;
			}
		}
	}
	// HACK^2    Korgothe...   this thing dont exist...
	if(TaskPlans[category].size())
	{
			L("TaskPlans[category]->size()");
			float largestime = 0;
			list<TaskPlan*>::iterator besttask;
			int units[10000];
			//redo:
			for(list<TaskPlan*>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++){
				float timebuilding = ((*i)->def->buildTime / (*i)->currentBuildPower ) - ai->math->ETA(builder,(*i)->pos);
				
				//L("timebuilding: " << timebuilding << " of " << i->def->humanName);
				// Must test if this builder can make this unit/building too
				if(timebuilding > largestime){
					const UnitDef *buildeDef = builderTracker->def;// ai->cb->GetUnitDef(builder);
					vector<int> * canBuildList = &ai->ut->unittypearray[buildeDef->id].canBuildList;
					int size = canBuildList->size();
					int thisBuildingID = (*i)->def->id;
					//bool canBuild = false; // Not needed, 
					for(int j = 0; j < size; j++)
					{
						if(canBuildList->at(j) == thisBuildingID)
						{
							//canBuild = true;
							largestime = timebuilding;
							besttask = i;
							break;
						}
					}
				}
				
				
				int num = ai->cb->GetFriendlyUnits(units, (*i)->pos,200); //returns all friendly units within radius from pos
				for(int j = 0; j < num; j++)
				{
					//L("Found unit at spot");
					
					if((ai->cb->GetUnitDef(units[j]) == (*i)->def) && (ai->cb->GetUnitPos(units[j]).distance2D((*i)->pos)) < 1 )
					{
						// HACK !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						L("if((ai->cb->GetUnitDef(units[j]) == i->def) && (ai->cb->GetUnitPos(units[j]).distance2D(i->pos)) < 1 )");
						L("TODO: Kill this TaskPlan -- this is BAD -- its on a spot where a building is");
						// TODO: Kill this TaskPlan
						// But not here... as that will mess up the iterator
						
						
						//assert(false);
						//TaskPlans[category]->erase(i);
						//largestime = 0;
						//goto redo;
					}
				}
				
			}
			L("largestime: " << largestime);
			
			if(largestime > 10){
				L("joining the building of " << (*besttask)->def->humanName);
				
				assert(builder >= 0);
				assert(ai->MyUnits[builder] !=  NULL);
				// This is bad. as ai->MyUnits[builder]->Build use TaskPlanCreate()
				// It will work however 
				if (ai->MyUnits[builder]->Build((*besttask)->pos, (*besttask)->def)) {
					return true;
				}
		}
	}
	
	
	return false;
}

void  CUnitHandler::TaskPlanCreate(int builder, float3 pos, const UnitDef* builtdef)
{
	int category = ai->ut->GetCategory(builtdef);
	// TODO: Temp hack
	if(category == -1)
		return;
	assert(category >= 0);
	assert(category < LASTCATEGORY);
	
	// Find this builder:
	BuilderTracker * builderTracker = GetBuilderTracker(builder);
	// Make shure this builder is free:
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	
	bool existingtp = false;
	for(list<TaskPlan*>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++){
		if(pos.distance2D((*i)->pos) < 20 && builtdef == (*i)->def){ // TODO: this fails...  if pos is between two TaskPlans ?
			assert(!existingtp); // Make shure there are no other TaskPlan there
			existingtp = true;
			TaskPlanAdd(*i, builderTracker);
		}
	}
	if(!existingtp){
		TaskPlan* tp = new TaskPlan;
		tp->pos = pos;
		tp->def = builtdef;
		tp->defname = builtdef->name;
		tp->currentBuildPower = 0;
		tp->id = taskPlanCounter++;
		TaskPlanAdd(tp, builderTracker);
		
		if(category == CAT_DEFENCE)
				ai->dm->AddDefense(pos,builtdef);
		TaskPlans[category].push_back(tp);
	}
}

void CUnitHandler::TaskPlanAdd (TaskPlan* taskPlan, BuilderTracker* builderTracker)
{
	//taskPlan->builders.push_back(builderTracker->builderID);
	taskPlan->builderTrackers.push_back(builderTracker);
	taskPlan->currentBuildPower += builderTracker->def->buildSpeed;
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	builderTracker->taskPlanId = taskPlan->id;
}

// TODO: refactor this
void CUnitHandler::TaskPlanRemoved (BuilderTracker* builderTracker)
{
	// Make shure this builder is in a TaskPlan:
	assert(builderTracker->taskPlanId != 0);
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	int builder = builderTracker->builderID;
	bool found = false;
	//bool found2 = false;
	
	TaskPlan* taskPlan = GetTaskPlan(builderTracker->taskPlanId);
	builderTracker->taskPlanId = 0;
	for(list<BuilderTracker*>::iterator i = taskPlan->builderTrackers.begin(); i != taskPlan->builderTrackers.end(); i++){
		if(builderTracker == *i)
		{
			L("Removing builder " << builder << ", from plan " << taskPlan->def->humanName);
			found = true;
			builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame(); // Give it time to change command
			taskPlan->builderTrackers.erase(i);
			taskPlan->currentBuildPower -= builderTracker->def->buildSpeed;
			break;
		}
	}
	// TODO: assert that is gone
	if(!found)
	{
		L("Failed to removing builder " << builder);
		assert(found);
	}
	
	
	// Temp: kill the taskPlan if its empty  (temp)
	if(taskPlan->builderTrackers.size() == 0)
	{
		int category = ai->ut->GetCategory(taskPlan->def);
		L("The TaskPlans lost all its workers, removeing it");
		if(category == CAT_DEFENCE)  // Removeing this is ok ?
			ai->dm->RemoveDefense(taskPlan->pos,taskPlan->def);
		
		found = false;
		for(list<TaskPlan*>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++){
			if(taskPlan == *i)
			{
				TaskPlans[category].erase(i);
				delete taskPlan;
				found = true;
				break;
			}
		}
		if(!found)
		{
			L("Failed to remove empty TaskPlan");
			assert(found);
		}
	}
}

TaskPlan* CUnitHandler::GetTaskPlan(int taskPlanId)
{
	for(int k = 0; k < LASTCATEGORY;k++)
	{
		for(list<TaskPlan*>::iterator i = TaskPlans[k].begin(); i != TaskPlans[k].end(); i++){
			if((*i)->id == taskPlanId)
				return  *i;
		}
	}
	// This better not happen
	assert(false);
	return NULL;
}

/*
Not used ??
*/
bool CUnitHandler::TaskPlanExist(float3 pos,const UnitDef* builtdef)
{
	int category = ai->ut->GetCategory(builtdef);
	// TODO: Hack fix
	if(category == -1)
		return false;
	assert(category >= 0);
	assert(category < LASTCATEGORY);
		
	
	for(list<TaskPlan*>::iterator i = TaskPlans[category].begin(); i != TaskPlans[category].end(); i++){
		if((*i)->pos.distance2D(pos) < 100 && ai->ut->GetCategory((*i)->def) == category){
			return true;
		}
	}
	return false;
}

/*
Add a new factory 
*/
void CUnitHandler::FactoryAdd(int factory)
{
	if(ai->ut->GetCategory(factory) == CAT_FACTORY){
		Factory addfact;
		addfact.id = factory;
		addfact.factoryDef = ai->cb->GetUnitDef(factory);
		addfact.currentBuildPower = addfact.factoryDef->buildSpeed;
		addfact.currentTargetBuildPower = addfact.currentBuildPower;
		Factories.push_back(addfact);
	} else
	{
		assert(false);
	}
}

void CUnitHandler::FactoryLost (int id)
{
	list<Factory>::iterator killfactory;
	bool factoryfound;
	for(list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++){
		if(i->id == id){
			killfactory = i;
			factoryfound = true;
		}
	}
	if(factoryfound){
		// Remove all builders from this plan.
		list<BuilderTracker*> builderTrackers = killfactory->supportBuilderTrackers; // This is a copy of the list
		for(list<BuilderTracker*>::iterator i = builderTrackers.begin(); i != builderTrackers.end(); i++) {
			FactoryBuilderRemoved(*i);
		}
		//for(list<int>::iterator i = killfactory->supportbuilders.begin(); i != killfactory->supportbuilders.end(); i++){
		//	ai->uh->IdleUnitAdd(*i); // Is this needed ?  or smart ??
		//}
		Factories.erase(killfactory);
	}
	
}

bool CUnitHandler::FactoryBuilderAdd (int builder)
{
	BuilderTracker* builderTracker = GetBuilderTracker(builder);
	return FactoryBuilderAdd(builderTracker);	
}

bool CUnitHandler::FactoryBuilderAdd (BuilderTracker* builderTracker)
{
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	assert(builderTracker->customOrderId == 0);
	for(list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++){ //add range check?
		float totalbuildercost = 0;
		// TODO: Fix the hack:
		for(list<BuilderTracker*>::iterator j = i->supportBuilderTrackers.begin(); j != i->supportBuilderTrackers.end(); j++){
			totalbuildercost += ai->math->GetUnitCost((*j)->builderID);
		}
		if(totalbuildercost < ai->math->GetUnitCost(i->id) * BUILDERFACTORYCOSTRATIO){
		//if(i->supportBuilderTrackers.size() < MAXBUILDERSPERFACTORY){
			if (ai->MyUnits[builderTracker->builderID]->Guard(i->id)) {
				builderTracker->factoryId = i->id;
				i->supportBuilderTrackers.push_back(builderTracker);
				i->currentBuildPower += builderTracker->def->buildSpeed;
				return true;
			}
		}
	}
	return false;	
}

void CUnitHandler::FactoryBuilderRemoved (BuilderTracker* builderTracker)
{
	assert(builderTracker->factoryId != 0);
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->customOrderId == 0);
	list<Factory>::iterator killfactory;
	//bool factoryfound;
	for(list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++){
		if(builderTracker->factoryId == i->id)
		{
			i->supportBuilderTrackers.remove(builderTracker);
			i->currentBuildPower -= builderTracker->def->buildSpeed;
			builderTracker->factoryId = 0;
			builderTracker->commandOrderPushFrame = ai->cb->GetCurrentFrame(); // Give it time to change command
		}
	}
}

void CUnitHandler::BuilderReclaimOrder(int builderId, int target)
{
	BuilderTracker* builderTracker = GetBuilderTracker(builderId);
	assert(builderTracker->buildTaskId == 0);
	assert(builderTracker->taskPlanId == 0);
	assert(builderTracker->factoryId == 0);
	//assert(builderTracker->customOrderId == 0);
	// Just use taskPlanCounter for the id.
	L("BuilderReclaimOrder: " << builderId);
	builderTracker->customOrderId = target; //taskPlanCounter++;
	
}

int CUnitHandler::GetFactoryHelpersCount()
{
	int cnt=0;
	for(list<BuilderTracker*>::iterator i = BuilderTrackers.begin(); i != BuilderTrackers.end(); i++){
		if ((*i)->factoryId) cnt++;
	}
	return cnt;
}

float CUnitHandler::Distance2DToNearestFactory (float x,float z)
{
	float3 pos(x,0,z);
	float dist=1000000;
	if (Factories.empty()) return 0;
	for(list<Factory>::iterator i = Factories.begin(); i != Factories.end(); i++){
		float3 fpos = ai->cb->GetUnitPos(i->id);
		float tempdist = fpos.distance2D(pos);
		if (tempdist<dist) dist=tempdist;
	}
	return dist;
}

void CUnitHandler::CloakUpdate()
{
	if (ai->cb->GetEnergyIncome()<ai->cb->GetEnergyUsage()) {
		if (ai->cb->GetEnergy() < ai->cb->GetEnergyStorage() * 0.4 && !CloakedUnits.empty()) {
			int r=RANDINT%CloakedUnits.size();
			if (!ai->cb->GetUnitDef(CloakedUnits[r])) {
				CloakedUnits.erase(CloakedUnits.begin()+r);
			} else {
//				L("Unit decloaked "<<CloakedUnits[r]);
				if (ai->MyUnits[CloakedUnits[r]]->Cloaking(false)) {
					UncloakedUnits.push_back(CloakedUnits[r]);
					CloakedUnits.erase(CloakedUnits.begin()+r);
				}
			}
		}
		if (ai->cb->GetEnergy() < ai->cb->GetEnergyStorage() * 0.2 && !CloakedBuildings.empty()) {
			int r=RANDINT%CloakedBuildings.size();
			if (!ai->cb->GetUnitDef(CloakedBuildings[r])) {
				CloakedBuildings.erase(CloakedBuildings.begin()+r);
			} else {
//				L("Building decloaked "<<CloakedUnits[r]);
				if (ai->MyUnits[CloakedBuildings[r]]->Cloaking(false)) {
					UncloakedBuildings.push_back(CloakedBuildings[r]);
					CloakedBuildings.erase(CloakedBuildings.begin()+r);
				}
			}
		}
	} else {
		if (ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.9 && !UncloakedUnits.empty()) {
			int r=RANDINT%UncloakedUnits.size();
			if (!ai->cb->GetUnitDef(UncloakedUnits[r])) {
				UncloakedUnits.erase(UncloakedUnits.begin()+r);
			} else if (ai->cb->GetEnergyUsage()+ai->MyUnits[UncloakedUnits[r]]->def()->cloakCostMoving<ai->cb->GetEnergyIncome()) {
//				L("Unit cloaked "<<CloakedUnits[r]);
				if (ai->MyUnits[UncloakedUnits[r]]->Cloaking(true)) {
					CloakedUnits.push_back(UncloakedUnits[r]);
					UncloakedUnits.erase(UncloakedUnits.begin()+r);
				}
			}
		}
		if (ai->cb->GetEnergy() > ai->cb->GetEnergyStorage() * 0.7 && !UncloakedBuildings.empty()) {
			int r=RANDINT%UncloakedBuildings.size();
			if (!ai->cb->GetUnitDef(UncloakedBuildings[r])) {
				UncloakedBuildings.erase(UncloakedBuildings.begin()+r);
			} else if (ai->cb->GetEnergyUsage()+ai->MyUnits[UncloakedBuildings[r]]->def()->cloakCost<ai->cb->GetEnergyIncome()) {
//				L("Building cloaked "<<CloakedUnits[r]);
				if (ai->MyUnits[UncloakedBuildings[r]]->Cloaking(true)) {
					CloakedBuildings.push_back(UncloakedBuildings[r]);
					UncloakedBuildings.erase(UncloakedBuildings.begin()+r);
				}
			}
		}
	}
}

void CUnitHandler::StockpileUpdate()
{
	for(vector<int>::iterator i = StockpileDefenceUnits.begin(); i != StockpileDefenceUnits.end(); i++){
		if ((ai->MyUnits[*i]->GetStockpiled()<5 || 
			(ai->cb->GetEnergyIncome()-ai->cb->GetEnergyUsage()>1 &&
			ai->cb->GetMetalIncome()-ai->cb->GetMetalUsage()>1 &&
			ai->MyUnits[*i]->GetStockpiled()<50)) &&
			ai->MyUnits[*i]->GetStockpileQued()==0) {
			ai->MyUnits[*i]->BuildWeapon();
		}
	}
	for(vector<int>::iterator i = StockpileAttackUnits.begin(); i != StockpileAttackUnits.end(); i++){
		if ((ai->cb->GetEnergyIncome()-ai->cb->GetEnergyUsage()>500 &&
			ai->cb->GetMetalIncome()-ai->cb->GetMetalUsage()>5 &&
			ai->MyUnits[*i]->GetStockpiled()<5) &&
			ai->MyUnits[*i]->GetStockpileQued()==0) {
			ai->MyUnits[*i]->BuildWeapon();
			break;
		}
//		if (ai->MyUnits[*i]->GetStockpiled()>0) {
//		}
	}
}
