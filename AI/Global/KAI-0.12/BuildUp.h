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
		void FactoryCycle(int);
		void FallbackBuild(int, int);

		int factoryCounter;
		int builderCounter;
		int storageCounter;

		AIClasses* ai;
};


#endif
