/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Factory.h"
#include "Game/GameHelper.h"
#include "Game/WaitCommandsAI.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/creg/DefTypes.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/SyncTracer.h"

#include "Game/GlobalUnsynced.h"


CR_BIND_DERIVED(CFactory, CBuilding, )
CR_REG_METADATA(CFactory, (
	CR_MEMBER(buildSpeed),
	CR_MEMBER(lastBuildUpdateFrame),
	CR_MEMBER(curBuildDef),
	CR_MEMBER(curBuild),
	CR_MEMBER(finishedBuildCommand),
	CR_MEMBER(nanoPieceCache)
))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFactory::CFactory():
	CBuilding(),
	buildSpeed(100.0f),
	curBuild(nullptr),
	curBuildDef(nullptr),
	lastBuildUpdateFrame(-1)
{
}

void CFactory::KillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, bool showDeathSequence)
{
	if (curBuild != nullptr) {
		curBuild->KillUnit(nullptr, false, true);
		curBuild = nullptr;
	}

	CUnit::KillUnit(attacker, selfDestruct, reclaimed, showDeathSequence);
}

void CFactory::PreInit(const UnitLoadParams& params)
{
	unitDef = params.unitDef;
	buildSpeed = unitDef->buildSpeed / TEAM_SLOWUPDATE_RATE;

	CBuilding::PreInit(params);
}



float3 CFactory::CalcBuildPos(int buildPiece)
{
	const float3 relBuildPos = script->GetPiecePos((buildPiece < 0)? script->QueryBuildInfo() : buildPiece);
	const float3 absBuildPos = this->GetObjectSpacePos(relBuildPos);
	return absBuildPos;
}



void CFactory::Update()
{
	nanoPieceCache.Update();

	if (beingBuilt) {
		// factory is under construction, cannot build anything yet
		CUnit::Update();

		// this can happen if we started being reclaimed *while* building a
		// unit, in which case our buildee can either be allowed to finish
		// construction (by assisting builders) or has to be killed --> the
		// latter is easier
		if (curBuild != nullptr)
			StopBuild();

		return;
	}


	if (curBuildDef != nullptr) {
		// if there is a unit blocking the factory's exit while
		// open and already in build-stance, StartBuild returns
		// early whereas while *closed* (!open) a blockee causes
		// CanOpenYard to return false so the Activate callin is
		// never called
		// the radius can not be too large or assisting (mobile)
		// builders around the factory will be disturbed by this
		if ((gs->frameNum & (UNIT_SLOWUPDATE_RATE >> 1)) == 0)
			CGameHelper::BuggerOff(pos + frontdir * radius * 0.5f, radius * 0.5f, true, true, team, this);

		if (!yardOpen && !IsStunned()) {
			if (groundBlockingObjectMap.CanOpenYard(this)) {
				groundBlockingObjectMap.OpenBlockingYard(this); // set yardOpen
				script->Activate(); // set buildStance

				// make sure the idle-check does not immediately trigger
				// (scripts have 7 seconds to set inBuildStance to true)
				lastBuildUpdateFrame = gs->frameNum;
			}
		}

		if (yardOpen && inBuildStance && !IsStunned()) {
			StartBuild(curBuildDef);
		}
	}

	if (curBuild != nullptr) {
		UpdateBuild(curBuild);
		FinishBuild(curBuild);
	}

	const bool wantClose = (!IsStunned() && yardOpen && (gs->frameNum >= (lastBuildUpdateFrame + GAME_SPEED * (UNIT_SLOWUPDATE_RATE >> 1))));
	const bool closeYard = (wantClose && curBuild == nullptr && groundBlockingObjectMap.CanCloseYard(this));

	if (closeYard) {
		// close the factory after inactivity
		groundBlockingObjectMap.CloseBlockingYard(this);
		script->Deactivate();
	}

	CBuilding::Update();
}



