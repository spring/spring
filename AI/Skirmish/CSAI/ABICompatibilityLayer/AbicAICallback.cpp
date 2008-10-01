
#include "StdAfx.h"

//#include "ExternalAI/aibase.h" // for DLL_EXPORT definition

#include "AbicAICallback.h"

#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDefHandler.h"

//extern ::IAICallback* aicallback;

// Add generated definitions:
#include "IAICallback_generated.gpp"
#include "IFeatureDef_generated.gpp"
#include "IMoveData_generated.gpp"
#include "IUnitDef_generated.gpp"

//todo (ie, exist in CSAIInterfaces)
//(givegrouporder)

//addmappoint
//getmappoints

// for reference, here is the definition of GetCurrentUnitCommands(): virtual const CCommandQueue* GetCurrentUnitCommands(int unitid);
AICALLBACK_API int IAICallback_GetCurrentUnitCommandsCount( const IAICallback *self, int unitid )
{
    const CCommandQueue*commands = ( (IAICallback *)self)->GetCurrentUnitCommands( unitid );
    return commands->size();
}

AICALLBACK_API const UnitDef *IAICallback_GetUnitDefByTypeId (const IAICallback *self, int unittypeid)
{
    //return unitDefHandler->GetUnitByID (unittypeid); // *cross fingers that dont have to add/subtract 1 to unittypeid
    return 0; // placeholder
}

// for reference, here is the definition of buildOptions: std::map<int,std::string> buildOptions;
AICALLBACK_API int UnitDef_GetNumBuildOptions( const UnitDef *self )
{
    return ( (UnitDef *)self )->buildOptions.size();
}

// assumes map is really a vector where the int is a contiguous index starting from 1
AICALLBACK_API const char *UnitDef_GetBuildOption( const UnitDef *self, int index )
{
    return ( (UnitDef *)self )->buildOptions[index + 1].c_str();
}

AICALLBACK_API int IAICallback_CreateLineFigure( const IAICallback *self, float pos1x, float pos1y, float pos1z,float pos2x, float pos2y, float pos2z,
    float width,int arrow,int lifetime,int group)
{
    return ( (IAICallback *)self)->CreateLineFigure( float3( pos1x, pos1y, pos1z ), float3(pos2x, pos2y, pos2z ),
       width, arrow, lifetime, group );
}

AICALLBACK_API bool IAICallback_IsGamePaused( const IAICallback *self )
{
    bool paused;
    ( (IAICallback *)self)->GetValue( AIVAL_GAME_PAUSED, &paused);
    return paused;
}

AICALLBACK_API void IAICallback_DrawUnit(const IAICallback *self, const char* name,float posx, float posy, float posz,float rotation,
    int lifetime,int team,bool transparent,bool drawBorder,int facing)
{
    ( (IAICallback *)self)->DrawUnit( name, float3( posx, posy, posz ), rotation, lifetime, team, transparent, drawBorder, facing );
}

AICALLBACK_API int MoveData_get_movetype( const MoveData *self )
{
    return ( (MoveData *)self )->moveType;
}

AICALLBACK_API const MoveData *UnitDef_get_movedata( const UnitDef *self  )
{
    return self->movedata;
}

AICALLBACK_API const FeatureDef *IAICallback_GetFeatureDef( const IAICallback *self, int featuredef  )
{
    return ( (IAICallback *)self)->GetFeatureDef( featuredef );
}

AICALLBACK_API int IAICallback_GetFeatures( const IAICallback *self, int *features, int max )
{
    return ( (IAICallback *)self)->GetFeatures( features, max );
}

AICALLBACK_API int IAICallback_GetFeaturesAt(const IAICallback *self, int *features, int max, float posx, float posy, float posz, float radius)
{
    return ( ( IAICallback *)self )->GetFeatures( features, max, float3( posx, posy, posz ), radius );
}

