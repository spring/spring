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

using System;
using System.IO;
using System.Collections;
using System.Threading;
using System.Globalization;

namespace CSharpAI
{
    // main AI class, that drives everything else
    public class CSAI : IGlobalAI
    {
        // make these available for public access to other classes, though we can get them directly through GetInstance too
        public IAICallback aicallback;
        public LogFile logfile;
        public Metal metal;
        public CSAIUserInteraction csaiuserinteraction;
            
        public const string AIVersion = "0.0010";
        public string CacheDirectoryPath = Path.Combine( Path.Combine( "AI", "CSAI" ), "cache" );
        
        public delegate void UnitCreatedHandler( int deployedunitid, IUnitDef unitdef );
        public delegate void UnitFinishedHandler( int deployedunitid, IUnitDef unitdef );
        public delegate void UnitDamagedHandler(int damaged,int attacker,float damage, Float3 dir);
        public delegate void UnitDestroyedHandler( int deployedunitid, int enemyid );
        public delegate void UnitIdleHandler( int deployedunitid );
        
        public delegate void UnitMoveFailedHandler(int unit);
            
        public delegate void EnemyEnterLOSHandler(int enemy);
        public delegate void EnemyEnterRadarHandler(int enemy);
        public delegate void EnemyLeaveLOSHandler(int enemy);
        public delegate void EnemyLeaveRadarHandler(int enemy);
        public delegate void EnemyDestroyedHandler( int enemy, int attacker );
        
        public delegate void TickHandler();

        public event UnitCreatedHandler UnitCreatedEvent;
        public event UnitFinishedHandler UnitFinishedEvent;
        public event UnitDamagedHandler UnitDamagedEvent;
        public event UnitDestroyedHandler UnitDestroyedEvent;
        public event UnitIdleHandler UnitIdleEvent;

        public event UnitMoveFailedHandler UnitMoveFailedEvent;
            
        public event EnemyEnterLOSHandler EnemyEnterLOSEvent;
        public event EnemyEnterRadarHandler EnemyEnterRadarEvent;
        public event EnemyLeaveLOSHandler EnemyLeaveLOSEvent;
        public event EnemyLeaveRadarHandler EnemyLeaveRadarEvent;
        public event EnemyDestroyedHandler EnemyDestroyedEvent;

        public event TickHandler TickEvent;
    
        static CSAI instance = new CSAI();
        public static CSAI GetInstance(){ return instance; } // Singleton pattern
        
        public int Team;
            
        public bool DebugOn = false;
        
        int reference;
        
        CSAI()
        {
            reference = new Random().Next(); // this is just to confirm that GetInstance retrieves a different instance for each AI
        }
        
        //int numOfUnits = 0;
        //IUnitDef[] unitList;
        //IUnitDef solarcollectordef;
    
