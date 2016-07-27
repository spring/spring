/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameHelper.h"

#include "Camera.h"
#include "GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaUI.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/BuildingMaskMap.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/SyncTracer.h"

#define NUM_WAITING_DAMAGE_LISTS 128

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGameHelper* helper;


CGameHelper::CGameHelper()
{
	waitingDamageLists.resize(NUM_WAITING_DAMAGE_LISTS);
}

CGameHelper::~CGameHelper()
{
	waitingDamageLists.clear();
}



//////////////////////////////////////////////////////////////////////
// Explosions/Damage
//////////////////////////////////////////////////////////////////////

float CGameHelper::CalcImpulseScale(const DamageArray& damages, const float expDistanceMod)
{
	// limit the impulse to prevent later FP overflow
	// (several weapons have _default_ damage values in the order of 1e4,
	// which make the simulation highly unstable because they can impart
	// speeds of several thousand elmos/frame to units and throw them far
	// outside the map)
	// DamageArray::operator* scales damage multipliers,
	// not the impulse factor --> need to scale manually
	// by it for impulse
	const float impulseDmgMult = (damages.GetDefault() + damages.impulseBoost);
	const float rawImpulseScale = damages.impulseFactor * expDistanceMod * impulseDmgMult;

	return Clamp(rawImpulseScale, -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE);
}

void CGameHelper::DoExplosionDamage(
	CUnit* unit,
	CUnit* owner,
	const float3& expPos,
	const float expRadius,
	const float expSpeed,
	const float expEdgeEffect,
	const bool ignoreOwner,
	const DamageArray& damages,
	const int weaponDefID,
	const int projectileID
) {
	assert(unit != NULL);

	if (ignoreOwner && (unit == owner))
		return;

	const LocalModelPiece* lhp = unit->GetLastHitPiece(gs->frameNum);
	const CollisionVolume* vol = unit->GetCollisionVolume(lhp);

	const float3& lhpPos = (lhp != NULL && vol == lhp->GetCollisionVolume())? lhp->GetAbsolutePos(): ZeroVector;
	const float3& volPos = vol->GetWorldSpacePos(unit, lhpPos);

	// linear damage falloff with distance
	const float expDist = (expRadius != 0.0f) ? vol->GetPointSurfaceDistance(unit, lhp, expPos) : 0.0f;
	const float expRim = expDist * expEdgeEffect;

	// return early if (distance > radius)
	if (expDist > expRadius)
		return;

	// expEdgeEffect should be in [0, 1], so expRadius >= expDist >= expDist*expEdgeEffect
	assert(expRadius >= expRim);

	// expMod will also be in [0, 1], no negatives
	// TODO: damage attenuation for underwater units from surface explosions?
	const float expDistanceMod = (expRadius + 0.001f - expDist) / (expRadius + 0.001f - expRim);
	const float modImpulseScale = CalcImpulseScale(damages, expDistanceMod);

	// NOTE: if an explosion occurs right underneath a
	// unit's map footprint, it might cause damage even
	// if the unit's collision volume is greatly offset
	// (because CQuadField coverage is based exclusively
	// on unit->radius, so the DoDamage() iteration will
	// include units that should not be touched)

	const float3 impulseDir = (volPos - expPos).SafeNormalize();
	const float3 expImpulse = impulseDir * modImpulseScale;

	const DamageArray expDamages = damages * expDistanceMod;

	if (expDist < (expSpeed * DIRECT_EXPLOSION_DAMAGE_SPEED_SCALE)) {
		// damage directly
		unit->DoDamage(expDamages, expImpulse, owner, weaponDefID, projectileID);
	} else {
		// damage later
		waitingDamageLists[(gs->frameNum + int(expDist / expSpeed) - 3) & 127].emplace_back((owner? owner->id: -1), unit->id, expDamages, expImpulse, weaponDefID, projectileID);
	}
}

void CGameHelper::DoExplosionDamage(
	CFeature* feature,
	CUnit* owner,
	const float3& expPos,
	const float expRadius,
	const float expEdgeEffect,
	const DamageArray& damages,
	const int weaponDefID,
	const int projectileID
) {
	assert(feature != NULL);

	const LocalModelPiece* lhp = feature->GetLastHitPiece(gs->frameNum);
	const CollisionVolume* vol = feature->GetCollisionVolume(lhp);

	const float3& lhpPos = (lhp != NULL && vol == lhp->GetCollisionVolume())? lhp->GetAbsolutePos(): ZeroVector;
	const float3& volPos = vol->GetWorldSpacePos(feature, lhpPos);

	const float expDist = (expRadius != 0.0f) ? vol->GetPointSurfaceDistance(feature, NULL, expPos) : 0.0f;
	const float expRim = expDist * expEdgeEffect;

	if (expDist > expRadius)
		return;

	assert(expRadius >= expRim);

	const float expDistanceMod = (expRadius + 0.001f - expDist) / (expRadius + 0.001f - expRim);
	const float modImpulseScale = CalcImpulseScale(damages, expDistanceMod);

	const float3 impulseDir = (volPos - expPos).SafeNormalize();
	const float3 expImpulse = impulseDir * modImpulseScale;

	feature->DoDamage(damages * expDistanceMod, expImpulse, owner, weaponDefID, projectileID);
}



