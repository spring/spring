#if !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_
//-------------------------------------------------------------------------
// NTai
// Copyright 2004-2008 AF
// Released under GPL 2 license
//-------------------------------------------------------------------------

#include "include.h"

namespace ntai {

	class CNTai : public IGlobalAI{
	public:
		CNTai();
		virtual ~CNTai();
		void InitAI(IGlobalAICallback* callback, int team);
		void UnitCreated(int unit);
		void UnitFinished(int unit);
		void UnitDestroyed(int unit, int attacker);
		void EnemyEnterLOS(int enemy);
		void EnemyLeaveLOS(int enemy);
		void EnemyEnterRadar(int enemy);
		void EnemyLeaveRadar(int enemy);
		void EnemyDamaged(int damaged,int attacker,float damage,float3 dir);
		void EnemyDestroyed(int enemy,int attacker);
		void UnitIdle(int unit);
		void GotChatMsg(const char* msg,int player);
		void UnitDamaged(int damaged,int attacker,float damage,float3 dir);
		void UnitMoveFailed(int unit);
		void Update();
		void Load(IGlobalAICallback* callback,std::istream *s){}
		void Save(std::ostream *s){}
		int HandleEvent (int msg, const void *data);
		int tteam;
		int team;
		Global* G;
		IGlobalAICallback* cg;
		IAICallback* acallback;

		bool Good;
	};

}

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
