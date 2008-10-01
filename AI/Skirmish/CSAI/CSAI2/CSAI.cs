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
using System.Collections.Generic;
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
            
        public const string AIVersion = "0.0011";
        public string AIDirectoryPath = "AI/CSAI";
        public string CacheDirectoryPath
        {
            get
            {
                return Path.Combine(AIDirectoryPath, "cache");
            }
        }
        
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
        
        public void InitAI( IAICallback aicallback, int team)
        {

            this.aicallback = aicallback;
            Thread.CurrentThread.CurrentCulture = new CultureInfo("en-GB");
            
            try{
                Directory.CreateDirectory(AIDirectoryPath);
                this.Team = team;
                logfile = LogFile.GetInstance().Init( Path.Combine( AIDirectoryPath, "csharpai_team" + team + ".log" ) );
                logfile.WriteLine( "C# AI started v" + AIVersion+ ", team " + team + " ref " + reference + " map " + aicallback.GetMapName() + " mod " + aicallback.GetModName() );
                logfile.WriteLine("RL Date/time: " + DateTime.Now.ToString());

                if( File.Exists( AIDirectoryPath + "/debug.flg" ) ) // if this file exists, activate debug mode; saves manually changing this for releases
                {
                    logfile.WriteLine( "Toggling debug on" );
                    DebugOn = true;
                }
                
                if( DebugOn )
                {
                    //new Testing.RunTests().Go();
                }
                
                InitCache();

                /*
                IUnitDef unitdef = aicallback.GetUnitDefByTypeId(34);
                aicallback.SendTextMsg(unitdef.name,0);
                aicallback.SendTextMsg(unitdef.id.ToString(), 0);
                aicallback.SendTextMsg(unitdef.humanName, 0);
                aicallback.SendTextMsg(unitdef.movedata.moveType.ToString(), 0);
                aicallback.SendTextMsg(unitdef.movedata.maxSlope.ToString(), 0);
                aicallback.GetMetalMap();
                aicallback.GetLosMap();
                aicallback.GetRadarMap();
                aicallback.GetFriendlyUnits();
                aicallback.GetFeatures();
                aicallback.GetEnemyUnitsInRadarAndLos();

                Metal.GetInstance();
                */
                //aicallback.GetElevation(300,300);
                //new SlopeMap().GetSlopeMap();
                //double[,] _SlopeMap = new double[256, 256];
                //LosMap.GetInstance();
                //MovementMaps.GetInstance();
                //BuildMap.GetInstance().Init();

                //return;
                // -- test stuff here --
                logfile.WriteLine("Is game paused? : " + aicallback.IsGamePaused());
                // -- end test stuff --

                BuildEconomy buildeconomy = new BuildEconomy();
                
               // UnitController.GetInstance().LoadExistingUnits();  // need this if we're being reloaded in middle of a game, to get already existing units

                SendTextMsg("C# AI initialized v" + AIVersion + ", team " + team);
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

        public void DebugSay(string message)
        {
            if( DebugOn )
            {
                aicallback.SendTextMsg( message, 0 );
                logfile.WriteLine(message);
            }
        }
        
        // creates cache directory, if doesnt exist
        void InitCache()
        {
            if( !Directory.Exists( CacheDirectoryPath ) )
            {
                Directory.CreateDirectory( CacheDirectoryPath );
            }
        }
        

        double unitcreatedtime = 0;
        double unitfinishedtime = 0;
        double unitdestroyedtime = 0;
        double unitidletime = 0;
        double ticktime = 0;
        public void UnitCreated(int deployedunitid)									//called when a new unit is created on ai team
        {
            logfile.WriteLine("UnitCreated()");
            DateTime start = DateTime.Now;
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
           unitcreatedtime += DateTime.Now.Subtract(start).TotalMilliseconds;
       }
        
        public void UnitFinished(int deployedunitid)								//called when an unit has finished building
        {
            DateTime start = DateTime.Now;
            try
            {
                IUnitDef unitdef = aicallback.GetUnitDef(deployedunitid);
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
           unitfinishedtime += DateTime.Now.Subtract(start).TotalMilliseconds;
       }
        
        public void UnitDestroyed(int deployedunitid,int attacker)								//called when a unit is destroyed
        {
            DateTime start = DateTime.Now;
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
            unitdestroyedtime += DateTime.Now.Subtract(start).TotalMilliseconds;
        }        
        
        public void UnitIdle(int deployedunitid )										//called when a unit go idle and is not assigned to any group
        {
            System.DateTime start = DateTime.Now;
            try
            {
                if (UnitIdleEvent != null)
                {
                    UnitIdleEvent( deployedunitid );
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
            unitidletime += DateTime.Now.Subtract(start).TotalMilliseconds;
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
                if( msg.ToLower().IndexOf( ".csai" ) == 0 )
                {
                    if( msg.ToLower().Substring( 5, 1 ) == "*" || msg.ToLower().Substring( 5, 1 ) == " " || msg.ToLower().Substring( 5, 1 ) == Team.ToString() )
                    {
                        string[] splitchatline = msg.Split( " ".ToCharArray() );
                        string command = splitchatline[1].ToLower();
                        bool commandfound = false;
                        foreach (VoiceCommandInfo info in VoiceCommands)
                        {
                            if (info.command == command)
                            {
                                info.handler(msg, splitchatline, player);
                                commandfound = true;
                            }
                        }
                        if( !commandfound )
                        {
                            string helpstring = "CSAI commands available: help, ";
                            foreach( VoiceCommandInfo info in VoiceCommands )
                            {
                                helpstring += info.command + ", ";
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

        void DumpTimings()
        {
            logfile.WriteLine( "Timings:" );
            logfile.WriteLine( "unitcreatedtime: " + unitcreatedtime );
            logfile.WriteLine( "unitfinishedtime: " + unitfinishedtime );
            logfile.WriteLine( "unitidletime: " + unitidletime );
            logfile.WriteLine( "ticktime: " + ticktime );
            unitcreatedtime = 0;
            unitfinishedtime = 0;
            unitidletime = 0;
            ticktime = 0;
        }
        
        //called every frame
        int lastdebugframe = 0;
        public void Update()
        {
            System.DateTime start = DateTime.Now;
            try
            {
                if (!aicallback.IsGamePaused())
                {
                    if (TickEvent != null)
                    {
                        TickEvent();
                    }
                    if (DebugOn)
                    {
                        //logfile.WriteLine("frame: " + aicallback.GetCurrentFrame());
                        if (aicallback.GetCurrentFrame() - lastdebugframe > 30)
                        {
                            DumpTimings();
                            lastdebugframe = aicallback.GetCurrentFrame();
                        }
                    }
                }
            }
            catch( Exception e )
            {
                logfile.WriteLine( "Exception: " + e.ToString() );
                SendTextMsg("Exception: " + e.ToString());
            }
            ticktime += DateTime.Now.Subtract(start).TotalMilliseconds;
        }    
        
        
        public delegate void VoiceCommandHandler( string chatstring, string[] splitchatstring, int player );

        class VoiceCommandInfo
        {
            public string command;
            public VoiceCommandHandler handler;
            public VoiceCommandInfo(string command, VoiceCommandHandler handler)
            {
                this.command = command;
                this.handler = handler;
            }
        }
        
        List<VoiceCommandInfo> VoiceCommands = new List<VoiceCommandInfo>();
        public void RegisterVoiceCommand( string commandstring, VoiceCommandHandler handler )
        {
            commandstring = commandstring.ToLower();
            //if( !VoiceCommands.Contains( commandstring ) && commandstring != "help" )
            //{
                logfile.WriteLine("CSAI registering voicecommand " + commandstring );
                VoiceCommands.Add( new VoiceCommandInfo( commandstring, handler ) );
            //}
        }
        public void UnregisterVoiceCommand( string commandstring )
        {
            commandstring = commandstring.ToLower();
            //if( VoiceCommands.Contains( commandstring ) )
            //{
              //  logfile.WriteLine("CSAI unregistering voicecommand " + commandstring );
               // VoiceCommands.Remove( commandstring );
            //}
        }

        public void SendTextMsg(string text )
        {
            aicallback.SendTextMsg(text, 0);
        }
    }
}
