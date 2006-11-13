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
    // manages player-owned factories
    public class FactoryController : IController, IFactoryController, IUnitRequester
    {
        ArrayList Requesters = new ArrayList();
        
        //static FactoryController instance = new FactoryController();
        //public static FactoryController GetInstance(){ return instance; }
        
        IPlayStyle playstyle;
    
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        BuildTable buildtable;
        UnitController unitcontroller;
        
        Random random = new Random();
        
        //ArrayList requests = new ArrayList();
        
        public class Request
        {
            //public int Unittypeid;
            public int Quantity;
            public double Priority;
            public IUnitDef UnitDef;
            public Request(){}
            public Request( IUnitDef unitdef, int quantity, double priority )
            {
                this.UnitDef = unitdef;
                this.Quantity = quantity;
                this.Priority = priority;
            }
        }
        
        UnitDefHelp unitdefhelp;
        
        public Hashtable FactoryUnitDefByDeployedId = new Hashtable();
        public Hashtable FactoriesByTypeName = new Hashtable(); // deployedid of factories hashed by typename (eg "armvp")
        
        public FactoryController( IPlayStyle playstyle )
        {
            this.playstyle = playstyle;
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            buildtable = BuildTable.GetInstance();
            unitcontroller = UnitController.GetInstance();
            
            unitdefhelp = new UnitDefHelp( aicallback );

            csai.UnitFinishedEvent += new CSAI.UnitFinishedHandler(csai_UnitFinishedEvent);
            csai.UnitDestroyedEvent += new CSAI.UnitDestroyedHandler(csai_UnitDestroyedEvent);
            
            csai.RegisterVoiceCommand( "dumpfactories", new CSAI.VoiceCommandHandler( DumpFactories ) );
        }

        void csai_UnitDestroyedEvent(int deployedunitid, int enemyid)
        {
            List<UnitInCreation> requeststoremove = new List<UnitInCreation>();
            foreach (UnitInCreation unitincreation in unitsincreation)
            {
                if (unitincreation.deployedid == deployedunitid)
                {
                    unitincreation.unitrequester.YourRequestWasDestroyedDuringBuilding_Sorry(unitdef);
                    requeststoremove.Add(unitincreation);
                }
            }
            foreach (UnitInCreation unitincreation in requeststoremove)
            {
                unitsincreation.Remove(unitsincreation);
            }
        }

        void csai_UnitFinishedEvent(int deployedunitid, IUnitDef unitdef)
        {
            List<UnitInCreation> requeststoremove = new List<UnitInCreation>();
            foreach (UnitInCreation unitincreation in unitsincreation)
            {
                if (unitincreation.deployedid == deployedunitid)
                {
                    unitincreation.unitrequester.UnitFinished(deployedunitid, unitdef);
                    requeststoremove.Add(unitincreation);
                }
            }
            foreach (UnitInCreation unitincreation in requeststoremove)
            {
                unitsincreation.Remove(unitsincreation);
            }
        }
        
        bool Active = false;
        
        public void DumpFactories( string cmd, string[] splitcmd, int player )
        {
            logfile.WriteLine( "dumping factories..." );
            foreach( DictionaryEntry de in FactoriesByTypeName )
            {
                string name = (string)de.Key;
                ArrayList al = de.Value as ArrayList;
                logfile.WriteLine( name + ": " + al.Count );
            }
        }
        
        public void Activate()
        {
            if( !Active )
            {
                csai.UnitIdleEvent += new CSAI.UnitIdleHandler( UnitIdle );
                
                foreach( IFactoryController factorycontroller in playstyle.GetControllersOfType( typeof( IFactoryController ) ) )
                {
                    factorycontroller.RegisterRequester( this );
                }
                
               // unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
                unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
                FactoryUnitDefByDeployedId.Clear();
                unitcontroller.RefreshMyMemory( new UnitController.UnitAddedHandler( UnitAdded ) );

                /*
                foreach( DictionaryEntry de in unitcontroller.UnitDefByDeployedId )
                {
                    int deployedid = (int)de.Key;
                    IUnitDef unitdef = de.Value as IUnitDef;
                    if( unitdefhelp.IsFactory( unitdef ) && !FactoryUnitDefByDeployedId.Contains( deployedid ) )
                    {
                        FactoryUnitDefByDeployedId.Add( deployedid, unitdef );
                        logfile.WriteLine( "Existing factory: " + unitdef.humanName + " id " + deployedid );
                    }
                }
                */
                
                Active = true;
            }
        }
        
        public void Disactivate()
        {
            if( Active )
            {
                csai.UnitIdleEvent -= new CSAI.UnitIdleHandler( UnitIdle );
                
                //unitcontroller.UnitAddedEvent -= new UnitController.UnitAddedHandler( UnitAdded );
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
        
        public void RegisterRequester( IUnitRequester requester )
        {
            if( !Requesters.Contains( requester ) )
            {
                logfile.WriteLine( "FactoryController: adding requester: " + requester.ToString() );
                Requesters.Add( requester );
            }
        }
        
        // return priority indicating how badly you need some units
        // they can ask factory what it is able to build
        public double DoYouNeedSomeUnits( IFactory factory )
        {
            if( factory.Name == "armcom" || factory.Name == "armcv" || factory.Name == "armck" )
            {
                if( FactoryUnitDefByDeployedId.Count < 1 )
                {
                    return 1.0;
                }
                return 0.5;  // just return a medium priority for now
            }
            else
            {
                return 0.0;
            }
        }
            
        // note to self: this should be adjusted according to enemy units, factory etc...
        public IUnitDef WhatUnitDoYouNeed( IFactory factory )
        {
            IUnitDef factorydef = null;

            if( FactoryUnitDefByDeployedId.Count > 8 )
            {
                if( !FactoriesByTypeName.Contains( "armap" ) )
                {
                    factorydef = buildtable.UnitDefByName[ "armap" ] as IUnitDef;  // airports tricky to manage for now, so we only build one, and only 
                                                                                                        // if we have lots of other factories
                }
                if( !FactoriesByTypeName.Contains( "armhp" ) )
                {
                    factorydef = buildtable.UnitDefByName[ "armhp" ] as IUnitDef;
                }
                //else if( !FactoriesByTypeName.Contains( "armavp" ) )
                else
                {
                    factorydef = buildtable.UnitDefByName[ "armavp" ] as IUnitDef;
                }
            }
            else
            {
                int randomnumber = random.Next( 1, 3 ); // 1 or 2            
                if( random.Next( 1, 4 ) == 1 || FactoryUnitDefByDeployedId.Count <= 2 )
                {
                    factorydef = buildtable.UnitDefByName[ "armvp" ] as IUnitDef;
                }
                //else if( random.Next( 1, 4 ) == 2 )
                //{
                  //  factorydef = buildtable.UnitDefByName[ "armap" ] as IUnitDef;  // airports tricky to manage for now
                //}
                else
                {
                    factorydef = buildtable.UnitDefByName[ "armlab" ] as IUnitDef;
                }
            }

            logfile.WriteLine( "FactoryController requesting " + factorydef.humanName );
            
            return factorydef;
        }

        // signal to requester that we granted its request
        public void WeAreBuildingYouA( IUnitDef unitwearebuilding )
        {
        }
            
        // your request got destroyed; new what-to-build negotations round
        public void YourRequestWasDestroyedDuringBuilding_Sorry( IUnitDef unitwearebuilding )
        {
        }
        
        public void UnitFinished( int unitid, IUnitDef unitdef )
        {
            FactoryUnitDefByDeployedId.Add( unitid, unitdef );
            logfile.WriteLine( "Factory added: " + unitdef.humanName + " id " + unitid );
            if( !FactoriesByTypeName.Contains( unitdef.name.ToLower() ) )
            {
                FactoriesByTypeName.Add( unitdef.name.ToLower(), new ArrayList() );
            }
            ArrayList unitypefactories = FactoriesByTypeName[ unitdef.name.ToLower() ] as ArrayList;
            if( !unitypefactories.Contains( unitid ) )
            {
                unitypefactories.Add( unitid );
            }
            // aicallback.SendTextMsg( "New factory, id " + unitid, 0 );
            BuildSomething( unitid );
        }
        
        public void UnitRemoved( int unitid )
        {
            if( FactoryUnitDefByDeployedId.Contains( unitid ) )
            {
                FactoryUnitDefByDeployedId.Remove( unitid );
            }
        }        
        
        void BuildSomething( int factoryid )
        {
            double highestpriority = 0;
            IUnitRequester requestertosatisfy = null;
            IUnitDef factorydef = FactoryUnitDefByDeployedId[ factoryid ] as IUnitDef;
            foreach( object requesterobject in Requesters )
            {
                IUnitRequester requester = requesterobject as IUnitRequester;
                double thisrequesterpriority = requester.DoYouNeedSomeUnits( new Factory( factoryid, factorydef ) );
                if( thisrequesterpriority > 0 && thisrequesterpriority > highestpriority )
                {
                    requestertosatisfy = requester;
                    highestpriority = thisrequesterpriority;
                    logfile.WriteLine( "Potential requester to use " + requestertosatisfy.ToString() + " priority: " + highestpriority );
                }
            }
            
            if( requestertosatisfy != null )
            {
                IUnitDef unitdef = requestertosatisfy.WhatUnitDoYouNeed( new Factory( factoryid, factorydef ) );
                logfile.WriteLine( "factorycontroller factory " + factoryid + " going with request from " + requestertosatisfy.ToString() + " for " + unitdef.humanName );
                requestertosatisfy.WeAreBuildingYouA( unitdef );
                Ownership.GetInstance().RegisterBuildingOrder(this, unitdef, aicallback.GetUnitPos(factoryid));
                aicallback.GiveOrder( factoryid, new Command( - unitdef.id ) );                
            }
        }
        
        public void UnitIdle( int unitid )
        {
            if( FactoryUnitDefByDeployedId.Contains( unitid ) )
            {
                logfile.WriteLine( "Factory " + unitid.ToString() + " on idle" );
                BuildSomething( unitid );
            }
        }

        class UnitInCreation
        {
            public IUnitRequester unitrequester;
            public IUnitDef unitdef;
            public Float3 pos;
            public IUnitOrder unitorder;
            public int deployedid;
            public UnitInCreation(){}
            public UnitInCreation(IUnitRequester requester, IUnitDef unitdef, Float3 pos)
            {
                this.unitrequester = requester;
                this.unitdef = unitdef;
                this.pos = pos;
            }
        }

        List<UnitInCreation> unitsincreation = new List<UnitInCreation>();

        void IFactoryController.UnitCreated(IUnitOrder order, int deployedid)
        {
            foreach (UnitInCreation unitincreation in unitsincreation)
            {
                if (unitincreation.unitorder == order)
                {
                    unitincreation.deployedid = deployedid;
                }
            }
        }
    }
}
