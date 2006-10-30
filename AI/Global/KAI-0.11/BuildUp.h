#ifndef BUILDUP_H
#define BUILDUP_H
/*pragma once removed*/
#include "GlobalAI.h"


class CBuildUp
{
	public:
	CBuildUp(AIClasses* ai);
	virtual ~CBuildUp();
	void Update();



	private:
	void Buildup();
	void EconBuildup(int builder, const UnitDef* factory);

	int factorycounter;
	int buildercounter;
	int storagecounter;

	AIClasses* ai;
};

#endif /* BUILDUP_H */
