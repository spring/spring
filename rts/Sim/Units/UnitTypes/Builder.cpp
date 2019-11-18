/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include <algorithm>
#include "Builder.h"
#include "Building.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "System/SpringMath.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Sound/ISoundChannels.h"

using std::min;
using std::max;

CR_BIND_DERIVED(CBuilder, CUnit, )
CR_REG_METADATA(CBuilder, (
	CR_MEMBER(range3D),
	CR_MEMBER(buildDistance),
	CR_MEMBER(buildSpeed),
	CR_MEMBER(repairSpeed),
	CR_MEMBER(reclaimSpeed),
	CR_MEMBER(resurrectSpeed),
	CR_MEMBER(captureSpeed),
	CR_MEMBER(terraformSpeed),
	CR_MEMBER(curResurrect),
	CR_MEMBER(lastResurrected),
	CR_MEMBER(curBuild),
	CR_MEMBER(curCapture),
	CR_MEMBER(curReclaim),
	CR_MEMBER(reclaimingUnit),
	CR_MEMBER(helpTerraform),
	CR_MEMBER(terraforming),
	CR_MEMBER(myTerraformLeft),
	CR_MEMBER(terraformHelp),
	CR_MEMBER(tx1), CR_MEMBER(tx2), CR_MEMBER(tz1), CR_MEMBER(tz2),
	CR_MEMBER(terraformCenter),
	CR_MEMBER(terraformRadius),
	CR_MEMBER(terraformType),
	CR_MEMBER(nanoPieceCache)
))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuilder::CBuilder():
	CUnit(),
	range3D(true),
	buildDistance(16),
	buildSpeed(100),
	repairSpeed(100),
	reclaimSpeed(100),
	resurrectSpeed(100),
	captureSpeed(100),
	terraformSpeed(100),
	curResurrect(0),
	lastResurrected(0),
	curBuild(0),
	curCapture(0),
	curReclaim(0),
	reclaimingUnit(false),
	helpTerraform(0),
	terraforming(false),
	terraformHelp(0),
	myTerraformLeft(0),
	terraformType(Terraform_Building),
	tx1(0),
	tx2(0),
	tz1(0),
	tz2(0),
	terraformCenter(ZeroVector),
	terraformRadius(0)
{
}



void CBuilder::PreInit(const UnitLoadParams& params)
{
	unitDef = params.unitDef;
	range3D = unitDef->buildRange3D;
	buildDistance = (params.unitDef)->buildDistance;

	const float scale = (1.0f / TEAM_SLOWUPDATE_RATE);

	buildSpeed     = scale * unitDef->buildSpeed;
	repairSpeed    = scale * unitDef->repairSpeed;
	reclaimSpeed   = scale * unitDef->reclaimSpeed;
	resurrectSpeed = scale * unitDef->resurrectSpeed;
	captureSpeed   = scale * unitDef->captureSpeed;
	terraformSpeed = scale * unitDef->terraformSpeed;

	CUnit::PreInit(params);
}


bool CBuilder::CanAssistUnit(const CUnit* u, const UnitDef* def) const
{
	if (!unitDef->canAssist)
		return false;

	return ((def == nullptr || u->unitDef == def) && u->beingBuilt && (u->buildProgress < 1.0f) && (u->soloBuilder == nullptr || u->soloBuilder == this));
}


bool CBuilder::CanRepairUnit(const CUnit* u) const
{
	if (!unitDef->canRepair)
		return false;
	if (u->beingBuilt)
		return false;
	if (u->health >= u->maxHealth)
		return false;

	return (u->unitDef->repairable);
}



