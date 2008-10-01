#include <windows.h>
#include <_vcclrit.h>

#include "CSAIProxy.h"
#include "ExternalAI/aibase.h"

#include "AbicAICallbackWrapper.h"

/////////////////////////////////////////////////////////////////////////////

// declare that we present a C interface.  Spring checks for this.
DLL_EXPORT bool IsCInterface()
{
    return true;
}

DLL_EXPORT int GetGlobalAiVersion()
{
	return GLOBALAI_C_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name, CSAIProxy::GetAiName() );
}

DLL_EXPORT void *InitAI( struct IAICallback *aicallback, int team)
{
    CSAIProxy *ai = new CSAIProxy();
    AbicAICallbackWrapper *aicallbackwrapper = new AbicAICallbackWrapper( aicallback );
    ai->InitAI( aicallbackwrapper, team );
    return ai;
}

DLL_EXPORT void UnitCreated( void *ai, int unit)									//called when a new unit is created on ai team
{
    ( ( CSAIProxy *)ai )->UnitCreated( unit );
}

DLL_EXPORT void UnitFinished(void *ai, int unit)							//called when an unit has finished building
{
    ( ( CSAIProxy *)ai )->UnitFinished( unit );
}

DLL_EXPORT void UnitDestroyed(void *ai, int unit,int attacker)								//called when a unit is destroyed
{
    ( ( CSAIProxy *)ai )->UnitDestroyed( unit, attacker );
}

DLL_EXPORT void EnemyEnterLOS(void *ai, int enemy)
{
    ( ( CSAIProxy *)ai )->EnemyEnterLOS( enemy );
}

DLL_EXPORT void EnemyLeaveLOS(void *ai, int enemy)
{
    ( ( CSAIProxy *)ai )->EnemyLeaveLOS( enemy );
}

DLL_EXPORT void EnemyEnterRadar(void *ai, int enemy)		
{
    ( ( CSAIProxy *)ai )->EnemyEnterRadar( enemy );
}

DLL_EXPORT void EnemyLeaveRadar(void *ai, int enemy)
{
    ( ( CSAIProxy *)ai )->EnemyLeaveRadar( enemy );
}

DLL_EXPORT void EnemyDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
{
    ( ( CSAIProxy *)ai )->EnemyDamaged( damaged, attacker ,damage, float3( dirx, diry, dirz ) );
}

DLL_EXPORT void EnemyDestroyed(void *ai, int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
{
    ( ( CSAIProxy *)ai )->EnemyDestroyed( enemy, attacker );
}

DLL_EXPORT void UnitIdle(void *ai, int unit)										//called when a unit go idle and is not assigned to any group
{
    ( ( CSAIProxy *)ai )->UnitIdle( unit );
}

DLL_EXPORT void GotChatMsg(void *ai, const char* msg,int player)					//called when someone writes a chat msg
{
    ( ( CSAIProxy *)ai )->GotChatMsg( msg, player );
}

DLL_EXPORT void UnitDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
{
    ( ( CSAIProxy *)ai )->UnitDamaged( damaged, attacker, damage, float3(dirx, diry, dirz ) );
}

DLL_EXPORT void UnitMoveFailed(void *ai, int unit)
{
    ( ( CSAIProxy *)ai )->UnitMoveFailed( unit );
}

//std::set<IGlobalAI*> ais;

//int HandleEvent (int msg,const void *data); // todo

//called every frame
DLL_EXPORT void Update(void *ai )
{
    ( ( CSAIProxy *)ai )->Update();
}
    
/* Old C++ interface bootstrap:
DLL_EXPORT int GetGlobalAiVersion()
{
	return GLOBAL_AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,AI_NAME);
}

DLL_EXPORT IGlobalAI* GetNewAI()
{
    __crt_dll_initialize();
    
	CSAIProxy* ai=new CSAIProxy;
	ais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(IGlobalAI* i)
{
	delete i;
	ais.erase(i);
}
*/
