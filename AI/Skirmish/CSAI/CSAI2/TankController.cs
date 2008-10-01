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
using System.Xml;

namespace CSharpAI
{
    // this will ultimately be migrated to tactics directory as groundmelee or such
    // for now it's a quick way of getting a battle going on!
    public class TankController
    {
        //static TankController instance = new TankController();
        //public static TankController GetInstance(){ return instance; }
        
        public int MinTanksForAttack = 5;
        public int MinTanksForSpreadSearch = 50;
        
        public Dictionary<int,IUnitDef> DefsById = new Dictionary<int,IUnitDef>();
        IUnitDef typicalunitdef;
            
        //string[]UnitsWeLike = new string[]{ "armsam", "armstump", "armrock", "armjeth", "armkam", "armanac", "armsfig", "armmh", "armah", 
         //   "armbull", "armmart", "armmav", "armyork" };
              
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        UnitController unitcontroller;
        EnemyController enemycontroller;
        EnemySelector2 enemyselector;
        BuildTable buildtable;

        AttackPackCoordinator attackpackcoordinator;
        GuardPackCoordinator guardpackcoordinator;
        SpreadSearchPackCoordinator spreadsearchpackcoordinator;
        MoveToPackCoordinator movetopackcoordinator;
        
        PackCoordinatorSelector packcoordinatorselector;
        
        Float3 LastAttackPos = null;
        
        Random random = new Random();
        
        public TankController( Dictionary< int,IUnitDef>UnitDefsById, IUnitDef typicalunitdef)
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();

            this.DefsById = UnitDefsById;
            this.typicalunitdef = typicalunitdef;
            
            unitcontroller = UnitController.GetInstance();
            enemycontroller = EnemyController.GetInstance();
            buildtable = BuildTable.GetInstance();

            enemyselector = new EnemySelector2( typicalunitdef.speed * 2, typicalunitdef );
            // speed here is experimental

            attackpackcoordinator = new AttackPackCoordinator(DefsById);
            spreadsearchpackcoordinator = new SpreadSearchPackCoordinator(DefsById);
            movetopackcoordinator = new MoveToPackCoordinator(DefsById);
            guardpackcoordinator = new GuardPackCoordinator(DefsById);
            
            packcoordinatorselector = new PackCoordinatorSelector();
            packcoordinatorselector.LoadCoordinator( attackpackcoordinator );
            packcoordinatorselector.LoadCoordinator( spreadsearchpackcoordinator );
            packcoordinatorselector.LoadCoordinator( movetopackcoordinator );
            packcoordinatorselector.LoadCoordinator( guardpackcoordinator );

            logfile.WriteLine( "*TankController Initialized*" );
        }
        
        bool Active = false;
        
        public void Activate()
        {
            if( !Active )
            {
                enemycontroller.NewEnemyAddedEvent += new EnemyController.NewEnemyAddedHandler( EnemyAdded );
                enemycontroller.EnemyRemovedEvent += new EnemyController.EnemyRemovedHandler( EnemyRemoved );
                
                csai.TickEvent += new CSAI.TickHandler( Tick );
                csai.UnitIdleEvent += new CSAI.UnitIdleHandler( UnitIdle );

                if (csai.DebugOn)
                {
                    csai.RegisterVoiceCommand("tankscount", new CSAI.VoiceCommandHandler(VoiceCommandCountTanks));
                    csai.RegisterVoiceCommand("tanksmoveto", new CSAI.VoiceCommandHandler(VoiceCommandMoveTo));
                    csai.RegisterVoiceCommand("tanksattackpos", new CSAI.VoiceCommandHandler(VoiceCommandAttackPos));
                    csai.RegisterVoiceCommand("dumptanks", new CSAI.VoiceCommandHandler(VoiceCommandDumpTanks));
                }

                //unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
                //unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
                //TankDefsById.Clear(); // note, dont just make a new one, because searchpackcoordinator wont get new one
                //unitcontroller.RefreshMyMemory( new UnitController.UnitAddedHandler( UnitAdded ) );
                
                //PackCoordinatorSelector.Activate();
                Active = true;
            }
        }