        public void InitAI( IAICallback aicallback, int team)
        {
            Thread.CurrentThread.CurrentCulture = new CultureInfo("en-GB");
            
            this.aicallback = aicallback;
            try{            
                this.Team = team;
                logfile = LogFile.GetInstance().Init( team );
                logfile.WriteLine( "C# AI started v" + AIVersion+ ", team " + team + " ref " + reference + " map " + aicallback.GetMapName() + " mod " + aicallback.GetModName() );

                csaiuserinteraction = CSAIUserInteraction.GetInstance();
                
                if( File.Exists( "AI/CSAI/debug.flg" ) ) // if this file exists, activate debug mode; saves manually changing this for releases
                {
                    logfile.WriteLine( "Toggling debug on" );
                    DebugOn = true;
                }
                
                if( DebugOn )
                {
                    new Testing.RunTests().Go();
                }
                
                InitCache();
                
                PlayStyleManager.GetInstance();
                LoadPlayStyles.Go();
                
                metal = Metal.GetInstance();
                CommanderController.GetInstance();
                BuildTable.GetInstance();
                UnitController.GetInstance();
                EnemyController.GetInstance();
                FriendlyUnitPositionObserver.GetInstance();
                
                EnergyController.GetInstance();

                MovementMaps.GetInstance();
                BuildMap.GetInstance();
                LosMap.GetInstance();
                                
                //FactoryController.GetInstance();
                //RadarController.GetInstance();
                //TankController.GetInstance();
                //ScoutController.GetInstance();
                //ConstructorController.GetInstance();
                
                metal.Init();
                BuildPlanner.GetInstance();
                
                UnitController.GetInstance().LoadExistingUnits();  // need this if we're being reloaded in middle of a game, to get already existing units
                EnemyController.GetInstance().LoadExistingUnits();
                
                StrategyController.GetInstance();
                LoadStrategies.Go();
                
                PlayStyleManager.GetInstance().ChoosePlayStyle( "tankrush" );
                
                if( aicallback.GetModName().ToLower().IndexOf( "aass" ) == 0 || aicallback.GetModName().ToLower().IndexOf( "xtape" ) == 0 )
                {
                    SendTextMsg( "C# AI initialized v" + AIVersion + ", team " + team );
                    SendTextMsg( "Please say '.csai help' for available commands" );
                    
                    PlayStyleManager.GetInstance().ListPlayStyles();
                    
                    //CommanderController.GetInstance().CommanderBuildPower();
                }
                else
                {
                    SendTextMsg( "Warning: CSAI needs AA2.23  or XTA7 to run correctly at this time" );
                    logfile.WriteLine( "*********************************************************" );
                    logfile.WriteLine( "*********************************************************" );
                    logfile.WriteLine( "****                                                                                           ****" );
                    logfile.WriteLine( "**** Warning: CSAI needs AA2.23 or XTA7 to run correctly at this time ****" );
                    logfile.WriteLine( "****                                                                                           ****" );
                    logfile.WriteLine( "*********************************************************" );
                    logfile.WriteLine( "*********************************************************" );
                }
           }
           catch( Exception e )
           {
               logfile.WriteLine( "Exception: " + e.ToString() );
               SendTextMsg( "Exception: " + e.ToString() );
           }
        }
        
        public void Shutdown()
        {
            logfile.Shutdown();
        }
        
       // int commander = 0;
        
        // creates cache directory, if doesnt exist
        void InitCache()
        {
            if( !Directory.Exists( CacheDirectoryPath ) )
            {
                Directory.CreateDirectory( CacheDirectoryPath );
            }
        }
        
        public void UnitCreated(int deployedunitid)									//called when a new unit is created on ai team
        {
            try
            {
                IUnitDef unitdef = aicallback.GetUnitDef( deployedunitid );                
                if( UnitCreatedEvent != null )
                {
                    UnitCreatedEvent( deployedunitid, unitdef );
                }
           }
           catch( Exception e )
           {
               logfile.WriteLine( "Exception: " + e.ToString() );
               SendTextMsg("Exception: " + e.ToString());
           }
        }
        
        public void UnitFinished(int deployedunitid)								//called when an unit has finished building
        {
            try{
                IUnitDef unitdef = aicallback.GetUnitDef( deployedunitid );
                if( UnitFinishedEvent != null )
                {
                    UnitFinishedEvent( deployedunitid, unitdef );
                }
           }
           catch( Exception e )
           {
               logfile.WriteLine( "Exception: " + e.ToString() );
               SendTextMsg("Exception: " + e.ToString());
           }
        }
        
