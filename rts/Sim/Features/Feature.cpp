/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Feature.h"
#include "FeatureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/QuadField.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/FireProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/GeoThermSmokeProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
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
	CR_MEMBER(resurrectProgress),
	CR_MEMBER(reclaimLeft),
	CR_MEMBER(finalHeight),
	CR_MEMBER(tempNum),
	CR_MEMBER(lastReclaim),
	CR_MEMBER(drawQuad),
	CR_MEMBER(fireTime),
	CR_MEMBER(smokeTime),
	CR_MEMBER(fireTime),
	CR_IGNORED(def), //reconstructed in PostLoad
	CR_MEMBER(udef),
	CR_MEMBER(myFire),
	CR_MEMBER(solidOnTop),
	CR_MEMBER(transMatrix),
	CR_POSTLOAD(PostLoad)
));


CFeature::CFeature() : CSolidObject(),
	defID(-1),
	isRepairingBeforeResurrect(false),
	isAtFinalHeight(false),
	inUpdateQue(false),
	resurrectProgress(0.0f),
	reclaimLeft(1.0f),
	finalHeight(0.0f),
	tempNum(0),
	lastReclaim(0),
	drawQuad(-2),
	fireTime(0),
	smokeTime(0),

	def(NULL),
	udef(NULL),

	myFire(NULL),
	solidOnTop(NULL)
{
	crushable = true;
	immobile = true;
}

CFeature::~CFeature()
{
	if (blocking) {
		UnBlock();
	}

	quadField->RemoveFeature(this);

	if (myFire) {
		myFire->StopFire();
		myFire = 0;
	}

	if (def->geoThermal) {
		CGeoThermSmokeProjectile::GeoThermDestroyed(this);
	}
}

void CFeature::PostLoad()
{
	def = featureHandler->GetFeatureDefByID(defID);
	objectDef = def;

	//FIXME is this really needed (aren't all those tags saved via creg?)
	if (def->drawType == DRAWTYPE_MODEL) {
		model = def->LoadModel();

		SetMidAndAimPos(model->relMidPos, model->relMidPos, true);
		SetRadiusAndHeight(model);
	} else if (def->drawType >= DRAWTYPE_TREE) {
		SetMidAndAimPos(UpVector * TREE_RADIUS, UpVector * TREE_RADIUS, true);
		SetRadiusAndHeight(TREE_RADIUS, TREE_RADIUS * 2.0f);
	}

	UpdateMidAndAimPos();
}


