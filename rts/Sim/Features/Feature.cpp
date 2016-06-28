/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Feature.h"
#include "FeatureDef.h"
#include "FeatureDefHandler.h"
#include "FeatureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/FireProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/Env/Particles/Classes/GeoThermSmokeProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/creg/DefTypes.h"
#include "System/Log/ILog.h"
#include <assert.h>


CR_BIND_DERIVED(CFeature, CSolidObject, )

CR_REG_METADATA(CFeature, (
	CR_MEMBER(isRepairingBeforeResurrect),
	CR_MEMBER(inUpdateQue),
	CR_MEMBER(deleteMe),
	CR_MEMBER(resurrectProgress),
	CR_MEMBER(reclaimLeft),
	CR_MEMBER(resources),
	CR_MEMBER(lastReclaim),
	CR_MEMBER(drawQuad),
	CR_MEMBER(drawFlag),
	CR_MEMBER(drawAlpha),
	CR_MEMBER(alphaFade),
	CR_MEMBER(fireTime),
	CR_MEMBER(smokeTime),
	CR_MEMBER(def),
	CR_MEMBER(udef),
	CR_MEMBER(moveCtrl),
	CR_MEMBER(myFire),
	CR_MEMBER(solidOnTop),
	CR_MEMBER(transMatrix),
	CR_POSTLOAD(PostLoad)
))

CR_BIND(CFeature::MoveCtrl,)

CR_REG_METADATA_SUB(CFeature,MoveCtrl,(
	CR_MEMBER(enabled),

	CR_MEMBER(movementMask),
	CR_MEMBER(velocityMask),
	CR_MEMBER(impulseMask),

	CR_MEMBER(velVector),
	CR_MEMBER(accVector)
))


CFeature::CFeature()
: CSolidObject()
, isRepairingBeforeResurrect(false)
, inUpdateQue(false)
, deleteMe(false)
, resurrectProgress(0.0f)
, reclaimLeft(1.0f)
, lastReclaim(0)
, resources(0.0f, 1.0f)

, drawQuad(-2)
, drawFlag(-1)

, drawAlpha(1.0f)
, alphaFade(true)

, fireTime(0)
, smokeTime(0)
, def(NULL)
, udef(NULL)
, myFire(NULL)
, solidOnTop(NULL)
{
	crushable = true;
	immobile = true;
}


CFeature::~CFeature()
{
	UnBlock();
	quadField->RemoveFeature(this);

	if (myFire != NULL) {
		myFire->StopFire();
		myFire = NULL;
	}

	if (def->geoThermal) {
		CGeoThermSmokeProjectile::GeoThermDestroyed(this);
	}
}


void CFeature::PostLoad()
{
	eventHandler.RenderFeatureCreated(this);
}


void CFeature::ChangeTeam(int newTeam)
{
	if (newTeam < 0) {
		// remap all negative teams to Gaia
		// if the Gaia team is not enabled, these would become
		// -1 and we remap them again (to 0) to prevent crashes
		team = std::max(0, teamHandler->GaiaTeamID());
		allyteam = std::max(0, teamHandler->GaiaAllyTeamID());
	} else {
		team = newTeam;
		allyteam = teamHandler->AllyTeam(newTeam);
	}
}


bool CFeature::IsInLosForAllyTeam(int argAllyTeam) const
{
	if (alwaysVisible || argAllyTeam == -1)
		return true;

	const bool isGaia = allyteam == std::max(0, teamHandler->GaiaAllyTeamID());

	switch (modInfo.featureVisibility) {
		case CModInfo::FEATURELOS_NONE:
		default:
			return losHandler->InLos(pos, argAllyTeam);

		// these next two only make sense when Gaia is enabled
		case CModInfo::FEATURELOS_GAIAONLY:
			return (isGaia || losHandler->InLos(pos, argAllyTeam));
		case CModInfo::FEATURELOS_GAIAALLIED:
			return (isGaia || allyteam == argAllyTeam || losHandler->InLos(pos, argAllyTeam));

		case CModInfo::FEATURELOS_ALL:
			return true;
	}
}


