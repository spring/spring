#include <stdio.h>

#include "ExternalAI/aibase.h" // we need this to define DLL_EXPORT

// ***** dont include any spring includes above this, except aibase.h ******
//#include "ExternalAI/GlobalAICInterface/AbicAICallback.h"
#include "AbicAICallbackWrapper.h"
#include "AbicFeatureDefWrapper.h"
#include "AbicMoveDataWrapper.h"
#include "AbicUnitDefWrapper.h" 

// spring includes can go here. Note: including UnitDef, FeatureDef, IAICallback or MoveInfo will break pointer opacity (we want pointer opacity)
#include "float3.h"

// ABI-called dll

// example AI class 
class MyAI
{
public:
    void InitAI( AbicAICallbackWrapper *aicallback, int team)
    {    
        char buffer[1024];
        this->aicallback = aicallback;
        
        aicallback->SendTextMsg( "Hello from abi-called dll!", 0 );
        
        sprintf( buffer, "The map name is: %s", aicallback->GetMapName() );
        aicallback->SendTextMsg( buffer, 0 );
        
        sprintf( buffer, "Our ally team is: %i", aicallback->GetMyTeam() );
        aicallback->SendTextMsg( buffer, 0 );
        
        int features[10000 + 1];
        int numfeatures = aicallback->GetFeatures( features, 10000 );
        sprintf( buffer, "Num features is: %i", numfeatures );
        aicallback->SendTextMsg( buffer, 0 );
        
        AbicFeatureDefWrapper *featuredef = aicallback->GetFeatureDef( features[0] );
        sprintf( buffer, "First feature: %s",featuredef->get_myName() );
        aicallback->SendTextMsg( buffer, 0 );    
		delete featuredef; // reponsible for deleting any AbicWrappers returned by AbicWrapper functions
    }
    
    void UnitCreated( int unit)									//called when a new unit is created on ai team
    {
        char buffer[1024];
        
        sprintf( buffer, "Unit created: %i", unit );
        aicallback->SendTextMsg( buffer, 0 );
    
        AbicUnitDefWrapper *unitdef = aicallback->GetUnitDef( unit );
        sprintf( buffer, "Unit created: %s", unitdef->get_name() );
        aicallback->SendTextMsg( buffer, 0 );
    
        AbicMoveDataWrapper *movedata = unitdef->get_movedata();
        if( movedata != 0 )
        {
            sprintf( buffer, "Max Slope: %f", movedata->get_maxSlope() );
            aicallback->SendTextMsg( buffer, 0 );
        }
    
        if( unitdef->get_isCommander() )
        {
            int numbuildoptions = unitdef->GetNumBuildOptions();
            sprintf( buffer, "Build options: " );
            for( int i = 0; i < numbuildoptions; i++ )
            {
                sprintf( buffer, "%s%s, ", buffer, unitdef->GetBuildOption( i ) );            
            }
            aicallback->SendTextMsg( buffer, 0 );
    
            float3 commanderpos = aicallback->GetUnitPos( unit );
            sprintf( buffer, "Commanderpos: %f %f %f", commanderpos.x, commanderpos.y, commanderpos.z );
            aicallback->SendTextMsg( buffer, 0 );
            
            int numunitdefs = aicallback->GetNumUnitDefs();
            sprintf( buffer, "Num unit defs: %i", numunitdefs );
            aicallback->SendTextMsg( buffer, 0 );

            for( int i = 1; i <= numunitdefs; i++ )
            {
				AbicUnitDefWrapper *thisunitdef = aicallback->GetUnitDefByTypeId( i );
                if( strcmp( thisunitdef->get_name(), "ARMSOLAR" ) == 0 )
                {
                    sprintf( buffer, "Found solar collector def: %i", thisunitdef->get_id() );
                    aicallback->SendTextMsg( buffer, 0 );
                    
                    float3 nearestbuildpos = aicallback->ClosestBuildSite( thisunitdef, commanderpos, 1400,2,0 );
                    sprintf( buffer, "Closest build site: %f %f %f", nearestbuildpos.x, nearestbuildpos.y, nearestbuildpos.z );
                    aicallback->SendTextMsg( buffer, 0 );
                    
                    aicallback->DrawUnit( "ARMSOLAR", nearestbuildpos,0,
                        200, aicallback->GetMyAllyTeam(),true,true,0);
    
					Command c;
					c.id = unit;
					c.params.resize( 3 );
					c.params[0] = nearestbuildpos.x;
					c.params[1] = nearestbuildpos.y;
					c.params[2] = nearestbuildpos.z;
                    aicallback->GiveOrder( unit, &c );
                }
				delete thisunitdef;
            }
        }
    }
    
