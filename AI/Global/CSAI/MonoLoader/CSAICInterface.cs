using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;

namespace CSharpAI
{
    public class CSAICInterface
    {
        IntPtr aicallback;
        
		public delegate void _InitAI( IntPtr aicallback, int team );
        IntPtr InitAIInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected
        
		public delegate void _UnitCreated( int unit );
        IntPtr UnitCreatedInstancePointer; // need to keep this as class instance so it doesnt get garbage-collected

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetInitAICallback( IntPtr initai );

		[MethodImpl(MethodImplOptions.InternalCall)]
		public extern static void SetUnitCreatedCallback( IntPtr initai );

        // start point, called by invoke
        public void Bind()
        {
            StreamWriter sw = new StreamWriter( "outo.txt", false );
            try
            {
                sw.WriteLine( "CSAICInterface.Bind >>>" );
                sw.Flush();
                
                //InitAIInstance = new _InitAI( InitAI );
                InitAIInstancePointer = Marshal.GetFunctionPointerForDelegate( new _InitAI( InitAI ) );
                SetInitAICallback( InitAIInstancePointer );
                
                UnitCreatedInstancePointer = Marshal.GetFunctionPointerForDelegate( new _UnitCreated( UnitCreated ) );
                SetUnitCreatedCallback( UnitCreatedInstancePointer );
                
                sw.WriteLine( "CSAICInterface.Bind <<<" );
                sw.Flush();
            }
            catch(Exception e )
            {
                sw.WriteLine( e.ToString() );
                sw.Flush();
            }
            sw.Close();
        }
        
		public void InitAI(IntPtr aicallback, int team )
		{
            StreamWriter sw = new StreamWriter( "outo.txt", true );
            try
            {
                sw.WriteLine( "CSAICInterface.InitAI >>>" );
                sw.Flush();
                this.aicallback = aicallback;
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Hello world from C# Mono 0.2!", 0 );
                
                ABICInterface.IAICallback_SendTextMsg( aicallback, "The map name is: " + ABICInterface.IAICallback_GetMapName( aicallback ), 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Our ally team is: " + ABICInterface.IAICallback_GetMyTeam( aicallback ), 0 );
                
                //int features[10000 + 1];
                //int numfeatures = IAICallback_GetFeatures( aicallback, features, 10000 );
                //sprintf( buffer, "Num features is: %i", numfeatures );
                //IAICallback_SendTextMsg( aicallback, buffer, 0 );
                
                //const FeatureDef *featuredef = IAICallback_GetFeatureDef( aicallback, features[0] );
                //sprintf( buffer, "First feature: %s", FeatureDef_get_myName( featuredef ) );
                //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
                
                IntPtr unitdef = ABICInterface.IAICallback_GetUnitDefByTypeId( aicallback, 34 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "gotunitdef", 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "type id 34 is " + ABICInterface.UnitDef_get_name( unitdef ), 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "name: " + ABICInterface.UnitDef_get_name( unitdef ), 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "human name: " + ABICInterface.UnitDef_get_humanName( unitdef ), 0 );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "id: " + ABICInterface.UnitDef_get_id( unitdef ), 0 );
                
                IntPtr movedata = ABICInterface.UnitDef_get_movedata( unitdef );
                //IAICallback_SendTextMsg( aicallback, "movedata is null? " + ( movedata == 0 );
                //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
                
                //IAICallback_SendTextMsg( aicallback, "movetype: %i" + MoveData_get_movetype( movedata ) );        
                //IAICallback_SendTextMsg( aicallback, buffer, 0 );    
                
                ABICInterface.IAICallback_SendTextMsg( aicallback, "maxslope: " + ABICInterface.MoveData_get_maxSlope( movedata ), 0 );
                
                sw.WriteLine( "CSAICInterface.InitAI <<<" );
                sw.Flush();
            }
            catch(Exception e )
            {
                sw.WriteLine( e.ToString() );
                sw.Flush();
            }
            sw.Close();
		}        
        
        public void UnitCreated( int unit)									//called when a new unit is created on ai team
        {
            ABICInterface.IAICallback_SendTextMsg( aicallback,  "Unit created: " + unit, 0 );
        
            IntPtr unitdef = ABICInterface.IAICallback_GetUnitDef( aicallback, unit );
            ABICInterface.IAICallback_SendTextMsg( aicallback, "Unit created: " + ABICInterface.UnitDef_get_name( unitdef ), 0 );
        
            IntPtr movedata = ABICInterface.UnitDef_get_movedata( unitdef );
            if( movedata != IntPtr.Zero )
            {
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Max Slope: " + ABICInterface.MoveData_get_maxSlope( movedata ), 0 );
            }
        
            if( ABICInterface.UnitDef_get_isCommander( unitdef ) )
            {
                int numbuildoptions = ABICInterface.UnitDef_GetNumBuildOptions( unitdef );
                string buildoptionsstring = "Build options: ";
                for( int i = 0; i < numbuildoptions; i++ )
                {
                    buildoptionsstring += ABICInterface.UnitDef_GetBuildOption( unitdef, i );            
                }
                ABICInterface.IAICallback_SendTextMsg( aicallback, buildoptionsstring, 0 );
        
                float commanderposx = 0; 
                float commanderposy = 0; 
                float commanderposz = 0; 
                ABICInterface.IAICallback_GetUnitPos( aicallback, ref commanderposx, ref commanderposy, ref commanderposz, unit );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Commanderpos: " + commanderposx + " " + commanderposy + " " + commanderposz, 0 );
                
                int numunitdefs = ABICInterface.IAICallback_GetNumUnitDefs( aicallback );
                ABICInterface.IAICallback_SendTextMsg( aicallback, "Num unit defs: " + numunitdefs, 0 );
                
                for( int i = 1; i <= numunitdefs; i++ )
                {
                    IntPtr thisunitdef = ABICInterface.IAICallback_GetUnitDefByTypeId( aicallback, i );
                    if( ABICInterface.UnitDef_get_name( thisunitdef ) == "ARMSOLAR" )
                    {
                        ABICInterface.IAICallback_SendTextMsg( aicallback, "Found solar collector def: " + ABICInterface.UnitDef_get_id( thisunitdef ), 0 );

                        float nbpx = 0, nbpy = 0, nbpz = 0; // ok, lazy, but this will be wrapped later anyway
                        ABICInterface.IAICallback_ClosestBuildSite( aicallback, ref nbpx, ref nbpy, ref nbpz, thisunitdef, commanderposx, commanderposy, commanderposz, 1400,2,0 );
                        ABICInterface.IAICallback_SendTextMsg( aicallback, "Closest build site: " + nbpx + " " + nbpy + " " + nbpz, 0 );
                        
                        ABICInterface.IAICallback_DrawUnit( aicallback, "ARMSOLAR", nbpx, nbpy, nbpz,0,
                            200, ABICInterface.IAICallback_GetMyAllyTeam( aicallback ),true,true,0);
        
                        ABICInterface.IAICallback_GiveOrder( aicallback, unit, -  ABICInterface.UnitDef_get_id( thisunitdef ), 3, nbpx, nbpy, nbpz, 0 );
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
        
        public void EnemyDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
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
        
        public void UnitDamaged( int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
        {
        }
        
        public void UnitMoveFailed( int unit)
        {
        }
        
        //int HandleEvent (int msg,const void *data); // todo
        
        //called every frame
        void Update()
        {
        }
    }
}
