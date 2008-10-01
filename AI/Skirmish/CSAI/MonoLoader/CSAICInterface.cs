// Copyright Jelmer Cnossen, Hugh Perkins 2006
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
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Text;
using System.IO;
using System.Reflection;
using System.Xml;

namespace CSharpAI
{
    // note: no longer used; links directly to CSAI.dll
    public class _CSAI : CSharpAI.IGlobalAI
    {
        IAICallback aicallback;
        int team;
        
        double[,]largebuffer = new double[ 1000, 1000];
        
        public void InitAI( IAICallback aicallback, int team )
        {
            this.aicallback = aicallback;
            this.team = team;
            aicallback.SendTextMsg( "Hello from Mono AbicWrappers", 0 );
            aicallback.SendTextMsg( "The map name is: " + aicallback.GetMapName(), 0 );
            aicallback.SendTextMsg( "Our ally team is: " + aicallback.GetMyTeam(), 0 );
            
            //int features[10000 + 1];
            //int numfeatures = IAICallback_GetFeatures( aicallback, features, 10000 );
            //sprintf( buffer, "Num features is: %i", numfeatures );
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );
            
            //const FeatureDef *featuredef = IAICallback_GetFeatureDef( aicallback, features[0] );
            //sprintf( buffer, "First feature: %s", FeatureDef_get_myName( featuredef ) );
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
            
            IUnitDef unitdef = aicallback.GetUnitDefByTypeId( 34 );
            aicallback.SendTextMsg( "gotunitdef", 0 );
            aicallback.SendTextMsg( "type id 34 is " + unitdef.name, 0 );
            aicallback.SendTextMsg( "human name: " + unitdef.humanName, 0 );
            aicallback.SendTextMsg( "id: " + unitdef.id, 0 );
            
            IMoveData movedata = unitdef.movedata;
            //IAICallback_SendTextMsg( aicallback, "movedata is null? " + ( movedata == 0 );
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
            
            //IAICallback_SendTextMsg( aicallback, "movetype: %i" + MoveData_get_movetype( movedata ) );        
            //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
            
            aicallback.SendTextMsg( "maxslope: " + movedata.maxSlope, 0 );
        }
        
        public void UnitCreated( int unit )
        {
            aicallback.SendTextMsg(  "Unit created: " + unit, 0 );
        
            IUnitDef unitdef = aicallback.GetUnitDef( unit );
            aicallback.SendTextMsg(  "Unit created: " + unitdef.name, 0 );
        
            IMoveData movedata = unitdef.movedata;
            if( movedata != null )
            {
                aicallback.SendTextMsg(  "Max Slope: " + movedata.maxSlope, 0 );
            }
        
            if( unitdef.isCommander )
            {
                int numbuildoptions = unitdef.GetNumBuildOptions();
                string buildoptionsstring = "Build options: ";
                for( int i = 0; i < numbuildoptions; i++ )
                {
                    buildoptionsstring += unitdef.GetBuildOption( i );            
                }
                aicallback.SendTextMsg( buildoptionsstring, 0 );
        
                Float3 commanderpos = aicallback.GetUnitPos( unit );
                aicallback.SendTextMsg( "Commanderpos: " + commanderpos.ToString(), 0 );
                
                int numunitdefs = aicallback.GetNumUnitDefs();
                aicallback.SendTextMsg( "Num unit defs: " + numunitdefs, 0 );
                
                for( int i = 1; i <= numunitdefs; i++ )
                {
                    IUnitDef thisunitdef = aicallback.GetUnitDefByTypeId( i );
                    if( thisunitdef.name == "ARMSOLAR" )
                    {
                        aicallback.SendTextMsg( "Found solar collector def: " + thisunitdef.id, 0 );

                        Float3 nearestbuildpos = aicallback.ClosestBuildSite( thisunitdef, commanderpos, 1400,2  );
                        aicallback.SendTextMsg( "Closest build site: " + nearestbuildpos.ToString(), 0 );
                        
                        aicallback.DrawUnit( "ARMSOLAR", nearestbuildpos,0,
                            200, aicallback.GetMyAllyTeam(),true,true );
        
                        aicallback.GiveOrder( unit, new Command( -  thisunitdef.id, nearestbuildpos.ToDoubleArray() ) );
                    }
                }
            }
        }
        public void UnitFinished(int unit)							//called when an unit has finished building
        {
        }
        
        public void UnitDestroyed( int unit,int attacker)								//called when a unit is destroyed
        {
        }
        
