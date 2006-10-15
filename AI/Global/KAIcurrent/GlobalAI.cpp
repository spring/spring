// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "GlobalAI.h"
#include "UNIT.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace std{
	void _xlen(){};
}


CGlobalAI::CGlobalAI()
{

}

CGlobalAI::~CGlobalAI()
{
	delete ai->LOGGER;
	delete ai->ah;
	delete ai->bu;
	delete ai->econTracker;
	delete ai->parser;
	delete ai->math;
	delete ai->debug;
	delete ai->pather;
	delete ai->tm;
	delete ai->ut;
	delete ai->mm;
	delete ai->uh;
	delete ai;
}

void CGlobalAI::InitAI(IGlobalAICallback* callback, int team)
{
	int timetaken = clock();

	// Initialize Log filename
	string mapname = string(callback->GetAICallback()->GetMapName());
	mapname.resize(mapname.size()-4);
	time_t now1;
	time(&now1);
	struct tm *now2;
	now2 = localtime(&now1);
	// Date logfile name
	sprintf(c, "%s%s %2.2d-%2.2d-%4.4d %2.2d%2.2d (%d).log",string(LOGFOLDER).c_str(),
            mapname.c_str(),now2->tm_mon+1, now2->tm_mday, now2->tm_year + 1900,
			now2->tm_hour,now2->tm_min, team);	

	ai				= new AIClasses;



	ai->cb			= callback->GetAICallback();
	ai->cheat		= callback->GetCheatInterface();

	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, c);

	MyUnits.reserve(MAXUNITS);
	ai->MyUnits.reserve(MAXUNITS);
	for (int i = 0; i < MAXUNITS;i++){		
		MyUnits.push_back(CUNIT(ai));
		MyUnits[i].myid = i;
		MyUnits[i].groupID = -1;
		ai->MyUnits.push_back(&MyUnits[i]);
	}

	ai->debug		= new CDebug(ai);
	ai->math		= new CMaths(ai);
	ai->LOGGER		= new std::ofstream(c);
	ai->parser		= new CSunParser(ai);
	ai->sh			= new CSurveillanceHandler(ai);
	ai->ut			= new CUnitTable(ai);
	ai->mm			= new CMetalMap(ai);
	ai->pather		= new CPathFinder(ai);
	ai->tm			= new CThreatMap(ai);
	ai->uh			= new CUnitHandler(ai);
	ai->dm			= new CDefenseMatrix(ai);
	ai->econTracker = new CEconomyTracker(ai); // This is a temp only
	ai->bu			= new CBuildUp(ai);
	ai->ah			= new CAttackHandler(ai);
	ai->dc			= new CDamageControl(ai);
	ai->em			= new CEconomyManager(ai);
	L("All Class pointers initialized");

	// Make the timer numbers:
	totalSumTime = 0;
	updateTimerGroup = ai->math->GetNewTimerGroupNumber("CGlobalAI::Update()");
	econTrackerFrameUpdate = ai->math->GetNewTimerGroupNumber("ai->econTracker->frameUpdate()");
	updateTheirDistributionTime = ai->math->GetNewTimerGroupNumber("ai->dc->UpdateTheirDistribution()");
	updateMyDistributionTime = ai->math->GetNewTimerGroupNumber("ai->dc->UpdateMyDistribution()");
	threatMapTime = ai->math->GetNewTimerGroupNumber("ai->tm->Create()  (threatMap)");
	builUpTime = ai->math->GetNewTimerGroupNumber("ai->bu->Update()  (buildup)");
	idleUnitUpdateTime = ai->math->GetNewTimerGroupNumber("idleUnitUpdateTime");
	attackHandlerUpdateTime = ai->math->GetNewTimerGroupNumber("ai->ah->Update()  (attackHandler)");
	MMakerUpdateTime = ai->math->GetNewTimerGroupNumber("ai->uh->MMakerUpdate()");
	economyManagerUpdateTime = ai->math->GetNewTimerGroupNumber("ai->em->Update()  (economyManager)");
	globalAILogTime = ai->math->GetNewTimerGroupNumber("GlobalAI log time  ( L() )");
	unitCreatedTime = ai->math->GetNewTimerGroupNumber("CGlobalAI::UnitCreated(int unit)");
	unitFinishedTime = ai->math->GetNewTimerGroupNumber("CGlobalAI::UnitFinished(int unit)");
	unitDestroyedTime = ai->math->GetNewTimerGroupNumber("CGlobalAI::UnitDestroyed(int unit,int attacker)");
	unitIdleTime = ai->math->GetNewTimerGroupNumber("CGlobalAI::UnitIdle(int unit)");
	
	L("Timers initialized");	
	ai->mm->Init();
	L("ai->mm->Init(); done");	
	ai->ut->Init();
	L("ai->ut->Init(); done");	
	ai->pather->Init();
	L("init done");	
	ai->dc->GenerateDPSTables();
	L("GenerateDPSTables done");
	ai->cb->SendTextMsg("KAI v0.21 loaded!",0);
}

