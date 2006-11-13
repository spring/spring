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
    // in charge of determining when we last had LOS to each part of the map
    // what we can do is to go through each of our units, get its radar radius, and check it off against map
    // we obviously dont need to do this very often for static units
    // slower movements (tanks) probably need this less often than faster ones (scouts, planes)
    // so: three timeframes:
    // - static, at creation and thereafter every 300 frames or so
    // - tanks, once a second (30 frames)
    // - fast units, once an update (?)
    
    // Update: we cache the position of each unit at the time we wrote its losprint onto the losmap
    // we only update the lospring from a unit when it has moved significantly, or a lot of time has passed
    //
    // possible optimization: dont do every unit, check when units are close to others, using a sector by sector index (caveat: need to check losradius is equivalent, or sort by losradius)
    //
    // why dont we just copy the spring-generated losmap each frame?  Well, because that would be slower(?)
    public class LosMap
    {
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        UnitController unitcontroller;
        FriendlyUnitPositionObserver friendlyunitpositionobserver;
        
        public int losrefreshallintervalframecount = 2000;
        public int distancethresholdforunitupdate = 300; // if unit moved more than this, in pos, we refresh its losprint, otherwise we ignore it for now
        //public int loscheckslowinframes = 30;
        //public int loscheckfastinupdates = 1;
        
        public Hashtable LastLosRefreshFrameCountByUnitId = new Hashtable(); // all units here, just dont update static ones very often
        public Hashtable PosAtLastRefreshByUnitId = new Hashtable();
        public int[,] LastSeenFrameCount; // map, mapwidth / 2 by mapheight / 2
        int lasttotalrefresh = -100000;
        //int lastloscheckslow = -100000;        
        
        //public int morethanthisspeedisfast = 100;
        
        int mapwidth;
        int mapheight;
        
        static LosMap instance = new LosMap();
        public static LosMap GetInstance(){ return instance; }
        
        LosMap()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();  
            
            unitcontroller = UnitController.GetInstance();
            unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
            unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
            
            friendlyunitpositionobserver = FriendlyUnitPositionObserver.GetInstance();
            
            csai.TickEvent += new CSAI.TickHandler( Tick );
            
            mapwidth = aicallback.GetMapWidth();
            mapheight = aicallback.GetMapHeight();
            
            logfile.WriteLine( "LosMap, create losarray" );
            LastSeenFrameCount = new int[ mapwidth / 2, mapheight / 2 ];
            logfile.WriteLine( "losarray created, initializing..." );
            for( int y = 0; y < mapheight / 2; y++ )
            {
                for( int x = 0; x < mapwidth / 2; x++ )
                {
                    LastSeenFrameCount[ x, y ] = -1000000;
                }
            }
            logfile.WriteLine( "losarray initialized" );
        }
        
        void DoIncrementalRefreshes()
        {
            logfile.WriteLine( "LosMap start incrementalrefreshes" );
            int numupdates = 0;
            int thresholddistancesquared = distancethresholdforunitupdate * distancethresholdforunitupdate;
            foreach( object mobileidobject in friendlyunitpositionobserver.MobileUnitIds )
            {
                int id = (int)mobileidobject;
                Float3 currentpos = friendlyunitpositionobserver.PosById[ id ] as Float3;
                Float3 lastpos = PosAtLastRefreshByUnitId[ id ] as Float3;
                double distancesquared = Float3Helper.GetSquaredDistance( lastpos, currentpos );
                if( distancesquared > thresholddistancesquared )
                {
                    UpdateLosForUnit( id, unitcontroller.UnitDefByDeployedId[ id ] as IUnitDef );
                    numupdates++;
                }
            }
            logfile.WriteLine( "LosMap end incrementalrefreshes, " + numupdates + " units refreshed" );
        }
        
        void UpdateLosForUnit( int unitid, IUnitDef unitdef )
        {
            int thisframecount = aicallback.GetCurrentFrame();
            Float3 pos = friendlyunitpositionobserver.PosById[ unitid ] as Float3;
            int seenmapx = (int)( pos.x / 16 );
            int seenmapy = (int)( pos.z / 16 );
            // int radius = (int)( unitdef.losRadius / 8 / 2 );
            int radius = (int)unitdef.losRadius;
            if( csai.DebugOn )
            {
                //aicallback.SendTextMsg( "Updating los for " + unitid + " " + unitdef.humanName + " los radius " + radius, 0 );
                DrawingUtils.DrawCircle( pos, radius * 16 );
            }
            logfile.WriteLine( "Updating los for " + unitid + " " + unitdef.humanName + " los radius " + radius + " pos " + pos.ToString() );
            // go line by line, determine positive and negative extent of line, mark lostime
            for( int deltay = -radius; deltay <= radius; deltay++ )
            {
                int xextent = (int)Math.Sqrt( radius * radius - deltay * deltay );
                for( int deltax = -xextent; deltax <= xextent; deltax++ )
                {
                    int thisx = seenmapx + deltax;
                    int thisy = seenmapy + deltay;
                    if( thisx >= 0 && thisx < mapwidth / 2 && thisy >= 0 && thisy < mapheight / 2 )
                    {
                        LastSeenFrameCount[ thisx, thisy ] = thisframecount;
                    }
                }
                // in progress
            }
            LastLosRefreshFrameCountByUnitId[ unitid ] = thisframecount;
            PosAtLastRefreshByUnitId[ unitid ] = pos;
            logfile.WriteLine( "...done" );
        }
        
        void TotalRefresh()
        {
            logfile.WriteLine( "LosMap start totalrefresh" );
            foreach( DictionaryEntry de in unitcontroller.UnitDefByDeployedId )
            {
                int id = (int)de.Key;
                IUnitDef unitdef = de.Value as IUnitDef;
                UpdateLosForUnit( id, unitdef );
            }
            logfile.WriteLine( "LosMap end totalrefresh" );
        }
        
        public void UnitAdded( int id, IUnitDef unitdef )
        {
            if( !LastLosRefreshFrameCountByUnitId.Contains( id ) )
            {
                LastLosRefreshFrameCountByUnitId.Add( id, aicallback.GetCurrentFrame() );
                PosAtLastRefreshByUnitId.Add( id, aicallback.GetUnitPos( id ) );
                UpdateLosForUnit( id, unitdef );
            }
        }
        
        public void UnitRemoved( int id )
        {
            if( LastLosRefreshFrameCountByUnitId.Contains( id ) )
            {
                LastLosRefreshFrameCountByUnitId.Remove( id );
                PosAtLastRefreshByUnitId.Remove( id );
            }
        }
        
        public void Tick()
        {
            DoIncrementalRefreshes();
            
            if( aicallback.GetCurrentFrame() - lasttotalrefresh > losrefreshallintervalframecount )
            {
                TotalRefresh();
                lasttotalrefresh = aicallback.GetCurrentFrame();
            }
        }

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
            int currentunitarea = MovementMaps.GetInstance().GetArea(unitdef, currentpos);
            int losmapwidth = LastSeenFrameCount.GetUpperBound(0) + 1;
            int losmapheight = LastSeenFrameCount.GetUpperBound(0) + 1;
            int maxradius = Math.Max(losmapheight, losmapwidth);
            int unitlosradius = (int)unitdef.losRadius; // this is in map / 2 units, so it's ok
            Int2[]circlepoints = CreateCirclePoints( unitlosradius );
            int bestradius = 10000000;
            int bestarea = 0;
            Float3 bestpos = null;
            int unitmapx = (int)( currentpos.x / 16 );
            int unitmapy = (int)( currentpos.y / 16 );
            int thisframecount = aicallback.GetCurrentFrame();
            // step around in unitlosradius / 2 steps
            for (int radius = unitlosradius * 2; radius <= maxradius; radius += unitlosradius / 2 )
            {
                // calculate angle for a unitlosradius / 2 step at this radius.
                double anglestepradians = 2 * Math.Asin( unitlosradius / 2 / 2 / radius );
                for (double angleradians = 0; angleradians <= Math.PI * 2; angleradians += anglestepradians)
                {
                    int unseenarea = 0;
                    int searchmapx = unitmapx + (int)( (double)radius * Math.Cos( angleradians ) );
                    int searchmapy = unitmapy + (int)( (double)radius * Math.Sin( angleradians ) );
                    int thisareanumber = MovementMaps.GetInstance().GetArea(unitdef, new Float3( searchmapx * 16, 0, searchmapy * 16));
                    if (thisareanumber == currentunitarea)
                    {
                        if (csai.DebugOn)
                        {
                            DrawingUtils.DrawCircle(new Float3(searchmapx * 16, 100, searchmapy * 16), unitlosradius);
                        }
                        foreach (Int2 point in circlepoints)
                        {
                            if (thisframecount - LastSeenFrameCount[searchmapx + point.x, searchmapx + point.y] > unseensmeansthismanyframes)
                            {
                                unseenarea++;
                            }
                        }
                        if (unseenarea >= (circlepoints.GetUpperBound(0) + 1) * 8 / 10)
                        {
                            return new Float3(searchmapx * 16, 0, searchmapy * 16);
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
