#include "EconomyManager.h"

CEconomyManager::CEconomyManager(AIClasses* ai)
{
	this->ai=ai;
}

CEconomyManager::~CEconomyManager()
{
}


void CEconomyManager::Update()
{
	L("Starting Economy Update");
	UpdateNeed4Def(3);
	L("Economy Update Done!");
}



void CEconomyManager::UpdateNeed4Def(float firingtime) // Very early sketch, dont bother changing it yet
{
	//L("Updating need for defence");
	TIMER_START;
	float Need4Def = 0;
	for(int i = 1; i <= ai->cb->GetNumUnitDefs(); i++){
		if(ai->dc->MyDefences[i].Damage > 0 && ai->dc->TheirArmy[i].Hitpoints > 0){			
			Need4Def += max(ai->dc->TheirArmy[i].Hitpoints - (ai->dc->MyDefences[i].Damage * firingtime),0.0f);
			//L("TheirArmor: " << ai->dc->TheirArmy[i].Hitpoints << " MyDamage" << ai->dc->MyDefences[i].Damage);
		}
	}
	if(ai->dc->TheirTotalCost > 0){
		Need4Def /= ai->dc->TheirTotalCost; // WIP 
		//L("Tempneed: " << Need4Def << " Time Taken: " << TIMER_SECS);
	}
	else
		Need4Def = 0;
}
