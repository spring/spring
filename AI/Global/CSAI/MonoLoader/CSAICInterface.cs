// Copyright Jelmer Cnossen, Hugh Perkins 2006
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
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;

namespace CSharpAI
{
    // note: no longer used; links directly to CSAI.dll
    public class _CSAI
    {
        AICallback aicallback;
        int team;
        
        public void InitAI( AICallback aicallback, int team )
        {
            this.aicallback = aicallback;
            this.team = team;
            aicallback.SendTextMsg( "Hello from Mono AbicWrappers", 0 );
            aicallback.SendTextMsg( "The map name is: " + aicallback.GetMapName(), 0 );
            aicallback.SendTextMsg( "Our ally team is: " + aicallback.GetMyTeam(), 0 );
            
            //int features[10000 + 1];
            //int numfeatures = IAICallback_GetFeatures( aicallback, features, 10000 );
            //sprintf( buffer, "Num features is: %i", numfeatures );
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );
            
            //const FeatureDef *featuredef = IAICallback_GetFeatureDef( aicallback, features[0] );
            //sprintf( buffer, "First feature: %s", FeatureDef_get_myName( featuredef ) );
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
            
            IUnitDef unitdef = aicallback.GetUnitDefByTypeId( 34 );
            aicallback.SendTextMsg( "gotunitdef", 0 );
            aicallback.SendTextMsg( "type id 34 is " + unitdef.name, 0 );
            aicallback.SendTextMsg( "human name: " + unitdef.humanName, 0 );
            aicallback.SendTextMsg( "id: " + unitdef.id, 0 );
            
            IMoveData movedata = unitdef.movedata;
            //IAICallback_SendTextMsg( aicallback, "movedata is null? " + ( movedata == 0 );
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
            
            //IAICallback_SendTextMsg( aicallback, "movetype: %i" + MoveData_get_movetype( movedata ) );        
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
            
            aicallback.SendTextMsg( "maxslope: " + movedata.maxSlope, 0 );
        }
        
        public void UnitCreated( int unit )
        {
            aicallback.SendTextMsg(  "Unit created: " + unit, 0 );
        
            IUnitDef unitdef = aicallback.GetUnitDef( unit );
            aicallback.SendTextMsg(  "Unit created: " + unitdef.name, 0 );
        
            IMoveData movedata = unitdef.movedata;
            if( movedata != null )
            {
                aicallback.SendTextMsg(  "Max Slope: " + movedata.maxSlope, 0 );
            }
        
            if( unitdef.isCommander )
            {
                int numbuildoptions = unitdef.GetNumBuildOptions();
                string buildoptionsstring = "Build options: ";
                for( int i = 0; i < numbuildoptions; i++ )
                {
                    buildoptionsstring += unitdef.GetBuildOption( i );            
                }
                aicallback.SendTextMsg( buildoptionsstring, 0 );
        
                Float3 commanderpos = aicallback.GetUnitPos( unit );
                aicallback.SendTextMsg( "Commanderpos: " + commanderpos.ToString(), 0 );
                
                int numunitdefs = aicallback.GetNumUnitDefs();
                aicallback.SendTextMsg( "Num unit defs: " + numunitdefs, 0 );
                
                for( int i = 1; i <= numunitdefs; i++ )
                {
                    IUnitDef thisunitdef = aicallback.GetUnitDefByTypeId( i );
                    if( thisunitdef.name == "ARMSOLAR" )
                    {
                        aicallback.SendTextMsg( "Found solar collector def: " + thisunitdef.id, 0 );

                        Float3 nearestbuildpos = aicallback.ClosestBuildSite( thisunitdef, commanderpos, 1400,2  );
                        aicallback.SendTextMsg( "Closest build site: " + nearestbuildpos.ToString(), 0 );
                        
                        aicallback.DrawUnit( "ARMSOLAR", nearestbuildpos,0,
                            200, aicallback.GetMyAllyTeam(),true,true );
        
                        aicallback.GiveOrder( unit, new Command( -  thisunitdef.id, nearestbuildpos.ToDoubleArray() ) );
                    }
                }
            }
        }
        public void UnitFinished(int unit)							//called when an unit has finished building
        {
        }
        
        public void UnitDestroyed( int unit,int attacker)								//called when a unit is destroyed
        {
        }
        
        public void EnemyEnterLOS( int enemy)
        {
        }
        
        public void EnemyLeaveLOS(int enemy)
        {
        }
        
        public void EnemyEnterRadar( int enemy)		
        {
        }
        
