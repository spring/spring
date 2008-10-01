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
    // this will ultimately be migrated to tactics directory as groundmelee or such
    // for now it's a quick way of getting a battle going on!
    public class TankController : IUnitRequester, IController
    {
        //static TankController instance = new TankController();
        //public static TankController GetInstance(){ return instance; }
        
        public int MinTanksForAttack = 5;
        public int MinTanksForSpreadSearch = 50;
        
        public Hashtable TankDefsById = new Hashtable();
            
        //string[]UnitsWeLike = new string[]{ "armsam", "armstump", "armrock", "armjeth", "armkam", "armanac", "armsfig", "armmh", "armah", 
        //    "armbull", "armmart" };
              
        IPlayStyle playstyle;
    
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        UnitDefHelp unitdefhelp;
        UnitController unitcontroller;
        EnemyController enemycontroller;
        EnemySelector enemyselector;
        BuildTable buildtable;

        IFactoryController factorycontroller;
        
        AttackPackCoordinator attackpackcoordinator;
        GuardPackCoordinator guardpackcoordinator;
        SpreadSearchPackCoordinator spreadsearchpackcoordinator;
        MoveToPackCoordinator movetopackcoordinator;
        
        PackCoordinatorSelector packcoordinatorselector;
        
        Float3 LastAttackPos = null;
        
        Random random = new Random();
        
        public TankController( IPlayStyle playstyle )
        {
            this.playstyle = playstyle;
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            unitdefhelp = new UnitDefHelp( aicallback );
            unitcontroller = UnitController.GetInstance();
            enemycontroller = EnemyController.GetInstance();
            buildtable = BuildTable.GetInstance();

            enemyselector = new EnemySelector( 110, false, false );
            
            attackpackcoordinator = new AttackPackCoordinator( TankDefsById );
            spreadsearchpackcoordinator = new SpreadSearchPackCoordinator( TankDefsById );
            movetopackcoordinator = new MoveToPackCoordinator( TankDefsById );
            guardpackcoordinator = new GuardPackCoordinator( TankDefsById );
            
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
                factorycontroller = playstyle.GetFirstControllerOfType( typeof( IFactoryController ) ) as IFactoryController;
                factorycontroller.RegisterRequester( this );
            
                enemycontroller.NewEnemyAddedEvent += new EnemyController.NewEnemyAddedHandler( EnemyAdded );
                enemycontroller.EnemyRemovedEvent += new EnemyController.EnemyRemovedHandler( EnemyRemoved );
                
                csai.TickEvent += new CSAI.TickHandler( Tick );
                csai.UnitIdleEvent += new CSAI.UnitIdleHandler( UnitIdle );
                
                csai.RegisterVoiceCommand( "tankscount", new CSAI.VoiceCommandHandler( VoiceCommandCountTanks ) );
                csai.RegisterVoiceCommand( "tanksmoveto", new CSAI.VoiceCommandHandler( VoiceCommandMoveTo ) );
                csai.RegisterVoiceCommand( "tanksattackpos", new CSAI.VoiceCommandHandler( VoiceCommandAttackPos ) );
                
          //      unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
                unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
                TankDefsById.Clear(); // note, dont just make a new one, because searchpackcoordinator wont get new one
                unitcontroller.RefreshMyMemory( new UnitController.UnitAddedHandler( UnitAdded ) );
                
                //PackCoordinatorSelector.Activate();
                Active = true;
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
                
            //    unitcontroller.UnitAddedEvent -= new UnitController.UnitAddedHandler( UnitAdded );
                unitcontroller.UnitRemovedEvent -= new UnitController.UnitRemovedHandler( UnitRemoved );
                
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
            aicallback.SendTextMsg( "Number tanks: " + TankDefsById.Count, 0 );
            logfile.WriteLine( "Number tanks: " + TankDefsById.Count );
        }
        
        public void VoiceCommandMoveTo( string voicestring, string[] splitchatstring, int player )
        {
            int targetteam = Convert.ToInt32( splitchatstring[2] );
            if( targetteam == csai.Team )
            {
                Float3 targetpos = new Float3();
                targetpos.x = Convert.ToDouble( splitchatstring[3] );
                targetpos.z = Convert.ToDouble( splitchatstring[4] );
                targetpos.y =  aicallback.GetElevation( targetpos.x, targetpos.z );

              //  GotoPos( targetpos );
                movetopackcoordinator.SetTarget( targetpos );
                packcoordinatorselector.ActivatePackCoordinator( movetopackcoordinator );
            }
        }
        
        public void VoiceCommandAttackPos( string voicestring, string[] splitchatstring, int player )
        {
            int targetteam = Convert.ToInt32( splitchatstring[2] );
            if( targetteam == csai.Team )
            {
                Float3 targetpos = new Float3();
                targetpos.x = Convert.ToDouble( splitchatstring[3] );
                targetpos.z = Convert.ToDouble( splitchatstring[4] );
                targetpos.y =  aicallback.GetElevation( targetpos.x, targetpos.z );
                
                attackpackcoordinator.SetTarget( targetpos );
                packcoordinatorselector.ActivatePackCoordinator( attackpackcoordinator );
            }
        }
        
        // return priority indicating how badly you need some units
        // they can ask factory what it is able to build
        public double DoYouNeedSomeUnits( IFactory factory )
        {
            if( TankDefsById.Count < MinTanksForAttack )
            {
                logfile.WriteLine( "TankController: signalling need unit priority 1" );
                return 1.0;
            }
            else
            {
                return 0.5;
            }
        }
            
        // note to self: this should be adjusted according to enemy units, factory etc...
        public IUnitDef WhatUnitDoYouNeed( IFactory factory )
        {
            IUnitDef unitdef = null;

//            if( random.Next( 1, 6 ) < 5 )
   //         {
            if( factory.Name.ToLower() == "armvp" )
            {
                if( random.Next( 1, 6 ) < 5 )
                {
                    unitdef = buildtable.UnitDefByName[ "armstump" ] as IUnitDef;
                }
                else
                {
                    unitdef = buildtable.UnitDefByName[ "armsam" ] as IUnitDef;
                }
            }
            else if( factory.Name.ToLower() == "armlab" )
            {
                if( random.Next( 1, 6 ) < 5 )
                {
                    unitdef = buildtable.UnitDefByName[ "armrock" ] as IUnitDef;
                }
                else
                {
                    unitdef = buildtable.UnitDefByName[ "armjeth" ] as IUnitDef;
                }
            }
            else if( factory.Name.ToLower() == "armap" )
            {
                unitdef = buildtable.UnitDefByName[ "armsfig" ] as IUnitDef;
            }
            else if( factory.Name.ToLower() == "armhp" )
            {
                int randomnumber = random.Next( 1, 7 );
                if( randomnumber < 5 )
                {
                    unitdef = buildtable.UnitDefByName[ "armanac" ] as IUnitDef;
                }
                else if( randomnumber < 6 )
                {
                    unitdef = buildtable.UnitDefByName[ "armmh" ] as IUnitDef;                    
                }
                else if( randomnumber < 7 )
                {
                    unitdef = buildtable.UnitDefByName[ "armah" ] as IUnitDef;                    
                }
            }
            else if ( factory.Name.ToLower() == "armavp" )
            {
                int randomnumber = random.Next( 1, 6 );
                if( randomnumber < 4 )
                {
                    unitdef = buildtable.UnitDefByName[ "armbull" ] as IUnitDef;
                }
                else if( randomnumber < 5 )
                {
                    unitdef = buildtable.UnitDefByName[ "armmart" ] as IUnitDef;
                }
                else
                {
                    unitdef = buildtable.UnitDefByName[ "armmart" ] as IUnitDef;
                }
            }
            return unitdef;
        }

        // signal to requester that we granted its request
        public void WeAreBuildingYouA( IUnitDef unitwearebuilding )
        {
        }
            
        // your request got destroyed; new what-to-build negotations round
        public void YourRequestWasDestroyedDuringBuilding_Sorry( IUnitDef unitwearebuilding )
        {
        }
        /*
        bool WeLikeUnit( string name )
        {
            foreach( string unitwelike in UnitsWeLike )
            {
                if( unitwelike == name )
                {
                    return true;
                }
            }
            return false;
        }
        */
        // shifted this to unitidle to give unit time to leave factory
        public void UnitIdle( int id )
        {
            // make this more elegant later, with concept of ownership etc
            if( !TankDefsById.Contains( id ) )
            {
                IUnitDef unitdef = aicallback.GetUnitDef( id );
                if( WeLikeUnit( unitdef.name.ToLower() ) )
                {
                    logfile.WriteLine( "TankController new tank " + " id " + id + " " + unitdef.humanName + " speed " + unitdef.speed );
                    TankDefsById.Add( id, unitdef );
                 //   DoSomething();
                }
            }
        }
        
        /*
        public void UnitAdded( int id, IUnitDef unitdef )
        {
            // make this more elegant later, with concept of ownership etc
            if( WeLikeUnit( unitdef.name.ToLower() ) )
            {
                if( !TankDefsById.Contains( id ) )
                {
                    logfile.WriteLine( "TankController new tank " + " id " + id + " " + unitdef.humanName + " speed " + unitdef.speed );
                    TankDefsById.Add( id, unitdef );
                 //   DoSomething();
                }
            }
        }
        */
        public void UnitRemoved( int id )
        {
            if( TankDefsById.Contains( id ) )
            {
                logfile.WriteLine( "TankController tank removed " + id );
                TankDefsById.Remove( id );
               // DoSomething();
            }
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
            if( TankDefsById.Count >= MinTanksForAttack ) // make sure at least have a few units before attacking
            {
                Float3 approximateattackpos = LastAttackPos;
                if( approximateattackpos == null )
                {
                    approximateattackpos = CommanderController.GetInstance().GetCommanderPos();
                }
                
                Float3 nearestenemypos = enemyselector.ChooseAttackPoint( approximateattackpos );
                if( nearestenemypos != null )
                {
                    attackpackcoordinator.SetTarget( nearestenemypos );
                    packcoordinatorselector.ActivatePackCoordinator( attackpackcoordinator );
                    LastAttackPos = nearestenemypos;
                }
                else
                {
                    if( TankDefsById.Count > MinTanksForSpreadSearch )
                    {
                        spreadsearchpackcoordinator.SetTarget( nearestenemypos );
                        packcoordinatorselector.ActivatePackCoordinator( spreadsearchpackcoordinator );
                    }
                    else
                    {
                        guardpackcoordinator.SetTarget( CommanderController.GetInstance().commanderid );
                        packcoordinatorselector.ActivatePackCoordinator( guardpackcoordinator );
                        // movetopackcoordinator.SetTarget( CommanderController.GetInstance().GetCommanderPos() );
                        //packcoordinatorselector.ActivatePackCoordinator( movetopackcoordinator );
                    }
                }
            }
            else
            {
                guardpackcoordinator.SetTarget( CommanderController.GetInstance().commanderid );
                packcoordinatorselector.ActivatePackCoordinator( guardpackcoordinator );
                //movetopackcoordinator.SetTarget( CommanderController.GetInstance().GetCommanderPos() );
                //packcoordinatorselector.ActivatePackCoordinator( movetopackcoordinator );
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

        void IUnitRequester.UnitFinished(int deployedid, IUnitDef unitwearebuilding)
        {
            logfile.WriteLine( "TankController new tank " + " id " + id + " " + unitdef.humanName + " speed " + unitdef.speed );
            TankDefsById.Add( deployedid, unitwearebuilding );
            //   DoSomething();
        }
    }
}
