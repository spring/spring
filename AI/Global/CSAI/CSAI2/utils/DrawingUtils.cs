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
    public class DrawingUtils
    {
        public static int DrawRectangle(Float3 pos, int width, int height, int groupnumber)
        {
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            double elevation = aicallback.GetElevation(pos.x, pos.z) + 10;
            groupnumber = aicallback.CreateLineFigure(pos + new Float3(0, elevation, 0),
                pos + new Float3(width, elevation, 0), 10, false, 200, groupnumber);
            groupnumber = aicallback.CreateLineFigure(pos + new Float3(width, elevation, 0),
                pos + new Float3(width, elevation, height), 10, false, 200, groupnumber);
            groupnumber = aicallback.CreateLineFigure(pos + new Float3(width, elevation, height),
                pos + new Float3(0, elevation, height), 10, false, 200, groupnumber);
            groupnumber = aicallback.CreateLineFigure(pos + new Float3(0, elevation, height),
                pos + new Float3(0, elevation, 0), 10, false, 200, groupnumber);

            return groupnumber;
        }

        public static int DrawCircle( Float3 pos, double radius )
        {
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            Float3 lastpos = null;
            int groupnumber = 0;
            for( int angle = 0; angle <= 360; angle += 10 )
            {
                int x = (int)( (double)radius * Math.Cos ( (double)angle * Math.PI / 180 ) );
                int y = (int)( (double)radius * Math.Sin ( (double)angle * Math.PI / 180 ) );
                Float3 thispos = new Float3( x, 0, y ) + pos;
                if( lastpos != null )
                {
                    groupnumber = aicallback.CreateLineFigure( thispos,lastpos,10,false,200, groupnumber);
                }
                lastpos = thispos;
            }
            return groupnumber;
        }

        // returns figure group
        public static int DrawMap(int[,] map )
        {
            int thismapwidth = map.GetUpperBound(0) + 1;
            int thismapheight = map.GetUpperBound(1) + 1;
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            int multiplier = ( aicallback.GetMapWidth() / thismapwidth ) * 8;

            int figuregroup = 0;
            for( int y = 0; y < thismapheight; y++ )
            {
                for (int x = 0; x < thismapwidth; x++ )
                {
                    double elevation = aicallback.GetElevation( x *multiplier, y * multiplier ) + 10;
                    if (x < (thismapwidth - 1) &&
                        map[x, y] != map[x + 1, y] )
                    {
                        figuregroup = aicallback.CreateLineFigure(new Float3((x + 1) * multiplier, elevation, y * multiplier),
                            new Float3((x + 1) * multiplier, elevation, (y + 1) * multiplier), 10, false, 200, figuregroup);
                    }
                    if (y < (thismapheight - 1) &&
                        map[x, y] != map[x, y + 1] )
                    {
                        figuregroup = aicallback.CreateLineFigure(new Float3(x * multiplier, elevation, (y + 1) * multiplier),
                            new Float3((x + 1) * multiplier, elevation, (y + 1) * multiplier), 10, false, 200, figuregroup);
                    }
                }
            }
            return figuregroup;
        }
        // returns figure group
        public static int DrawMap(bool[,] map)
        {
            int thismapwidth = map.GetUpperBound(0) + 1;
            int thismapheight = map.GetUpperBound(1) + 1;
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            int multiplier = (aicallback.GetMapWidth() / thismapwidth) * 8;

            int figuregroup = 0;
            for (int y = 0; y < thismapheight; y++)
            {
                for (int x = 0; x < thismapwidth; x++)
                {
                    double elevation = aicallback.GetElevation(x * multiplier, y * multiplier) + 10;
                    if (x < (thismapwidth - 1) &&
                        map[x, y] != map[x + 1, y])
                    {
                        figuregroup = aicallback.CreateLineFigure(new Float3((x + 1) * multiplier, elevation, y * multiplier),
                            new Float3((x + 1) * multiplier, elevation, (y + 1) * multiplier), 10, false, 200, figuregroup);
                    }
                    if (y < (thismapheight - 1) &&
                        map[x, y] != map[x, y + 1])
                    {
                        figuregroup = aicallback.CreateLineFigure(new Float3(x * multiplier, elevation, (y + 1) * multiplier),
                            new Float3((x + 1) * multiplier, elevation, (y + 1) * multiplier), 10, false, 200, figuregroup);
                    }
                }
            }
            return figuregroup;
        }
    }
}
