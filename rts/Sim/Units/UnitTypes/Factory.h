/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include "Building.h"
#include "Sim/Units/UnitDef.h"
#include <string>

class CFactory : public CBuilding
{
public:
	CR_DECLARE(CFactory);

	CFactory();
	virtual ~CFactory();
	void PostLoad();
	void StopBuild();
	void StartBuild(const UnitDef* ud);
	void Update();
	void DependentDied(CObject* o);
	void FinishedBuilding(void);
	void CreateNanoParticle(void);

	float3 CalcBuildPos(int buildPiece=-1); // supply the build piece to speed up
	int GetBuildPiece();

	void UnitInit(const UnitDef* def, int team, const float3& position);

	float buildSpeed;

	bool quedBuild;						//if we have a unit that we want to start to nanolath when script is ready
	const UnitDef* nextBuild;
	std::string nextBuildName;
	CUnit* curBuild;					//unit that we are nanolathing
	bool opening;

	int lastBuild;						//last frame we wanted to build something

	void SlowUpdate(void);
	bool ChangeTeam(int newTeam, ChangeType type);

private:
	void SendToEmptySpot(CUnit* unit);
	void AssignBuildeeOrders(CUnit* unit);
};

#endif // __FACTORY_H__
