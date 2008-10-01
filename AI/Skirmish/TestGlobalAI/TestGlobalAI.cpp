// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "TestGlobalAI.h"
#include "ExternalAI/IGlobalAICallback.h"
#include "Sim/Units/UnitDef.h"
#include <vector>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace std{
	void _xlen(){};
}


TestGlobalAI::TestGlobalAI()
{

}

TestGlobalAI::~TestGlobalAI()
{

}

void TestGlobalAI::InitAI(IGlobalAICallback* callback, int team)
{
	this->callback=callback;
}

void TestGlobalAI::UnitCreated(int unit)
{
	myUnits.insert(unit);
}

void TestGlobalAI::UnitFinished(int unit)
{
}

void TestGlobalAI::UnitDestroyed(int unit,int attacker)
{
	myUnits.erase(unit);
}

void TestGlobalAI::EnemyEnterLOS(int enemy)
{
	enemies.insert(enemy);
}

void TestGlobalAI::EnemyLeaveLOS(int enemy)
{
	enemies.erase(enemy);
}

void TestGlobalAI::EnemyEnterRadar(int enemy)
{
}

void TestGlobalAI::EnemyLeaveRadar(int enemy)
{

}

void TestGlobalAI::EnemyDamaged(int damaged,int attacker,float damage,float3 dir)
{
}

void TestGlobalAI::EnemyDestroyed(int enemy,int attacker)
{
	enemies.erase(enemy);
}

void TestGlobalAI::UnitIdle(int unit)
{
	const UnitDef* ud=callback->GetAICallback()->GetUnitDef(unit);

	/*char c[200];
	sprintf(c,"Idle unit %s",ud->humanName.c_str());
	callback->GetAICallback()->SendTextMsg(c,0);*/
}

void TestGlobalAI::GotChatMsg(const char* msg,int player)
{

}

void TestGlobalAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir)
{

}

void TestGlobalAI::UnitMoveFailed(int unit)
{
}

int TestGlobalAI::HandleEvent(int msg,const void* data)
{
	return 0;
}

void TestGlobalAI::Update()
{
	int frame=callback->GetAICallback()->GetCurrentFrame();

	if(!(frame%60)){
/*		char c[200];
		sprintf(c,"Friendly %i Enemy %i",myUnits.size(),enemies.size());
		callback->GetAICallback()->SendTextMsg(c,0);*/
	}
}
