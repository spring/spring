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
	ai->LOGGER		= NULL; //new std::ofstream(c);
	ai->parser		= new CSunParser(ai);
	ai->ut			= new CUnitTable(ai);
	ai->mm			= new CMetalMap(ai);
	ai->pather		= new CPathFinder(ai);
	ai->tm			= new CThreatMap(ai);
	ai->uh			= new CUnitHandler(ai);
	ai->dm			= new CDefenseMatrix(ai);
	ai->econTracker = new CEconomyTracker(ai); // This is a temp only
	ai->bu			= new CBuildUp(ai);
	ai->ah			= new CAttackHandler(ai);
	//L("All Class pointers initialized");	



	ai->mm->Init();
	ai->ut->Init();
	ai->pather->Init();
	

}

void CGlobalAI::UnitCreated(int unit)
{
	ai->uh->UnitCreated(unit);
	ai->econTracker->UnitCreated(unit);
}

void CGlobalAI::UnitFinished(int unit)
{	
	//let attackhandler handle cat_g_attack units
	ai->econTracker->UnitFinished(unit);
	if(GCAT(unit) == CAT_G_ATTACK) {
		//attackHandler->AddUnit(unit);
		ai->ah->AddUnit(unit);
	}
	else if((ai->cb->GetCurrentFrame() < 20 || ai->cb->GetUnitDef(unit)->speed <= 0)) {//Add comm at begginning of game and factories when theyre built
		ai->uh->IdleUnitAdd(unit);
	}
	ai->uh->BuildTaskRemove(unit);
}

void CGlobalAI::UnitDestroyed(int unit,int attacker)
{
//	//L("GlobalAI::UnitDestroyed is called on unit:" << unit <<". its groupid:" << GUG(unit));
	ai->econTracker->UnitDestroyed(unit);
	if(GUG(unit) != -1) {
		//attackHandler->UnitDestroyed(unit);
		ai->ah->UnitDestroyed(unit);
	}
	ai->uh->UnitDestroyed(unit);
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

void CGlobalAI::EnemyDestroyed(int enemy,int attacker)
{

}

void CGlobalAI::UnitIdle(int unit)
{
	//attackhandler handles cat_g_attack units atm
	if(GCAT(unit) == CAT_G_ATTACK && ai->MyUnits.at(unit)->groupID != -1) {
		//attackHandler->UnitIdle(unit);
	} else {
		ai->uh->IdleUnitAdd(unit);
	}
}

void CGlobalAI::GotChatMsg(const char* msg,int player)
{

}

void CGlobalAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir)
{
	ai->econTracker->UnitDamaged(damaged, damage);
}

void CGlobalAI::EnemyDamaged(int damaged,int attacker,float damage,float3 dir)
{

}

void CGlobalAI::UnitMoveFailed(int unit)
{
	//this is/was bugged in a certain version
	//bool cGlobalAI_UnitMoveFailed_actually_works = false;
	//assert(cGlobalAI_UnitMoveFailed_actually_works);
}

int CGlobalAI::HandleEvent(int msg,const void* data)
{
	return 0;
}

void CGlobalAI::Update()
{
	int frame=ai->cb->GetCurrentFrame();
	ai->econTracker->frameUpdate();
	if(frame == 1){
		ai->dm->Init();
	}
	if(frame > 80){
		ai->bu->Update();
		ai->uh->IdleUnitUpdate();
	}
	ai->ah->Update();
	ai->uh->MMakerUpdate();
}
