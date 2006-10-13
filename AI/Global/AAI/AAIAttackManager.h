//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------

#pragma once

#include "aidef.h"

class AAI;
class AAIBrain;
class AAIBuildTable;
class AAIMap;
class AAIAttack;


class AAIAttackManager
{
public:
	AAIAttackManager(AAI *ai, IAICallback *cb, AAIBuildTable *bt);
	~AAIAttackManager(void);

	void Update();

	void GetNextDest(AAIAttack *attack);

	void LaunchAttack(); 

	void StopAttack(AAIAttack *attack);

	list<AAIAttack*> attacks;


private:

	IAICallback *cb;
	AAI *ai;
	AAIBrain *brain;
	AAIBuildTable *bt;
	AAIMap *map;
};