        public void VoiceCommandDumpTanks(string cmd, string[] split, int player)
        {
            logfile.WriteLine("Tankcontroller dump:");
            foreach (KeyValuePair<int, IUnitDef> kvp in DefsById)
            {
                logfile.WriteLine( kvp.Key + " " + kvp.Value.humanName);
            }
        }

        public void Disactivate()
        {
            if( Active )
            {
                enemycontroller.NewEnemyAddedEvent -= new EnemyController.NewEnemyAddedHandler( EnemyAdded );
                enemycontroller.EnemyRemovedEvent -= new EnemyController.EnemyRemovedHandler( EnemyRemoved );
                
                csai.TickEvent -= new CSAI.TickHandler( Tick );
                csai.UnitIdleEvent -= new CSAI.UnitIdleHandler( UnitIdle );
                
                csai.UnregisterVoiceCommand( "tankscount" );
                csai.UnregisterVoiceCommand( "tanksmoveto" );
                csai.UnregisterVoiceCommand( "tanksattackpos" );
                
                //unitcontroller.UnitAddedEvent -= new UnitController.UnitAddedHandler( UnitAdded );
                //unitcontroller.UnitRemovedEvent -= new UnitController.UnitRemovedHandler( UnitRemoved );
                
                packcoordinatorselector.DisactivateAll();
                Active = false;
            }
        }
        
        // planned, controller can control any units at its discretion, for now:
        public void AssignUnits( ArrayList unitdefs ){}  // give units to this controller
        public void RevokeUnits( ArrayList unitdefs ){}  // remove these units from this controller
        
        // planned, not used yet, controller can use energy and metal at its discretion for now:
        public void AssignEnergy( int energy ){} // give energy to controller; negative to revoke
        public void AssignMetal( int metal ){} // give metal to this controller; negative to revoke
        public void AssignPower( double power ){} // assign continuous power flow to this controller; negative for reverse flow
        public void AssignMetalStream( double metalstream ){} // assign continuous metal flow to this controller; negative for reverse flow        
                
        public void VoiceCommandCountTanks( string voicestring, string[] splitchatstring, int player )
        {
            aicallback.SendTextMsg( "Number tanks: " + DefsById.Count, 0 );
            logfile.WriteLine( "Number tanks: " + DefsById.Count );
        }
        
        public void VoiceCommandMoveTo( string voicestring, string[] splitchatstring, int player )
        {
         //   int targetteam = Convert.ToInt32( splitchatstring[2] );
            //if( targetteam == csai.Team )
           // {
                Float3 targetpos = new Float3();
                targetpos.x = Convert.ToDouble( splitchatstring[2] );
                targetpos.z = Convert.ToDouble( splitchatstring[3] );
                targetpos.y =  aicallback.GetElevation( targetpos.x, targetpos.z );

              //  GotoPos( targetpos );
                movetopackcoordinator.SetTarget( targetpos );
                packcoordinatorselector.ActivatePackCoordinator( movetopackcoordinator );
            //}
        }
        
        public void VoiceCommandAttackPos( string voicestring, string[] splitchatstring, int player )
        {
            //int targetteam = Convert.ToInt32( splitchatstring[2] );
            //if( targetteam == csai.Team )
            //{
                Float3 targetpos = new Float3();
                targetpos.x = Convert.ToDouble( splitchatstring[2] );
                targetpos.z = Convert.ToDouble( splitchatstring[3] );
                targetpos.y =  aicallback.GetElevation( targetpos.x, targetpos.z );
                
                attackpackcoordinator.SetTarget( targetpos );
                packcoordinatorselector.ActivatePackCoordinator( attackpackcoordinator );
            //}
        }
        
        //bool WeLikeUnit( string name )
        //{
          //  foreach( string unitwelike in UnitsWeLike )
            //{
              //  if( unitwelike == name )
                //{
                  //  return true;
                //}
            //}
            //return false;
        //}
        
