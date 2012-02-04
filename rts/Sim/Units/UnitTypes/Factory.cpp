/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Factory.h"
#include "Game/GameHelper.h"
#include "Game/WaitCommandsAI.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/Sound/SoundChannels.h"
#include "System/Sync/SyncTracer.h"
#include "System/mmgr.h"

#define PLAY_SOUNDS 1
#if (PLAY_SOUNDS == 1)
#include "Game/GlobalUnsynced.h"
#endif

CR_BIND_DERIVED(CFactory, CBuilding, );

CR_REG_METADATA(CFactory, (
	CR_MEMBER(buildSpeed),
	CR_MEMBER(opening),
	CR_MEMBER(curBuild),
	CR_MEMBER(nextBuildUnitDefID),
	CR_MEMBER(lastBuildUpdateFrame),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFactory::CFactory():
	buildSpeed(100.0f),
	opening(false),
	curBuildDef(NULL),
	curBuild(NULL),
	nextBuildUnitDefID(-1),
	lastBuildUpdateFrame(-1),
	finishedBuildFunc(NULL)
{
}

CFactory::~CFactory() {
	if (curBuild != NULL) {
		curBuild->KillUnit(false, true, NULL);
		curBuild = NULL;
	}
}



void CFactory::PostLoad()
{
	curBuildDef = unitDefHandler->GetUnitDefByID(nextBuildUnitDefID);

	if (opening) {
		script->Activate();
	}
	if (curBuild) {
		script->StartBuilding();
	}
}

void CFactory::PreInit(const UnitDef* def, int team, int facing, const float3& position, bool build)
{
	buildSpeed = def->buildSpeed / TEAM_SLOWUPDATE_RATE;
	CBuilding::PreInit(def, team, facing, position, build);
}

int CFactory::GetBuildPiece()
{
	return script->QueryBuildInfo();
}

// GetBuildPiece() is called if piece < 0
float3 CFactory::CalcBuildPos(int buildPiece)
{
	const float3 relBuildPos = script->GetPiecePos(buildPiece < 0 ? GetBuildPiece() : buildPiece);
	const float3 absBuildPos = pos + frontdir * relBuildPos.z + updir * relBuildPos.y + rightdir * relBuildPos.x;
	return absBuildPos;
}



void CFactory::Update()
{
	if (beingBuilt) {
		// factory is under construction, cannot build anything yet
		CUnit::Update();
		return;
	}


	if (curBuildDef != NULL) {
		if (!opening && !stunned) {
			script->Activate();
			groundBlockingObjectMap->OpenBlockingYard(this, curYardMap);
			opening = true;

			// make sure the idle-check does not immediately trigger
			// (scripts have 7 seconds to set inBuildStance to true)
			lastBuildUpdateFrame = gs->frameNum;
		}

		if (inBuildStance && !stunned) {
			StartBuild(curBuildDef);
		}
	}

	if (curBuild != NULL && !beingBuilt) {
		UpdateBuild(curBuild);
		FinishBuild(curBuild);
	}

	const bool mayClose = (!stunned && opening && (gs->frameNum >= (lastBuildUpdateFrame + GAME_SPEED * 7)));
	const bool canClose = (curBuild == NULL && groundBlockingObjectMap->CanCloseYard(this));

	if (mayClose && canClose) {
		// close the factory after inactivity
		groundBlockingObjectMap->CloseBlockingYard(this, curYardMap);
		opening = false;
		script->Deactivate();
	}

	CBuilding::Update();
}



void CFactory::StartBuild(const UnitDef* buildeeDef) {
	const float3        buildPos = CalcBuildPos();
	const CSolidObject* solidObj = groundBlockingObjectMap->GroundBlocked(buildPos, true);

	// wait until buildPos is no longer blocked (eg. by a previous buildee)
	if (solidObj == NULL || solidObj == this) {
		CUnit* b = unitLoader->LoadUnit(buildeeDef, buildPos, team, true, buildFacing, this);

		if (!unitDef->canBeAssisted) {
			b->soloBuilder = this;
			b->AddDeathDependence(this, DEPENDENCE_BUILDER);
		}

		AddDeathDependence(b, DEPENDENCE_BUILD);
		script->StartBuilding();

		// set curBuildDef to NULL to indicate construction
		// has started, otherwise we would keep being called
		curBuild = b;
		curBuildDef = NULL;

		#if (PLAY_SOUNDS == 1)
		if (losStatus[gu->myAllyTeam] & LOS_INLOS) {
			Channels::General.PlayRandomSample(unitDef->sounds.build, buildPos);
		}
		#endif
	} else {
		helper->BuggerOff(buildPos, radius + 8, true, true, team, NULL);
	}
}

void CFactory::UpdateBuild(CUnit* buildee) {
	if (stunned)
		return;

	// factory not under construction and
	// nanolathing unit: continue building
	lastBuildUpdateFrame = gs->frameNum;

	// buildPiece is the rotating platform
	const int buildPiece = GetBuildPiece();
	const float3& buildPos = CalcBuildPos(buildPiece);
	const CMatrix44f& mat = script->GetPieceMatrix(buildPiece);
	const int h = GetHeadingFromVector(mat[2], mat[10]); //! x.z, z.z

	float3 buildeePos = buildPos;

	// rotate unit nanoframe with platform
	buildee->heading = (-h + GetHeadingFromFacing(buildFacing)) & (SPRING_CIRCLE_DIVS - 1);

	if (buildee->unitDef->floatOnWater && (buildeePos.y <= 0.0f))
		buildeePos.y = -buildee->unitDef->waterline;

	buildee->Move3D(buildeePos, false);
	buildee->UpdateDirVectors(false);
	buildee->UpdateMidPos();

	const CCommandQueue& queue = commandAI->commandQue;

	if (!queue.empty() && (queue.front().GetID() == CMD_WAIT)) {
		buildee->AddBuildPower(0, this);
	} else {
		if (buildee->AddBuildPower(buildSpeed, this)) {
			CreateNanoParticle();
		}
	}
}

void CFactory::FinishBuild(CUnit* buildee) {
	if (buildee->beingBuilt) { return; }
	if (unitDef->fullHealthFactory && buildee->health < buildee->maxHealth) { return; }

	{
		GML_RECMUTEX_LOCK(group); // FinishBuild

		if (group && buildee->group == 0) {
			buildee->SetGroup(group);
		}
	}

	const CCommandAI* bcai = buildee->commandAI;
	const bool assignOrders = bcai->commandQue.empty() || (dynamic_cast<const CMobileCAI*>(bcai) && static_cast<const CMobileCAI*>(bcai)->unimportantMove);

	if (assignOrders) {
		AssignBuildeeOrders(buildee);
		waitCommandsAI.AddLocalUnit(buildee, this);
	}

	// inform our commandAI
	finishedBuildFunc(this, finishedBuildCommand);
	finishedBuildFunc = NULL;

	eventHandler.UnitFromFactory(buildee, this, !assignOrders);
	StopBuild();
}



unsigned int CFactory::QueueBuild(const UnitDef* buildeeDef, const Command& buildCmd, FinishBuildCallBackFunc buildFunc)
{
	assert(!beingBuilt);
	assert(buildeeDef != NULL);

	if (uh->unitsByDefs[team][buildeeDef->id].size() >= buildeeDef->maxThisUnit)
		return FACTORY_SKIP_BUILD_ORDER;
	if (teamHandler->Team(team)->AtUnitLimit())
		return FACTORY_KEEP_BUILD_ORDER;
	if (luaRules && !luaRules->AllowUnitCreation(buildeeDef, this, NULL))
		return FACTORY_SKIP_BUILD_ORDER;
	if (curBuild != NULL)
		return FACTORY_KEEP_BUILD_ORDER;

	finishedBuildFunc = buildFunc;
	finishedBuildCommand = buildCmd;
	curBuildDef = buildeeDef;
	nextBuildUnitDefID = buildeeDef->id;
	return FACTORY_NEXT_BUILD_ORDER;
}

void CFactory::StopBuild()
{
	// cancel a build-in-progress
	script->StopBuilding();

	if (curBuild) {
		if (curBuild->beingBuilt) {
			AddMetal(curBuild->metalCost * curBuild->buildProgress, false);
			curBuild->KillUnit(false, true, NULL);
		}
		DeleteDeathDependence(curBuild, DEPENDENCE_BUILD);
	}

	curBuild = NULL;
	curBuildDef = NULL;
	finishedBuildFunc = NULL;
}

void CFactory::DependentDied(CObject* o)
{
	if (o == curBuild) {
		curBuild = NULL;
		StopBuild();
	}

	CUnit::DependentDied(o);
}



void CFactory::SendToEmptySpot(CUnit* unit)
{
	float r = radius * 1.7f + unit->radius * 4;
	float3 foundPos = pos + frontdir * r;

	for (int a = 0; a < 20; ++a) {
		float3 testPos = pos + frontdir * r * cos(a * PI / 10) + rightdir * r * sin(a * PI / 10);
		testPos.y = ground->GetHeightAboveWater(testPos.x, testPos.z);

		if (qf->GetSolidsExact(testPos, unit->radius * 1.5f).empty()) {
			foundPos = testPos;
			break;
		}
	}

	Command c(CMD_MOVE);
	c.params.push_back(foundPos.x);
	c.params.push_back(foundPos.y);
	c.params.push_back(foundPos.z);
	unit->commandAI->GiveCommand(c);
}

void CFactory::AssignBuildeeOrders(CUnit* unit) {
	const CFactoryCAI* facAI = static_cast<CFactoryCAI*>(commandAI);
	const CCommandQueue& newUnitCmds = facAI->newUnitCommands;

	if (newUnitCmds.empty()) {
		SendToEmptySpot(unit);
		return;
	}

	Command c(CMD_MOVE);

	if (!unit->unitDef->canfly) {
		// HACK: when a factory has a rallypoint set far enough away
		// to trigger the non-admissable path estimators, we want to
		// avoid units getting stuck inside by issuing them an extra
		// move-order. However, this order can *itself* cause the PF
		// system to consider the path blocked if the extra waypoint
		// falls within the factory's confines, so use a wide berth.
		const float xs = unitDef->xsize * SQUARE_SIZE * 0.5f;
		const float zs = unitDef->zsize * SQUARE_SIZE * 0.5f;

		float tmpDst = 2.0f;
		float3 tmpPos = unit->pos + (frontdir * this->radius * tmpDst);

		if (buildFacing == FACING_NORTH || buildFacing == FACING_SOUTH) {
			while ((tmpPos.z >= unit->pos.z - zs && tmpPos.z <= unit->pos.z + zs)) {
				tmpDst += 0.5f;
				tmpPos = unit->pos + (frontdir * this->radius * tmpDst);
			}
		} else {
			while ((tmpPos.x >= unit->pos.x - xs && tmpPos.x <= unit->pos.x + xs)) {
				tmpDst += 0.5f;
				tmpPos = unit->pos + (frontdir * this->radius * tmpDst);
			}
		}

		c.params.push_back(tmpPos.x);
		c.params.push_back(tmpPos.y);
		c.params.push_back(tmpPos.z);
		unit->commandAI->GiveCommand(c);
	}

	for (CCommandQueue::const_iterator ci = newUnitCmds.begin(); ci != newUnitCmds.end(); ++ci) {
		c = *ci;
		c.options |= SHIFT_KEY;
		unit->commandAI->GiveCommand(c);
	}
}



void CFactory::SlowUpdate(void)
{
	if (!transporter)
		helper->BuggerOff(pos - float3(0.01f, 0, 0.02f), radius, true, true, team, NULL);

	CBuilding::SlowUpdate();
}

bool CFactory::ChangeTeam(int newTeam, ChangeType type)
{
	StopBuild();
	return (CBuilding::ChangeTeam(newTeam, type));
}


void CFactory::CreateNanoParticle(bool highPriority)
{
	const int piece = script->QueryNanoPiece();

#ifdef USE_GML
	if (gs->frameNum - lastDrawFrame > 20)
		return;
#endif

	const float3 relWeaponFirePos = script->GetPiecePos(piece);
	const float3 weaponPos = pos
		+ (frontdir * relWeaponFirePos.z)
		+ (updir    * relWeaponFirePos.y)
		+ (rightdir * relWeaponFirePos.x);

	// unsynced
	ph->AddNanoParticle(weaponPos, curBuild->midPos, unitDef, team, highPriority);
}
