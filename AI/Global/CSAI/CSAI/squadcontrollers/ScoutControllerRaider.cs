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
    public class ScoutControllerRaider : IUnitRequester, IController
    {
        public int nearbyforenemiesmeans = 250;
        public int enemysightedsearchradius = 1000;
            
     //   int[,] sectorlastcheckedtickcount;
      //  bool[,] sectorispriority;
        
        //static ScoutControllerRaider instance = new ScoutControllerRaider();
        //public static ScoutControllerRaider GetInstance(){ return instance; }
        
        IPlayStyle playstyle;
    
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        UnitController unitcontroller;
        BuildTable buildtable;
        EnemyController enemycontroller;
        
        Queue PriorityTargets = new Queue();

        IFactoryController factorycontroller;
        
        SpreadSearchPackCoordinatorWithSearchGrid searchcoordinator;
        
        Hashtable ScoutUnitDefsById = new Hashtable();
        
        //int numrequestsinqueue = 0;
        Random random;
        
       // int terrainwidth;
       // int terrainheight;
        
        public ScoutControllerRaider( IPlayStyle playstyle )
        {
            this.playstyle = playstyle;
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            random = new Random();

            //terrainwidth = aicallback.GetMapWidth() * MovementMaps.SQUARE_SIZE;
           // terrainheight = aicallback.GetMapHeight() * MovementMaps.SQUARE_SIZE;
            
            //int[,] sectorlastcheckedtickcount = new int[ terrainwidth, terrainheight ];
           // bool[,] sectorispriority = new bool[ terrainwidth, terrainheight ];;
            
            unitcontroller = UnitController.GetInstance();
            buildtable = BuildTable.GetInstance();
            enemycontroller = EnemyController.GetInstance();
            
            searchcoordinator = new SpreadSearchPackCoordinatorWithSearchGrid( ScoutUnitDefsById );
            
            logfile.WriteLine( "*ScoutControllerRaider initialized*" );
        }
        
        bool Active = false;
        
        public void Activate()
        {
            if( !Active )
            {
                factorycontroller = playstyle.GetFirstControllerOfType( typeof( IFactoryController ) ) as IFactoryController;
                factorycontroller.RegisterRequester( this );
                
                csai.TickEvent += new CSAI.TickHandler( Tick );
                csai.RegisterVoiceCommand( "countscouts", new CSAI.VoiceCommandHandler( VoiceCommandCountScouts ) );
                
                enemycontroller.NewEnemyAddedEvent += new EnemyController.NewEnemyAddedHandler( EnemyAdded );
                
                unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
                unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
                ScoutUnitDefsById.Clear(); // note, dont just make a new one, because searchpackcoordinator wont get new one
                unitcontroller.RefreshMyMemory( new UnitController.UnitAddedHandler( UnitAdded ) );

                searchcoordinator.Activate();
                Active = true;
            }
        }
        
        public void Disactivate()
        {
            if( Active )
            {
                csai.TickEvent -= new CSAI.TickHandler( Tick );
                csai.UnregisterVoiceCommand( "countscouts" );
                
                enemycontroller.NewEnemyAddedEvent -= new EnemyController.NewEnemyAddedHandler( EnemyAdded );
                
                unitcontroller.UnitAddedEvent -= new UnitController.UnitAddedHandler( UnitAdded );
                unitcontroller.UnitRemovedEvent -= new UnitController.UnitRemovedHandler( UnitRemoved );
                
                searchcoordinator.Disactivate();
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
        
        public void VoiceCommandCountScouts( string cmdline, string[]splitstring, int player )
        {
            aicallback.SendTextMsg( "scouts: " + ScoutUnitDefsById.Count, 0 );
        }
        
        // return priority indicating how badly you need some units
        // they can ask factory what it is able to build
        public double DoYouNeedSomeUnits( IFactory factory )
        {
            //if( numrequestsinqueue == 0 && ScoutUnitDefsById.Count == 0 )
            if( ScoutUnitDefsById.Count < 2 ) // ignore requests in queue for now, since factory isnt transactional
            {
                logfile.WriteLine( "ScoutControllerRaider: signalling need unit priority 2.0" );
                return 2.0;
            }
            return 0.0;
        }
            
        // Ask what unit they need building
        public IUnitDef WhatUnitDoYouNeed( IFactory factory )
        {
            IUnitDef unitdef = null;
            if( factory.Name == "armvp" )
            {
                unitdef = buildtable.UnitDefByName[ "armfav" ] as IUnitDef;
            }
            else if( factory.Name == "armlab" )
            {
                if( aicallback.GetModName().ToLower().IndexOf( "xta" ) == 0 )
                {
                    unitdef = buildtable.UnitDefByName[ "armpw" ] as IUnitDef;
                }
                else
                {
                    unitdef = buildtable.UnitDefByName[ "armflea" ] as IUnitDef;
                }
            }
            else if( factory.Name == "armap" )
            {
                unitdef = buildtable.UnitDefByName[ "armpeep" ] as IUnitDef;
            }
            logfile.WriteLine( "ScoutController: requesting " + unitdef.humanName );
            return unitdef;
        }

        // signal to requester that we granted its request
        public void WeAreBuildingYouA( IUnitDef unitwearebuilding )
        {
            //numrequestsinqueue++;
        }
            
        // your request got destroyed; new what-to-build negotations round
        public void YourRequestWasDestroyedDuringBuilding_Sorry( IUnitDef unitwearebuilding )
        {
            //numrequestsinqueue--;
        }
        
        // later on, we'll get the factory to assign ownership to whoever requested it
        // for now we use unitcontroller
        public void UnitAdded( int unitid, IUnitDef unitdef )
        {
            if( unitdef.name.ToLower() == "armfav" || unitdef.name.ToLower() == "armflea" || unitdef.name.ToLower() == "armpeep"  ||
                   unitdef.name.ToLower() == "armpw" ) 
            {
                logfile.WriteLine( "ScoutControllerRaider: new scout added " + unitid );
                ScoutUnitDefsById.Add( unitid, unitdef );
                //ExploreWith( unitid );
            }
        }
        
        public void UnitRemoved( int unitid )
        {
            if( ScoutUnitDefsById.Contains( unitid ) )
            {
                logfile.WriteLine( "ScoutController: scout removed " + unitid );
                ScoutUnitDefsById.Remove( unitid );
            }
        }
        
        void ExploreWith( int unitid )
        {
            Float3 destination = new Float3();
            if( PriorityTargets.Count > 0 )
            {            
                destination = PriorityTargets.Dequeue() as Float3;
                logfile.WriteLine( "dequeued next destination: " + destination.ToString() );
            }
            else
            {
                destination.x = random.Next(0, aicallback.GetMapWidth() * MovementMaps.SQUARE_SIZE );
                destination.z = random.Next( 0, aicallback.GetMapHeight() * MovementMaps.SQUARE_SIZE );
                destination.y = aicallback.GetElevation( destination.x, destination.y );
                logfile.WriteLine( "mapwidth: " + aicallback.GetMapWidth() + " squaresize: " + MovementMaps.SQUARE_SIZE );
                logfile.WriteLine( "ScoutController sending scout " + unitid + " to " + destination.ToString() );
            }
            aicallback.GiveOrder( unitid, new Command( Command.CMD_MOVE, destination.ToDoubleArray() ) );
        }
        
        public void EnemyAdded( int id, IUnitDef unitdef )
        {
            Float3 enemypos = aicallback.GetUnitPos( id );
            for( int i = 0; i < 3; i++ )
            {
                Float3 randomoffset = new Float3( random.Next( -enemysightedsearchradius, enemysightedsearchradius ), 0, random.Next( -enemysightedsearchradius, enemysightedsearchradius ) );
                if( csai.DebugOn )
                {
                    aicallback.DrawUnit("ARMARAD", enemypos + randomoffset, 0.0f, 500, aicallback.GetMyAllyTeam(), true, true);
                }
                PriorityTargets.Enqueue( enemypos + randomoffset );
            }
        }
        
        void Reappraise()
        {
            foreach( DictionaryEntry scoutde in ScoutUnitDefsById )
            {
                int scoutdeployedid = (int)scoutde.Key;
                Float3 scoutpos = aicallback.GetUnitPos( scoutdeployedid );
                
                Float3 nearestpos = null;
                double bestsquareddistance = 100000000;
                int targetid = 0;
                // need to add index by position for this, to speed things up
                foreach( DictionaryEntry de in EnemyController.GetInstance().EnemyUnitDefByDeployedId )
                {
                    int deployedid = (int)de.Key;
                    IUnitDef unitdef = de.Value as IUnitDef;
                    if( unitdef != null )
                    {
                        Float3 thispos = aicallback.GetUnitPos( deployedid );
                        if( IsPriorityTarget( unitdef ) )
                        {
                            double thissquareddistance = Float3Helper.GetSquaredDistance( scoutpos, thispos );
                            if( thissquareddistance < bestsquareddistance )
                            {
                                bool nolasertowersnear = true;
                                foreach( DictionaryEntry attackerde in EnemyController.GetInstance().EnemyUnitDefByDeployedId )
                                {
                                    int attackerid = (int)attackerde.Key;
                                    IUnitDef attackerunitdef = attackerde.Value as IUnitDef;
                                    if( attackerunitdef != null )
                                    {
                                        if( IsLaserTower( attackerunitdef ) )
                                        {
                                            Float3 attackerpos = aicallback.GetUnitPos( attackerid );
                                            if( Float3Helper.GetSquaredDistance( attackerpos, thispos ) < nearbyforenemiesmeans * nearbyforenemiesmeans )
                                            {
                                                nolasertowersnear = false;
                                            }
                                        }
                                    }
                                }
                                if( nolasertowersnear )
                                {
                                    nearestpos = thispos;
                                    bestsquareddistance = thissquareddistance;
                                    targetid = deployedid;
                                }
                            }
                        }
                    }
                }
                if( nearestpos != null )
                {
                    aicallback.GiveOrder( scoutdeployedid, new Command( Command.CMD_ATTACK, new double[]{ targetid } ) );
                    if( !attackingscouts.Contains( scoutdeployedid ) )
                    {
                        attackingscouts.Add( scoutdeployedid );
                    }
                }
                else
                {
                    if( attackingscouts.Contains( scoutdeployedid ) )
                    {
                        ExploreWith( scoutdeployedid );
                        attackingscouts.Remove( scoutdeployedid );
                    }
                }
            }
        }
        
        bool IsPriorityTarget(IUnitDef unitdef )
        {
            return unitdef.name.ToLower() == "armmex" || unitdef.name.ToLower() == "cormex" || unitdef.name.ToLower() == "corrad" ||
                unitdef.name.ToLower() == "corrad";
        }
        
        bool IsLaserTower(IUnitDef unitdef )
        {
            if( unitdef.name.ToLower() == "armllt" ||  unitdef.name.ToLower() == "corllt" ||
                unitdef.name.ToLower() == "armfrt" ||  unitdef.name.ToLower() == "corfrt" )
            {
                return true;
            }
            return false;
        }
        
        ArrayList attackingscouts = new ArrayList(); // scoutid of scouts currently attacking
        
        int ticks = 0;
        public void Tick()
        {
           // logfile.WriteLine( "ScoutControllerRaider Tick" );
            ticks++;
            if( ticks == 15 )
            {
                Reappraise();
                ticks = 0;
            }
        }
        
        //public void UnitIdle( int unitid )
        //{
          //  if( ScoutUnitDefsById.Contains( unitid ) )
            //{
              //  ExploreWith( unitid );
            //}
        //}
    }
}
