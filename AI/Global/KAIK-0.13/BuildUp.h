#ifndef BUILDUP_H
#define BUILDUP_H


#include "GlobalAI.h"

class CBuildUp {
	public:
		CR_DECLARE(CBuildUp);

		CBuildUp(AIClasses* ai);
		~CBuildUp();

		void Update(int);

	private:
		void Buildup(int);
		void FactoryCycle(void);
		void NukeSiloCycle(void);
		void FallbackBuild(int, int);
		bool BuildNow(int, int);
		bool BuildNow(int, int, const UnitDef*);
		const UnitDef* GetLeastBuiltBuilder(void);
		bool BuildUpgradeExtractor(int);

		int factoryTimer;
		int builderTimer;
		int storageTimer;
		int nukeSiloTimer;

		AIClasses* ai;
};


#endif
