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


// The goal of this is to read AICallback.h and generate an approximate version of:
// - IAICallback.cs
// - AICallbackProxy.h
//
// From there they can be tweaked by hand to add in further extra functions, and then maintained by hand
//
// Scope:
// - the goal of this is NOT to automatically and entirely generate these files, and to automatically maintain them.  We are not aiming to rewrite Swig ;-)
// - we ignore any function with pointers or references (&) as return value or as a parameter; when something is a pointer, it's probably some kind of array,
//   not easy to guestimate from a generator without further information
//
// Note that the header and footer of the files will be added by hand
//
// Assumptions:
// 
// - no multiline /* */ blocks
// - no /* */ blocks inside unitdef
// - maximum one ; per line inside class AICallback.h
// - maximum one curly left bracket and one curly right bracket per line (one left and one right, eg "{}", is ok)
// - no curly brackets inside comment blocks
// - no curly brackets, semicolons inside any quotation marks, eg "{"
// - "class CAICallback" is start of line at start of AICallback class
// - one method per line
// - no multline method declarations
//
// Note: we should probably be reading IAICallback.h not AICallback.h, but for now this is designed for AICallback.h

using System;
using System.IO;
using System.Collections;

namespace BuildTools
{
    class AICallbackClassGenerator
    {
        class Method
        {
            public string Name = "";
            public string ReturnType = "";
            public ArrayList Parameters = new ArrayList();
            public Method(){}
            public Method( string name, string returntype, ArrayList parameters )
            {
                this.Name = name;
                this.ReturnType = returntype;
                this.Parameters = parameters;
            }
            public override string ToString()
            {
                string returnstring = "Method \"" + Name + "\" returns [" + ReturnType + "]\n";
                foreach( object parameterobject in Parameters )
                {
                    returnstring += ( parameterobject as Parameter ).ToString() + "\n";
                }
                return returnstring;
            }
        }
        
        class Parameter
        {
            public string Type;
            public string Name;
            public Parameter( string type, string name )
            {
                this.Type = type;
                this.Name = name;
            }
            public override string ToString()
            {
                return "Parameter \"" + Name + "\" type [" + Type + "]";
            }
        }
        
        // we assume name is anything after the last " " or "*" (whichever is later)
        // and that type is the rest
        // we strip the spaces in front of any "*" or "&", seems easiest
        string[] ParseNameTypePair( string typepair )
        {
            //Console.WriteLine( "" );
            //Console.WriteLine( "typepair: [" + typepair + "]" );
            string[]splittypepair0 = typepair.Split(" ".ToCharArray() );
            string candidatename = splittypepair0[ splittypepair0.GetUpperBound(0) ];
            string[]splittypepair1 = candidatename.Split("*".ToCharArray() );
            string name = splittypepair1[ splittypepair1.GetUpperBound(0) ];
            string type = typepair.Substring( 0, typepair.Length - name.Length );
            //Console.WriteLine( "type: [" + type + "]" );
            //Console.WriteLine( "name: [" + name + "]" );
            name = name.Trim();
            type = type.Trim().Replace( " *", "*" ).Replace( " &", "&" );
            return new string[]{ type, name };
        }
        
