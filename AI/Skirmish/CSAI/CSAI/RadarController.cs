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
    // manages radar
    // manages our radar coverage, recommends new radar positions
    public class RadarController : IController, IRadarController, IUnitRequester
    {
        Hashtable Radars = new Hashtable();
        
        int minradardistance = 1000;
        
        //static RadarController instance = new RadarController();
        //public static RadarController GetInstance(){ return instance; }
    
        IPlayStyle playstyle;
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        UnitController unitcontroller;
        BuildTable buildtable;
        
        public class Radar
        {
            public Float3 Pos;
            public double radius;
            public IUnitDef unitdef;
            public Radar(){}
            public Radar( Float3 pos, double radius, IUnitDef unitdef )
            {
                this.Pos = pos;
                this.radius = radius;
                this.unitdef = unitdef;
            }
        }
        
        public RadarController( IPlayStyle playstyle )
        {
            this.playstyle = playstyle;
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            unitcontroller = UnitController.GetInstance();
            buildtable = BuildTable.GetInstance();
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
                
                unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
                unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
                Radars.Clear(); // note, dont just make a new one, because searchpackcoordinator wont get new one
                unitcontroller.RefreshMyMemory( new UnitController.UnitAddedHandler( UnitAdded ) );
                Active = true;
            }
        }
        
        public void Disactivate()
        {
            if( Active )
            {
                unitcontroller.UnitAddedEvent -= new UnitController.UnitAddedHandler( UnitAdded );
                unitcontroller.UnitRemovedEvent -= new UnitController.UnitRemovedHandler( UnitRemoved );
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
        
        // return priority indicating how badly you need some units
        // they can ask factory what it is able to build
        public double DoYouNeedSomeUnits( IFactory factory )
        {
            if( factory.Name != "armcom" && factory.Name != "armcv" )
            {
                return 0.0;
            }
            if( NeedRadarAt( factory.Pos ) )
            {
                return 0.8;
            }
            else
            {
                return 0.0;
            }
        }
            
        // Ask what unit they need building
        public IUnitDef WhatUnitDoYouNeed( IFactory factory )
        {
            IUnitDef unitdef = null;
            unitdef = buildtable.UnitDefByName[ "armrad" ] as IUnitDef;
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
        
        public bool NeedRadarAt( Float3 pos )
        {
            // scan through all radars and check if we are within two thirds of its range or not.  If yes we return false (not need radar)
            // if not , we return true (need radar)
            foreach( DictionaryEntry de in Radars )
            {
                Radar radar = de.Value as Radar;
                if( Float3Helper.GetSquaredDistance( radar.Pos, pos ) < ( radar.radius * radar.radius * 4 / 9 ) )
                {
                    return false;
                }
            }
            return true;
        }
        
        public void UnitAdded( int id, IUnitDef def )
        {
            //logfile.WriteLine( "Unit " + def.humanName + " radarradius: " + def.radarRadius );
            if( def.name.ToLower() == "armrad" )
            {
                logfile.WriteLine( "RadarController UnitAdded: " + def.humanName + " id: " + id );
                Radars.Add( id, new Radar( aicallback.GetUnitPos( id ), def.radarRadius, def ) );
            }
        }
        
        public void UnitRemoved( int id )
        {
            if( Radars.Contains( id ) )
            {
                logfile.WriteLine( "RadarController UnitRemoved: " + id );
                Radars.Remove( id );
            }
        }
    }
}