bool CBuilder::UpdateTerraform(const Command&)
{
	CUnit* curBuildee = curBuild;

	if (!terraforming || !inBuildStance)
		return false;

	const float* heightmap = readMap->GetCornerHeightMapSynced();
	float terraformScale = 0.1f;

	assert(!mapDamage->Disabled());

	switch (terraformType) {
		case Terraform_Building: {
			if (curBuildee != nullptr) {
				if (curBuildee->terraformLeft <= 0.0f)
					terraformScale = 0.0f;
				else
					terraformScale = (terraformSpeed + terraformHelp) / curBuildee->terraformLeft;

				curBuildee->terraformLeft -= (terraformSpeed + terraformHelp);

				terraformHelp = 0.0f;
				terraformScale = std::min(terraformScale, 1.0f);

				// prevent building from timing out while terraforming for it
				curBuildee->AddBuildPower(this, 0.0f);

				for (int z = tz1; z <= tz2; z++) {
					for (int x = tx1; x <= tx2; x++) {
						const int idx = z * mapDims.mapxp1 + x;

						readMap->AddHeight(idx, (curBuildee->pos.y - heightmap[idx]) * terraformScale);
					}
				}

				if (curBuildee->terraformLeft <= 0.0f) {
					terraforming = false;

					mapDamage->RecalcArea(tx1, tx2, tz1, tz2);
					curBuildee->groundLevelled = true;

					if (eventHandler.TerraformComplete(this, curBuildee)) {
						StopBuild();
					}
				}
			}
		} break;
		case Terraform_Restore: {
			if (myTerraformLeft <= 0.0f)
				terraformScale = 0.0f;
			else
				terraformScale = (terraformSpeed + terraformHelp) / myTerraformLeft;

			myTerraformLeft -= (terraformSpeed + terraformHelp);

			terraformHelp = 0.0f;
			terraformScale = std::min(terraformScale, 1.0f);

			for (int z = tz1; z <= tz2; z++) {
				for (int x = tx1; x <= tx2; x++) {
					int idx = z * mapDims.mapxp1 + x;
					float ch = heightmap[idx];
					float oh = readMap->GetOriginalHeightMapSynced()[idx];

					readMap->AddHeight(idx, (oh - ch) * terraformScale);
				}
			}

			if (myTerraformLeft <= 0.0f) {
				terraforming = false;

				mapDamage->RecalcArea(tx1, tx2, tz1, tz2);
				StopBuild();
			}
		} break;
	}

	ScriptDecloak(curBuildee, nullptr);
	CreateNanoParticle(terraformCenter, terraformRadius * 0.5f, false);

	// smooth the x-borders
	for (int z = tz1; z <= tz2; z++) {
		for (int x = 1; x <= 3; x++) {
			if (tx1 - 3 >= 0) {
				const float ch3 = heightmap[z * mapDims.mapxp1 + tx1    ];
				const float ch  = heightmap[z * mapDims.mapxp1 + tx1 - x];
				const float ch2 = heightmap[z * mapDims.mapxp1 + tx1 - 3];
				const float amount = ((ch3 * (3 - x) + ch2 * x) / 3 - ch) * terraformScale;

				readMap->AddHeight(z * mapDims.mapxp1 + tx1 - x, amount);
			}
			if (tx2 + 3 < mapDims.mapx) {
				const float ch3 = heightmap[z * mapDims.mapxp1 + tx2    ];
				const float ch  = heightmap[z * mapDims.mapxp1 + tx2 + x];
				const float ch2 = heightmap[z * mapDims.mapxp1 + tx2 + 3];
				const float amount = ((ch3 * (3 - x) + ch2 * x) / 3 - ch) * terraformScale;

				readMap->AddHeight(z * mapDims.mapxp1 + tx2 + x, amount);
			}
		}
	}

	// smooth the z-borders
	for (int z = 1; z <= 3; z++) {
		for (int x = tx1; x <= tx2; x++) {
			if ((tz1 - 3) >= 0) {
				const float ch3 = heightmap[(tz1    ) * mapDims.mapxp1 + x];
				const float ch  = heightmap[(tz1 - z) * mapDims.mapxp1 + x];
				const float ch2 = heightmap[(tz1 - 3) * mapDims.mapxp1 + x];
				const float adjust = ((ch3 * (3 - z) + ch2 * z) / 3 - ch) * terraformScale;

				readMap->AddHeight((tz1 - z) * mapDims.mapxp1 + x, adjust);
			}
			if ((tz2 + 3) < mapDims.mapy) {
				const float ch3 = heightmap[(tz2    ) * mapDims.mapxp1 + x];
				const float ch  = heightmap[(tz2 + z) * mapDims.mapxp1 + x];
				const float ch2 = heightmap[(tz2 + 3) * mapDims.mapxp1 + x];
				const float adjust = ((ch3 * (3 - z) + ch2 * z) / 3 - ch) * terraformScale;

				readMap->AddHeight((tz2 + z) * mapDims.mapxp1 + x, adjust);
			}
		}
	}

	return true;
}

