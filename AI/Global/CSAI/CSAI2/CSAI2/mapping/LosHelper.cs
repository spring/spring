using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class LosHelper
    {
        public class Int2
        {
            public int x;
            public int y;
            public Int2() { }
            public Int2(int x, int y)
            {
                this.x = x;
                this.y = y;
            }
        }

        // note: need to check compatible area
        public Float3 GetNearestUnseen(Float3 currentpos, IUnitDef unitdef, int unseensmeansthismanyframes)
        {
            LosMap losmap = LosMap.GetInstance();
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            int mapwidth = aicallback.GetMapWidth();
            int mapheight = aicallback.GetMapHeight();

            int currentunitarea = MovementMaps.GetInstance().GetArea(unitdef, currentpos);
            int losmapwidth = losmap.LastSeenFrameCount.GetUpperBound(0) + 1;
            int losmapheight = losmap.LastSeenFrameCount.GetUpperBound(0) + 1;
            int maxradius = (int)Math.Sqrt(losmapheight * losmapheight + losmapwidth * losmapwidth);
            int unitlosradius = (int)unitdef.losRadius; // this is in map / 2 units, so it's ok
            Int2[] circlepoints = CreateCirclePoints(unitlosradius);
            int bestradius = 10000000;
            int bestarea = 0;
            Float3 bestpos = null;
            int unitmapx = (int)(currentpos.x / 16);
            int unitmapy = (int)(currentpos.y / 16);
            int thisframecount = aicallback.GetCurrentFrame();
            // step around in unitlosradius steps
            for (int radiuslosunits = unitlosradius * 2; radiuslosunits <= maxradius; radiuslosunits += unitlosradius)
            {
                // calculate angle for a unitlosradius / 2 step at this radius.
                // DrawingUtils.DrawCircle(currentpos, radiuslosunits * 16);

                double anglestepradians = 2 * Math.Asin((double)unitlosradius / 2 / (double)radiuslosunits);
                //csai.DebugSay("anglestepradians: " + anglestepradians);
                //return null;
                for (double angleradians = 0; angleradians <= Math.PI * 2; angleradians += anglestepradians)
                {
                    int unseenarea = 0;
                    int searchmapx = unitmapx + (int)((double)radiuslosunits * Math.Cos(angleradians));
                    int searchmapy = unitmapy + (int)((double)radiuslosunits * Math.Sin(angleradians));
                    if (searchmapx >= 0 && searchmapy >= 0 && searchmapx < (mapwidth / 2) && searchmapy < (mapheight / 2))
                    {
                        // if (csai.DebugOn)
                        //  {
                        //      int groupnumber = DrawingUtils.DrawCircle(new Float3(searchmapx * 16, 50 + aicallback.GetElevation( searchmapx * 16, searchmapy * 16 ), searchmapy * 16), unitlosradius * 16);
                        //     aicallback.SetFigureColor(groupnumber, 1, 1, 0, 0.5);
                        // }

                        int thisareanumber = MovementMaps.GetInstance().GetArea(unitdef, new Float3(searchmapx * 16, 0, searchmapy * 16));
                        if (thisareanumber == currentunitarea)
                        {//
                            //if (csai.DebugOn)
                            // {
                            //     int groupnumber = DrawingUtils.DrawCircle(new Float3(searchmapx * 16, 100, searchmapy * 16), unitlosradius * 16);
                            //     aicallback.SetFigureColor(groupnumber, 1, 1, 0, 0.5);
                            // }
                            foreach (Int2 point in circlepoints)
                            {
                                int thismapx = searchmapx + point.x;
                                int thismapy = searchmapy + point.y;
                                if (thismapx >= 0 && thismapy >= 0 && thismapx < mapwidth / 2 && thismapy < mapheight / 2)
                                {
                                    if (thisframecount - losmap.LastSeenFrameCount[thismapx, thismapy] > unseensmeansthismanyframes)
                                    {
                                        unseenarea++;
                                    }
                                }
                            }
                            if (unseenarea >= (circlepoints.GetUpperBound(0) + 1) * 8 / 10)
                            {
                                int groupnumber = DrawingUtils.DrawCircle(new Float3(searchmapx * 16, 100 * aicallback.GetElevation(searchmapx * 16, searchmapy * 16), searchmapy * 16), unitlosradius * 16);
                                aicallback.SetFigureColor(groupnumber, 1, 0, 1, 0.5);
                                return new Float3(searchmapx * 16, 0, searchmapy * 16);
                            }
                            // return new Float3(searchmapx * 16, 0, searchmapy * 16); // for debugging, remove later
                        }
                    }
                }
            }
            return null;
        }

        // returns points for a circle centered at origin of radius radius
        // the points are all integer points that lie in this circle
        Int2[] CreateCirclePoints(int radius)
        {
            ArrayList pointsal = new ArrayList();
            for (int y = -radius; y <= radius; y++)
            {
                int xextent = (int)Math.Sqrt(radius * radius - y * y);
                for (int x = -xextent; x <= xextent; x++)
                {
                    pointsal.Add(new Int2(x, y));
                }
            }
            return (Int2[])pointsal.ToArray(typeof(Int2));
        }

    }
}
