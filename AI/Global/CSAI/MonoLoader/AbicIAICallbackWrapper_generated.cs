// *** This is a generated file; if you want to change it, please change CSAIInterfaces.dll, which is the reference
// 
// This file was generated by MonoAbicWrappersGenerator, by Hugh Perkins hughperkins@gmail.com http://manageddreams.com
// 

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;

namespace CSharpAI
{
    public class AICallback
    {
        public IntPtr self = IntPtr.Zero;
        public AICallback( IntPtr self )
        {
           this.self = self;
        }
      public System.Void SendTextMsg(System.String text, System.Int32 priority )

      {
         ABICInterface.IAICallback_SendTextMsg( self, text, priority );
      }

      public System.Int32 CreateGroup(System.String dll, System.Int32 aiNumber )

      {
         return ABICInterface.IAICallback_CreateGroup( self, dll, aiNumber );
      }

      public System.Void EraseGroup(System.Int32 groupid )

      {
         ABICInterface.IAICallback_EraseGroup( self, groupid );
      }

      public System.String GetModName( )

      {
         return ABICInterface.IAICallback_GetModName( self );
      }

      public System.String GetMapName( )

      {
         return ABICInterface.IAICallback_GetMapName( self );
      }

      public System.Void SetFigureColor(System.Int32 group, System.Double red, System.Double green, System.Double blue, System.Double alpha )

      {
         ABICInterface.IAICallback_SetFigureColor( self, group, red, green, blue, alpha );
      }

      public System.Void DeleteFigureGroup(System.Int32 group )

      {
         ABICInterface.IAICallback_DeleteFigureGroup( self, group );
      }

      public System.Boolean IsGamePaused( )

      {
         return ABICInterface.IAICallback_IsGamePaused( self );
      }

      public System.Int32 GetCurrentFrame( )

      {
         return ABICInterface.IAICallback_GetCurrentFrame( self );
      }

      public System.Int32 GetMyTeam( )

      {
         return ABICInterface.IAICallback_GetMyTeam( self );
      }

      public System.Int32 GetMyAllyTeam( )

      {
         return ABICInterface.IAICallback_GetMyAllyTeam( self );
      }

      public System.Int32 GetPlayerTeam(System.Int32 player )

      {
         return ABICInterface.IAICallback_GetPlayerTeam( self, player );
      }

      public System.Boolean AddUnitToGroup(System.Int32 unitid, System.Int32 groupid )

      {
         return ABICInterface.IAICallback_AddUnitToGroup( self, unitid, groupid );
      }

      public System.Boolean RemoveUnitFromGroup(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_RemoveUnitFromGroup( self, unitid );
      }

      public System.Int32 GetUnitGroup(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitGroup( self, unitid );
      }

      public System.Int32 GetUnitAiHint(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitAiHint( self, unitid );
      }

      public System.Int32 GetUnitTeam(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitTeam( self, unitid );
      }

      public System.Int32 GetUnitAllyTeam(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitAllyTeam( self, unitid );
      }

      public System.Double GetUnitHealth(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitHealth( self, unitid );
      }

      public System.Double GetUnitMaxHealth(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitMaxHealth( self, unitid );
      }

      public System.Double GetUnitSpeed(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitSpeed( self, unitid );
      }

      public System.Double GetUnitPower(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitPower( self, unitid );
      }

      public System.Double GetUnitExperience(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitExperience( self, unitid );
      }

      public System.Double GetUnitMaxRange(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetUnitMaxRange( self, unitid );
      }

      public System.Boolean IsUnitActivated(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_IsUnitActivated( self, unitid );
      }

      public System.Boolean UnitBeingBuilt(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_UnitBeingBuilt( self, unitid );
      }

      public System.Int32 GetBuildingFacing(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_GetBuildingFacing( self, unitid );
      }

      public System.Boolean IsUnitCloaked(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_IsUnitCloaked( self, unitid );
      }

      public System.Boolean IsUnitParalyzed(System.Int32 unitid )

      {
         return ABICInterface.IAICallback_IsUnitParalyzed( self, unitid );
      }

      public System.Int32 GetMapWidth( )

      {
         return ABICInterface.IAICallback_GetMapWidth( self );
      }

      public System.Int32 GetMapHeight( )

      {
         return ABICInterface.IAICallback_GetMapHeight( self );
      }

      public System.Double GetMaxMetal( )

      {
         return ABICInterface.IAICallback_GetMaxMetal( self );
      }

      public System.Double GetExtractorRadius( )

      {
         return ABICInterface.IAICallback_GetExtractorRadius( self );
      }

      public System.Double GetMinWind( )

      {
         return ABICInterface.IAICallback_GetMinWind( self );
      }

      public System.Double GetMaxWind( )

      {
         return ABICInterface.IAICallback_GetMaxWind( self );
      }

      public System.Double GetTidalStrength( )

      {
         return ABICInterface.IAICallback_GetTidalStrength( self );
      }

      public System.Double GetGravity( )

