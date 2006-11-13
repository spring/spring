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

// This class proxies callbacks from the C# AI back up to the spring game engine
// This is a managed class, but it contains a pointer to the unmanaged rts callback
// so we can call into that directly
//
// This class is probably the biggest dev timesink for creating a C# interface for AIs

#ifndef _AICALLBACKFORCS_H
#define _AICALLBACKFORCS_H

#using <mscorlib.dll>
#include <vcclr.h>
using namespace System;
using namespace System::Runtime::InteropServices;    
    
#include "ExternalAI/IAICallback.h"
#include "ExternalAI/AICallback.h"
    
#include "UnitDefProxy.h"
#include "FeatureDefProxy.h"

void WriteLine( System::String *Message );

__gc class AICallbackToGiveToCS : public CSharpAI::IAICallback
{
    ::IAICallback *callbacktorts;
    
    public:
    AICallbackToGiveToCS( ::IAICallback *callbacktorts )
    {
        this->callbacktorts = callbacktorts;
    }

    void CSFloat3ToCppFloat3( float3 &cppfloat3, CSharpAI::Float3 *csfloat3 )
    {
        cppfloat3.x = csfloat3->x;
        cppfloat3.y = csfloat3->y;
        cppfloat3.z = csfloat3->z;      
    }

    ::Command *CsCommandToCppCommand( CSharpAI::Command *cscommand )
    {
        ::Command *commandtorts = new ::Command();
        commandtorts->id = cscommand->id;
        commandtorts->params.resize( cscommand->parameters->Count );
        for( int i = 0; i < cscommand->parameters->Count; i++ )
        {
            commandtorts->params[ i ] = cscommand->parameters[ i ];
        }
        return commandtorts;
    }
    
    CSharpAI::Float3 *CppFloat3ToCSFloat3( float3 &cppfloat3 )
    {
        if( cppfloat3.x != 0 && cppfloat3.y != 0 && cppfloat3.z != 0 )
        {
            return new CSharpAI::Float3( cppfloat3.x, cppfloat3.y, cppfloat3.z );
        }
        else
        {
            return 0;
        }
    }
    
    void SendTextMsg( System::String* text, int priority )
    {
        char* messageansi = (char*)(void*)Marshal::StringToHGlobalAnsi(text);
        callbacktorts->SendTextMsg( messageansi, priority );
        Marshal::FreeHGlobal(messageansi);
    }    

    CSharpAI::IUnitDef* GetUnitDef(int unitid)
    {
        const ::UnitDef *unitdef = callbacktorts->GetUnitDef( unitid );
        if( unitdef != 0 )
        {
            return new UnitDefForCSharp( unitdef );
        }
        return 0;
    }
    
    void AddMapPoint( CSharpAI::Float3 *pos, System::String *label )
    {
        AIHCAddMapPoint aihcaddmappoint;
        CSFloat3ToCppFloat3( aihcaddmappoint.pos, pos );
        char* labelansi = (char*)(void*)Marshal::StringToHGlobalAnsi(label);
        sprintf( aihcaddmappoint.label, labelansi );
        callbacktorts->HandleCommand( AIHCAddMapPointId, &aihcaddmappoint );
        Marshal::FreeHGlobal(labelansi);        
    }
    
    int GiveOrder( int unit, CSharpAI::Command *command )
    {
        ::Command *commandtorts = new ::Command();
        commandtorts->id = command->id;
        commandtorts->params.resize( command->parameters->Count );
        for( int i = 0; i < command->parameters->Count; i++ )
        {
            commandtorts->params[ i ] = command->parameters[ i ];
        }
        return callbacktorts->GiveOrder( unit, commandtorts );
    }
    
    CSharpAI::IUnitDef *GetUnitDefList() []
    {
        int numOfUnits = callbacktorts->GetNumUnitDefs();    
        const UnitDef **unitList = new const UnitDef*[numOfUnits];
        callbacktorts->GetUnitDefList(unitList);
        UnitDefForCSharp *unitlistforcs[] = new UnitDefForCSharp*[ numOfUnits ];
        for( int i = 0; i < numOfUnits; i++ )
        {
            unitlistforcs[i] = new UnitDefForCSharp( unitList[i] );
        }
        return unitlistforcs;
    }
    
    CSharpAI::Float3 *GetUnitPos( int unitid )
    {
        return CppFloat3ToCSFloat3( callbacktorts->GetUnitPos( unitid ) );
    }
    
    CSharpAI::Float3 *ClosestBuildSite( CSharpAI::IUnitDef *unittobuilddef, CSharpAI::Float3 *targetpos, double searchRadius, int minDistance )
    {
        float3 targetposforcpp;
        CSFloat3ToCppFloat3( targetposforcpp, targetpos );
        CSharpAI::Float3 *result = CppFloat3ToCSFloat3(
            callbacktorts->ClosestBuildSite(
                ( dynamic_cast< UnitDefForCSharp * >( unittobuilddef ) )->actualunitdef, 
                targetposforcpp, searchRadius, minDistance ) );
        return result;
    }
    
    void DrawUnit( System::String* name, CSharpAI::Float3 *pos, double rotation, int lifetime, int team, bool transparent, bool drawBorder )
    {
        char* nameansi = (char*)(void*)Marshal::StringToHGlobalAnsi(name);
        
        float3 targetposforcpp;
        CSFloat3ToCppFloat3( targetposforcpp, pos );
        callbacktorts->DrawUnit( nameansi, targetposforcpp, rotation, lifetime, team, transparent, drawBorder );
        
        Marshal::FreeHGlobal(nameansi);
    }
    
    // I'd like to return a double dimension array, but this crashes the C#/C++ interface, so we stick with one-dimension version
    // note to self: maybe this will work now, using mapwidth/2
    System::Byte GetMetalMap()[]
    {     
        
        const unsigned char*mapfromrts = callbacktorts->GetMetalMap();
        int width = GetMapWidth() / 2;//metal map has 1/2 resolution of normal map // WHY???? THIS TOOK SOOOO LONG TO DEBUG </rant>
        int height = GetMapHeight() / 2;
        int numbercells = width * height;
        //int numbercells = 0;
        
        System::Byte mapforcs[] = new System::Byte[ numbercells ];
        for( int i = 0; i < numbercells; i++ )
        {
            mapforcs[i] = mapfromrts[i];
        }
        return mapforcs;
        //return new System::Byte[0];
    }
    
    System::Boolean GetLosMap()[]
    {
        const unsigned short*losmapfromrts = callbacktorts->GetLosMap();
        int width = GetMapWidth() / 2;
        int height = GetMapHeight() / 2;
        int numbercells = width * height;
        
        System::Boolean mapforcs[] = new System::Boolean[ numbercells ];
//        for( int x = 0; x < width; x++ )
  //      {
    //        for( int y = 0; y < height; y++ )
      //      {
                // dont need to set to false, already false by default
        for( int i = 0; i < numbercells; i++ )
        {
                if( losmapfromrts[ i ] != 0 )
                {
                    mapforcs[ i ] = true;
                }
        }
         //   }
       // }
        return mapforcs;
    }

    System::Boolean GetRadarMap()[]
    {
        const unsigned short*mapfromrts = callbacktorts->GetRadarMap();
        int width = GetMapWidth() / 8;
        int height = GetMapHeight() / 8;
        int numbercells = width * height;
        
        System::Boolean mapforcs[] = new System::Boolean[ numbercells ];
        for( int i = 0; i < numbercells; i++ )
        {
                if( mapfromrts[ i ] != 0 )
                {
                    mapforcs[ i ] = true;
                }
        }
        return mapforcs;
    }
    
    /*
    double GetSlopeMap() __gc []
    {
        const float *mapfromrts = ( dynamic_cast< CAICallback *>( callbacktorts ) )->GetSlopeMap();
        int width = GetMapWidth();
        int height = GetMapHeight();
        int numbercells = width * height;
        double mapforcs __gc [] = new double __gc[ numbercells ];
        for( int i = 0; i < numbercells; i++ )
        {
            mapforcs[ i ] = mapfromrts[ i ];
        }
        return mapforcs;
    }
    */
    
    double GetCentreHeightMap() __gc []
    {
        const float *mapfromrts = callbacktorts->GetHeightMap();
        int width = GetMapWidth();
        int height = GetMapHeight();
        int numbercells = width * height;
        double mapforcs __gc [] = new double __gc[ numbercells ];
        for( int i = 0; i < numbercells; i++ )
        {
            mapforcs[ i ] = mapfromrts[ i ];
        }
        return mapforcs;
    }

    CSharpAI::MapPoint *GetMapPoints()[]
    {
        PointMarker pointmarkers[1000];
        int numpoints = callbacktorts->GetMapPoints(pointmarkers, 1000);
        char messagebuffer[1024];
        CSharpAI::MapPoint *mappointsforcs[] = new CSharpAI::MapPoint *[ numpoints ];
        for( int i = 0 ; i < numpoints; i++ )
        {
            mappointsforcs[i] = new CSharpAI::MapPoint();
            mappointsforcs[i]->Pos = new CSharpAI::Float3( pointmarkers[i].pos.x, pointmarkers[i].pos.y, pointmarkers[i].pos.z );
          //  mappointsforcs[i]->Color = pointmarkers[i].color;
            mappointsforcs[i]->Label = new System::String( pointmarkers[i].label );
        }
        return mappointsforcs;
    }
    
    System::String *GetMapName()
    {
        return new System::String( callbacktorts->GetMapName() );
       // return new System::String( "TESTING 0.2" );
    }
    
    System::String *GetModName()
    {
        return new System::String( callbacktorts->GetModName() );
    }
    
    bool CanBuildAt( CSharpAI::IUnitDef *unitDef, CSharpAI::Float3 *pos )
    {
        float3 targetposforcpp;
        CSFloat3ToCppFloat3( targetposforcpp, pos );
        return callbacktorts->CanBuildAt( ( dynamic_cast< UnitDefForCSharp * >( unitDef ) )->actualunitdef,
            targetposforcpp );
    }
    
    System::Int32 GetFriendlyUnits()[]
    {
        // how to find out how big to make this array???? Edit: documentation says maximum 10000
        int units[10000];
        int numunits = callbacktorts->GetFriendlyUnits( units );
        System::Int32 unitsforcs[] = new System::Int32[ numunits ];
        for( int i = 0; i < numunits; i++ )
        {
            unitsforcs[i] = units[i];
        }
        return unitsforcs;
    }
    
    System::Int32 GetEnemyUnitsInRadarAndLos()[]
    {
        // how to find out how big to make this array???? Edit: documentation says maximum 10000
        int units[10000];
        int numunits = callbacktorts->GetEnemyUnitsInRadarAndLos( units );
        System::Int32 unitsforcs[] = new System::Int32[ numunits ];
        for( int i = 0; i < numunits; i++ )
        {
            unitsforcs[i] = units[i];
        }
        return unitsforcs;
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
    
    // simplified/fast version of GetCurrentUnitCommands
    bool UnitIsBusy( int unitid )
    {
        if( callbacktorts->GetCurrentUnitCommands( unitid )->empty() )
        {
            return false;
        }
        return true;
    }
    
    int CreateGroup( System::String *dll, int aiNumber)							//creates a group and return the id it was given, return -1 on failure (dll didnt exist etc)
    {
        char* dllansi = (char*)(void*)Marshal::StringToHGlobalAnsi(dll);
        int groupid = callbacktorts->CreateGroup( dllansi, aiNumber );
        Marshal::FreeHGlobal(dllansi);
        return groupid;
    }
    
    void EraseGroup(int groupid)										//erases a specified group
    {
        return callbacktorts->EraseGroup( groupid );
    }
    
    //const std::vector<CommandDescription>* GetGroupCommands(int unitid);	//the commands that this unit can understand, other commands will be ignored
    
    int GiveGroupOrder(int unitid, CSharpAI::Command *c)
    {
        return callbacktorts->GiveGroupOrder( unitid, CsCommandToCppCommand( c ) );
    }
    
    int CreateLineFigure(CSharpAI::Float3 *pos1,CSharpAI::Float3 *pos2,double width,bool arrow,int lifetime,int group)
    {
        float3 targetposforcpp1;
        CSFloat3ToCppFloat3( targetposforcpp1, pos1 );
        float3 targetposforcpp2;
        CSFloat3ToCppFloat3( targetposforcpp2, pos2 );
        return callbacktorts->CreateLineFigure( targetposforcpp1, targetposforcpp2, width, arrow, lifetime, group );
    }
    
    void SetFigureColor(int group,double red,double green,double blue,double alpha)
    {
        callbacktorts->SetFigureColor( group, red, green, blue, alpha );
    }
    
    void DeleteFigureGroup(int group)
    {
        callbacktorts->DeleteFigureGroup( group );
    }
    
    bool IsGamePaused()
    {
        bool gameispaused;
        callbacktorts->GetValue( AIVAL_GAME_PAUSED, &gameispaused );
        return gameispaused;
    }
    
    int GetFeatures () __gc []
    {
        int features[10000];
        int numfeatures = callbacktorts->GetFeatures( features, 10000 );
        int csfeatures __gc [] = new int __gc[numfeatures];
        for( int i = 0; i < numfeatures; i++ )
        {
            csfeatures[i] = features[i];
        }
        return csfeatures;
    }
    
    int GetFeatures ( CSharpAI::Float3 *pos, double radius) __gc []
    {
        int features[10000];
        float3 targetposforcpp;
        CSFloat3ToCppFloat3( targetposforcpp, pos );
        int numfeatures = callbacktorts->GetFeatures( features, 10000, targetposforcpp, radius );
        int csfeatures __gc [] = new int __gc[numfeatures];
        for( int i = 0; i < numfeatures; i++ )
        {
            csfeatures[i] = features[i];
        }
        return csfeatures;
    }
    
    CSharpAI::IFeatureDef *GetFeatureDef (int feature) 
    {
        const ::FeatureDef *featuredef = callbacktorts->GetFeatureDef( feature );
        if( featuredef != 0 )
        {
            return new FeatureDefForCSharp( featuredef );
        }
        return 0;
    }

    CSharpAI::Float3 *GetFeaturePos (int feature)
    {
        return CppFloat3ToCSFloat3( callbacktorts->GetFeaturePos( feature ) );
    }

    // **********************************************************************************************************************
    // all beyond this point was auto-generated by GenerateIACallbackClasses, then copy and pasted in:
    // **********************************************************************************************************************
    
    int GetCurrentFrame(  )
    {
        return callbacktorts->GetCurrentFrame(  );
    }

    int GetMyTeam(  )
    {
        return callbacktorts->GetMyTeam(  );
    }

    int GetMyAllyTeam(  )
    {
        return callbacktorts->GetMyAllyTeam(  );
    }

    int GetPlayerTeam( int player )
    {
        return callbacktorts->GetPlayerTeam( player );
    }

    bool AddUnitToGroup( int unitid, int groupid )
    {
        return callbacktorts->AddUnitToGroup( unitid, groupid );
    }

    bool RemoveUnitFromGroup( int unitid )
    {
        return callbacktorts->RemoveUnitFromGroup( unitid );
    }

    int GetUnitGroup( int unitid )
    {
        return callbacktorts->GetUnitGroup( unitid );
    }

    int GetUnitAiHint( int unitid )
    {
        return callbacktorts->GetUnitAiHint( unitid );
    }

    int GetUnitTeam( int unitid )
    {
        return callbacktorts->GetUnitTeam( unitid );
    }

    int GetUnitAllyTeam( int unitid )
    {
        return callbacktorts->GetUnitAllyTeam( unitid );
    }

    double GetUnitHealth( int unitid )
    {
        return callbacktorts->GetUnitHealth( unitid );
    }

    double GetUnitMaxHealth( int unitid )
    {
        return callbacktorts->GetUnitMaxHealth( unitid );
    }

    double GetUnitSpeed( int unitid )
    {
        return callbacktorts->GetUnitSpeed( unitid );
    }

    double GetUnitPower( int unitid )
    {
        return callbacktorts->GetUnitPower( unitid );
    }

    double GetUnitExperience( int unitid )
    {
        return callbacktorts->GetUnitExperience( unitid );
    }

    double GetUnitMaxRange( int unitid )
    {
        return callbacktorts->GetUnitMaxRange( unitid );
    }

    bool IsUnitActivated( int unitid )
    {
        return callbacktorts->IsUnitActivated( unitid );
    }

    bool UnitBeingBuilt( int unitid )
    {
        return callbacktorts->UnitBeingBuilt( unitid );
    }

    int GetBuildingFacing( int unitid )
    {
        return callbacktorts->GetBuildingFacing( unitid );
    }

    bool IsUnitCloaked( int unitid )
    {
        return callbacktorts->IsUnitCloaked( unitid );
    }

    bool IsUnitParalyzed( int unitid )
    {
        return callbacktorts->IsUnitParalyzed( unitid );
    }

    int GetMapWidth(  )
    {
        return callbacktorts->GetMapWidth(  );
    }

    int GetMapHeight(  )
    {
        return callbacktorts->GetMapHeight(  );
    }

    double GetMaxMetal(  )
    {
        return callbacktorts->GetMaxMetal(  );
    }

    double GetExtractorRadius(  )
    {
        return callbacktorts->GetExtractorRadius(  );
    }

    double GetMinWind(  )
    {
        return callbacktorts->GetMinWind(  );
    }

    double GetMaxWind(  )
    {
        return callbacktorts->GetMaxWind(  );
    }

    double GetTidalStrength(  )
    {
        return callbacktorts->GetTidalStrength(  );
    }

    double GetGravity(  )
    {
        return callbacktorts->GetGravity(  );
    }

    double GetElevation( double x, double z )
    {
        return callbacktorts->GetElevation( x, z );
    }

    double GetMetal(  )
    {
        return callbacktorts->GetMetal(  );
    }

    double GetMetalIncome(  )
    {
        return callbacktorts->GetMetalIncome(  );
    }

    double GetMetalUsage(  )
    {
        return callbacktorts->GetMetalUsage(  );
    }

    double GetMetalStorage(  )
    {
        return callbacktorts->GetMetalStorage(  );
    }

    double GetEnergy(  )
    {
        return callbacktorts->GetEnergy(  );
    }

    double GetEnergyIncome(  )
    {
        return callbacktorts->GetEnergyIncome(  );
    }

    double GetEnergyUsage(  )
    {
        return callbacktorts->GetEnergyUsage(  );
    }

    double GetEnergyStorage(  )
    {
        return callbacktorts->GetEnergyStorage(  );
    }

    double GetFeatureHealth( int feature )
    {
        return callbacktorts->GetFeatureHealth( feature );
    }

    double GetFeatureReclaimLeft( int feature )
    {
        return callbacktorts->GetFeatureReclaimLeft( feature );
    }

    int GetNumUnitDefs(  )
    {
        return callbacktorts->GetNumUnitDefs(  );
    }

    double GetUnitDefRadius( int def )
    {
        return callbacktorts->GetUnitDefRadius( def );
    }

    double GetUnitDefHeight( int def )
    {
        return callbacktorts->GetUnitDefHeight( def );
    }
};

#endif // _AICALLBACKFORCS_H