void CFeature::Initialize(const FeatureLoadParams& params)
{
	def = params.featureDef;
	udef = params.unitDef;

	id = params.featureID;

	team = params.teamID;
	allyteam = params.allyTeamID;

	heading = params.heading;
	buildFacing = params.facing;
	smokeTime = params.smokeTime;

	mass = def->mass;
	health = def->health;
	maxHealth = def->health;

	resources = SResourcePack(def->metal, def->energy);

	crushResistance = def->crushResistance;

	xsize = ((buildFacing & 1) == 0) ? def->xsize : def->zsize;
	zsize = ((buildFacing & 1) == 1) ? def->xsize : def->zsize;

	noSelect = !def->selectable;

	// by default, any feature that is a dead unit can move
	// in all dimensions; all others (trees, rocks, ...) can
	// only move vertically and also do not allow velocity to
	// build in XZ
	// (movementMask exists mostly for trees, which depend on
	// speed for falling animations but should never actually
	// *move* in XZ, so their velocityMask *does* include XZ)
	moveCtrl.SetMovementMask(mix(OnesVector, UpVector, udef == nullptr                                 ));
	moveCtrl.SetVelocityMask(mix(OnesVector, UpVector, udef == nullptr && def->drawType < DRAWTYPE_TREE));

	// set position before mid-position
	Move((params.pos).cClampInMap(), false);
	// use base-class version, AddFeature() below
	// will already insert us in the update-queue
	CWorldObject::SetVelocity(params.speed);

	switch (def->drawType) {
		case DRAWTYPE_NONE: {
		} break;

		case DRAWTYPE_MODEL: {
			if ((model = def->LoadModel()) != NULL) {
				SetMidAndAimPos(model->relMidPos, model->relMidPos, true);
				SetRadiusAndHeight(model);

				// only initialize the LM for modelled features
				// (this is still never animated but allows for
				// custom piece display-lists, etc)
				localModel.SetModel(model);
			} else {
				LOG_L(L_ERROR, "[%s] couldn't load model for %s", __FUNCTION__, def->name.c_str());
			}
		} break;

		default: {
			// always >= DRAWTYPE_TREE here
			// LoadFeaturesFromMap() doesn't set a scale for trees
			SetMidAndAimPos(UpVector * TREE_RADIUS, UpVector * TREE_RADIUS, true);
			SetRadiusAndHeight(TREE_RADIUS, TREE_RADIUS * 2.0f);
			drawPos = pos;
			drawMidPos = midPos;
		} break;
	}

	UpdateMidAndAimPos();
	UpdateTransformAndPhysState();

	collisionVolume = def->collisionVolume;

	if (collisionVolume.DefaultToSphere())
		collisionVolume.InitSphere(radius);
	if (collisionVolume.DefaultToFootPrint())
		collisionVolume.InitBox(float3(xsize * SQUARE_SIZE, height, zsize * SQUARE_SIZE));


	// feature does not have an assigned ID yet
	// this MUST be done before the Block() call
	featureHandler->AddFeature(this);
	quadField->AddFeature(this);

	ChangeTeam(team);
	UpdateCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS, def->collidable);
	Block();

	// allow Spring.SetFeatureBlocking to be called from gadget:FeatureCreated
	// (callin sees the complete default state, but can change any part of it)
	eventHandler.FeatureCreated(this);
	eventHandler.RenderFeatureCreated(this);
}


