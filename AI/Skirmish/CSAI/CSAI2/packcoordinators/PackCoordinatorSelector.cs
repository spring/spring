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
    // use this if your class uses many packcoordinators, and you want to switch easily between them
    public class PackCoordinatorSelector
    {
        ArrayList packcoordinators = new ArrayList();
        
        LogFile logfile;
        
        public PackCoordinatorSelector()
        {
            logfile = LogFile.GetInstance();
        }
        
        // calling class calls this for each packcoordinator it wants to use
        public void LoadCoordinator( IPackCoordinator packcoordinator )
        {
            if( !packcoordinators.Contains( packcoordinator ) )
            {
                packcoordinators.Add( packcoordinator );
            }
        }
        
        IPackCoordinator activecoordinator = null;
        
        // calling class runs this to activate packcoordinator
        public void ActivatePackCoordinator( IPackCoordinator packcoordinator )
        {
            if( !packcoordinators.Contains( packcoordinator ) )
            {
                throw new Exception( "PackCoordinatorSelector: pack coordinator " + packcoordinator.ToString() + " not found" );
            }
            
            if( activecoordinator != packcoordinator )
            {
                logfile.WriteLine( "PackCoordinator selector: changing to coordinator " + packcoordinator.ToString() );
                // first disactivate others, then activate new one
                // the order of operations is important since they will probalby be using GiveORder, and GiveOrder overwrites old orders with new ones
                foreach( object packcoordinatorobject in packcoordinators )
                {
                    IPackCoordinator thispackcoordinator = packcoordinatorobject as IPackCoordinator;
                    if( thispackcoordinator != packcoordinator )
                    {
                        thispackcoordinator.Disactivate();
                    }
                }
                foreach( object packcoordinatorobject in packcoordinators )
                {
                    IPackCoordinator thispackcoordinator = packcoordinatorobject as IPackCoordinator;
                    if( thispackcoordinator == packcoordinator )
                    {
                        thispackcoordinator.Activate();
                    }
                }
                activecoordinator = packcoordinator;
            }
        }
        
        public void DisactivateAll()
        {
            if( activecoordinator != null )
            {
                logfile.WriteLine( "PackCoordinator selector: disactivating all" );
                foreach( object packcoordinatorobject in packcoordinators )
                {
                    IPackCoordinator thispackcoordinator = packcoordinatorobject as IPackCoordinator;
                    thispackcoordinator.Disactivate();
                }
                activecoordinator = null;
            }
        }
    }
}
