/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Feature.h"
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
#include "Sim/Units/UnitHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include <assert.h>

CR_BIND_DERIVED(CFeature, CSolidObject, )

CR_REG_METADATA(CFeature, (
	CR_MEMBER(defID),
	CR_MEMBER(isRepairingBeforeResurrect),
	CR_MEMBER(isAtFinalHeight),
	CR_MEMBER(inUpdateQue),
	CR_MEMBER(deleteMe),
	CR_MEMBER(resurrectProgress),
	CR_MEMBER(reclaimLeft),
	CR_MEMBER(resources),
	CR_MEMBER(finalHeight),
	CR_MEMBER(lastReclaim),
	CR_MEMBER(drawQuad),
	CR_MEMBER(drawFlag),
	CR_MEMBER(drawAlpha),
	CR_MEMBER(alphaFade),
	CR_MEMBER(fireTime),
	CR_MEMBER(smokeTime),
	CR_MEMBER(fireTime),
	CR_IGNORED(def), //reconstructed in PostLoad
	CR_MEMBER(udef),
	CR_MEMBER(myFire),
	CR_MEMBER(solidOnTop),
	CR_MEMBER(transMatrix),
	CR_POSTLOAD(PostLoad)
))


CFeature::CFeature()
: CSolidObject()
, defID(-1)
, isRepairingBeforeResurrect(false)
, isAtFinalHeight(false)
, inUpdateQue(false)
, deleteMe(false)
, finalHeight(0.0f)
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
	def = featureHandler->GetFeatureDefByID(defID);
	objectDef = def;

	// FIXME is this really needed (aren't all those tags saved via creg?)
	switch (def->drawType) {
		case DRAWTYPE_NONE: {
		} break;

		case DRAWTYPE_MODEL: {
			if ((model = def->LoadModel()) != nullptr) {
				SetMidAndAimPos(model->relMidPos, model->relMidPos, true);
				SetRadiusAndHeight(model);

				localModel.SetModel(model);
			}
		} break;

		default: {
			// always >= DRAWTYPE_TREE here
			SetMidAndAimPos(UpVector * TREE_RADIUS, UpVector * TREE_RADIUS, true);
			SetRadiusAndHeight(TREE_RADIUS, TREE_RADIUS * 2.0f);
		} break;
	}

	UpdateMidAndAimPos();
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
	if (alwaysVisible)
		return true;

	const bool inLOS = (argAllyTeam == -1 || losHandler->InLos(this->pos, argAllyTeam));

	switch (modInfo.featureVisibility) {
		case CModInfo::FEATURELOS_NONE:
		default:
			return inLOS;

		// these next two only make sense when Gaia is enabled
		case CModInfo::FEATURELOS_GAIAONLY:
			return (this->allyteam == std::max(0, teamHandler->GaiaAllyTeamID()) || inLOS);
		case CModInfo::FEATURELOS_GAIAALLIED:
			return (this->allyteam == std::max(0, teamHandler->GaiaAllyTeamID()) || this->allyteam == argAllyTeam || inLOS);

		case CModInfo::FEATURELOS_ALL:
			return true;
	}
}


void CFeature::Initialize(const FeatureLoadParams& params)
{
	id = params.featureID;
	defID = (params.featureDef)->id;

	def = params.featureDef;
	udef = params.unitDef;
	objectDef = params.featureDef;

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
		} break;
	}

	UpdateMidAndAimPos();
	UpdateFinalHeight(true);
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
		if (udef == NULL)
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
		} else {
			// update the energy and metal required counts
			teamHandler->Team(builder->team)->resPull.energy += energyUse;
			teamHandler->Team(builder->team)->resPull.metal  += metalUse;
		}
		return false;
	} else {
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

		if (!builder->IssueResourceOrder(&order)) {
			return false;
		}

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

	// Should never get here
	assert(false);
	return false;
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

	if (eventHandler.FeaturePreDamaged(this, attacker, baseDamage, weaponDefID, projectileID, &baseDamage, &impulseMult)) {
		return;
	}

	// NOTE:
	//   for trees, impulse is used to drive their falling animation
	//   this also calls our SetVel() to put us in the update queue
	ApplyImpulse((impulse * impulseMult) / mass);

	// clamp in case Lua-modified damage is negative
	health -= baseDamage;
	health = std::min(health, def->health);

	eventHandler.FeatureDamaged(this, attacker, baseDamage, weaponDefID, projectileID);

	if (health <= 0.0f && def->destructable) {
		FeatureLoadParams params = {featureHandler->GetFeatureDefByID(def->deathFeatureDefID), NULL, pos, ZeroVector, -1, team, -1, heading, buildFacing, 0};
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
	CWorldObject::SetVelocity(v);
	UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, v != ZeroVector);

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
	UpdateTransform();
}


