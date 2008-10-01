// Copyright TASpring Project, Hugh Perkins 2006
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

// This file ported from IGlobalAI.h

namespace CSharpAI
{
    public interface IGlobalAI
    {
        void InitAI( IAICallback aicallback, int team);
        void Shutdown(); // to signal AI to shut down threads, clean up file handles etc
    
        void UnitCreated( int unit );									//called when a new unit is created on ai team
        void UnitFinished( int unit );								//called when an unit has finished building
        void UnitDestroyed( int unit, int attacker );				//called when a unit is destroyed
    
        void EnemyEnterLOS( int enemy );
        void EnemyLeaveLOS( int enemy );
    
        void EnemyEnterRadar( int enemy );						//called when an enemy enter radar coverage (los always count as radar coverage to)
        void EnemyLeaveRadar( int enemy );						//called when an enemy leave radar coverage (los always count as radar coverage to)
        
        void EnemyDamaged( int damaged, int attacker, float damage, Float3 dir );	//called when an enemy inside los or radar is damaged
        void EnemyDestroyed( int enemy, int attacker );		//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
    
        void UnitIdle( int unit );										//called when a unit go idle and is not assigned to any group
    
        void GotChatMsg( string msg, int player );					//called when someone writes a chat msg
    
        void UnitDamaged( int damaged, int attacker, float damage, Float3 dir);					//called when one of your units are damaged
        void UnitMoveFailed( int unit );							// called when a ground unit failed to reach it's destination
        int HandleEvent ( int msg, object data); // general messaging function to be used for future API extensions.
        void Update();
    }
}

