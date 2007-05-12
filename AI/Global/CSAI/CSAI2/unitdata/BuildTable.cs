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

        //int numOfUnitTypes = 0;
        IUnitDef[] availableunittypes;
        public Dictionary<string, IUnitDef> UnitDefByName = new Dictionary<string,IUnitDef>();
        public Dictionary<int, IUnitDef> UnitDefById = new Dictionary<int, IUnitDef>(); // yes, we could use array, but this is easier to read

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
            
            logfile.WriteLine( "calling GetUnitDefList... " );
            List<IUnitDef> unittypeslist = new List<IUnitDef>();
            int numunittypes = aicallback.GetNumUnitDefs();
            for (int i = 1; i <= numunittypes; i++)
            {
                unittypeslist.Add( aicallback.GetUnitDefByTypeId( i ) );
            }
            availableunittypes = unittypeslist.ToArray();
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
                string logline = unitdef.id + " " + unitdef.name + " " + unitdef.humanName + " size: " + unitdef.xsize + "," + unitdef.ysize;
                IMoveData movedata = unitdef.movedata;
                if( movedata != null )
                {
                    logline += " maxSlope: " + movedata.maxSlope + " depth " + movedata.depth + " slopeMod " + movedata.slopeMod +
                    " depthMod: " + movedata.depthMod + " movefamily: " + movedata.moveFamily;
                }
                logfile.WriteLine(  logline );
                    
                if (!UnitDefByName.ContainsKey(unitdef.name.ToLower()))
                {
                    UnitDefByName.Add( unitdef.name.ToLower(), unitdef );
                    UnitDefById.Add(unitdef.id, unitdef);
                }
                else
                {
                    logfile.WriteLine( "Warning: duplicate name: " + unitdef.name.ToLower() );
                }
            }
            
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