        public void EnemyLeaveRadar( int enemy)
        {
        }
        
        public void EnemyDamaged( int damaged,int attacker,float damage, Float3 dir)	//called when an enemy inside los or radar is damaged
        {
        }
        
        public void EnemyDestroyed( int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
        {
        }
        
        public void UnitIdle( int unit)										//called when a unit go idle and is not assigned to any group
        {
        }
        
        public void GotChatMsg( string msg,int player)					//called when someone writes a chat msg
        {
        }
        
        public void UnitDamaged( int damaged,int attacker,float damage, Float3 dir)					//called when one of your units are damaged
        {
        }
        
        public void UnitMoveFailed( int unit)
        {
        }
        
        //int HandleEvent (int msg,const void *data); // todo
        
        //called every frame
        public void Update()
        {
        }
    }
    
    public class CSAICInterface
    {
        IntPtr aicallback;
        CSAI ai;
        
		public delegate void _InitAI( IntPtr aicallback, int team );
        IntPtr InitAIInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected
        
		public delegate void _UnitCreated( int unit );
        IntPtr UnitCreatedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _UnitFinished( int unit );
        IntPtr UnitFinishedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _UnitDestroyed( int unit,int attacker );
        IntPtr UnitDestroyedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _EnemyEnterLOS( int unit );
        IntPtr EnemyEnterLOSInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _EnemyLeaveLOS( int unit );
        IntPtr EnemyLeaveLOSInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _EnemyEnterRadar( int unit );
        IntPtr EnemyEnterRadarInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _EnemyLeaveRadar( int unit );
        IntPtr EnemyLeaveRadarInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _EnemyDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz);
        IntPtr EnemyDamagedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _EnemyDestroyed( int enemy,int attacker);
        IntPtr EnemyDestroyedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _UnitIdle( int unit );
        IntPtr UnitIdleInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected
        
		public delegate void _GotChatMsg( string msg, int player );
        IntPtr GotChatMsgInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _UnitDamaged(  int damaged,int attacker,float damage, float dirx, float diry, float dirz);
        IntPtr UnitDamagedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _UnitMoveFailed( int unit );
        IntPtr UnitMoveFailedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		public delegate void _Update( );
        IntPtr UpdateInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetInitAICallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUnitCreatedCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUnitFinishedCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUnitDestroyedCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetEnemyEnterLOSCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetEnemyLeaveLOSCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetEnemyEnterRadarCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetEnemyLeaveRadarCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetEnemyDamagedCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetEnemyDestroyedCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUnitIdleCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetGotChatMsgCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUnitDamagedCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUnitMoveFailedCallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUpdateCallback( IntPtr initai );

