// Copyright Spring project, Hugh Perkins 2006
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


// The goal of this is to read UnitDef.h and generate an approximate version of:
// - IUnitDef.cs
// - UnitDefProxy.h
//
// From there they can be tweaked by hand to add in further extra functions, and then maintained by hand
//
// Scope: the goal of this is NOT to automatically and entirely generate these files, and to automatically maintain them.  We are not aiming to rewrite Swig ;-)
//
// Note that the header and footer of the files will be added by hand
//
// Assumptions:
// - one property per line
// - "struct UnitDef" is line at start of UnitDef class
// - { is on next line after struct, at start of line
// - maximum one curly left bracket and one curly right bracket per line (one left and one right, eg "{}", is ok)
// - multiline /* */ blocks
// - no /* */ blocks inside unitdef
// - maximum one ; per line inside struct UnitDef
// - no curly brackets inside comment blocks
// - no curly brackets, semicolons inside any quotation marks, eg "{"

using System;
using System.IO;

namespace BuildTools
{
    class UnitDefClassGenerator
    {
        public void Go( string UnitDefFilePath, string outputdirectory )
        {
            StreamReader sr = new StreamReader( UnitDefFilePath );
            StreamWriter csinterfacefile = new StreamWriter( outputdirectory + "\\_IUnitDef.cs" );
            StreamWriter cppproxyfile = new StreamWriter( outputdirectory + "\\_UnitDefProxy.h" );
            
            bool insideunitdefclass = false;
            int nestlevel = 0;
            string line = sr.ReadLine();
            bool haveenteredclassfully = false;
            while( line != null )
            {
                string trimmedline = line.Trim();
                if( !insideunitdefclass )
                {
                    if( trimmedline.ToLower() == "struct unitdef" )
                    {
                        insideunitdefclass = true;
                        Console.WriteLine( "inside class" );
                    }
                }
                else
                {
                    if( nestlevel == 0 && haveenteredclassfully ) // we left unitdef class
                    {
                        insideunitdefclass = false;
                        Console.WriteLine( "left class" );
                    }
                    if( nestlevel == 1  ) // ie we're not in some nested class inside unitdef, which we're ignoring here
                    {
                        haveenteredclassfully = true;
                        string[] splitline = trimmedline.Split( ";".ToCharArray() );
                        string statement = splitline[0];   // remove everything from ; onwards
                        
                        string[] splitstatement = statement.Split( " ".ToCharArray() );
                        if( splitstatement.GetUpperBound(0) + 1 >= 2 )
                        {
                            string type = splitstatement[0]; // get type
                            string name = splitstatement[1]; // and name
                            
                            // check not pointer type (treat those by hand)
                            if( ( type + name ).IndexOf("*" ) == -1 )
                            {
                                Console.WriteLine( name );
                                if( type == "float" )
                                {
                                    cppproxyfile.WriteLine( "   double get_" + name + "(){ return actualunitdef->" + name + "; }" );
                                    csinterfacefile.WriteLine( "        double " + name + "{ get; }" );
                                }
                                else if( type == "bool" )
                                {
                                    cppproxyfile.WriteLine( "   bool get_" + name + "(){ return actualunitdef->" + name + "; }" );
                                    csinterfacefile.WriteLine( "        bool " + name + "{ get; }" );
                                }
                                else if( type == "int" )
                                {
                                    cppproxyfile.WriteLine( "   int get_" + name + "(){ return actualunitdef->" + name + "; }" );
                                    csinterfacefile.WriteLine( "        int " + name + "{ get; }" );
                                }
                                else if( type == "std::string" )
                                {
                                    cppproxyfile.WriteLine( "   System::String *get_" + name + "(){ return new System::String( actualunitdef->" + name + ".c_str() ); }" );                                    
                                    csinterfacefile.WriteLine( "        string " + name + "{ get; }" );
                                }
                            }
                        }
                    }
                }
                
                if( trimmedline.IndexOf("{") >= 0 )
                {
                    nestlevel++;
                    Console.WriteLine( "new nestlevel: " + nestlevel );
                }
                if( trimmedline.IndexOf("}") >= 0 )
                {
                    nestlevel--;
                    Console.WriteLine( "new nestlevel: " + nestlevel );
                }
                
                line = sr.ReadLine();
            }
            
            sr.Close();
            csinterfacefile.Close();
            cppproxyfile.Close(); 
        }
    }
}

class EntryPoint
{
    public static int Main( string[] args )
    {
        if( args.GetUpperBound(0) + 1 < 2 )
        {
            Console.WriteLine( "Usage: GenerateUnitDefClasses [UnitDef.h full file path] [outputdirectory]" );
            return 1;
        }
        
        new BuildTools.UnitDefClassGenerator().Go( args[0], args[1] );
        return 0;
    }
}

