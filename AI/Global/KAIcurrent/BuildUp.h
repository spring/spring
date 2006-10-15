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
	list<int> MexUpgraders;


	private:
	void Buildup();
	void FactoryBuildup();
	
	/*
	Returns the factory that is needed most (globaly).
	*/
	const UnitDef* CBuildUp::GetBestFactoryThatCanBeBuilt(float3 builderPos);

	const UnitDef* GetBestMexThatCanBeBuilt();

	/*
	Returns true if the builder was assigned an order.
	factory is the current target factory thats needed (globaly).
	*/
	bool EconBuildup(int builder, const UnitDef* factory, bool forceUseBuilder);
	
	/*
	Returns true if the builder was assigned an order.
	*/
	bool DefenceBuildup(int builder, bool forceUseBuilder);

	/*
	Returns true if the builder was assigned an order to build a factory or
	the builder was added to a factory (assist) or
	it helps to make a factory.
	*/
	bool AddBuilderToFactory(int builder, const UnitDef* factory, bool forceUseBuilder);

	/*
	Returns true if the builder was assigned an order.
	Makes either metal or energy storage
	*/
	bool MakeStorage(int builder, bool forceUseBuilder);
	
	float totalbuildingcosts;
	float totaldefensecosts;


	int factorycounter;
	int buildercounter;
	int storagecounter;

	AIClasses* ai;
};

#endif /* BUILDUP_H */
