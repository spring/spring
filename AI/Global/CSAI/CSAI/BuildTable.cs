// Copyright Submarine, Hugh Perkins 2006
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

// derived from BuildTable.cpp/BuildTable.h by Submarine

using System;
using System.Collections;
using System.Collections.Generic;

namespace CSharpAI
{
    public class BuildTable
    {
        static BuildTable instance = new BuildTable();
        public static BuildTable GetInstance(){ return instance; } // Singleton pattern

        public const string ArmCommander = "arm_commander";
        public const string ArmMex = "arm_metal_extractor";
        public const string ArmConstructionVehicle = "arm_construction_vehicle";
        public const string ArmVehicleFactory = "arm_vehicle_plant";
        public const string ArmSolar = "arm_solar_collector";
        public const string ArmL1Tank= "arm_stumpy";
        public const string ArmL1AntiAir = "arm_samson";
        public const string ArmL1Radar = "arm_radar_tower";
        public const string ArmMoho = "arm_moho_mine";
        public const string ArmGroundScout = "arm_jeffy";
        
        public const string PlacementUnit = "core_krogoth_gantry";

        public static List<string> Tanks = new List<string>(new string[]{
                                                           	ArmL1Tank, ArmL1AntiAir
                                                           });
                                                           	//string[]UnitsWeLike = new string[]{ "armsam", "armstump", "armrock", "armjeth", "armkam", "armanac", "armsfig", "armmh", "armah",
        //    "armbull", "armmart" };

        //int numOfUnitTypes = 0;
        public IUnitDef[] availableunittypes;
        public List<IUnitDef> MetalExtractors = new List<IUnitDef>();
        public List<IUnitDef> EnergyProducers = new List<IUnitDef>();
        public List<IUnitDef> Factories = new List<IUnitDef>();
        public List<IUnitDef> Constructors = new List<IUnitDef>();
        public List<IUnitDef> GroundAttack = new List<IUnitDef>();
        public List<IUnitDef> AirAttack = new List<IUnitDef>();
            
        public Dictionary<string,IUnitDef> UnitDefByName = new Dictionary<string,IUnitDef>();

        CSAI CSAI;
        IAICallback aicallback;
        LogFile logfile;
        string modname;
                
        BuildTable() // protected constructor to force Singleton instantiation
        {
            CSAI = CSAI.GetInstance();
            aicallback = CSAI.aicallback;
            logfile = LogFile.GetInstance();
            
            modname = aicallback.GetModName();
            
            int numunitdefs = aicallback.GetNumUnitDefs();
            logfile.WriteLine( "calling GetUnitDefList, for " + numunitdefs + " units ... " );
            //availableunittypes = aicallback.GetUnitDefList();
            availableunittypes = new IUnitDef[numunitdefs + 1];
            for (int i = 1; i <= numunitdefs; i++)
            {
                availableunittypes[i] = aicallback.GetUnitDefByTypeId(i);
                logfile.WriteLine( i + " " + availableunittypes[i].name + " " + availableunittypes[i].humanName );
            }
            logfile.WriteLine( "... done" );
            
            if( !LoadCache( modname ) )
            {
                aicallback.SendTextMsg( "Creating new cachefile for mod " + modname, 0 );
                GenerateBuildTable( modname );
                SaveCache( modname );
            }
        }
        
