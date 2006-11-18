// *** This is a generated file; if you want to change it, please change CSAIInterfaces.dll, which is the reference
// 
// This file was generated by MonoABICInterfaceGenerator, by Hugh Perkins hughperkins@gmail.com http://manageddreams.com
// 
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;

namespace CSharpAI
{
    public class ABICInterface
    {
//  This is appended to teh generated C# interface to create ABICInterface_generated.cs
// Its inserted inside the class definition

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static IntPtr IAICallback_GetUnitDefByTypeId( IntPtr aicallback, int unittypeid );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static IntPtr UnitDef_get_movedata( IntPtr unitdef );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static IntPtr IAICallback_GetUnitDef( IntPtr aicallback, int deployedid );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void IAICallback_GetUnitPos( IntPtr aicallback, ref float commanderposx, ref float commanderposy, ref float commanderposz, int unit );
        
		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void IAICallback_ClosestBuildSite( IntPtr aicallback, ref float nx, ref float ny, ref float nz, IntPtr unitdef, float x, float y, float z, float searchRadius, int minDistance, int facing );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void IAICallback_GiveOrder( IntPtr aicallback, int id, int commandid, int numparams, float p1, float p2, float p3, float p4 );
        
		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void IAICallback_DrawUnit( IntPtr aicallback, string name, float x, float y, float z,float rotation,
                            int lifetime, int team,bool transparent,bool drawBorder,int facing);
      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Void IAICallback_SendTextMsg( IntPtr self, System.String text, System.Int32 priority );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_CreateGroup( IntPtr self, System.String dll, System.Int32 aiNumber );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Void IAICallback_EraseGroup( IntPtr self, System.Int32 groupid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String IAICallback_GetModName( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String IAICallback_GetMapName( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Void IAICallback_SetFigureColor( IntPtr self, System.Int32 group, System.Double red, System.Double green, System.Double blue, System.Double alpha );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Void IAICallback_DeleteFigureGroup( IntPtr self, System.Int32 group );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean IAICallback_IsGamePaused( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetCurrentFrame( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetMyTeam( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetMyAllyTeam( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetPlayerTeam( IntPtr self, System.Int32 player );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean IAICallback_AddUnitToGroup( IntPtr self, System.Int32 unitid, System.Int32 groupid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean IAICallback_RemoveUnitFromGroup( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetUnitGroup( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetUnitAiHint( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetUnitTeam( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetUnitAllyTeam( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetUnitHealth( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetUnitMaxHealth( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetUnitSpeed( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetUnitPower( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetUnitExperience( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetUnitMaxRange( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean IAICallback_IsUnitActivated( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean IAICallback_UnitBeingBuilt( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetBuildingFacing( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean IAICallback_IsUnitCloaked( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean IAICallback_IsUnitParalyzed( IntPtr self, System.Int32 unitid );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetMapWidth( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetMapHeight( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetMaxMetal( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetExtractorRadius( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetMinWind( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetMaxWind( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetTidalStrength( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetGravity( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetElevation( IntPtr self, System.Double x, System.Double z );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetMetal( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetMetalIncome( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetMetalUsage( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetMetalStorage( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetEnergy( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetEnergyIncome( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetEnergyUsage( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetEnergyStorage( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetFeatureHealth( IntPtr self, System.Int32 feature );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetFeatureReclaimLeft( IntPtr self, System.Int32 feature );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 IAICallback_GetNumUnitDefs( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetUnitDefRadius( IntPtr self, System.Int32 def );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double IAICallback_GetUnitDefHeight( IntPtr self, System.Int32 def );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_GetNumBuildOptions( IntPtr self );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_GetBuildOption( IntPtr self, System.Int32 index );

      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_name( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_humanName( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_filename( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_loaded( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_id( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_buildpicname( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_aihint( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_techLevel( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_metalUpkeep( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_energyUpkeep( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_metalMake( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_makesMetal( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_energyMake( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_metalCost( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_energyCost( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_buildTime( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_extractsMetal( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_extractRange( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_windGenerator( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_tidalGenerator( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_metalStorage( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_energyStorage( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_autoHeal( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_idleAutoHeal( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_idleTime( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_power( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_health( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_speed( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_turnRate( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_moveType( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_upright( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_controlRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_losRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_airLosRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_losHeight( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_radarRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_sonarRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_jammerRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_sonarJamRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_seismicRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_seismicSignature( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_stealth( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_buildSpeed( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_buildDistance( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_mass( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxSlope( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxHeightDif( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_minWaterDepth( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_waterline( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxWaterDepth( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_armoredMultiple( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_armorType( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_type( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_tooltip( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_wreckName( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_deathExplosion( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_selfDExplosion( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_TEDClassString( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_categoryString( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String UnitDef_get_iconType( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_selfDCountdown( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canfly( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canmove( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canhover( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_floater( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_builder( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_activateWhenBuilt( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_onoffable( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_reclaimable( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canRestore( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canRepair( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canReclaim( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_noAutoFire( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canAttack( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canPatrol( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canFight( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canGuard( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canBuild( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canAssist( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canRepeat( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_wingDrag( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_wingAngle( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_drag( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_frontToSpeed( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_speedToFront( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_myGravity( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxBank( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxPitch( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_turnRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_wantedHeight( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_hoverAttack( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_dlHoverFactor( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxAcc( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxDec( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxAileron( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxElevator( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxRudder( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_xsize( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_ysize( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_buildangle( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_loadingRadius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_transportCapacity( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_transportSize( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_isAirBase( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_transportMass( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canCloak( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_startCloaked( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_cloakCost( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_cloakCostMoving( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_decloakDistance( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canKamikaze( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_kamikazeDist( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_targfac( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canDGun( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_needGeo( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_isFeature( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_hideDamage( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_isCommander( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_showPlayerName( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canResurrect( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canCapture( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_highTrajectoryType( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_leaveTracks( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_trackWidth( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_trackOffset( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_trackStrength( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_trackStretch( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_trackType( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canDropFlare( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_flareReloadTime( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_flareEfficieny( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_flareDelay( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_flareTime( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_flareSalvoSize( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_flareSalvoDelay( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_smoothAnim( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_isMetalMaker( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_canLoopbackAttack( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_levelGround( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_useBuildingGroundDecal( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_buildingDecalType( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_buildingDecalSizeX( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 UnitDef_get_buildingDecalSizeY( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_buildingDecalDecaySpeed( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_isfireplatform( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_showNanoFrame( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean UnitDef_get_showNanoSpray( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_maxFuel( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_refuelTime( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double UnitDef_get_minAirBasePower( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 MoveData_get_size( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double MoveData_get_depth( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double MoveData_get_maxSlope( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double MoveData_get_slopeMod( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double MoveData_get_depthMod( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 MoveData_get_pathType( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double MoveData_get_crushStrength( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 MoveData_get_moveFamily( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String FeatureDef_get_myName( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String FeatureDef_get_description( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double FeatureDef_get_metal( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double FeatureDef_get_energy( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double FeatureDef_get_maxHealth( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double FeatureDef_get_radius( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Double FeatureDef_get_mass( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean FeatureDef_get_upright( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 FeatureDef_get_drawType( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean FeatureDef_get_destructable( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean FeatureDef_get_blocking( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean FeatureDef_get_burnable( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean FeatureDef_get_floating( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Boolean FeatureDef_get_geoThermal( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.String FeatureDef_get_deathFeature( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 FeatureDef_get_xsize( IntPtr self );


      [MethodImpl(MethodImplOptions.InternalCall)]
      public extern static System.Int32 FeatureDef_get_ysize( IntPtr self );


    }
}
