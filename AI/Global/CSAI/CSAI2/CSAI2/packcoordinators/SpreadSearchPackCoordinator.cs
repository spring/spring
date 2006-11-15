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
    // this carries out a spread search: sends all units around randomly
    // can be used by scoutcontroller, or by tankcontroller, for example
    public class SpreadSearchPackCoordinator : IPackCoordinator
    {
        Dictionary<int,IUnitDef> UnitDefListByDeployedId;
        Float3 targetpos;
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        Random random = new Random();
        
        // can pass in pointer to a hashtable in another class if we want
        // ie other class can directly modify our hashtable
        public SpreadSearchPackCoordinator(Dictionary<int, IUnitDef> UnitDefListByDeployedId)
        {
            this.UnitDefListByDeployedId = UnitDefListByDeployedId;
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            debugon = csai.DebugOn;
            
            csai.TickEvent += new CSAI.TickHandler( this.Tick );
            csai.UnitIdleEvent += new CSAI.UnitIdleHandler( UnitIdle );
        }
        
        bool debugon = false;

        bool activated = false; // not started until Activate or SetTarget is called        

        // does NOT imply Activate()
        public void SetTarget( Float3 newtarget )
        {
            this.targetpos = newtarget;
            //Activate();
        }
        
        public void Activate()
        {
            if( !activated )
            {
                logfile.WriteLine( "SpreadSearchPackCoordinator initiating spreadsearch" );
                activated = true;
                restartedfrompause = true;
                Recoordinate();
            }
        }
        
        public void Disactivate()
        {
            if( activated )
            {
                activated = false;
                logfile.WriteLine( "SpreadSearchPackCoordinator shutting down" );
            }
        }
        
        bool restartedfrompause = true;
        Float3 lasttargetpos = null;
                
        void Recoordinate()
        {
            if( !activated )
            {
                return;
            }
            
            // just send each unit to random destination
            // in unit onidle, we send each unit to a new place
            foreach( int deployedid in UnitDefListByDeployedId.Keys )
            {
                //int deployedid = (int)de.Key;
                //IUnitDef unitdef = de.Value as IUnitDef;
                ExploreWith( deployedid );
            }
        }
        
        void ExploreWith( int unitid )
        {
            Float3 destination = GetRandomDestination();
            GiveOrderWrapper.GetInstance().MoveTo(unitid, destination);
        }

        Float3 GetRandomDestination()
        {
            Float3 destination = new Float3();
            destination.x = random.Next(0, aicallback.GetMapWidth() * MovementMaps.SQUARE_SIZE );
            destination.z = random.Next( 0, aicallback.GetMapHeight() * MovementMaps.SQUARE_SIZE );
            destination.y = aicallback.GetElevation( destination.x, destination.y );
            return destination;
        }

        public void UnitIdle( int unitid )
        {
            if( activated )
            {
                if( UnitDefListByDeployedId.ContainsKey( unitid ) )
                {
                    ExploreWith( unitid );
                }
            }
        }
        
        int ticks = 0;
        public void Tick()
        {
            ticks++;
            if( ticks >= 30 )
            {
                //Recoordinate();
                
                ticks = 0;
            }
        }
    }
}
