// Copyright TASpring project, Hugh Perkins 2006
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

#include "stdafx.h"

#include "AbicProxy.h"
#include "ExternalAI/IGlobalAICallback.h"
#include "Sim/Units/UnitDef.h"
#include <vector>

#include "LogOutput.h"
// #include "tinyxml.h"

//#include "AICallbackProxy.h"
#include "Platform/SharedLib.h" 

// ABI compatibility layer converts Spring's GlobalAI C++ interface to C-interface
// Actions:
// - determines name of dll to load (todo)
// - loads dll and binds functions
// - forwards function calls to dll

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace std{
	void _xlen(){};
}

// For last ditch debugging of Abic Layer.
// THis should probably be migrated to use Spring's internal debug log
void WriteLine( char *message )
{
    FILE *file = fopen("out.log", "a+" );
    fprintf( file, message );
    fprintf( file, "\n" );
    fclose( file );
}

AbicProxy::AbicProxy()
{
}

AbicProxy::~AbicProxy()
{
    //delete csai;
}

void AbicProxy::InitAI( const char *DllFilename, IGlobalAICallback* callback, int team)
{
    //logOutput.Print( "abicproxy::InitAI" );
    this->team = team;
	globalaicallback = callback;
    //logOutput.Print("getting callback...");
	aicallback = globalaicallback->GetAICallback();
    
    // string DllFilename = GetDllFilename();
    
    lib = SharedLib::Instantiate( DllFilename );
    
	_InitAI =  (INITAI)lib->FindAddress("InitAI");
    
    _UnitCreated = (UNITCREATED)lib->FindAddress("UnitCreated");
    _UnitFinished =  (UNITFINISHED)lib->FindAddress("UnitFinished");
    _UnitDestroyed =  (UNITDESTROYED)lib->FindAddress("UnitDestroyed");
    _UnitIdle =  (UNITIDLE)lib->FindAddress("UnitIdle");
    _UnitDamaged =  (UNITDAMAGED)lib->FindAddress("UnitDamaged");
    _UnitMoveFailed =  (UNITMOVEFAILED)lib->FindAddress("UnitMoveFailed");
    
    _EnemyEnterLOS =  (ENEMYENTERLOS)lib->FindAddress("EnemyEnterLOS");
    _EnemyLeaveLOS =  (ENEMYLEAVELOS)lib->FindAddress("EnemyLeaveLOS");
    _EnemyEnterRadar =  (ENEMYENTERRADAR)lib->FindAddress("EnemyEnterRadar");
    _EnemyLeaveRadar =  (ENEMYLEAVERADAR)lib->FindAddress("EnemyLeaveRadar");
    _EnemyDamaged =  (ENEMYDAMAGED)lib->FindAddress("EnemyDamaged");
    _EnemyDestroyed =  (ENEMYDESTROYED)lib->FindAddress("EnemyDestroyed");
        
    _GotChatMsg =  (GOTCHATMSG)lib->FindAddress("GotChatMsg");

    _Update =  (UPDATE)lib->FindAddress("Update");

    // call initai in loaded dll
    ai = _InitAI( aicallback, team );
    
    //logOutput.Print("ai initialized");
}

// fires for every unit, including initial commander
void AbicProxy::UnitCreated(int unit)
{
    //logOutput.Print( "abicproxy::UnitCreated >>>" );
    _UnitCreated( ai, unit );
    //logOutput.Print( "abicproxy::UnitCreated <<<" );
}

void AbicProxy::UnitFinished(int unit)
{
    _UnitFinished( ai, unit );
}

void AbicProxy::UnitDestroyed(int unit,int attacker)
{
    _UnitDestroyed( ai, unit, attacker );
}

void AbicProxy::EnemyEnterLOS(int enemy)
{
    _EnemyEnterLOS( ai, enemy );
}

void AbicProxy::EnemyLeaveLOS(int enemy)
{
    _EnemyLeaveLOS( ai, enemy );
}

void AbicProxy::EnemyEnterRadar(int enemy)
{
    _EnemyEnterRadar( ai, enemy );
}

void AbicProxy::EnemyLeaveRadar(int enemy)
{
    _EnemyLeaveRadar( ai, enemy );
}

void AbicProxy::EnemyDamaged(int damaged,int attacker,float damage,float3 dir)
{
    _EnemyDamaged( ai, damaged, attacker, damage, dir.x, dir.y, dir.z );
}

void AbicProxy::EnemyDestroyed(int enemy,int attacker)
{
    _EnemyDestroyed( ai, enemy, attacker );
}

void AbicProxy::UnitIdle(int unit)
{
    _UnitIdle( ai, unit );
}

void AbicProxy::GotChatMsg(const char* msg,int player)
{
    //logOutput.Print( "abicproxy::GotChatMsg >>>" );
    _GotChatMsg( ai, msg, player );
    //logOutput.Print( "abicproxy::GotChatMsg <<<" );
}

void AbicProxy::UnitDamaged(int damaged,int attacker,float damage,float3 dir)
{
    _UnitDamaged( ai, damaged, attacker, damage, dir.x, dir.y, dir.z );
}

void AbicProxy::UnitMoveFailed(int unit)
{
    _UnitMoveFailed( ai, unit );
}

// Ignore
int AbicProxy::HandleEvent(int msg,const void* data)
{
	return 0;
}

void AbicProxy::Update()
{
    //logOutput.Print( "abicproxy::Update >>>" );
    _Update(ai );
    //logOutput.Print( "abicproxy::Update <<<" );
}