void CGameHelper::DamageObjectsInExplosionRadius(
	const CExplosionParams& params,
	const float expRad,
	const int weaponDefID
) {
	static std::vector<CUnit*> unitCache;
	static std::vector<CFeature*> featureCache;

	const unsigned int oldNumUnits = unitCache.size();
	const unsigned int oldNumFeatures = featureCache.size();

	quadField->GetUnitsAndFeaturesColVol(params.pos, expRad, unitCache, featureCache);

	const unsigned int newNumUnits = unitCache.size();
	const unsigned int newNumFeatures = featureCache.size();

	// damage all units within the explosion radius
	// NOTE:
	//   this can recursively trigger ::Explosion() again
	//   which would overwrite our object cache if we did
	//   not keep track of end-markers --> certain objects
	//   would not be damaged AT ALL (!)
	for (unsigned int n = oldNumUnits; n < newNumUnits; n++)
		DoExplosionDamage(unitCache[n], params.owner, params.pos, expRad, params.explosionSpeed, params.edgeEffectiveness, params.ignoreOwner, params.damages, weaponDefID, params.projectileID);

	unitCache.resize(oldNumUnits);

	// damage all features within the explosion radius
	for (unsigned int n = oldNumFeatures; n < newNumFeatures; n++)
		DoExplosionDamage(featureCache[n], params.owner, params.pos, expRad, params.edgeEffectiveness, params.damages, weaponDefID, params.projectileID);

	featureCache.resize(oldNumFeatures);
}

void CGameHelper::Explosion(const CExplosionParams& params) {
	const DamageArray& damages = params.damages;

	// if weaponDef is NULL, this is a piece-explosion
	// (implicit damage-type -DAMAGE_EXPLOSION_DEBRIS)
	const WeaponDef* weaponDef = params.weaponDef;

	const int weaponDefID = (weaponDef != NULL)? weaponDef->id: -CSolidObject::DAMAGE_EXPLOSION_DEBRIS;
	const int explosionID = (weaponDef != NULL)? weaponDef->impactExplosionGeneratorID: CExplosionGeneratorHandler::EXPGEN_ID_STANDARD;


	const float craterAOE = std::max(1.0f, params.craterAreaOfEffect);
	const float damageAOE = std::max(1.0f, params.damageAreaOfEffect);

	const float realHeight = CGround::GetHeightReal(params.pos);
	const float altitude = (params.pos).y - realHeight;

	// NOTE: event triggers before damage is applied to objects
	const bool noGfx = eventHandler.Explosion(weaponDefID, params.projectileID, params.pos, params.owner);

	if (luaUI != NULL) {
		if (weaponDef != NULL && weaponDef->cameraShake > 0.0f) {
			luaUI->ShockFront(params.pos, weaponDef->cameraShake, damageAOE);
		}
	}

	if (params.impactOnly) {
		if (params.hitUnit != NULL) {
			DoExplosionDamage(
				params.hitUnit,
				params.owner,
				params.pos,
				0.0f,
				params.explosionSpeed,
				params.edgeEffectiveness,
				params.ignoreOwner,
				params.damages,
				weaponDefID,
				params.projectileID
			);
		}

		if (params.hitFeature != NULL) {
			DoExplosionDamage(
				params.hitFeature,
				params.owner,
				params.pos,
				0.0f,
				params.edgeEffectiveness,
				params.damages,
				weaponDefID,
				params.projectileID
			);
		}
	} else {
		DamageObjectsInExplosionRadius(params, damageAOE, weaponDefID);

		// deform the map if the explosion was above-ground
		// (but had large enough radius to touch the ground)
		if (altitude >= -1.0f) {
			if (params.damageGround && !mapDamage->disabled && (craterAOE > altitude) && (damages.craterMult > 0.0f)) {
				// limit the depth somewhat
				const float craterDepth = damages.GetDefault() * (1.0f - (altitude / craterAOE));
				const float damageDepth = std::min(craterAOE * 10.0f, craterDepth);
				const float craterStrength = (damageDepth + damages.craterBoost) * damages.craterMult;
				const float craterRadius = craterAOE - altitude;

				mapDamage->Explosion(params.pos, craterStrength, craterRadius);
			}
		}
	}

	if (!noGfx) {
		explGenHandler->GenExplosion(
			explosionID,
			params.pos,
			params.dir,
			damages.GetDefault(),
			damageAOE,
			params.gfxMod,
			params.owner,
			params.hitUnit
		);
	}

	CExplosionCreator::FireExplosionEvent(params);

	if (weaponDef != NULL) {
		const GuiSoundSet& soundSet = weaponDef->hitSound;

		const unsigned int soundFlags = CCustomExplosionGenerator::GetFlagsFromHeight(params.pos.y, realHeight);
		const unsigned int soundMask = CCustomExplosionGenerator::SPW_WATER | CCustomExplosionGenerator::SPW_UNDERWATER;

		const int soundNum = ((soundFlags & soundMask) != 0);
		const int soundID = soundSet.getID(soundNum);

		if (soundID > 0) {
			Channels::Battle->PlaySample(soundID, params.pos, soundSet.getVolume(soundNum));
		}
	}
}



//////////////////////////////////////////////////////////////////////
// Spatial unit queries
//////////////////////////////////////////////////////////////////////

/**
 * @brief Generic spatial unit query.
 *
 * Filter should implement two methods:
 *  - bool Team(int allyTeam): returns true if this allyteam should be considered
 *  - bool Unit(const CUnit*): returns true if the unit should be returned
 *
 * Query should implement three methods:
 *  - float3 GetPos(): returns the center of the (circular) search area
 *  - float GetRadius(): returns the radius of the search area
 *  - void AddUnit(const CUnit*): add the unit to the result
 *
 * The area as returned by Query is approximate; exact circular filtering
 * should be implemented in the Query object if desired.
 * (It isn't necessary for e.g. GetClosest** methods.)
 */
template<typename TFilter, typename TQuery>
static inline void QueryUnits(TFilter filter, TQuery& query)
{
	const auto& quads = quadField->GetQuads(query.pos, query.radius);
	const int tempNum = gs->GetTempNum();

	for (int t = 0; t < teamHandler->ActiveAllyTeams(); ++t) { //FIXME
		if (!filter.Team(t))
			continue;

		for (const int qi: quads) {
			const auto& allyTeamUnits = quadField->GetQuad(qi).teamUnits[t];

			for (CUnit* u: allyTeamUnits) {
				if (u->tempNum == tempNum)
					continue;

				u->tempNum = tempNum;

				if (!filter.Unit(u))
					continue;

				query.AddUnit(u);
			}
		}
	}
}