        // shifted this to unitidle to give unit time to leave factory
        public void UnitIdle( int id )
        {
            //if( !DefsById.Contains( id ) )
            //{
              //  IUnitDef unitdef = aicallback.GetUnitDef( id );
                //if( WeLikeUnit( unitdef.name.ToLower() ) )
                //{
                  //  logfile.WriteLine( "TankController new tank " + " id " + id + " " + unitdef.humanName + " speed " + unitdef.speed );
                    //TankDefsById.Add( id, unitdef );
                 //   DoSomething();
                //}
            //}
        }
      
        
        public void UnitAdded( int id, IUnitDef unitdef )
        {
            /*
            // make this more elegant later, with concept of ownership etc
            logfile.WriteLine("tankcontroller.UnitAdded " + unitdef.humanName);
            if( WeLikeUnit( unitdef.name.ToLower() ) )
            {
                logfile.WriteLine("unit in tanktype list");
                if (!TankDefsById.Contains(id))
                {
                    logfile.WriteLine("TankController new tank " + " id " + id + " " + unitdef.humanName + " speed " + unitdef.speed);
                    TankDefsById.Add( id, unitdef );
                    if (LastAttackPos == null)
                    {
                        LastAttackPos = aicallback.GetUnitPos(id);
                    }
                    enemyselector.InitStartPos(aicallback.GetUnitPos(id));
                  //  aicallback.GiveOrder( id, new Command( Command.CMD_TRAJECTORY, new double[]{1}) );
                 //   DoSomething();
                }
            }
             */
        }
        
        public void UnitRemoved( int id )
        {
            /*
            if( TankDefsById.Contains( id ) )
            {
                logfile.WriteLine( "TankController tank removed " + id );
                TankDefsById.Remove( id );
               // DoSomething();
            }
             */
        }
        
        public void EnemyAdded( int id, IUnitDef unitdef )
        {
            //DoSomething();
        }
        
        public void EnemyRemoved( int id )
        {
            //DoSomething();
        }
        
        void DoSomething()
        {
            if( DefsById.Count >= MinTanksForAttack ) // make sure at least have a few units before attacking
            {
                Float3 approximateattackpos = LastAttackPos;
                if( approximateattackpos == null )
                {
                    foreach (int id in DefsById.Keys)
                    {
                        Float3 thispos = aicallback.GetUnitPos(id);
                        if (thispos != null)
                        {
                            approximateattackpos = thispos;
                        }
                    }
                    //approximateattackpos = aicallback.GetUnitPos(DefsById.Keys.GetEnumerator().Current);
                }
                logfile.WriteLine("tankcontroller approximateattackpos: " + approximateattackpos);
                Float3 nearestenemypos = enemyselector.ChooseAttackPoint( approximateattackpos );
                if( nearestenemypos != null )
                {
                    attackpackcoordinator.SetTarget( nearestenemypos );
                    packcoordinatorselector.ActivatePackCoordinator( attackpackcoordinator );
                    LastAttackPos = nearestenemypos;
                }
                else
                {
                    if( DefsById.Count > MinTanksForSpreadSearch )
                    {
                        spreadsearchpackcoordinator.SetTarget( nearestenemypos );
                        packcoordinatorselector.ActivatePackCoordinator( spreadsearchpackcoordinator );
                    }
                    else
                    {
                        if (unitcontroller.UnitDefsByName.ContainsKey("arm_commander" ))
                        {
                            guardpackcoordinator.SetTarget(unitcontroller.UnitDefsByName["arm_commander"][0].id);
                            packcoordinatorselector.ActivatePackCoordinator(guardpackcoordinator);
                        }
                    }
                }
            }
            else
            {
                if (unitcontroller.UnitDefsByName.ContainsKey("arm_commander"))
                {
                    guardpackcoordinator.SetTarget(unitcontroller.UnitDefsByName["arm_commander"][0].id);
                    packcoordinatorselector.ActivatePackCoordinator(guardpackcoordinator);
                }
            }
        }
        
        int tickcount = 0;
        
        int itick = 0;
        public void Tick()
        {
            tickcount++;
            itick++;
            if( itick >= 30 )
            {
                DoSomething();
                itick = 0;
            }
        }

    }
}