        // start point, called by invoke
        public void Bind()
        {
            StreamWriter sw = new StreamWriter( "outo.txt", false );
            try
            {
                sw.WriteLine( "CSAICInterface.Bind >>>" );
                sw.Flush();
                
                //InitAIInstance = new _InitAI( InitAI );
                InitAIInstancePointer = Marshal.GetFunctionPointerForDelegate( new _InitAI( InitAI ) );
                SetInitAICallback( InitAIInstancePointer );
                
                UnitCreatedInstancePointer = Marshal.GetFunctionPointerForDelegate( new _UnitCreated( UnitCreated ) );
                SetUnitCreatedCallback( UnitCreatedInstancePointer );
                
                UnitFinishedInstancePointer = Marshal.GetFunctionPointerForDelegate( new _UnitFinished( UnitFinished ) );
                SetUnitFinishedCallback( UnitFinishedInstancePointer );
                
                UnitDestroyedInstancePointer = Marshal.GetFunctionPointerForDelegate( new _UnitDestroyed( UnitDestroyed ) );
                SetUnitDestroyedCallback( UnitDestroyedInstancePointer );
                
                EnemyEnterLOSInstancePointer = Marshal.GetFunctionPointerForDelegate( new _EnemyEnterLOS( EnemyEnterLOS ) );
                SetEnemyEnterLOSCallback( EnemyEnterLOSInstancePointer );
                
                EnemyLeaveLOSInstancePointer = Marshal.GetFunctionPointerForDelegate( new _EnemyLeaveLOS( EnemyLeaveLOS ) );
                SetEnemyLeaveLOSCallback( EnemyLeaveLOSInstancePointer );
                
                EnemyEnterRadarInstancePointer = Marshal.GetFunctionPointerForDelegate( new _EnemyEnterRadar( EnemyEnterRadar ) );
                SetEnemyEnterRadarCallback( EnemyEnterRadarInstancePointer );
                
                EnemyLeaveRadarInstancePointer = Marshal.GetFunctionPointerForDelegate( new _EnemyLeaveRadar( EnemyLeaveRadar ) );
                SetEnemyLeaveRadarCallback( EnemyLeaveRadarInstancePointer );
                
                EnemyDamagedInstancePointer = Marshal.GetFunctionPointerForDelegate( new _EnemyDamaged( EnemyDamaged ) );
                SetEnemyDamagedCallback( EnemyDamagedInstancePointer );
                
                EnemyDestroyedInstancePointer = Marshal.GetFunctionPointerForDelegate( new _EnemyDestroyed( EnemyDestroyed ) );
                SetEnemyDestroyedCallback( EnemyDestroyedInstancePointer );
                
                UnitIdleInstancePointer = Marshal.GetFunctionPointerForDelegate( new _UnitIdle( UnitIdle ) );
                SetUnitIdleCallback( UnitIdleInstancePointer );
                
                GotChatMsgInstancePointer = Marshal.GetFunctionPointerForDelegate( new _GotChatMsg( GotChatMsg ) );
                SetGotChatMsgCallback( UnitCreatedInstancePointer );
                
                UnitDamagedInstancePointer = Marshal.GetFunctionPointerForDelegate( new _UnitDamaged( UnitDamaged ) );
                SetUnitDamagedCallback( UnitDamagedInstancePointer );
                
                UnitMoveFailedInstancePointer = Marshal.GetFunctionPointerForDelegate( new _UnitMoveFailed( UnitMoveFailed ) );
                SetUnitMoveFailedCallback( UnitMoveFailedInstancePointer );
                
                UpdateInstancePointer = Marshal.GetFunctionPointerForDelegate( new _Update( Update ) );
                SetUpdateCallback( UpdateInstancePointer );
                
                sw.WriteLine( "CSAICInterface.Bind <<<" );
                sw.Flush();
            }
            catch(Exception e )
            {
                sw.WriteLine( e.ToString() );
                sw.Flush();
            }
            sw.Close();
        }
        
		public void InitAI(IntPtr aicallback, int team )
		{
            StreamWriter sw = new StreamWriter( "outo.txt", true );
            try
            {
                sw.WriteLine( "CSAICInterface.InitAI >>>" );
                sw.Flush();
                
                ai = new CSAI();
                
                this.aicallback = aicallback;
                
                ai.InitAI( new AICallback( aicallback ), team );                
            }
            catch(Exception e )
            {
                sw.WriteLine( e.ToString() );
                sw.Flush();
            }
            sw.Close();
		}        
        
        public void UnitCreated( int unit)									//called when a new unit is created on ai team
        {
            ai.UnitCreated( unit );
        }
        
        public void UnitFinished(int unit)							//called when an unit has finished building
        {
            ai.UnitFinished( unit );
        }
        
        public void UnitDestroyed( int unit,int attacker)								//called when a unit is destroyed
        {
            ai.UnitDestroyed( unit, attacker );
        }
        
        public void EnemyEnterLOS( int enemy)
        {
            ai.EnemyEnterLOS( enemy );
        }
        
        public void EnemyLeaveLOS(int enemy)
        {
            ai.EnemyLeaveLOS( enemy );
        }
        
        public void EnemyEnterRadar( int enemy)		
        {
            ai.EnemyEnterRadar( enemy );
        }
        
        public void EnemyLeaveRadar( int enemy)
        {
            ai.EnemyLeaveRadar( enemy );
        }
        
        public void EnemyDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
        {
            ai.EnemyDamaged( damaged, attacker, damage, new Float3( dirx, diry, dirz ) );
        }
        
        public void EnemyDestroyed( int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
        {
            ai.EnemyDestroyed( enemy, attacker );
        }
        
        public void UnitIdle( int unit)										//called when a unit go idle and is not assigned to any group
        {
            ai.UnitIdle( unit );
        }
        
        public void GotChatMsg( string msg,int player)					//called when someone writes a chat msg
        {
            ai.GotChatMsg( msg, player );
        }
        
        public void UnitDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
        {
            ai.UnitDamaged( damaged, attacker, damage, new Float3( dirx, diry, dirz ) );
        }
        
        public void UnitMoveFailed( int unit)
        {
            ai.UnitMoveFailed( unit );
        }
        
        //int HandleEvent (int msg,const void *data); // todo
        
        //called every frame
        public void Update()
        {
            ai.Update(  );
        }
    }
}