        void GenerateBuildTable( string modname )
        {
            logfile.WriteLine( "Generating indexes mod " + modname );
            
            foreach( IUnitDef unitdef in availableunittypes )
            {
            	if( unitdef != null )
            	{
	                string logline = unitdef.id + " " + unitdef.name + " " + unitdef.humanName + " size: " + unitdef.xsize + "," + unitdef.ysize;
	                IMoveData movedata = unitdef.movedata;
	                if( movedata != null )
	                {
	                    logline += " maxSlope: " + movedata.maxSlope + " depth " + movedata.depth + " slopeMod " + movedata.slopeMod +
	                    " depthMod: " + movedata.depthMod + " movefamily: " + movedata.moveFamily;
	                }
	                logfile.WriteLine(  logline );
	                    
	                if( unitdef.extractsMetal > 0 )
	                {
	                    MetalExtractors.Add( unitdef );
	                }
	                if( unitdef.energyMake > 0 || unitdef.windGenerator > 0 || unitdef.tidalGenerator > 0 )
	                {
	                    EnergyProducers.Add( unitdef );
	                }
	                if( unitdef.builder && unitdef.speed >= 1 )
	                {
	                    Constructors.Add( unitdef );
	                }
	                if( unitdef.builder && unitdef.speed < 1 )
	                {
	                    Factories.Add( unitdef );
	                }
	                if( unitdef.canAttack && unitdef.canmove && unitdef.minWaterDepth < 0 )
	                {
	                    GroundAttack.Add( unitdef );
	                }
	                if( unitdef.canAttack && unitdef.canmove && unitdef.canfly )
	                {
	                    AirAttack.Add( unitdef );
	                }
	                if( !UnitDefByName.ContainsKey( unitdef.name.ToLower() ) )
	                {
	                    UnitDefByName.Add( unitdef.name.ToLower(), unitdef );
	                }
	                else
	                {
	                    logfile.WriteLine( "Warning: duplicate name: " + unitdef.name.ToLower() );
	                }
            	}
            }
            logfile.WriteLine( "=======================================================" );
            logfile.WriteLine( "Metal Extractors" );
            logfile.WriteLine( MetalExtractors.ToString() );
            logfile.WriteLine( "=======================================================" );
            logfile.WriteLine( "EnergyProducers" );
            logfile.WriteLine( EnergyProducers.ToString() );
            logfile.WriteLine( "=======================================================" );
            logfile.WriteLine( "Constructors" );
            logfile.WriteLine( Constructors.ToString() );
            logfile.WriteLine( "=======================================================" );
            logfile.WriteLine( "GroundAttack" );
            logfile.WriteLine( GroundAttack.ToString() );
            logfile.WriteLine( "=======================================================" );
            logfile.WriteLine( "Factories" );
            logfile.WriteLine( Factories.ToString() );
            
            IUnitDef constuctionvehicle =  UnitDefByName[ BuildTable.ArmConstructionVehicle ] as IUnitDef;
            //int 
            //BuildOption[] buildoptions = constuctionvehicle.GetNumBuildOptions;
            //string buildoptionsstring = "Constructionvehicle build options: ";
            //foreach( BuildOption buildoption in buildoptions )
            //{
                //buildoptionsstring += "( " + buildoption.TablePosition.ToString() + ", " + buildoption.UnitTypeName + " ) ";
            //}
            //logfile.WriteLine( buildoptionsstring );
        }
        
        void SaveCache( string modname )
        {
        }
        
        bool LoadCache( string modname )
        {
            return false;
        }
        
        //int BiggestMexUnit = -1; // cache value to save time later
        public IUnitDef GetBiggestMexUnit()
        {
            //if( BiggestMexUnit != -1 )
            //{
              //  return BiggestMexUnit;
            //}
            
            logfile.WriteLine( "Entering GetBiggestMexUnit()");
            
            int biggest_mex_id = 0, biggest_area = 0;
            
            logfile.WriteLine( "Scanning unitdef list...");
            IUnitDef biggestmexunit = null;
            foreach( IUnitDef unitdef in availableunittypes )
            {
                if( unitdef.extractsMetal > 0 )
                {
                    int thisarea = unitdef.xsize * unitdef.ysize;
                    if( thisarea > biggest_area )
                    {
                        biggest_mex_id = unitdef.id;
                        biggest_area = thisarea;
                        biggestmexunit = unitdef;
                    }
                }
            }
            
            logfile.WriteLine( "Leaving GetBiggestMexUnit(), it's unittypeid is " + biggest_mex_id  );
            return biggestmexunit;
        }
    }
}
