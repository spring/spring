// Copyright Hugh Perkins 2006
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURVector3E. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//
// ======================================================================================
//

// this class wraps the ABIC IAICallback functions

#ifndef _ABICAICALLBACKWRAPPER_H
#define _ABICAICALLBACKWRAPPER_H

#include "AbicAICallback.h"

#include "AbicUnitDefWrapper.h"
#include "AbicFeatureDefWrapper.h"

struct IAICallback;

class AbicAICallbackWrapper
{
        
public:
    IAICallback *self;
    AbicAICallbackWrapper( const IAICallback *self )
    {
        this->self = ( IAICallback *)self;
    }
    
    int GiveOrder( int unit, Command *commandtorts )
    {
        if( commandtorts->params.size() == 0 ){ return IAICallback_GiveOrder( self, unit, commandtorts->id, 0, 0, 0, 0, 0 ); }
        if( commandtorts->params.size() == 1 ){ return IAICallback_GiveOrder( self, unit, commandtorts->id, 1, commandtorts->params[0], 0, 0, 0 ); }
        if( commandtorts->params.size() == 2 ){ return IAICallback_GiveOrder( self, unit, commandtorts->id, 2, commandtorts->params[0], commandtorts->params[1], 0, 0 ); }
        if( commandtorts->params.size() == 3 ){ return IAICallback_GiveOrder( self, unit, commandtorts->id, 3, commandtorts->params[0], commandtorts->params[1], commandtorts->params[2], 0 ); }
        if( commandtorts->params.size() == 4 ){ return IAICallback_GiveOrder( self, unit, commandtorts->id, 3, commandtorts->params[0], commandtorts->params[1], commandtorts->params[2], commandtorts->params[3] ); }
        return 0;
    }
    
    int GetCurrentUnitCommandsCount(int unit )
    {
        return IAICallback_GetCurrentUnitCommandsCount(self, unit);
    }
    
    bool IsGamePaused()
    {
        return IAICallback_IsGamePaused(self);
    }
    
    // please use GetUnitDefByTypeId instead
    //void GetUnitDefList(const AbicUnitDefWrapper**list)
    //{
      //  IAICallback_GetUnitDefList (self, list);
    //}
    
    AbicUnitDefWrapper *GetUnitDefByTypeId( int unittypeid )
    {
        const UnitDef *unitdef = IAICallback_GetUnitDefByTypeId( self, unittypeid );
        if( unitdef == 0 )
        {
            return 0;
        }
        return new AbicUnitDefWrapper( unitdef );
    }

    AbicUnitDefWrapper *GetUnitDef( int deployedid )
    {
        const UnitDef *unitdef = IAICallback_GetUnitDef( self, deployedid );
        if( unitdef == 0 )
        {
            return 0;
        }
        return new AbicUnitDefWrapper( unitdef );
    }

    float3 GetUnitPos( int unitid )
    {
        float3 pos;
        IAICallback_GetUnitPos( self, pos.x, pos.y, pos.z, unitid );
        return pos;
    }
    
    float3 ClosestBuildSite( const AbicUnitDefWrapper *unittobuilddef, float3 &targetpos, double searchRadius, int minDistance, int facing = 0 )
    {
        float3 closestbuildsite;
        IAICallback_ClosestBuildSite( self, closestbuildsite.x, closestbuildsite.y, closestbuildsite.z, unittobuilddef->self, targetpos.x, targetpos.y, targetpos.z, searchRadius, minDistance, facing );
        return closestbuildsite;
    }
    
    void DrawUnit( const char*name, float3 &pos, double rotation, int lifetime, int team, bool transparent, bool drawBorder, int facing = 0 )
    {
        IAICallback_DrawUnit( self, name, pos.x, pos.y, pos.z, rotation, lifetime, team, transparent, drawBorder, facing );
    }
    
    const unsigned char*GetMetalMap()
    {
        return IAICallback_GetMetalMap( self );
    }
    
    const unsigned short*GetLosMap()
    {
        return IAICallback_GetLosMap( self );
    }
    
    const unsigned short*GetRadarMap()
    {
        return IAICallback_GetRadarMap( self );
    }
    
    const float *GetHeightMap()
    {
        return IAICallback_GetHeightMap( self );
    }
    
    bool CanBuildAt( const AbicUnitDefWrapper *unitDef, float3 &pos, int facing = 0 )
    {
        return IAICallback_CanBuildAt( self, unitDef->self, pos.x, pos.y, pos.z, facing );
    }
    
    int GetFriendlyUnits( int *units )
    {
        return IAICallback_GetFriendlyUnits( self, units );
    }
    
    int GetEnemyUnitsInRadarAndLos( int *units )
    {
        return IAICallback_GetEnemyUnitsInRadarAndLos( self, units );
    }

/*    
    CSharpAI::Command *GetCurrentUnitCommands(int unitid)[]
    {
        const deque<::Command> *commands = callbacktorts->GetCurrentUnitCommands( unitid );    
        CSharpAI::Command *commandsforcs[] = new CSharpAI::Command*[ commands.size() ];
        for( int i = 0; i < commands.size(); i++ )
        {
            ::Command *thiscommandcpp = commands[i];
            commandsforcs[i] = new CSharpAI::Command( unitList[i] );
        }
        return unitlistforcs;
    }
    */
    
    //const std::vector<CommandDescription>* GetGroupCommands(int unitid);	//the commands that this unit can understand, other commands will be ignored
    /*
    int GiveGroupOrder(int unitid, CSharpAI::Command *c)
    {
        return callbacktorts->GiveGroupOrder( unitid, CsCommandToCppCommand( c ) );
    }
    */
    int CreateLineFigure(float3 pos1,float3 pos2,double width,bool arrow,int lifetime,int group)
    {
        return IAICallback_CreateLineFigure( self, pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, width, arrow, lifetime, group );
    }
    
    int GetFeatures ( int *features, int max)
    {
        return IAICallback_GetFeatures( self, features, max );
    }
    
    int GetFeaturesAt ( int *features, int max, float3 pos, float radius)
    {
        return IAICallback_GetFeaturesAt( self, features, max, pos.x, pos.y, pos.z, radius );
    }
    
    AbicFeatureDefWrapper *GetFeatureDef (int feature) 
    {
        const FeatureDef *featuredef = IAICallback_GetFeatureDef( self, feature );
        if( featuredef == 0 )
        {
            return 0;
        }
        return new AbicFeatureDefWrapper( featuredef );
    }

    float3 GetFeaturePos ( int feature)
    {
        float3 pos;
        IAICallback_GetFeaturePos( self, pos.x, pos.y, pos.z, feature );
        return pos;
    }
    
    #include "AbicIAICallbackWrapper_generated.h"
};

#endif
