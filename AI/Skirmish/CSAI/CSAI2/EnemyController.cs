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
using System.Collections.Generic;
using System.Xml;

namespace CSharpAI
{
    // responsible for tracking enemy units
    // Note that the UnitDefs of units outside of LOS are not generally available, so we have to cope with the possibility of null UnitDefs floating around...
    public class EnemyController
    {
        public delegate void NewEnemyAddedHandler( int enemyid, IUnitDef unitdef );
        public delegate void NewStaticEnemyAddedHandler( int enemyid, Float3 pos, IUnitDef unitdef );
        public delegate void EnemyRemovedHandler( int enemyid );
            
        public event NewEnemyAddedHandler NewEnemyAddedEvent;
        public event NewStaticEnemyAddedHandler NewStaticEnemyAddedEvent;
        public event EnemyRemovedHandler EnemyRemovedEvent;

        public Dictionary<int, IUnitDef> EnemyUnitDefByDeployedId = new Dictionary<int, IUnitDef>();
        public Dictionary<int, Float3> EnemyStaticPosByDeployedId = new Dictionary<int, Float3>();
        
        static EnemyController instance = new EnemyController();
        public static EnemyController GetInstance(){ return instance; }
    
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        UnitDefHelp unitdefhelp;
        UnitController unitcontroller;
        
        EnemyController()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();

            autoshowenemies = csai.DebugOn;
            
            unitcontroller = UnitController.GetInstance();
            
            csai.TickEvent += new CSAI.TickHandler( Tick );
            csai.EnemyEnterRadarEvent += new CSAI.EnemyEnterRadarHandler( this.EnemyEnterRadar );
            csai.EnemyEnterLOSEvent += new CSAI.EnemyEnterLOSHandler( this.EnemyEnterLOS );
            csai.EnemyLeaveRadarEvent += new CSAI.EnemyLeaveRadarHandler( this.EnemyLeaveRadar );
            csai.EnemyDestroyedEvent += new CSAI.EnemyDestroyedHandler( this.EnemyDestroyed );
            csai.UnitDamagedEvent += new CSAI.UnitDamagedHandler(csai_UnitDamagedEvent);
        
            csai.RegisterVoiceCommand( "enemiescount", new CSAI.VoiceCommandHandler( VoiceCommandCountEnemies ) );
            csai.RegisterVoiceCommand( "showenemies", new CSAI.VoiceCommandHandler( VoiceCommandShowEnemies ) );
            csai.RegisterVoiceCommand( "autoshowenemieson", new CSAI.VoiceCommandHandler( VoiceCommandAutoShowEnemiesOn ) );
            csai.RegisterVoiceCommand( "autoshowenemiesoff", new CSAI.VoiceCommandHandler( VoiceCommandAutoShowEnemiesOff ) );
            
            unitdefhelp = new UnitDefHelp( aicallback );
        }

