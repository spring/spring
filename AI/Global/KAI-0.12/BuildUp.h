#ifndef BUILDUP_H
#define BUILDUP_H


#include "GlobalAI.h"

class CBuildUp {
	public:
		CR_DECLARE(CBuildUp);
		CBuildUp(AIClasses* ai);
		virtual ~CBuildUp();

		void Update();

	private:
		void Buildup();
		void FactoryCycle(void);
		void NukeSiloCycle(void);
		void FallbackBuild(int, int);
		const UnitDef* GetLeastBuiltBuilder(void);

		int factoryTimer;
		int builderTimer;
		int storageTimer;
		int nukeSiloTimer;

		AIClasses* ai;
};


#endif
