// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "GlobalAI.h"
#include "IGlobalAICallback.h"
#include "UnitDef.h"
#include <vector>

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

}

void CGlobalAI::InitAI(IGlobalAICallback* callback, int team)
{
	this->callback=callback;
}

void CGlobalAI::UnitCreated(int unit)
{
	myUnits.insert(unit);
}

void CGlobalAI::UnitFinished(int unit)
{
}

void CGlobalAI::UnitDestroyed(int unit)
{
	myUnits.erase(unit);
}

void CGlobalAI::EnemyEnterLOS(int enemy)
{
	enemies.insert(enemy);
}

void CGlobalAI::EnemyLeaveLOS(int enemy)
{
	enemies.erase(enemy);
}

void CGlobalAI::EnemyEnterRadar(int enemy)
{
}

void CGlobalAI::EnemyLeaveRadar(int enemy)
{

}

void CGlobalAI::EnemyDestroyed(int enemy)
{
	enemies.erase(enemy);
}

void CGlobalAI::UnitIdle(int unit)
{
	const UnitDef* ud=callback->GetUnitDef(unit);

	char c[200];
	sprintf(c,"Idle unit %s",ud->humanName.c_str());
	callback->SendTextMsg(c,0);
}

void CGlobalAI::GotChatMsg(const char* msg,int player)
{

}

void CGlobalAI::UnitDamaged(int damaged,int attacker,float damage,float3 dir)
{

}

void CGlobalAI::Update()
{
	int frame=callback->GetCurrentFrame();

	if(!(frame%60)){
		char c[200];
		sprintf(c,"Friendly %i Enemy %i",myUnits.size(),enemies.size());
		callback->SendTextMsg(c,0);
	}
}

