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
    // this moves the units to a particular point
    // the point is assumed to be friendly; no particular attack pattern is assumed
    public class MoveToPackCoordinator : IPackCoordinator
    {
        Hashtable UnitDefListByDeployedId;
        Float3 targetpos;
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        // can pass in pointer to a hashtable in another class if we want
        // ie other class can directly modify our hashtable
        public MoveToPackCoordinator( Hashtable UnitDefListByDeployedId )
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
                activated = true;
                restartedfrompause = true;
                Recoordinate();
            }
        }
        
        public void Disactivate()
        {
            activated = false;
        }
        
        bool restartedfrompause = true;
        Float3 lasttargetpos = null;
                
        void Recoordinate()
        {
            if( !activated )
            {
                return;
            }

            if( restartedfrompause || Float3Helper.GetSquaredDistance(targetpos, lasttargetpos ) > ( 20 * 20 ) )
            {
                foreach( DictionaryEntry de in UnitDefListByDeployedId )
                {
                    int deployedid = (int)de.Key;
                    IUnitDef unitdef = de.Value as IUnitDef;
                    Move( deployedid );
                }
                lasttargetpos = targetpos;
            }
        }
        
        void Move( int unitid )
        {
            aicallback.GiveOrder( unitid, new Command( Command.CMD_MOVE, targetpos.ToDoubleArray() ) );
        }
        
        public void UnitIdle( int unitid )
        {
            if( activated )
            {
                if( UnitDefListByDeployedId.Contains( unitid ) )
                {
                    Move( unitid );
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
