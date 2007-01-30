#ifndef BUILDUP_H
#define BUILDUP_H


#include "GlobalAI.h"

class CBuildUp {
	public:
		CBuildUp(AIClasses* ai);
		virtual ~CBuildUp();

		void Update();

	private:
		void Buildup();
		void FactoryCycle(int);
		void EconBuildup(int builder, const UnitDef* factory);

		int factoryCounter;
		int builderCounter;
		int storageCounter;

		AIClasses* ai;
};


#endif
