#pragma once
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