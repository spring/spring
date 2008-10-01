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
    public class PlayStyle : IPlayStyle
    {
        protected CSAI csai;
        protected IAICallback aicallback;
        protected LogFile logfile;
        
        public bool IsActive = false;        
        protected ArrayList controllers = new ArrayList();
        
        public PlayStyle()
        {  
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();

            PlayStyleManager.GetInstance().RegisterPlayStyle( this );            
        }
        
        public virtual string GetName(){ return ""; }
        public virtual string GetDescription(){ return ""; }
        
        // so can be used in interface
        public ArrayList Controllers
        {
            get{
                return controllers;
            }
        }
                
        public IController GetFirstControllerOfType( Type targettype )
        {
            logfile.WriteLine( this.GetType().ToString() + " GetFirstControllerOfType");
            foreach( object controllerobject in controllers )
            {
                logfile.WriteLine( controllerobject.GetType().ToString() + " " + targettype.ToString() );
                IController controller = controllerobject as IController;
                if( targettype.IsInstanceOfType( controller ) )
                {
                    return controller;
                }
            }
            return null;
        }
        
        public IController[] GetControllersOfType( Type targettype )
        {
            logfile.WriteLine( this.GetType().ToString() + " GetFirstControllerOfType");
            ArrayList controllersal = new ArrayList();
            foreach( object controllerobject in controllers )
            {
                logfile.WriteLine( controllerobject.GetType().ToString() + " " + targettype.ToString() );
                IController controller = controllerobject as IController;
                if( targettype.IsInstanceOfType( controller ) )
                {
                    controllersal.Add( controller );
                }
            }
            return controllersal.ToArray( typeof( IController) ) as IController[];
        }
        
        public void Activate()
        {
            if( !IsActive )
            {
                logfile.WriteLine( this.GetType().ToString() + " activating" );
                
                foreach( object controllerobject in controllers )
                {
                    IController controller = controllerobject as IController;
                    controller.Activate();
                }
                
                IsActive = true;
            }
        }
        
        public void Disactivate()
        {
            if( IsActive )
            {
                logfile.WriteLine( this.GetType().ToString() + " deactivating" );

                foreach( object controllerobject in controllers )
                {
                    IController controller = controllerobject as IController;
                    controller.Disactivate();
                }
                
                IsActive = false;
            }
        }
    }
}

