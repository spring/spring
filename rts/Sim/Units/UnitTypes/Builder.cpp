/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include <algorithm>
#include "Builder.h"
#include "Building.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "System/myMath.h"
#include "Sim/Features/Feature.h"
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
#include "System/Sound/SoundChannels.h"
#include "System/mmgr.h"

#define PLAY_SOUNDS 1

using std::min;
using std::max;

CR_BIND_DERIVED(CBuilder, CUnit, );

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
				CR_ENUM_MEMBER(terraformType),
				CR_RESERVED(12),
				CR_POSTLOAD(PostLoad)
				));


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuilder::CBuilder():
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
	terraformCenter(0,0,0),
	terraformRadius(0)
{
}


CBuilder::~CBuilder()
{
}

void CBuilder::PostLoad()
{
	if (curResurrect)  SetBuildStanceToward(curResurrect->pos);
	if (curBuild)      SetBuildStanceToward(curBuild->pos);
	if (curCapture)    SetBuildStanceToward(curCapture->pos);
	if (curReclaim)    SetBuildStanceToward(curReclaim->pos);
	if (terraforming)  SetBuildStanceToward(terraformCenter);
	if (helpTerraform) SetBuildStanceToward(helpTerraform->terraformCenter);
}


void CBuilder::PreInit(const UnitDef* def, int team, int facing, const float3& position, bool build)
{
	range3D = def->buildRange3D;
	buildDistance = def->buildDistance;

	const float scale = (1.0f / TEAM_SLOWUPDATE_RATE);

	buildSpeed     = scale * def->buildSpeed;
	repairSpeed    = scale * def->repairSpeed;
	reclaimSpeed   = scale * def->reclaimSpeed;
	resurrectSpeed = scale * def->resurrectSpeed;
	captureSpeed   = scale * def->captureSpeed;
	terraformSpeed = scale * def->terraformSpeed;

	CUnit::PreInit(def, team, facing, position, build);
}


bool CBuilder::CanAssistUnit(const CUnit* u, const UnitDef* def) const
{
	return
		   unitDef->canAssist
		&& (!def || (u->unitDef == def))
		&& u->beingBuilt && (u->buildProgress < 1.0f)
		&& (!u->soloBuilder || (u->soloBuilder == this));
}


bool CBuilder::CanRepairUnit(const CUnit* u) const
{
	return 
		   unitDef->canRepair
		&& (!u->beingBuilt)
		&& u->unitDef->repairable && (u->health < u->maxHealth);
}


