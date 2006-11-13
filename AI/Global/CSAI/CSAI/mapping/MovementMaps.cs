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
    class MovementMaps
    {
        static MovementMaps instance = new MovementMaps();
        public static MovementMaps GetInstance(){ return instance; }
        
        CSAI CSAI;
        IAICallback aicallback;
        LogFile logfile;
        
        public double maxvehicleslope = 0.08;  // arbitrary cutoff, for simplicity
        public double maxinfantryslope = 0.33; // arbitrary cutoff, for simplicity

        public const int SQUARE_SIZE = 8;
            
        public double[,] slopemap;
        public double[] heightmap;
        
        // these are at half map resolution, so 16 pos per sector
        public bool[,] infantrymap; // true means troops can move freely in sector
        public bool[,] vehiclemap; // true means vehicles can move freely in sector
        public bool[,] boatmap; // true means boats can move freely in sector
            
        // these are at half map resolution, so 16 pos per sector
        public int[,] infantryareas; // each area has its own number, 0 means infantry cant go there
        public int[,] vehicleareas; // each area has its own number, 0 means vehicles cant go there
        public int[,] boatareas; // each area has its own number, 0 means boats cant go there
            
        public int[] infantryareasizes = null; // indexed by ( area number )
        public int[] vehicleareasizes = null; // indexed by ( area number )
        public int[] boatareasizes = null; // indexed by ( area number )
        
        public int mapwidth;
        public int mapheight;
            
        public MovementMaps()
        {
            CSAI = CSAI.GetInstance();
            aicallback = CSAI.aicallback;
            logfile = LogFile.GetInstance();  

            GenerateMaps();
        }
        
        public int GetArea( IUnitDef unitdef, Float3 currentpos )
        {
            int mapx = (int)( currentpos.x / 16 );
            int mapy = (int)( currentpos.z / 16 );
            UnitDefHelp unitdefhelp = new UnitDefHelp( aicallback );
            if( unitdefhelp.IsBoat( unitdef ) )
            {
                return boatareas[ mapx, mapy ];
            }
            else if( unitdef.movedata.maxSlope > maxvehicleslope ) // so infantry, approximately
            {
                return infantryareas[ mapx, mapy ];
            }
            else // vehicle, approximately
            {
                return vehicleareas[ mapx, mapy ];
            }
        }
        
        public void GenerateMaps()
        {
            logfile.WriteLine( "MovementMaps.GenerateMaps start" );
            slopemap = new SlopeMap().GetSlopeMap();
            heightmap = aicallback.GetCentreHeightMap();
            
            mapwidth = aicallback.GetMapWidth();
            mapheight = aicallback.GetMapHeight();
            
            GenerateInfantryAccessibleMap();
            GenerateVehicleAccessibleMap();
            GenerateBoatAccessibleMap();
            
            infantryareas = CreateAreas( infantryareasizes, infantrymap );
            vehicleareas = CreateAreas( vehicleareasizes, vehiclemap );
            boatareas = CreateAreas( boatareasizes, boatmap );
            
            logfile.WriteLine( "MovementMaps.GenerateMaps done" );
        }
        
        // start from a valid point on map for unit type, ie accessibilitymap is true, and see where we can get to, this is area 1
        // repeat till all valid points on map have been visited
        int[,] CreateAreas( int[]infantryareasizes, bool[,]accessibilitymap )
        {
            logfile.WriteLine( "CreateAreas" );
            bool[,]visited = new bool[ mapwidth / 2, mapheight / 2 ];
            int[,] areamap = new int[ mapwidth / 2, mapheight / 2 ];
            
            ArrayList areasizesal = new ArrayList();
            areasizesal.Add( 0 );
            int areanumber = 1;
            for( int y = 0; y < mapwidth / 2; y++ )
            {
                for( int x = 0; x < mapwidth / 2; x++ )
                {
                    if( accessibilitymap[x, y] && !visited[ x, y ] )
                    {
                        int count = Bfs( areamap, areanumber, x, y, accessibilitymap, visited );
                        logfile.WriteLine( "area " + areanumber + " size: " + count );
                        areasizesal.Add( count );
                        areanumber++;
                        //return areamap;
                    }
                }
            }
            logfile.WriteLine( "number areas: " + ( areanumber - 1 ).ToString() );
            infantryareasizes = (int[])areasizesal.ToArray( typeof( int ) );
            return areamap;
        }
        
        class Direction
        {
            public int x; 
            public int y;
            public Direction( int x, int y )
            {
                this.x = x; this.y = y;
            }
        }
        
        // run a breath first search for everywhere in the area
        // assume can only move north/south/east/west (not diagonally)
        int Bfs( int[,] areamap, int areanumber, int startx, int starty, bool[,]accessibilitymap, bool[,]visited )
        {
            Direction[] directions = new Direction[4];
            directions[0] = new Direction( 0, 1 );
            directions[1] = new Direction( 0, -1 );
            directions[2] = new Direction( 1, 0 );
            directions[3] = new Direction( -1, 0 );
            
            int numbersquares = 1;
            visited[ startx, starty ] = true;
            areamap[ startx, starty ] = areanumber;
            Queue queue = new Queue();
			queue.Enqueue( ( startx << 16 ) + ( starty ) );  // easier than using new int[]{} or making a class or queueing an arraylist.  And it looks cool ;-)
            
            while( queue.Count > 0 )
            {
                int value = (int)queue.Dequeue();
                int thisx = value >> 16;
                int thisy = value & 65535;
                
               // if( numbersquares > 30 )
               // {
               //     return numbersquares;
              //  }
                
                // add surrounding cells to queue, as long as they are legal for this unit type (accessibilitymap is true)
                foreach( Direction direction in directions )
                {
                    int deltax = direction.x;
                    int deltay = direction.y;
                    int newx = thisx + deltax;
                    int newy = thisy + deltay;
                    //logfile.WriteLine( "newx " + newx + "  " + newy + " delta: " + deltax + " " + deltay );
                    if( newx >= 0 && newy >= 0 && newx < ( mapwidth / 2 ) && newy < ( mapheight / 2 ) &&
                        !visited[ newx, newy ] && accessibilitymap[ newx, newy ] )
                    {
                        visited[ newx, newy ] = true;
                        numbersquares++;
                        areamap[ newx, newy ] = areanumber;
                        //areamap[ newx, newy ] = numbersquares;
                        queue.Enqueue( ( newx << 16 ) + newy );
                    }
                }
            }
            return numbersquares;
        }
        
        void GenerateInfantryAccessibleMap()
        {
            logfile.WriteLine( "GenerateInfantryAccessibleMap" );
            infantrymap = new bool[ mapwidth / 2, mapheight / 2 ];
            for( int y = 0; y < mapheight / 2; y++ )
            {
                for( int x = 0; x < mapwidth / 2; x++ )
                {
                    infantrymap[ x , y ] = true;
                }
            }
            
            ArrayIndexer arrayindexer = new ArrayIndexer( mapwidth, mapheight );
            for( int y = 0; y < mapheight; y++ )
            {
                for( int x = 0; x < mapwidth; x++ )
                {
                    if( slopemap[ x/ 2, y / 2 ] > 0.33 )
                    {
                        infantrymap[ x /2, y / 2 ] = false;
                    }
                    if( heightmap[ arrayindexer.GetIndex( x, y ) ] < 0 )
                    {
                        infantrymap[ x /2, y / 2 ] = false;
                    }
                }
            }
        }
        
        void GenerateVehicleAccessibleMap()
        {
            logfile.WriteLine( "GenerateVehicleAccessibleMap" );
            vehiclemap = new bool[ mapwidth / 2, mapheight / 2 ];
            for( int y = 0; y < mapheight / 2; y++ )
            {
                for( int x = 0; x < mapwidth / 2; x++ )
                {
                    vehiclemap[ x , y ] = true;
                }
            }
            
            ArrayIndexer arrayindexer = new ArrayIndexer( mapwidth, mapheight );
            for( int y = 0; y < mapheight; y++ )
            {
                for( int x = 0; x < mapwidth; x++ )
                {
                    if( slopemap[ x/ 2, y / 2 ] > 0.08 )
                    {
                        vehiclemap[ x /2, y / 2 ] = false;
                    }
                    if( heightmap[ arrayindexer.GetIndex( x, y ) ] < 0 )
                    {
                        vehiclemap[ x /2, y / 2 ] = false;
                    }
                }
            }
        }
        
        void GenerateBoatAccessibleMap()
        {
            logfile.WriteLine( "GenerateBoatAccessibleMap" );
            boatmap = new bool[ mapwidth / 2, mapheight / 2 ];
            for( int y = 0; y < mapheight / 2; y++ )
            {
                for( int x = 0; x < mapwidth / 2; x++ )
                {
                    boatmap[ x , y ] = true;
                }
            }
            
            ArrayIndexer arrayindexer = new ArrayIndexer( mapwidth, mapheight );
            for( int y = 0; y < mapheight; y++ )
            {
                for( int x = 0; x < mapwidth; x++ )
                {
                    if( heightmap[ arrayindexer.GetIndex( x, y ) ] > 0 )
                    {
                        boatmap[ x /2, y / 2 ] = false;
                    }
                }
            }
        }
        
        // from Map.cpp, by Submarine
        /*
        public static Float3 PosToFinalBuildPos( Float3 pos, IUnitDef def)
        {
            Float3 newpos = pos;
            if( ( def.xsize & 2 ) > 0 ) // check if xsize is a multiple of 4  // does this calculation do that???
            {
                newpos.x=Math.Floor((newpos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
            }
            else
            {
                newpos.x=Math.Floor((newpos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
            }
        
            if( ( def.ysize & 2 ) > 0 )
            {
                newpos.z=Math.Floor((newpos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
            }
            else
            {
                newpos.z=Math.Floor((newpos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
            }
            return newpos;
        }
        */
        
        /*
        // converts unit positions to cell coordinates
        // from Map.cpp, by Submarine
        public static Float3 PosToBuildMapPos( Float3 unitpos, IUnitDef def)
        {
            if( unitpos == null )
            {
                throw new Exception( "ERROR: Null-pointer float3 *pos in Pos2BuildMapPos()" );
            }
        
            if( def == null )
            {
                throw new Exception(  "ERROR: Null-pointer UnitDef *def in Pos2BuildMapPos()");
            }
        
            Float3 mappos = new Float3();
            // get cell index of middlepoint
            mappos.x = (int) (unitpos.x/SQUARE_SIZE);
            mappos.z = (int) (unitpos.z/SQUARE_SIZE);
            mappos.y = (int) (unitpos.y );
        
            // shift to the leftmost uppermost cell
            mappos.x -= def.xsize/2;
            mappos.z -= def.ysize/2;
        
            // check if pos is still in that map, otherwise retun 0
            if( mappos.x < 0 && mappos.z < 0 )
            {
                mappos.x = mappos.z = 0;
            }
            return mappos;
        }
        */
    }
}