bool CBuilder::AssistTerraform(const Command&)
{
	CBuilder* helpTerraformee = helpTerraform;

	if (helpTerraformee == nullptr || !inBuildStance)
		return false;

	if (!helpTerraformee->terraforming) {
		// delete our helpTerraform dependence
		StopBuild(true);
		return true;
	}

	ScriptDecloak(helpTerraformee, nullptr);

	helpTerraformee->terraformHelp += terraformSpeed;
	CreateNanoParticle(helpTerraformee->terraformCenter, helpTerraformee->terraformRadius * 0.5f, false);
	return true;
}

bool CBuilder::UpdateBuild(const Command& fCommand)
{
	CUnit* curBuildee = curBuild;
	CBuilderCAI* cai = static_cast<CBuilderCAI*>(commandAI);

	if (curBuildee == nullptr || !cai->IsInBuildRange(curBuildee))
		return false;

	if (fCommand.GetID() == CMD_WAIT) {
		if (curBuildee->buildProgress < 1.0f) {
			// prevent buildee from decaying (we cannot call StopBuild here)
			curBuildee->AddBuildPower(this, 0.0f);
		} else {
			// stop repairing (FIXME: should be much cleaner to let BuilderCAI
			// call this instead when a wait command is given?)
			StopBuild();
		}

		return true;
	}

	if (curBuildee->soloBuilder != nullptr && (curBuildee->soloBuilder != this)) {
		StopBuild();
		return true;
	}

	// NOTE:
	//   technically this block of code should be guarded by
	//   "if (inBuildStance)", but doing so can create zombie
	//   guarders because scripts might not set inBuildStance
	//   to true when guard or repair orders are executed and
	//   SetRepairTarget does not check for it
	//
	//   StartBuild *does* ensure construction will not start
	//   until inBuildStance is set to true by the builder's
	//   script, and there are no cases during construction
	//   when inBuildStance can become false yet the buildee
	//   should be kept from decaying, so this is free from
	//   serious side-effects (when repairing, a builder might
	//   start adding build-power before having fully finished
	//   its opening animation)
	if (!(inBuildStance || true))
		return true;

	ScriptDecloak(curBuildee, nullptr);

	// adjusted build-speed: use repair-speed on units with
	// progress >= 1 rather than raw build-speed on buildees
	// with progress < 1
	float adjBuildSpeed = buildSpeed;

	if (curBuildee->buildProgress >= 1.0f)
		adjBuildSpeed = std::min(repairSpeed, unitDef->maxRepairSpeed * 0.5f - curBuildee->repairAmount); // repair

	if (adjBuildSpeed > 0.0f && curBuildee->AddBuildPower(this, adjBuildSpeed)) {
		CreateNanoParticle(curBuildee->midPos, curBuildee->radius * 0.5f, false);
		return true;
	}

	// check if buildee finished construction
	if (curBuildee->beingBuilt || curBuildee->health < curBuildee->maxHealth)
		return true;

	StopBuild();
	return true;
}

