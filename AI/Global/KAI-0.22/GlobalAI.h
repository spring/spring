#ifndef GLOBALAI_H
#define GLOBALAI_H


#include "Include.h"

const char AI_NAME[] = "KAI 0.22 (0.21 Patched)";

using namespace std;
class CAttackHandler;

class CGlobalAI : public IGlobalAI {
	public:
		CGlobalAI();
		virtual ~CGlobalAI();

		void InitAI(IGlobalAICallback* callback, int team);

		void UnitCreated(int unit);
		void UnitFinished(int unit);
		void UnitDestroyed(int unit, int attacker);
		void UnitIdle(int unit);
		void UnitDamaged(int damaged, int attacker, float damage, float3 dir);
		void UnitMoveFailed(int unit);

		void EnemyEnterLOS(int enemy);
		void EnemyLeaveLOS(int enemy);
		void EnemyEnterRadar(int enemy);
		void EnemyLeaveRadar(int enemy);
		void EnemyDamaged(int damaged, int attacker, float damage, float3 dir);
		void EnemyDestroyed(int enemy,int attacker);

		void GotChatMsg(const char* msg, int player);
		int HandleEvent (int msg,const void *data);

		void Update();


		AIClasses* ai;
		vector<CUNIT> MyUnits;
		char c[512];

	private:
		int totalSumTime;
		int updateTimerGroup;
		int econTrackerFrameUpdate;
		int updateTheirDistributionTime;
		int updateMyDistributionTime;
		int builUpTime;
		int idleUnitUpdateTime;
		int attackHandlerUpdateTime;
		int MMakerUpdateTime;
		int unitCreatedTime;
		int unitFinishedTime;
		int unitDestroyedTime;
		int unitIdleTime;
		int economyManagerUpdateTime;
		int globalAILogTime;
		int threatMapTime;
};


#endif
