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
    // This is in charge of constructors
    class ConstructorController : IUnitRequester, IController, IFactoryController
    {
        int optimalnumberconstructors = 2;
        int nudgesize = 50;
        int maxreclaimradius = 500;
        int reclaimradiusperonehundredmetal = 150;
        
        //static ConstructorController instance = new ConstructorController();
        //public static ConstructorController GetInstance(){ return instance; }
        
        IPlayStyle playstyle;
    
        UnitDefHashtable UnitDefByUnitId = new UnitDefHashtable();
        ArrayList Requesters = new ArrayList();
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        UnitController unitcontroller;
        BuildTable buildtable;
        Metal metal;

        FactoryController factorycontroller = null;
        IRadarController radarcontroller;
        
        Random random = new Random();
    
        public ConstructorController( IPlayStyle playstyle )
        {
            this.playstyle = playstyle;
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            ShowNextBuildSite = csai.DebugOn;
            
            unitcontroller = UnitController.GetInstance();
            // factorycontroller = FactoryController.GetInstance();
            buildtable = BuildTable.GetInstance();
            metal = Metal.GetInstance();

            ShowNextBuildSite = csai.DebugOn;
            logfile.WriteLine("ConstructorController::ConstructorController() finished" );
        }
        
        public void RegisterRequester( IUnitRequester requester )
        {
            if( !Requesters.Contains( requester ) )
            {
                logfile.WriteLine( "FactoryController: adding requester: " + requester.ToString() );
                Requesters.Add( requester );
            }
        }
        
        bool Active = false;
        
        public void Activate()
        {
            if( !Active )
            {
                logfile.WriteLine( "Activating Constructorcontroller" );
                
                foreach( IFactoryController factorycontroller in playstyle.GetControllersOfType( typeof( IFactoryController ) ) )
                {
                    factorycontroller.RegisterRequester( this );
                }
                
                radarcontroller = playstyle.GetFirstControllerOfType( typeof( IRadarController ) ) as IRadarController;
                
                csai.UnitIdleEvent += new CSAI.UnitIdleHandler( UnitIdle );
                csai.TickEvent += new CSAI.TickHandler( Tick );
                csai.UnitMoveFailedEvent += new CSAI.UnitMoveFailedHandler( UnitMoveFailed );
                
                csai.RegisterVoiceCommand( "constructorshownextbuildsiteon", new CSAI.VoiceCommandHandler( VoiceCommandShowNextBuildSiteOn ) );
                csai.RegisterVoiceCommand( "constructorshownextbuildsiteoff", new CSAI.VoiceCommandHandler( VoiceCommandShowNextBuildSiteOff ) );
                
                unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
                unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
                UnitDefByUnitId.Clear(); // note, dont just make a new one, because searchpackcoordinator wont get new one
                unitcontroller.RefreshMyMemory( new UnitController.UnitAddedHandler( UnitAdded ) );
                
                CheckIdleUnits();
                
                logfile.WriteLine( "Constructorcontroller activated" );
                Active = true;
            }
        }
        
        
        
        void CheckIdleUnits()
        {
            logfile.WriteLine( "constructor controller Checking idle units" );
            foreach( int unitid in UnitDefByUnitId.GetUnitIds() )
            {
                if( !aicallback.UnitIsBusy( unitid ) )
                {
                    logfile.WriteLine( "Constructor " + unitid + " detected as idle, putting to work" );
                    BuildSomething( unitid );
                }
            }
        }

        public void Disactivate()
        {
            if( Active )
            {
                csai.UnitIdleEvent -= new CSAI.UnitIdleHandler( UnitIdle );
                csai.TickEvent -= new CSAI.TickHandler( Tick );
                csai.UnitMoveFailedEvent -= new CSAI.UnitMoveFailedHandler( UnitMoveFailed );
                
                csai.UnregisterVoiceCommand( "constructorshownextbuildsiteon" );
                csai.UnregisterVoiceCommand( "constructorshownextbuildsiteoff" );
                
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
        
        bool ShowNextBuildSite = false;
        
        public void VoiceCommandShowNextBuildSiteOn( string chatstring, string[] splitchatstring, int player )
        {
            ShowNextBuildSite = true;
        }
        
        public void VoiceCommandShowNextBuildSiteOff( string chatstring, string[] splitchatstring, int player )
        {
            ShowNextBuildSite = false;
        }
        
        public void VoiceCommandListConstructors( string chatstring, string[] splitchatstring, int player )
        {
        }
        
        public void  VoiceCommandMoveConstructor( string chatstring, string[] splitchatstring, int player )
        {
        }
        
        public void VoiceCommandBuild( string chatstring, string[] splitchatstring, int player )
        {
        }
        
        void DoSomething( int constructorid )
        {
             BuildSomething( constructorid );
        }
        
        bool DoReclaim( int constructorid )
        {
            if( totalticks == 0 )// check ticks first, beacuse metal shows as zero at start
            {
                return false;
            }
            Float3 mypos = aicallback.GetUnitPos( constructorid );
            //double nearestreclaimdistancesquared = 1000000;
            //Float3 nearestreclaimpos = null;
            double bestmetaldistanceratio = 0;
            int bestreclaimid = 0;
            int metalspace = (int)( aicallback.GetMetalStorage() - aicallback.GetMetal() );
            logfile.WriteLine( "available space in metal storage: " + metalspace );
            int[] nearbyfeatures = aicallback.GetFeatures( mypos, maxreclaimradius );
            bool reclaimfound = false;
            foreach( int feature in nearbyfeatures )
            {
                IFeatureDef featuredef = aicallback.GetFeatureDef( feature );
                if( featuredef.metal > 0 && featuredef.metal <= metalspace )
                {
                    Float3 thisfeaturepos = aicallback.GetFeaturePos( feature );
                    double thisdistance = Math.Sqrt( Float3Helper.GetSquaredDistance( thisfeaturepos, mypos ) );
                    double thismetaldistanceratio = featuredef.metal / thisdistance;
                    if( thismetaldistanceratio > bestmetaldistanceratio )
                    {
                        logfile.WriteLine( "Potential reclaim, distance = " + thisdistance + " metal = " + featuredef.metal + " ratio = " + thismetaldistanceratio );
                        bestmetaldistanceratio = thismetaldistanceratio;
                        bestreclaimid = feature;
               //         nearestreclaimpo
                        reclaimfound = true;
                    }
                }
            }
            if( reclaimfound && ( bestmetaldistanceratio > ( 1.0 / ( 100 * reclaimradiusperonehundredmetal ) ) ) )
            {
                Float3 reclaimpos = aicallback.GetFeaturePos( bestreclaimid );
                logfile.WriteLine( "Reclaim found, pos " + reclaimpos.ToString() );
                if( csai.DebugOn )
                {
                	aicallback.DrawUnit( BuildTable.ArmMex.ToUpper(), reclaimpos, 0.0f, 200, aicallback.GetMyAllyTeam(), true, true);
                }
                aicallback.GiveOrder( constructorid, new Command( Command.CMD_RECLAIM, 
                    new double[]{ reclaimpos.x, reclaimpos.y, reclaimpos.z, 10 } ) );
            }
            else
            {
                logfile.WriteLine( "No reclaim within parameters" );
            }
            return reclaimfound;
        }
        
        // note to self: all this should be added to some sort of requester architecture, cf FactoryController
        // question to self: factorycontroller will be a requester in this architecture???
        // question to self: use same architecture as for factorycontroller or create separate architecture?
        void BuildSomething( int constructorid )
        {
            
            logfile.WriteLine( "ConstructorController.BuildSomething()" );
            IUnitDef radardef = buildtable.UnitDefByName[ BuildTable.ArmL1Radar ] as IUnitDef;
            
            if( aicallback.GetEnergy() * 100 / aicallback.GetEnergyStorage() < 10 )
            {
                logfile.WriteLine( "constructor building energy" );
                BuildPower( constructorid );
            }
            else if( aicallback.GetMetal() * 100 / aicallback.GetMetalStorage() < 50 )
            {
                logfile.WriteLine( "constructor building metal" );
                if( !DoReclaim( constructorid ) )
                {
                    logfile.WriteLine( "constructor building extractor" );
                    BuildExtractor( constructorid );
                }
            }
            //else if( 
                    
            // only build radar if we have at least one factory already
            //else if( factorycontroller.FactoryUnitDefByDeployedId.Count > 0 &&
            //    EnergyController.GetInstance().CanBuild( radardef ) &&
            //    radarcontroller.NeedRadarAt( aicallback.GetUnitPos( constructorid ) ) )
           // {
            //    BuildRadar( constructorid );
            //}
            else 
            {
                double highestpriority = 0;
                IUnitRequester requestertosatisfy = null;
                IUnitDef constructordef = UnitDefByUnitId[ constructorid ] as IUnitDef;
                logfile.WriteLine( "ConstructorController asking requesters" );
                foreach( object requesterobject in Requesters )
                {
                    IUnitRequester requester = requesterobject as IUnitRequester;
                    logfile.WriteLine( "ConstructorController asking " + requester.ToString() );
                    double thisrequesterpriority = requester.DoYouNeedSomeUnits( new Factory( constructorid, constructordef ) );
                    if( thisrequesterpriority > 0 && thisrequesterpriority > highestpriority )
                    {
                        requestertosatisfy = requester;
                        highestpriority = thisrequesterpriority;
                        logfile.WriteLine( "ConstructorController: Potential requester to use " + requestertosatisfy.ToString() + " priority: " + highestpriority );
                    }
                }
                
                if( requestertosatisfy != null )
                {
                    IUnitDef unitdef = requestertosatisfy.WhatUnitDoYouNeed( new Factory( constructorid, constructordef ) );
                    logfile.WriteLine( "constructorcontroller constructor " + constructorid + " going with request from " + requestertosatisfy.ToString() + " for " + unitdef.humanName );
                    requestertosatisfy.WeAreBuildingYouA( unitdef );                
                    if( MetalController.GetInstance().CanBuild( unitdef ) )
                    {
                        if( EnergyController.GetInstance().CanBuild( unitdef ) )
                        {
                            Build( constructorid, unitdef );
                        }
                        else
                        {
                            BuildPower( constructorid );
                        }
                    }
                    else
                    {
                        if( !DoReclaim( constructorid ) )
                        {
                            BuildExtractor( constructorid );
                        }
                    }
                }
            }
        }
        
        void  BuildPower( int constructorid )
        {
            IUnitDef unitdef = buildtable.UnitDefByName[ BuildTable.ArmSolar ] as IUnitDef;
            Build( constructorid, unitdef );
        }
        
        void  BuildFactory( int constructorid )
        {
            IUnitDef unitdef = buildtable.UnitDefByName[ BuildTable.ArmVehicleFactory ] as IUnitDef;
            Build( constructorid, unitdef );
        }
        
        void  BuildRadar( int constructorid )
        {
            IUnitDef unitdef = buildtable.UnitDefByName[ BuildTable.ArmL1Radar ] as IUnitDef;
            Build( constructorid, unitdef );
        }
        
        void  BuildExtractor( int constructorid )
        {
            IUnitDef unitdef = buildtable.UnitDefByName[ BuildTable.ArmMex ] as IUnitDef;;
            Float3 constructorpos = aicallback.GetUnitPos( constructorid );
            Float3 nearestmetalpos = metal.GetNearestMetalSpot( constructorpos );
            logfile.WriteLine( "BuildExtractor nearestmetalpos is null? " + ( nearestmetalpos == null ).ToString() );
            Float3 buildsite = aicallback.ClosestBuildSite( unitdef, nearestmetalpos, 1400.0, 2 );        
            BuildAt( constructorid, unitdef, buildsite );
        }
        
        void Build( int constructorid, IUnitDef unitdef )
        {
            Float3 constructorpos = aicallback.GetUnitPos( constructorid );
            BuildAt( constructorid, unitdef, constructorpos );
        }
        
        void BuildAt( int constructorid, IUnitDef unitdef, Float3 buildsite )
        {
            IUnitDef placeholder;
            /*
            if( unitdef.name.ToLower() == "armmex" ) // use smaller placeholder for mexes so fit closer to intended metalspot
            {
                placeholder = buildtable.UnitDefByName[ "ARMMOHO".ToLower() ] as IUnitDef;
            }
            else
            {
                placeholder = buildtable.UnitDefByName[ "CORGANT".ToLower() ] as IUnitDef;
            }
            buildsite = aicallback.ClosestBuildSite( placeholder, buildsite, 1400.0, 2 );            
            buildsite = aicallback.ClosestBuildSite( unitdef, buildsite, 1400.0, 2 );            
            */
            if( unitdef.name == BuildTable.ArmMex )
            {
                placeholder = buildtable.UnitDefByName[ BuildTable.ArmMoho ] as IUnitDef;
                buildsite = aicallback.ClosestBuildSite( placeholder, buildsite, 1400.0, 2 );            
                buildsite = aicallback.ClosestBuildSite( unitdef, buildsite, 1400.0, 2 );            
            }
            else
            {
                buildsite = BuildPlanner.GetInstance().ClosestBuildSite( unitdef, buildsite, 3000 );
                buildsite.y = aicallback.GetElevation( buildsite.x, buildsite.z );
                buildsite = aicallback.ClosestBuildSite( unitdef, buildsite, 1400, 2 );            
            }
            
            logfile.WriteLine( "building " + unitdef.name + " at " + buildsite.ToString() );
            if( ShowNextBuildSite )
            {
                aicallback.DrawUnit(unitdef.name, buildsite, 0.0f, 500, aicallback.GetMyAllyTeam(), true, true);
            }
            aicallback.GiveOrder( constructorid, new Command( - unitdef.id, buildsite.ToDoubleArray() ) );
        }
        
        // return priority indicating how badly you need some units
        // they can ask factory what it is able to build
        public double DoYouNeedSomeUnits( IFactory factory )
        {
            if( UnitDefByUnitId.Count < optimalnumberconstructors && ( 
                 factory.Name == BuildTable.ArmVehicleFactory || factory.Name == "armlab" ) )
            {
                logfile.WriteLine( "ConstructorController: signalling need unit priority 1" );
                return 0.8;
            }
            else
            {
                return 0.0;
            }
        }
            
        // note to self: this should be adjusted according to enemy units, factory etc...
        public IUnitDef WhatUnitDoYouNeed( IFactory factory )
        {
            IUnitDef unitdef = null;

            if( factory.Name == BuildTable.ArmVehicleFactory )
            {
                unitdef = buildtable.UnitDefByName[ BuildTable.ArmConstructionVehicle ] as IUnitDef;
            }
//            else if( factory.Name == "armlab" )
//            {
//                unitdef = buildtable.UnitDefByName[ "armck" ] as IUnitDef;
//            }
//            else
//            {
//                unitdef = buildtable.UnitDefByName[ "armca" ] as IUnitDef;
//            }
            logfile.WriteLine( "ConstructorController requesting " + unitdef.humanName );
            
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
        
        // if stuck or something
        void NudgeUnit( int unitid )
        {
            Float3 currentpos = aicallback.GetUnitPos( unitid );
            Float3 nudge = new Float3( random.Next( - nudgesize, nudgesize ), 0, random.Next( - nudgesize, nudgesize ) );
            aicallback.GiveOrder( unitid, new Command( Command.CMD_MOVE, ( nudge + currentpos ).ToDoubleArray() ) );
        }
        
        public void UnitAdded( int deployedunitid, IUnitDef unitdef )
        {
            if( unitdef.name.ToLower() == BuildTable.ArmConstructionVehicle
        	   || unitdef.name.ToLower() == "armck" || unitdef.name.ToLower() == "armca" 
        	   || unitdef.isCommander )
            {
                if( !UnitDefByUnitId.Contains( deployedunitid ) )
                {
                    UnitDefByUnitId.Add( deployedunitid, unitdef );
                    logfile.WriteLine( "Constructor controller; new constructor: " + unitdef.humanName );
                    DoSomething( deployedunitid );
                }
            }
            if( unitdef.isCommander && unitdef.name.ToLower() != BuildTable.ArmCommander )
            {
                aicallback.SendTextMsg( "Warning: please make sure the AI is running as ARM", 0 );
                aicallback.SendTextMsg( "CSAI will not function correctly if not running as ARM", 0 );
                logfile.WriteLine( "************************************************" );
                logfile.WriteLine( "************************************************" );
                logfile.WriteLine( "**** Warning: please make sure the AI is running as ARM ****" );
                logfile.WriteLine( "**** CSAI will not function correctly if not running as ARM ****" );
                logfile.WriteLine( "************************************************" );
                logfile.WriteLine( "************************************************" );
            }
        }
        
        public void UnitRemoved( int deployedunitid )
        {
            if( UnitDefByUnitId.Contains( deployedunitid ) )
            {
                logfile.WriteLine( "Constructor controller: constructor destroyed " + deployedunitid );
                UnitDefByUnitId.Remove( deployedunitid );
            }
        }
        
        public void UnitIdle( int unitid )
        {
            if( UnitDefByUnitId.Contains( unitid ) )
            {
                logfile.WriteLine( "Constructor " + unitid + " detected as idle, putting to work" );
                DoSomething( unitid );
            }            
        }
        
        int totalticks = 0;
        int lastnudge = 0;
        public void UnitMoveFailed( int unitid )
        {
            if( UnitDefByUnitId.Contains( unitid ) && totalticks - lastnudge > 10 )
            {
                logfile.WriteLine( "Constructor " + unitid + " detected as unitmovefailed, moving randomly" );
                lastnudge = totalticks;
                NudgeUnit( unitid );
            }            
        }
        
        public void Tick()
        {
            totalticks++;
        }
    }    
}