AICALLBACK_API void IAICallback_GetFeaturePos ( const IAICallback *self, float &posx, float&posy, float&posz, int feature)
{
    float3 pos = ( ( IAICallback *)self )->GetFeaturePos( feature );
    posx = pos.x;
    posy = pos.y;
    posz = pos.z;
}

 // This function is deprecated, please use IAICallback_GetUnitDefByTypeId
AICALLBACK_API void IAICallback_GetUnitDefList (const IAICallback *self, const UnitDef** list)
{
    ( ( IAICallback *)self )->GetUnitDefList( list );
}

AICALLBACK_API void IAICallback_GetUnitPos( const IAICallback *self, float &posx, float&posy, float&posz, int unitid)
{
    float3 pos = ( ( IAICallback *)self )->GetUnitPos( unitid );
    posx = pos.x;
    posy = pos.y;
    posz = pos.z;
}

AICALLBACK_API int IAICallback_GiveOrder( const IAICallback *self, int unitid, int commandid, int numparams, float param1, float param2, float param3, float param4 )
{
    Command c;
    c.id = commandid;
    c.params.resize( numparams );
    if( numparams >= 1 ){ c.params[0] = param1; }
    if( numparams >= 2 ){ c.params[1] = param2; }
    if( numparams >= 3 ){ c.params[2] = param3; }
    if( numparams >= 4 ){ c.params[3] = param4; }
    return ( ( IAICallback *)self )->GiveOrder( unitid, &c );
}


AICALLBACK_API bool IAICallback_CanBuildAt( const IAICallback *self, const UnitDef* unitDef, float posx, float posy, float posz,int facing )
{
    return ( ( IAICallback *)self )->CanBuildAt( unitDef, float3( posx, posy, posz ), facing );
}

AICALLBACK_API void IAICallback_ClosestBuildSite( const IAICallback *self, float &resultx, float&resulty, float&resultz, const UnitDef* unitdef, float posx, float posy, float posz,float searchRadius,int minDist,int facing)	//returns the closest position from a position that the building can be built, minDist is the distance in squares that the building must keep to other buildings (to make it easier to create paths through a base)
{
    float3 nearestbuildsite = ( ( IAICallback *)self )->ClosestBuildSite( unitdef, float3( posx, posy, posz ), searchRadius, minDist, facing );
    resultx = nearestbuildsite.x;
    resulty = nearestbuildsite.y;
    resultz = nearestbuildsite.z;
}


AICALLBACK_API int IAICallback_GetEnemyUnitsInRadarAndLos( const IAICallback *self, int *units)       //returns all enemy units in radar and los
{
    return ( (IAICallback *)self)->GetEnemyUnitsInRadarAndLos( units );
}

AICALLBACK_API int IAICallback_GetFriendlyUnits( const IAICallback *self, int *units )					//returns all friendly units
{
    return ( (IAICallback *)self)->GetFriendlyUnits( units );
}


AICALLBACK_API const float* IAICallback_GetHeightMap( const IAICallback *self )						//this is the height for the center of the squares, this differs slightly from the drawn map since it uses the height at the corners
{
    return ( (IAICallback *)self)->GetHeightMap();
}

AICALLBACK_API const unsigned short* IAICallback_GetLosMap( const IAICallback *self )			//a square with value zero means you dont have los to the square, this is half the resolution of the standard map
{
    return ( (IAICallback *)self)->GetLosMap();
}

AICALLBACK_API const unsigned short* IAICallback_GetRadarMap( const IAICallback *self )		//a square with value zero means you dont have radar to the square, this is 1/8 the resolution of the standard map
{
    return ( (IAICallback *)self)->GetRadarMap();
}

AICALLBACK_API const unsigned short* IAICallback_GetJammerMap( const IAICallback *self )		//a square with value zero means you dont have radar jamming on the square, this is 1/8 the resolution of the standard map
{
    return ( (IAICallback *)self)->GetJammerMap();
}

AICALLBACK_API const unsigned char* IAICallback_GetMetalMap( const IAICallback *self )			//this map shows the metal density on the map, this is half the resolution of the standard map
{
    return ( (IAICallback *)self)->GetMetalMap();
}


