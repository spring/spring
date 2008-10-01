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
    public class ScoutControllerRandomSearch : IUnitRequester, IController
    {
        //static ScoutControllerRandomSearch instance = new ScoutController();
        //public static ScoutControllerRandomSearch GetInstance(){ return instance; }
    
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        IFactoryController factorycontroller;
        UnitController unitcontroller;
        BuildTable buildtable;
        
        SpreadSearchPackCoordinator searchcoordinator;
        
        Dictionary<int,IUnitDef> ScoutUnitDefsById = new Dictionary<int,IUnitDef>();
        
        int numrequestsinqueue = 0;
        Random random;
        
        IPlayStyle playstyle;
        
        public ScoutControllerRandomSearch( IPlayStyle playstyle )
        {
            random = new Random();
            
            this.playstyle = playstyle;
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            unitcontroller = UnitController.GetInstance();
            buildtable = BuildTable.GetInstance();
            
            searchcoordinator = new SpreadSearchPackCoordinator( ScoutUnitDefsById );
            
            logfile.WriteLine( "*ScoutController initialized*" );
        }
        
        bool Active = false;
        
        public void Activate()
        {
            if( !Active )
            {
                foreach( IFactoryController factorycontroller in playstyle.GetControllersOfType( typeof( IFactoryController ) ) )
                {
                    factorycontroller.RegisterRequester( this );
                }
                
                csai.TickEvent += new CSAI.TickHandler( Tick );
                csai.RegisterVoiceCommand( "countscouts", new CSAI.VoiceCommandHandler( VoiceCommandCountScouts ) );
                
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
            logfile.WriteLine( "factoryname: " + factory.Name );
            if( ScoutUnitDefsById.Count == 0 && ( factory.Name == BuildTable.ArmVehicleFactory || factory.Name == "armlab" ) ) // ignore requests in queue for now, since factory isnt transactional
            {
                logfile.WriteLine( "ScoutControllerRandomSearch: signalling need unit priority 0.8" );
                return 0.8;
            }
            return 0.0;
        }
            
        // Ask what unit they need building
        public IUnitDef WhatUnitDoYouNeed( IFactory factory )
        {
            IUnitDef unitdef = null;
            if( factory.Name == BuildTable.ArmVehicleFactory )
            {
                unitdef = buildtable.UnitDefByName[ BuildTable.ArmGroundScout ] as IUnitDef;
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
            //if( unitdef != null )
            //{
            logfile.WriteLine( "ScoutController: requesting " + unitdef.humanName );
            //}
            return unitdef;
        }

        // signal to requester that we granted its request
        public void WeAreBuildingYouA( IUnitDef unitwearebuilding )
        {
            numrequestsinqueue++;
        }
            
        // your request got destroyed; new what-to-build negotations round
        public void YourRequestWasDestroyedDuringBuilding_Sorry( IUnitDef unitwearebuilding )
        {
            numrequestsinqueue--;
        }
        
        // later on, we'll get the factory to assign ownership to whoever requested it
        // for now we use unitcontroller
        public void UnitAdded( int unitid, IUnitDef unitdef )
        {
            if( unitdef.name.ToLower() == BuildTable.ArmGroundScout || unitdef.name.ToLower() == "armflea" || unitdef.name.ToLower() == "armpeep"  ||
                   unitdef.name.ToLower() == "armpw" ) 
            {
                logfile.WriteLine( "ScoutControllerRandomSearch: new scout added " + unitid );
                ScoutUnitDefsById.Add( unitid, unitdef );
                //ExploreWith( unitid );
            }
        }
        
        public void UnitRemoved( int unitid )
        {
            if( ScoutUnitDefsById.ContainsKey( unitid ) )
            {
                logfile.WriteLine( "ScoutController: scout removed " + unitid );
                ScoutUnitDefsById.Remove( unitid );
            }
        }
        
        public void Tick()
        {
           // logfile.WriteLine( "ScoutControllerRandomSearch Tick" );
        }        
    }
}
