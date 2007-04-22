using System;

// This is used by the MonoLoader, to allow .reloadai, and to separate AIs
// Its MarshalByRefObject, so it can cross AppDomain boundaries

namespace CSharpAI
{
    public class GlobalAIProxy : MarshalByRefObject, IGlobalAI
    {
       IGlobalAI child;
       public GlobalAIProxy( IGlobalAI child )
       {
           this.child = child;
       }
        public void InitAI( IAICallback aicallback, int team){ child.InitAI( aicallback, team ); }
        public void Shutdown(){ child.Shutdown(); } // to signal AI to shut down threads, clean up file handles etc
    
        public void UnitCreated( int unit ){ child.UnitCreated( unit ); }									//called when a new unit is created on ai team
        public void UnitFinished( int unit ){ child.UnitFinished( unit ); }								//called when an unit has finished building
        public void UnitDestroyed( int unit, int attacker ){ child.UnitDestroyed( unit, attacker ); }				//called when a unit is destroyed
    
        public void EnemyEnterLOS( int enemy ){ child.EnemyEnterLOS( enemy ); }
        public void EnemyLeaveLOS( int enemy ){ child.EnemyLeaveLOS( enemy ); }
    
        public void EnemyEnterRadar( int enemy ){ child.EnemyEnterRadar( enemy ); }						//called when an enemy enter radar coverage (los always count as radar coverage to)
        public void EnemyLeaveRadar( int enemy ){ child.EnemyLeaveRadar( enemy ); }						//called when an enemy leave radar coverage (los always count as radar coverage to)
        
        public void EnemyDamaged( int damaged, int attacker, float damage, Float3 dir )	//called when an enemy inside los or radar is damaged
        {
            child.EnemyDamaged( damaged, attacker, damage, dir );
        }
        public void EnemyDestroyed( int enemy, int attacker )		//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
        { child.EnemyDestroyed( enemy, attacker ); }
    
        public void UnitIdle( int unit )										//called when a unit go idle and is not assigned to any group
        { child.UnitIdle( unit ); }
    
        public void GotChatMsg( string msg, int player )					//called when someone writes a chat msg
        { child.GotChatMsg( msg, player ); }
    
        public void UnitDamaged( int damaged, int attacker, float damage, Float3 dir)					//called when one of your units are damaged
        { child.UnitDamaged( damaged, attacker, damage, dir ); }
        public void UnitMoveFailed( int unit )							// called when a ground unit failed to reach it's destination
        { child.UnitMoveFailed( unit ); }
        public int HandleEvent ( int msg, object data) // general messaging function to be used for future API extensions.
        { return child.HandleEvent( msg, data ); }
        public void Update(){ child.Update(); }
    }
}