        // cheap hack to respond to enemy shooting us
        void csai_UnitDamagedEvent(int damaged, int attacker, float damage, Float3 dir)
        {
            /*
            if (!EnemyUnitDefByDeployedId.ContainsKey(attacker))
            {
                EnemyUnitDefByDeployedId.Add(attacker, aicallback.GetUnitDef(attacker));
            }
             */
            //if (!EnemyStaticPosByDeployedId.ContainsKey(attacker))
            //{
            Float3 enemypos = aicallback.GetUnitPos(attacker);
            if (enemypos != null &&
                Float3Helper.GetSquaredDistance(enemypos, new Float3(0, 0, 0)) > 10 * 10)
            {
                logfile.WriteLine("unitdamaged, attacker " + attacker + " pos " + enemypos);
                if (!EnemyStaticPosByDeployedId.ContainsKey(attacker))
                {
                    EnemyStaticPosByDeployedId.Add(attacker, enemypos);
                }
                else
                {
                    EnemyStaticPosByDeployedId[attacker] = enemypos;
                }
                if (NewStaticEnemyAddedEvent != null)
                {
                    NewStaticEnemyAddedEvent(attacker, enemypos, null);
                }
            }
            else // else we guess...
            {
                if (FriendlyUnitPositionObserver.GetInstance().PosById.ContainsKey(damaged))
                {
                    Float3 ourunitpos = FriendlyUnitPositionObserver.GetInstance().PosById[damaged];
                    if (ourunitpos != null)
                    {
                        Float3 guessvectortotarget = dir * 300.0;
                        logfile.WriteLine("vectortotarget guess: " + guessvectortotarget.ToString());
                        Float3 possiblepos = ourunitpos + guessvectortotarget;

                        if (!EnemyStaticPosByDeployedId.ContainsKey(attacker))
                        {
                            EnemyStaticPosByDeployedId.Add(attacker, possiblepos);
                        }
                        else
                        {
                            EnemyStaticPosByDeployedId[attacker] = possiblepos;
                        }

                        if (NewStaticEnemyAddedEvent != null)
                        {
                            NewStaticEnemyAddedEvent(attacker, possiblepos, null);
                        }
                        logfile.WriteLine("unitdamaged, attacker " + attacker + " our unit pos " + ourunitpos + " dir " + dir.ToString() + " guess: " + possiblepos.ToString());
                    }
                }
            }
            //}
        }
        
        bool autoshowenemies = false;
        
        public void VoiceCommandAutoShowEnemiesOn( string voicestring, string[] splitchatstring, int player )
        {
            autoshowenemies = true;
            ShowEnemies();
        }
        
        public void VoiceCommandAutoShowEnemiesOff( string voicestring, string[] splitchatstring, int player )
        {
            autoshowenemies = false;
        }
        
        public void VoiceCommandShowEnemies( string voicestring, string[] splitchatstring, int player )
        {
            aicallback.SendTextMsg( "Number enemies: " + EnemyUnitDefByDeployedId.Count, 0 );
            aicallback.SendTextMsg( "Static enemies: " + EnemyStaticPosByDeployedId.Count, 0 );
            ShowEnemies();
        }
        
        public void VoiceCommandCountEnemies( string voicestring, string[] splitchatstring, int player )
        {
            aicallback.SendTextMsg( "Number enemies: " + EnemyUnitDefByDeployedId.Count, 0 );
            aicallback.SendTextMsg( "Static enemies: " + EnemyStaticPosByDeployedId.Count, 0 );
            logfile.WriteLine( "Number enemies: " + EnemyUnitDefByDeployedId.Count );
        }
        
        void ShowEnemies()
        {
            foreach( KeyValuePair<int,Float3>kvp in EnemyStaticPosByDeployedId )
            {
                Float3 pos = kvp.Value;
                aicallback.DrawUnit("ARMAMD", pos, 0.0f, 50, aicallback.GetMyAllyTeam(), true, true);
            }
            foreach (KeyValuePair<int,IUnitDef> kvp in EnemyUnitDefByDeployedId)
            {
                int enemyid = kvp.Key;
                //IUnitDef unit = de.Value as Float3;
                Float3 pos = aicallback.GetUnitPos( enemyid );
                if (pos != null)
                {
                    aicallback.DrawUnit("ARMSAM", pos, 0.0f, 50, aicallback.GetMyAllyTeam(), true, true);
                }
            }
          //  logfile.WriteLine( "Number enemies: " + EnemyUnitDefByDeployedId.Count );
        }
                
        public void LoadExistingUnits()
        {
            int[] enemyunits = aicallback.GetEnemyUnitsInRadarAndLos();	
            foreach( int enemyid in enemyunits )
            {
                IUnitDef unitdef = aicallback.GetUnitDef( enemyid );
                logfile.WriteLine("enemy unit existing: " + enemyid );
                AddEnemy( enemyid );
            }
        }

        public void EnemyEnterRadar( int enemyid )
        {
            AddEnemy( enemyid );
        }
        
        public void EnemyEnterLOS( int enemyid )
        {
            AddEnemy( enemyid );
        }        
        
