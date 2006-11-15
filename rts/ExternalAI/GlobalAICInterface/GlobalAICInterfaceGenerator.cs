using System;
using System.Collections;
using System.Reflection;
using System.IO;

using CSharpAI;

// The goal of this generator is to generator the bulk of the function calls for AICallback, UnitDef and MoveData.  It will also create a .def file for them
//
// You can define which types are targeted for generation in the Go() method right at the bottom of the file
//
// You may also want to mess with CsTypeToCppTypeString , but you probably wont have to; be careful with this function
//
// For each type we will generate the following files:
// <typename>_generated.cpp
// <typename>_generated.h
//
// The code in generated.cpp will look something like:
// AICALLBACK_API int UnitDef_get_id( const UnitDef *self )
// {
//     return self->id;
// }
//
// There is always an additional argument called self which is an opaque pointer.  An opaque pointer means the AI knows it's a struct, but doesnt
// know what is in the struct
//
// The code in generated.h will look something like:
// AICALLBACK_API int UnitDef_get_id( const UnitDef *self );

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
                return "const char *";
            }
            return "char *";
            // return "std::string";
        }
        if( CsType == typeof( bool ) )
        {
            return "bool";
        }
        if( CsType == typeof( IUnitDef ) )
        {
            return "const UnitDef *";
        }
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
        sw.WriteLine( "// This file was generated by ABICCodeGenerator, by Hugh Perkins hughperkins@gmail.com http://manageddreams.com" );
        sw.WriteLine( "// " );
    }
    
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
            PropertyInfo pi = targettype.GetProperty( memberinfo.Name );
            string thismethoddeclaration = "AICALLBACK_API ";
            thismethoddeclaration += CsTypeToCppTypeString( pi.PropertyType, true ) + " " + basetypename + "_get_" + memberinfo.Name;
            thismethoddeclaration += "( ";
            thismethoddeclaration += "const ";
            thismethoddeclaration += basetypename + " *self )";
            return thismethoddeclaration;
        }
        else if( memberinfo.MemberType == MemberTypes.Method )
        {
            if( memberinfo.Name.IndexOf( "get_" ) != 0 ) // ignore property accessors
            {
                MethodBase methodbase = memberinfo as MethodBase;
                MethodInfo methodinfo = memberinfo as MethodInfo;
                ParameterInfo[] parameterinfos = methodbase.GetParameters();
                string thismethoddeclaration = "AICALLBACK_API ";
                thismethoddeclaration += CsTypeToCppTypeString( methodinfo.ReturnType, true ) + " ";
                thismethoddeclaration += basetypename + "_" + methodinfo.Name;
                thismethoddeclaration += "( ";
                thismethoddeclaration += "const ";
                thismethoddeclaration += basetypename + " *self";
                for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                {
                    if( i < parameterinfos.GetUpperBound(0) + 1 )
                    {
                        thismethoddeclaration += ", ";
                    }
                    thismethoddeclaration += CsTypeToCppTypeString( parameterinfos[i].ParameterType, false ) + " " + parameterinfos[i].Name;
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
        string basefilename = typename + "_generated";

        StreamWriter headerfile = new StreamWriter( basefilename + ".h", false );        
        WriteHeaderComments( headerfile );    
        
// target code to generate for header:
// AICALLBACK_API int UnitDef_get_id( const UnitDef *self );
        
// target code to generate for cpp:
// AICALLBACK_API int UnitDef_get_id( const UnitDef *self )
// {
//     return self->id;
// }
        StreamWriter cppfile = new StreamWriter( basefilename + ".gpp", false );        
        WriteHeaderComments( cppfile );    
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
                        string cppdeclaration = declarationstring + "\n";
                        cppdeclaration += "{\n";
                        string defmethodname = "";
                        if( memberinfo.MemberType == MemberTypes.Property )
                        {
                            PropertyInfo pi = targettype.GetProperty( memberinfo.Name );
                            defmethodname = basetypename + "_get_" + memberinfo.Name;
                            cppdeclaration += "   return ( ( " + basetypename + " *) self)->" + memberinfo.Name; // we can assume a property wont return void
                            if( pi.PropertyType == typeof( string ) )
                            {
                                cppdeclaration += ".c_str()";
                            }
                            cppdeclaration += ";\n";
                        }
                        else if( memberinfo.MemberType == MemberTypes.Method )
                        {
                            defmethodname = basetypename + "_" + memberinfo.Name;
                            MethodInfo methodinfo = memberinfo as MethodInfo;
                            if( methodinfo.ReturnType != typeof( void ) )
                            {
                                cppdeclaration += "   return ( ( " + basetypename + " *)self )->" + memberinfo.Name + "( ";
                            }
                            else
                            {
                                cppdeclaration += "   ( ( " + basetypename + " *)self )->" + memberinfo.Name + "( ";
                            }
                            
                            ParameterInfo[] parameterinfos = methodinfo.GetParameters();
                            for( int i = 0; i < parameterinfos.GetUpperBound(0) + 1; i++ )
                            {
                                if( i > 0 )
                                {
                                    cppdeclaration += ", ";
                                }
                                cppdeclaration += parameterinfos[i].Name;
                            }
                                
                            cppdeclaration += "  );\n";                        
                        }
                        cppdeclaration += "}\n";
                        cppdeclaration += "\n";
                        
                        headerfile.WriteLine( declarationstring + ";" );
                        cppfile.WriteLine( cppdeclaration );
                        //deffile.WriteLine( nextdefnum + "\t" + defmethodname );
                        deffile.WriteLine( defmethodname );
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
        cppfile.Close();
    }
    
    string deffilename;
    //int nextdefnum;
    
    StreamWriter deffile;
    
    public void Go()
    {
        // analyze gives you information on a type; useful during development
         //Analyze( typeof( IAICallback ) );
         // Analyze( typeof( IUnitDef ) );
         //Analyze( typeof( IMoveData ) );
        
        deffilename = "def_generated.def";
        //nextdefnum = 1; // can change this to begin def numbering at different number
        
        deffile = new StreamWriter( deffilename, false );
        deffile.WriteLine("EXPORTS");
        
        // Parameters: typename in CSAIInterfaces, native Spring typename, methods to ignore
        GenerateFor( typeof( IAICallback ), "IAICallback", new string[]{
            "UnitIsBusy", "IsGamePaused","GetUnitDefByTypeId" } );
        GenerateFor( typeof( IUnitDef ), "UnitDef", new string[]{"GetNumBuildOptions","GetBuildOption"} );
        GenerateFor( typeof( IMoveData ), "MoveData", new string[]{} );
        GenerateFor( typeof( IFeatureDef ), "FeatureDef", new string[]{} );
            
        deffile.Close();
    }
}
    
class entrypont
{
    public static void Main(string[] args)
    {
        new GenerateCode().Go();
    }
}