bool CBuilder::UpdateReclaim(const Command& fCommand)
{
	// AddBuildPower can invoke StopBuild indirectly even if returns true
	// and reset curReclaim to null (which would crash CreateNanoParticle)
	CSolidObject* curReclaimee = curReclaim;

	if (curReclaimee == nullptr || f3SqDist(curReclaimee->pos, pos) >= Square(buildDistance + curReclaimee->radius) || !inBuildStance)
		return false;

	if (fCommand.GetID() == CMD_WAIT) {
		StopBuild();
		return true;
	}

	ScriptDecloak(curReclaimee, nullptr);

	if (!curReclaimee->AddBuildPower(this, -reclaimSpeed))
		return true;

	CreateNanoParticle(curReclaimee->midPos, curReclaimee->radius * 0.7f, true, (reclaimingUnit && curReclaimee->team != team));
	return true;
}

bool CBuilder::UpdateResurrect(const Command& fCommand)
{
	CBuilderCAI* cai = static_cast<CBuilderCAI*>(commandAI);
	CFeature* curResurrectee = curResurrect;

	if (curResurrectee == nullptr || f3SqDist(curResurrectee->pos, pos) >= Square(buildDistance + curResurrectee->radius) || !inBuildStance)
		return false;

	if (fCommand.GetID() == CMD_WAIT) {
		StopBuild();
		return true;
	}

	if (curResurrectee->udef == nullptr) {
		StopBuild(true);
		return true;
	}

	if ((modInfo.reclaimMethod != 1) && (curResurrectee->reclaimLeft < 1)) {
		// this corpse has been reclaimed a little, need to restore
		// its resources before we can let the player resurrect it
		curResurrectee->AddBuildPower(this, resurrectSpeed);
		return true;
	}

	const UnitDef* resurrecteeDef = curResurrectee->udef;

	// corpse has been restored, begin resurrection
	const float step = resurrectSpeed / resurrecteeDef->buildTime;

	const bool resurrectAllowed = eventHandler.AllowFeatureBuildStep(this, curResurrectee, step);
	const bool canExecResurrect = (resurrectAllowed && UseEnergy(resurrecteeDef->energy * step * modInfo.resurrectEnergyCostFactor));

	if (canExecResurrect) {
		curResurrectee->resurrectProgress += step;
		curResurrectee->resurrectProgress = std::min(curResurrectee->resurrectProgress, 1.0f);

		CreateNanoParticle(curResurrectee->midPos, curResurrectee->radius * 0.7f, gsRNG.NextInt(2));
	}

	if (curResurrectee->resurrectProgress < 1.0f)
		return true;

	if (!curResurrectee->deleteMe) {
		// resurrect finished and we are the first
		curResurrectee->UnBlock();

		UnitLoadParams resurrecteeParams = {resurrecteeDef, this, curResurrectee->pos, ZeroVector, -1, team, curResurrectee->buildFacing, false, false};
		CUnit* resurrectee = unitLoader->LoadUnit(resurrecteeParams);

		assert(resurrecteeDef == resurrectee->unitDef);
		resurrectee->SetSoloBuilder(this, resurrecteeDef);
		resurrectee->SetHeading(curResurrectee->heading, !resurrectee->upright && resurrectee->IsOnGround(), false);

		// TODO: make configurable if this should happen
		resurrectee->health *= 0.05f;

		for (const int resurrecterID: cai->resurrecters) {
			CBuilder* resurrecter = static_cast<CBuilder*>(unitHandler.GetUnit(resurrecterID));
			CCommandAI* resurrecterCAI = resurrecter->commandAI;

			if (resurrecterCAI->commandQue.empty())
				continue;

			Command& c = resurrecterCAI->commandQue.front();

			if (c.GetID() != CMD_RESURRECT || c.GetNumParams() != 1)
				continue;

			if ((c.GetParam(0) - unitHandler.MaxUnits()) != curResurrectee->id)
				continue;

			if (!teamHandler.Ally(allyteam, resurrecter->allyteam))
				continue;

			// all units that were rezzing shall assist the repair too
			resurrecter->lastResurrected = resurrectee->id;

			// prevent FinishCommand from removing this command when the
			// feature is deleted, since it is needed to start the repair
			// (WTF!)
			c.SetParam(0, INT_MAX / 2);
		}

		// this takes one simframe to do the deletion
		featureHandler.DeleteFeature(curResurrectee);
	}

	StopBuild(true);
	return true;
}