void CFactory::StartBuild(const UnitDef* buildeeDef) {
	if (isDead)
		return;

	const float3& buildPos = CalcBuildPos();
	const bool blocked = groundBlockingObjectMap.GroundBlocked(buildPos, this);

	// wait until buildPos is no longer blocked (eg. by a previous buildee)
	//
	// it might rarely be the case that a unit got stuck inside the factory
	// or died right after completion and left some wreckage, but that is up
	// to players to fix
	if (blocked)
		return;

	UnitLoadParams buildeeParams = {buildeeDef, this, buildPos, ZeroVector, -1, team, buildFacing, true, false};
	CUnit* buildee = unitLoader->LoadUnit(buildeeParams);

	if (!unitDef->canBeAssisted) {
		buildee->soloBuilder = this;
		buildee->AddDeathDependence(this, DEPENDENCE_BUILDER);
	}

	AddDeathDependence(buildee, DEPENDENCE_BUILD);
	script->StartBuilding();

	// set curBuildDef to NULL to indicate construction
	// has started, otherwise we would keep being called
	curBuild = buildee;
	curBuildDef = nullptr;

	if (losStatus[gu->myAllyTeam] & LOS_INLOS) {
		Channels::General->PlayRandomSample(unitDef->sounds.build, buildPos);
	}
}

void CFactory::UpdateBuild(CUnit* buildee) {
	if (IsStunned())
		return;

	// factory not under construction and
	// nanolathing unit: continue building
	lastBuildUpdateFrame = gs->frameNum;

	// buildPiece is the rotating platform
	const int buildPiece = script->QueryBuildInfo();

	const float3& buildPos = CalcBuildPos(buildPiece);
	const CMatrix44f& buildPieceMat = script->GetPieceMatrix(buildPiece);

	const int buildPieceHeading = GetHeadingFromVector(buildPieceMat[2], buildPieceMat[10]); //! x.z, z.z
	const int buildFaceHeading = GetHeadingFromFacing(buildFacing);

	float3 buildeePos = buildPos;

	// note: basically StaticMoveType::SlowUpdate()
	if (buildee->FloatOnWater() && buildee->IsInWater())
		buildeePos.y = -buildee->moveType->GetWaterline();

	// rotate unit nanoframe with platform
	buildee->Move(buildeePos, false);
	buildee->SetHeading((-buildPieceHeading + buildFaceHeading) & (SPRING_CIRCLE_DIVS - 1), false);

	const CCommandQueue& queue = commandAI->commandQue;

	if (!queue.empty() && (queue.front().GetID() == CMD_WAIT)) {
		buildee->AddBuildPower(this, 0.0f);
		return;
	}

	if (!buildee->AddBuildPower(this, buildSpeed))
		return;

	CreateNanoParticle();
}

void CFactory::FinishBuild(CUnit* buildee) {
	if (buildee->beingBuilt)
		return;
	if (unitDef->fullHealthFactory && buildee->health < buildee->maxHealth)
		return;

	if (group != nullptr && buildee->group == nullptr)
		buildee->SetGroup(group, true);

	const CCommandAI* bcai = buildee->commandAI;
	// if not idle, the buildee already has user orders
	const bool buildeeIdle = (bcai->commandQue.empty());
	const bool buildeeMobile = (dynamic_cast<const CMobileCAI*>(bcai) != nullptr);

	if (buildeeIdle || buildeeMobile) {
		AssignBuildeeOrders(buildee);
		waitCommandsAI.AddLocalUnit(buildee, this);
	}

	// inform our commandAI
	CFactoryCAI* factoryCAI = static_cast<CFactoryCAI*>(commandAI);
	factoryCAI->FactoryFinishBuild(finishedBuildCommand);

	eventHandler.UnitFromFactory(buildee, this, !buildeeIdle);
	StopBuild();
}



