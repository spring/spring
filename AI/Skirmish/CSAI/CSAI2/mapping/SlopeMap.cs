// Copyright Spring project, Hugh Perkins 2006
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

namespace CSharpAI
{
    public class SlopeMap
    {
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
                
        public SlopeMap()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();            
        }
        
        // ported from Spring's ReadMap.cpp by Hugh Perkins
        public double[,]GetSlopeMap()
        {
            int mapwidth = aicallback.GetMapWidth();
            int mapheight = aicallback.GetMapHeight();
            
            int slopemapwidth = mapwidth / 2;
            int slopemapheight = mapheight / 2;
            
            int squaresize = MovementMaps.SQUARE_SIZE; // jsut to save typing...

            double[,] heightmap = HeightMap.GetInstance().GetHeightMap();
            logfile.WriteLine( "Getting heightmap, this could take a while... " );            

            // ArrayIndexer heightmapindexer = new ArrayIndexer( mapwidth + 1, mapheight + 1 );
            logfile.WriteLine("calculating slopes..." );
            logfile.WriteLine("mapwidth: " + slopemapwidth + " " + slopemapheight);
                        
            double[,]SlopeMap = new double[ slopemapwidth, slopemapheight ];
            for(int y = 2; y < mapheight - 2; y+= 2)
            {
                for(int x = 2; x < mapwidth - 2; x+= 2)
                {
                    AdvancedFloat3 e1 = new AdvancedFloat3(-squaresize * 4, heightmap[x - 1, y - 1] - heightmap[x + 3, y - 1], 0);
                    AdvancedFloat3 e2 = new AdvancedFloat3(0, heightmap[x - 1, y - 1] - heightmap[x - 1, y + 3], -squaresize * 4);
        
                    AdvancedFloat3 n=e2.Cross( e1 );
        
                    n.Normalize();

                    e1 = new AdvancedFloat3(squaresize * 4, heightmap[x + 3, y + 3] - heightmap[x - 1, y + 3], 0);
                    e2 = new AdvancedFloat3(0, heightmap[x + 3, y + 3] - heightmap[x + 3, y - 1], squaresize * 4);
        
                    AdvancedFloat3 n2 = e2.Cross(e1);
                    n2.Normalize();

                    SlopeMap[ x / 2, y / 2 ]= 1 - ( n.y + n2.y ) * 0.5;
                }
            }
            logfile.WriteLine("... slopes calculated" );
            return SlopeMap;
        }
	}
}
