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
using System.Reflection;
using System.IO;

using CSharpAI;

// The goal of this generator is to generator the bulk of the function calls for AICallback, UnitDef and MoveData. 
//
// The context is to generate Abic Wrapper classes for use within Mono
//
// You can define which types are targeted for generation in the Go() method right at the bottom of the file
//
// You may also want to mess with CsTypeToCppTypeString , but you probably wont have to; be careful with this function
//

class GenerateCode
{
    byte[] ReadFile( String filename )
    {
        FileStream fs = new FileStream( filename, FileMode.Open );
        BinaryReader br = new BinaryReader( fs );
        Byte[] bytes = br.ReadBytes( (int)fs.Length );
        br.Close();
        return bytes;
    }
    
    // returns array of the types of the parameters of the method specified by methodinfo
    public static Type[] GetParameterTypes( MethodBase methodbase )
    {
        ParameterInfo[] parameterinfos = methodbase.GetParameters();
        Type[] paramtypes = new Type[ parameterinfos.GetUpperBound(0) + 1 ];
        for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i ++ )
        {
            paramtypes[i] = parameterinfos[i].ParameterType;
        }
        return paramtypes;
    }

    
    // returns public class members having the attribute attributetype
    // if attributetype is null, returns all public members
    public static MemberInfo[] GetMembersForAttribute( Type objecttype )
    {
        ArrayList members = new ArrayList();
        foreach( MemberInfo memberinfo in objecttype.GetMembers() )
        {
    //        if( attributetype == null || HasAttribute( memberinfo, attributetype ) )
       //     {
                members.Add( memberinfo );
       //     }
        }
        return (MemberInfo[])members.ToArray( typeof( MemberInfo ) );
    }
    
    class ExceptionCsTypeNotHandled : Exception
    {
        public ExceptionCsTypeNotHandled( string msg ) : base( msg )
        {
        }
    }
    
    string CsTypeToCppTypeString( Type CsType, bool IsReturnType )
    {
        if( CsType == typeof( int ) )
        {
            return "int";
        }
        if( CsType == typeof( double ) )
        {
            return "float";
        }
        if( CsType == typeof( string ) )
        {
            //return "const char *";
            if( IsReturnType )
            {
                return "MonoString *";
            }
            return "MonoString *";
            // return "std::string";
        }
        if( CsType == typeof( bool ) )
        {
            return "bool";
        }
        //if( CsType == typeof( IUnitDef ) )
       // {
        //    return "const UnitDef *";
        //}
        if( CsType == typeof( void ) )
        {
            return "void";
        }
        
        throw new ExceptionCsTypeNotHandled("type " + CsType.ToString() + " not handled");
    }
    
    void Analyze( Type targettype )
    {
        Console.WriteLine( "analyzing " + targettype.ToString() + " ... " );
        foreach( MemberInfo memberinfo in targettype.GetMembers() )
        {
            //Console.WriteLine( memberinfo.MemberType.ToString() + " " + memberinfo.Name );
            if( memberinfo.MemberType == MemberTypes.Field )
            {
                //Console.WriteLine( memberinfo.Name );
                FieldInfo fi = targettype.GetField( memberinfo.Name );
                Console.WriteLine( "Field: " + memberinfo.Name + " " + fi.FieldType + " isliteral " + fi.IsLiteral.ToString() );
            }
            else if( memberinfo.MemberType == MemberTypes.Property )
            {
                //Console.WriteLine( memberinfo.Name );
                PropertyInfo pi = targettype.GetProperty( memberinfo.Name );
                Console.WriteLine( "Property: " + pi.PropertyType + " " + memberinfo.Name );
            }
            else if( memberinfo.MemberType == MemberTypes.Method )
            {
                Console.WriteLine( memberinfo.MemberType.ToString() + " " + memberinfo.Name );
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                Console.WriteLine( "   returntype " + methodinfo.ReturnType.ToString() );
                ParameterInfo[] parameterinfos = methodbase.GetParameters();
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i ++ )
                {
                    Console.WriteLine( "   parameter " + parameterinfos[i].ParameterType + " " + parameterinfos[i].Name );
                }
                //Console.WriteLine( memberinfo.Name );
                //MethodInfo mi = targettype.GetMethod( memberinfo.Name );
                //Console.WriteLine( memberinfo.Name );
            }
        }
    }
    
    void WriteHeaderComments( StreamWriter sw )
    {
        sw.WriteLine( "// *** This is a generated file; if you want to change it, please change CSAIInterfaces.dll, which is the reference" );
        sw.WriteLine( "// " );
        sw.WriteLine( "// This file was generated by MonoAbicWrappersGenerator, by Hugh Perkins hughperkins@gmail.com http://manageddreams.com" );
        sw.WriteLine( "// " );
    }
    
    // patterns:
      //public property string name
      //public void SendTextMsg( string message, int priority )
    string GetAbicWrapperMethodDeclaration( Type targettype, string basetypename, MemberInfo memberinfo )
    {
        if( memberinfo.MemberType == MemberTypes.Field )
        {
            FieldInfo fi = targettype.GetField( memberinfo.Name );
            if( !fi.IsLiteral ) // ignore constants
            {
              //  headerfile.WriteLine( "   " + "mono_field_get_value( monoobject, mono_class_get_field_from_name (thisclass, \"" + memberinfo.Name + "\"), &(" + nativeobjectname + "->" + memberinfo.Name + " ) );" );
            }
        }
        else if( memberinfo.MemberType == MemberTypes.Property )
        {
            PropertyInfo pi = targettype.GetProperty( memberinfo.Name );
            string csstring = "";
            CsTypeToCppTypeString( pi.PropertyType, true ); // this is just to force an excpetion if we dont support the type, eg Float3, Command
            csstring +=  "      public " + pi.PropertyType.ToString() + " " + memberinfo.Name + "";
            return csstring;
        }
        else if( memberinfo.MemberType == MemberTypes.Method )
        {
    // pattern:
      //public void SendTextMsg( string message, int priority )
            if( memberinfo.Name.IndexOf( "get_" ) != 0 ) // ignore property accessors
            {
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                ParameterInfo[] parameterinfos = methodbase.GetParameters();
                CsTypeToCppTypeString( methodinfo.ReturnType, true ); // this is just to force an excpetion if we dont support the type, eg Float3, Command
                string csstring = "      public " + methodinfo.ReturnType.ToString() + " " + memberinfo.Name + "(";
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    if( i > 0 )
                    {
                        csstring += ", ";
                    }
                    CsTypeToCppTypeString( parameterinfos[i].ParameterType, false ); // this is just to force an excpetion if we dont support the type, eg Float3, Command
                    csstring += parameterinfos[i].ParameterType.ToString() + " " + parameterinfos[i].Name;
                }
                csstring += " )\n";
                return csstring;
            }
        }
        
        return "";
    }
    
    string GetAbicWrapperMethodImplementation( Type targettype, string basetypename, MemberInfo memberinfo )
    {
        if( memberinfo.MemberType == MemberTypes.Field )
        {
            FieldInfo fi = targettype.GetField( memberinfo.Name );
            if( !fi.IsLiteral ) // ignore constants
            {
              //  headerfile.WriteLine( "   " + "mono_field_get_value( monoobject, mono_class_get_field_from_name (thisclass, \"" + memberinfo.Name + "\"), &(" + nativeobjectname + "->" + memberinfo.Name + " ) );" );
            }
        }
        else if( memberinfo.MemberType == MemberTypes.Property )
        {
    // pattern:
      //{
      //    get{ return ABICInterface.IUnitDef_get_name( self ); }
      //}
            PropertyInfo pi = targettype.GetProperty( memberinfo.Name );
            string csstring = "";
            csstring +=  "      {\n";
            csstring +=  "          get{ return ABICInterface." + basetypename + "_get_" + memberinfo.Name + "( self ); }\n";
            csstring +=  "      }\n";
            return csstring;
        }
        else if( memberinfo.MemberType == MemberTypes.Method )
        {
    // pattern:
      //{
      //     ABICInterface.IAICallback_SendTextMsg( self, message, priority );
      //}
            if( memberinfo.Name.IndexOf( "get_" ) != 0 ) // ignore property accessors
            {
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                ParameterInfo[] parameterinfos = methodbase.GetParameters();
                string csstring = "      {\n";
                
                 csstring += "         ";
                if( methodinfo.ReturnType != typeof( void ) )
                {
                    csstring += "return ";
                }
                csstring += "ABICInterface." + basetypename + "_" + memberinfo.Name + "( self";
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    if( i < parameterinfos.GetUpperBound(0) + 1 )
                    {
                        csstring += ", ";
                    }
                    if( parameterinfos[i].ParameterType == typeof(double ) )
                    {
                        csstring += "(float)";
                    }
                    csstring += parameterinfos[i].Name;
                }
                csstring += " );\n";
                csstring += "      }\n";
                return csstring;
            }
        }
        
        return "";
    }
    
    bool ArrayContains( string[] array, string value )
    {
        foreach( string thisvalue in array )
        {
            if( thisvalue == value )
            {
                return true;
            }
        }
        return false;
    }
    
    void GenerateFor( Type targettype, string basetypename, string[]methodstoignore )
    {
        string[] splittargettypestring = targettype.ToString().Split(".".ToCharArray() );
        string namespacename = "";
        if( splittargettypestring.GetUpperBound(0) + 1 > 1 )
        {
            namespacename = splittargettypestring[ 0 ];
        }
        string typename = splittargettypestring[ splittargettypestring.GetUpperBound(0) ];
        string csbasetypename = typename.Substring(1); // remove leading "I"
        string basefilename = typename + "_generated";

        StreamWriter csfile = new StreamWriter( "Abic" + typename + "Wrapper_generated.cs", false );        
        WriteHeaderComments( csfile );
        csfile.WriteLine( "" );
        csfile.WriteLine( "using System;" );
        csfile.WriteLine( "using System.Runtime.CompilerServices;" );
        csfile.WriteLine( "using System.Runtime.InteropServices;" );
        csfile.WriteLine( "using System.Text;" );
        csfile.WriteLine( "using System.IO;" );
        csfile.WriteLine( "" );
        csfile.WriteLine( "namespace CSharpAI" );
        csfile.WriteLine( "{");
        csfile.WriteLine( "    public class " + csbasetypename + " : MarshalByRefObject, " + typename );
        //csfile.WriteLine( "    public class " + csbasetypename );
        csfile.WriteLine( "    {" );
        csfile.WriteLine( "        public IntPtr self = IntPtr.Zero;" );
        csfile.WriteLine( "        public " + csbasetypename + "( IntPtr self )" );
        csfile.WriteLine( "        {" );
        csfile.WriteLine( "           this.self = self;" );
        csfile.WriteLine( "        }" );
        
        foreach( MemberInfo memberinfo in targettype.GetMembers() )
        {
            if( !ArrayContains( methodstoignore, memberinfo.Name ) )
            {
                if( memberinfo.MemberType != MemberTypes.Method || memberinfo.Name.IndexOf( "get_" ) != 0 )
                {
                    try
                    {
                        string csstringdec = GetAbicWrapperMethodDeclaration( targettype, basetypename, memberinfo );
                        string csstringimp = GetAbicWrapperMethodImplementation( targettype, basetypename, memberinfo );
                        if( csstringimp.Trim() == "" ||  csstringdec.Trim() == "" ) // this is not required, its more like an assert
                        {
                            Console.WriteLine( memberinfo.Name + " has an issue" );
                        }
                        csfile.WriteLine( csstringdec + "\n" + csstringimp );
                    }
                    catch( ExceptionCsTypeNotHandled e )
                    {
                        Console.WriteLine("Ignoring " + memberinfo.MemberType.ToString() + " " + typename + "." + memberinfo.Name + " " + e.Message );
                    }
                    catch( Exception e )
                    {
                        Console.WriteLine( e.ToString() );
                    }
                }
            }
        }
        
        StreamReader sr = new StreamReader( "Abic" + typename + "Wrapper_manualtweaks.txt" );
        csfile.WriteLine(  "#line 0 \"" + "Abic" + typename + "Wrapper_manualtweaks.txt" + "\"\n" );
        csfile.Write( sr.ReadToEnd() );
        sr.Close();
        
        csfile.WriteLine( "    }" );
        csfile.WriteLine( "}" );
        csfile.Close();
    }
    
    public void Go()
    {
        // analyze gives you information on a type; useful during development
         //Analyze( typeof( IAICallback ) );
         // Analyze( typeof( IUnitDef ) );
         //Analyze( typeof( IMoveData ) );
        
        // Parameters: typename in CSAIInterfaces, native Spring typename, methods to ignore
        GenerateFor( typeof( IAICallback ), "IAICallback", new string[]{"UnitIsBusy"} );
        GenerateFor( typeof( IUnitDef ), "UnitDef", new string[]{} );
        GenerateFor( typeof( IMoveData ), "MoveData", new string[]{} );
        GenerateFor( typeof( IFeatureDef ), "FeatureDef", new string[]{} );
    }
}
    
class entrypont
{
    public static void Main(string[] args)
    {
        new GenerateCode().Go();
    }
}

