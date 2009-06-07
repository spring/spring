// TestGlobalAI.cpp: implementation of the TestGlobalAI class.
//
////////////////////////////////////////////////////////////////////////////////

#include "TestGlobalAI.h"

#include <stdlib.h>
#include <stdio.h>
#include "ExternalAI/IGlobalAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "CUtils/Util.h" // we only use the defines

////////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
////////////////////////////////////////////////////////////////////////////////

namespace std {
	void _xlen(){};
}


TestGlobalAI::TestGlobalAI()
{
}

TestGlobalAI::~TestGlobalAI()
{
}

////////////////////////////////////////////////////////////////////////////////
// AI Event functions
////////////////////////////////////////////////////////////////////////////////

void TestGlobalAI::InitAI(IGlobalAICallback* callback, int team)
{
	this->callback=callback;
}

void TestGlobalAI::UnitCreated(int unit, int builder)
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

void TestGlobalAI::EnemyDamaged(int damaged, int attacker, float damage,
		float3 dir)
{
}

void TestGlobalAI::EnemyDestroyed(int enemy,int attacker)
{
	enemies.erase(enemy);
}

void TestGlobalAI::UnitIdle(int unit)
{
	const UnitDef* ud=callback->GetAICallback()->GetUnitDef(unit);

	static char c[200];
	SNPRINTF(c, 200, "Idle unit %s", ud->humanName.c_str());
	callback->GetAICallback()->SendTextMsg(c, 0);
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
	return 0; // signaling: OK
}

void TestGlobalAI::Update()
{
	int frame=callback->GetAICallback()->GetCurrentFrame();

	if((frame % 60) == 0){
		//char c[200];
		//SNPRINTF(c, 200, "Friendly %i Enemy %i", myUnits.size(),
		//		enemies.size());
		//callback->GetAICallback()->SendTextMsg(c, 0);
		//callback->GetAICallback()->SendTextMsg(
		//		"NullLegacyCppAI: an other 60 frames passed!", 0);
	}
}
