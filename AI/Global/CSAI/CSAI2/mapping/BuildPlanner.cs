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
    // arguably reservations should go through this class, since this is essentially the controller and BuildMap is the model
    public class BuildPlanner
    {
        public const int BuildMargin = 8;
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        int mapwidth;
        int mapheight;
        
        static BuildPlanner instance = new BuildPlanner();
        public static BuildPlanner GetInstance(){ return instance; }
        
        UnitDefHelp unitdefhelp;
        
        BuildPlanner()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();  
            
            unitdefhelp = new UnitDefHelp( aicallback );
            
            mapwidth = aicallback.GetMapWidth();
            mapheight = aicallback.GetMapHeight();
        }
        
        public Float3 ClosestBuildSite( IUnitDef unitdef, Float3 approximatepos, int maxposradius, int mindistancemapunits )
        {
            // ok, so plan is we work our way out in a kindof spiral
            // we start in centre, and increase radius, and work around in a square at each radius, until we find something
            int centrex = (int)( approximatepos.x / 8 );
            int centrey = (int)( approximatepos.z / 8 );
            //int radius = 0;
            int radius = mindistancemapunits;
            int unitsizexwithmargin = unitdef.xsize + 2 * BuildMargin;
            int unitsizeywithmargin = unitdef.ysize + 2 * BuildMargin;
            if( unitdefhelp.IsFactory( unitdef ) )
            {
                unitsizeywithmargin += 8;
                centrey += 4;
            }
            BuildMap buildmap = BuildMap.GetInstance();
            MovementMaps movementmaps = MovementMaps.GetInstance();
            // while( radius < mapwidth || radius < mapheight ) // hopefully we never get quite this far...
            while( radius < ( maxposradius / 8 ) )
            {
              //  logfile.WriteLine( "ClosestBuildSite radius " + radius );
                for( int deltax = -radius; deltax <= radius; deltax++ )
                {
                    for( int deltay = -radius; deltay <= radius; deltay++ )
                    {
                        if( deltax == radius || deltax == -radius || deltay == radius || deltay == -radius ) // ignore the hollow centre of square
                        {
                           // logfile.WriteLine( "delta " + deltax + " " + deltay );
                            bool positionok = true;
                            // go through each square in proposed site, check not build on
                            for( int buildmapy = centrey - unitsizeywithmargin / 2; positionok && buildmapy < centrey + unitsizeywithmargin / 2; buildmapy++ )
                            {
                               //string logline = "";
                                for( int buildmapx = centrex - unitsizexwithmargin / 2; positionok && buildmapx < centrex + unitsizexwithmargin / 2; buildmapx++ )
                                {
                                    int thisx = buildmapx + deltax;
                                    int thisy = buildmapy + deltay;
                                    if( thisx < 0 || thisy < 0 || thisx >= mapwidth || thisy >= mapwidth ||
                                        !buildmap.SquareAvailable[ thisx, thisy ] ||
                                        !movementmaps.vehiclemap[ thisx / 2, thisy / 2 ] )
                                    {
                                        //logfile.WriteLine( "check square " + buildmapx + " " + buildmapy + " NOK" );
                                        positionok = false;
                                     //   logline += "*";
                                    }
                                    else
                                    {
                                     //   logline += "-";
                                    }
                                    //logfile.WriteLine( "check square " + buildmapx + " " + buildmapy + "Ok" );
                                }
                            //    logfile.WriteLine( logline );
                            }
                           // logfile.WriteLine("");
                            if( positionok )
                            {
                                return new Float3( ( centrex + deltax ) * 8, 0, ( centrey + deltay ) * 8 );
                            }
                        }
                    }
                }
                radius++;
            }
            return null;
        }
    }
}