bool CFeature::AddBuildPower(CUnit* builder, float amount)
{
	const float oldReclaimLeft = reclaimLeft;

	if (amount > 0.0f) {
		// 'Repairing' previously-sucked features prior to resurrection
		// This is reclaim-option independant - repairing features should always
		// be like other repairing - gradual and multi-unit
		// Lots of this code is stolen from unit->AddBuildPower
		//
		// Check they are trying to repair a feature that can be resurrected
		if (udef == nullptr)
			return false;

		isRepairingBeforeResurrect = true; // Stop them exploiting chunk reclaiming

		// cannot repair a 'fresh' feature
		if (reclaimLeft >= 1.0f)
			return false;
		// feature most likely has been deleted
		if (reclaimLeft <= 0.0f)
			return false;

		const CTeam* builderTeam = teamHandler->Team(builder->team);

		// Work out how much to try to put back, based on the speed this unit would reclaim at.
		const float step = amount / def->reclaimTime;

		// Work out how much that will cost
		const float metalUse  = step * def->metal;
		const float energyUse = step * def->energy;
		const bool canExecRepair = (builderTeam->res.metal >= metalUse && builderTeam->res.energy >= energyUse);
		const bool repairAllowed = !canExecRepair ? false : eventHandler.AllowFeatureBuildStep(builder, this, step);

		if (repairAllowed) {
			builder->UseMetal(metalUse);
			builder->UseEnergy(energyUse);

			reclaimLeft += step;
			resources.metal  += metalUse;
			resources.energy += energyUse;
			resources.metal  = std::min(resources.metal, def->metal);
			resources.energy = std::min(resources.energy, def->energy);

			if (reclaimLeft >= 1.0f) {
				isRepairingBeforeResurrect = false; // They can start reclaiming it again if they so wish
				reclaimLeft = 1;
			} else if (reclaimLeft <= 0.0f) {
				// this can happen when a mod tampers the feature in AllowFeatureBuildStep
				featureHandler->DeleteFeature(this);
				return false;
			}

			return true;
		}

		// update the energy and metal required counts
		teamHandler->Team(builder->team)->resPull.energy += energyUse;
		teamHandler->Team(builder->team)->resPull.metal  += metalUse;
		return false;
	}

	// Reclaiming
	// avoid multisuck when reclaim has already completed during this frame
	if (reclaimLeft <= 0.0f)
		return false;

	// don't let them exploit chunk reclaim
	if (isRepairingBeforeResurrect && (modInfo.reclaimMethod > 1))
		return false;

	// make sure several units cant reclaim at once on a single feature
	if ((modInfo.multiReclaim == 0) && (lastReclaim == gs->frameNum))
		return true;

	const float step = (-amount) / def->reclaimTime;

	if (!eventHandler.AllowFeatureBuildStep(builder, this, -step))
		return false;

	// stop the last bit giving too much resource
	const float reclaimLeftTemp = std::max(0.0f, reclaimLeft - step);
	const float fractionReclaimed = oldReclaimLeft - reclaimLeftTemp;
	const float metalFraction  = std::min(def->metal  * fractionReclaimed, resources.metal);
	const float energyFraction = std::min(def->energy * fractionReclaimed, resources.energy);
	const float energyUseScaled = metalFraction * modInfo.reclaimFeatureEnergyCostFactor;

	SResourceOrder order;
	order.quantum    = false;
	order.overflow   = builder->harvestStorage.empty();
	order.separate   = true;
	order.use.energy = energyUseScaled;

	if (reclaimLeftTemp == 0.0f) {
		// always give remaining resources at the end
		order.add.metal  = resources.metal;
		order.add.energy = resources.energy;
	}
	else if (modInfo.reclaimMethod == 0) {
		// Gradual reclaim
		order.add.metal  = metalFraction;
		order.add.energy = energyFraction;
	}
	else if (modInfo.reclaimMethod == 1) {
		// All-at-end method
		// see `reclaimLeftTemp == 0.0f` case
	}
	else {
		// Chunky reclaiming, work out how many chunk boundaries we crossed
		const float chunkSize = 1.0f / modInfo.reclaimMethod;
		const int oldChunk = ChunkNumber(oldReclaimLeft);
		const int newChunk = ChunkNumber(reclaimLeft);

		if (oldChunk != newChunk) {
			const float numChunks = oldChunk - newChunk;
			order.add.metal  = std::min(numChunks * def->metal * chunkSize,  resources.metal);
			order.add.energy = std::min(numChunks * def->energy * chunkSize, resources.energy);
		}
	}

	if (!builder->IssueResourceOrder(&order))
		return false;

	resources  -= order.add;
	reclaimLeft = reclaimLeftTemp;
	lastReclaim = gs->frameNum;

	// Has the reclaim finished?
	if (reclaimLeft <= 0) {
		featureHandler->DeleteFeature(this);
		return false;
	}

	return true;
}


void CFeature::DoDamage(
	const DamageArray& damages,
	const float3& impulse,
	CUnit* attacker,
	int weaponDefID,
	int projectileID
) {
	// paralyzers do not damage features
	if (damages.paralyzeDamageTime)
		return;
	if (IsInVoid())
		return;

	// features have no armor-type, so use default damage
	float baseDamage = damages.GetDefault();
	float impulseMult = float((def->drawType >= DRAWTYPE_TREE) || (udef != NULL && !udef->IsImmobileUnit()));

	if (eventHandler.FeaturePreDamaged(this, attacker, baseDamage, weaponDefID, projectileID, &baseDamage, &impulseMult))
		return;

	// NOTE:
	//   for trees, impulse is used to drive their falling animation
	//   this also calls our SetVelocity, which puts us in the update
	//   queue
	ApplyImpulse((impulse * moveCtrl.impulseMask * impulseMult) / mass);

	// clamp in case Lua-modified damage is negative
	health -= baseDamage;
	health = std::min(health, def->health);

	eventHandler.FeatureDamaged(this, attacker, baseDamage, weaponDefID, projectileID);

	if (health <= 0.0f && def->destructable) {
		FeatureLoadParams params = {featureDefHandler->GetFeatureDefByID(def->deathFeatureDefID), NULL, pos, speed, -1, team, -1, heading, buildFacing, 0};
		CFeature* deathFeature = featureHandler->CreateWreckage(params, 0, false);

		if (deathFeature != NULL) {
			// if a partially reclaimed corpse got blasted,
			// ensure its wreck is not worth the full amount
			// (which might be more than the amount remaining)
			deathFeature->resources.metal  *= (def->metal != 0.0f)  ? resources.metal  / def->metal  : 1.0f;
			deathFeature->resources.energy *= (def->energy != 0.0f) ? resources.energy / def->energy : 1.0f;
		}

		featureHandler->DeleteFeature(this);
		blockHeightChanges = false;
	}
}