namespace {
	namespace Filter {

		/**
		 * Base class for Filter::Friendly and Filter::Enemy.
		 */
		struct Base
		{
			const int searchAllyteam;
			Base(int at) : searchAllyteam(at) {}
		};

		/**
		 * Look for friendly units only.
		 * All units are included by default.
		 */
		struct Friendly : public Base
		{
		public:
			Friendly(const CUnit* exclUnit, int allyTeam) : Base(allyTeam), excludeUnit(exclUnit) {}
			bool Team(int allyTeam) { return teamHandler->Ally(searchAllyteam, allyTeam); }
			bool Unit(const CUnit* unit) { return (unit != excludeUnit); }
		protected:
			const CUnit* excludeUnit;
		};

		/**
		 * Look for enemy units only.
		 * All units are included by default.
		 */
		struct Enemy : public Base
		{
		public:
			Enemy(const CUnit* exclUnit, int allyTeam) : Base(allyTeam), excludeUnit(exclUnit) {}
			bool Team(int allyTeam) { return !teamHandler->Ally(searchAllyteam, allyTeam); }
			bool Unit(const CUnit* unit) { return (unit != excludeUnit && !unit->IsNeutral()); }
		protected:
			const CUnit* excludeUnit;
		};

		/**
		 * Look for enemy units which are in LOS/Radar only.
		 */
		struct Enemy_InLos : public Enemy
		{
			Enemy_InLos(const CUnit* exclUnit, int allyTeam) : Enemy(exclUnit, allyTeam) {}
			bool Unit(const CUnit* u) {
				return (u->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR) && Enemy::Unit(u));
			}
		};

		/**
		 * Look for enemy aircraft which are in LOS/Radar only.
		 */
		struct EnemyAircraft : public Enemy_InLos
		{
			EnemyAircraft(const CUnit* exclUnit, int allyTeam) : Enemy_InLos(exclUnit, allyTeam) {}
			bool Unit(const CUnit* u) {
				return (u->unitDef->canfly && !u->IsCrashing() && Enemy_InLos::Unit(u));
			}
		};

		/**
		 * Look for units of any team. Enemy units must be in LOS/Radar.
		 *
		 * NOT SYNCED
		 */
		struct Friendly_All_Plus_Enemy_InLos_NOT_SYNCED
		{
			bool Team(int) const { return true; }
			bool Unit(const CUnit* u) const {
				return (u->allyteam == gu->myAllyTeam) ||
					   (u->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)) ||
					   gu->spectatingFullView;
			}
		};

		/**
		 * Delegates filtering to CMobileCAI::IsValidTarget.
		 *
		 * This is necessary in CMobileCAI and CAirCAI so they can select the closest
		 * enemy unit which they consider a valid target.
		 *
		 * Without the valid target condition, units don't attack anything if an
		 * the nearest enemy is an invalid target. (e.g. noChaseCategory)
		 */
		struct Enemy_InLos_ValidTarget : public Enemy_InLos
		{
			const CMobileCAI* const cai;

			Enemy_InLos_ValidTarget(int at, const CMobileCAI* cai) :
				Enemy_InLos(NULL, at), cai(cai) {}

			bool Unit(const CUnit* u) {
				return Enemy_InLos::Unit(u) && cai->IsValidTarget(u);
			}
		};

	} // end of namespace Filter


	namespace Query {

		/**
		 * Base class for Query objects, containing the basic methods needed by
		 * QueryUnits which defined the search area.
		 */
		struct Base
		{
			const float3& pos;
			const float radius;
			const float sqRadius;
			Base(const float3& pos, float searchRadius) :
				pos(pos), radius(searchRadius), sqRadius(searchRadius * searchRadius) {}
		};

		/**
		 * Return the closest unit.
		 */
		struct ClosestUnit : public Base
		{
		protected:
			float closeSqDist;
			CUnit* closeUnit;

		public:
			ClosestUnit(const float3& pos, float searchRadius) :
				Base(pos, searchRadius), closeSqDist(sqRadius), closeUnit(NULL) {}

			void AddUnit(CUnit* u) {
				const float sqDist = (pos - u->midPos).SqLength2D();
				if (sqDist <= closeSqDist) {
					closeSqDist = sqDist;
					closeUnit = u;
				}
			}

			CUnit* GetClosestUnit() const { return closeUnit; }
		};

		/**
		 * Return the closest unit, using GetUnitErrorPos
		 * instead of the unit's actual position.
		 *
		 * NOT SYNCED
		 */
		struct ClosestUnit_ErrorPos_NOT_SYNCED : public ClosestUnit
		{
			ClosestUnit_ErrorPos_NOT_SYNCED(const float3& pos, float searchRadius) :
				ClosestUnit(pos, searchRadius) {}

			void AddUnit(CUnit* u) {
				float3 unitPos;
				if (gu->spectatingFullView) {
					unitPos = u->midPos;
				} else {
					unitPos = u->GetErrorPos(gu->myAllyTeam);
				}
				const float sqDist = (pos - unitPos).SqLength2D();
				if (sqDist <= closeSqDist) {
					closeSqDist = sqDist;
					closeUnit = u;
				}
			}
		};

		/**
		 * Returns the closest unit (3D) which may have LOS on the search position.
		 * LOS is spherical in the context of this query. Whether the unit actually has
		 * LOS depends on nearby obstacles.
		 *
		 * Search area just needs to touch the unit's radius: this query includes the
		 * target unit's radius.
		 *
		 * If canBeBlind is true then the LOS test is skipped.
		 */
		struct ClosestUnit_InLos : public Base
		{
		protected:
			float closeDist;
			CUnit* closeUnit;
			const bool canBeBlind;

		public:
			ClosestUnit_InLos(const float3& pos, float searchRadius, bool canBeBlind) :
				Base(pos, searchRadius + unitHandler->MaxUnitRadius()),
				closeDist(searchRadius), closeUnit(NULL), canBeBlind(canBeBlind) {}

			void AddUnit(CUnit* u) {
				// FIXME: use volumeBoundingRadius?
				// (more for consistency than need)
				const float dist = pos.distance(u->midPos) - u->radius;

				if (dist <= closeDist &&
					(canBeBlind || (u->losRadius > dist))
				) {
					closeDist = dist;
					closeUnit = u;
				}
			}

			CUnit* GetClosestUnit() const { return closeUnit; }
		};

		/**
		 * Returns the closest unit (2D) which may have LOS on the search position.
		 * Whether it actually has LOS depends on nearby obstacles.
		 *
		 * If canBeBlind is true then the LOS test is skipped.
		 */
		struct ClosestUnit_InLos_Cylinder : public ClosestUnit
		{
			const bool canBeBlind;

			ClosestUnit_InLos_Cylinder(const float3& pos, float searchRadius, bool canBeBlind) :
				ClosestUnit(pos, searchRadius), canBeBlind(canBeBlind) {}

			void AddUnit(CUnit* u) {
				const float sqDist = (pos - u->midPos).SqLength2D();

				if (sqDist <= closeSqDist &&
					(canBeBlind || Square(u->losRadius) > sqDist)
				) {
					closeSqDist = sqDist;
					closeUnit = u;
				}
			}
		};

		/**
		 * Return the unitIDs of all units exactly within the search area.
		 */
		struct AllUnitsById : public Base
		{
		protected:
			vector<int>& found;

		public:
			AllUnitsById(const float3& pos, float searchRadius, vector<int>& found) :
				Base(pos, searchRadius), found(found) {}

			void AddUnit(CUnit* u) {
				if ((pos - u->midPos).SqLength2D() <= sqRadius) {
					found.push_back(u->id);
				}
			}
		};

	} // end of namespace Query
} // end of namespace


