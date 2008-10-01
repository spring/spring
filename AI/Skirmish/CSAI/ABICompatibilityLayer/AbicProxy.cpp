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

#include "AbicProxy.h"
#include "ExternalAI/IGlobalAICallback.h"
#include "Sim/Units/UnitDef.h"
#include <vector>

// #include "tinyxml.h"

//#include "AICallbackProxy.h"
#include "Platform/SharedLib.h" // borrow from spring

// ABI compatibility layer converts Spring's GlobalAI C++ interface to C-interface
// Actions:
// - determines name of dll to load (todo)
// - loads dll and binds functions
// - forwards function calls to dll

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// ::IAICallback* aicallback;

namespace std{
	void _xlen(){};
}

// For last ditch debugging of Abic Layer.
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

string GetDllFilename()
{
    // allows us to add quotes around AIDLLNAME
#define xstr(s) str(s)
#define str(s) #s
    
    #ifndef AIDLLNAME
    #define AIDLLNAME myai.dll
    #endif
    return xstr( AIDLLNAME ); // note to self, need to make this dynamic somehow.  Can we get name of our own dll somehow???
}

void AbicProxy::InitAI(IGlobalAICallback* callback, int team)
{
    this->team = team;
	globalaicallback = callback;
    WriteLine("getting callback...");
	aicallback = globalaicallback->GetAICallback();
    //::aicallback = aicallback;
    
    string DllFilename = GetDllFilename();

    WriteLine( DllFilename );
    
    lib = SharedLib::instantiate( DllFilename );
    
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
    WriteLine("Calling _InitAI...");
    ai = _InitAI( aicallback, team );
    
    WriteLine("ai initialized");
}

// fires for every unit, including initial commander
void AbicProxy::UnitCreated(int unit)
{
    _UnitCreated( ai, unit );
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
    _GotChatMsg( ai, msg, player );
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
    _Update(ai );
}
