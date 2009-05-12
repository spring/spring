// Builder.cpp: implementation of the CBuilder class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <assert.h>
#include <algorithm>
#include "Builder.h"
#include "Building.h"
#include "Game/GameHelper.h"
#include "LogOutput.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "myMath.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/GfxProjectile.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "GlobalUnsynced.h"
#include "EventHandler.h"
#include "Sound/AudioChannel.h"
#include "mmgr.h"

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


void CBuilder::UnitInit(const UnitDef* def, int team, const float3& position)
{
	range3D = def->buildRange3D;
	buildDistance  = def->buildDistance;

	const float scale = (1.0f / 32.0f);
	buildSpeed     = scale * def->buildSpeed;
	repairSpeed    = scale * def->repairSpeed;
	reclaimSpeed   = scale * def->reclaimSpeed;
	resurrectSpeed = scale * def->resurrectSpeed;
	captureSpeed   = scale * def->captureSpeed;
	terraformSpeed = scale * def->terraformSpeed;

	CUnit::UnitInit(def, team, position);
}


void CBuilder::Update()
{
	if (beingBuilt) {
		return;
	}

	if (!stunned) {
		if (terraforming && inBuildStance) {
			const float* heightmap = readmap->GetHeightmap();
			assert(!mapDamage->disabled); // The map should not be deformed in the first place.
			float terraformScale = 0.1;

			switch (terraformType) {
				case Terraform_Building:
					if (curBuild) {
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
								int idx = z * (gs->mapx + 1) + x;
								float ch = heightmap[idx];

								readmap->AddHeight(idx, (curBuild->pos.y - ch) * terraformScale);
							}
						}

						if(curBuild->terraformLeft<=0){
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
					terraformScale = (terraformSpeed + terraformHelp) / myTerraformLeft;
					myTerraformLeft -= (terraformSpeed + terraformHelp);
					terraformHelp = 0;

					if (terraformScale > 1.0f) {
						terraformScale = 1.0f;
					}

					for (int z = tz1; z <= tz2; z++) {
						for (int x = tx1; x <= tx2; x++) {
							int idx = z * (gs->mapx + 1) + x;
							float ch = heightmap[idx];
							float oh = readmap->orgheightmap[idx];

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

			CreateNanoParticle(terraformCenter,terraformRadius*0.5f,false);

			for (int z = tz1; z <= tz2; z++) {
				// smooth the borders x
				for (int x = 1; x <= 3; x++) {
					if (tx1 - 3 >= 0) {
						float ch3 = heightmap[z * (gs->mapx + 1) + tx1    ];
						float ch  = heightmap[z * (gs->mapx + 1) + tx1 - x];
						float ch2 = heightmap[z * (gs->mapx + 1) + tx1 - 3];
						float amount = ((ch3 * (3 - x) + ch2 * x) / 3 - ch) * terraformScale;

						readmap->AddHeight(z * (gs->mapx + 1) + tx1 - x, amount);
					}
					if (tx2 + 3 < gs->mapx) {
						float ch3 = heightmap[z * (gs->mapx + 1) + tx2    ];
						float ch  = heightmap[z * (gs->mapx + 1) + tx2 + x];
						float ch2 = heightmap[z * (gs->mapx + 1) + tx2 + 3];
						float amount = ((ch3 * (3 - x) + ch2 * x) / 3 - ch) * terraformScale;

						readmap->AddHeight(z * (gs->mapx + 1) + tx2 + x, amount);
					}
				}
			}
			for (int z = 1; z <= 3; z++) {
				// smooth the borders z
				for (int x = tx1; x <= tx2; x++) {
					if (tz1 - 3 >= 0) {
						float ch3 = heightmap[(tz1    ) * (gs->mapx + 1) + x];
						float ch  = heightmap[(tz1 - z) * (gs->mapx + 1) + x];
						float ch2 = heightmap[(tz1 - 3) * (gs->mapx + 1) + x];
						float adjust = ((ch3 * (3 - z) + ch2 * z) / 3 - ch) * terraformScale;

						readmap->AddHeight((tz1 - z) * (gs->mapx + 1) + x, adjust);
					}
					if (tz2 + 3 < gs->mapy) {
						float ch3 = heightmap[(tz2    ) * (gs->mapx + 1) + x];
						float ch  = heightmap[(tz2 + z) * (gs->mapx + 1) + x];
						float ch2 = heightmap[(tz2 + 3) * (gs->mapx + 1) + x];
						float adjust = ((ch3 * (3 - z) + ch2 * z) / 3 - ch) * terraformScale;

						readmap->AddHeight((tz2 + z) * (gs->mapx + 1) + x, adjust);
					}
				}
			}
		}
		else if (helpTerraform && inBuildStance) {
			if (helpTerraform->terraforming) {
				helpTerraform->terraformHelp += terraformSpeed;
				CreateNanoParticle(helpTerraform->terraformCenter,helpTerraform->terraformRadius*0.5f,false);
			} else {
				DeleteDeathDependence(helpTerraform);
				helpTerraform=0;
				StopBuild(true);
			}
		}
		else if (curBuild && f3SqDist(curBuild->pos, pos) < Square(buildDistance + curBuild->radius)) {
			if (curBuild->soloBuilder && (curBuild->soloBuilder != this)) {
				StopBuild();
			} else {
				if (!inBuildStance) {
					curBuild->AddBuildPower(0.0f, this); //prevent building timing out
				} else {
					if (scriptCloak <= 2) {
						if (isCloaked) {
							isCloaked = false;
							eventHandler.UnitDecloaked(this);
						}
						curCloakTimeout = gs->frameNum + cloakTimeout;
					}

					float adjBuildSpeed; // adjusted build speed
					if (curBuild->buildProgress < 1.0f) {
						adjBuildSpeed = buildSpeed;  // new build
					} else {
						adjBuildSpeed = min(unitDef->maxRepairSpeed / 2.0f - curBuild->repairAmount, repairSpeed); // repair
					}

					if(adjBuildSpeed > 0 && !commandAI->commandQue.empty()
							&& commandAI->commandQue.front().id == CMD_WAIT) {
						curBuild->AddBuildPower(0, this);
					} else if (adjBuildSpeed > 0 && curBuild->AddBuildPower(adjBuildSpeed, this)) {
						CreateNanoParticle(curBuild->midPos, curBuild->radius * 0.5f, false);
					} else {
						if(!curBuild->beingBuilt && curBuild->health >= curBuild->maxHealth) {
							StopBuild();
						}
					}
				}
			}
		}
		else if(curReclaim && f3SqDist(curReclaim->pos, pos)<Square(buildDistance+curReclaim->radius) && inBuildStance){
			if (scriptCloak <= 2) {
				if (isCloaked) {
					isCloaked = false;
					eventHandler.UnitDecloaked(this);
				}
				curCloakTimeout = gs->frameNum + cloakTimeout;
			}
			if (curReclaim->AddBuildPower(-reclaimSpeed, this)) {
				CreateNanoParticle(curReclaim->midPos, curReclaim->radius * 0.7f, true);
			}
		}
		else if(curResurrect && f3SqDist(curResurrect->pos, pos)<Square(buildDistance+curResurrect->radius) && inBuildStance){
			const UnitDef* ud=unitDefHandler->GetUnitByName(curResurrect->createdFromUnit);
			if(ud){
				if ((modInfo.reclaimMethod != 1) && (curResurrect->reclaimLeft < 1)) {
					// This corpse has been reclaimed a little, need to restore the resources
					// before we can let the player resurrect it.
					curResurrect->AddBuildPower(repairSpeed, this);
				}
				else {
					// Corpse has been restored, begin resurrection
					if (UseEnergy(ud->energyCost * resurrectSpeed / ud->buildTime * modInfo.resurrectEnergyCostFactor)) {
						curResurrect->resurrectProgress+=resurrectSpeed/ud->buildTime;
						CreateNanoParticle(curResurrect->midPos,curResurrect->radius*0.7f,gs->randInt()&1);
					}
					if(curResurrect->resurrectProgress>1){		//resurrect finished
						curResurrect->UnBlock();
						CUnit* u = unitLoader.LoadUnit(curResurrect->createdFromUnit, curResurrect->pos,
													team, false, curResurrect->buildFacing, this);
						if (!this->unitDef->canBeAssisted) {
							u->soloBuilder = this;
							u->AddDeathDependence(this);
						}
						u->health*=0.05f;
						u->lineage = this->lineage;
						lastResurrected=u->id;
						curResurrect->resurrectProgress=0;
						featureHandler->DeleteFeature(curResurrect);
						StopBuild(true);
					}
				}
			} else {
				StopBuild(true);
			}
		}
		else if(curCapture && f3SqDist(curCapture->pos, pos)<Square(buildDistance+curCapture->radius) && inBuildStance){
			if(curCapture->team!=team){

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
					CreateNanoParticle(curCapture->midPos,curCapture->radius*0.7f,false);
					if(curCapture->captureProgress >= 1.0f){
						if (!curCapture->ChangeTeam(team, CUnit::ChangeCaptured)) {
							// capture failed
							if (team == gu->myTeam) {
								logOutput.Print("%s: Capture failed, unit type limit reached", unitDef->humanName.c_str());
								logOutput.SetLastMsgPos(pos);
							}
						} else {
							// capture succesful
							int oldLineage = curCapture->lineage;
							curCapture->lineage = this->lineage;
							teamHandler->Team(oldLineage)->LeftLineage(curCapture);
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


void CBuilder::SlowUpdate(void)
{
	if(terraforming){
		mapDamage->RecalcArea(tx1,tx2,tz1,tz2);
	}
	CUnit::SlowUpdate();
}


void CBuilder::SetRepairTarget(CUnit* target)
{
	if(target==curBuild)
		return;

	StopBuild(false);
	TempHoldFire();

	curBuild=target;
	AddDeathDependence(curBuild);

	if(!target->groundLevelled) {
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
	if(dynamic_cast<CFeature*>(target) && !((CFeature*) target)->def->reclaimable){
		return;
	}

	if(dynamic_cast<CUnit*>(target) && !((CUnit*) target)->unitDef->reclaimable){
		return;
	}

	if(curReclaim==target || this == target){
		return;
	}

	StopBuild(false);
	TempHoldFire();

	curReclaim=target;
	AddDeathDependence(curReclaim);

	SetBuildStanceToward(target->pos);
}


void CBuilder::SetResurrectTarget(CFeature* target)
{
	if(curResurrect==target || target->createdFromUnit=="")
		return;

	StopBuild(false);
	TempHoldFire();

	curResurrect=target;
	AddDeathDependence(curResurrect);

	SetBuildStanceToward(target->pos);
}


void CBuilder::SetCaptureTarget(CUnit* target)
{
	if(target==curCapture)
		return;

	StopBuild(false);
	TempHoldFire();

	curCapture=target;
	AddDeathDependence(curCapture);

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
	const float* heightmap = readmap->GetHeightmap();

	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			float delta = readmap->orgheightmap[z * (gs->mapx + 1) + x] - heightmap[z * (gs->mapx + 1) + x];
			tcost += fabs(delta);
		}
	}
	myTerraformLeft=tcost;

	SetBuildStanceToward(centerPos);
}


void CBuilder::StopBuild(bool callScript)
{
	if(curBuild)
		DeleteDeathDependence(curBuild);
	if(curReclaim)
		DeleteDeathDependence(curReclaim);
	if(helpTerraform)
		DeleteDeathDependence(helpTerraform);
	if(curResurrect)
		DeleteDeathDependence(curResurrect);
	if(curCapture)
		DeleteDeathDependence(curCapture);
	curBuild=0;
	curReclaim=0;
	helpTerraform=0;
	curResurrect=0;
	curCapture=0;
	terraforming=false;
	if(callScript)
		script->StopBuilding();
	ReleaseTempHoldFire();
//	logOutput.Print("stop build");
}


bool CBuilder::StartBuild(BuildInfo& buildInfo)
{
	StopBuild(false);
//	logOutput.Print("start build");

	buildInfo.pos=helper->Pos2BuildPos(buildInfo);

	CFeature* feature;
	// Pass -1 as allyteam to behave like we have maphack.
	// This is needed to prevent building on top of cloaked stuff.
	int canBuild=uh->TestUnitBuildSquare(buildInfo, feature, -1);
	if(canBuild<2){
		CUnit* u=helper->GetClosestFriendlyUnit(buildInfo.pos,5,allyteam);
		if(u && u->unitDef==buildInfo.def && unitDef->canAssist){
			curBuild=u;
			AddDeathDependence(u);
			SetBuildStanceToward(buildInfo.pos);
			return true;
		}
		return false;
	}
	if(feature)
		return false;

	const UnitDef* unitDef = buildInfo.def;
	SetBuildStanceToward(buildInfo.pos);

	CUnit* b = unitLoader.LoadUnit(buildInfo.def, buildInfo.pos, team,
	                               true, buildInfo.buildFacing, this);

	// floating structures don't terraform the seabed
	const float groundheight = ground->GetHeight2(b->pos.x, b->pos.z);
	const bool onWater = (unitDef->floater && groundheight <= 0.0f);

	if (mapDamage->disabled || !unitDef->levelGround || onWater ||
	    (unitDef->canmove && (unitDef->speed > 0.0f))) {
		// skip the terraforming job
		b->terraformLeft = 0;
		b->groundLevelled=true;
	}
	else {
		tx1 = (int)max((float)0,(b->pos.x - (b->unitDef->xsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
		tx2 = min(gs->mapx,tx1+b->unitDef->xsize);
		tz1 = (int)max((float)0,(b->pos.z - (b->unitDef->zsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
		tz2 = min(gs->mapy,tz1+b->unitDef->zsize);

		b->terraformLeft = CalculateBuildTerraformCost(buildInfo);
		b->groundLevelled=false;

		terraforming=true;
		terraformType=Terraform_Building;
		terraformRadius=(tx2-tx1)*SQUARE_SIZE;
		terraformCenter = b->pos;
	}

	if (!this->unitDef->canBeAssisted) {
		b->soloBuilder = this;
		b->AddDeathDependence(this);
	}
	b->lineage = this->lineage;
	AddDeathDependence(b);
	curBuild=b;
	if (mapDamage->disabled && !(curBuild->floatOnWater)) {
		/* The ground isn't going to be terraformed.
		 * When the building is completed, it'll 'pop'
		 * into the correct height for the (un-flattened)
		 * terrain it's on.
		 *
		 * To prevent this visual artifact, put the building
		 * at the 'right' height to begin with.
		 *
		 * Duplicated from CMoveType::SlowUpdate(), which
		 * is why we use the regular code for floating things.
		 */
		curBuild->pos.y = groundheight;
		curBuild->midPos.y = groundheight + curBuild->relMidPos.y;
	}
	else {
		float d=buildInfo.pos.y-curBuild->pos.y;
		curBuild->pos.y+=d;
		curBuild->midPos.y+=d;
	}

	return true;
}


float CBuilder::CalculateBuildTerraformCost(BuildInfo& buildInfo)
{
	float3& buildPos=buildInfo.pos;

	float tcost = 0.0f;
	const float* heightmap = readmap->GetHeightmap();

	for (int z = tz1; z <= tz2; z++) {
		for (int x = tx1; x <= tx2; x++) {
			int idx = z * (gs->mapx + 1) + x;
			float delta = buildPos.y - heightmap[idx];
			float cost;
			if (delta > 0) {
				cost = max(3.0f, heightmap[idx]-readmap->orgheightmap[idx] + delta * 0.5f);
			} else {
				cost = max(3.0f, readmap->orgheightmap[idx] - heightmap[idx] - delta * 0.5f);
			}
			tcost += fabs(delta) * cost;
		}
	}

	return tcost;
}


void CBuilder::DependentDied(CObject *o)
{
	if(o==curBuild){
		curBuild=0;
		StopBuild();
	}
	if(o==curReclaim){
		curReclaim=0;
		StopBuild();
	}
	if(o==helpTerraform){
		helpTerraform=0;
		StopBuild();
	}
	if(o==curResurrect){
		curResurrect=0;
		StopBuild();
	}
	if(o==curCapture){
		curCapture=0;
		StopBuild();
	}
	CUnit::DependentDied(o);
}


void CBuilder::SetBuildStanceToward(float3 pos)
{
	if (script->HasFunction(COBFN_StartBuilding)) {
		const float3 wantedDir = (pos - midPos).Normalize();
		const float h = GetHeadingFromVectorF(wantedDir.x, wantedDir.z);
		const float p = asin(wantedDir.dot(updir));
		const float pitch = asin(frontdir.dot(updir));

		// clamping p - pitch not needed, range of asin is -PI/2..PI/2,
		// so max difference between two asin calls is PI.
		// FIXME: convert CSolidObject::heading to radians too.
		script->StartBuilding(ClampRad(h - heading * TAANG2RAD), p - pitch);
	}

	int soundIdx = unitDef->sounds.build.getRandomIdx();
	if (soundIdx >= 0) {
		Channels::UnitReply.PlaySample(
			unitDef->sounds.build.getID(soundIdx), pos,
			unitDef->sounds.build.getVolume(soundIdx));
	}
}


void CBuilder::HelpTerraform(CBuilder* unit)
{
	if(helpTerraform==unit)
		return;

	StopBuild(false);

	helpTerraform=unit;
	AddDeathDependence(helpTerraform);

	SetBuildStanceToward(unit->terraformCenter);
}


void CBuilder::CreateNanoParticle(float3 goal, float radius, bool inverse)
{
	const int piece = script->QueryNanoPiece();

#ifdef USE_GML
	if (gs->frameNum - lastDrawFrame > 20)
		return;
#endif

	if (ph->currentParticles < ph->maxParticles && unitDef->showNanoSpray) {
		float3 relWeaponFirePos = script->GetPiecePos(piece);
		float3 weaponPos = pos + frontdir * relWeaponFirePos.z + updir * relWeaponFirePos.y + rightdir * relWeaponFirePos.x;

		float3 dif = goal - weaponPos;
		const float l = fastmath::sqrt2(dif.SqLength());

		dif /= l;
		float3 error = gu->usRandVector() * (radius / l);
		float3 color = unitDef->nanoColor;

		if (gu->teamNanospray) {
			unsigned char* tcol = teamHandler->Team(team)->color;
			color = float3(tcol[0] * (1.f / 255.f), tcol[1] * (1.f / 255.f), tcol[2] * (1.f / 255.f));
		}

		if (inverse) {
			new CGfxProjectile(weaponPos + (dif + error) * l, -(dif + error) * 3, int(l / 3), color);
		} else {
			new CGfxProjectile(weaponPos, (dif + error) * 3, int(l / 3), color);
		}
	}
}