void CGameHelper::GenerateWeaponTargets(const CWeapon* weapon, const CUnit* avoidUnit, std::multimap<float, CUnit*>& targets)
{
	const CUnit* owner    = weapon->owner;
	const float radius    = weapon->range;
	const float3& pos     = owner->pos;
	const float aHeight   = weapon->aimFromPos.y;
	const CUnit* lastAttacker = ((owner->lastAttackFrame + 200) <= gs->frameNum) ? owner->lastAttacker : nullptr;

	const WeaponDef* weaponDef = weapon->weaponDef;
	const float heightMod = weaponDef->heightmod;

	// how much damage the weapon deals over 1 second
	const float secDamage = weapon->damages->GetDefault() * weapon->salvoSize / weapon->reloadTime * GAME_SPEED;
	const bool paralyzer  = (weapon->damages->paralyzeDamageTime != 0);

	// copy on purpose since the below calls lua
	const std::vector<int> quads = quadField->GetQuads(pos, radius + (aHeight - std::max(0.0f, readMap->GetInitMinHeight())) * heightMod);
	const int tempNum = gs->GetTempNum();

	for (int t = 0; t < teamHandler->ActiveAllyTeams(); ++t) {
		if (teamHandler->Ally(owner->allyteam, t))
			continue;

		for (const int qi: quads) {
			const std::vector<CUnit*>& allyTeamUnits = quadField->GetQuad(qi).teamUnits[t];

			for (CUnit* targetUnit: allyTeamUnits) {
				if (targetUnit->tempNum == tempNum)
					continue;

				targetUnit->tempNum = tempNum;

				float targetPriority = 1.0f;

				if (!weapon->TestTarget(float3(), SWeaponTarget(targetUnit)))
					continue;

				if (targetUnit == avoidUnit)
					targetPriority *= 10.0f;

				float3 targPos;
				const unsigned short targetLOSState = targetUnit->losStatus[owner->allyteam];

				if (targetLOSState & LOS_INLOS) {
					targPos = targetUnit->aimPos;
				} else if (targetLOSState & LOS_INRADAR) {
					targPos = weapon->GetUnitPositionWithError(targetUnit);
					targetPriority *= 10.0f;
				} else {
					continue;
				}

				const float modRange = radius + (aHeight - targPos.y) * heightMod;

				if (pos.SqDistance2D(targPos) > modRange * modRange)
					continue;

				const float dist2D = (pos - targPos).Length2D();
				const float rangeMul = (dist2D * weaponDef->proximityPriority + modRange * 0.4f + 100.0f);
				const float damageMul = weapon->damages->Get(targetUnit->armorType) * targetUnit->curArmorMultiple;

				targetPriority *= rangeMul;

				if (targetLOSState & LOS_INLOS) {
					targetPriority *= (secDamage + targetUnit->health);

					if (paralyzer && targetUnit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? targetUnit->maxHealth: targetUnit->health))
						targetPriority *= 4.0f;

					if (weapon->hasTargetWeight)
						targetPriority *= weapon->TargetWeight(targetUnit);

				} else {
					targetPriority *= (secDamage + 10000.0f);
				}

				if (targetLOSState & LOS_PREVLOS) {
					targetPriority /= (damageMul * targetUnit->power * (0.7f + gs->randFloat() * 0.6f));

					if (targetUnit->category & weapon->badTargetCategory)
						targetPriority *= 100.0f;

					if (targetUnit->IsCrashing())
						targetPriority *= 1000.0f;

					if (targetUnit == lastAttacker)
						targetPriority *= 0.5f;
				}

				const bool allow = eventHandler.AllowWeaponTarget(owner->id, targetUnit->id, weapon->weaponNum, weaponDef->id, &targetPriority);
				//Lua call may have changed tempNum, so needs to be set again.
				targetUnit->tempNum = tempNum;

				if (!allow)
					continue;

				targets.insert(std::pair<float, CUnit*>(targetPriority, targetUnit));
			}
		}
	}

