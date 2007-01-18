#ifndef SURVEILLANCEHANDLER_H
#define SURVEILLANCEHANDLER_H

#include "GlobalAI.h"


class CSurveillanceHandler
{
public:
	CSurveillanceHandler(AIClasses* ai);
	virtual ~CSurveillanceHandler();
	
	/*
	Returns a list of all current enemys "ai->cheat->GetEnemyUnits()"
	Its cached and fast.
	Dont change the data in the list.
	*/
	const int* GetEnemiesList();
	
	/*
	Used to know how far to index the list returned by GetEnemiesList()
	*/
	int GetNumberOfEnemies();
	
	// Just a fun function. A speed test thing
	int GetCountEnemiesFullThisFrame();


	/*
	Returns a list of all current enemys "ai->cb->GetEnemyUnits()"
	Its cached and fast.
	Dont change the data in the list.
	*/
	//const int* GetEnemiesList();
	
	/*
	Used to know how far to index the list returned by GetEnemiesList()
	*/
	//int GetNumberOfEnemies();

	/*
	Returns a list of the count of all current enemys.
	Its indexed by GetUnitDef()->id
	So Sum { GetUnitDef(enemies[i])->id == index } == enemyUnitsOfType[index]
	Its cached and fast.
	Dont change the data in the list.
	*/
	const int* GetNumOfEnemiesUnitsOfType();




	/*const int* GetMyUnitsList();

	int GetNumberOfMyUnits();

	int GetCountMyUnitsFullThisFrame();
	const int* GetNumOfMyUnitsUnitsOfType();*/

private:
	AIClasses *ai;
	int currentEnemiesFullFrame;
	int numEnemiesFull;
	int enemiesFull[MAXUNITS];
	int countEnemiesFullThisFrame;
	//int enemiesBuildings[MAXUNITS];
	
	int currentEnemiesUnitsOfTypeFrame;
	int* enemiesUnitsOfType; // Dont know the size yet

	/*int currentMyUnitsFullFrame;
	int numMyUnitsFull;
	int myUnitsFull[MAXUNITS];
	int countMyUnitsFullThisFrame;
	//int enemiesBuildings[MAXUNITS];
	
	int currentMyUnitsUnitsOfTypeFrame;
	int* myUnitsUnitsOfType; // Dont know the size yet*/
	
};

#endif /* SURVEILLANCEHANDLER_H */