        public void EnemyEnterLOS( int enemy)
        {
        }
        
        public void EnemyLeaveLOS(int enemy)
        {
        }
        
        public void EnemyEnterRadar( int enemy)		
        {
        }
        
        public void EnemyLeaveRadar( int enemy)
        {
        }
        
        public void EnemyDamaged( int damaged,int attacker,float damage, Float3 dir)	//called when an enemy inside los or radar is damaged
        {
        }
        
        public void EnemyDestroyed( int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
        {
        }
        
        public void UnitIdle( int unit)										//called when a unit go idle and is not assigned to any group
        {
        }
        
        public void GotChatMsg( string msg,int player)					//called when someone writes a chat msg
        {
        }
        
        public void UnitDamaged( int damaged,int attacker,float damage, Float3 dir)					//called when one of your units are damaged
        {
        }
        
        public void UnitMoveFailed( int unit)
        {
        }
        
        //int HandleEvent (int msg,const void *data); // todo
        public int HandleEvent( int msg, object data )
        {
            return 0;
        }
        
        //called every frame
        public void Update()
        {
        }
        
        public void Shutdown()
        {
        }
    }
    
    public class CSAICInterface
    {
        IntPtr aicallback;
        CSharpAI.IGlobalAI ai;
        
          public delegate void _InitAI( IntPtr aicallback,int team);
          IntPtr InitAIInstancePointer;
          _InitAI initai;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetInitAICallback( IntPtr callback );
          
          public delegate void _Update( );
          IntPtr UpdateInstancePointer;
          _Update update;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetUpdateCallback( IntPtr callback );
          
          public delegate void _GotChatMsg( string msg,int priority);
          IntPtr GotChatMsgInstancePointer;
          _GotChatMsg gotchatmsg;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetGotChatMsgCallback( IntPtr callback );
          
          public delegate void _UnitCreated( int unit);
          IntPtr UnitCreatedInstancePointer;
          _UnitCreated unitcreated;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetUnitCreatedCallback( IntPtr callback );
          
          public delegate void _UnitFinished( int unit);
          IntPtr UnitFinishedInstancePointer;
          _UnitFinished unitfinished;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetUnitFinishedCallback( IntPtr callback );
          
          public delegate void _UnitIdle( int unit);
          IntPtr UnitIdleInstancePointer;
          _UnitIdle unitidle;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetUnitIdleCallback( IntPtr callback );
          
          public delegate void _UnitMoveFailed( int unit);
          IntPtr UnitMoveFailedInstancePointer;
          _UnitMoveFailed unitmovefailed;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetUnitMoveFailedCallback( IntPtr callback );
          
          public delegate void _UnitDamaged( int damaged,int attacker,float damage,float dirx,float diry,float dirz);
          IntPtr UnitDamagedInstancePointer;
          _UnitDamaged unitdamaged;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetUnitDamagedCallback( IntPtr callback );
          
          public delegate void _UnitDestroyed( int enemy,int attacker);
          IntPtr UnitDestroyedInstancePointer;
          _UnitDestroyed unitdestroyed;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetUnitDestroyedCallback( IntPtr callback );
          
          public delegate void _EnemyEnterLOS( int unit);
          IntPtr EnemyEnterLOSInstancePointer;
          _EnemyEnterLOS enemyenterlos;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetEnemyEnterLOSCallback( IntPtr callback );
          
          public delegate void _EnemyLeaveLOS( int unit);
          IntPtr EnemyLeaveLOSInstancePointer;
          _EnemyLeaveLOS enemyleavelos;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetEnemyLeaveLOSCallback( IntPtr callback );
          
          public delegate void _EnemyEnterRadar( int unit);
          IntPtr EnemyEnterRadarInstancePointer;
          _EnemyEnterRadar enemyenterradar;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetEnemyEnterRadarCallback( IntPtr callback );
          
          public delegate void _EnemyLeaveRadar( int unit);
          IntPtr EnemyLeaveRadarInstancePointer;
          _EnemyLeaveRadar enemyleaveradar;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetEnemyLeaveRadarCallback( IntPtr callback );
          
          public delegate void _EnemyDamaged( int damaged,int attacker,float damage,float dirx,float diry,float dirz);
          IntPtr EnemyDamagedInstancePointer;
          _EnemyDamaged enemydamaged;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetEnemyDamagedCallback( IntPtr callback );
          