void CFeature::DependentDied(CObject *o)
{
	if (o == solidOnTop)
		solidOnTop = 0;

	CSolidObject::DependentDied(o);
}


void CFeature::SetVelocity(const float3& v)
{
	CWorldObject::SetVelocity(v * moveCtrl.velocityMask);
	CWorldObject::SetSpeed(v * moveCtrl.velocityMask);

	UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, speed.w != 0.0f);

	if (IsMoving()) {
		featureHandler->SetFeatureUpdateable(this);
	}
}


void CFeature::ForcedMove(const float3& newPos)
{
	// remove from managers
	quadField->RemoveFeature(this);

	const float3 oldPos = pos;

	UnBlock();
	Move(newPos - pos, true);
	Block();

	// ForcedMove calls might cause the pstate to go stale
	// (features are only Update()'d when in the FH queue)
	UpdateTransformAndPhysState();

	eventHandler.FeatureMoved(this, oldPos);

	// insert into managers
	quadField->AddFeature(this);
}


void CFeature::ForcedSpin(const float3& newDir)
{
	// update local direction-vectors
	CSolidObject::ForcedSpin(newDir);
	UpdateTransform(pos, true);
}


void CFeature::UpdateTransformAndPhysState()
{
	UpdateDirVectors(!def->upright);
	UpdateTransform(pos, true);

	UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, (SetSpeed(speed) != 0.0f));
	UpdatePhysicalState(0.1f);
}

void CFeature::UpdateQuadFieldPosition(const float3& moveVec)
{
	quadField->RemoveFeature(this);
	UnBlock();

	Move(moveVec, true);

	Block();
	quadField->AddFeature(this);
}


bool CFeature::UpdateVelocity(
	const float3& dragAccel,
	const float3& gravAccel,
	const float3& movMask,
	const float3& velMask
) {
	// apply drag and gravity to speed; leave more advanced physics (water
	// buoyancy, etc) to Lua
	// NOTE:
	//   all these calls use the base-class because FeatureHandler::Update
	//   iterates over updateFeatures and our ::SetVelocity will insert us
	//   into that
	//
	// drag is only valid for current speed, needs to be applied first
	CWorldObject::SetVelocity((speed + dragAccel) * velMask);

	if (!IsInWater()) {
		// quadratic downward acceleration if not in water
		CWorldObject::SetVelocity(((speed * OnesVector) + gravAccel) * velMask);
	} else {
		// constant downward speed otherwise, unless floating
		CWorldObject::SetVelocity(((speed *   XZVector) + gravAccel * (1 - def->floating)) * velMask);
	}

	const float oldGroundHeight = CGround::GetHeightReal(pos        );
	const float newGroundHeight = CGround::GetHeightReal(pos + speed);

	// adjust vertical speed so we do not sink into the ground
	if ((pos.y + speed.y) <= newGroundHeight) {
		speed.y  = std::min(newGroundHeight - pos.y, math::fabs(newGroundHeight - oldGroundHeight));
		speed.y *= moveCtrl.velocityMask.y;
	}

	// indicates whether to update quadfield position
	return ((speed.x * movMask.x) != 0.0f || (speed.z * movMask.z) != 0.0f);
}

