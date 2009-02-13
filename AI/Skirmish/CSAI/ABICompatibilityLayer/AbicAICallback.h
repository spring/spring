
// #include "ExternalAI/aibase.h" // for SHARED_EXPORT definition

#include "dllbuild.h"

#ifdef BUILDING_SKIRMISH_AI
	// declare these as opaque pointers
	struct UnitDef;
	struct MoveData;
	struct FeatureDef;
	struct IAICallback;
#else
	#include <map>
	#include <set>
	#include <stdio.h>
	#include "float3.h"

	#include "ExternalAI/IGlobalAICallback.h"
	#include "ExternalAI/IAICallback.h"
	#include "Sim/Units/UnitDef.h"
	#include "Sim/Features/FeatureDef.h"
	#include "Sim/MoveTypes/MoveInfo.h" // for MoveData struct
#endif

// Add generated headers:
#include "IAICallback_generated.h"
#include "IFeatureDef_generated.h"
#include "IMoveData_generated.h"
#include "IUnitDef_generated.h"

#define GLOBALAI_C_INTERFACE_VERSION 2
    
AICALLBACK_API const MoveData *UnitDef_get_movedata( const UnitDef *self  );
AICALLBACK_API const FeatureDef *IAICallback_GetFeatureDef( const IAICallback *self, int featuredef  );
AICALLBACK_API int IAICallback_GetFeatures( const IAICallback *self, int *features, int max );
AICALLBACK_API int IAICallback_GetFeaturesAt(const IAICallback *self, int *features, int max, float posx, float posy, float posz, float radius);
AICALLBACK_API void IAICallback_GetFeaturePos( const IAICallback *self, float &posx, float&posy, float&posz, int featureid);
AICALLBACK_API void IAICallback_GetUnitDefList (const IAICallback *self, const UnitDef** list); // This function is deprecated, please use IAICallback_GetUnitDefByTypeId
AICALLBACK_API const UnitDef *IAICallback_GetUnitDefByTypeId (const IAICallback *self, int unittypeid);
AICALLBACK_API void IAICallback_GetUnitPos( const IAICallback *self, float &posx, float&posy, float&posz, int unitid);
AICALLBACK_API int IAICallback_GiveOrder( const IAICallback *self, int unitid, int commandid, int numparams, float param1, float param2, float param3, float param4 );
AICALLBACK_API bool IAICallback_CanBuildAt( const IAICallback *self, const UnitDef* unitDef, float posx, float posy, float posz,int facing );
AICALLBACK_API void IAICallback_ClosestBuildSite( const IAICallback *self, float &resultx, float&resulty, float&resultz, const UnitDef* unitdef, float posx, float posy, float posz,float searchRadius,int minDist,int facing);	//returns the closest position from a position that the building can be built, minDist is the distance in squares that the building must keep to other buildings (to make it easier to create paths through a base)
AICALLBACK_API int IAICallback_GetEnemyUnitsInRadarAndLos( const IAICallback *self, int *units);       //returns all enemy units in radar and los
AICALLBACK_API int IAICallback_GetFriendlyUnits( const IAICallback *self, int *units );					//returns all friendly units
AICALLBACK_API const float* IAICallback_GetHeightMap( const IAICallback *self );						//this is the height for the center of the squares, this differs slightly from the drawn map since it uses the height at the corners
AICALLBACK_API const unsigned short* IAICallback_GetLosMap( const IAICallback *self );			//a square with value zero means you dont have los to the square, this is half the resolution of the standard map
AICALLBACK_API const unsigned short* IAICallback_GetRadarMap( const IAICallback *self );		//a square with value zero means you dont have radar to the square, this is 1/8 the resolution of the standard map
AICALLBACK_API const unsigned short* IAICallback_GetJammerMap( const IAICallback *self );		//a square with value zero means you dont have radar jamming on the square, this is 1/8 the resolution of the standard map
AICALLBACK_API const unsigned char* IAICallback_GetMetalMap( const IAICallback *self );			//this map shows the metal density on the map, this is half the resolution of the standard map

AICALLBACK_API int IAICallback_GetCurrentUnitCommandsCount( const IAICallback *self, int unitid );
AICALLBACK_API int UnitDef_GetNumBuildOptions( const UnitDef *self );
AICALLBACK_API const char *UnitDef_GetBuildOption( const UnitDef *self, int index ); // assumes map is really a vector where the int is a contiguous index starting from 1
AICALLBACK_API int IAICallback_CreateLineFigure( const IAICallback *self, float pos1x, float pos1y, float pos1z,float pos2x, float pos2y, float pos2z,
    float width,int arrow,int lifetime,int group);
AICALLBACK_API bool IAICallback_IsGamePaused( const IAICallback *self );
AICALLBACK_API void IAICallback_DrawUnit(const IAICallback *self, const char* name,float posx, float posy, float posz,float rotation,
    int lifetime,int team,bool transparent,bool drawBorder,int facing);
AICALLBACK_API int MoveData_get_movetype( const MoveData *self );

