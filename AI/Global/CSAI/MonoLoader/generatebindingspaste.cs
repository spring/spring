// This will generate IGlobalAI bindings that can be pasted into the relevant files
// This isnt for speed (its already done) but for accuracy (saves checking)

using System;
using System.IO;

class GenerateBindingsPaste
{
   string csaideclarations = "";
   string csaibindings = "";
   string cbindings = "";
   string typedefs = "";
   string cpointerinstances = "";
   string csetpointerfunctions = "";

   void GenerateCS( string fnname, string[] parameters )
   {
      csaideclarations += "      public delegate void _" + fnname + "( " + String.Join( ",", parameters ) + ");\n";
      csaideclarations += "      IntPtr " + fnname + "InstancePointer;\n";
      csaideclarations += "      _" + fnname + " " + fnname.ToLower() + ";\n";
      csaideclarations += "      [MethodImpl(MethodImplOptions.InternalCall)]\n";
      csaideclarations += "      public extern static void Set" + fnname + "Callback( IntPtr callback );\n";
      csaideclarations += "      \n";

      csaibindings += "            " + fnname.ToLower() + " = new _" + fnname + "( " + fnname  + " );\n";
      csaibindings += "            " + fnname + "InstancePointer = Marshal.GetFunctionPointerForDelegate( " + fnname.ToLower() + " );\n";
      csaibindings += "            Set" + fnname + "Callback( " + fnname + "InstancePointer );\n";
      csaibindings += "            \n";

      cbindings += "mono_add_internal_call(PREFIX \"Set" + fnname + "Callback\", (void*)Set" + fnname + "Callback);\n";

      string cparametersstring = String.Join( ",", parameters );
      //cparametersstring = cparametersstring.Replace( "string", "MonoString *" );
      cparametersstring = cparametersstring.Replace( "string", "const char *" ); // you might expect this to be a MonoString *, but it's not
                                                                                                 // picture several hours of trial and error to find this out ;-)
      cparametersstring = cparametersstring.Replace( "IntPtr", "void *" );
      typedefs += "typedef void( PLATFORMCALLINGCONVENTION *" + fnname.ToUpper() + ")( " + cparametersstring + ");\n";

      cpointerinstances += "   " + fnname.ToUpper() + " " + fnname + ";\n";

      csetpointerfunctions += "void Set" + fnname + "Callback(void *fnpointer )\n";
      csetpointerfunctions += "{\n";
      csetpointerfunctions += "   WriteLine( \"Set" + fnname + "Callback >>>\" );\n";
      csetpointerfunctions += "   thisaiproxy->" + fnname + " = ( " + fnname.ToUpper() + " )fnpointer;\n";
      csetpointerfunctions += "   WriteLine( \"Set" + fnname + "Callback <<<\" );\n";
      csetpointerfunctions += "}\n";
   } 

   void WriteFile( string filename, string contents )
   {
      StreamWriter sw = new StreamWriter( filename, false );
      sw.WriteLine( contents );
      sw.Close();
   }

   public void Go()
   {
      GenerateCS( "InitAI", new string[]{ "IntPtr aicallback", "int team"} );
      GenerateCS( "Update", new string[]{} );
      GenerateCS( "GotChatMsg", new string[]{ "string msg", "int priority"} );
      GenerateCS( "UnitCreated", new string[]{ "int unit"} );
      GenerateCS( "UnitFinished", new string[]{ "int unit"} );
      GenerateCS( "UnitIdle", new string[]{ "int unit"} );
      GenerateCS( "UnitMoveFailed", new string[]{ "int unit"} );
      GenerateCS( "UnitDamaged", new string[]{ "int damaged","int attacker","float damage", "float dirx", "float diry", "float dirz"} );
      GenerateCS( "UnitDestroyed", new string[]{ "int enemy", "int attacker"} );
      GenerateCS( "EnemyEnterLOS", new string[]{ "int unit"} );
      GenerateCS( "EnemyLeaveLOS", new string[]{ "int unit"} );
      GenerateCS( "EnemyEnterRadar", new string[]{"int unit"} );
      GenerateCS( "EnemyLeaveRadar", new string[]{ "int unit"} );
      GenerateCS( "EnemyDamaged", new string[]{ "int damaged","int attacker","float damage", "float dirx", "float diry", "float dirz"} );
      GenerateCS( "EnemyDestroyed", new string[]{  "int enemy", "int attacker"} );

      WriteFile( "csaideclarations_generated.txt", csaideclarations );
      WriteFile( "csaibind_generated.txt", csaibindings );
      WriteFile( "cbind_generated.h", cbindings );
      WriteFile( "typedefs_generated.h", typedefs );
      WriteFile( "cpointerinstances_generated.h", cpointerinstances );
      WriteFile( "csetpointerfunctions_generated.h", csetpointerfunctions );
   }
}

class EntryPoint
{
   public static void Main()
   {
      new GenerateBindingsPaste().Go();
   }
}



