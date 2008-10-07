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
using System.Collections;
using Microsoft.Win32;
using System.Windows.Forms;
using System.IO;

using ajma.Utils; // for inputbox

// This is used to install CSAI

class Installer
{
    string installerdirectory = "";
    const int springfilesize = 3502080; // used to verify we are using correct version of spring
    
    // list of source file and destination path pairs, one pair per line
    string[]filedestinationpairs = new string[]{
        "CSAIInterfaces/CSAIInterfaces.dll", "",
        "CSAIInterfaces/CSAIInterfaces.pdb", "",
        "CSAI/CSAI.dll", "AI/CSAI",
        "CSAI/CSAI.pdb", "AI/CSAI", 
        "CSAILoader/csailoader.dll", "AI/Skirmish/impls",
        "CSAILoader/csailoader.xml", "AI/Skirmish/impls" };
    
    string GetSpringInstallDirectory()
    {
        RegistryKey springclientkey = Registry.LocalMachine.OpenSubKey( @"Software\Microsoft\Windows\CurrentVersion\App Paths\SpringClient.exe", false );
        string springclientstring = springclientkey.GetValue("").ToString();
        string[] splitspringclientstring = springclientstring.Split(@"\".ToCharArray() );
        string springclientfilename = splitspringclientstring[ splitspringclientstring.GetUpperBound(0) ];
        string springapplicationdirectory = springclientstring.Substring( 0, springclientstring.Length - springclientfilename.Length - 1 );
        //Console.WriteLine( "[" + springapplicationdirectory + "]" );
        return springapplicationdirectory;
    }
    
    void InstallFiles( string applicationdirectory )
    {
        for( int i = 0; i < filedestinationpairs.GetUpperBound(0) + 1; i += 2 )
        {
            string sourcefilepath = filedestinationpairs[i];
            string destinationdirectory = filedestinationpairs[i + 1];
            string filename = Path.GetFileName( sourcefilepath );
            
            Directory.CreateDirectory( Path.Combine( applicationdirectory, destinationdirectory ) );
            
            string sourcefullfilepath = Path.Combine( installerdirectory, sourcefilepath );
            string destinationfullfilepath = Path.Combine( Path.Combine( applicationdirectory, destinationdirectory ), filename );
            
            try
            {
                Console.WriteLine( "copying " + sourcefilepath + " to " + destinationfullfilepath );
                File.Copy( sourcefilepath, destinationfullfilepath, true );
            }
            catch( Exception )
            {
                MessageBox.Show( "Couldnt copy " + sourcefilepath + " to " + destinationfullfilepath + ".  Please check Spring is not running, then try again" );
                System.Environment.Exit(1);
            }
        }
    }
    
    public void Go()
    {
        installerdirectory = Application.StartupPath;
        //Console.WriteLine( Application.StartupPath );
        
        //CheckFramework(); // Hmmm, setup wont run without framework, no need to check :-D
        
        string SpringInstallDirectory = GetSpringInstallDirectory();
        
        InputBoxForm frmInputBox = new InputBoxForm();
        frmInputBox.Title = "Locate Spring Application directory";
        frmInputBox.Prompt = "Please confirm the Spring Application directory:";
        frmInputBox.DefaultResponse = SpringInstallDirectory;
        //if (XPos >= 0 && YPos >= 0)
        //{
          //  frmInputBox.StartLocation = new Point(XPos, YPos);
        //}
        frmInputBox.ShowDialog();
        SpringInstallDirectory = frmInputBox.ReturnValue;
        
        if( SpringInstallDirectory == "" )
        {
            MessageBox.Show( "No directory specified.  Aborting setup.exe." );
            return;
        }
        
        if( !Directory.Exists( SpringInstallDirectory ) )
        {
            MessageBox.Show( "Directory " + SpringInstallDirectory + " does not exist.  Please run setup again." );
            return;
        }
        
        string springexefilepath = Path.Combine( SpringInstallDirectory, "Spring.exe" );
        if( !File.Exists( springexefilepath ) )
        {
            MessageBox.Show( "Spring.exe not found in " + SpringInstallDirectory + ".  Please rerun setup." );
            return;
        }
        
        FileInfo springfile = new FileInfo( springexefilepath );
        if( springfile.Length != 3502080 )
        {
            MessageBox.Show( "Spring.exe appears to be a different version than what we are looking for.  Please reinstall the latest version of Spring, and try again." );
            return;
        }
        
        Console.WriteLine( SpringInstallDirectory );
        
        InstallFiles( SpringInstallDirectory );
        
        MessageBox.Show( "Installation completed successfully.  Please add bot \"csailoader.dll\" to your games to use CSAI" );
    }
}

class entrypoint
{
    public static void Main()
    {
        new Installer().Go();
    }
}