#ifdef TRACE_SYNC
	{
		tracefile << "[GenerateWeaponTargets] ownerID, attackRadius: " << owner->id << ", " << radius << " ";

		for (std::multimap<float, CUnit*>::const_iterator ti = targets.begin(); ti != targets.end(); ++ti)
			tracefile << "\tpriority: " << (ti->first) <<  ", targetID: " << (ti->second)->id <<  " ";

		tracefile << "\n";
	}
#endif
}

CUnit* CGameHelper::GetClosestUnit(const float3& pos, float searchRadius)
{
	Query::ClosestUnit_ErrorPos_NOT_SYNCED q(pos, searchRadius);
	QueryUnits(Filter::Friendly_All_Plus_Enemy_InLos_NOT_SYNCED(), q);
	return q.GetClosestUnit();
}

CUnit* CGameHelper::GetClosestEnemyUnit(const CUnit* excludeUnit, const float3& pos, float searchRadius, int searchAllyteam)
{
	Query::ClosestUnit q(pos, searchRadius);
	QueryUnits(Filter::Enemy_InLos(excludeUnit, searchAllyteam), q);
	return q.GetClosestUnit();
}

CUnit* CGameHelper::GetClosestValidTarget(const float3& pos, float searchRadius, int searchAllyteam, const CMobileCAI* cai)
{
	Query::ClosestUnit q(pos, searchRadius);
	QueryUnits(Filter::Enemy_InLos_ValidTarget(searchAllyteam, cai), q);
	return q.GetClosestUnit();
}

CUnit* CGameHelper::GetClosestEnemyUnitNoLosTest(
	const CUnit* excludeUnit,
	const float3& pos,
	float searchRadius,
	int searchAllyteam,
	bool sphere,
	bool canBeBlind
) {
	if (sphere) {
		// includes target radius
		Query::ClosestUnit_InLos q(pos, searchRadius, canBeBlind);
		QueryUnits(Filter::Enemy(excludeUnit, searchAllyteam), q);
		return q.GetClosestUnit();
	} else {
		// cylinder (doesn't include target radius)
		Query::ClosestUnit_InLos_Cylinder q(pos, searchRadius, canBeBlind);
		QueryUnits(Filter::Enemy(excludeUnit, searchAllyteam), q);
		return q.GetClosestUnit();
	}
}

CUnit* CGameHelper::GetClosestFriendlyUnit(const CUnit* excludeUnit, const float3& pos, float searchRadius, int searchAllyteam)
{
	Query::ClosestUnit q(pos, searchRadius);
	QueryUnits(Filter::Friendly(excludeUnit, searchAllyteam), q);
	return q.GetClosestUnit();
}

CUnit* CGameHelper::GetClosestEnemyAircraft(const CUnit* excludeUnit, const float3& pos, float searchRadius, int searchAllyteam)
{
	Query::ClosestUnit q(pos, searchRadius);
	QueryUnits(Filter::EnemyAircraft(excludeUnit, searchAllyteam), q);
	return q.GetClosestUnit();
}

void CGameHelper::GetEnemyUnits(const float3& pos, float searchRadius, int searchAllyteam, vector<int> &found)
{
	Query::AllUnitsById q(pos, searchRadius, found);
	QueryUnits(Filter::Enemy_InLos(NULL, searchAllyteam), q);
}

void CGameHelper::GetEnemyUnitsNoLosTest(const float3& pos, float searchRadius, int searchAllyteam, vector<int> &found)
{
	Query::AllUnitsById q(pos, searchRadius, found);
	QueryUnits(Filter::Enemy(NULL, searchAllyteam), q);
}


//////////////////////////////////////////////////////////////////////
// Miscellaneous (i.e. not yet categorized)
//////////////////////////////////////////////////////////////////////

void CGameHelper::BuggerOff(float3 pos, float radius, bool spherical, bool forced, int teamId, CUnit* excludeUnit)
{
	//copy on purpose since BuggerOff can call risky stuff
	const std::vector<CUnit*> units = quadField->GetUnitsExact(pos, radius + SQUARE_SIZE, spherical);
	const int allyTeamId = teamHandler->AllyTeam(teamId);

	for (std::vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* u = *ui;

		// don't send BuggerOff commands to enemy units
		const int uAllyTeamId = u->allyteam;
		const bool allied = (
				teamHandler->Ally(uAllyTeamId,  allyTeamId) ||
				teamHandler->Ally(allyTeamId, uAllyTeamId));

		if ((u != excludeUnit) && allied && ((!u->unitDef->pushResistant && !u->UsingScriptMoveType()) || forced)) {
			u->commandAI->BuggerOff(pos, radius + SQUARE_SIZE);
		}
	}
}


float3 CGameHelper::Pos2BuildPos(const BuildInfo& buildInfo, bool synced)
{
	float3 pos;

	static const int HALFMAP_SQ = SQUARE_SIZE * 2;

	if (buildInfo.GetXSize() & 2)
		pos.x = math::floor((buildInfo.pos.x              ) / (HALFMAP_SQ)) * HALFMAP_SQ + SQUARE_SIZE;
	else
		pos.x = math::floor((buildInfo.pos.x + SQUARE_SIZE) / (HALFMAP_SQ)) * HALFMAP_SQ;

	if (buildInfo.GetZSize() & 2)
		pos.z = math::floor((buildInfo.pos.z              ) / (HALFMAP_SQ)) * HALFMAP_SQ + SQUARE_SIZE;
	else
		pos.z = math::floor((buildInfo.pos.z + SQUARE_SIZE) / (HALFMAP_SQ)) * HALFMAP_SQ;

	pos.y = CGameHelper::GetBuildHeight(pos, buildInfo.def, synced);
	return pos;
}


