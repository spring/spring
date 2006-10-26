#ifndef ECONOMYMANAGER_H
#define ECONOMYMANAGER_H
/*pragma once removed*/
#include "GlobalAI.h"

class CEconomyManager
{
public:
	CEconomyManager(AIClasses* ai);
	virtual ~CEconomyManager();
	void Update();

private:

	void CreateResourceTask();
	
	const UnitDef* GetUnitByScore(int category);
	float GetScore(const UnitDef* unit);

	AIClasses* ai;
};

#endif /* ECONOMYMANAGER_H */
