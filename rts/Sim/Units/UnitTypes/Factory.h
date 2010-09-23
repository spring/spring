/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include "Building.h"
#include "System/float3.h"

#include <string>

struct UnitDef;

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
	void FinishedBuilding();
	void CreateNanoParticle();

	/// supply the build piece to speed up
	float3 CalcBuildPos(int buildPiece = -1);
	int GetBuildPiece();

	void UnitInit(const UnitDef* def, int team, const float3& position);

	float buildSpeed;

	/// if we have a unit that we want to start to nanolath when script is ready
	bool quedBuild;
	const UnitDef* nextBuild;
	std::string nextBuildName;
	CUnit* curBuild;            ///< unit that we are nanolathing
	bool opening;

	int lastBuild;              ///< last frame we wanted to build something

	void SlowUpdate();
	bool ChangeTeam(int newTeam, ChangeType type);

private:
	void SendToEmptySpot(CUnit* unit);
	void AssignBuildeeOrders(CUnit* unit);
};

#endif // __FACTORY_H__