struct SearchOffset {
	int dx,dy;
	int qdist; // dx*dx+dy*dy
};
static bool SearchOffsetComparator (const SearchOffset& a, const SearchOffset& b)
{
	return a.qdist < b.qdist;
}
static const vector<SearchOffset>& GetSearchOffsetTable (int radius)
{
	static vector <SearchOffset> searchOffsets;
	unsigned int size = radius*radius*4;
	if (size > searchOffsets.size()) {
		searchOffsets.resize (size);

		for (int y = 0; y < radius*2; y++)
			for (int x = 0; x < radius*2; x++)
			{
				SearchOffset& i = searchOffsets[y*radius*2 + x];

				i.dx = x - radius;
				i.dy = y - radius;
				i.qdist = i.dx*i.dx + i.dy*i.dy;
			}

		std::stable_sort(searchOffsets.begin(), searchOffsets.end(), SearchOffsetComparator);
	}

	return searchOffsets;
}

//! only used by the AI callback of the same name
float3 CGameHelper::ClosestBuildSite(int team, const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing)
{
	if (unitDef == NULL) {
		return -RgtVector;
	}

	CFeature* feature = NULL;

	const int allyTeam = teamHandler->AllyTeam(team);
	const int endr = (int) (searchRadius / (SQUARE_SIZE * 2));
	const vector<SearchOffset>& ofs = GetSearchOffsetTable(endr);

	for (int so = 0; so < endr * endr * 4; so++) {
		const float x = pos.x + ofs[so].dx * SQUARE_SIZE * 2;
		const float z = pos.z + ofs[so].dy * SQUARE_SIZE * 2;

		BuildInfo bi(unitDef, float3(x, 0.0f, z), facing);
		bi.pos = Pos2BuildPos(bi, false);

		if (CGameHelper::TestUnitBuildSquare(bi, feature, allyTeam, false) && (!feature || feature->allyteam != allyTeam)) {
			const int xs = (int) (x / SQUARE_SIZE);
			const int zs = (int) (z / SQUARE_SIZE);
			const int xsize = bi.GetXSize();
			const int zsize = bi.GetZSize();

			bool good = true;

			int z2Min = std::max(       0, zs - (zsize    ) / 2 - minDist);
			int z2Max = std::min(mapDims.mapy, zs + (zsize + 1) / 2 + minDist);
			int x2Min = std::max(       0, xs - (xsize    ) / 2 - minDist);
			int x2Max = std::min(mapDims.mapx, xs + (xsize + 1) / 2 + minDist);

			// check for nearby blocking features
			for (int z2 = z2Min; z2 < z2Max; ++z2) {
				for (int x2 = x2Min; x2 < x2Max; ++x2) {
					CSolidObject* solObj = groundBlockingObjectMap->GroundBlockedUnsafe(z2 * mapDims.mapx + x2);

					if (solObj && solObj->immobile && !dynamic_cast<CFeature*>(solObj)) {
						good = false;
						break;
					}
				}
			}

			if (good) {
				z2Min = std::max(       0, zs - (zsize    ) / 2 - minDist - 2);
				z2Max = std::min(mapDims.mapy, zs + (zsize + 1) / 2 + minDist + 2);
				x2Min = std::max(       0, xs - (xsize    ) / 2 - minDist - 2);
				x2Max = std::min(mapDims.mapx, xs + (xsize + 1) / 2 + minDist + 2);

				// check for nearby factories with open yards
				for (int z2 = z2Min; z2 < z2Max; ++z2) {
					for (int x2 = x2Min; x2 < x2Max; ++x2) {
						CSolidObject* solObj = groundBlockingObjectMap->GroundBlockedUnsafe(z2 * mapDims.mapx + x2);

						if (solObj == NULL)
							continue;
						if (!solObj->immobile)
							continue;
						if (!solObj->yardOpen)
							continue;

						good = false;
						break;
					}
				}
			}

			if (good) {
				return bi.pos;
			}
		}
	}

	return -RgtVector;
}

// find the reference height for a build-position
// against which to compare all footprint squares
float CGameHelper::GetBuildHeight(const float3& pos, const UnitDef* unitdef, bool synced)
{
	// we are not going to terraform the ground for mobile units
	// (so we do not care about maxHeightDif constraints either)
	// TODO: maybe respect waterline if <pos> is in water
	if (!unitdef->IsImmobileUnit())
		return (std::max(pos.y, CGround::GetHeightReal(pos.x, pos.z, synced)));

	const float* orgHeightMap = readMap->GetOriginalHeightMapSynced();
	const float* curHeightMap = readMap->GetCornerHeightMapSynced();

	#ifdef USE_UNSYNCED_HEIGHTMAP
	if (!synced) {
		orgHeightMap = readMap->GetCornerHeightMapUnsynced();
		curHeightMap = readMap->GetCornerHeightMapUnsynced();
	}
	#endif

	const float maxDifHgt = unitdef->maxHeightDif;

	float minHgt = readMap->GetCurrMinHeight();
	float maxHgt = readMap->GetCurrMaxHeight();

	unsigned int numBorderSquares = 0;
	float sumBorderSquareHeight = 0.0f;

	static const int xsize = 1;
	static const int zsize = 1;

	// top-left footprint corner (sans clamping)
	const int px = (pos.x - (xsize * (SQUARE_SIZE >> 1))) / SQUARE_SIZE;
	const int pz = (pos.z - (zsize * (SQUARE_SIZE >> 1))) / SQUARE_SIZE;
	// top-left and bottom-right footprint corner (clamped)
	const int x1 = std::min(mapDims.mapx, std::max(0, px));
	const int z1 = std::min(mapDims.mapy, std::max(0, pz));
	const int x2 = std::max(0, std::min(mapDims.mapx, x1 + xsize));
	const int z2 = std::max(0, std::min(mapDims.mapy, z1 + zsize));

	for (int x = x1; x <= x2; x++) {
		for (int z = z1; z <= z2; z++) {
			const float sqOrgHgt = orgHeightMap[z * mapDims.mapxp1 + x];
			const float sqCurHgt = curHeightMap[z * mapDims.mapxp1 + x];
			const float sqMinHgt = std::min(sqCurHgt, sqOrgHgt);
			const float sqMaxHgt = std::max(sqCurHgt, sqOrgHgt);

			if (x == x1 || x == x2 || z == z1 || z == z2) {
				sumBorderSquareHeight += sqCurHgt;
				numBorderSquares += 1;
			}

			// restrict the range of [minH, maxH] to
			// the minimum and maximum square height
			// within the footprint
			if (minHgt < (sqMinHgt - maxDifHgt)) { minHgt = sqMinHgt - maxDifHgt; }
			if (maxHgt > (sqMaxHgt + maxDifHgt)) { maxHgt = sqMaxHgt + maxDifHgt; }
		}
	}

	// find the average height of the footprint-border squares
	float avgHgt = sumBorderSquareHeight / numBorderSquares;

	// and clamp it to [minH, maxH] if necessary
	if (avgHgt < minHgt && minHgt < maxHgt) { avgHgt = (minHgt + 0.01f); }
	if (avgHgt > maxHgt && maxHgt > minHgt) { avgHgt = (maxHgt - 0.01f); }

	if (unitdef->floatOnWater && avgHgt < 0.0f)
		avgHgt = -unitdef->waterline;

	return avgHgt;
}


