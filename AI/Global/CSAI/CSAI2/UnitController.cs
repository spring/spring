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
using System.Collections.Generic;

namespace CSharpAI
{
    // The role of UnitController is to keep track of what units we have
    // Really it's just a wrapper for unitfinished and unitdestroyed events from csai.
    // Note to self: do we really need this class???
    public class UnitController
    {
        public delegate void ExistingUnitHandler(int deployedid, IUnitDef unitdef);
        public delegate void UnitAddedHandler(int deployedid, IUnitDef unitdef);
        public delegate void UnitRemovedHandler( int deployedid );

        public event ExistingUnitHandler ExistingUnitEvent;
        public event UnitAddedHandler UnitAddedEvent;
        public event UnitRemovedHandler UnitRemovedEvent;
        
        static UnitController instance = new UnitController();
        public static UnitController GetInstance(){ return instance; }
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        UnitDefHelp unitdefhelp;
        
        //public Hashtable UnitDefByDeployedId = new Hashtable();
        public Dictionary<int,IUnitDef> UnitDefByDeployedId = new Dictionary<int,IUnitDef>();
        public Dictionary<string, List<IUnitDef>> UnitDefsByName = new Dictionary<string, List<IUnitDef>>();
        
        // do we need this???  handled by specific controllers???
        //IntArrayList commanders = new IntArrayList();
        //IntArrayList constructors = new IntArrayList();
        //IntArrayList metalcollectors = new IntArrayList();
        //IntArrayList energycollectors = new IntArrayList();
        //IntArrayList groundattack = new IntArrayList();
        
        UnitController()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            unitdefhelp = new UnitDefHelp( aicallback );
            
            csai.UnitFinishedEvent += new CSAI.UnitFinishedHandler( this.NewUnitFinished );
            csai.UnitDestroyedEvent += new CSAI.UnitDestroyedHandler( this.UnitDestroyed );   

            csai.RegisterVoiceCommand( "killallfriendly", new CSAI.VoiceCommandHandler( this.VoiceCommandKillAllFriendly ) );
            csai.RegisterVoiceCommand( "countunits", new CSAI.VoiceCommandHandler( this.VoiceCommandCountUnits ) );
            
            logfile.WriteLine ("*UnitController initialized*");
        }
        
        public void VoiceCommandCountUnits( string cmd, string[]splitcmd, int player )
        {
            aicallback.SendTextMsg( "friendly unit count: " + UnitDefByDeployedId.Count, 0 );
        }

        public delegate void AllUnitsLoadedHandler();
        public event AllUnitsLoadedHandler AllUnitsLoaded;
        
        public void LoadExistingUnits()
        {
            int[] friendlyunits = aicallback.GetFriendlyUnits();	
            foreach( int friendlyunit in friendlyunits )
            {
                IUnitDef unitdef = aicallback.GetUnitDef( friendlyunit );
                logfile.WriteLine("friendly unit existing: " + friendlyunit + " " + unitdef.name + " " + unitdef.humanName );
                AddUnit( friendlyunit );
                if (ExistingUnitEvent != null)
                {
                    ExistingUnitEvent(friendlyunit, unitdef);
                }
            }
            if (AllUnitsLoaded != null)
            {
                AllUnitsLoaded();
            }
        }
        
        public void RefreshMyMemory( UnitAddedHandler unitadded )
        {
            foreach( KeyValuePair<int,IUnitDef> kvp in UnitDefByDeployedId )
            {
                int deployedid = kvp.Key;
                IUnitDef unitdef = kvp.Value;
                unitadded( deployedid, unitdef );
            }
        }
        
        void AddUnit( int friendlyunit )
        {
            IUnitDef unitdef = aicallback.GetUnitDef( friendlyunit );
            if( !UnitDefByDeployedId.ContainsKey( friendlyunit ) )
            {
                logfile.WriteLine("UnitController.AddUnit: unit id " + friendlyunit.ToString() + " " + unitdef.humanName );
                string name = unitdef.name.ToLower();
                UnitDefByDeployedId.Add( friendlyunit, unitdef );
                if( !UnitDefsByName.ContainsKey( name ) )
                {
                    UnitDefsByName.Add( name, new List<IUnitDef>() );
                }
                UnitDefsByName[name].Add(unitdef);
                
                if( UnitAddedEvent != null )
                {
                    UnitAddedEvent( friendlyunit, unitdef );
                }
                logfile.WriteLine("UnitController.AddUnit finished");
            }
            else
            {
                logfile.WriteLine( "UnitController.AddUnit: unit id " + friendlyunit.ToString() + " " + unitdef.humanName + " already exists" );
            }
        }
        
        void RemoveUnit( int friendlyunit )
        {
            if( UnitDefByDeployedId.ContainsKey( friendlyunit ) )
            {
                IUnitDef unitdef = UnitDefByDeployedId[friendlyunit];
                string name = unitdef.name.ToLower();
                UnitDefByDeployedId.Remove(friendlyunit);
                UnitDefsByName[name].Remove(unitdef);
            }
            if (UnitRemovedEvent != null)
            {
                UnitRemovedEvent( friendlyunit );
            }
        }
        
        public void NewUnitFinished( int deployedunitid, IUnitDef unitdef )
        {
            logfile.WriteLine( "UnitController.NewUnitFinished " + unitdef.humanName + " " + deployedunitid );
            AddUnit( deployedunitid );
        }
        
        public void UnitDestroyed( int deployedunitid, int enemyid )
        {
            logfile.WriteLine( "UnitController.UnitDestroyed " + deployedunitid );
            RemoveUnit( deployedunitid );
        }
        
        // kills all except commander, allows testing expansion from nothing, without having to relaod spring.exe
        void VoiceCommandKillAllFriendly( string cmd, string[]splitcmd, int player )
        {
            foreach( KeyValuePair< int, IUnitDef> kvp in UnitDefByDeployedId )
            {
                int id = kvp.Key;
                IUnitDef unitdef = kvp.Value;
                if( !unitdef.isCommander )
                {
                    GiveOrderWrapper.GetInstance().SelfDestruct(id);
                }
            }
        }
    }
}