          public delegate void _EnemyDestroyed( int enemy,int attacker);
          IntPtr EnemyDestroyedInstancePointer;
          _EnemyDestroyed enemydestroyed;
          [MethodImpl(MethodImplOptions.InternalCall)]
          public extern static void SetEnemyDestroyedCallback( IntPtr callback );
          
        
        // start point, called by invoke
        public void Bind()
        {
            try
            {
                //WriteLine( "CSAICInterface.Bind >>>" );
                
                initai = new _InitAI( InitAI );
                InitAIInstancePointer = Marshal.GetFunctionPointerForDelegate( initai );
                SetInitAICallback( InitAIInstancePointer );
                
                update = new _Update( Update );
                UpdateInstancePointer = Marshal.GetFunctionPointerForDelegate( update );
                SetUpdateCallback( UpdateInstancePointer );
                
                gotchatmsg = new _GotChatMsg( GotChatMsg );
                GotChatMsgInstancePointer = Marshal.GetFunctionPointerForDelegate( gotchatmsg );
                SetGotChatMsgCallback( GotChatMsgInstancePointer );
                
                unitcreated = new _UnitCreated( UnitCreated );
                UnitCreatedInstancePointer = Marshal.GetFunctionPointerForDelegate( unitcreated );
                SetUnitCreatedCallback( UnitCreatedInstancePointer );
                
                unitfinished = new _UnitFinished( UnitFinished );
                UnitFinishedInstancePointer = Marshal.GetFunctionPointerForDelegate( unitfinished );
                SetUnitFinishedCallback( UnitFinishedInstancePointer );
                
                unitidle = new _UnitIdle( UnitIdle );
                UnitIdleInstancePointer = Marshal.GetFunctionPointerForDelegate( unitidle );
                SetUnitIdleCallback( UnitIdleInstancePointer );
                
                unitmovefailed = new _UnitMoveFailed( UnitMoveFailed );
                UnitMoveFailedInstancePointer = Marshal.GetFunctionPointerForDelegate( unitmovefailed );
                SetUnitMoveFailedCallback( UnitMoveFailedInstancePointer );
                
                unitdamaged = new _UnitDamaged( UnitDamaged );
                UnitDamagedInstancePointer = Marshal.GetFunctionPointerForDelegate( unitdamaged );
                SetUnitDamagedCallback( UnitDamagedInstancePointer );
                
                unitdestroyed = new _UnitDestroyed( UnitDestroyed );
                UnitDestroyedInstancePointer = Marshal.GetFunctionPointerForDelegate( unitdestroyed );
                SetUnitDestroyedCallback( UnitDestroyedInstancePointer );
                
                enemyenterlos = new _EnemyEnterLOS( EnemyEnterLOS );
                EnemyEnterLOSInstancePointer = Marshal.GetFunctionPointerForDelegate( enemyenterlos );
                SetEnemyEnterLOSCallback( EnemyEnterLOSInstancePointer );
                
                enemyleavelos = new _EnemyLeaveLOS( EnemyLeaveLOS );
                EnemyLeaveLOSInstancePointer = Marshal.GetFunctionPointerForDelegate( enemyleavelos );
                SetEnemyLeaveLOSCallback( EnemyLeaveLOSInstancePointer );
                
                enemyenterradar = new _EnemyEnterRadar( EnemyEnterRadar );
                EnemyEnterRadarInstancePointer = Marshal.GetFunctionPointerForDelegate( enemyenterradar );
                SetEnemyEnterRadarCallback( EnemyEnterRadarInstancePointer );
                
                enemyleaveradar = new _EnemyLeaveRadar( EnemyLeaveRadar );
                EnemyLeaveRadarInstancePointer = Marshal.GetFunctionPointerForDelegate( enemyleaveradar );
                SetEnemyLeaveRadarCallback( EnemyLeaveRadarInstancePointer );
                
                enemydamaged = new _EnemyDamaged( EnemyDamaged );
                EnemyDamagedInstancePointer = Marshal.GetFunctionPointerForDelegate( enemydamaged );
                SetEnemyDamagedCallback( EnemyDamagedInstancePointer );
                
                enemydestroyed = new _EnemyDestroyed( EnemyDestroyed );
                EnemyDestroyedInstancePointer = Marshal.GetFunctionPointerForDelegate( enemydestroyed );
                SetEnemyDestroyedCallback( EnemyDestroyedInstancePointer );
                            
               // WriteLine( "CSAICInterface.Bind <<<" );
            }
            catch(Exception e )
            {
                sw = new StreamWriter("outbind.log", false );
                WriteLine( e.ToString() );
                sw.Flush();
                sw.Close();
            }
        }
        
