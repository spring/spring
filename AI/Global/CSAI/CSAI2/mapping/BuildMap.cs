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
using System.Collections;

namespace CSharpAI
{
    public class BuildMap
    {
        static BuildMap instance = new BuildMap();
        public static BuildMap GetInstance(){ return instance; }
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        UnitDefHelp unitdefhelp;
        UnitController unitcontroller;
        
        class BuildingInfo
        {
            public int mapx;
            public int mapy;
            public int mapsizex;
            public int mapsizey;
            public BuildingInfo( int mapx, int mapy, int mapsizex, int mapsizey )
            {
                this.mapx = mapx;
                this.mapy = mapy;
                this.mapsizex = mapsizex;
                this.mapsizey = mapsizey;
            }
        }
        
        Hashtable buildinginfobyid = new Hashtable(); // BuildingInfo by deployedid
    
        public bool[,] SquareAvailable; // mapwidth by mapheight
        int mapwidth;
        int mapheight;
        
        BuildMap()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();  
            
            unitdefhelp = new UnitDefHelp( aicallback );
            unitcontroller = UnitController.GetInstance();
            
            csai.UnitCreatedEvent += new CSAI.UnitCreatedHandler( UnitCreated );
            csai.UnitDestroyedEvent += new CSAI.UnitDestroyedHandler( UnitDestroyed );
            
            unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitCreated );

            if (csai.DebugOn)
            {
                csai.RegisterVoiceCommand("dumpbuildmap", new CSAI.VoiceCommandHandler(DumpBuildMap));
            }

            Init();
        }

        public void DumpBuildMap(string cmd, string[] split, int player)
        {
            DrawingUtils.DrawMap(SquareAvailable);
        }
        
        public void Init()
        {
            // plan: mark every square available, then remove squares as we build
            // also, remove planned buildings, ie: metal extractors
            // note: get metal do do this for us
            // so we need UnitController to alert us about buildings in progress etc
            
            logfile.WriteLine( "BuildMap.Init()" );
            mapwidth = aicallback.GetMapWidth();
            mapheight = aicallback.GetMapHeight();
            
            SquareAvailable = new bool[ mapwidth, mapheight ];
            for( int x = 0; x < mapwidth; x++ )
            {
                for( int y = 0; y < mapheight; y++ )
                {
                    SquareAvailable[ x, y ] = true;
                }
            }
            logfile.WriteLine( "BuildMap.Init finished()" );
        }
        
        // ReserveSpace; ideally the reserver would pass himself in as IBuildMapReserver or something , but object for now (not yet used)
        // note that destroying a unit over this space wont undo reservation, since not included in buildinginfobyid list
        // (maybe buildinginfobyowner???)
        public void ReserveSpace( object owner, int mapx, int mapy, int sizex, int sizey )
        {
            if (csai.DebugOn)
            {
                DrawingUtils.DrawRectangle(
                    new Float3((mapx - sizex / 2) * 8,
                    0,
                    ( mapy - sizey / 2 ) * 8),
                    sizex * 8,
                    sizey * 8,
                    0);
                // logfile.WriteLine( "marking " + thisx + " " + thisy + " as used by " + owner.ToString() );
                //   aicallback.DrawUnit( "ARMMINE1", new Float3( thisx * 8, aicallback.GetElevation( thisx * 8, thisy * 8 ), thisy * 8 ), 0.0f, 400, aicallback.GetMyAllyTeam(), true, true);
            }
            for (int x = 0; x < sizex; x++)
            {
                for( int y = 0; y < sizey; y++ )
                {
                    int thisx = mapx + x - sizex / 2;
                    int thisy = mapy + y - sizey / 2;
                    if( thisx >= 0 && thisy >= 0 && thisx < mapwidth && thisy < mapheight )
                    {
                        SquareAvailable[ thisx, thisy ] = false;
                    }
                }
            }
        }
        
        // mark squares as used
        // and keep record of where this unit was, and how big, in case it is destroyed
        public void UnitCreated( int id, IUnitDef unitdef )
        {
            if( !buildinginfobyid.Contains( id ) && !unitdefhelp.IsMobile( unitdef ) )
            {
                Float3 pos = aicallback.GetUnitPos( id );
                int mapposx = (int)( pos.x / 8 );
                int mapposy = (int)( pos.z / 8 );
                //int unitsizex = (int)Math.Ceiling( unitdef.xsize / 8.0 );
                //int unitsizey = (int)Math.Ceiling( unitdef.ysize / 8.0 );
                int unitsizex = unitdef.xsize;
                int unitsizey = unitdef.ysize;
                logfile.WriteLine( "Buildmap unitcreated " + unitdef.name + " mappos " + mapposx + " " + mapposy + " unitsize " + unitsizex + " " + unitsizey );
                buildinginfobyid.Add( id, new BuildingInfo( mapposx, mapposy, unitsizex, unitsizey ) );
                if (csai.DebugOn)
                {
                    DrawingUtils.DrawRectangle(
                        new Float3((mapposx - unitdef.xsize / 2) * 8,
                        0,
                        (mapposy - unitdef.ysize / 2) * 8),
                        unitdef.xsize * 8,
                        unitdef.ysize * 8,
                        0);
                }
                for( int x = 0; x < unitsizex; x++ )
                {
                    for( int y = 0; y < unitsizey; y++ )
                    {
                        int thisx = mapposx + x - unitdef.xsize / 2;
                        int thisy = mapposy + y - unitdef.ysize / 2;
                        SquareAvailable[ thisx, thisy ] = false;
                        //if( csai.DebugOn )
                        //{
                           // logfile.WriteLine( "marking " + thisx + " " + thisy + " as used by " + unitdef.name );
                          //  aicallback.DrawUnit( "ARMMINE1", new Float3( thisx * 8, aicallback.GetElevation( thisx * 8, thisy * 8 ), thisy * 8 ), 0.0f, 400, aicallback.GetMyAllyTeam(), true, true);
                        //}
                    }
                }
            }
        }
        
        public void UnitDestroyed( int id, int enemy )
        {
            if( buildinginfobyid.Contains( id ) )
            {
                BuildingInfo buildinginfo = buildinginfobyid[ id] as BuildingInfo;
                for( int x = 0; x < buildinginfo.mapsizex / 8; x++ )
                {
                    for( int y = 0; y < buildinginfo.mapsizey / 8; y++ )
                    {
                        SquareAvailable[ buildinginfo.mapx + x, buildinginfo.mapy + y ] = true;
                    }
                }                
                buildinginfobyid.Remove( id );
            }
        }
    }
}
