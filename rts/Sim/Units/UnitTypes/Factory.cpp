#include "StdAfx.h"
// Factory.cpp: implementation of the CFactory class.
//
//////////////////////////////////////////////////////////////////////

#include "Factory.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/WaitCommandsAI.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/GfxProjectile.h"
#include "Sim/Units/COB/UnitScript.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sync/SyncTracer.h"
#include "GlobalUnsynced.h"
#include "EventHandler.h"
#include "Sound/AudioChannel.h"
#include "LogOutput.h"
#include "Matrix44f.h"
#include "myMath.h"
#include "mmgr.h"

CR_BIND_DERIVED(CFactory, CBuilding, );

CR_REG_METADATA(CFactory, (
				CR_MEMBER(buildSpeed),
				CR_MEMBER(quedBuild),
				//CR_MEMBER(nextBuild),
				CR_MEMBER(nextBuildName),
				CR_MEMBER(curBuild),
				CR_MEMBER(opening),
				CR_MEMBER(lastBuild),
				CR_RESERVED(16),
				CR_POSTLOAD(PostLoad)
				));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFactory::CFactory():
	buildSpeed(100),
	quedBuild(false),
	nextBuild(0),
	curBuild(0),
	opening(false),
	lastBuild(-1000)
{
}


CFactory::~CFactory()
{
	// if uh == NULL then all pointers to units should be considered dangling pointers
	if (uh && curBuild) {
		curBuild->KillUnit(false, true, NULL);
		curBuild = NULL;
	}
}


void CFactory::PostLoad()
{
	nextBuild = unitDefHandler->GetUnitByName(nextBuildName);
	if(opening){
		script->Activate();
	}
	if (curBuild) {
		script->StartBuilding();
	}
}

void CFactory::UnitInit (const UnitDef* def, int team, const float3& position)
{
	buildSpeed = def->buildSpeed / 32.0f;
	CBuilding::UnitInit(def, team, position);
}

int CFactory::GetBuildPiece()
{
	return script->QueryBuildInfo();
}

// GetBuildPiece() is called if piece < 0
float3 CFactory::CalcBuildPos(int buildPiece)
{
	float3 relBuildPos = script->GetPiecePos(buildPiece < 0 ? GetBuildPiece() : buildPiece);
	float3 buildPos = pos + frontdir * relBuildPos.z + updir * relBuildPos.y + rightdir * relBuildPos.x;
	return buildPos;
}

void CFactory::Update()
{
	if (beingBuilt) {
		// factory under construction
		CUnit::Update();
		return;
	}

	if (quedBuild && !opening && !stunned) {
		script->Activate();
		groundBlockingObjectMap->OpenBlockingYard(this, yardMap);
		opening = true;
	}

	if (quedBuild && inBuildStance && !stunned) {
		// start building a unit
		const float3        buildPos = CalcBuildPos();
		const CSolidObject* solidObj = groundBlockingObjectMap->GroundBlocked(buildPos);

		if (solidObj == NULL || (dynamic_cast<const CUnit*>(solidObj) == this)) {
			quedBuild = false;
			CUnit* b = unitLoader.LoadUnit(nextBuild, buildPos + float3(0.01f, 0.01f, 0.01f), team,
											true, buildFacing, this);
			b->lineage = this->lineage;

			if (!unitDef->canBeAssisted) {
				b->soloBuilder = this;
				b->AddDeathDependence(this);
			}

			AddDeathDependence(b);
			curBuild = b;

			script->StartBuilding();

			int soundIdx = unitDef->sounds.build.getRandomIdx();
			if (soundIdx >= 0) {
				Channels::UnitReply.PlaySample(
					unitDef->sounds.build.getID(soundIdx), pos,
					unitDef->sounds.build.getVolume(0));
			}
		} else {
			helper->BuggerOff(buildPos - float3(0.01f, 0, 0.02f), radius + 8);
		}
	}


	if (curBuild && !beingBuilt) {
		if (!stunned) {
			// factory not under construction and
			// nanolathing unit: continue building
			lastBuild = gs->frameNum;

			// buildPiece is the rotating platform
			const int buildPiece = GetBuildPiece();
			const CMatrix44f& mat = script->GetPieceMatrix(buildPiece);
			const int h = GetHeadingFromVector(mat[2], mat[10]);

			// rotate unit nanoframe with platform
			curBuild->heading = (h + GetHeadingFromFacing(buildFacing)) & 65535;

			const float3 buildPos = CalcBuildPos(buildPiece);
			curBuild->pos = buildPos;

			if (curBuild->floatOnWater) {
				float waterline = ground->GetHeight(buildPos.x, buildPos.z) - curBuild->unitDef->waterline;
				if (waterline > curBuild->pos.y)
					curBuild->pos.y = waterline;
			}
			curBuild->midPos = curBuild->pos + (UpVector * curBuild->relMidPos.y);

			const CCommandQueue& queue = commandAI->commandQue;

			if(!queue.empty() && (queue.front().id == CMD_WAIT)) {
				curBuild->AddBuildPower(0, this);
			} else {
				if (curBuild->AddBuildPower(buildSpeed, this)) {
					CreateNanoParticle();
				}
			}
		}

		if (!curBuild->beingBuilt &&
				(!unitDef->fullHealthFactory ||
						(curBuild->health >= curBuild->maxHealth)))
		{
			if (group && curBuild->group == 0) {
				curBuild->SetGroup(group);
			}

			bool userOrders = true;
			if (curBuild->commandAI->commandQue.empty() ||
					(dynamic_cast<CMobileCAI*>(curBuild->commandAI) &&
					 ((CMobileCAI*)curBuild->commandAI)->unimportantMove)) {
				userOrders = false;
				const CFactoryCAI* facAI = (CFactoryCAI*) commandAI;
				const CCommandQueue& newUnitCmds = facAI->newUnitCommands;

				if (newUnitCmds.empty()) {
					SendToEmptySpot(curBuild);
				} else {
					// XXX the pathfinder sometimes... makes mistakes, try to hack around it
					// XXX note this qualifies as HACK HACK HACK
					float3 testpos = curBuild->pos + frontdir * (this->radius - 1.0f);
					Command c;
					c.id = CMD_MOVE;
					c.params.push_back(testpos.x);
					c.params.push_back(testpos.y);
					c.params.push_back(testpos.z);
					curBuild->commandAI->GiveCommand(c);

					for (CCommandQueue::const_iterator ci = newUnitCmds.begin(); ci != newUnitCmds.end(); ++ci) {
						c = *ci;
						c.options |= SHIFT_KEY;
						curBuild->commandAI->GiveCommand(c);
					}
				}
				waitCommandsAI.AddLocalUnit(curBuild, this);
			}
			eventHandler.UnitFromFactory(curBuild, this, userOrders);

			StopBuild();
		}
	}

	if (((lastBuild + 200) < gs->frameNum) && !stunned &&
	    !quedBuild && opening && groundBlockingObjectMap->CanCloseYard(this)) {
		// close the factory after inactivity
		groundBlockingObjectMap->CloseBlockingYard(this, yardMap);
		opening = false;
		script->Deactivate();
	}

	CBuilding::Update();
}

