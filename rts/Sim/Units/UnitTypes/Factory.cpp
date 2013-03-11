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
	CR_MEMBER(curBuildDef),
	CR_MEMBER(curBuild),
	//CR_MEMBER(finishedBuildFunc), FIXME is pointer
	CR_MEMBER(finishedBuildCommand),
	CR_MEMBER(nanoPieceCache),
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

void CFactory::PreInit(const UnitLoadParams& params)
{
	unitDef = params.unitDef;
	buildSpeed = unitDef->buildSpeed / TEAM_SLOWUPDATE_RATE;

	CBuilding::PreInit(params);
}



float3 CFactory::CalcBuildPos(int buildPiece)
{
	const float3 relBuildPos = script->GetPiecePos((buildPiece < 0)? script->QueryBuildInfo() : buildPiece);
	const float3 absBuildPos = pos + frontdir * relBuildPos.z + updir * relBuildPos.y + rightdir * relBuildPos.x;
	return absBuildPos;
}



void CFactory::Update()
{
	nanoPieceCache.Update();

	if (beingBuilt) {
		// factory is under construction, cannot build anything yet
		CUnit::Update();

		#if 1
		// this can happen if we started being reclaimed *while* building a
		// unit, in which case our buildee can either be allowed to finish
		// construction (by assisting builders) or has to be killed --> the
		// latter is easier
		if (curBuild != NULL) {
			StopBuild();
		}
		#endif

		return;
	}


	if (curBuildDef != NULL) {
		if (!opening && !IsStunned()) {
			if (groundBlockingObjectMap->CanOpenYard(this)) {
				script->Activate();
				groundBlockingObjectMap->OpenBlockingYard(this);
				opening = true;

				// make sure the idle-check does not immediately trigger
				// (scripts have 7 seconds to set inBuildStance to true)
				lastBuildUpdateFrame = gs->frameNum;
			}
		}

		if (opening && inBuildStance && !IsStunned()) {
			StartBuild(curBuildDef);
		}
	}

	if (curBuild != NULL) {
		UpdateBuild(curBuild);
		FinishBuild(curBuild);
	}

	const bool wantClose = (!IsStunned() && opening && (gs->frameNum >= (lastBuildUpdateFrame + GAME_SPEED * 7)));
	const bool closeYard = (wantClose && curBuild == NULL && groundBlockingObjectMap->CanCloseYard(this));

	if (closeYard) {
		// close the factory after inactivity
		groundBlockingObjectMap->CloseBlockingYard(this);
		opening = false;
		script->Deactivate();
	}

	CBuilding::Update();
}

/*
void CFactory::SlowUpdate(void)
{
	// this (ancient) code was intended to keep vicinity around factories clear
	// so units could exit more easily among crowds of assisting builders, etc.
	// it is unneeded now that units can flow / push through a non-moving crowd
	// (so we no longer have to override CBuilding::SlowUpdate either)
	if (transporter == NULL) {
		CGameHelper::BuggerOff(pos - float3(0.01f, 0, 0.02f), radius, true, true, team, NULL);
	}

	CBuilding::SlowUpdate();
}
*/



