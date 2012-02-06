/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "Feature.h"
#include "FeatureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/QuadField.h"
#include "Rendering/Env/ITreeDrawer.h"
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
	// CR_MEMBER(model),
	CR_MEMBER(isRepairingBeforeResurrect),
	CR_MEMBER(resurrectProgress),
	CR_MEMBER(health),
	CR_MEMBER(reclaimLeft),
	CR_MEMBER(luaDraw),
	CR_MEMBER(noSelect),
	CR_MEMBER(tempNum),
	CR_MEMBER(lastReclaim),
	// CR_MEMBER(def),
	// CR_MEMBER(udef),
	CR_MEMBER(defName),
	CR_MEMBER(transMatrix),
	CR_MEMBER(inUpdateQue),
	CR_MEMBER(drawQuad),
	CR_MEMBER(finalHeight),
	CR_MEMBER(myFire),
	CR_MEMBER(fireTime),
	CR_MEMBER(emitSmokeTime),
	CR_RESERVED(64),
	CR_POSTLOAD(PostLoad)
));


CFeature::CFeature() : CSolidObject(),
	isRepairingBeforeResurrect(false),
	resurrectProgress(0.0f),
	health(0.0f),
	reclaimLeft(1.0f),
	luaDraw(false),
	noSelect(false),
	tempNum(0),
	lastReclaim(0),
	def(NULL),
	udef(NULL),
	inUpdateQue(false),
	drawQuad(-2),
	finalHeight(0.0f),
	reachedFinalPos(true),
	myFire(NULL),
	fireTime(0),
	emitSmokeTime(0),
	solidOnTop(NULL)
{
	crushable = true;
	immobile = true;

	physicalState = OnGround;
}

