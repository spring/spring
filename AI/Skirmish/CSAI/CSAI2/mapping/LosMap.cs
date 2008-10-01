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
using System.Collections.Generic;
using System.Xml;

namespace CSharpAI
{
    // in charge of determining when we last had LOS to each part of the map
    // what we can do is to go through each of our units, get its radar radius, and check it off against map
    // we obviously dont need to do this very often for static units
    // slower movements (tanks) probably need this less often than faster ones (scouts, planes)    
    //
    // We cache the position of each unit at the time we wrote its losprint onto the losmap
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

            if (csai.DebugOn)
            {
                csai.RegisterVoiceCommand("dumplosmap", new CSAI.VoiceCommandHandler(DumpLosMap));
            }
        }

        public void DumpLosMap(string cmd, string[] split, int player)
        {
            bool[,]losmap = new bool[mapwidth / 2, mapheight/ 2 ];
            for( int y = 0; y < mapheight / 2; y++ )
            {
                for( int x = 0; x < mapwidth / 2; x++ )
                {
                    losmap[x,y] = ( aicallback.GetCurrentFrame() - LastSeenFrameCount[ x, y ] < 6000 );
                }
            }
            DrawingUtils.DrawMap(losmap);
        }

        void DoIncrementalRefreshes()
        {
          //  logfile.WriteLine( "LosMap start incrementalrefreshes" );
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
         //   logfile.WriteLine( "LosMap end incrementalrefreshes, " + numupdates + " units refreshed" );
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
            foreach( KeyValuePair<int, IUnitDef>kvp in unitcontroller.UnitDefByDeployedId )
            {
                int id = kvp.Key;
                IUnitDef unitdef = kvp.Value;
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

    }
}
