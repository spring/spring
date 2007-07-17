#include "SurveillanceHandler.h"



CSurveillanceHandler::CSurveillanceHandler(AIClasses* ai)
{
	this->ai = ai;
	currentEnemiesFullFrame = -2;
	numEnemiesFull = -2;
	
	currentEnemiesUnitsOfTypeFrame = -2;
	int numUnitDefs = ai->cb->GetNumUnitDefs();
	enemiesUnitsOfType = new int[numUnitDefs+1];
}

CSurveillanceHandler::~CSurveillanceHandler()
{
	delete [] enemiesUnitsOfType;
}


const int* CSurveillanceHandler::GetEnemiesList()
{
	if(currentEnemiesFullFrame != ai->cb->GetCurrentFrame())
	{
		currentEnemiesFullFrame = ai->cb->GetCurrentFrame();
		numEnemiesFull = ai->cheat->GetEnemyUnits(enemiesFull);
		countEnemiesFullThisFrame = 0;
	}
	countEnemiesFullThisFrame++;
	return enemiesFull;
}

int CSurveillanceHandler::GetNumberOfEnemies()
{
	if(currentEnemiesFullFrame != ai->cb->GetCurrentFrame())
	{
		currentEnemiesFullFrame = ai->cb->GetCurrentFrame();
		numEnemiesFull = ai->cheat->GetEnemyUnits(enemiesFull);
		countEnemiesFullThisFrame = 0;
	}
	return numEnemiesFull;
}

int CSurveillanceHandler::GetCountEnemiesFullThisFrame()
{
	if(currentEnemiesFullFrame != ai->cb->GetCurrentFrame())
	{
		return 0;
	}
	return countEnemiesFullThisFrame;
}



const int* CSurveillanceHandler::GetNumOfEnemiesUnitsOfType()
{
	if(currentEnemiesUnitsOfTypeFrame != ai->cb->GetCurrentFrame())
	{
		int numUnitDefs = ai->cb->GetNumUnitDefs() +1;
		// Clear the old data
		for(int i = 1; i < numUnitDefs; i++)
		{
			enemiesUnitsOfType[i] = 0;
		}
		// Get the enemy list
		int numOfEnemies = GetNumberOfEnemies();
		const int* enemies = GetEnemiesList();
		for(int i = 0; i < numOfEnemies; i++){
			enemiesUnitsOfType[ai->cheat->GetUnitDef(enemies[i])->id]++;
			//assert(ai->cheat->GetUnitDef(enemies[i]) == unittypearray[ai->cheat->GetUnitDef(enemies[i])->id].def);
		}
	}
	return enemiesUnitsOfType;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*const int* CSurveillanceHandler::GetMyUnitsList()
{
	if(currentMyUnitsFullFrame != ai->cb->GetCurrentFrame())
	{
		currentMyUnitsFullFrame = ai->cb->GetCurrentFrame();
		numMyUnitsFull = ai->cb->GetFriendlyUnits(myUnitsFull);
		countMyUnitsFullThisFrame = 0;
	}
	countMyUnitsFullThisFrame++;
	return myUnitsFull;
}

int CSurveillanceHandler::GetNumberOfMyUnits()
{
	if(currentMyUnitsFullFrame != ai->cb->GetCurrentFrame())
	{
		currentMyUnitsFullFrame = ai->cb->GetCurrentFrame();
		numMyUnitsFull = ai->cheat->GetEnemyUnits(myUnitsFull);
		countMyUnitsFullThisFrame = 0;
	}
	return numMyUnitsFull;
}

int CSurveillanceHandler::GetCountMyUnitsFullThisFrame()
{
	if(currentMyUnitsFullFrame != ai->cb->GetCurrentFrame())
	{
		return 0;
	}
	return countMyUnitsFullThisFrame;
}



const int* CSurveillanceHandler::GetNumOfMyUnitsUnitsOfType()
{
	if(currentMyUnitsUnitsOfTypeFrame != ai->cb->GetCurrentFrame())
	{
		int numUnitDefs = ai->cb->GetNumUnitDefs() +1;
		// Clear the old data
		for(int i = 1; i < numUnitDefs; i++)
		{
			myUnitsUnitsOfType[i] = 0;
		}
		// Get the enemy list
		int numOfMyUnits = GetNumberOfMyUnits();
		const int* myUnits = GetMyUnitsList();
		for(int i = 0; i < numOfMyUnits; i++){
			myUnitsUnitsOfType[ai->cb->GetUnitDef(myUnits[i])->id]++;
			//assert(ai->cheat->GetUnitDef(myUnits[i]) == unittypearray[ai->cheat->GetUnitDef(myUnits[i])->id].def);
		}
	}
	return myUnitsUnitsOfType;
}
*/
