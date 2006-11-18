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
    public class CSAI
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
            
            UnitDef unitdef = aicallback.GetUnitDefByTypeId( 34 );
            aicallback.SendTextMsg( "gotunitdef", 0 );
            aicallback.SendTextMsg( "type id 34 is " + unitdef.name, 0 );
            aicallback.SendTextMsg( "human name: " + unitdef.humanName, 0 );
            aicallback.SendTextMsg( "id: " + unitdef.id, 0 );
            
            MoveData movedata = unitdef.movedata;
            //IAICallback_SendTextMsg( aicallback, "movedata is null? " + ( movedata == 0 );
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
            
            //IAICallback_SendTextMsg( aicallback, "movetype: %i" + MoveData_get_movetype( movedata ) );        
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
            
            aicallback.SendTextMsg( "maxslope: " + movedata.maxSlope, 0 );
        }
        
        public void UnitCreated( int unit )
        {
            aicallback.SendTextMsg(  "Unit created: " + unit, 0 );
        
            UnitDef unitdef = aicallback.GetUnitDef( unit );
            aicallback.SendTextMsg(  "Unit created: " + unitdef.name, 0 );
        
            MoveData movedata = unitdef.movedata;
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
                    UnitDef thisunitdef = aicallback.GetUnitDefByTypeId( i );
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
    }
    
    public class CSAICInterface
    {
        IntPtr aicallback;
        CSAI ai;
        
		public delegate void _InitAI( IntPtr aicallback, int team );
        IntPtr InitAIInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected
        
		public delegate void _UnitCreated( int unit );
        IntPtr UnitCreatedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetInitAICallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUnitCreatedCallback( IntPtr initai );

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
                
                /*
                
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Hello world from C# Mono 0.2!", 0 );
                
                ABICInterface.IAICallback_SendTextMsg( aicallback, "The map name is: " + ABICInterface.IAICallback_GetMapName( aicallback ), 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Our ally team is: " + ABICInterface.IAICallback_GetMyTeam( aicallback ), 0 );
                
                //int features[10000 + 1];
                //int numfeatures = IAICallback_GetFeatures( aicallback, features, 10000 );
                //sprintf( buffer, "Num features is: %i", numfeatures );
                //IAICallback_SendTextMsg( aicallback, buffer, 0 );
                
                //const FeatureDef *featuredef = IAICallback_GetFeatureDef( aicallback, features[0] );
                //sprintf( buffer, "First feature: %s", FeatureDef_get_myName( featuredef ) );
                //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
                
                IntPtr unitdef = ABICInterface.IAICallback_GetUnitDefByTypeId( aicallback, 34 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "gotunitdef", 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "type id 34 is " + ABICInterface.UnitDef_get_name( unitdef ), 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "name: " + ABICInterface.UnitDef_get_name( unitdef ), 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "human name: " + ABICInterface.UnitDef_get_humanName( unitdef ), 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "id: " + ABICInterface.UnitDef_get_id( unitdef ), 0 );
                
                IntPtr movedata = ABICInterface.UnitDef_get_movedata( unitdef );
                //IAICallback_SendTextMsg( aicallback, "movedata is null? " + ( movedata == 0 );
                //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
                
                //IAICallback_SendTextMsg( aicallback, "movetype: %i" + MoveData_get_movetype( movedata ) );        
                //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
                
                ABICInterface.IAICallback_SendTextMsg( aicallback, "maxslope: " + ABICInterface.MoveData_get_maxSlope( movedata ), 0 );
                
                sw.WriteLine( "CSAICInterface.InitAI <<<" );
                sw.Flush();
                */
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
            /*
            ABICInterface.IAICallback_SendTextMsg( aicallback,  "Unit created: " + unit, 0 );
        
            IntPtr unitdef = ABICInterface.IAICallback_GetUnitDef( aicallback, unit );
            ABICInterface.IAICallback_SendTextMsg( aicallback, "Unit created: " + ABICInterface.UnitDef_get_name( unitdef ), 0 );
        
            IntPtr movedata = ABICInterface.UnitDef_get_movedata( unitdef );
            if( movedata != IntPtr.Zero )
            {
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Max Slope: " + ABICInterface.MoveData_get_maxSlope( movedata ), 0 );
            }
        
            if( ABICInterface.UnitDef_get_isCommander( unitdef ) )
            {
                int numbuildoptions = ABICInterface.UnitDef_GetNumBuildOptions( unitdef );
                string buildoptionsstring = "Build options: ";
                for( int i = 0; i < numbuildoptions; i++ )
                {
                    buildoptionsstring += ABICInterface.UnitDef_GetBuildOption( unitdef, i );            
                }
                ABICInterface.IAICallback_SendTextMsg( aicallback, buildoptionsstring, 0 );
        
                float commanderposx = 0; 
                float commanderposy = 0; 
                float commanderposz = 0; 
                ABICInterface.IAICallback_GetUnitPos( aicallback, ref commanderposx, ref commanderposy, ref commanderposz, unit );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Commanderpos: " + commanderposx + " " + commanderposy + " " + commanderposz, 0 );
                
                int numunitdefs = ABICInterface.IAICallback_GetNumUnitDefs( aicallback );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Num unit defs: " + numunitdefs, 0 );
                
                for( int i = 1; i <= numunitdefs; i++ )
                {
                    IntPtr thisunitdef = ABICInterface.IAICallback_GetUnitDefByTypeId( aicallback, i );
                    if( ABICInterface.UnitDef_get_name( thisunitdef ) == "ARMSOLAR" )
                    {
                        ABICInterface.IAICallback_SendTextMsg( aicallback, "Found solar collector def: " + ABICInterface.UnitDef_get_id( thisunitdef ), 0 );

                        float nbpx = 0, nbpy = 0, nbpz = 0; // ok, lazy, but this will be wrapped later anyway
                        ABICInterface.IAICallback_ClosestBuildSite( aicallback, ref nbpx, ref nbpy, ref nbpz, thisunitdef, commanderposx, commanderposy, commanderposz, 1400,2,0 );
                        ABICInterface.IAICallback_SendTextMsg( aicallback, "Closest build site: " + nbpx + " " + nbpy + " " + nbpz, 0 );
                        
                        ABICInterface.IAICallback_DrawUnit( aicallback, "ARMSOLAR", nbpx, nbpy, nbpz,0,
                            200, ABICInterface.IAICallback_GetMyAllyTeam( aicallback ),true,true,0);
        
                        ABICInterface.IAICallback_GiveOrder( aicallback, unit, -  ABICInterface.UnitDef_get_id( thisunitdef ), 3, nbpx, nbpy, nbpz, 0 );
                    }
                }
            }
            */
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
        
        public void EnemyDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
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
        
        public void UnitDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
        {
        }
        
        public void UnitMoveFailed( int unit)
        {
        }
        
        //int HandleEvent (int msg,const void *data); // todo
        
        //called every frame
        void Update()
        {
        }
    }
}
