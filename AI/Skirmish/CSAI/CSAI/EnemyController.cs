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
using System.Xml;

namespace CSharpAI
{
    // responsible for tracking enemy units
    // Note that the UnitDefs of units outside of LOS are not generally available, so we have to cope with the possibility of null UnitDefs floating around...
    public class EnemyController
    {
        public delegate void NewEnemyAddedHandler( int enemyid, IUnitDef unitdef );
        public delegate void EnemyRemovedHandler( int enemyid );
            
        public event NewEnemyAddedHandler NewEnemyAddedEvent;
        public event EnemyRemovedHandler EnemyRemovedEvent;
        
        public Hashtable EnemyUnitDefByDeployedId = new Hashtable();
        public Hashtable EnemyStaticPosByDeployedId = new Hashtable();
        
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
        
            csai.RegisterVoiceCommand( "enemiescount", new CSAI.VoiceCommandHandler( VoiceCommandCountEnemies ) );
            csai.RegisterVoiceCommand( "showenemies", new CSAI.VoiceCommandHandler( VoiceCommandShowEnemies ) );
            csai.RegisterVoiceCommand( "autoshowenemieson", new CSAI.VoiceCommandHandler( VoiceCommandAutoShowEnemiesOn ) );
            csai.RegisterVoiceCommand( "autoshowenemiesoff", new CSAI.VoiceCommandHandler( VoiceCommandAutoShowEnemiesOff ) );
            
            unitdefhelp = new UnitDefHelp( aicallback );
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
            foreach( DictionaryEntry de in EnemyStaticPosByDeployedId )
            {
                Float3 pos = de.Value as Float3;
                aicallback.DrawUnit(BuildTable.ArmGroundScout, pos, 0.0f, 50, aicallback.GetMyAllyTeam(), true, true);
            }
            foreach( DictionaryEntry de in EnemyUnitDefByDeployedId )
            {
                int enemyid = (int)de.Key;
                //IUnitDef unit = de.Value as Float3;
                Float3 pos = aicallback.GetUnitPos( enemyid );
                aicallback.DrawUnit(BuildTable.ArmL1AntiAir, pos, 0.0f, 50, aicallback.GetMyAllyTeam(), true, true);
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
        /*
        public int GetNearestEnemy( Float3 pos )
        {
            double mindistancesquared = 100000000000;
            int closestenemy = 0;
            foreach( DictionaryEntry de in EnemyUnitDefByDeployedId )
            {
                int enemyid = (int)de.Key;
                IUnitDef unitdef = de.Value as IUnitDef;
                Float3 enemypos = aicallback.GetUnitPos( enemyid );
                logfile.WriteLine( "GetNearestEnemy, checking id " + enemyid + " pos " + enemypos.ToString() );
                double thisdistancesquared = Float3Helper.GetSquaredDistance( enemypos, pos );
                //logfile.WriteLine( " thisdistancesquared: " + thisdistancesquared );
                if( thisdistancesquared < mindistancesquared )
                {
                    logfile.WriteLine( "GetNearestEnemy, candidate: " + enemyid + " distancesquared: " + thisdistancesquared );
                    mindistancesquared = thisdistancesquared;
                    closestenemy = enemyid;
                }
            }
            return closestenemy;
        }
        */
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
            RemoveEnemy( enemyid );
        }
        
        public void EnemyDestroyed( int enemyid, int attacker )
        {
            if( EnemyStaticPosByDeployedId.Contains( enemyid ) )
            {
                EnemyStaticPosByDeployedId.Remove( enemyid );
            }
            RemoveEnemy( enemyid );
        }
        
        void AddEnemy( int enemyid )
        {
            IUnitDef unitdef = aicallback.GetUnitDef( enemyid );
            if( !EnemyUnitDefByDeployedId.Contains( enemyid ) )
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
                    if( !EnemyStaticPosByDeployedId.Contains( enemyid ) )
                    {
                        EnemyStaticPosByDeployedId.Add( enemyid, aicallback.GetUnitPos( enemyid ) );
                    }
                }
            }
        }
        
        void RemoveEnemy( int enemyid )
        {
            if( EnemyUnitDefByDeployedId.Contains( enemyid ) )
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
                //logfile.WriteLine("EnemyController running static enemy clean" );
                foreach( DictionaryEntry de in unitcontroller.UnitDefByDeployedId )
                {
                    int unitid = (int)de.Key;
                    Float3 thispos = aicallback.GetUnitPos( unitid );
                    IntArrayList enemiestoclean = new IntArrayList();
                    foreach( DictionaryEntry enemyde in EnemyStaticPosByDeployedId )
                    {
                        int enemyid = (int)enemyde.Key;
                        Float3 pos = enemyde.Value as Float3;
                        if( Float3Helper.GetSquaredDistance( pos, thispos ) < squaredvisibilitydistance )
                        {
                            enemiestoclean.Add( enemyid );
                        }
                    }
                    foreach( int enemyidtoclean in enemiestoclean )
                    {
                        EnemyStaticPosByDeployedId.Remove( enemyidtoclean );
                    }
                }
                if( autoshowenemies )
                {
                    ShowEnemies();
                }
            }
        }
    }
}