CGameHelper::BuildSquareStatus CGameHelper::TestUnitBuildSquare(
	const BuildInfo& buildInfo,
	CFeature*& feature,
	int allyteam,
	bool synced,
	std::vector<float3>* canbuildpos,
	std::vector<float3>* featurepos,
	std::vector<float3>* nobuildpos,
	const std::vector<Command>* commands)
{
	feature = NULL;

	const int xsize = buildInfo.GetXSize();
	const int zsize = buildInfo.GetZSize();

	const float3 pos = buildInfo.pos;

	const int x1 = int(pos.x / SQUARE_SIZE) - (xsize >> 1), x2 = x1 + xsize;
	const int z1 = int(pos.z / SQUARE_SIZE) - (zsize >> 1), z2 = z1 + zsize;
	int2 xrange = int2(x1, x2);
	int2 zrange = int2(z1, z2);

	const MoveDef* moveDef = (buildInfo.def->pathType != -1U) ? moveDefHandler->GetMoveDefByPathType(buildInfo.def->pathType) : NULL;
	/*const S3DModel* model =*/ buildInfo.def->LoadModel();

	const float buildHeight = GetBuildHeight(pos, buildInfo.def, synced);
	// const float modelHeight = (model != NULL) ? math::fabs(model->height) : 10.0f;

	BuildSquareStatus canBuild = BUILDSQUARE_OPEN;

	if (buildInfo.def->needGeo) {
		canBuild = BUILDSQUARE_BLOCKED;

		const std::vector<CFeature*>& features = quadField->GetFeaturesExact(pos, std::max(xsize, zsize) * 6);

		const int mindx = xsize * (SQUARE_SIZE >> 1) - (SQUARE_SIZE >> 1);
		const int mindz = zsize * (SQUARE_SIZE >> 1) - (SQUARE_SIZE >> 1);

		// look for a nearby geothermal feature if we need one
		for (std::vector<CFeature*>::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
			if (!(*fi)->def->geoThermal)
				continue;

			const float dx = math::fabs((*fi)->pos.x - pos.x);
			const float dz = math::fabs((*fi)->pos.z - pos.z);

			if (dx < mindx && dz < mindz) {
				canBuild = BUILDSQUARE_OPEN;
				break;
			}
		}
	}

	if (commands != NULL) {
		// this is only called in unsynced context (ShowUnitBuildSquare)
		assert(!synced);

		for (int z = z1; z < z2; z++) {
			for (int x = x1; x < x2; x++) {
				const float3 bpos(x * SQUARE_SIZE, buildHeight, z * SQUARE_SIZE);
				BuildSquareStatus tbs = (bpos.IsInBounds()) ? TestBuildSquare(bpos, xrange, zrange, buildInfo.def, moveDef, feature, gu->myAllyTeam, buildInfo.def->buildingMask, synced) : BUILDSQUARE_BLOCKED;

				if (tbs != BUILDSQUARE_BLOCKED) {
					// test if build-position overlaps a queued command
					for (std::vector<Command>::const_iterator ci = commands->begin(); ci != commands->end(); ++ci) {
						BuildInfo bc(*ci);
						if (std::max(bc.pos.x - bpos.x - SQUARE_SIZE, bpos.x - bc.pos.x) * 2 < bc.GetXSize() * SQUARE_SIZE &&
							std::max(bc.pos.z - bpos.z - SQUARE_SIZE, bpos.z - bc.pos.z) * 2 < bc.GetZSize() * SQUARE_SIZE) {
							tbs = BUILDSQUARE_BLOCKED;
							break;
						}
					}
				}

				switch (tbs) {
					case BUILDSQUARE_OPEN:
						canbuildpos->push_back(bpos);
						break;
					case BUILDSQUARE_RECLAIMABLE:
					case BUILDSQUARE_OCCUPIED:
						featurepos->push_back(bpos);
						break;
					case BUILDSQUARE_BLOCKED:
						nobuildpos->push_back(bpos);
						break;
				}

				canBuild = std::min(canBuild, tbs);
			}
		}
	} else {
		// out of map?
		if (unsigned(x1) > mapDims.mapx || unsigned(x2) > mapDims.mapx || unsigned(z1) > mapDims.mapy || unsigned(z2) > mapDims.mapy)
			return BUILDSQUARE_BLOCKED;

		// this can be called in either context
		for (int z = z1; z < z2; z++) {
			for (int x = x1; x < x2; x++) {
				canBuild = std::min(canBuild, TestBuildSquare(float3(x * SQUARE_SIZE, buildHeight, z * SQUARE_SIZE), xrange, zrange, buildInfo.def, moveDef, feature, allyteam, buildInfo.def->buildingMask, synced));

				if (canBuild == BUILDSQUARE_BLOCKED) {
					return BUILDSQUARE_BLOCKED;
				}
			}
		}
	}

	return canBuild;
}