void CFeature::UpdateTransformAndPhysState()
{
	UpdateDirVectors(!def->upright);
	UpdateTransform();

	UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, ((SetSpeed(speed) != 0.0f) || (std::fabs(pos.y - finalHeight) >= 0.01f)));
	UpdatePhysicalState(0.1f);
}

bool CFeature::UpdatePosition()
{
	const float3 oldPos = pos;

	if (udef != NULL) {
		// we are a wreck of a dead unit, possibly with residual impulse
		// in this case we do not care about <finalHeight> and are always
		// affected by gravity
		// note that def->floating is unreliable (eg. it can be true for
		// ground-unit wrecks), so just assume wrecks always sink in water
		// even if their "owner" was a floating object (as is the case for
		// ships anyway)
		if (IsMoving()) {
			const float realGroundHeight = CGround::GetHeightReal(pos.x, pos.z);
			const bool reachedWater  = ( pos.y                     <= 0.1f);
			const bool reachedGround = ((pos.y - realGroundHeight) <= 0.1f);

			// NOTE:
			//   all these calls use the base-class because FeatureHandler::Update
			//   iterates over updateFeatures and our ::SetVelocity will insert us
			//   into that
			CWorldObject::SetVelocity(speed + GetDragAccelerationVec(float4(mapInfo->atmosphere.fluidDensity, mapInfo->water.fluidDensity, 1.0f, 0.1f)));

			if (speed.SqLength2D() > 0.01f) {
				//hint: only this updates horizontal position all other
				//   lines in this function update vertical speed only!

				quadField->RemoveFeature(this);
				UnBlock();

				// update our forward speed (and quadfield
				// position) if it is still greater than 0
				Move(speed, true);

				Block();
				quadField->AddFeature(this);
			} else {
				CWorldObject::SetVelocity(speed * UpVector);
			}

			if (!reachedGround) {
				if (!reachedWater) {
					// quadratic acceleration if not in water
					CWorldObject::SetVelocity(speed + (UpVector * mapInfo->map.gravity));
				} else {
					// constant downward speed otherwise
					CWorldObject::SetVelocity((speed * XZVector) + (UpVector * mapInfo->map.gravity));
				}

				Move(UpVector * speed.y, true);
			} else {
				CWorldObject::SetVelocity(speed * XZVector);

				// last Update() may have sunk us into
				// ground if pos.y was only marginally
				// larger than ground height, correct
				Move(UpVector * (realGroundHeight - pos.y), true);
			}

			if (!pos.IsInBounds()) {
				pos.ClampInBounds();

				// ensure that no more forward-speed updates are done
				// (prevents wrecks floating in mid-air at edge of map
				// due to gravity no longer being applied either)
				CWorldObject::SetVelocity(ZeroVector);
			}

			eventHandler.FeatureMoved(this, oldPos);
		}
	} else {
		// any feature that is not a dead unit (ie. rocks, trees, ...)
		// these never move in the xz-plane no matter how much impulse
		// is applied, only gravity affects them (FIXME: arbitrary..?)
		if (pos.y > finalHeight) {
			if (pos.y > 0.0f) {
				CWorldObject::SetVelocity(speed + (UpVector * mapInfo->map.gravity));
			} else {
				CWorldObject::SetVelocity((speed * XZVector) + (UpVector * mapInfo->map.gravity));
			}

			// stop falling when we reach our finalHeight
			// (which can be arbitrary, even below ground)
			Move(UpVector * std::min(pos.y - finalHeight, speed.y), true);
			eventHandler.FeatureMoved(this, oldPos);
		} else if (pos.y < finalHeight) {
			// stop vertical movement and teleport up
			CWorldObject::SetVelocity(speed * XZVector);

			Move(UpVector * (finalHeight - pos.y), true);
			eventHandler.FeatureMoved(this, oldPos);
		}
	}

	UpdateTransformAndPhysState();
	Block(); // does the check if wanted itself

	return (IsMoving());
}


void CFeature::UpdateFinalHeight(bool useGroundHeight)
{
	if (isAtFinalHeight)
		return;

	if (useGroundHeight) {
		if (def->floating) {
			finalHeight = CGround::GetHeightAboveWater(pos.x, pos.z);
		} else {
			finalHeight = CGround::GetHeightReal(pos.x, pos.z);
		}
	} else {
		// permanently stay at this height,
		// even if terrain changes under us
		// later
		isAtFinalHeight = true;
		finalHeight = pos.y;
	}
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
	if (fireTime == 1) {
		featureHandler->DeleteFeature(this);
	}
	if (def->geoThermal) {
		EmitGeoSmoke();
	}

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

		CSolidObject* so = NULL;

		for (vector<CSolidObject*>::const_iterator oi = objs.begin(); oi != objs.end(); ++oi) {
			const float dist = ((*oi)->pos - pos).SqLength();

			if (dist < bestDist)  {
				bestDist = dist; so = *oi;
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

