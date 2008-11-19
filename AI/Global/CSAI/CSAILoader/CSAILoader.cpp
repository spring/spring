#include <windows.h>
#include <_vcclrit.h>

#include "CSAIProxy.h"
#include "ExternalAI/aibase.h"

#include "AbicAICallbackWrapper.h"

/////////////////////////////////////////////////////////////////////////////

// declare that we present a C interface.  Spring checks for this.
SHARED_EXPORT bool IsCInterface()
{
    return true;
}

SHARED_EXPORT int GetGlobalAiVersion()
{
	return GLOBALAI_C_INTERFACE_VERSION;
}

SHARED_EXPORT void GetAiName(char* name)
{
	strcpy(name, CSAIProxy::GetAiName() );
}

SHARED_EXPORT void *InitAI( struct IAICallback *aicallback, int team)
{
    CSAIProxy *ai = new CSAIProxy();
    AbicAICallbackWrapper *aicallbackwrapper = new AbicAICallbackWrapper( aicallback );
    ai->InitAI( aicallbackwrapper, team );
    return ai;
}

SHARED_EXPORT void UnitCreated( void *ai, int unit)									//called when a new unit is created on ai team
{
    ( ( CSAIProxy *)ai )->UnitCreated( unit );
}

SHARED_EXPORT void UnitFinished(void *ai, int unit)							//called when an unit has finished building
{
    ( ( CSAIProxy *)ai )->UnitFinished( unit );
}

SHARED_EXPORT void UnitDestroyed(void *ai, int unit,int attacker)								//called when a unit is destroyed
{
    ( ( CSAIProxy *)ai )->UnitDestroyed( unit, attacker );
}

SHARED_EXPORT void EnemyEnterLOS(void *ai, int enemy)
{
    ( ( CSAIProxy *)ai )->EnemyEnterLOS( enemy );
}

SHARED_EXPORT void EnemyLeaveLOS(void *ai, int enemy)
{
    ( ( CSAIProxy *)ai )->EnemyLeaveLOS( enemy );
}

SHARED_EXPORT void EnemyEnterRadar(void *ai, int enemy)		
{
    ( ( CSAIProxy *)ai )->EnemyEnterRadar( enemy );
}

SHARED_EXPORT void EnemyLeaveRadar(void *ai, int enemy)
{
    ( ( CSAIProxy *)ai )->EnemyLeaveRadar( enemy );
}

SHARED_EXPORT void EnemyDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
{
    ( ( CSAIProxy *)ai )->EnemyDamaged( damaged, attacker ,damage, float3( dirx, diry, dirz ) );
}

SHARED_EXPORT void EnemyDestroyed(void *ai, int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
{
    ( ( CSAIProxy *)ai )->EnemyDestroyed( enemy, attacker );
}

SHARED_EXPORT void UnitIdle(void *ai, int unit)										//called when a unit go idle and is not assigned to any group
{
    ( ( CSAIProxy *)ai )->UnitIdle( unit );
}

SHARED_EXPORT void GotChatMsg(void *ai, const char* msg,int player)					//called when someone writes a chat msg
{
    ( ( CSAIProxy *)ai )->GotChatMsg( msg, player );
}

SHARED_EXPORT void UnitDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
{
    ( ( CSAIProxy *)ai )->UnitDamaged( damaged, attacker, damage, float3(dirx, diry, dirz ) );
}

SHARED_EXPORT void UnitMoveFailed(void *ai, int unit)
{
    ( ( CSAIProxy *)ai )->UnitMoveFailed( unit );
}

//std::set<IGlobalAI*> ais;

//int HandleEvent (int msg,const void *data); // todo

//called every frame
SHARED_EXPORT void Update(void *ai )
{
    ( ( CSAIProxy *)ai )->Update();
}
    
/* Old C++ interface bootstrap:
SHARED_EXPORT int GetGlobalAiVersion()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

SHARED_EXPORT void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

SHARED_EXPORT IGlobalAI* GetNewAI()
{
    __crt_dll_initialize();
    
	CSAIProxy* ai=new CSAIProxy;
	ais.insert(ai);
	return ai;
}

SHARED_EXPORT void ReleaseAI(IGlobalAI* i)
{
	delete i;
	ais.erase(i);
}
*/