void CGlobalAI::UnitCreated(int unit)
{
	ai->math->StartTimer(totalSumTime);
	ai->math->StartTimer(globalAILogTime);
	L("GlobalAI::UnitCreated is called on unit:" << unit <<". its groupid:" << ai->MyUnits[unit]->groupID << " name:" << ai->MyUnits[unit]->def()->humanName);
	ai->math->StopTimer(globalAILogTime);
	ai->math->StartTimer(unitCreatedTime);
	ai->uh->UnitCreated(unit);
	ai->econTracker->UnitCreated(unit);
	ai->math->StopTimer(unitCreatedTime);
	ai->math->StopTimer(totalSumTime);
}

void CGlobalAI::UnitFinished(int unit)
{
	ai->math->StartTimer(totalSumTime);
	ai->math->StartTimer(globalAILogTime);
	L("GlobalAI::UnitFinished is called on unit:" << unit <<". its groupid:" << ai->MyUnits[unit]->groupID << " name:" << ai->MyUnits[unit]->def()->humanName);
	ai->math->StopTimer(globalAILogTime);
	ai->math->StartTimer(unitFinishedTime);
	ai->econTracker->UnitFinished(unit);
	if(ai->ah->CanHandleThisUnit(unit)) {
		ai->ah->AddUnit(unit);
	}
	else if((ai->cb->GetCurrentFrame() < 2 || ai->cb->GetUnitDef(unit)->speed <= 0)) {//Add comm at begginning of game and factories when theyre built
		ai->uh->IdleUnitAdd(unit);
	}
	ai->uh->BuildTaskRemove(unit);
	ai->math->StopTimer(unitFinishedTime);
	ai->math->StopTimer(totalSumTime);
}

void CGlobalAI::UnitDestroyed(int unit,int attacker)
{
	ai->bu->MexUpgraders.remove(unit);
	ai->math->StartTimer(totalSumTime);
	ai->math->StartTimer(globalAILogTime);
	L("GlobalAI::UnitDestroyed is called on unit:" << unit <<". its groupid:" << ai->MyUnits[unit]->groupID << " name:" << ai->MyUnits[unit]->def()->humanName);
	ai->math->StopTimer(globalAILogTime);
	ai->math->StartTimer(unitDestroyedTime);
	ai->econTracker->UnitDestroyed(unit);
	if(GUG(unit) != -1) {
		ai->ah->UnitDestroyed(unit);
	}
	ai->uh->UnitDestroyed(unit);
	ai->math->StopTimer(unitDestroyedTime);
	ai->math->StopTimer(totalSumTime);
}

void CGlobalAI::EnemyEnterLOS(int enemy)
{

}

void CGlobalAI::EnemyLeaveLOS(int enemy)
{

}

void CGlobalAI::EnemyEnterRadar(int enemy)
{
}

void CGlobalAI::EnemyLeaveRadar(int enemy)
{

}

void CGlobalAI::EnemyDamaged(int damaged,int attacker,float damage,float3 dir)
{
}

void CGlobalAI::EnemyDestroyed(int enemy,int attacker)
{

}

void CGlobalAI::UnitIdle(int unit)
{
	ai->math->StartTimer(totalSumTime);
	ai->math->StartTimer(globalAILogTime);
	L("Idle: " << unit);
	ai->math->StopTimer(globalAILogTime);
	ai->math->StartTimer(unitIdleTime);
	ai->econTracker->frameUpdate();
	//attackhandler handles cat_g_attack units atm
	if(GCAT(unit) == CAT_G_ATTACK && ai->MyUnits.at(unit)->groupID != -1) {
		//attackHandler->UnitIdle(unit);
	} else {
		ai->uh->IdleUnitAdd(unit);
	}
	ai->math->StopTimer(unitIdleTime);
	ai->math->StopTimer(totalSumTime);
}

