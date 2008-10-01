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
    // handles communicating with user the playstyle options, getting their choice, activating/deactivating playstyles
    public class PlayStyleManager
    {
        static PlayStyleManager instance = new PlayStyleManager();
        public static PlayStyleManager GetInstance(){ return instance; }
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;        
        
        ArrayList playstyles = new ArrayList();
        IPlayStyle currentplaystyle = null;
       // string defaultplaystylename = "tankrush";
        
        PlayStyleManager()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            csai.RegisterVoiceCommand( "showplaystyles", new CSAI.VoiceCommandHandler( this.VoiceCommandListPlayStyles ) );                        
            csai.RegisterVoiceCommand( "chooseplaystyle", new CSAI.VoiceCommandHandler( this.VoiceCommandChoosePlayStyle ) );                        
        }
        
        public IPlayStyle GetCurrentPlayStyle()
        {
            return currentplaystyle;
        }
        
        public void ListPlayStyles()
        {            
            string reply = "Play styles available: ";
            foreach( object playstyleobject in playstyles )
            {
                IPlayStyle playstyle = playstyleobject as IPlayStyle;
                reply += playstyle.GetName() + ", ";
            }
            aicallback.SendTextMsg( reply, 0 );
            aicallback.SendTextMsg( ".csai chooseplaystyle <name> to choose" , 0 );
        }
        
        public void VoiceCommandListPlayStyles( string cmd, string[]splitcmd, int player )
        {
            ListPlayStyles();
        }
        
        public void ChoosePlayStyle( string playstylename )
        {
            IPlayStyle requestedstyle = null;
            foreach( object playstyleobject in playstyles )
            {
                IPlayStyle playstyle = playstyleobject as IPlayStyle;
                if( playstyle.GetName().ToLower() == playstylename.ToLower() )
                {
                    requestedstyle = playstyle;
                }
            }
            
            if( requestedstyle != null )
            {
                aicallback.SendTextMsg( "Activating play style " + requestedstyle.GetName(), 0 ); 
                logfile.WriteLine( "Activating play style " + requestedstyle.GetName() );
            }
            else
            {
                aicallback.SendTextMsg( "Playstyle " + playstylename + " not found", 0 ); 
                return;
            }
            
            foreach( object playstyleobject in playstyles )
            {
                IPlayStyle playstyle = playstyleobject as IPlayStyle;
                if( playstyle != requestedstyle )
                {
                    playstyle.Disactivate();
                }
            }
            requestedstyle.Activate();
            currentplaystyle = requestedstyle;
            logfile.WriteLine( "ChoosePlayStyle done" );
        }
        
        public void VoiceCommandChoosePlayStyle( string cmd, string[]splitcmd, int player )
        {
            string requestedname = splitcmd[2];
            logfile.WriteLine( "player requested style " + requestedname );
            ChoosePlayStyle( requestedname );
        }
        
        public void RegisterPlayStyle( IPlayStyle playstyle )
        {
            if( !playstyles.Contains( playstyle ) )
            {
                logfile.WriteLine( "PlayStyleManager: registering playstyle " + playstyle.GetName() );
                playstyles.Add( playstyle );
                //if( playstyle.GetName().ToLower() == defaultplaystylename )
                //{
                  //  currentplaystyle = playstyle;
                   // playstyle.Activate();
               // }
            }
        }
    }
}
