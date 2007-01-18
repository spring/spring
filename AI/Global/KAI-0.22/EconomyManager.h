#ifndef ECONOMYMANAGER_H
#define ECONOMYMANAGER_H

#include "GlobalAI.h"

class CEconomyManager
{
public:
	CEconomyManager(AIClasses* ai);
	virtual ~CEconomyManager();
	void Update();

	float Need4M;
	float Need4E;
	float Need4BT;
	float Need4Army;
	float Need4Def;	
	float Need4IS;

private:
	
	void UpdateNeed4Def(float firingtime);


	void CreateResourceTask();

	
	const UnitDef* GetUnitByScore(int category);
	float GetScore(const UnitDef* unit);

	AIClasses* ai;
};

#endif /* ECONOMYMANAGER_H */