bool CFeature::UpdatePosition()
{
	const float3 oldPos = pos;
	// const float4 oldSpd = speed;

	if (moveCtrl.enabled) {
		// raw movement; not masked or clamped
		UpdateQuadFieldPosition(speed = (moveCtrl.velVector += moveCtrl.accVector));
	} else {
		const float3 dragAccel = GetDragAccelerationVec(float4(mapInfo->atmosphere.fluidDensity, mapInfo->water.fluidDensity, 1.0f, 0.1f));
		const float3 gravAccel = UpVector * mapInfo->map.gravity;

		// horizontal movement
		if (UpdateVelocity(dragAccel, gravAccel, moveCtrl.movementMask, moveCtrl.velocityMask))
			UpdateQuadFieldPosition((speed * XZVector) * moveCtrl.movementMask);

		// vertical movement
		Move((speed * UpVector) * moveCtrl.movementMask, true);
		// adjusting vertical speed won't help if the ground moved and buried us
		Move(UpVector * (std::max(CGround::GetHeightReal(pos.x, pos.z), pos.y) - pos.y), true);

		// clamp final position
		if (!pos.IsInBounds()) {
			Move(pos.cClampInBounds(), false);

			// ensure that no more horizontal movement is done
			CWorldObject::SetVelocity((speed * UpVector) * moveCtrl.velocityMask);
		}
	}

	UpdateTransformAndPhysState(); // updates speed.w and BIT_MOVING
	Block(); // does the check if wanted itself

	// use an exact comparison for the y-component (gravity is small)
	if (!pos.equals(oldPos, float3(float3::cmp_eps(), 0.0f, float3::cmp_eps()))) {
		eventHandler.FeatureMoved(this, oldPos);
		return true;
	}

	return (moveCtrl.enabled);
}


bool CFeature::Update()
{
	bool continueUpdating = UpdatePosition();

	continueUpdating |= (smokeTime != 0);
	continueUpdating |= (fireTime != 0);
	continueUpdating |= (def->geoThermal);

	if (smokeTime != 0) {
		if (!((gs->frameNum + id) & 3) && projectileHandler->GetParticleSaturation() < 0.7f) {
			new CSmokeProjectile(NULL, midPos + gu->RandVector() * radius * 0.3f,
				gu->RandVector() * 0.3f + UpVector, smokeTime / 6 + 20, 6, 0.4f, 0.5f);
		}
	}
	if (fireTime == 1)
		featureHandler->DeleteFeature(this);

	if (def->geoThermal)
		EmitGeoSmoke();

	smokeTime = std::max(smokeTime - 1, 0);
	fireTime = std::max(fireTime - 1, 0);

	// return true so long as we need to stay in the FH update-queue
	return continueUpdating;
}


void CFeature::StartFire()
{
	if (fireTime != 0 || !def->burnable)
		return;

	fireTime = 200 + (int)(gs->randFloat() * GAME_SPEED);
	featureHandler->SetFeatureUpdateable(this);

	myFire = new CFireProjectile(midPos, UpVector, 0, 300, 70, radius * 0.8f, 20.0f);
}


void CFeature::EmitGeoSmoke()
{
	if ((gs->frameNum + id % 5) % 5 == 0) {
		// Find the unit closest to the geothermal
		const vector<CSolidObject*>& objs = quadField->GetSolidsExact(pos, 0.0f, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);
		float bestDist = std::numeric_limits<float>::max();

		CSolidObject* so = nullptr;

		for (CSolidObject* obj: objs) {
			const float dist = (obj->pos - pos).SqLength();

			if (dist < bestDist)  {
				bestDist = dist; so = obj;
			}
		}

		if (so != solidOnTop) {
			if (solidOnTop)
				DeleteDeathDependence(solidOnTop, DEPENDENCE_SOLIDONTOP);
			if (so)
				AddDeathDependence(so, DEPENDENCE_SOLIDONTOP);
		}

		solidOnTop = so;
	}

	// Hide the smoke if there is a geothermal unit on the vent
	const CUnit* u = dynamic_cast<CUnit*>(solidOnTop);

	if (u == NULL || !u->unitDef->needGeo) {
		const float partSat = !(gs->frameNum & 3) ? 1.0f : 0.7f;
		if (projectileHandler->GetParticleSaturation() < partSat) {
			const float3 pPos = gu->RandVector() * 10.0f + float3(pos.x, pos.y - 10.0f, pos.z);
			const float3 pSpeed = (gu->RandVector() * 0.5f) + (UpVector * 2.0f);

			new CGeoThermSmokeProjectile(pPos, pSpeed, int(50 + gu->RandFloat() * 7), this);
		}
	}
}


int CFeature::ChunkNumber(float f) { return int(math::ceil(f * modInfo.reclaimMethod)); }

// note: this is not actually used by GroundBlockingObjectMap anymore, just
// to distinguish unit and feature ID's (values >= MaxUnits() correspond to
// features in object commands)
int CFeature::GetBlockingMapID() const { return (id + unitHandler->MaxUnits()); }