void CBuilder::Update()
{
	CBuilderCAI* cai = static_cast<CBuilderCAI*>(commandAI);

	if (!beingBuilt && !stunned) {
		if (terraforming && inBuildStance) {
			assert(!mapDamage->disabled); // The map should not be deformed in the first place.

			const float* heightmap = readmap->GetCornerHeightMapSynced();
			float terraformScale = 0.1;

			switch (terraformType) {
				case Terraform_Building:
					if (curBuild) {
						if (curBuild->terraformLeft <= 0)
							terraformScale = 0.0f;
						else
							terraformScale = (terraformSpeed + terraformHelp) / curBuild->terraformLeft;
						curBuild->terraformLeft -= (terraformSpeed + terraformHelp);
						terraformHelp = 0;

						if (terraformScale > 1.0f) {
							terraformScale = 1.0f;
						}

						// prevent building from timing out while terraforming for it
						curBuild->AddBuildPower(0.0f, this);
						for (int z = tz1; z <= tz2; z++) {
							for (int x = tx1; x <= tx2; x++) {
								int idx = z * gs->mapxp1 + x;
								float ch = heightmap[idx];

								readmap->AddHeight(idx, (curBuild->pos.y - ch) * terraformScale);
							}
						}

						if (curBuild->terraformLeft<=0) {
							terraforming=false;
							mapDamage->RecalcArea(tx1,tx2,tz1,tz2);
							curBuild->groundLevelled = true;
							if (luaRules && luaRules->TerraformComplete(this, curBuild)) {
								StopBuild();
							}
						}
					}
					break;
				case Terraform_Restore:
					if (myTerraformLeft <= 0)
						terraformScale = 0.0f;
					else
						terraformScale = (terraformSpeed + terraformHelp) / myTerraformLeft;
					myTerraformLeft -= (terraformSpeed + terraformHelp);
					terraformHelp = 0;

					if (terraformScale > 1.0f) {
						terraformScale = 1.0f;
					}

					for (int z = tz1; z <= tz2; z++) {
						for (int x = tx1; x <= tx2; x++) {
							int idx = z * gs->mapxp1 + x;
							float ch = heightmap[idx];
							float oh = readmap->GetOriginalHeightMapSynced()[idx];

							readmap->AddHeight(idx, (oh - ch) * terraformScale);
						}
					}

					if (myTerraformLeft <= 0) {
						terraforming=false;
						mapDamage->RecalcArea(tx1,tx2,tz1,tz2);
						StopBuild();
					}
					break;
			}

			ScriptDecloak(true);
			CreateNanoParticle(terraformCenter, terraformRadius * 0.5f, false);

			for (int z = tz1; z <= tz2; z++) {
				// smooth the borders x
				for (int x = 1; x <= 3; x++) {
					if (tx1 - 3 >= 0) {
						const float ch3 = heightmap[z * gs->mapxp1 + tx1    ];
						const float ch  = heightmap[z * gs->mapxp1 + tx1 - x];
						const float ch2 = heightmap[z * gs->mapxp1 + tx1 - 3];
						const float amount = ((ch3 * (3 - x) + ch2 * x) / 3 - ch) * terraformScale;

						readmap->AddHeight(z * gs->mapxp1 + tx1 - x, amount);
					}
					if (tx2 + 3 < gs->mapx) {
						const float ch3 = heightmap[z * gs->mapxp1 + tx2    ];
						const float ch  = heightmap[z * gs->mapxp1 + tx2 + x];
						const float ch2 = heightmap[z * gs->mapxp1 + tx2 + 3];
						const float amount = ((ch3 * (3 - x) + ch2 * x) / 3 - ch) * terraformScale;

						readmap->AddHeight(z * gs->mapxp1 + tx2 + x, amount);
					}
				}
			}
			for (int z = 1; z <= 3; z++) {
				// smooth the borders z
				for (int x = tx1; x <= tx2; x++) {
					if (tz1 - 3 >= 0) {
						const float ch3 = heightmap[(tz1    ) * gs->mapxp1 + x];
						const float ch  = heightmap[(tz1 - z) * gs->mapxp1 + x];
						const float ch2 = heightmap[(tz1 - 3) * gs->mapxp1 + x];
						const float adjust = ((ch3 * (3 - z) + ch2 * z) / 3 - ch) * terraformScale;

						readmap->AddHeight((tz1 - z) * gs->mapxp1 + x, adjust);
					}
					if (tz2 + 3 < gs->mapy) {
						const float ch3 = heightmap[(tz2    ) * gs->mapxp1 + x];
						const float ch  = heightmap[(tz2 + z) * gs->mapxp1 + x];
						const float ch2 = heightmap[(tz2 + 3) * gs->mapxp1 + x];
						const float adjust = ((ch3 * (3 - z) + ch2 * z) / 3 - ch) * terraformScale;

						readmap->AddHeight((tz2 + z) * gs->mapxp1 + x, adjust);
					}
				}
			}
		}
		else if (helpTerraform && inBuildStance) {
			if (helpTerraform->terraforming) {
				ScriptDecloak(true);

				helpTerraform->terraformHelp += terraformSpeed;
				CreateNanoParticle(helpTerraform->terraformCenter, helpTerraform->terraformRadius * 0.5f, false);
			} else {
				DeleteDeathDependence(helpTerraform, DEPENDENCE_TERRAFORM);
				helpTerraform = NULL;
				StopBuild(true);
			}
		}
		else if (curBuild && cai->IsInBuildRange(curBuild)) {
			if (curBuild->soloBuilder && (curBuild->soloBuilder != this)) {
				StopBuild();
			} else {
				if (inBuildStance || true) {
					// NOTE:
					//     technically this block of code should be guarded by
					//     "if (inBuildStance)", but doing so can create zombie
					//     guarders because scripts might not set inBuildStance
					//     to true when guard or repair orders are executed and
					//     SetRepairTarget does not check for it
					//
					//     StartBuild *does* ensure construction will not start
					//     until inBuildStance is set to true by the builder's
					//     script, and there are no cases during construction
					//     when inBuildStance can become false yet the buildee
					//     should be kept from decaying, so this is free from
					//     serious side-effects (when repairing, a builder might
					//     start adding build-power before having fully finished
					//     its opening animation)
					//
					ScriptDecloak(true);

					// adjusted build-speed: use repair-speed on units with
					// progress >= 1 rather than raw build-speed on buildees
					// with progress < 1
					float adjBuildSpeed = buildSpeed;

					if (curBuild->buildProgress >= 1.0f) {
						adjBuildSpeed = std::min(unitDef->maxRepairSpeed / 2.0f - curBuild->repairAmount, repairSpeed);
					}

					const CCommandQueue& queue = cai->commandQue;
					const Command& command = (!queue.empty())? queue.front(): Command();

					if (adjBuildSpeed > 0.0f && command.GetID() == CMD_WAIT) {
						// prevent buildee from decaying
						curBuild->AddBuildPower(0.0f, this);
					} else if (adjBuildSpeed > 0.0f && curBuild->AddBuildPower(adjBuildSpeed, this)) {
						CreateNanoParticle(curBuild->midPos, curBuild->radius * 0.5f, false);
					} else {
						// check if buildee finished construction
						if (!curBuild->beingBuilt && curBuild->health >= curBuild->maxHealth) {
							StopBuild();
						}
					}
				}
			}
		}
		else if (curReclaim && f3SqDist(curReclaim->pos, pos)<Square(buildDistance+curReclaim->radius) && inBuildStance) {
			ScriptDecloak(true);

			if (curReclaim->AddBuildPower(-reclaimSpeed, this)) {
				CreateNanoParticle(curReclaim->midPos, curReclaim->radius * 0.7f, true, (reclaimingUnit && curReclaim->team != team));
			}
		}
		else if (curResurrect && f3SqDist(curResurrect->pos, pos)<Square(buildDistance+curResurrect->radius) && inBuildStance) {
			const UnitDef* ud = curResurrect->udef;

			if (ud) {
				if ((modInfo.reclaimMethod != 1) && (curResurrect->reclaimLeft < 1)) {
					// This corpse has been reclaimed a little, need to restore the resources
					// before we can let the player resurrect it.
					curResurrect->AddBuildPower(repairSpeed, this);
				}
				else {
					// Corpse has been restored, begin resurrection
					if (UseEnergy(ud->energy * resurrectSpeed / ud->buildTime * modInfo.resurrectEnergyCostFactor)) {
						curResurrect->resurrectProgress += (resurrectSpeed / ud->buildTime);
						CreateNanoParticle(curResurrect->midPos, curResurrect->radius * 0.7f, (gs->randInt() & 1));
					}
					if (curResurrect->resurrectProgress > 1) {
						// resurrect finished
						curResurrect->UnBlock();
						CUnit* u = unitLoader->LoadUnit(ud, curResurrect->pos,
													team, false, curResurrect->buildFacing, this);

						if (!this->unitDef->canBeAssisted) {
							u->soloBuilder = this;
							u->AddDeathDependence(this, DEPENDENCE_BUILDER);
						}
						u->health *= 0.05f;

						for (CUnitSet::iterator it = cai->resurrecters.begin(); it != cai->resurrecters.end(); ++it) {
							CBuilder* bld = (CBuilder*) *it;
							CCommandAI* bldCAI = bld->commandAI;

							if (bldCAI->commandQue.empty())
								continue;

							Command& c = bldCAI->commandQue.front();

							if (c.GetID() != CMD_RESURRECT || c.params.size() != 1)
								continue;

							const int cmdFeatureId = c.params[0];

							if (cmdFeatureId - uh->MaxUnits() == curResurrect->id && teamHandler->Ally(allyteam, bld->allyteam)) {
								bld->lastResurrected = u->id; // all units that were rezzing shall assist the repair too
								c.params[0] = INT_MAX / 2; // prevent FinishCommand from removing this command when the feature is deleted, since it is needed to start the repair
							}
						}

						curResurrect->resurrectProgress = 0;
						featureHandler->DeleteFeature(curResurrect);
						StopBuild(true);
					}
				}
			} else {
				StopBuild(true);
			}
		}
		else if (curCapture && f3SqDist(curCapture->pos, pos)<Square(buildDistance+curCapture->radius) && inBuildStance) {
			if (curCapture->team!=team) {

				float captureProgressTemp = curCapture->captureProgress + 1.0f/(150+curCapture->buildTime/captureSpeed*(curCapture->health+curCapture->maxHealth)/curCapture->maxHealth*0.4f);
				if (captureProgressTemp >= 1.0f) {
					captureProgressTemp = 1.0f;
				}

				const float captureFraction = captureProgressTemp - curCapture->captureProgress;
				const float energyUseScaled = curCapture->energyCost * captureFraction * modInfo.captureEnergyCostFactor;

				if (!UseEnergy(energyUseScaled)) {
					teamHandler->Team(team)->energyPull += energyUseScaled;
				} else {
					curCapture->captureProgress = captureProgressTemp;
					CreateNanoParticle(curCapture->midPos, curCapture->radius * 0.7f, false, true);
					if (curCapture->captureProgress >= 1.0f) {
						if (!curCapture->ChangeTeam(team, CUnit::ChangeCaptured)) {
							// capture failed
							if (team == gu->myTeam) {
								LOG_L(L_WARNING, "%s: Capture failed, unit type limit reached", unitDef->humanName.c_str());
								eventHandler.LastMessagePosition(pos);
							}
						}
						curCapture->captureProgress=0.0f;
						StopBuild(true);
					}
				}
			} else {
				StopBuild(true);
			}
		}
	}

	CUnit::Update();
}