bool CBuilder::UpdateCapture(const Command& fCommand)
{
	CUnit* curCapturee = curCapture;

	if (curCapturee == nullptr || f3SqDist(curCapturee->pos, pos) >= Square(buildDistance + curCapturee->radius) || !inBuildStance)
		return false;

	if (fCommand.GetID() == CMD_WAIT) {
		StopBuild();
		return true;
	}

	if (curCapturee->team == team) {
		StopBuild(true);
		return true;
	}

	const float captureMagicNumber = (150.0f + (curCapturee->buildTime / captureSpeed) * (curCapturee->health + curCapturee->maxHealth) / curCapturee->maxHealth * 0.4f);
	const float captureProgressStep = 1.0f / captureMagicNumber;
	const float captureProgressTemp = std::min(curCapturee->captureProgress + captureProgressStep, 1.0f);

	const float captureFraction = captureProgressTemp - curCapturee->captureProgress;
	const float energyUseScaled = curCapturee->cost.energy * captureFraction * modInfo.captureEnergyCostFactor;

	const bool captureAllowed = (eventHandler.AllowUnitBuildStep(this, curCapturee, captureProgressStep));
	const bool canExecCapture = (captureAllowed && UseEnergy(energyUseScaled));

	if (!canExecCapture)
		return true;

	curCapturee->captureProgress += captureProgressStep;
	curCapturee->captureProgress = std::min(curCapturee->captureProgress, 1.0f);

	CreateNanoParticle(curCapturee->midPos, curCapturee->radius * 0.7f, false, true);

	if (curCapturee->captureProgress < 1.0f)
		return true;

	if (!curCapturee->ChangeTeam(team, CUnit::ChangeCaptured)) {
		// capture failed
		if (team == gu->myTeam) {
			LOG_L(L_WARNING, "%s: Capture failed, unit type limit reached", unitDef->humanName.c_str());
			eventHandler.LastMessagePosition(pos);
		}
	}

	curCapturee->captureProgress = 0.0f;
	StopBuild(true);
	return true;
}



void CBuilder::Update()
{
	const CBuilderCAI* cai = static_cast<CBuilderCAI*>(commandAI);

	const CCommandQueue& cQueue = cai->commandQue;
	const Command& fCommand = (!cQueue.empty())? cQueue.front(): Command(CMD_STOP);

	bool updated = false;

	nanoPieceCache.Update();

	if (!beingBuilt && !IsStunned()) {
		updated = updated || UpdateTerraform(fCommand);
		updated = updated || AssistTerraform(fCommand);
		updated = updated || UpdateBuild(fCommand);
		updated = updated || UpdateReclaim(fCommand);
		updated = updated || UpdateResurrect(fCommand);
		updated = updated || UpdateCapture(fCommand);
	}

	CUnit::Update();
}


void CBuilder::SlowUpdate()
{
	if (terraforming)
		mapDamage->RecalcArea(tx1, tx2, tz1, tz2);

	CUnit::SlowUpdate();
}


void CBuilder::SetRepairTarget(CUnit* target)
{
	if (target == curBuild)
		return;

	StopBuild(false);
	TempHoldFire(CMD_REPAIR);

	curBuild = target;
	AddDeathDependence(curBuild, DEPENDENCE_BUILD);

	if (!target->groundLevelled) {
		// resume levelling the ground
		tx1 = (int)std::max(0.0f, (target->pos.x - (target->xsize * 0.5f * SQUARE_SIZE)) / SQUARE_SIZE);
		tz1 = (int)std::max(0.0f, (target->pos.z - (target->zsize * 0.5f * SQUARE_SIZE)) / SQUARE_SIZE);
		tx2 = std::min(mapDims.mapx, tx1 + target->xsize);
		tz2 = std::min(mapDims.mapy, tz1 + target->zsize);

		terraformCenter = target->pos;
		terraformRadius = (tx1 - tx2) * SQUARE_SIZE;
		terraformType = Terraform_Building;
		terraforming = true;
	}

	ScriptStartBuilding(target->pos, false);
}