    void UnitFinished( int unit )
    {
        char buffer[1024];
        sprintf( buffer, "Unit finished: %i", unit );
        aicallback->SendTextMsg( buffer, 0 );
    }

    void UnitIdle( int unit)										//called when a unit go idle and is not assigned to any group
    {
        char buffer[1024];
        sprintf( buffer, "Unit idle: %i", unit );
        aicallback->SendTextMsg( buffer, 0 );
    }
    
    void UnitMoveFailed(int unit)
    {
        char buffer[1024];
        sprintf( buffer, "Unit move failed: %i", unit );
        aicallback->SendTextMsg( buffer, 0 );
    }

private:
    AbicAICallbackWrapper *aicallback;
};

// exported dll symbols, wrap our AI class

// This has to exist to state that we are present a CInterface.
DLL_EXPORT bool IsCInterface()
{
    return true;
}

// returns pointers to our new AI object
DLL_EXPORT void *InitAI( struct IAICallback *aicallback, int team)
{
    MyAI *ai = new MyAI();
    AbicAICallbackWrapper *aicallbackwrapper = new AbicAICallbackWrapper( aicallback );
    ai->InitAI( aicallbackwrapper, team );
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
    //( ( MyAI *)ai )->UnitDestroyed( unit, attacker );
}

DLL_EXPORT void EnemyEnterLOS(void *ai, int enemy)
{
    //( ( MyAI *)ai )->EnemyEnterLOS( enemy );
}

DLL_EXPORT void EnemyLeaveLOS(void *ai, int enemy)
{
    //( ( MyAI *)ai )->EnemyLeaveLOS( enemy );
}

DLL_EXPORT void EnemyEnterRadar(void *ai, int enemy)		
{
    //( ( MyAI *)ai )->EnemyEnterRadar( enemy );
}

DLL_EXPORT void EnemyLeaveRadar(void *ai, int enemy)
{
    //( ( MyAI *)ai )->EnemyLeaveRadar( enemy );
}

DLL_EXPORT void EnemyDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
{
   // ( ( MyAI *)ai )->EnemyDamaged( damaged, attacker ,damage, float3( dirx, diry, dirz ) );
}

DLL_EXPORT void EnemyDestroyed(void *ai, int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
{
   // ( ( MyAI *)ai )->EnemyDestroyed( enemy, attacker );
}

DLL_EXPORT void UnitIdle(void *ai, int unit)										//called when a unit go idle and is not assigned to any group
{
    ( ( MyAI *)ai )->UnitIdle( unit );
}

DLL_EXPORT void GotChatMsg(void *ai, const char* msg,int player)					//called when someone writes a chat msg
{
   // ( ( MyAI *)ai )->GotChatMsg( msg, player );
}

DLL_EXPORT void UnitDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
{
   // ( ( MyAI *)ai )->UnitDamaged( damaged, attacker, damage, float3(dirx, diry, dirz ) );
}

DLL_EXPORT void UnitMoveFailed(void *ai, int unit)
{
    ( ( MyAI *)ai )->UnitMoveFailed( unit );
}

//std::set<IGlobalAI*> ais;

//int HandleEvent (int msg,const void *data); // todo

//called every frame
DLL_EXPORT void Update(void *ai )
{
   // ( ( MyAI *)ai )->Update();
}