CGameHelper::BuildSquareStatus CGameHelper::TestBuildSquare(
	const float3& pos,
	const int2& xrange,
	const int2& zrange,
	const UnitDef* unitDef,
	const MoveDef* moveDef,
	CFeature*& feature,
	int allyteam,
	boost::uint16_t mask,
	bool synced
) {
	assert(pos.IsInBounds());

	const int sqx = unsigned(pos.x) / SQUARE_SIZE;
	const int sqz = unsigned(pos.z) / SQUARE_SIZE;
	const float groundHeight = CGround::GetApproximateHeightUnsafe(sqx, sqz, synced);

	if (!unitDef->CheckTerrainConstraints(moveDef, groundHeight))
		return BUILDSQUARE_BLOCKED;

	if (!buildingMaskMap->TestTileMaskUnsafe(sqx >> 1, sqz >> 1, mask))
		return BUILDSQUARE_BLOCKED;


	// check maxHeightDif constraint (structures only)
	//
	// if we are capable of floating, only test local
	// height difference IF terrain is above sea-level
	if (unitDef->IsImmobileUnit()) {
		if (!unitDef->floatOnWater || groundHeight > 0.0f) {
			if (std::abs(pos.y - groundHeight) > unitDef->maxHeightDif) {
				return BUILDSQUARE_BLOCKED;
			}
		}
	}

	BuildSquareStatus ret = BUILDSQUARE_OPEN;
	const int yardxpos = unsigned(pos.x + (SQUARE_SIZE >> 1)) / SQUARE_SIZE;
	const int yardypos = unsigned(pos.z + (SQUARE_SIZE >> 1)) / SQUARE_SIZE;
	CSolidObject* so = groundBlockingObjectMap->GroundBlocked(yardxpos, yardypos);
	if (so != NULL) {
		CFeature* f = dynamic_cast<CFeature*>(so);
		CUnit* u = dynamic_cast<CUnit*>(so);

		// blocking-map can lag behind because it is not updated every frame
		assert(true || (so->pos.x >= xrange.x && so->pos.x <= xrange.y));
		assert(true || (so->pos.z >= zrange.x && so->pos.z <= zrange.y));

		if (f != NULL) {
			if ((allyteam < 0) || f->IsInLosForAllyTeam(allyteam)) {
				if (!f->def->reclaimable) {
					ret = BUILDSQUARE_BLOCKED;
				} else {
					ret = BUILDSQUARE_RECLAIMABLE;
					feature = f;
				}
			}
		} else {
			assert(u);
			if ((allyteam < 0) || (u->losStatus[allyteam] & LOS_INLOS)) {
				if (so->immobile) {
					ret = BUILDSQUARE_BLOCKED;
				} else {
					ret = BUILDSQUARE_OCCUPIED;
				}
			}
		}

		if (ret == BUILDSQUARE_BLOCKED || ret == BUILDSQUARE_OCCUPIED) {
			// if the to-be-buildee has a MoveDef, test if <so> would block it
			// note:
			//   <so> might be another new buildee and if that happens to be located
			//   on sloped ground, then so->pos.y will equal Builder::StartBuild -->
			//   ::Pos2BuildPos --> ::GetBuildHeight which can differ from the actual
			//   ground height at so->pos (s.t. !so->IsOnGround() and the object will
			//   be non-blocking)
			//   fixed: no longer true for mobile units
			#if 0
			if (synced) {
				so->PushPhysicalStateBit(CSolidObject::PSTATE_BIT_ONGROUND);
				so->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_ONGROUND, (math::fabs(so->pos.y - groundHeight) <= 0.5f));
			}
			#endif

			if (moveDef != NULL && CMoveMath::IsNonBlocking(*moveDef, so, NULL)) {
				ret = BUILDSQUARE_OPEN;
			}

			#if 0
			if (synced) {
				so->PopPhysicalStateBit(CSolidObject::PSTATE_BIT_ONGROUND);
			}
			#endif
		}

		if (ret == BUILDSQUARE_BLOCKED) {
			return ret;
		}
	}

	return ret;
}

/**
 * Returns a build Command that intersects the ray described by pos and dir from
 * the command queues of the units 'units' on team number 'team'.
 * @brief returns a build Command that intersects the ray described by pos and dir
 * @return the build Command, or a Command with id 0 if none is found
 */
Command CGameHelper::GetBuildCommand(const float3& pos, const float3& dir) {
	float3 tempF1 = pos;

	CCommandQueue::iterator ci;


	for (CUnit *unit: unitHandler->activeUnits) {

		if (unit->team != gu->myTeam) {
			continue;
		}

		ci = unit->commandAI->commandQue.begin();

		for (; ci != unit->commandAI->commandQue.end(); ++ci) {
			const Command& cmd = *ci;

			if (cmd.GetID() < 0 && cmd.params.size() >= 3) {
				BuildInfo bi(cmd);
				tempF1 = pos + dir * ((bi.pos.y - pos.y) / dir.y) - bi.pos;

				if (bi.def && (bi.GetXSize() / 2) * SQUARE_SIZE > math::fabs(tempF1.x) && (bi.GetZSize() / 2) * SQUARE_SIZE > math::fabs(tempF1.z)) {
					return cmd;
				}
			}
		}
	}

	Command c(CMD_STOP);
	return c;
}




void CGameHelper::Update()
{
	std::vector<WaitingDamage>& wdList = waitingDamageLists[gs->frameNum & 127];

	for (const WaitingDamage& wd: wdList) {
		CUnit* attackee = unitHandler->GetUnit(wd.target);
		CUnit* attacker = unitHandler->GetUnit(wd.attacker); // null if wd.attacker is -1

		if (attackee != nullptr) {
			attackee->DoDamage(wd.damage, wd.impulse, attacker, wd.weaponID, wd.projectileID);
		}
	}

	wdList.clear();
}

