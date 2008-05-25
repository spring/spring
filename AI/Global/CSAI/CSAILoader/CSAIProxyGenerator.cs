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

// The goal of this generator is to generator the CSAIProxy classes for CSAILoader
//
// You can define which types are targeted for generation in the Go() method right at the bottom of the file
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
            return "double";
        }
        if( CsType == typeof( string ) )
        {
            //return "const char *";
            if( IsReturnType )
            {
                return "System::String *";
            }
            //return "System::String *";
            // return "std::string";
        }
        if( CsType == typeof( bool ) )
        {
            return "bool";
        }
        //if( CsType == typeof( IUnitDef ) )
        //{
          //  return "const UnitDef *";
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
        sw.WriteLine( "// *** This is a generated file; if you want to change it, please change GlobalAIInterfaces.dll, which is the reference" );
        sw.WriteLine( "// " );
        sw.WriteLine( "// This file was generated by CSAIProxyGenerator, by Hugh Perkins hughperkins@gmail.com http://manageddreams.com" );
        sw.WriteLine( "// " );
    }
    
        // pattern:
        // 	double get_maxSlope()
        //    double GetUnitDefRadius( int def )
    
	string GetDeclarationString( Type targettype, string basetypename, MemberInfo memberinfo )
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
            // pattern for strings:
            // System::String *get_myName(){ return new System::String( actualfeaturedef->myName.c_str() ); }
            PropertyInfo pi = targettype.GetProperty( memberinfo.Name );
            string thismethoddeclaration = "";
            thismethoddeclaration += CsTypeToCppTypeString( pi.PropertyType, true ) + " get_" + memberinfo.Name;
            thismethoddeclaration += "()";
            return thismethoddeclaration;
        }
        else if( memberinfo.MemberType == MemberTypes.Method )
        {
            if( memberinfo.Name.IndexOf( "get_" ) != 0 ) // ignore property accessors
            {
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                ParameterInfo[] parameterinfos = methodbase.GetParameters();
                string thismethoddeclaration = "";
                thismethoddeclaration += CsTypeToCppTypeString( methodinfo.ReturnType, true ) + " ";
                thismethoddeclaration += methodinfo.Name;
                thismethoddeclaration += "( ";
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    thismethoddeclaration += CsTypeToCppTypeString( parameterinfos[i].ParameterType, false ) + " " + parameterinfos[i].Name;
                    if( i < parameterinfos.GetUpperBound(0) )
                    {
                        thismethoddeclaration += ", ";
                    }
                }
                thismethoddeclaration += " )";
                return thismethoddeclaration;
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
        //string basetypename = typename.Substring(1); // remove leading "I"
        string basefilename = "CSAIProxy" + typename + "_generated";

        StreamWriter headerfile = new StreamWriter( basefilename + ".h", false );        
        WriteHeaderComments( headerfile );    
        
        // pattern:
        // 	double get_maxSlope(){ return actualmovedata->get_maxSlope(); }
        //    double GetUnitDefRadius( int def )
        //    {
        //        return callbacktorts->GetUnitDefRadius( def );
        //    }

        // StreamWriter cppfile = new StreamWriter( basefilename + ".cpp", false );        
        //WriteHeaderComments( cppfile );    
        foreach( MemberInfo memberinfo in targettype.GetMembers() )
        {
            if( !ArrayContains( methodstoignore, memberinfo.Name ) )
            {
                try
                {
                    string declarationstring = GetDeclarationString( targettype, basetypename, memberinfo );
                    if( declarationstring != "" )
                    {
                        // first we generate the string, throwing exceptions as appropriate
                        // then we write them to file if they worked out ok
                        // this needs to be atomic, so that we always have both header declaration + cpp definition, or neither
                        string cppdeclaration = "   " + declarationstring + "\r\n";
                        cppdeclaration += "   " + "{\r\n";
                        // pattern for strings:
                        // System::String *get_myName(){ return new System::String( string( actualfeaturedef->myName ).c_str() ); }
                        if( memberinfo.MemberType == MemberTypes.Property )
                        {
                            PropertyInfo pi = targettype.GetProperty( memberinfo.Name );
                            if( pi.PropertyType == typeof( string ) )
                            {
                                cppdeclaration += "   " + "   return new System::String( std::string( self->get_" + memberinfo.Name + "() ).c_str() );\r\n"; // we can assume a property wont return void
                            }
                            else
                            {
                                cppdeclaration += "   " + "   return self->get_" + memberinfo.Name + "();\r\n"; // we can assume a property wont return void
                            }
                        }
                        else if( memberinfo.MemberType == MemberTypes.Method )
                        {
                            MethodInfo methodinfo = memberinfo as MethodInfo;
                            if( methodinfo.ReturnType == typeof( void ) )
                            {
                                cppdeclaration += "   " + "   self->" + memberinfo.Name + "( ";
                            }
                            else if( methodinfo.ReturnType == typeof( string ) )
                            {
                                cppdeclaration += "   " + "   return new System::String( std::string( self->" + memberinfo.Name + "( ";
                            }
                            else
                            {
                                cppdeclaration += "   " + "   return self->" + memberinfo.Name + "( ";
                            }
                            
                            ParameterInfo[] parameterinfos = methodinfo.GetParameters();
                            for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                            {
                                if( i > 0 )
                                //if( i < parameterinfos.GetUpperBound(0) )
                                {
                                    cppdeclaration += ", ";
                                }
                                cppdeclaration += parameterinfos[i].Name;
                            }
                                
                            cppdeclaration += "  )";
                            
                            if( methodinfo.ReturnType == typeof( string ) )
                            {
                                cppdeclaration += " ).c_str() )";
                            }
                            
                            cppdeclaration += ";\r\n";                        
                        }
                        cppdeclaration += "   " + "}\r\n";
                        cppdeclaration += "\r\n";
                        
                        //headerfile.WriteLine( declarationstring + ";" );
                        headerfile.Write( cppdeclaration );
                    }
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
        
        headerfile.Close();
        //cppfile.Close();
    }
    
    public void Go()
    {
        // analyze gives you information on a type; useful during development
         //Analyze( typeof( IAICallback ) );
         // Analyze( typeof( IUnitDef ) );
         //Analyze( typeof( IMoveData ) );
        
        // Parameters: typename in CSAIInterfaces, native Spring typename, methods to ignore
        GenerateFor( typeof( IAICallback ), "IAICallback", new string[]{"UnitIsBusy", "IsGamePaused" } );
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