void CFeature::ChangeTeam(int newTeam)
{
	if (newTeam < 0) {
		team = 0; // NOTE: this should probably be -1, would need work
		allyteam = -1;
	} else {
		team = newTeam;
		allyteam = teamHandler->AllyTeam(newTeam);
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
	crushResistance = def->crushResistance;

	health   = def->health;
	blocking = def->blocking;

	xsize = ((buildFacing & 1) == 0) ? def->xsize : def->zsize;
	zsize = ((buildFacing & 1) == 1) ? def->xsize : def->zsize;

	noSelect = def->noSelect;

	// set position before mid-position
	Move3D((params.pos).cClampInMap(), false);

	if (def->drawType == DRAWTYPE_MODEL) {
		if ((model = def->LoadModel()) == NULL) {
			LOG_L(L_ERROR, "Features: Couldn't load model for %s", def->name.c_str());
		} else {
			SetMidAndAimPos(model->relMidPos, model->relMidPos, true);
			SetRadiusAndHeight(model);
		}
	} else {
		if (def->drawType >= DRAWTYPE_TREE) {
			// LoadFeaturesFromMap() doesn't set a scale for trees
			SetMidAndAimPos(UpVector * TREE_RADIUS, UpVector * TREE_RADIUS, true);
			SetRadiusAndHeight(TREE_RADIUS, TREE_RADIUS * 2.0f);
		}
	}

	UpdateMidAndAimPos();
	CalculateTransform();

	// note: gets deleted in ~CSolidObject
	collisionVolume = new CollisionVolume(def->collisionVolume);

	if (collisionVolume->DefaultToSphere())
		collisionVolume->InitSphere(radius);
	if (collisionVolume->DefaultToFootPrint())
		collisionVolume->InitBox(float3(xsize * SQUARE_SIZE, height, zsize * SQUARE_SIZE));

	featureHandler->AddFeature(this);
	quadField->AddFeature(this);

	// maybe should not be here, but it prevents crashes caused by team = -1
	ChangeTeam(team);

	if (blocking) {
		Block();
	}

	if (def->floating) {
		finalHeight = ground->GetHeightAboveWater(pos.x, pos.z);
	} else {
		finalHeight = ground->GetHeightReal(pos.x, pos.z);
	}

	speed = params.speed;
	isMoving = ((speed != ZeroVector) || (std::fabs(pos.y - finalHeight) >= 0.01f));
}


void CFeature::CalculateTransform()
{
	updir    = (!def->upright)? ground->GetNormal(pos.x, pos.z): UpVector;
	frontdir = GetVectorFromHeading(heading);
	rightdir = (frontdir.cross(updir)).Normalize();
	frontdir = (updir.cross(rightdir)).Normalize();

	transMatrix = CMatrix44f(pos, -rightdir, updir, frontdir);
}


bool CFeature::AddBuildPower(float amount, CUnit* builder)
{
	const float oldReclaimLeft = reclaimLeft;

	if (amount > 0.0f) {
		// Check they are trying to repair a feature that can be resurrected
		if (udef == NULL) {
			return false;
		}

		// 'Repairing' previously-sucked features prior to resurrection
		// This is reclaim-option independant - repairing features should always
		// be like other repairing - gradual and multi-unit
		// Lots of this code is stolen from unit->AddBuildPower

		isRepairingBeforeResurrect = true; // Stop them exploiting chunk reclaiming

		if (reclaimLeft >= 1) {
			return false; // cant repair a 'fresh' feature
		}

		if (reclaimLeft <= 0) {
			return false; // feature most likely has been deleted
		}

		// Work out how much to try to put back, based on the speed this unit would reclaim at.
		const float part = amount / def->reclaimTime;

		// Work out how much that will cost
		const float metalUse  = part * def->metal;
		const float energyUse = part * def->energy;
		if ((teamHandler->Team(builder->team)->metal  >= metalUse)  &&
		    (teamHandler->Team(builder->team)->energy >= energyUse) &&
				(!luaRules || luaRules->AllowFeatureBuildStep(builder, this, part))) {
			builder->UseMetal(metalUse);
			builder->UseEnergy(energyUse);
			reclaimLeft+=part;
			if (reclaimLeft >= 1) {
				isRepairingBeforeResurrect = false; // They can start reclaiming it again if they so wish
				reclaimLeft = 1;
			} else if (reclaimLeft <= 0) {
				// this can happen when a mod tampers the feature in AllowFeatureBuildStep
				featureHandler->DeleteFeature(this);
				return false;
			}
			return true;
		}
		else {
			// update the energy and metal required counts
			teamHandler->Team(builder->team)->energyPull += energyUse;
			teamHandler->Team(builder->team)->metalPull  += metalUse;
		}
		return false;
	} else { // Reclaiming
		// avoid multisuck when reclaim has already completed during this frame
		if (reclaimLeft <= 0) {
			return false;
		}

		// don't let them exploit chunk reclaim
		if (isRepairingBeforeResurrect && (modInfo.reclaimMethod > 1)) {
			return false;
		}

		// make sure several units cant reclaim at once on a single feature
		if ((modInfo.multiReclaim == 0) && (lastReclaim == gs->frameNum)) {
			return true;
		}

		const float part = (-amount) / def->reclaimTime;

		if (luaRules && !luaRules->AllowFeatureBuildStep(builder, this, -part)) {
			return false;
		}

		float reclaimLeftTemp = reclaimLeft - part;
		// stop the last bit giving too much resource
		if (reclaimLeftTemp < 0) {
			reclaimLeftTemp = 0;
		}

		const float fractionReclaimed = oldReclaimLeft - reclaimLeftTemp;
		const float metalFraction = def->metal * fractionReclaimed;
		const float energyFraction = def->energy * fractionReclaimed;
		const float energyUseScaled = metalFraction * modInfo.reclaimFeatureEnergyCostFactor;

		if (!builder->UseEnergy(energyUseScaled)) {
			teamHandler->Team(builder->team)->energyPull += energyUseScaled;
			return false;
		}

		reclaimLeft = reclaimLeftTemp;

		if ((modInfo.reclaimMethod == 1) && (reclaimLeft == 0)) {
			// All-at-end method
			builder->AddMetal(def->metal, false);
			builder->AddEnergy(def->energy, false);
		}
		else if (modInfo.reclaimMethod == 0) {
			// Gradual reclaim
			builder->AddMetal(metalFraction, false);
			builder->AddEnergy(energyFraction, false);
		}
		else {
			// Chunky reclaiming, work out how many chunk boundaries we crossed
			const float chunkSize = 1.0f / modInfo.reclaimMethod;
			const int oldChunk = ChunkNumber(oldReclaimLeft);
			const int newChunk = ChunkNumber(reclaimLeft);
			if (oldChunk != newChunk) {
				const float noChunks = (float)oldChunk - (float)newChunk;
				builder->AddMetal(noChunks * def->metal * chunkSize, false);
				builder->AddEnergy(noChunks * def->energy * chunkSize, false);
			}
		}

		// Has the reclaim finished?
		if (reclaimLeft <= 0) {
			featureHandler->DeleteFeature(this);
			return false;
		}

		lastReclaim = gs->frameNum;
		return true;
	}

	// Should never get here
	assert(false);
	return false;
}


void CFeature::DoDamage(const DamageArray& damages, const float3& impulse, CUnit*, int, int)
{
	if (damages.paralyzeDamageTime) {
		return; // paralyzers do not damage features
	}

	// NOTE: for trees, impulse is used to drive their falling animation
	// TODO: replace RHS condition by adding Feature{Pre}Damaged callins
	if ((def->drawType >= DRAWTYPE_TREE) || (udef != NULL && !udef->IsImmobileUnit())) {
		StoreImpulse(impulse / mass);
		ApplyImpulse();
	}

	if (impulse != ZeroVector) {
		featureHandler->SetFeatureUpdateable(this);
	}

	if ((health -= damages[0]) <= 0.0f && def->destructable) {
		FeatureLoadParams params = {featureHandler->GetFeatureDefByID(def->deathFeatureDefID), NULL, pos, ZeroVector, -1, team, -1, heading, buildFacing, 0};
		CFeature* deathFeature = featureHandler->CreateWreckage(params, 0, false);

		if (deathFeature != NULL) {
			// if a partially reclaimed corpse got blasted,
			// ensure its wreck is not worth the full amount
			// (which might be more than the amount remaining)
			deathFeature->reclaimLeft = reclaimLeft;
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


void CFeature::ForcedMove(const float3& newPos)
{
	if (blocking) {
		UnBlock();
	}

	// remove from managers
	quadField->RemoveFeature(this);

	const float3 oldPos = pos;

	Move3D(newPos - pos, true);
	eventHandler.FeatureMoved(this, oldPos);

	// setup the visual transformation matrix
	CalculateTransform();

	// insert into managers
	quadField->AddFeature(this);

	if (blocking) {
		Block();
	}
}

void CFeature::ForcedSpin(const float3& newDir)
{
	float3 updir = UpVector;
	if (updir == newDir) {
		//FIXME perhaps save the old right,up,front directions, so we can
		// reconstruct the old upvector and generate a better assumption for updir
		updir -= GetVectorFromHeading(heading);
	}
	float3 rightdir = newDir.cross(updir).Normalize();
	updir = rightdir.cross(newDir);
	transMatrix = CMatrix44f(pos, -rightdir, updir, newDir);
	heading = GetHeadingFromVector(newDir.x, newDir.z);
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
		if (isMoving) {
			const float realGroundHeight = ground->GetHeightReal(pos.x, pos.z);
			const bool reachedWater  = ( pos.y                     <= 0.1f);
			const bool reachedGround = ((pos.y - realGroundHeight) <= 0.1f);

			speed *= 0.999999f;
			speed *= (1.0f - (int(reachedWater ) * 0.05f));
			speed *= (1.0f - (int(reachedGround) * 0.10f));

			if (speed.SqLength2D() > 0.01f) {
				UnBlock();
				quadField->RemoveFeature(this);

				// update our forward speed (and quadfield
				// position) if it is still greater than 0
				Move3D(speed, true);

				quadField->AddFeature(this);
				Block();
			} else {
				speed.x = 0.0f;
				speed.z = 0.0f;
			}

			if (!reachedGround) {
				if (!reachedWater) {
					// quadratic acceleration if not in water
					speed.y += mapInfo->map.gravity;
				} else {
					// constant downward speed otherwise
					speed.y = mapInfo->map.gravity;
				}

				Move1D(speed.y, 1, true);
			} else {
				speed.y = 0.0f;

				// last Update() may have sunk us into
				// ground if pos.y was only marginally
				// larger than ground height, correct
				Move1D(realGroundHeight, 1, false);
			}

			if (!pos.IsInBounds()) {
				pos.ClampInBounds();

				// ensure that no more forward-speed updates are done
				// (prevents wrecks floating in mid-air at edge of map
				// due to gravity no longer being applied either)
				speed = ZeroVector;
			}

			eventHandler.FeatureMoved(this, oldPos);
			CalculateTransform();
		}
	} else {
		// any feature that is not a dead unit (ie. rocks, trees, ...)
		// these never move in the xz-plane no matter how much impulse
		// is applied, only gravity affects them
		if (pos.y > finalHeight) {
			if (pos.y > 0.0f) {
				speed.y += mapInfo->map.gravity;
			} else {
				speed.y = mapInfo->map.gravity;
			}

			// stop falling when we reach our finalHeight
			// (which can be arbitrary, even below ground)
			Move1D(std::min(pos.y - finalHeight, speed.y), 1, true);
			eventHandler.FeatureMoved(this, oldPos);
		} else if (pos.y < finalHeight) {
			// stop vertical movement and teleport up
			speed.y = 0.0f;

			Move1D((finalHeight - pos.y), 1, true);
			eventHandler.FeatureMoved(this, oldPos);
		}

		transMatrix[13] = pos.y;
	}

	residualImpulse *= impulseDecayRate;
	isMoving = ((speed != ZeroVector) || (std::fabs(pos.y - finalHeight) >= 0.01f));

	UpdatePhysicalState();

	return isMoving;
}

void CFeature::UpdateFinalHeight(bool useGroundHeight)
{
	if (isAtFinalHeight)
		return;

	if (useGroundHeight) {
		if (def->floating) {
			finalHeight = ground->GetHeightAboveWater(pos.x, pos.z);
		} else {
			finalHeight = ground->GetHeightReal(pos.x, pos.z);
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
		if (!((gs->frameNum + id) & 3) && projectileHandler->particleSaturation < 0.7f) {
			new CSmokeProjectile(midPos + gu->RandVector() * radius * 0.3f,
				gu->RandVector() * 0.3f + UpVector, smokeTime / 6 + 20, 6, 0.4f, 0, 0.5f);
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

	myFire = new CFireProjectile(midPos, UpVector, 0, 300, radius * 0.8f, 70, 20);
}
void CFeature::EmitGeoSmoke()
{
	if ((gs->frameNum + id % 5) % 5 == 0) {
		// Find the unit closest to the geothermal
		const vector<CSolidObject*>& objs = quadField->GetSolidsExact(pos, 0.0f);
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
		if ((projectileHandler->particleSaturation < 0.7f) || (projectileHandler->particleSaturation < 1 && !(gs->frameNum & 3))) {
			const float3 pPos = gu->RandVector() * 10.0f + float3(pos.x, pos.y - 10.0f, pos.z);
			const float3 pSpeed = (gu->RandVector() * 0.5f) + (UpVector * 2.0f);

			new CGeoThermSmokeProjectile(pPos, pSpeed, int(50 + gu->RandFloat() * 7), this);
		}
	}
}



float CFeature::RemainingResource(float res) const
{
	// Gradual reclaim
	if (modInfo.reclaimMethod == 0) {
		return res * reclaimLeft;
	}

	// Old style - all reclaimed at the end
	if (modInfo.reclaimMethod == 1) {
		return res;
	}

	// Otherwise we are doing chunk reclaiming
	float chunkSize = res / modInfo.reclaimMethod; // resource/no_chunks
	float chunksLeft = math::ceil(reclaimLeft * modInfo.reclaimMethod);
	return chunkSize * chunksLeft;
}

float CFeature::RemainingMetal() const { return RemainingResource(def->metal); }
float CFeature::RemainingEnergy() const { return RemainingResource(def->energy); }
int CFeature::ChunkNumber(float f) const { return int(math::ceil(f * modInfo.reclaimMethod)); }