        public void EnemyLeaveRadar( int enemyid )
        {
            //RemoveEnemy( enemyid );
        }
        
        public void EnemyDestroyed( int enemyid, int attacker )
        {
            if( EnemyStaticPosByDeployedId.ContainsKey( enemyid ) )
            {
                EnemyStaticPosByDeployedId.Remove( enemyid );
            }
            RemoveEnemy( enemyid );
        }
        
        void AddEnemy( int enemyid )
        {
            IUnitDef unitdef = aicallback.GetUnitDef( enemyid );
            if( !EnemyUnitDefByDeployedId.ContainsKey( enemyid ) )
            {
                EnemyUnitDefByDeployedId.Add( enemyid, unitdef );
                logfile.WriteLine("EnemyController New enemy: " + enemyid );
                if( NewEnemyAddedEvent != null )
                {
                    NewEnemyAddedEvent( enemyid, unitdef );
                }
            }
            if( unitdef != null )
            {
                if( EnemyUnitDefByDeployedId[ enemyid ] == null )
                {
                    EnemyUnitDefByDeployedId[ enemyid ] = unitdef;
                    logfile.WriteLine("enemy " + enemyid + " is " + unitdef.humanName );
                }
                if( !unitdefhelp.IsMobile( unitdef ) )
                {
                    if( !EnemyStaticPosByDeployedId.ContainsKey( enemyid ) )
                    {
                        Float3 thispos = aicallback.GetUnitPos( enemyid );
                        if (thispos != null)
                        {
                            EnemyStaticPosByDeployedId.Add(enemyid, thispos);
                            if (NewStaticEnemyAddedEvent != null)
                            {
                                NewStaticEnemyAddedEvent(enemyid, thispos, unitdef);
                            }
                        }
                    }
                }
            }
        }
        
        void RemoveEnemy( int enemyid )
        {
            if( EnemyUnitDefByDeployedId.ContainsKey( enemyid ) )
            {
                EnemyUnitDefByDeployedId.Remove( enemyid );
                if( EnemyRemovedEvent != null )
                {
                    logfile.WriteLine("EnemyController Enemy removed: " + enemyid );
                    EnemyRemovedEvent( enemyid );
                }
            }
        }
        
        int itickcount = 0;
        public void Tick()
        {
            itickcount++;
            double squaredvisibilitydistance = 40 * 40;
            if( itickcount > 30 )
            {
                itickcount = 0;
                /*
                foreach (KeyValuePair<int, Float3> kvp in EnemyStaticPosByDeployedId)
                {
                    int enemyid = kvp.Key;
                    Float3 pos = kvp.Value;
                    if (aicallback.GetCurrentFrame() - LosMap.GetInstance().LastSeenFrameCount[pos.x / 16, pos.z / 16] < 30)
                    {

                    }
                }
                 */
                //logfile.WriteLine("EnemyController running static enemy clean" );
                foreach( KeyValuePair<int, IUnitDef> keyvaluepair in unitcontroller.UnitDefByDeployedId )
                {
                    int unitid = keyvaluepair.Key;
                    IUnitDef thisfriendlyunitdef = keyvaluepair.Value;
                    Float3 thispos = aicallback.GetUnitPos( unitid );
                    IntArrayList enemiestoclean = new IntArrayList();
                    foreach (KeyValuePair<int,Float3> kvp in EnemyStaticPosByDeployedId)
                    {
                        int enemyid = kvp.Key;
                        Float3 pos = kvp.Value;
                        if (Float3Helper.GetSquaredDistance(pos, thispos) < thisfriendlyunitdef.losRadius * thisfriendlyunitdef.losRadius * 16 * 16 )
                        {
                            enemiestoclean.Add( enemyid );
                        }
                    }
                    foreach( int enemyidtoclean in enemiestoclean )
                    {
                        EnemyStaticPosByDeployedId.Remove( enemyidtoclean );
                    }
                }
                if( csai.DebugOn )
                {
                    ShowEnemies();
                }
            }
        }
    }
}