        ArrayList ParseMethods( string headerfilepath, string classname )
        {
            ArrayList Methods = new ArrayList();
            
            StreamReader sr = new StreamReader( headerfilepath );
            bool insideunitdefclass = false;
            int nestlevel = 0;
            string line = sr.ReadLine();
            bool haveenteredclassfully = false;
            while( line != null )
            {
                string trimmedline = line.Trim();
                if( !insideunitdefclass )
                {
                    string classnamestring = ( "class " + classname ).ToLower();
                    if( trimmedline.ToLower().IndexOf( classnamestring ) == 0 )
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
                        string methoddeclaration = splitline[0];   // remove everything from ; onwards
                        
                        string[] splitdeclaration = methoddeclaration.Split( " ".ToCharArray() );
                        if( splitdeclaration.GetUpperBound(0) + 1 >= 2 )
                        {
                            Method thismethod = new Method();
                            
                            splitdeclaration = methoddeclaration.Split( "(".ToCharArray() );
                            if( splitdeclaration.GetUpperBound(0) + 1 >= 2 )
                            {
                                string[] nametypepair = ParseNameTypePair( splitdeclaration[0].Trim() );
                                thismethod.ReturnType = nametypepair[0];
                                thismethod.Name = nametypepair[1];
                                
                                string parameterstring = splitdeclaration[1].Trim();
                                parameterstring = parameterstring.Split( ")".ToCharArray() )[0].Trim();
                                if( parameterstring.Length > 0 )
                                {
                                    string[] parameters = parameterstring.Split( ",".ToCharArray() );
                                    
                                    ArrayList parametersarraylist = new ArrayList();
                                    for( int i = 0; i < parameters.GetUpperBound(0) + 1; i++ )
                                    {
                                        nametypepair = ParseNameTypePair( parameters[i].Trim()  );
                                        string type = nametypepair[0];
                                        string name = nametypepair[1];
                                        parametersarraylist.Add( new Parameter( type, name ) );
                                    }
                                    thismethod.Parameters = parametersarraylist;
                                }
                                Methods.Add( thismethod );
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
            
            return Methods;
        }
        
        bool IsKnownType( string type )
        {
            if( type == "int" || type == "bool" || type == "float" )
            {
                return true;
            }
            return false;
        }
        
        // this should be two functions really: one for C# types in C++ proxy, one for Spring types in C++ proxy
        string headertypetocppproxytype( string headertype )
        {
            if( headertype == "float" )
            {
                return "double";
            }
            else if( headertype == "int" )
            {
                return "int";
            }
            else if( headertype == "bool" )
            {
                return "bool";
            }
            return "<unknown>";
        }
        
        string headertypetocstype( string headertype )
        {
            if( headertype == "float" )
            {
                return "double";
            }
            else if( headertype == "int" )
            {
                return "int";
            }
            else if( headertype == "bool" )
            {
                return "bool";
            }
            return "<unknown>";
        }
        
        void WriteOutMethods( ArrayList methods, string csfullfilepath, string cppfullfilepath )
        {            
            StreamWriter csinterfacefile = new StreamWriter( csfullfilepath );
            StreamWriter cppproxyfile = new StreamWriter( cppfullfilepath );
            
            foreach( object methodobject in methods )
            {
                Method method = methodobject as Method;
                bool alltypesknown = true;
                if( !IsKnownType( method.ReturnType ) )
                {
                    alltypesknown = false;
                }
                foreach( object parameterobject in method.Parameters )
                {
                    Parameter parameter = parameterobject as Parameter;
                    if( !IsKnownType( parameter.Type ) )
                    {
                        alltypesknown = false;
                    }
                }
                
                if( alltypesknown )
                {
                    // C++ proxy functions in C++ header file
                    string cppdeclaration = "    " + headertypetocppproxytype( method.ReturnType ) + " " + method.Name + "( ";
                    bool IsFirstParameter = true;
                    foreach( object parameterobject in method.Parameters )
                    {
                        Parameter parameter = parameterobject as Parameter;
                        if( !IsFirstParameter )
                        {
                            cppdeclaration += ", ";
                        }
                        cppdeclaration += headertypetocppproxytype( parameter.Type ) + " " + parameter.Name;
                        IsFirstParameter = false;
                    }
                    cppdeclaration += " )\n";
                    cppdeclaration += "    {\n";
                    cppdeclaration += "        return callbacktorts->" + method.Name + "( ";       
                    IsFirstParameter = true;                    
                    foreach( object parameterobject in method.Parameters )
                    {
                        Parameter parameter = parameterobject as Parameter;
                        if( !IsFirstParameter )
                        {
                            cppdeclaration += ", ";
                        }
                        cppdeclaration += parameter.Name;
                        IsFirstParameter = false;
                    }
                    cppdeclaration += " );\n";                    
                    cppdeclaration += "    }\n";
                    cppproxyfile.WriteLine( cppdeclaration );
                    
                    
                      //      double GetElevation( double x, double z );
                    // C# declarations in interface
                    string csdeclaration = "        " + headertypetocstype( method.ReturnType ) + " " + method.Name + "( ";
                    IsFirstParameter = true;
                    foreach( object parameterobject in method.Parameters )
                    {
                        Parameter parameter = parameterobject as Parameter;
                        if( !IsFirstParameter )
                        {
                            csdeclaration += ", ";
                        }
                        csdeclaration += headertypetocstype( parameter.Type ) + " " + parameter.Name;
                        IsFirstParameter = false;
                    }
                    csdeclaration += " );";
                    csinterfacefile.WriteLine( csdeclaration );
                }
            }
            
            csinterfacefile.Close();
            cppproxyfile.Close(); 
        }
        
        public void Go( string AICallbackFilePath, string outputdirectory )
        {
            ArrayList methods = ParseMethods( AICallbackFilePath, "CAICallback" );
            
            foreach( object methodobject in methods )
            {
                Console.WriteLine( ( methodobject as Method ).ToString() );
            }
            
            WriteOutMethods( methods, outputdirectory + "\\_IAICallback.cs", outputdirectory + "\\_AICallbackProxy.h" );
        }
    }
}

class EntryPoint
{
    public static int Main( string[] args )
    {
        if( args.GetUpperBound(0) + 1 < 2 )
        {
            Console.WriteLine( "Usage: GenerateAICallbackClasses [AICallbackFilePath.h full file path] [outputdirectory]" );
            return 1;
        }
        
        new BuildTools.AICallbackClassGenerator().Go( args[0], args[1] );
        return 0;
    }
}

