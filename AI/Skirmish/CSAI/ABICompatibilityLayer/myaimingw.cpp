#include <stdio.h>

#include "ExternalAI/aibase.h" // we need this to define DLL_EXPORT

// ***** dont include any spring includes above this, except aibase.h ******
#include "AbicAICallback.h"

// spring includes can go here.  CAREFUL: do NOT include UnitDef, FeatureDef, IAICallback or MoveInfo or you will break pointer opacity
#include "float3.h"

// ABI-called dll

// example AI class 
class MyAI
{
public:
    void InitAI( struct IAICallback *aicallback, int team)
    {    
        char buffer[1024];
        this->aicallback = aicallback;
        
        IAICallback_SendTextMsg( aicallback, "Hello from mingw!", 0 );
        
        sprintf( buffer, "The map name is: %s", IAICallback_GetMapName( aicallback ) );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );
        
        sprintf( buffer, "Our ally team is: %i", IAICallback_GetMyTeam( aicallback ) );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );
        
        int features[10000 + 1];
        int numfeatures = IAICallback_GetFeatures( aicallback, features, 10000 );
        sprintf( buffer, "Num features is: %i", numfeatures );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );
        
        const FeatureDef *featuredef = IAICallback_GetFeatureDef( aicallback, features[0] );
        sprintf( buffer, "First feature: %s", FeatureDef_get_myName( featuredef ) );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );    
    }
    
    void UnitCreated( int unit)									//called when a new unit is created on ai team
    {
        char buffer[1024];
        
        sprintf( buffer, "Unit created: %i", unit );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );
    
        const UnitDef *unitdef = IAICallback_GetUnitDef( aicallback, unit );
        sprintf( buffer, "Unit created: %s", UnitDef_get_name( unitdef ) );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );
    
        const MoveData *movedata = UnitDef_get_movedata( unitdef );
        if( movedata != 0 )
        {
            sprintf( buffer, "Max Slope: %f", MoveData_get_maxSlope( movedata ) );
            IAICallback_SendTextMsg( aicallback, buffer, 0 );
        }
    
        if( UnitDef_get_isCommander( unitdef ) )
        {
            int numbuildoptions = UnitDef_GetNumBuildOtions( unitdef );
            sprintf( buffer, "Build options: " );
            for( int i = 0; i < numbuildoptions; i++ )
            {
                sprintf( buffer, "%s%s, ", buffer, UnitDef_GetBuildOption( unitdef, i ) );            
            }
            IAICallback_SendTextMsg( aicallback, buffer, 0 );
    
            float3 commanderpos;
            IAICallback_GetUnitPos( aicallback, commanderpos.x, commanderpos.y, commanderpos.z, unit );
            sprintf( buffer, "Commanderpos: %f %f %f", commanderpos.x, commanderpos.y, commanderpos.z );
            IAICallback_SendTextMsg( aicallback, buffer, 0 );
            
            int numunitdefs = IAICallback_GetNumUnitDefs( aicallback );
            sprintf( buffer, "Num unit defs: %i", numunitdefs );
            IAICallback_SendTextMsg( aicallback, buffer, 0 );
            
            const UnitDef *unitdefs[10000];
            IAICallback_GetUnitDefList( aicallback, unitdefs );
            const UnitDef *solardef;
            for( int i = 0; i < numunitdefs; i++ )
            {
                if( strcmp( UnitDef_get_name( unitdefs[i] ), "ARMSOLAR" ) == 0 )
                {
                    solardef = unitdefs[i];
                    sprintf( buffer, "Found solar collector def: %i", UnitDef_get_id( solardef ) );
                    IAICallback_SendTextMsg( aicallback, buffer, 0 );
                    
                    float3 nearestbuildpos;
                    IAICallback_ClosestBuildSite( aicallback, nearestbuildpos.x, nearestbuildpos.y, nearestbuildpos.z, solardef, commanderpos.x, commanderpos.y, commanderpos.z, 1400,2,0 );
                    sprintf( buffer, "Closest build site: %f %f %f", nearestbuildpos.x, nearestbuildpos.y, nearestbuildpos.z );
                    IAICallback_SendTextMsg( aicallback, buffer, 0 );
                    
                    IAICallback_DrawUnit( aicallback, "ARMSOLAR", nearestbuildpos.x, nearestbuildpos.y, nearestbuildpos.z,0,
                        200, IAICallback_GetMyAllyTeam( aicallback ),true,true,0);
    
                    IAICallback_GiveOrder( aicallback, unit, - UnitDef_get_id( solardef ), 3, nearestbuildpos.x, nearestbuildpos.y, nearestbuildpos.z, 0 );
                }
            }
        }
    }
    
    void UnitFinished( int unit )
    {
        char buffer[1024];
        sprintf( buffer, "Unit finished: %i", unit );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );
    }

    void UnitIdle( int unit)										//called when a unit go idle and is not assigned to any group
    {
        char buffer[1024];
        sprintf( buffer, "Unit idle: %i", unit );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );
    }
    
    void UnitMoveFailed(int unit)
    {
        char buffer[1024];
        sprintf( buffer, "Unit move failed: %i", unit );
        IAICallback_SendTextMsg( aicallback, buffer, 0 );
    }

private:
    struct IAICallback *aicallback;
};

// exported dll symbols, wrap our AI class

// returns pointers to our new AI object
DLL_EXPORT void *InitAI( struct IAICallback *aicallback, int team)
{
    MyAI *ai = new MyAI();
    ai->InitAI( aicallback, team );
    return ai;
}

DLL_EXPORT void UnitCreated( void *ai, int unit)									//called when a new unit is created on ai team
{
    ( ( MyAI *)ai )->UnitCreated( unit );
}

DLL_EXPORT void UnitFinished(void *ai, int unit)							//called when an unit has finished building
{
    ( ( MyAI *)ai )->UnitFinished( unit );
}

DLL_EXPORT void UnitDestroyed(void *ai, int unit,int attacker)								//called when a unit is destroyed
{
}

DLL_EXPORT void EnemyEnterLOS(void *ai, int enemy)
{
}

DLL_EXPORT void EnemyLeaveLOS(void *ai, int enemy)
{
}

DLL_EXPORT void EnemyEnterRadar(void *ai, int enemy)		
{
}

DLL_EXPORT void EnemyLeaveRadar(void *ai, int enemy)
{
}

DLL_EXPORT void EnemyDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
{
}

DLL_EXPORT void EnemyDestroyed(void *ai, int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
{
}

DLL_EXPORT void UnitIdle(void *ai, int unit)										//called when a unit go idle and is not assigned to any group
{
    ( ( MyAI *)ai )->UnitIdle( unit );
}

DLL_EXPORT void GotChatMsg(void *aik, const char* msg,int player)					//called when someone writes a chat msg
{
}

DLL_EXPORT void UnitDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
{
}

DLL_EXPORT void UnitMoveFailed(void *ai, int unit)
{
    ( ( MyAI *)ai )->UnitMoveFailed( unit );
}

//int HandleEvent (int msg,const void *data); // todo

//called every frame
DLL_EXPORT void Update(void *ai )
{
}
    