void CBuilder::SetReclaimTarget(CSolidObject* target)
{
	if (dynamic_cast<CFeature*>(target) != nullptr && !static_cast<CFeature*>(target)->def->reclaimable)
		return;

	CUnit* recUnit = dynamic_cast<CUnit*>(target);

	if (recUnit != nullptr && !recUnit->unitDef->reclaimable)
		return;

	if (curReclaim == target || this == target)
		return;

	StopBuild(false);
	TempHoldFire(CMD_RECLAIM);

	reclaimingUnit = (recUnit != nullptr);
	curReclaim = target;

	AddDeathDependence(curReclaim, DEPENDENCE_RECLAIM);
	ScriptStartBuilding(target->pos, false);
}


void CBuilder::SetResurrectTarget(CFeature* target)
{
	if (curResurrect == target || target->udef == nullptr)
		return;

	StopBuild(false);
	TempHoldFire(CMD_RESURRECT);

	curResurrect = target;

	AddDeathDependence(curResurrect, DEPENDENCE_RESURRECT);
	ScriptStartBuilding(target->pos, false);
}


void CBuilder::SetCaptureTarget(CUnit* target)
{
	if (target == curCapture)
		return;

	StopBuild(false);
	TempHoldFire(CMD_CAPTURE);

	curCapture = target;

	AddDeathDependence(curCapture, DEPENDENCE_CAPTURE);
	ScriptStartBuilding(target->pos, false);
}


void CBuilder::StartRestore(float3 centerPos, float radius)
{
	StopBuild(false);
	TempHoldFire(CMD_RESTORE);

	terraforming = true;
	terraformType = Terraform_Restore;
	terraformCenter = centerPos;
	terraformRadius = radius;

	tx1 = (int)max((float)0,(centerPos.x-radius)/SQUARE_SIZE);
	tx2 = (int)min((float)mapDims.mapx,(centerPos.x+radius)/SQUARE_SIZE);
	tz1 = (int)max((float)0,(centerPos.z-radius)/SQUARE_SIZE);
	tz2 = (int)min((float)mapDims.mapy,(centerPos.z+radius)/SQUARE_SIZE);

	float tcost = 0.0f;
	const float* curHeightMap = readMap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readMap->GetOriginalHeightMapSynced();

	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			float delta = orgHeightMap[z * mapDims.mapxp1 + x] - curHeightMap[z * mapDims.mapxp1 + x];
			tcost += math::fabs(delta);
		}
	}
	myTerraformLeft = tcost;

	ScriptStartBuilding(centerPos, false);
}


void CBuilder::StopBuild(bool callScript)
{
	if (curBuild != nullptr)
		DeleteDeathDependence(curBuild, DEPENDENCE_BUILD);
	if (curReclaim != nullptr)
		DeleteDeathDependence(curReclaim, DEPENDENCE_RECLAIM);
	if (helpTerraform != nullptr)
		DeleteDeathDependence(helpTerraform, DEPENDENCE_TERRAFORM);
	if (curResurrect != nullptr)
		DeleteDeathDependence(curResurrect, DEPENDENCE_RESURRECT);
	if (curCapture != nullptr)
		DeleteDeathDependence(curCapture, DEPENDENCE_CAPTURE);

	curBuild = nullptr;
	curReclaim = nullptr;
	helpTerraform = nullptr;
	curResurrect = nullptr;
	curCapture = nullptr;

	terraforming = false;

	if (callScript)
		script->StopBuilding();

	SetHoldFire(false);
}