void CGlobalAI::GotChatMsg(const char* msg,int player)
{

}

void CGlobalAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir)
{
	ai->econTracker->UnitDamaged(damaged, damage);
}

void CGlobalAI::UnitMoveFailed(int unit)
{
	if (ai->cb->GetUnitDef(unit) != NULL) { //habit :P
		ai->MyUnits[unit]->stuckCounter++;
	}
}

int CGlobalAI::HandleEvent(int msg,const void* data)
{
	// Unit steal and donate support:
	L("msg: " << msg);
	if(msg == AI_EVENT_UNITGIVEN)
	{
		const IGlobalAI::ChangeTeamEvent* cte = (const IGlobalAI::ChangeTeamEvent*) data;
		if(cte->newteam == ai->cb->GetMyTeam())
		{
			// Just got a unit
			UnitCreated(cte->unit);
			UnitFinished(cte->unit);
			ai->MyUnits[cte->unit]->Stop();
		}
	}
	else if(msg == AI_EVENT_UNITCAPTURED)
	{
		const IGlobalAI::ChangeTeamEvent* cte = (const IGlobalAI::ChangeTeamEvent*) data;
		if(cte->oldteam == ai->cb->GetMyTeam())
		{
			// Just lost a unit
			UnitDestroyed(cte->unit, 0);
		}
	}
	L("msg end");
	return 0;
}

void CGlobalAI::Update()
{
	ai->math->StartTimer(totalSumTime);
	int frame=ai->cb->GetCurrentFrame();
	ai->math->StartTimer(globalAILogTime);
//	L("start: " << frame);
	ai->math->StopTimer(globalAILogTime);
	ai->math->StartTimer(updateTimerGroup);
	
	ai->math->StartTimer(econTrackerFrameUpdate);
	ai->econTracker->frameUpdate();
	ai->math->StopTimer(econTrackerFrameUpdate);
	
	if(frame%60 == 1){
		ai->math->StartTimer(updateTheirDistributionTime);
		ai->dc->UpdateTheirDistribution();
		ai->math->StopTimer(updateTheirDistributionTime);
	}
	if(frame%60 == 20){
		ai->math->StartTimer(updateMyDistributionTime);
		ai->dc->UpdateMyDistribution();
		ai->math->StopTimer(updateMyDistributionTime);
	}
	if(frame%60 == 40){
		ai->math->StartTimer(economyManagerUpdateTime);
		ai->em->Update();
		ai->math->StopTimer(economyManagerUpdateTime);
	}
	if(frame > 80){
		ai->math->StartTimer(threatMapTime);
		if(frame%15 ==0)
			ai->tm->Create();
		ai->math->StopTimer(threatMapTime);
		
		ai->math->StartTimer(builUpTime);
		ai->bu->Update();
		ai->math->StopTimer(builUpTime);
		ai->math->StartTimer(idleUnitUpdateTime);
		ai->uh->IdleUnitUpdate();
		ai->math->StopTimer(idleUnitUpdateTime);
	}
	ai->math->StartTimer(attackHandlerUpdateTime);
	ai->ah->Update();
	ai->math->StopTimer(attackHandlerUpdateTime);
	
	ai->math->StartTimer(MMakerUpdateTime);
	ai->uh->MMakerUpdate();
	ai->math->StopTimer(MMakerUpdateTime);
	//int count = ai->sh->GetCountEnemiesFullThisFrame();
	//if(count > 0)
	//	L("GetCountEnemiesFullThisFrame: " << count);
	float updateTime = ai->math->StopTimer(updateTimerGroup);
	ai->math->StartTimer(globalAILogTime);
//	L("end: " << frame << ", updateTime: " << updateTime);
	ai->math->StopTimer(globalAILogTime);
	
	ai->math->StopTimer(totalSumTime);
	
	if(frame == 1){ // Dont include the setup time of defenseMatrix in the total
		//ai->math->StartTimer(defenseMatrixInitTime);
		ai->dm->Init();
		//ai->math->StopTimer(defenseMatrixInitTime);
	}
	// Print the times every 2 mins
	if(frame % 3600 == 0 && frame > 1)
	{
		L("Here is the time distribution after " << (frame / 1800) << " mins");
		ai->math->PrintAllTimes();
	}
}