      {
         return ABICInterface.IAICallback_GetGravity( self );
      }

      public System.Double GetElevation(System.Double x, System.Double z )

      {
         return ABICInterface.IAICallback_GetElevation( self, x, z );
      }

      public System.Double GetMetal( )

      {
         return ABICInterface.IAICallback_GetMetal( self );
      }

      public System.Double GetMetalIncome( )

      {
         return ABICInterface.IAICallback_GetMetalIncome( self );
      }

      public System.Double GetMetalUsage( )

      {
         return ABICInterface.IAICallback_GetMetalUsage( self );
      }

      public System.Double GetMetalStorage( )

      {
         return ABICInterface.IAICallback_GetMetalStorage( self );
      }

      public System.Double GetEnergy( )

      {
         return ABICInterface.IAICallback_GetEnergy( self );
      }

      public System.Double GetEnergyIncome( )

      {
         return ABICInterface.IAICallback_GetEnergyIncome( self );
      }

      public System.Double GetEnergyUsage( )

      {
         return ABICInterface.IAICallback_GetEnergyUsage( self );
      }

      public System.Double GetEnergyStorage( )

      {
         return ABICInterface.IAICallback_GetEnergyStorage( self );
      }

      public System.Double GetFeatureHealth(System.Int32 feature )

      {
         return ABICInterface.IAICallback_GetFeatureHealth( self, feature );
      }

      public System.Double GetFeatureReclaimLeft(System.Int32 feature )

      {
         return ABICInterface.IAICallback_GetFeatureReclaimLeft( self, feature );
      }

      public System.Int32 GetNumUnitDefs( )

      {
         return ABICInterface.IAICallback_GetNumUnitDefs( self );
      }

      public System.Double GetUnitDefRadius(System.Int32 def )

      {
         return ABICInterface.IAICallback_GetUnitDefRadius( self, def );
      }

      public System.Double GetUnitDefHeight(System.Int32 def )

      {
         return ABICInterface.IAICallback_GetUnitDefHeight( self, def );
      }

#line 0 "AbicIAICallbackWrapper_manualtweaks.txt"


      //public void SendTextMsg( string message, int priority )
      //{
      //     ABICInterface.IAICallback_SendTextMsg( self, message, priority );
      //}


public UnitDef GetUnitDefByTypeId( int unitdeftypeid )
{
   IntPtr unitdefptr = ABICInterface.IAICallback_GetUnitDefByTypeId( self, unitdeftypeid );
   if( unitdefptr == IntPtr.Zero )
   {
      return null;
   }
   return new UnitDef( unitdefptr );
}

public UnitDef GetUnitDef( int deployedid )
{
   IntPtr unitdefptr = ABICInterface.IAICallback_GetUnitDef( self, deployedid );
   if( unitdefptr == IntPtr.Zero )
   {
      return null;
   }
   return new UnitDef( unitdefptr );
}

public Float3 GetUnitPos( int unitid )
{
   float x = 0; float y = 0; float z = 0;
   ABICInterface.IAICallback_GetUnitPos( self, ref x, ref y, ref z, unitid );
   return new Float3( x, y, z );
}

public Float3 ClosestBuildSite( UnitDef thisunitdef, Float3 pos, double searchradius, int mindistance )
{
   float x = 0; float y = 0; float z = 0;
   ABICInterface.IAICallback_ClosestBuildSite( self, ref x, ref y, ref z, thisunitdef.self, (float)pos.x, (float)pos.y, (float)pos.z, (float)searchradius, mindistance, 0 );
   return new Float3( x, y, z );    
}

public void DrawUnit( string name, Float3 pos, double rotation, int lifetime, int team,bool transparent, bool drawborder )
{
   ABICInterface.IAICallback_DrawUnit( self, name, (float)pos.x, (float)pos.y, (float)pos.z, (float)rotation, lifetime, team, transparent, drawborder, 0 );
}

public void GiveOrder( int unit, Command c )
{
   int numparams = c.parameters.GetUpperBound(0) + 1;
   if( numparams == 0 ){ ABICInterface.IAICallback_GiveOrder( self, unit, c.id, 0,0,0,0,0 ); }
   if( numparams == 1 ){ ABICInterface.IAICallback_GiveOrder( self, unit, c.id, 1,(float)c.parameters[0],0,0,0 ); }
   if( numparams == 2 ){ ABICInterface.IAICallback_GiveOrder( self, unit, c.id, 2,(float)c.parameters[0],(float)c.parameters[1],0,0 ); }
   if( numparams == 3 ){ ABICInterface.IAICallback_GiveOrder( self, unit, c.id, 3,(float)c.parameters[0],(float)c.parameters[1],(float)c.parameters[2],0 ); }
   if( numparams == 4 ){ ABICInterface.IAICallback_GiveOrder( self, unit, c.id, 4,(float)c.parameters[0],(float)c.parameters[1],(float)c.parameters[2],(float)c.parameters[3] ); }
}

    }
}
