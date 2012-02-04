/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FACTORY_H
#define _FACTORY_H

#include "Building.h"
#include "Sim/Units/CommandAI/Command.h"
#include "System/float3.h"

struct UnitDef;
struct Command;
class CFactory;

typedef void (*FinishBuildCallBackFunc) (CFactory*, const Command&);

class CFactory : public CBuilding
{
public:
	CR_DECLARE(CFactory);

	CFactory();
	virtual ~CFactory();

	void PostLoad();

	void StartBuild(const UnitDef* buildeeDef);
	void UpdateBuild(CUnit* buildee);
	void FinishBuild(CUnit* buildee);
	void StopBuild();
	/// @return whether the to-be-built unit is enqueued
	unsigned int QueueBuild(const UnitDef* buildeeDef, const Command& buildCmd, FinishBuildCallBackFunc buildCB);

	void Update();
	void SlowUpdate();
	void DependentDied(CObject* o);
	void CreateNanoParticle(bool highPriority = false);

	/// supply the build piece to speed up
	float3 CalcBuildPos(int buildPiece = -1);
	int GetBuildPiece();

	void PreInit(const UnitDef* def, int team, int facing, const float3& position, bool build);
	bool ChangeTeam(int newTeam, ChangeType type);

	float buildSpeed;

	/// whether we are currently opening in preparation to start building
	bool opening;

	const UnitDef* curBuildDef;
	CUnit* curBuild;

	enum {
		FACTORY_SKIP_BUILD_ORDER = 0,
		FACTORY_KEEP_BUILD_ORDER = 1,
		FACTORY_NEXT_BUILD_ORDER = 2,
	};

private:
	void SendToEmptySpot(CUnit* unit);
	void AssignBuildeeOrders(CUnit* unit);

	int nextBuildUnitDefID;
	int lastBuildUpdateFrame;

	FinishBuildCallBackFunc finishedBuildFunc;
	Command finishedBuildCommand;
};

#endif // _FACTORY_H