bool CBuilder::StartBuild(BuildInfo& buildInfo, CFeature*& feature, bool& inWaitStance, bool& limitReached)
{
	const CUnit* prvBuild = curBuild;

	StopBuild(false);
	TempHoldFire(-1);

	buildInfo.pos = CGameHelper::Pos2BuildPos(buildInfo, true);

	// Pass -1 as allyteam to behave like we have maphack.
	// This is needed to prevent building on top of cloaked stuff.
	const CGameHelper::BuildSquareStatus tbs = CGameHelper::TestUnitBuildSquare(buildInfo, feature, -1, true);

	switch (tbs) {
		case CGameHelper::BUILDSQUARE_OPEN:
			break;

		case CGameHelper::BUILDSQUARE_BLOCKED:
		case CGameHelper::BUILDSQUARE_OCCUPIED: {
			// the ground is blocked at the position we want
			// to build at; check if the blocking object is
			// of the same type as our buildee (which means
			// another builder has already started it)
			// note: even if construction has already started,
			// the buildee is *not* guaranteed to be the unit
			// closest to us
			const CGroundBlockingObjectMap::BlockingMapCell& cell = groundBlockingObjectMap.GetCellUnsafeConst(buildInfo.pos);

			const CSolidObject* o = nullptr;
			const CUnit* u = nullptr;

			// look for any blocking assistable buildee at build.pos
			for (size_t i = 0, n = cell.size(); i < n; i++) {
				const CUnit* cu = dynamic_cast<const CUnit*>(o = cell[i]);

				if (cu == nullptr)
					continue;
				if (!CanAssistUnit(cu, buildInfo.def))
					continue;

				u = cu;
				break;
			}

			// <pos> might map to a non-blocking portion
			// of the buildee's yardmap, fallback check
			if (u == nullptr)
				u = CGameHelper::GetClosestFriendlyUnit(nullptr, buildInfo.pos, buildDistance, allyteam);

			if (u != nullptr) {
				if (CanAssistUnit(u, buildInfo.def)) {
					// StopBuild sets this to false, fix it here if picking up the same buildee again
					terraforming = (u == prvBuild && u->terraformLeft > 0.0f);

					AddDeathDependence(curBuild = const_cast<CUnit*>(u), DEPENDENCE_BUILD);
					ScriptStartBuilding(u->pos, false);
					return true;
				}

				// let BuggerOff handle this case (TODO: non-landed aircraft should not count)
				if (buildInfo.FootPrintOverlap(u->pos, u->GetFootPrint(SQUARE_SIZE * 0.5f)))
					return false;
			}
		} break;

		case CGameHelper::BUILDSQUARE_RECLAIMABLE:
			// caller should handle this
			return false;
	}

	// at this point we know the builder is going to create a new unit, bail if at the limit
	if ((limitReached = (unitHandler.NumUnitsByTeamAndDef(team, buildInfo.def->id) >= buildInfo.def->maxThisUnit)))
		return false;

	if ((inWaitStance = !ScriptStartBuilding(buildInfo.pos, true)))
		return false;

	const UnitDef* buildeeDef = buildInfo.def;
	const UnitLoadParams buildeeParams = {buildeeDef, this, buildInfo.pos, ZeroVector, -1, team, buildInfo.buildFacing, true, false};

	CUnit* buildee = unitLoader->LoadUnit(buildeeParams);

	// floating structures don't terraform the seabed
	const bool buildeeOnWater = (buildee->FloatOnWater() && buildee->IsInWater());
	const bool allowTerraform = (!mapDamage->Disabled() && buildeeDef->levelGround);
	const bool  skipTerraform = (buildeeOnWater || buildeeDef->IsAirUnit() || !buildeeDef->IsImmobileUnit());

	if (!allowTerraform || skipTerraform) {
		// skip the terraforming job
		buildee->terraformLeft = 0.0f;
		buildee->groundLevelled = true;
	} else {
		tx1 = (int)std::max(0.0f, (buildee->pos.x - (buildee->xsize * 0.5f * SQUARE_SIZE)) / SQUARE_SIZE);
		tz1 = (int)std::max(0.0f, (buildee->pos.z - (buildee->zsize * 0.5f * SQUARE_SIZE)) / SQUARE_SIZE);
		tx2 = std::min(mapDims.mapx, tx1 + buildee->xsize);
		tz2 = std::min(mapDims.mapy, tz1 + buildee->zsize);

		buildee->terraformLeft = CalculateBuildTerraformCost(buildInfo);
		buildee->groundLevelled = false;

		terraforming    = true;
		terraformType   = Terraform_Building;
		terraformRadius = (tx2 - tx1) * SQUARE_SIZE;
		terraformCenter = buildee->pos;
	}

	// pass the *builder*'s udef for checking canBeAssisted; if buildee
	// happens to be a non-assistable factory then it would also become
	// impossible to *construct* with multiple builders
	buildee->SetSoloBuilder(this, this->unitDef);
	AddDeathDependence(curBuild = buildee, DEPENDENCE_BUILD);

	// if the ground is not going to be terraformed the buildee would
	// 'pop' to the correct height over the (un-flattened) terrain on
	// completion, so put it there to begin with
	curBuild->moveType->SlowUpdate();
	return true;
}


