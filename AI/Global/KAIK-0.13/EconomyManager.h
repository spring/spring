#ifndef ECONOMYMANAGER_H
#define ECONOMYMANAGER_H


#include "GlobalAI.h"

class CEconomyManager {
	public:
		CEconomyManager(AIClasses* ai);
		~CEconomyManager();

		void Update();

	private:
		void CreateResourceTask();
		AIClasses* ai;
};


#endif
