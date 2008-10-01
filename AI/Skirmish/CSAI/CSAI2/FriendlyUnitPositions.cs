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
    // caches the position of each friendly unit once an Update
    // unsure if this makes things faster or not???
    // arguably this could be integrated into UnitController??? pros/cons?
    public class FriendlyUnitPositionObserver
    {
        public Dictionary<int, Float3> PosById = new Dictionary<int, Float3>();
        public ArrayList StaticUnitIds = new ArrayList();
        public ArrayList MobileUnitIds = new ArrayList();
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        UnitController unitcontroller;
        
        //UnitController unitcontroller;
        
        static FriendlyUnitPositionObserver instance = new FriendlyUnitPositionObserver();
        public static FriendlyUnitPositionObserver GetInstance(){ return instance; }
        
        UnitDefHelp unitdefhelp;
        
        FriendlyUnitPositionObserver()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            //csai.UnitFinishedEvent += new CSAI.UnitFinishedHandler( this.NewUnitFinished );
            //csai.UnitDestroyedEvent += new CSAI.UnitDestroyedHandler( this.UnitDestroyed );   
            
            csai.TickEvent += new CSAI.TickHandler( Tick );
            
            unitcontroller = UnitController.GetInstance();
            unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
            unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
            
            unitdefhelp = new UnitDefHelp( aicallback );
        }
        
        void UpdatePoses()
        {
          //  logfile.WriteLine( "FriendlyUnitPositionObserver start UpdatePoses" );
            foreach( object idobject in MobileUnitIds )
            {
                int id = (int)idobject;
                Float3 newpos = aicallback.GetUnitPos( id );
                PosById[ id ] = newpos;
            }
         //   logfile.WriteLine( "FriendlyUnitPositionObserver end UpdatePoses" );
        }
        
        public void UnitAdded( int id, IUnitDef unitdef )
        {
            if( !PosById.ContainsKey( id ) )
            {
                PosById.Add( id, aicallback.GetUnitPos( id ) );
                if( unitdefhelp.IsMobile( unitdef ) )
                {
                    MobileUnitIds.Add( id );
                }
                else
                {
                    StaticUnitIds.Add( id );
                }
            }
        }
        
        public void UnitRemoved( int id )
        {
            if( PosById.ContainsKey( id ) )
            {
                PosById.Remove( id );
            }
            if( StaticUnitIds.Contains( id ) )
            {
                StaticUnitIds.Remove( id );
            }
            if( MobileUnitIds.Contains( id ) )
            {
                MobileUnitIds.Remove( id );
            }
        }
        
        public void Tick()
        {
            UpdatePoses();
        }
    }
}
