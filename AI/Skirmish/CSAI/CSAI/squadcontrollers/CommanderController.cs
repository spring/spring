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
    // This is in charge of the commander
    // ultimately most of the stuff here should be in a tactics class
    // this is just for bootstrapping the project to a level where it does something useful-looking
    class CommanderController
    {
        static CommanderController instance = new CommanderController();
        public static CommanderController GetInstance(){ return instance; }
    
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        BuildTable buildtable;
        UnitController unitcontroller;
        //FactoryController factorycontroller;
        Metal metal;
        
        public int commanderid;
        bool commanderalive = true;

        Random random;
      //  int BuildOffsetDistance = 25;
        
        // protected constructor to enforce Singleton pattern
        CommanderController()
        {
            random = new Random();
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            unitcontroller = UnitController.GetInstance();
            buildtable = BuildTable.GetInstance();
            metal = Metal.GetInstance();

//            factorycontroller = PlayStyleManager.GetInstance().GetCurrentPlayStyle().GetFirstControllerOfType( typeof( IFactoryController ) ) as FactoryController;
            
            //csai.UnitFinishedEvent += new CSAI.UnitFinishedHandler( UnitFinished );
            //csai.UnitDestroyedEvent += new CSAI.UnitDestroyedHandler( UnitDestroyed );
            csai.UnitIdleEvent += new CSAI.UnitIdleHandler( UnitIdle );
            //csai.UnitDamagedEvent += new CSAI.UnitDamagedHandler( UnitDamaged );
            csai.TickEvent += new CSAI.TickHandler( Tick );
            
            unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
            unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
            
            csai.RegisterVoiceCommand( "commandermove", new CSAI.VoiceCommandHandler( VoiceCommandMoveCommander ) );
            csai.RegisterVoiceCommand( "commandergetpos", new CSAI.VoiceCommandHandler( VoiceCommandCommanderGetPos ) );
            csai.RegisterVoiceCommand( "commanderbuildat", new CSAI.VoiceCommandHandler( VoiceCommandCommanderBuildAt ) );
            csai.RegisterVoiceCommand( "commanderbuild", new CSAI.VoiceCommandHandler( VoiceCommandCommanderBuild ) );
            csai.RegisterVoiceCommand( "commanderbuildpower", new CSAI.VoiceCommandHandler( VoiceCommandCommanderBuildPower ) );
            csai.RegisterVoiceCommand( "commanderbuildextractor", new CSAI.VoiceCommandHandler( VoiceCommandCommanderBuildExtractor ) );
            csai.RegisterVoiceCommand( "commanderisactive", new CSAI.VoiceCommandHandler( VoiceCommandCommanderIsActive ) );
        }
        
        public void  VoiceCommandCommanderIsActive( string chatstring, string[] splitchatstring, int player )
        {
            aicallback.SendTextMsg( "Commander UnitIsBusy: " + aicallback.UnitIsBusy( commanderid ).ToString(), 0 );
        }
        
        public void  VoiceCommandMoveCommander( string chatstring, string[] splitchatstring, int player )
        {
            int targetteam = Convert.ToInt32( splitchatstring[2] );
            if( targetteam == csai.Team )
            {
                double x = Convert.ToDouble( splitchatstring[3] );
                double z = Convert.ToDouble( splitchatstring[4] );
                double y =  aicallback.GetElevation( x, z );
                aicallback.SendTextMsg( "moving commander to " + x + "," + y + "," + z, 0 );
                logfile.WriteLine( "moving commander to " + x + "," + y + "," + z );
                aicallback.GiveOrder( commanderid, new Command( Command.CMD_MOVE, new double[]{ x, y, z } ) );
            }
        }
        /*
        void DoSomething()
        {
            logfile.WriteLine( "CommanderController.DoSomething()" );
            IUnitDef factorydef = buildtable.UnitDefByName[ "armvp" ] as IUnitDef;
            IUnitDef radardef = buildtable.UnitDefByName[ "armrad" ] as IUnitDef;
            
            if( aicallback.GetEnergy() * 100 / aicallback.GetEnergyStorage() < 10 )
            {
                CommanderBuildPower();
            }
            else if( aicallback.GetMetal() * 100 / aicallback.GetMetalStorage() < 50 )
            {
                CommanderBuildExtractor();
            }
            else if( FactoryController.GetInstance().FactoryUnitDefByDeployedId.Count > 0 &&
                PowerController.GetInstance().CanBuild( radardef ) &&
                RadarController.GetInstance().NeedRadarAt( aicallback.GetUnitPos( commanderid ) ) )
            {
                CommanderBuildRadar();
            }
            else 
            {
                if( MetalController.GetInstance().CanBuild( factorydef ) )
                {
                    if( PowerController.GetInstance().CanBuild( factorydef ) )
                    {
                        CommanderBuildFactory();
                    }
                    else
                    {
                        CommanderBuildPower();
                    }
                }
                else
                {
                    CommanderBuildExtractor();
                }
            }
        }
        */
        public void  VoiceCommandCommanderBuildAt( string chatstring, string[] splitchatstring, int player )
        {
            string unitname = splitchatstring[2];
            IUnitDef unitdef = buildtable.UnitDefByName[ unitname.ToLower() ] as IUnitDef;
            if( unitdef == null )
            {
                aicallback.SendTextMsg( "Unit " + unitname + " not found.", 0 );
                return;
            }
            
            Float3 pos = new Float3();
            pos.x = Convert.ToDouble( splitchatstring[3] );
            pos.z = Convert.ToDouble( splitchatstring[4] );
            pos.y =  aicallback.GetElevation( pos.x, pos.z );            
            
            aicallback.SendTextMsg( "giving build order, id " + unitdef.humanName + " at " + pos.ToString(), 0 );
            
            CommanderBuildAt( unitdef, pos );
        }
        
        public void  VoiceCommandCommanderBuild( string chatstring, string[] splitchatstring, int player )
        {
            string unitname = splitchatstring[2];

            IUnitDef unitdef = buildtable.UnitDefByName[ unitname.ToLower() ] as IUnitDef;
            if( unitdef == null )
            {
                aicallback.SendTextMsg( "Unit " + unitname + " not found.", 0 );
                return;
            }
            
            CommanderBuild( unitdef );
                        
            aicallback.SendTextMsg( "building " + unitdef.humanName, 0 );
        }
        
        public void  VoiceCommandCommanderBuildPower( string chatstring, string[] splitchatstring, int player )
        {
            CommanderBuildPower();
        }
        
        public void  VoiceCommandCommanderBuildExtractor( string chatstring, string[] splitchatstring, int player )
        {
            CommanderBuildExtractor();
        }
        
        public void  VoiceCommandCommanderGetPos( string chatstring, string[] splitchatstring, int player )
        {
            aicallback.SendTextMsg( "Commander pos: " + GetCommanderPos().ToString(), 0 );
        }
        
        void CommanderBuild( IUnitDef unitdef )
        {
            Float3 commanderpos = aicallback.GetUnitPos( commanderid );
            IUnitDef corasydef = buildtable.UnitDefByName[ "CORGANT".ToLower() ] as IUnitDef;
            Float3 buildsite = aicallback.ClosestBuildSite( corasydef, commanderpos, 1400.0, 2 );            
            buildsite = aicallback.ClosestBuildSite( unitdef, buildsite, 1400.0, 2 );            
            CommanderBuildAt( unitdef, buildsite );
        }
        
        void CommanderBuildAt( IUnitDef unitdef, Float3 buildsite )
        {
            logfile.WriteLine( "Commander building " + unitdef.name + " at " + buildsite.ToString() );
            aicallback.DrawUnit(unitdef.name, buildsite, 0.0f, 500, aicallback.GetMyAllyTeam(), true, true);
            aicallback.GiveOrder( commanderid, new Command( - unitdef.id, buildsite.ToDoubleArray() ) );
        }
        
        public void  CommanderBuildPower()
        {
            IUnitDef unitdef = buildtable.UnitDefByName[ "armsolar" ] as IUnitDef;
            CommanderBuild( unitdef );
        }
        
        public void  CommanderBuildFactory()
        {
            IUnitDef unitdef = buildtable.UnitDefByName[ "armvp" ] as IUnitDef;
            CommanderBuild( unitdef );
        }
        
        public void  CommanderBuildRadar()
        {
            IUnitDef unitdef = buildtable.UnitDefByName[ "armrad" ] as IUnitDef;
            CommanderBuild( unitdef );
        }
        
        public void  CommanderBuildExtractor()
        {
            IUnitDef unitdef = buildtable.UnitDefByName[ "armmex" ] as IUnitDef;;
            Float3 commanderpos = aicallback.GetUnitPos( commanderid );
            Float3 nearestmetalpos = metal.GetNearestMetalSpot( commanderpos );
            logfile.WriteLine( "CommanderBuildExtractor nearestmetalpos is null? " + ( nearestmetalpos == null ).ToString() );
            //Float3 nearestmetalpos = metalspot.Pos;
            Float3 buildsite = aicallback.ClosestBuildSite( unitdef, nearestmetalpos, 1400.0, 2 );        
            CommanderBuildAt( unitdef, buildsite );
           // metal.MarkMetalSpotUsed( metalspot );
        }
        
        public Float3 GetCommanderPos()
        {
            return aicallback.GetUnitPos( commanderid );
        }
        
        public void UnitAdded( int deployedunitid, IUnitDef unitdef )
        {
            if( unitdef.isCommander )
            {
                commanderid = deployedunitid;
                logfile.WriteLine( "CommanderController got commanderid: " + commanderid );
                //DoSomething();
            }
        }
        
        public void UnitRemoved( int deployedunitid )
        {
            if( deployedunitid == commanderid )
            {
                commanderalive = false;
            }
        }
        
        public void UnitIdle( int deployedunitid )
        {
           // if( deployedunitid == commanderid )
            //{
              //  logfile.WriteLine( "CommanderController: commander detected as idle, putting to work" );
             //   DoSomething();
           // }
        }
        
        public void Tick()
        {
          //  if( commanderalive )
           // {
                //commanderid = unitcontroller.GetCommanderId();
                //if( aicallback.
                //if( !aicallback.UnitIsBusy( commanderid ) )
               // {
                  //  logfile.WriteLine( "Reactivating commander" );
                  //  DoSomething();
                //}
          //  }
        }
    }    
}