void CFactory::StartBuild(const UnitDef* buildeeDef) {
	const float3& buildPos = CalcBuildPos();
	const bool blocked = groundBlockingObjectMap->GroundBlocked(buildPos, this);

	// wait until buildPos is no longer blocked (eg. by a previous buildee)
	//
	// it might rarely be the case that a unit got stuck inside the factory
	// or died right after completion and left some wreckage, but that is up
	// to players to fix (we no longer broadcast BuggerOff directives, since
	// those are indiscriminate and ineffective)
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
	curBuildDef = NULL;

	#if (PLAY_SOUNDS == 1)
	if (losStatus[gu->myAllyTeam] & LOS_INLOS) {
		Channels::General.PlayRandomSample(unitDef->sounds.build, buildPos);
	}
	#endif
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

	float3 buildeePos = buildPos;

	// rotate unit nanoframe with platform
	buildee->heading = (-buildPieceHeading + GetHeadingFromFacing(buildFacing)) & (SPRING_CIRCLE_DIVS - 1);

	if (buildee->unitDef->floatOnWater && (buildeePos.y <= 0.0f))
		buildeePos.y = -buildee->unitDef->waterline;

	buildee->Move3D(buildeePos, false);
	buildee->UpdateDirVectors(false);
	buildee->UpdateMidAndAimPos();

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
			buildee->SetGroup(group, true);
		}
	}

	const CCommandAI* bcai = buildee->commandAI;
	const bool assignOrders = bcai->commandQue.empty() || (dynamic_cast<const CMobileCAI*>(bcai) != NULL);

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

	if (finishedBuildFunc != NULL)
		return FACTORY_KEEP_BUILD_ORDER;
	if (curBuild != NULL)
		return FACTORY_KEEP_BUILD_ORDER;
	if (unitHandler->unitsByDefs[team][buildeeDef->id].size() >= buildeeDef->maxThisUnit)
		return FACTORY_SKIP_BUILD_ORDER;
	if (teamHandler->Team(team)->AtUnitLimit())
		return FACTORY_KEEP_BUILD_ORDER;
	if (luaRules && !luaRules->AllowUnitCreation(buildeeDef, this, NULL))
		return FACTORY_SKIP_BUILD_ORDER;

	finishedBuildFunc = buildFunc;
	finishedBuildCommand = buildCmd;
	curBuildDef = buildeeDef;
	nextBuildUnitDefID = buildeeDef->id;

	// signal that the build-order was accepted (queued)
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
	const float searchRadius = radius * 4.0f + unit->radius * 4.0f;
	const int numSteps = 36;

	float3 foundPos = pos + frontdir * searchRadius;
	float3 tempPos = foundPos;

	for (int x = 0; x < numSteps; ++x) {
		const float a = searchRadius * math::cos(x * PI / (numSteps * 0.5f));
		const float b = searchRadius * math::sin(x * PI / (numSteps * 0.5f));

		float3 testPos = pos + frontdir * a + rightdir * b;

		if (!testPos.IsInMap())
			continue;
		// don't pick spots behind the factory (because
		// units will want to path through it when open
		// which slows down production)
		if ((testPos - pos).dot(frontdir) < 0.0f)
			continue;

		testPos.y = ground->GetHeightAboveWater(testPos.x, testPos.z);

		if (quadField->GetSolidsExact(testPos, unit->radius * 1.5f).empty()) {
			foundPos = testPos; break;
		}
	}

	if (foundPos == tempPos) {
		// no empty spot found, pick one randomly so units do not pile up even more
		// also make sure not to loop forever if we happen to be facing a map border
		foundPos.y = 0.0f;

		do {
			const float x = ((gs->randInt() * 1.0f) / RANDINT_MAX) * numSteps;
			const float a = searchRadius * math::cos(x * PI / (numSteps * 0.5f));
			const float b = searchRadius * math::sin(x * PI / (numSteps * 0.5f));

			foundPos.x = pos.x + frontdir.x * a + rightdir.x * b;
			foundPos.z = pos.z + frontdir.z * a + rightdir.z * b;
			foundPos.y += 1.0f;
		} while ((!foundPos.IsInMap() || (foundPos - pos).dot(frontdir) < 0.0f) && (foundPos.y < 100.0f));

		foundPos.y = ground->GetHeightAboveWater(foundPos.x, foundPos.z);
	}

	// first queue a temporary waypoint outside the factory
	// (otherwise units will try to turn before exiting when
	// foundPos lies behind exit and cause jams / get stuck)
	// we assume this temporary point is not itself blocked
	// (redundant now foundPos always lies in front of us)
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
	//   Command c0(CMD_MOVE, tempPos);
	Command c1(CMD_MOVE, SHIFT_KEY | INTERNAL_ORDER, foundPos);
	Command c2(CMD_MOVE, SHIFT_KEY,                  foundPos + frontdir * 20.0f);
	// unit->commandAI->GiveCommand(c0);
	unit->commandAI->GiveCommand(c1);
	unit->commandAI->GiveCommand(c2);
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

		c.PushPos(tmpPos);
	} else {
		// dummy rallypoint for aircraft
		c.PushPos(unit->pos);
	}

	if (unitCmdQue.empty()) {
		unitCAI->GiveCommand(c);

		// copy factory orders for new unit
		for (CCommandQueue::const_iterator ci = factoryCmdQue.begin(); ci != factoryCmdQue.end(); ++ci) {
			c = *ci;
			c.options |= SHIFT_KEY;

			if (c.GetID() == CMD_MOVE) {
				const float3 p1 = c.GetPos(0);
				const float3 p2 = float3(p1.x + gs->randFloat() * TWOPI, p1.y, p1.z + gs->randFloat() * TWOPI);
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
	StopBuild();
	return (CBuilding::ChangeTeam(newTeam, type));
}


void CFactory::CreateNanoParticle(bool highPriority)
{
	const int modelNanoPiece = nanoPieceCache.GetNanoPiece(script);

#ifdef USE_GML
	if (GML::Enabled() && ((gs->frameNum - lastDrawFrame) > 20))
		return;
#endif

	if (localModel == NULL || !localModel->HasPiece(modelNanoPiece))
		return;

	const float3 relNanoFirePos = localModel->GetRawPiecePos(modelNanoPiece);

	const float3 nanoPos = pos
		+ (frontdir * relNanoFirePos.z)
		+ (updir    * relNanoFirePos.y)
		+ (rightdir * relNanoFirePos.x);

	// unsynced
	projectileHandler->AddNanoParticle(nanoPos, curBuild->midPos, unitDef, team, highPriority);
}