float CBuilder::CalculateBuildTerraformCost(BuildInfo& buildInfo)
{
	float3& buildPos = buildInfo.pos;

	float tcost = 0.0f;
	const float* curHeightMap = readMap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readMap->GetOriginalHeightMapSynced();

	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			const int idx = z * mapDims.mapxp1 + x;
			float delta = buildPos.y - curHeightMap[idx];
			float cost;
			if (delta > 0) {
				cost = max(3.0f, curHeightMap[idx] - orgHeightMap[idx] + delta * 0.5f);
			} else {
				cost = max(3.0f, orgHeightMap[idx] - curHeightMap[idx] - delta * 0.5f);
			}
			tcost += math::fabs(delta) * cost;
		}
	}

	return tcost;
}


void CBuilder::DependentDied(CObject* o)
{
	if (o == curBuild) {
		curBuild = nullptr;
		StopBuild();
	}
	if (o == curReclaim) {
		curReclaim = nullptr;
		StopBuild();
	}
	if (o == helpTerraform) {
		helpTerraform = nullptr;
		StopBuild();
	}
	if (o == curResurrect) {
		curResurrect = nullptr;
		StopBuild();
	}
	if (o == curCapture) {
		curCapture = nullptr;
		StopBuild();
	}
	CUnit::DependentDied(o);
}


bool CBuilder::ScriptStartBuilding(float3 pos, bool silent)
{
	if (script->HasStartBuilding()) {
		const float3 wantedDir = (pos - midPos).Normalize();
		const float h = GetHeadingFromVectorF(wantedDir.x, wantedDir.z);
		const float p = math::asin(wantedDir.dot(updir));
		const float pitch = math::asin(frontdir.dot(updir));

		// clamping p - pitch not needed, range of asin is -PI/2..PI/2,
		// so max difference between two asin calls is PI.
		// FIXME: convert CSolidObject::heading to radians too.
		script->StartBuilding(ClampRad(h - heading * TAANG2RAD), p - pitch);
	}

	if ((!silent || inBuildStance) && IsInLosForAllyTeam(gu->myAllyTeam))
		Channels::General->PlayRandomSample(unitDef->sounds.build, pos);

	return inBuildStance;
}


void CBuilder::HelpTerraform(CBuilder* unit)
{
	if (helpTerraform == unit)
		return;

	StopBuild(false);

	helpTerraform = unit;

	AddDeathDependence(helpTerraform, DEPENDENCE_TERRAFORM);
	ScriptStartBuilding(unit->terraformCenter, false);
}


void CBuilder::CreateNanoParticle(const float3& goal, float radius, bool inverse, bool highPriority)
{
	const int modelNanoPiece = nanoPieceCache.GetNanoPiece(script);

	if (!localModel.Initialized() || !localModel.HasPiece(modelNanoPiece))
		return;

	const float3 relNanoFirePos = localModel.GetRawPiecePos(modelNanoPiece);
	const float3 nanoPos = this->GetObjectSpacePos(relNanoFirePos);

	// unsynced
	projectileHandler.AddNanoParticle(nanoPos, goal, unitDef, team, radius, inverse, highPriority);
}