        // lastline debug only
        StreamWriter sw;
        void WriteLine( string message )
        {
        //    StreamWriter sw = new StreamWriter( "outo.txt", true );
            sw.WriteLine( message );
            sw.Flush();
           // sw.Close();
        }

        
        byte[] ReadFile( string filename )
        {
            FileStream fs = new FileStream( filename, FileMode.Open );
            BinaryReader br = new BinaryReader( fs );
            byte[] bytes = br.ReadBytes( (int)fs.Length );
            br.Close();
            fs.Close();
            return bytes;
        }
        
        Assembly a = null;
        object getResult( string assemblyfilepath, byte[] assemblybytes, string targettypename, string methodname )
        {
            a = Assembly.Load( assemblybytes );
               // a = Assembly.Load( assemblyfilepath );
            Type t = a.GetType( targettypename );
            MethodInfo mi = t.GetMethod( methodname );    
            object result = mi.Invoke( 0, null);
            WriteLine( result.GetType().ToString() );
            return result;
        }
        
        byte[] assemblybytes = null;
        
        object DynLoad( string assemblyfilename, string pdbfilename, string targettypename, string methodname )
        {
            WriteLine( "reading assembly [" + assemblyfilename + "]..." );
            assemblybytes = ReadFile( assemblyfilename );
            WriteLine( "... assembly read" );
            
            return getResult( assemblyfilename, assemblybytes, targettypename, methodname );
        }
        
        void WriteSomething()
        {
            WriteLine("blah");
        }
        
        static int nextappdomainref = 0;
        string appdomainname;
        AppDomain ourappdomain;
        // DynLoadInAppDomain loads the AI dll in a separate appdomain, one per AI
        // This is absolutely lagtastic.  So dont do that.
        CSharpAI.GlobalAIProxy DynLoadInAppDomain( string dllpath, string classname, string methodname )
        {
            appdomainname = "csai" + nextappdomainref;
            nextappdomainref++;
            WriteLine( "appdomain name: " + appdomainname );
            //Evidence baseEvidence = AppDomain.CurrentDomain.Evidence;
            //WriteLine( "got base evidence" );
            //Evidence evidence = new Evidence(baseEvidence); 
            Evidence evidence = new Evidence(); 
            WriteLine( "created new evidence" );
            ourappdomain = AppDomain.CreateDomain( appdomainname, evidence );
            WriteLine( "domain created" );
              IMonoLoaderProxy monoloaderproxy = ourappdomain.CreateInstanceAndUnwrap( "MonoLoaderProxy, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null", "CSharpAI.MonoLoaderProxy" ) as IMonoLoaderProxy;
            WriteLine( "got monoloaderproxy" );
              GlobalAIProxy globalaiproxy = monoloaderproxy.DynLoad(dllpath, "", classname, methodname);
            WriteLine( "back from dynload" );
            return globalaiproxy;
        }
        
        CSharpAI.IGlobalAI LoadCSAI()
        {
            WriteLine( "LoadCSAI" );
            WriteLine( "Codebase: " + Assembly.GetCallingAssembly().CodeBase );
            string configfilename = Assembly.GetCallingAssembly().CodeBase.Replace( ".dll", ".xml" ); // on linux, just append xml to end of filename will work, or tweak this
            WriteLine( "configfile: [" + configfilename + "]" );
            XmlDocument configdom = new XmlDocument();
            
            configdom.Load( configfilename );
            WriteLine("loaded dom" );
            XmlElement configelement = configdom.SelectSingleNode( "/root/config" ) as XmlElement;
            WriteLine("loaded dom" );
            string CSAIDirectory = configelement.GetAttribute("csaidirectory" );
            string CSAIDllName = configelement.GetAttribute("csaidllname" );
            string PdbName = CSAIDllName.Replace( ".dll", ".pdb" );
            string CSAIClassName = configelement.GetAttribute("csaiclassname" );
            WriteLine("got attributes" );
            
            string dllpath = Path.Combine( CSAIDirectory, CSAIDllName );
            string pdbpath = Path.Combine( CSAIDirectory, PdbName );
            WriteLine(dllpath );
            WriteLine(pdbpath );
            WriteLine(CSAIClassName );
            object aiexecobject = DynLoad( dllpath, pdbpath,  CSAIClassName, "GetInstance" );
            //object aiexecobject = DynLoadInAppDomain( dllpath, CSAIClassName, "GetInstance" );
            WriteLine("Did load" );
            //WriteLine( aiexecobject->GetType()->ToString() );
            return aiexecobject as CSharpAI.IGlobalAI;
            
            //return null;
        }
        
