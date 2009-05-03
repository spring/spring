#ifndef KAIK_BUILDUP_HDR
#define KAIK_BUILDUP_HDR

#include "Defines.h"
struct AIClasses;

enum BuildState {
	BUILD_INIT,
	BUILD_NUKE,
	BUILD_M_STALL,
	BUILD_E_STALL,
	BUILD_E_EXCESS,
	BUILD_DEFENSE,
	BUILD_FACTORY
};

struct EconState {
	float mIncome;
	float eIncome;
	float mLevel;
	float eLevel;
	float mStorage;
	float eStorage;
	float mUsage;
	float eUsage;
	bool makersOn;

	float m1;
	float m2;
	float e1;
	float e2;

	bool mLevel50;
	bool eLevel50;
	bool eLevel80;

	bool mStall;
	bool eStall;
	bool mOverflow;

	bool eLevelMed;
	bool mLevelLow;

	bool factFeasM;
	bool factFeasE;
	bool factFeas;

	bool b1;
	bool b2;
	bool b3;

	bool buildNukeSilo;

	int numM;
	int numE;

	int numDefenses;
	int numFactories;

	int nIdleBuilders;
	int builderID;
	const UnitDef* builderDef;
	const UnitDef* factoryDef;
};

class CBuildUp {
	public:
		CR_DECLARE(CBuildUp);

		CBuildUp(AIClasses* ai);
		~CBuildUp();

		void Update(int);

	private:
		BuildState GetBuildState(int, const EconState*) const;
		void GetEconState(EconState*) const;

		void Buildup(int);
		void FactoryCycle(int);
		void NukeSiloCycle(void);
		void FallbackBuild(int, int);
		bool BuildNow(int, UnitCategory, const UnitDef*);
		const UnitDef* GetLeastBuiltBuilder(void);
		bool BuildUpgradeExtractor(int);
		bool BuildUpgradeReactor(int);

		int factoryTimer;
		int builderTimer;
		int storageTimer;
		int nukeSiloTimer;

		EconState econState;

		AIClasses* ai;
};


#endif