void CFactory::StartBuild(const UnitDef* ud)
{
	if (beingBuilt)
		return;

#ifdef TRACE_SYNC
	tracefile << "Start build: ";
	tracefile << type.c_str() << "\n";
#endif

	if (curBuild)
		StopBuild();

	quedBuild = true;
	nextBuild = ud;
	nextBuildName = ud->name;

	if (!opening && !stunned) {
		script->Activate();
		groundBlockingObjectMap->OpenBlockingYard(this, yardMap);
		opening = true;
	}
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
		DeleteDeathDependence(curBuild);
	}
	curBuild = 0;
	quedBuild = false;
}

void CFactory::DependentDied(CObject* o)
{
	if (o == curBuild) {
		curBuild = 0;
		StopBuild();
	}
	CUnit::DependentDied(o);
}

void CFactory::FinishedBuilding(void)
{
	CBuilding::FinishedBuilding();
}

void CFactory::SendToEmptySpot(CUnit* unit)
{
	float r = radius * 1.7f + unit->radius * 4;
	float3 foundPos = pos + frontdir * r;

	for (int a = 0; a < 20; ++a) {
		float3 testPos = pos + frontdir * r * cos(a * PI / 10) + rightdir * r * sin(a * PI / 10);
		testPos.y = ground->GetHeight(testPos.x, testPos.z);

		if (qf->GetSolidsExact(testPos, unit->radius * 1.5f).empty()) {
			foundPos = testPos;
			break;
		}
	}

	Command c;
	c.id = CMD_MOVE;
	c.options = 0;
	c.params.push_back(foundPos.x);
	c.params.push_back(foundPos.y);
	c.params.push_back(foundPos.z);
	unit->commandAI->GiveCommand(c);
}

void CFactory::SlowUpdate(void)
{
	helper->BuggerOff(pos - float3(0.01f, 0, 0.02f), radius);
	CBuilding::SlowUpdate();
}

bool CFactory::ChangeTeam(int newTeam, ChangeType type)
{
	StopBuild();
	return CBuilding::ChangeTeam(newTeam, type);
}


void CFactory::CreateNanoParticle(void)
{
	const int piece = script->QueryNanoPiece();

#ifdef USE_GML
	if (gs->frameNum - lastDrawFrame > 20)
		return;
#endif

	if (ph->currentNanoParticles < ph->maxNanoParticles && unitDef->showNanoSpray) {
		const float3 relWeaponFirePos = script->GetPiecePos(piece);
		const float3 weaponPos = pos + (frontdir * relWeaponFirePos.z)
			+ (updir    * relWeaponFirePos.y)
			+ (rightdir * relWeaponFirePos.x);
		float3 dif = (curBuild->midPos - weaponPos);
		const float l = fastmath::sqrt2(dif.SqLength());
		dif /= l;
		dif += gu->usRandVector() * 0.15f;
		float3 color = unitDef->nanoColor;

		if (gu->teamNanospray) {
			unsigned char* tcol = teamHandler->Team(team)->color;
			color = float3(tcol[0] * (1.0f / 255.0f), tcol[1] * (1.0f / 255.0f), tcol[2] * (1.0f / 255.0f));
		}
		new CGfxProjectile(weaponPos, dif, (int) l, color);
	}
}