CFeature::~CFeature()
{
	if (blocking) {
		UnBlock();
	}

	qf->RemoveFeature(this);

	if (def->drawType >= DRAWTYPE_TREE && treeDrawer) {
		treeDrawer->DeleteTree(pos);
	}

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
	def = featureHandler->GetFeatureDef(defName);

	float fRadius = 1.0f;
	float fHeight = 0.0f;

	//FIXME is this really needed (aren't all those tags saved via creg?)
	if (def->drawType == DRAWTYPE_MODEL) {
		model = def->LoadModel();

		relMidPos = model->relMidPos;
		fRadius = model->radius;
		fHeight = model->height;
	} else if (def->drawType >= DRAWTYPE_TREE) {
		relMidPos = UpVector * TREE_RADIUS;
		fRadius = TREE_RADIUS;
		fHeight = fRadius * 2.0f;
	}

	SetRadiusAndHeight(fRadius, fHeight);
	UpdateMidPos();
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


void CFeature::Initialize(const float3& _pos, const FeatureDef* _def, short int _heading,
	int facing, int _team, int _allyteam, const UnitDef* _udef, const float3& speed, int _smokeTime)
{
	def = _def;
	udef = _udef;
	defName = def->myName;
	heading = _heading;
	buildFacing = facing;
	team = _team;
	allyteam = _allyteam;
	emitSmokeTime = _smokeTime;

	mass = def->mass;
	crushResistance = def->crushResistance;

	health   = def->maxHealth;
	blocking = def->blocking;

	xsize    = ((facing & 1) == 0) ? def->xsize : def->zsize;
	zsize    = ((facing & 1) == 1) ? def->xsize : def->zsize;

	noSelect = def->noSelect;

	float fRadius = 1.0f;
	float fHeight = 0.0f;

	if (def->drawType == DRAWTYPE_MODEL) {
		model = def->LoadModel();

		if (!model) {
			LOG_L(L_ERROR, "Features: Couldn't load model for %s", defName.c_str());
		} else {
			relMidPos = model->relMidPos;
			fRadius = model->radius;
			fHeight = model->height;

			// note: gets deleted in ~CSolidObject
			collisionVolume = new CollisionVolume(def->collisionVolume, fRadius);
		}
	}
	else if (def->drawType >= DRAWTYPE_TREE) {
		relMidPos = UpVector * TREE_RADIUS;
		fRadius = TREE_RADIUS;
		fHeight = fRadius * 2.0f;

		// LoadFeaturesFromMap() doesn't set a scale for trees
		// note: gets deleted in ~CSolidObject
		collisionVolume = new CollisionVolume(def->collisionVolume, fRadius);
	}

	Move3D(_pos.cClampInMap(), false);
	SetRadiusAndHeight(fRadius, fHeight);
	UpdateMidPos();
	CalculateTransform();

	featureHandler->AddFeature(this);
	qf->AddFeature(this);

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

	if (speed != ZeroVector) {
		deathSpeed = speed;
	}

	reachedFinalPos = (speed == ZeroVector && pos.y == finalHeight);
}


void CFeature::CalculateTransform()
{
	float3 frontDir = GetVectorFromHeading(heading);
	float3 upDir;

	if (def->upright) upDir = float3(0.0f, 1.0f, 0.0f);
	else upDir = ground->GetNormal(pos.x, pos.z);

	float3 rightDir = frontDir.cross(upDir);
	rightDir.Normalize();
	frontDir = upDir.cross(rightDir);
	frontDir.Normalize ();

	transMatrix = CMatrix44f(pos, -rightDir, upDir, frontDir);
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
	}
	else { // Reclaiming
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


void CFeature::DoDamage(const DamageArray& damages, const float3& impulse)
{
	if (damages.paralyzeDamageTime) {
		return; // paralyzers do not damage features
	}

	residualImpulse = impulse;
	health -= damages[0];

	if (health <= 0 && def->destructable) {
		CFeature* deathFeature = featureHandler->CreateWreckage(
			pos, def->deathFeature, heading,
			buildFacing, 1, team, -1, false, NULL
		);

		if (deathFeature) {
			// if a partially reclaimed corpse got blasted,
			// ensure its wreck is not worth the full amount
			// (which might be more than the amount remaining)
			deathFeature->reclaimLeft = reclaimLeft;
		}

		featureHandler->DeleteFeature(this);
		blockHeightChanges = false;

		if (def->drawType >= DRAWTYPE_TREE) {
			if (impulse.SqLength2D() > 0.25f) {
				treeDrawer->AddFallingTree(pos, impulse, def->drawType - 1);
			}
		}
	}
}


void CFeature::Kill(const float3& impulse, bool crushKill) {
	crushKilled = crushKill;

	DamageArray damage;
	DoDamage(damage * (health + 1.0f), impulse);
}


void CFeature::DependentDied(CObject *o)
{
	if (o == solidOnTop)
		solidOnTop = 0;

	CSolidObject::DependentDied(o);
}


void CFeature::ForcedMove(const float3& newPos, bool snapToGround)
{
	if (blocking) {
		UnBlock();
	}

	// remove from managers
	qf->RemoveFeature(this);
	if (def->drawType >= DRAWTYPE_TREE) {
		treeDrawer->DeleteTree(pos);
	}

	Move3D(pos, false);
	eventHandler.FeatureMoved(this);

	// setup finalHeight
	if (snapToGround) {
		if (def->floating) {
			finalHeight = ground->GetHeightAboveWater(pos.x, pos.z);
		} else {
			finalHeight = ground->GetHeightReal(pos.x, pos.z);
		}
	} else {
		finalHeight = newPos.y;
	}

	// setup the visual transformation matrix
	CalculateTransform();

	// insert into managers
	qf->AddFeature(this);
	if (def->drawType >= DRAWTYPE_TREE) {
		treeDrawer->AddTree(def->drawType - 1, pos, 1.0f);
	}

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
	if (udef != NULL) {
		// we are a wreck of a dead unit
		if (!reachedFinalPos) {
			// NOTE: apply more drag if we were a tank or bot?
			// (would require passing extra data to Initialize())
			deathSpeed *= 0.95f;

			if (deathSpeed.SqLength2D() > 0.01f) {
				UnBlock();
				qf->RemoveFeature(this);

				// update our forward speed (and quadfield
				// position) if it is still greater than 0
				Move3D(deathSpeed, true);

				qf->AddFeature(this);
				Block();
			} else {
				deathSpeed.x = 0.0f;
				deathSpeed.z = 0.0f;
			}

			// def->floating is unreliable (true for land unit wrecks),
			// just assume wrecks always sink even if their "owner" was
			// a floating object (as is the case for ships anyway)
			const float realGroundHeight = ground->GetHeightReal(pos.x, pos.z);
			const bool reachedGround = (pos.y <= realGroundHeight);

			if (!reachedGround) {
				if (pos.y > 0.0f) {
					// quadratic acceleration if not in water
					deathSpeed.y += mapInfo->map.gravity;
				} else {
					// constant downward speed otherwise
					deathSpeed.y = mapInfo->map.gravity;
				}

				Move1D(deathSpeed.y, 1, true);
			} else {
				deathSpeed.y = 0.0f;

				// last Update() may have sunk us into
				// ground if pos.y was only marginally
				// larger than ground height, correct
				Move1D(realGroundHeight, 1, false);
			}

			reachedFinalPos = (deathSpeed == ZeroVector);

			if (!pos.IsInBounds()) {
				pos.ClampInBounds();
				// ensure that no more forward-speed updates are done
				// (prevents wrecks floating in mid-air at edge of map
				// due to gravity no longer being applied either)
				deathSpeed = ZeroVector;
			}

			eventHandler.FeatureMoved(this);
			CalculateTransform();
		}
	} else {
		if (pos.y > finalHeight) {
			// feature is falling (note: gravity is negative)
			if (def->drawType >= DRAWTYPE_TREE) {
				treeDrawer->DeleteTree(pos);
			}

			if (pos.y > 0.0f) {
				speed.y += mapInfo->map.gravity;
			} else {
				// fall slower in water
				speed.y += mapInfo->map.gravity * 0.5;
			}

			transMatrix[13] += speed.y;
			Move1D(speed.y, 1, true);

			if (def->drawType >= DRAWTYPE_TREE) {
				treeDrawer->AddTree(def->drawType - 1, pos, 1.0f);
			}
		} else if (pos.y < finalHeight) {
			// if ground is restored, make sure feature does not get buried
			if (def->drawType >= DRAWTYPE_TREE) {
				treeDrawer->DeleteTree(pos);
			}

			const float dy = finalHeight - pos.y;

			speed.y = 0.0f;
			transMatrix[13] += dy;

			Move1D(dy, 1, true);

			if (def->drawType >= DRAWTYPE_TREE) {
				treeDrawer->AddTree(def->drawType - 1, pos, 1.0f);
			}
		}

		reachedFinalPos = (pos.y == finalHeight);
	}

	isUnderWater = ((pos.y + height) < 0.0f);
	return reachedFinalPos;
}

bool CFeature::Update()
{
	bool finishedUpdate = UpdatePosition();

	if (emitSmokeTime != 0) {
		--emitSmokeTime;
		if (!((gs->frameNum + id) & 3) && ph->particleSaturation < 0.7f) {
			new CSmokeProjectile(midPos + gu->usRandVector() * radius * 0.3f,
				gu->usRandVector() * 0.3f + UpVector, emitSmokeTime / 6 + 20, 6, 0.4f, 0, 0.5f);
		}
		if (emitSmokeTime > 0)
			finishedUpdate = false;
	}

	if (fireTime > 0) {
		fireTime--;
		if (fireTime == 1)
			featureHandler->DeleteFeature(this);
		finishedUpdate = false;
	}

	if (def->geoThermal) {
		if ((gs->frameNum + id % 5) % 5 == 0) {
			// Find the unit closest to the geothermal
			const vector<CSolidObject*> &objs = qf->GetSolidsExact(pos, 0.0f);
			float bestDist2 = 0;
			CSolidObject* so = NULL;

			for (vector<CSolidObject*>::const_iterator oi = objs.begin(); oi != objs.end(); ++oi) {
				float dist2 = ((*oi)->pos - pos).SqLength();
				if (!so || dist2 < bestDist2)  {
					bestDist2 = dist2;
					so = *oi;
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
		CUnit *u = dynamic_cast<CUnit*>(solidOnTop);
		if (!u || !u->unitDef->needGeo) {
			if ((ph->particleSaturation < 0.7f) || (ph->particleSaturation < 1 && !(gs->frameNum & 3))) {
				float3 speed = gu->usRandVector() * 0.5f;
				speed.y += 2.0f;

				new CGeoThermSmokeProjectile(gu->usRandVector() * 10 +
					float3(pos.x, pos.y-10, pos.z), speed, int(50 + gu->usRandFloat() * 7), this);
			}
		}

		finishedUpdate = false;
	}

	return !finishedUpdate;
}


void CFeature::StartFire()
{
	if (fireTime || !def->burnable)
		return;

	fireTime = 200 + (int)(gs->randFloat() * 30);
	featureHandler->SetFeatureUpdateable(this);

	myFire = new CFireProjectile(midPos, UpVector, 0, 300, radius * 0.8f, 70, 20);
}

int CFeature::ChunkNumber(float f)
{
	return (int) ceil(f * modInfo.reclaimMethod);
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
	float chunksLeft = ceil(reclaimLeft * modInfo.reclaimMethod);
	return chunkSize * chunksLeft;
}


float CFeature::RemainingMetal() const
{
	return RemainingResource(def->metal);
}


float CFeature::RemainingEnergy() const
{
	return RemainingResource(def->energy);
}