        public void UnitDestroyed(int deployedunitid,int attacker)								//called when a unit is destroyed
        {
            try
            {
                if( UnitDestroyedEvent != null )
                {
                    UnitDestroyedEvent( deployedunitid, attacker );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }        
        
        public void UnitIdle(int deployedunitid )										//called when a unit go idle and is not assigned to any group
        {
            try
            {
                if( UnitIdleEvent != null )
                {
                    UnitIdleEvent( deployedunitid );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }
        
        public void UnitDamaged(int damaged,int attacker,float damage, Float3 dir)				//called when one of your units are damaged
        {
            try
            {
                if( UnitDamagedEvent != null )
                {
                    UnitDamagedEvent( damaged, attacker, damage, dir );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }       
        
        public void UnitMoveFailed(int unit)
        {
            try
            {
                if( UnitMoveFailedEvent != null )
                {
                    UnitMoveFailedEvent( unit );
                }
                if( DebugOn )
                {
                    aicallback.DrawUnit( "ARMRAD", aicallback.GetUnitPos( unit ), 0.0f, 500, aicallback.GetMyAllyTeam(), true, true);
                }
                // note to self: add add map ponits to interface
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }       
    
        public void EnemyEnterLOS(int enemy)
        {
            try
            {
                if( EnemyEnterLOSEvent != null )
                {
                    EnemyEnterLOSEvent( enemy );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }
        
        public void EnemyLeaveLOS(int enemy)
        {
            try
            {
                if( EnemyLeaveLOSEvent != null )
                {
                    EnemyLeaveLOSEvent( enemy );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }
    
        public void EnemyEnterRadar(int enemy)		
        {
            try
            {
                if( EnemyEnterRadarEvent != null )
                {
                    aicallback.SendTextMsg("enemy entered radar: " + enemy, 0 );
                    logfile.WriteLine("enemy entered radar: " + enemy);
                    EnemyEnterRadarEvent( enemy );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }
        
        public void EnemyLeaveRadar(int enemy)	
        {
            try
            {
                if( EnemyLeaveRadarEvent != null )
                {
                    EnemyLeaveRadarEvent( enemy );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }
            
        public void EnemyDamaged(int damaged,int attacker,float damage, Float3 dir) //called when an enemy inside los or radar is damaged
        {
        }
        
        public void EnemyDestroyed(int enemy,int attacker)						//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
        {
            try
            {
                if( EnemyDestroyedEvent != null )
                {
                    EnemyDestroyedEvent( enemy, attacker );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }
            
        public void GotChatMsg( string msg,int player)					//called when someone writes a chat msg
        {
            try
            {
                csaiuserinteraction.controlpanel.textboxlogarea.Text += 
                    player.ToString() + ": " + msg;
                if( msg.ToLower().IndexOf( ".csai" ) == 0 )
                {
                    if( msg.ToLower().Substring( 5, 1 ) == "*" || msg.ToLower().Substring( 5, 1 ) == " " || msg.ToLower().Substring( 5, 1 ) == Team.ToString() )
                    {
                        string[] splitchatline = msg.Split( " ".ToCharArray() );
                        string command = splitchatline[1].ToLower();
                        if( VoiceCommands.Contains( command ) )
                        {
                            ( VoiceCommands[ command ] as VoiceCommandHandler )( msg, splitchatline, player );
                        }
                        else
                        {
                            string helpstring = "CSAI commands available: help, ";
                            foreach( DictionaryEntry entry in VoiceCommands )
                            {
                                helpstring += entry.Key + ", ";
                            }
                            aicallback.SendTextMsg( helpstring, 0 );
                            aicallback.SendTextMsg( "Example: .csai" + Team + " showplaystyles", 0 );
                        }
                    }
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }       
    
        public int HandleEvent (int msg, object data)
        {
            return 0;
        }       
        
        //called every frame
        public void Update()
        {
            try
            {
                if( TickEvent != null )
                {
                    TickEvent();
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
        }    
        
        
        public delegate void VoiceCommandHandler( string chatstring, string[] splitchatstring, int player );
        
        Hashtable VoiceCommands = new Hashtable();
        public void RegisterVoiceCommand( string commandstring, VoiceCommandHandler handler )
        {
            commandstring = commandstring.ToLower();
            if( !VoiceCommands.Contains( commandstring ) && commandstring != "help" )
            {
                logfile.WriteLine("CSAI registering voicecommand " + commandstring );
                VoiceCommands.Add( commandstring, handler );
            }
        }
        public void UnregisterVoiceCommand( string commandstring )
        {
            commandstring = commandstring.ToLower();
            if( VoiceCommands.Contains( commandstring ) )
            {
                logfile.WriteLine("CSAI unregistering voicecommand " + commandstring );
                VoiceCommands.Remove( commandstring );
            }
        }

        public void SendTextMsg(string text )
        {
            aicallback.SendTextMsg(text, 0);
            csaiuserinteraction.controlpanel.textboxlogarea.Text += text + "\n\r";
        }
    }
}
