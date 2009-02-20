#ifndef GLOBALAI_H
#define GLOBALAI_H


#include "Unit.h"
#include "Include.h"

class CGlobalAI: public IGlobalAI {
	public:
		CR_DECLARE(CGlobalAI);

		CGlobalAI();
		~CGlobalAI();

		void InitAI(IGlobalAICallback* callback, int team);

		void UnitCreated(int unit);
		void UnitFinished(int unit);
		void UnitDestroyed(int unit, int attacker);
		void UnitIdle(int unit);
		void UnitDamaged(int damaged, int attacker, float damage, float3 dir);
		void EnemyDamaged(int damaged, int attacker, float damage, float3 dir);
		void UnitMoveFailed(int unit);

		void EnemyEnterLOS(int enemy);
		void EnemyLeaveLOS(int enemy);
		void EnemyEnterRadar(int enemy);
		void EnemyLeaveRadar(int enemy);
		void EnemyDestroyed(int enemy, int attacker);

		void GotChatMsg(const char* msg, int player);
		int HandleEvent(int msg, const void* data);

		void Update();

		void Load(IGlobalAICallback* callback, std::istream* ifs);
		void Save(std::ostream* ofs);
		void PostLoad(void);
		void Serialize(creg::ISerializer* s);

		AIClasses* GetAi();

	private:
		AIClasses* ai;

		static const unsigned int c_maxSize = 1024;
		char c[c_maxSize];
};

#endif // GLOBALAI_H
