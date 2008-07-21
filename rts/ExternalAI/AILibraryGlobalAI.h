/*
	Copyright 2008  Nicolas Wu
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AILIBRARYGLOBALAI_H
#define AILIBRARYGLOBALAI_H


// This class extends GlobalAI, but implements the
// AILibrary interface, which means it can deal with 
// the new AI interface.

// Unfortunately, GlobalAIHandler uses the ai parameter of CGlobalAI,
// and so expects to use the IGlobalAI object.
// We get around this by implementing all the IGlobalAI interface here
// as well, and setting ai to this.

#include "AILibrary.h"
#include "GlobalAI.h"
#include "IGlobalAI.h"

class CAILibraryGlobalAI : public CGlobalAI, public IGlobalAI, public CAILibrary {
	
public:
	CAILibraryGlobalAI(const char* libName, int team);
	~CAILibraryGlobalAI();
	
	virtual void LoadAILib(int team, const char* libName, bool postLoad);
	virtual void Serialize(creg::ISerializer*);
	virtual void PostLoad();
	virtual void Load(std::istream *s);
	virtual void PreDestroy();
	
	virtual void InitAI(IGlobalAICallback* callback, int team);
	virtual void UnitCreated(int unit);
	virtual void UnitFinished(int unit);
	virtual void UnitDestroyed(int unit, int attacker);
	virtual void EnemyEnterLOS(int enemy);
	virtual void EnemyLeaveLOS(int enemy);
	virtual void EnemyEnterRadar(int enemy);
	virtual void EnemyLeaveRadar(int enemy);
	virtual void EnemyDamaged(int damaged,int attacker,float damage,float3 dir);
	virtual void EnemyDestroyed(int enemy, int attacker);
	virtual void UnitIdle(int unit);
	virtual void GotChatMsg(const char* msg,int player);
	virtual void UnitDamaged(int damaged,int attacker,float damage,float3 dir);
	virtual void UnitMoveFailed(int unit);
	virtual int HandleEvent (int msg, const void *data);
	virtual void Update();
	virtual void Load(IGlobalAICallback* callback,std::istream *s);
	virtual void Save(std::ostream *s);
};

#endif /*AILIBRARYGLOBALAI_H*/