unsigned int CFactory::QueueBuild(const UnitDef* buildeeDef, const Command& buildCmd)
{
	assert(!beingBuilt);
	assert(buildeeDef != nullptr);

	if (curBuild != nullptr)
		return FACTORY_KEEP_BUILD_ORDER;
	if (unitHandler.NumUnitsByTeamAndDef(team, buildeeDef->id) >= buildeeDef->maxThisUnit)
		return FACTORY_SKIP_BUILD_ORDER;
	if (teamHandler.Team(team)->AtUnitLimit())
		return FACTORY_KEEP_BUILD_ORDER;
	if (!eventHandler.AllowUnitCreation(buildeeDef, this, nullptr))
		return FACTORY_SKIP_BUILD_ORDER;

	finishedBuildCommand = buildCmd;
	curBuildDef = buildeeDef;

	// signal that the build-order was accepted (queued)
	return FACTORY_NEXT_BUILD_ORDER;
}

void CFactory::StopBuild()
{
	// cancel a build-in-progress
	script->StopBuilding();

	if (curBuild) {
		if (curBuild->beingBuilt) {
			AddMetal(curBuild->cost.metal * curBuild->buildProgress, false);
			curBuild->KillUnit(nullptr, false, true);
		}
		DeleteDeathDependence(curBuild, DEPENDENCE_BUILD);
	}

	curBuild = nullptr;
	curBuildDef = nullptr;
}

void CFactory::DependentDied(CObject* o)
{
	if (o == curBuild) {
		curBuild = nullptr;
		StopBuild();
	}

	CUnit::DependentDied(o);
}



