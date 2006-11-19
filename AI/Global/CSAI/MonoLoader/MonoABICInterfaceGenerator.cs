// Created by Hugh Perkins

using System;
using System.Collections;
using System.Reflection;
using System.IO;

using CSharpAI;

// The goal of this generator is to generator the bulk of the function calls for AICallback, UnitDef and MoveData. 
//
// The context is to generate .gpp files to be included into CSAIMonoLoader.cpp and a .cs file ABICInterface_generated.cs,
// which contains the ABIC interface in C# format
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
    
    string InterfaceTypeToCsType( Type CsType, bool IsReturnType )
    {
        if( CsType == typeof( int ) )
        {
            return "int";
        }
        if( CsType == typeof( double ) )
        {
            return "float"; // the key reason for this function
        }
        if( CsType == typeof( string ) )
        {
            //return "const char *";
            if( IsReturnType )
            {
                return "string";
            }
            return "string";
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
    
    string InterfaceTypeToCppType( Type CsType, bool IsReturnType )
    {
        if( CsType == typeof( int ) )
        {
            return "int";
        }
        if( CsType == typeof( double ) )
        {
            return "float"; // the key reason for this function
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
        sw.WriteLine( "// This file was generated by MonoABICInterfaceGenerator, by Hugh Perkins hughperkins@gmail.com http://manageddreams.com" );
        sw.WriteLine( "// " );
    }
    
    // for cs file:
    //        [MethodImpl(MethodImplOptions.InternalCall )]
    //    public extern static void IAICallback_SendTextMsg( IntPtr aicallback, string message, int priority );
    string GetCSString( Type targettype, string basetypename, MemberInfo memberinfo )
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
            string csstring = "      [MethodImpl(MethodImplOptions.InternalCall)]\n";
            csstring += "      public extern static " + InterfaceTypeToCsType( pi.PropertyType, true ) + " " + basetypename + "_get_" + memberinfo.Name + "( IntPtr self );\n";
            return csstring;
        }
        else if( memberinfo.MemberType == MemberTypes.Method )
        {
            if( memberinfo.Name.IndexOf( "get_" ) != 0 ) // ignore property accessors
            {
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                ParameterInfo[] parameterinfos = methodbase.GetParameters();
                string csstring = "      [MethodImpl(MethodImplOptions.InternalCall)]\n";
                csstring += "      public extern static " + InterfaceTypeToCsType( methodinfo.ReturnType, true ) + " " + basetypename + "_" + memberinfo.Name + "( IntPtr self";
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    if( i < parameterinfos.GetUpperBound(0) + 1 )
                    {
                        csstring += ", ";
                    }
                    csstring += InterfaceTypeToCsType( parameterinfos[i].ParameterType, false ) + " " + parameterinfos[i].Name;
                }
                csstring += " );";
                return csstring;
            }
        }
        
        return "";
    }
    
    // pattern:
    //void _IAICallback_SendTextMsg( void *aicallback, double somedouble, int priority )
    string GetDefinitionPrototypeString( Type targettype, string basetypename, MemberInfo memberinfo )
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
            csstring +=  InterfaceTypeToCppType( pi.PropertyType, true ) + " _" + basetypename + "_get_" + memberinfo.Name + "( void *self )\n";
            return csstring;
        }
        else if( memberinfo.MemberType == MemberTypes.Method )
        {
            if( memberinfo.Name.IndexOf( "get_" ) != 0 ) // ignore property accessors
            {
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                ParameterInfo[] parameterinfos = methodbase.GetParameters();
                string csstring = "";
                csstring +=  InterfaceTypeToCppType( methodinfo.ReturnType, true ) + " _" + basetypename + "_" + memberinfo.Name + "( void *self";
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    if( i < parameterinfos.GetUpperBound(0) + 1 )
                    {
                        csstring += ", ";
                    }
                    csstring += InterfaceTypeToCppType( parameterinfos[i].ParameterType, false ) + " " + parameterinfos[i].Name;
                }
                csstring += " )";
                return csstring;
            }
        }
        
        return "";
    }
    
    // pattern:
    //{
   //     IAICallback_SendTextMsg( ( const IAICallback *)aicallback, somedouble, priority );
   // }
    string GetDefinitionImplementationString( Type targettype, string basetypename, MemberInfo memberinfo )
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
            csstring +=  "{\n";
            string resultstring = basetypename + "_get_" + memberinfo.Name + "( ( const " + basetypename + " *)self )";
            if( pi.PropertyType == typeof( string ) )
            {
                csstring +=  "    return mono_string_new_wrapper( " + resultstring + " );\n";
            }
            else
            {
                csstring +=  "    return " + resultstring + ";\n";
            }
            csstring +=  "}\n";
            return csstring;
        }
        else if( memberinfo.MemberType == MemberTypes.Method )
        {
            if( memberinfo.Name.IndexOf( "get_" ) != 0 ) // ignore property accessors
            {
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                ParameterInfo[] parameterinfos = methodbase.GetParameters();
                string csstring = "{\n";
                
                // string conversion
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    if( parameterinfos[i].ParameterType == typeof( string ) )
                    {
                        csstring += "   char *" + parameterinfos[i].Name + "_utf8string = mono_string_to_utf8( " + parameterinfos[i].Name + " );\n";
                    }
                }
    //char *message = mono_string_to_utf8(strobj);
    //cout << message << endl;
    //cout << "C's SendTextMsg ends" <<endl;
    //g_free(message); 
                
                 csstring += "   ";
                if( methodinfo.ReturnType != typeof( void ) )
                {
                    csstring += "return ";
                    if( methodinfo.ReturnType == typeof( string ) )
                    {
                        csstring += "mono_string_new_wrapper( ";
                    }
                }
                csstring += basetypename + "_" + memberinfo.Name + "( ( const " + basetypename + " * )self";
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    if( i < parameterinfos.GetUpperBound(0) + 1 )
                    {
                        csstring += ", ";
                    }
                    if( parameterinfos[i].ParameterType == typeof( string ) )
                    {
                        csstring += parameterinfos[i].Name + "_utf8string";
                    }
                    else
                    {
                        csstring += parameterinfos[i].Name;
                    }
                }
                if( methodinfo.ReturnType == typeof( string ) )
                {
                    csstring += ") ";
                }
                csstring += " );\n";
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    if( parameterinfos[i].ParameterType == typeof( string ) )
                    {
                        csstring += "   g_free( " + parameterinfos[i].Name + "_utf8string );\n";
                    }
                }
                csstring += "}\n";
                return csstring;
            }
        }
        
        return "";
    }
    
    // pattern:
     //       mono_add_internal_call(PREFIX "IAICallback_SendTextMsg", (void*)_IAICallback_SendTextMsg);
    string GetBindingString( Type targettype, string basetypename, MemberInfo memberinfo )
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
            string csstring = "    mono_add_internal_call(PREFIX \"" + basetypename + "_get_" + memberinfo.Name + "\", ";
            csstring += "(void *)_" + basetypename + "_get_" + memberinfo.Name + " );\n";
            return csstring;
        }
        else if( memberinfo.MemberType == MemberTypes.Method )
        {
            if( memberinfo.Name.IndexOf( "get_" ) != 0 ) // ignore property accessors
            {
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                // finally, an easy one; dont care about parameters
                string csstring = "    mono_add_internal_call(PREFIX \"" + basetypename + "_" + memberinfo.Name + "\", ";
                csstring += "(void *)_" + basetypename + "_" + memberinfo.Name + " );\n";
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
    
    // patterns:
    // for cs file:
    //        [MethodImpl(MethodImplOptions.InternalCall )]
    //    public extern static void IAICallback_SendTextMsg( IntPtr aicallback, string message, int priority );
    //
    // for definition gpp file:
    //void _IAICallback_SendTextMsg( void *aicallback, double somedouble, int priority )
    //{
   //     IAICallback_SendTextMsg( ( const IAICallback *)aicallback, somedouble, priority );
   // }
    //
    // for binding gpp file:
     //       mono_add_internal_call(PREFIX "IAICallback_SendTextMsg", (void*)_IAICallback_SendTextMsg);

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
        string basefilename = typename + "_generated";
        
        StreamWriter definitionfile = new StreamWriter( typename + "definitions_generated.gpp", false );        
        WriteHeaderComments( definitionfile );    
        
        StreamWriter bindingfile = new StreamWriter( typename + "bindings_generated.gpp", false );        
        WriteHeaderComments( bindingfile );    
        foreach( MemberInfo memberinfo in targettype.GetMembers() )
        {
            if( !ArrayContains( methodstoignore, memberinfo.Name ) )
            {
                if( memberinfo.MemberType != MemberTypes.Method || memberinfo.Name.IndexOf( "get_" ) != 0 )
                {
                    try
                    {
                        string csstring = GetCSString( targettype, basetypename, memberinfo );
                        string definitionstringprot = GetDefinitionPrototypeString( targettype, basetypename, memberinfo );
                        string definitionstringimp = GetDefinitionImplementationString( targettype, basetypename, memberinfo );
                        string bindingstring = GetBindingString( targettype, basetypename, memberinfo );
                            
                        if( definitionstringimp.Trim() == "" )
                        {
                            Console.WriteLine( memberinfo.Name + " has an issue" );
                        }
                        csfile.WriteLine( csstring + "\n" );
                        definitionfile.WriteLine( definitionstringprot + "\n" + definitionstringimp );
                        bindingfile.Write( bindingstring );
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
        
        definitionfile.Close();
        bindingfile.Close();
    }
    
    StreamWriter csfile;
    
    public void Go()
    {
        // analyze gives you information on a type; useful during development
         //Analyze( typeof( IAICallback ) );
         // Analyze( typeof( IUnitDef ) );
         //Analyze( typeof( IMoveData ) );
        
        csfile = new StreamWriter( "ABICInterface_generated.cs", false );
        
        WriteHeaderComments( csfile );
        csfile.WriteLine( "using System;" );
        csfile.WriteLine( "using System.Runtime.CompilerServices;" );
        csfile.WriteLine( "using System.Runtime.InteropServices;" );
        csfile.WriteLine( "using System.Text;" );
        csfile.WriteLine( "using System.IO;" );
        csfile.WriteLine( "" );
        csfile.WriteLine( "namespace CSharpAI" );
        csfile.WriteLine( "{");
        csfile.WriteLine( "    public class ABICInterface" );
        csfile.WriteLine( "    {" );
        
        StreamReader sr = new StreamReader( "ABICInterface_manualtweaks.txt" );
        csfile.Write( sr.ReadToEnd() );
        sr.Close();
        
        // Parameters: typename in CSAIInterfaces, native Spring typename, methods to ignore
        GenerateFor( typeof( IAICallback ), "IAICallback", new string[]{"UnitIsBusy"} );
        GenerateFor( typeof( IUnitDef ), "UnitDef", new string[]{} );
        GenerateFor( typeof( IMoveData ), "MoveData", new string[]{} );
        GenerateFor( typeof( IFeatureDef ), "FeatureDef", new string[]{} );
            
        csfile.WriteLine( "    }" );
        csfile.WriteLine( "}");
        csfile.Close();
    }
}
    
class entrypont
{
    public static void Main(string[] args)
    {
        new GenerateCode().Go();
    }
}