void CBuilder::SlowUpdate()
{
	if (terraforming) {
		mapDamage->RecalcArea(tx1,tx2,tz1,tz2);
	}
	CUnit::SlowUpdate();
}


void CBuilder::SetRepairTarget(CUnit* target)
{
	if (target == curBuild)
		return;

	StopBuild(false);
	TempHoldFire();

	curBuild = target;
	AddDeathDependence(curBuild, DEPENDENCE_BUILD);

	if (!target->groundLevelled) {
		//resume levelling the ground
		tx1 = (int)max((float)0,(target->pos.x - (target->unitDef->xsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
		tx2 = min(gs->mapx,tx1+target->unitDef->xsize);
		tz1 = (int)max((float)0,(target->pos.z - (target->unitDef->zsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
		tz2 = min(gs->mapy,tz1+target->unitDef->zsize);
		terraformCenter = target->pos;
		terraformRadius = (tx1 - tx2) * SQUARE_SIZE;
		terraformType=Terraform_Building;
		terraforming = true;
	}

	SetBuildStanceToward(target->pos);
}


void CBuilder::SetReclaimTarget(CSolidObject* target)
{
	if (dynamic_cast<CFeature*>(target) && !static_cast<CFeature*>(target)->def->reclaimable) {
		return;
	}

	CUnit* recUnit = dynamic_cast<CUnit*>(target);
	if (recUnit && !recUnit->unitDef->reclaimable) {
		return;
	}

	if (curReclaim==target || this == target) {
		return;
	}

	StopBuild(false);
	TempHoldFire();

	reclaimingUnit = !!recUnit;
	curReclaim=target;
	AddDeathDependence(curReclaim, DEPENDENCE_RECLAIM);

	SetBuildStanceToward(target->pos);
}


void CBuilder::SetResurrectTarget(CFeature* target)
{
	if (curResurrect == target || target->udef == NULL)
		return;

	StopBuild(false);
	TempHoldFire();

	curResurrect = target;
	AddDeathDependence(curResurrect, DEPENDENCE_RESURRECT);

	SetBuildStanceToward(target->pos);
}


void CBuilder::SetCaptureTarget(CUnit* target)
{
	if (target == curCapture)
		return;

	StopBuild(false);
	TempHoldFire();

	curCapture = target;
	AddDeathDependence(curCapture, DEPENDENCE_CAPTURE);

	SetBuildStanceToward(target->pos);
}


void CBuilder::StartRestore(float3 centerPos, float radius)
{
	StopBuild(false);
	TempHoldFire();

	terraforming=true;
	terraformType=Terraform_Restore;
	terraformCenter=centerPos;
	terraformRadius=radius;

	tx1 = (int)max((float)0,(centerPos.x-radius)/SQUARE_SIZE);
	tx2 = (int)min((float)gs->mapx,(centerPos.x+radius)/SQUARE_SIZE);
	tz1 = (int)max((float)0,(centerPos.z-radius)/SQUARE_SIZE);
	tz2 = (int)min((float)gs->mapy,(centerPos.z+radius)/SQUARE_SIZE);

	float tcost = 0.0f;
	const float* curHeightMap = readmap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readmap->GetOriginalHeightMapSynced();

	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			float delta = orgHeightMap[z * gs->mapxp1 + x] - curHeightMap[z * gs->mapxp1 + x];
			tcost += math::fabs(delta);
		}
	}
	myTerraformLeft = tcost;

	SetBuildStanceToward(centerPos);
}


void CBuilder::StopBuild(bool callScript)
{
	if (curBuild)
		DeleteDeathDependence(curBuild, DEPENDENCE_BUILD);
	if (curReclaim)
		DeleteDeathDependence(curReclaim, DEPENDENCE_RECLAIM);
	if (helpTerraform)
		DeleteDeathDependence(helpTerraform, DEPENDENCE_TERRAFORM);
	if (curResurrect)
		DeleteDeathDependence(curResurrect, DEPENDENCE_RESURRECT);
	if (curCapture)
		DeleteDeathDependence(curCapture, DEPENDENCE_CAPTURE);

	curBuild = 0;
	curReclaim = 0;
	helpTerraform = 0;
	curResurrect = 0;
	curCapture = 0;
	terraforming = false;

	if (callScript)
		script->StopBuilding();
	ReleaseTempHoldFire();
}


bool CBuilder::StartBuild(BuildInfo& buildInfo, CFeature*& feature, bool& waitstance)
{
	StopBuild(false);

	buildInfo.pos = helper->Pos2BuildPos(buildInfo, true);

	// Pass -1 as allyteam to behave like we have maphack.
	// This is needed to prevent building on top of cloaked stuff.
	const BuildSquareStatus tbs = uh->TestUnitBuildSquare(buildInfo, feature, -1, true);

	switch (tbs) {
		case BUILDSQUARE_OPEN:
			break;

		case BUILDSQUARE_BLOCKED:
		case BUILDSQUARE_OCCUPIED: {
			// the ground is blocked at the position we want
			// to build at; check if the blocking object is
			// of the same type as our buildee (which means
			// another builder has already started it)
			// note: even if construction has already started,
			// the buildee is *not* guaranteed to be the unit
			// closest to us
			CSolidObject* o = groundBlockingObjectMap->GroundBlocked(buildInfo.pos);
			CUnit* u = NULL;

			if (o != NULL) {
				u = dynamic_cast<CUnit*>(o);
			} else {
				// <pos> might map to a non-blocking portion
				// of the buildee's yardmap, fallback check
				u = helper->GetClosestFriendlyUnit(buildInfo.pos, buildDistance, allyteam);
			}

			if (u != NULL && CanAssistUnit(u, buildInfo.def)) {
				curBuild = u;
				AddDeathDependence(u, DEPENDENCE_BUILD);
				SetBuildStanceToward(u->pos);
				return true;
			}

			return false;
		}

		case BUILDSQUARE_RECLAIMABLE:
			// caller should handle this
			return false;
	}

	SetBuildStanceToward(buildInfo.pos);

	if (!inBuildStance) {
		waitstance = true;
		return false;
	}

	const UnitDef* buildeeDef = buildInfo.def;
	CUnit* buildee = unitLoader->LoadUnit(buildeeDef, buildInfo.pos, team,
	                               true, buildInfo.buildFacing, this);

	// floating structures don't terraform the seabed
	const float groundheight = ground->GetHeightReal(buildee->pos.x, buildee->pos.z);
	const bool onWater = (buildeeDef->floatOnWater && groundheight <= 0.0f);

	if (mapDamage->disabled || !buildeeDef->levelGround || onWater ||
	    (buildeeDef->canmove && (buildeeDef->speed > 0.0f))) {
		// skip the terraforming job
		buildee->terraformLeft = 0;
		buildee->groundLevelled = true;
	} else {
		tx1 = (int)max(    0.0f, (buildee->pos.x - (buildeeDef->xsize * 0.5f * SQUARE_SIZE)) / SQUARE_SIZE);
		tx2 =      min(gs->mapx,             tx1 +  buildeeDef->xsize                                     );
		tz1 = (int)max(    0.0f, (buildee->pos.z - (buildeeDef->zsize * 0.5f * SQUARE_SIZE)) / SQUARE_SIZE);
		tz2 =      min(gs->mapy,             tz1 +  buildeeDef->zsize                                     );

		buildee->terraformLeft = CalculateBuildTerraformCost(buildInfo);
		buildee->groundLevelled = false;

		terraforming    = true;
		terraformType   = Terraform_Building;
		terraformRadius = (tx2 - tx1) * SQUARE_SIZE;
		terraformCenter = buildee->pos;
	}

	if (!this->unitDef->canBeAssisted) {
		buildee->soloBuilder = this;
		buildee->AddDeathDependence(this, DEPENDENCE_BUILDER);
	}

	AddDeathDependence(buildee, DEPENDENCE_BUILD);
	curBuild = buildee;

	/* The ground isn't going to be terraformed.
	 * When the building is completed, it'll 'pop'
	 * into the correct height for the (un-flattened)
	 * terrain it's on.
	 *
	 * To prevent this visual artifact, put the building
	 * at the 'right' height to begin with.
	 */
	curBuild->moveType->SlowUpdate();
	return true;
}


float CBuilder::CalculateBuildTerraformCost(BuildInfo& buildInfo)
{
	float3& buildPos=buildInfo.pos;

	float tcost = 0.0f;
	const float* curHeightMap = readmap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readmap->GetOriginalHeightMapSynced();

	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			const int idx = z * gs->mapxp1 + x;
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


void CBuilder::DependentDied(CObject *o)
{
	if (o == curBuild) {
		curBuild = 0;
		StopBuild();
	}
	if (o == curReclaim) {
		curReclaim = 0;
		StopBuild();
	}
	if (o == helpTerraform) {
		helpTerraform = 0;
		StopBuild();
	}
	if (o == curResurrect) {
		curResurrect = 0;
		StopBuild();
	}
	if (o == curCapture) {
		curCapture = 0;
		StopBuild();
	}
	CUnit::DependentDied(o);
}


void CBuilder::SetBuildStanceToward(float3 pos)
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

	#if (PLAY_SOUNDS == 1)
	if (losStatus[gu->myAllyTeam] & LOS_INLOS) {
		Channels::General.PlayRandomSample(unitDef->sounds.build, pos);
	}
	#endif
}


void CBuilder::HelpTerraform(CBuilder* unit)
{
	if (helpTerraform==unit)
		return;

	StopBuild(false);

	helpTerraform=unit;
	AddDeathDependence(helpTerraform, DEPENDENCE_TERRAFORM);

	SetBuildStanceToward(unit->terraformCenter);
}


void CBuilder::CreateNanoParticle(const float3& goal, float radius, bool inverse, bool highPriority)
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
	ph->AddNanoParticle(weaponPos, goal, unitDef, team, radius, inverse, highPriority);
}
