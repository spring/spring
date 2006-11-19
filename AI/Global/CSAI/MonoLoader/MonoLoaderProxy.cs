using System;
using System.IO;
using System.Reflection;

namespace CSharpAI
{
    public class MonoLoaderProxy : MarshalByRefObject, IMonoLoaderProxy
    {
        byte[] ReadFile( string filename )
        {
            FileStream fs = new FileStream( filename, FileMode.Open );
            BinaryReader br = new BinaryReader( fs );
            byte[] bytes = br.ReadBytes( (int)fs.Length );
            br.Close();
            fs.Close();
            return bytes;
        }
        
        // lastline debug only
        StreamWriter sw;
        void WriteLine( string message )
        {
        //    StreamWriter sw = new StreamWriter( "outo.txt", true );
            Console.WriteLine( message );
           // sw.Close();
        }
        
        Assembly a = null;
        object getResult( byte[] assemblybytes, string targettypename, string methodname )
        {
            a = Assembly.Load( assemblybytes );
               // a = Assembly.Load( assemblyfilepath );
            Type t = a.GetType( targettypename );
            MethodInfo mi = t.GetMethod( methodname );    
            object result = mi.Invoke( 0, null);
            WriteLine( result.GetType().ToString() );
            return result;
            //return null;
        }
        
        byte[] assemblybytes = null;
        
        public GlobalAIProxy DynLoad( string assemblyfilename, string debugfilename, string targettypename, string methodname )
        {
            WriteLine( "reading assembly [" + assemblyfilename + "]..." );
            assemblybytes = ReadFile( assemblyfilename );
            WriteLine( "... assembly read" );
            
            IGlobalAI iglobalaiobject = getResult( assemblybytes, targettypename, methodname ) as IGlobalAI;
           GlobalAIProxy globalaiproxy = new GlobalAIProxy( iglobalaiobject );
            return globalaiproxy;
        }    
    }
}
