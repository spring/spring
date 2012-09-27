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

	if (curBuild != NULL && !beingBuilt) {
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
		helper->BuggerOff(pos - float3(0.01f, 0, 0.02f), radius, true, true, team, NULL);
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
}

void CFactory::UpdateBuild(CUnit* buildee) {
	if (IsStunned())
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
	if (uh->unitsByDefs[team][buildeeDef->id].size() >= buildeeDef->maxThisUnit)
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
	const float searchRadius = radius * 1.7f + unit->radius * 4.0f;

	float3 foundPos = pos + frontdir * searchRadius;
	float3 tempPos = pos + frontdir * radius * 2.0f;

	for (int x = 0; x < 20; ++x) {
		const float a = searchRadius * math::cos(x * PI / 10);
		const float b = searchRadius * math::sin(x * PI / 10);
		float3 testPos = pos + frontdir * a + rightdir * b;
		testPos.y = ground->GetHeightAboveWater(testPos.x, testPos.z);

		if (qf->GetSolidsExact(testPos, unit->radius * 1.5f).empty()) {
			foundPos = testPos; break;
		}
	}

	// first queue a temporary waypoint outside the factory
	// (otherwise units will try to turn before exiting when
	// foundPos lies behind it and cause jams / get stuck)
	// assume this temporary point is not itself blocked
	//
	// make sure CAI does not cancel if foundPos == tempPos
	Command c1(CMD_MOVE,                             tempPos);
	Command c2(CMD_MOVE, SHIFT_KEY | INTERNAL_ORDER, foundPos);
	unit->commandAI->GiveCommand(c1);
	unit->commandAI->GiveCommand(c2);
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

		c.PushPos(tmpPos);
		unit->commandAI->GiveCommand(c);
	}

	for (CCommandQueue::const_iterator ci = newUnitCmds.begin(); ci != newUnitCmds.end(); ++ci) {
		c = *ci;
		c.options |= SHIFT_KEY;
		unit->commandAI->GiveCommand(c);
	}
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
	if (GML::Enabled() && ((gs->frameNum - lastDrawFrame) > 20))
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
