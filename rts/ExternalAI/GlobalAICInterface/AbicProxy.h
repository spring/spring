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

// for documentation please see CSAIProxy.cpp

#if !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ExternalAI/IGlobalAI.h"
#include "ExternalAI/IAICallback.h"
#include <map>
#include <set>
#include <stdio.h>

#include "LogOutput.h"

#include "Platform/SharedLib.h"

// #include "AICallbackProxy.h"

const char AI_NAME[]="ABIC"; // probably should read this from config file or something

#ifdef WIN32
	#define AILOG_PATH "AI\\Bot-libs\\"
#else
	#define AILOG_PATH "AI/Bot-libs/"
#endif

class AbicProxy : public ::IGlobalAI  
{
public:
	AbicProxy();
	virtual ~AbicProxy();

	void InitAI(const char *dllname, IGlobalAICallback* callback, int team);
	void InitAI(IGlobalAICallback* callback, int team)
	{
        logOutput << "WARNING: Call to AbicProxy::InitAI( callback, team ).  This function should never be called, should use InitAI( dllname, callback, team\n";
	}

	void UnitCreated(int unit);									//called when a new unit is created on ai team
	void UnitFinished(int unit);								//called when an unit has finished building
	void UnitDestroyed(int unit,int attacker);								//called when a unit is destroyed

	void EnemyEnterLOS(int enemy);
	void EnemyLeaveLOS(int enemy);

	void EnemyEnterRadar(int enemy);				
	void EnemyLeaveRadar(int enemy);				

	void EnemyDamaged(int damaged,int attacker,float damage,float3 dir);	//called when an enemy inside los or radar is damaged
	void EnemyDestroyed(int enemy,int attacker);							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)

	void UnitIdle(int unit);										//called when a unit go idle and is not assigned to any group

	void GotChatMsg(const char* msg,int player);					//called when someone writes a chat msg

	void UnitDamaged(int damaged,int attacker,float damage,float3 dir);					//called when one of your units are damaged

	void UnitMoveFailed(int unit);
	int HandleEvent (int msg,const void *data);

	//called every frame
	void Update();

	::IGlobalAICallback* globalaicallback;
	::IAICallback* aicallback;

	std::set<int> myUnits;
	std::set<int> enemies;
    
private:
  //  FILE *logfile;
    int team;

	SharedLib *lib;

    void *ai; // pointer to object returned by ai, which we pass as first parameter to each function call

	typedef void *(* INITAI)( class IAICallback *aicallback, int team );
	typedef void (* UNITCREATED )(void *ai, int unit);									//called when a new unit is created on ai team
	typedef void (* UNITFINISHED )(void *ai, int unit);								//called when an unit has finished building
	typedef void (* UNITDESTROYED )(void *ai, int unit,int attacker);								//called when a unit is destroyed
	typedef void (* UNITIDLE )(void *ai, int unit);										//called when a unit go idle and is not assigned to any group
	typedef void (* UNITDAMAGED )(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz);					//called when one of your units are damaged
	typedef void (* UNITMOVEFAILED )(void *ai, int unit);

	typedef void (* ENEMYENTERLOS )(void *ai, int enemy);
	typedef void (* ENEMYLEAVELOS )(void *ai, int enemy);

	typedef void (* ENEMYENTERRADAR )(void *ai, int enemy);				
	typedef void (* ENEMYLEAVERADAR )(void *ai, int enemy);				

	typedef void (* ENEMYDAMAGED )(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz );	//called when an enemy inside los or radar is damaged
	typedef void (* ENEMYDESTROYED )(void *ai, int enemy,int attacker);							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)

	typedef void (* GOTCHATMSG )(void *ai, const char* msg,int player);					//called when someone writes a chat msg

	//typedef int (* HandleEvent )(int msg,const void *data); // Ignore

	//called every frame
	typedef void( *UPDATE )(void *ai );
    
	INITAI _InitAI;
    
    UNITCREATED _UnitCreated;
    UNITFINISHED _UnitFinished;
    UNITDESTROYED _UnitDestroyed;
    UNITIDLE _UnitIdle;
    UNITDAMAGED _UnitDamaged;
    UNITMOVEFAILED _UnitMoveFailed;
    
    ENEMYENTERLOS _EnemyEnterLOS;
    ENEMYLEAVELOS _EnemyLeaveLOS;
    ENEMYENTERRADAR _EnemyEnterRadar;
    ENEMYLEAVERADAR _EnemyLeaveRadar;
    ENEMYDAMAGED _EnemyDamaged;
    ENEMYDESTROYED _EnemyDestroyed;
        
    GOTCHATMSG _GotChatMsg;

    UPDATE _Update;

    //void OpenLogFile( int team );
};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