void CFactory::SendToEmptySpot(CUnit* unit)
{
	constexpr int numSteps = 100;

	const float searchRadius = radius * 4.0f + unit->radius * 4.0f;
	const float searchAngle = math::PI / (numSteps * 0.5f);

	const float3 exitPos = pos + frontdir * (radius + unit->radius);
	const float3 tempPos = pos + frontdir * searchRadius;

	float3 foundPos = tempPos;

	for (int i = 0; i < numSteps; ++i) {
		const float a = searchRadius * math::cos(i * searchAngle);
		const float b = searchRadius * math::sin(i * searchAngle);

		float3 testPos = pos + frontdir * a + rightdir * b;

		if (!testPos.IsInBounds())
			continue;
		// don't pick spots behind the factory, because
		// units will want to path through it when open
		// (which slows down production)
		if ((testPos - pos).dot(frontdir) < 0.0f)
			continue;

		testPos.y = CGround::GetHeightAboveWater(testPos.x, testPos.z);

		if (!quadField.NoSolidsExact(testPos, unit->radius * 1.5f, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;
		if (unit->moveDef != nullptr && !unit->moveDef->TestMoveSquare(nullptr, testPos, ZeroVector, true, true))
			continue;

		foundPos = testPos;
		break;
	}

	if (foundPos == tempPos) {
		// no empty spot found, pick one randomly so units do not pile up even more
		// also make sure not to loop forever if we happen to be facing a map border
		foundPos.y = 0.0f;

		for (int i = 0; i < numSteps; ++i) {
			const float x = gsRNG.NextFloat() * numSteps;
			const float a = searchRadius * math::cos(x * searchAngle);
			const float b = searchRadius * math::sin(x * searchAngle);

			foundPos.x = pos.x + frontdir.x * a + rightdir.x * b;
			foundPos.z = pos.z + frontdir.z * a + rightdir.z * b;

			if (!foundPos.IsInBounds())
				continue;
			if ((foundPos - pos).dot(frontdir) < 0.0f)
				continue;

			if (unit->moveDef != nullptr && !unit->moveDef->TestMoveSquare(nullptr, foundPos, ZeroVector, true, true))
				continue;

			break;
		}

		foundPos.y = CGround::GetHeightAboveWater(foundPos.x, foundPos.z);
	}

	// first queue a temporary waypoint outside the factory
	// (otherwise units will try to turn before exiting when
	// foundPos lies behind exit and cause jams / get stuck)
	// we assume this temporary point is not itself blocked,
	// unlike the second for which we do call TestMoveSquare
	//
	// NOTE:
	//   MobileCAI::AutoGenerateTarget inserts a _third_
	//   command when |foundPos - tempPos| >= 100 elmos,
	//   because MobileCAI::FinishCommand only updates
	//   lastUserGoal for non-internal orders --> the
	//   final order given here should not be internal
	//   (and should also be more than CMD_CANCEL_DIST
	//   elmos distant from foundPos)
	//
	if (!unit->unitDef->canfly && exitPos.IsInBounds())
		unit->commandAI->GiveCommand(Command(CMD_MOVE, SHIFT_KEY, exitPos));

	// second actual empty-spot waypoint
	unit->commandAI->GiveCommand(Command(CMD_MOVE, SHIFT_KEY, foundPos));
}

void CFactory::AssignBuildeeOrders(CUnit* unit) {
	CCommandAI* unitCAI = unit->commandAI;
	CCommandQueue& unitCmdQue = unitCAI->commandQue;

	const CFactoryCAI* factoryCAI = static_cast<CFactoryCAI*>(commandAI);
	const CCommandQueue& factoryCmdQue = factoryCAI->newUnitCommands;

	if (factoryCmdQue.empty() && unitCmdQue.empty()) {
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
		const float3 fpSize = {unitDef->xsize * SQUARE_SIZE * 0.5f, 0.0f, unitDef->zsize * SQUARE_SIZE * 0.5f};
		const float3 fpMins = {unit->pos.x - fpSize.x, 0.0f, unit->pos.z - fpSize.z};
		const float3 fpMaxs = {unit->pos.x + fpSize.x, 0.0f, unit->pos.z + fpSize.z};

		float3 tmpVec;
		float3 tmpPos;

		for (int i = 0, k = 2 * (math::fabs(frontdir.z) > math::fabs(frontdir.x)); i < 128; i++) {
			tmpVec = frontdir * radius * (2.0f + i * 0.5f);
			tmpPos = unit->pos + tmpVec;

			if ((tmpPos[k] < fpMins[k]) || (tmpPos[k] > fpMaxs[k]))
				break;
		}

		c.PushPos(tmpPos.cClampInBounds());
	} else {
		// dummy rallypoint for aircraft
		c.PushPos(unit->pos);
	}

	if (unitCmdQue.empty()) {
		unitCAI->GiveCommand(c);

		// copy factory orders for new unit
		for (auto ci = factoryCmdQue.begin(); ci != factoryCmdQue.end(); ++ci) {
			Command c = *ci;
			c.SetOpts(c.GetOpts() | SHIFT_KEY);

			if (c.GetID() == CMD_MOVE) {
				float xjit = gsRNG.NextFloat() * math::TWOPI;
				float zjit = gsRNG.NextFloat() * math::TWOPI;

				const float3 p1 = c.GetPos(0);
				const float3 p2 = float3(p1.x + xjit, p1.y, p1.z + zjit);

				// apply a small amount of random jitter to move commands
				// such that new units do not all share the same goal-pos
				// and start forming a "trail" back to the factory exit
				c.SetPos(0, p2);
			}

			unitCAI->GiveCommand(c);
		}
	} else {
		unitCmdQue.push_front(c);
	}
}



bool CFactory::ChangeTeam(int newTeam, ChangeType type)
{
	if (!CBuilding::ChangeTeam(newTeam, type))
		return false;

	if (curBuild)
		curBuild->ChangeTeam(newTeam, type);

	return true;
}


void CFactory::CreateNanoParticle(bool highPriority)
{
	const int modelNanoPiece = nanoPieceCache.GetNanoPiece(script);

	if (!localModel.Initialized() || !localModel.HasPiece(modelNanoPiece))
		return;

	const float3 relNanoFirePos = localModel.GetRawPiecePos(modelNanoPiece);
	const float3 nanoPos = this->GetObjectSpacePos(relNanoFirePos);

	// unsynced
	projectileHandler.AddNanoParticle(nanoPos, curBuild->midPos, unitDef, team, highPriority);
}
