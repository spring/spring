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
    // this manages an attack pack, eg group of tanks
    // specifically ,ensures they stay together, rather than streaming
    // in theory
    public class AttackPackCoordinator : IPackCoordinator
    {
        public double MaxPackDiameter = 250;
        public double movetothreshold = 20;
        public int MinPackSize = 5;
        
        Hashtable UnitDefListByDeployedId;
        Float3 targetpos;
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        //public delegate void AttackPackDeadHandler();
        
        //public event AttackPackDeadHandler AttackPackDeadEvent;
        
        // can pass in pointer to a hashtable in another class if we want
        // ie other class can directly modify our hashtable
        public AttackPackCoordinator( Hashtable UnitDefListByDeployedId )
        {
            this.UnitDefListByDeployedId = UnitDefListByDeployedId;
            
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            debugon = csai.DebugOn;
            
            csai.TickEvent += new CSAI.TickHandler( this.Tick );
        }
        
        bool debugon = false;

        bool started = false; // not started until Activate or SetTarget is called        

        // does NOT imply Activate()
        public void SetTarget( Float3 newtarget )
        {
            this.targetpos = newtarget;
            //Activate();
        }
        
        public void Activate()
        {
            if( !started )
            {
                started = true;
                restartedfrompause = true;
                Recoordinate();
            }
        }
        
        public void Disactivate()
        {
            started = false;
        }
        
        bool restartedfrompause = true;
        Float3 lasttargetpos = null;
                
        void Recoordinate()
        {
            if( !started )
            {
                return;
            }
            
            // first we scan the list to find the 5 closest units to target, in order
            // next we measure distance of first unit from 5th
            // if distance is less than MaxPackDiameter, we move in
            // otherwise 5th unit holds, and we move units to position of 5th unit
            // pack must have at least 5 units, otherwise we call AttackPackDead event
            
            if( !CheckIfPackAlive() )
            {
                return;
            }
            
            UnitInfo[] closestunits = GetClosestUnits( UnitDefListByDeployedId, targetpos, 5 );
            Float3 packheadpos = closestunits[0].pos;
            Float3 packtailpos = closestunits[MinPackSize - 1].pos;
            double packsquareddiameter = Float3Helper.GetSquaredDistance( packheadpos, packtailpos );
           // logfile.WriteLine( "AttackPackCoordinator packheadpos " + packheadpos.ToString() + " packtailpos " + packtailpos.ToString() + " packsquareddiamter " + packsquareddiameter );
            if( packsquareddiameter < ( MaxPackDiameter * MaxPackDiameter ) )
            {
                Attack();
            }
            else
            {
                Regroup( closestunits[( MinPackSize / 2 )].pos );
            }
        }
        
        void Attack()
        {
            //logfile.WriteLine( "AttackPackCoordinator attacking" );
            //if( debugon )
           // {
          //      aicallback.SendTextMsg( "AttackPackCoordinator attacking", 0 );
           // }
            MoveTo( targetpos );
        }
        
        void Regroup( Float3 regouppos )
        {
         //   logfile.WriteLine( "AttackPackCoordinator regrouping to " + regouppos.ToString() );
          //  if( debugon )
        //    {
        //        aicallback.SendTextMsg( "AttackPackCoordinator regrouping to " + regouppos.ToString(), 0 );
         //   }
            MoveTo( regouppos );
        }
        
        void MoveTo( Float3 pos )
        {
            // check whether we really need to do anything or if order is roughly same as last one
            if( csai.DebugOn )
            {
                aicallback.DrawUnit("ARMSOLAR", pos, 0.0f, 50, aicallback.GetMyAllyTeam(), true, true);
            }
            if( restartedfrompause || Float3Helper.GetSquaredDistance( pos, lasttargetpos ) > ( movetothreshold * movetothreshold ) )
            {
                foreach( DictionaryEntry de in UnitDefListByDeployedId )
                {
                    int deployedid = (int)de.Key;
                    IUnitDef unitdef = de.Value as IUnitDef;
                    aicallback.GiveOrder( deployedid, new Command( Command.CMD_MOVE, pos.ToDoubleArray() ) );
                }
                restartedfrompause = false;
                lasttargetpos = pos;
            }
        }

        bool CheckIfPackAlive()
        {
            if( UnitDefListByDeployedId.Count < MinPackSize )
            {
           //     if( AttackPackDeadEvent != null )
             //   {
               //     AttackPackDeadEvent();
               // }
                return false;
            }
            return true;
        }
        
        class UnitInfo
        {
            public int deployedid;
            public Float3 pos;
            public IUnitDef unitdef;
            public double squareddistance;
            public UnitInfo(){}
            public UnitInfo( int deployedid, Float3 pos, IUnitDef unitdef, double squareddistance )
            {
                this.deployedid = deployedid;
                this.pos = pos;
                this.unitdef = unitdef;
                this.squareddistance = squareddistance;
            }
        }
        
        UnitInfo[] GetClosestUnits( Hashtable UnitDefListByDeployedId, Float3 targetpos, int numclosestunits )
        {
            UnitInfo[] closestunits = new UnitInfo[ numclosestunits ];
            double worsttopfivesquareddistance = 0; // got to get better than this to enter the list
            int numclosestunitsfound = 0;
            foreach( DictionaryEntry de in UnitDefListByDeployedId )
            {
                int deployedid = (int)de.Key;
                IUnitDef unitdef = de.Value as IUnitDef;
                Float3 unitpos = aicallback.GetUnitPos( deployedid );
                double unitsquareddistance = Float3Helper.GetSquaredDistance( unitpos, targetpos );
                if( numclosestunitsfound < numclosestunits )
                {
                    UnitInfo unitinfo = new UnitInfo( deployedid, unitpos, unitdef, unitsquareddistance );
                    InsertIntoArray( closestunits, unitinfo, numclosestunitsfound );
                    numclosestunitsfound++;
                    worsttopfivesquareddistance = closestunits[ numclosestunitsfound - 1 ].squareddistance;
                }
                else if( unitsquareddistance < worsttopfivesquareddistance )
                {
                    UnitInfo unitinfo = new UnitInfo( deployedid, unitpos, unitdef, unitsquareddistance );
                    InsertIntoArray( closestunits, unitinfo, numclosestunits );
                    worsttopfivesquareddistance = closestunits[ numclosestunits - 1 ].squareddistance;
                }
            }
            return closestunits;
        }
        
        // we add it to the bottom, then bubble it up
        void InsertIntoArray( UnitInfo[] closestunits, UnitInfo newunit, int numexistingunits )
        {
            if( numexistingunits < closestunits.GetUpperBound(0) + 1 )
            {
                closestunits[ numexistingunits ] = newunit;
                numexistingunits++;
            }
            else
            {
                closestunits[ numexistingunits - 1 ] = newunit;
            }
            // bubble new unit up
            for( int i = numexistingunits - 2; i >= 0; i-- )
            {
                if( closestunits[ i ].squareddistance > closestunits[ i + 1 ].squareddistance )
                {
                    UnitInfo swap = closestunits[ i ];
                    closestunits[ i ] = closestunits[ i + 1 ];
                    closestunits[ i + 1 ] = swap;  
                }
            }
            // debug;
          //  logfile.WriteLine( "AttackPackCoordinator.InsertIntoArray");
          //  for( int i = 0; i < numexistingunits; i++ )
         //   {
         //       logfile.WriteLine(i + ": " + closestunits[ i ].squareddistance );
         //   }
        }
        
        int ticks = 0;
        public void Tick()
        {
            ticks++;
            if( ticks >= 30 )
            {
                Recoordinate();
                
                ticks = 0;
            }
        }
    }
}