        int team;
        static int numais = 0;
		public void InitAI(IntPtr aicallback, int team )
		{
            try
            {
                sw = new StreamWriter( "outo_" + team + ".log", false );
                WriteLine( "CSAICInterface.InitAI >>>" );
                
                this.team = team;
                    //ai = LoadCSAI();
                if( numais == 0 )
                {
                    ai = LoadCSAI();
                    //ai = new _CSAI();
                }
                else
                {
                    //ai = LoadCSAI();
                    ABICInterface.IAICallback_SendTextMsg( aicallback, "Only one Mono AI allowed per game", 0 );
                    ai = new _CSAI();
                }
                numais++;
                
                this.aicallback = aicallback;
                
                ai.InitAI( new AICallback( aicallback ), team );                
                WriteLine( "CSAICInterface.InitAI <<<" );
            }
            catch(Exception e )
            {
                WriteLine( e.ToString() );
            }
		}        
        
        public void UnitCreated( int unit)									//called when a new unit is created on ai team
        {
                WriteLine( "CSAICInterface.UnitCreated >>>" );
            ai.UnitCreated( unit );
                WriteLine( "CSAICInterface.UnitCreated <<<" );
        }
        
        public void UnitFinished(int unit)							//called when an unit has finished building
        {
                WriteLine( "CSAICInterface.UnitFinished >>>" );
            ai.UnitFinished( unit );
                WriteLine( "CSAICInterface.UnitFinished <<<" );
        }
        
        public void UnitDestroyed( int unit,int attacker)								//called when a unit is destroyed
        {
            ai.UnitDestroyed( unit, attacker );
        }
        
        public void EnemyEnterLOS( int enemy)
        {
            ai.EnemyEnterLOS( enemy );
        }
        
        public void EnemyLeaveLOS(int enemy)
        {
            ai.EnemyLeaveLOS( enemy );
        }
        
        public void EnemyEnterRadar( int enemy)		
        {
            ai.EnemyEnterRadar( enemy );
        }
        
        public void EnemyLeaveRadar( int enemy)
        {
            ai.EnemyLeaveRadar( enemy );
        }
        
        public void EnemyDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
        {
            ai.EnemyDamaged( damaged, attacker, damage, new Float3( dirx, diry, dirz ) );
        }
        
        public void EnemyDestroyed( int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
        {
            ai.EnemyDestroyed( enemy, attacker );
        }
        
        public void UnitIdle( int unit)										//called when a unit go idle and is not assigned to any group
        {
                WriteLine( "CSAICInterface.UnitIdle >>>" );
            ai.UnitIdle( unit );
                WriteLine( "CSAICInterface.UnitIdle <<<" );
        }
        
        public void GotChatMsg( string msg,int player)					//called when someone writes a chat msg
        {
                WriteLine( "CSAICInterface.GotChatMsg >>> " + msg );
            // ABICInterface.IAICallback_SendTextMsg( aicallback,"GotChatMsg: " + msg, 0 );
            // update on reloadai: doesnt work with Mono, unless you use appdomains which are sllllooooowwwwww
            // if( msg == ".reloadai" ) // unsure if this works with Mono, linux etc.  If it crashes, dont use it ;-)
            // {
                // WriteLine( "shutting down old ai..." );
                // ai.Shutdown(); // release logfile lock, any threads, etc...
                // WriteLine( "unloading appdomain..." );
                // AppDomain.Unload( ourappdomain ); // unload assembly
                // WriteLine( "loading new ai..." );
                // ai = LoadCSAI();                
                // WriteLine( "calling initai..." );
                // ai.InitAI( new AICallback( aicallback ), team );
            // }
            
            ai.GotChatMsg( msg, player );
                WriteLine( "CSAICInterface.GotChatMsg <<<" );
        }
        
        public void UnitDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
        {
            ai.UnitDamaged( damaged, attacker, damage, new Float3( dirx, diry, dirz ) );
        }
        
        public void UnitMoveFailed( int unit)
        {
            ai.UnitMoveFailed( unit );
        }
        
        //int HandleEvent (int msg,const void *data); // todo
        
        //called every frame
        public void Update()
        {
                //WriteLine( "CSAICInterface.Update >>>" );
            ai.Update(  );
                //WriteLine( "CSAICInterface.Update <<<" );
        }
    }
}
