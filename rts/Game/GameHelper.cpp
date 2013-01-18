/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameHelper.h"

#include "Camera.h"
#include "GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaUI.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/ModInfo.h"
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
#include "System/Sound/SoundChannels.h"
#include "System/Sync/SyncTracer.h"

#define PLAY_SOUNDS 1

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGameHelper* helper;


CGameHelper::CGameHelper()
{
	stdExplosionGenerator = new CStdExplosionGenerator();
}

CGameHelper::~CGameHelper()
{
	delete stdExplosionGenerator;

	for (int a = 0; a < 128; ++a) {
		std::list<WaitingDamage*>* wd = &waitingDamages[a];
		while (!wd->empty()) {
			delete wd->back();
			wd->pop_back();
		}
	}
}



//////////////////////////////////////////////////////////////////////
// Explosions/Damage
//////////////////////////////////////////////////////////////////////

void CGameHelper::DoExplosionDamage(
	CUnit* unit,
	CUnit* owner,
	const float3& expPos,
	const float expRadius,
	const float expSpeed,
	const float expEdgeEffect,
	const bool ignoreOwner,
	const DamageArray& damages,
	const int weaponDefID
) {
	if (ignoreOwner && (unit == owner)) {
		return;
	}

	float3 colVolPos;

	const CollisionVolume* colVol = CollisionVolume::GetVolume(unit, colVolPos);

	// linear damage falloff with distance
	const float expDist = colVol->GetPointDistance(unit, expPos);
	const float expRim = expDist * expEdgeEffect;

	// return early if (distance > radius)
	if (expDist >= expRadius)
		return;

	// expEdgeEffect should be in [0, 1], so expDist >= expDist*expEdgeEffect
	assert(expRadius >= expRim);

	// expMod will also be in [0, 1], no negatives
	const float expMod = (expRadius - expDist) / (expRadius + 0.01f - expRim);
	const float dmgMult = (damages.GetDefaultDamage() + damages.impulseBoost);

	// TODO: damage attenuation for underwater units?
	if (expPos.y >= 0.0f && unit->pos.y <  0.0f) {}
	if (expPos.y <  0.0f && unit->pos.y >= 0.0f) {}

	// NOTE: if an explosion occurs right underneath a
	// unit's map footprint, it might cause damage even
	// if the unit's collision volume is greatly offset
	// (because CQuadField coverage is based exclusively
	// on unit->radius, so the DoDamage() iteration will
	// include units that should not be touched)
	//
	// also limit the impulse to prevent later FP overflow
	// (several weapons have _default_ damage values in the order of 1e4,
	// which make the simulation highly unstable because they can impart
	// speeds of several thousand elmos/frame to units and throw them far
	// outside the map)
	// DamageArray::operator* scales damage multipliers,
	// not the impulse factor --> need to scale manually
	// by it for impulse
	const float rawImpulseScale = damages.impulseFactor * expMod * dmgMult;
	const float modImpulseScale = Clamp(rawImpulseScale, -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE);

	const float3 impulseDir = (colVolPos - expPos).SafeNormalize();
	const float3 expImpulse = impulseDir * modImpulseScale;

	const DamageArray expDamages = damages * expMod;

	if (expDist < (expSpeed * DIRECT_EXPLOSION_DAMAGE_SPEED_SCALE)) {
		// damage directly
		unit->DoDamage(expDamages, expImpulse, owner, weaponDefID);
	} else {
		// damage later
		WaitingDamage* wd = new WaitingDamage((owner? owner->id: -1), unit->id, expDamages, expImpulse, weaponDefID);
		waitingDamages[(gs->frameNum + int(expDist / expSpeed) - 3) & 127].push_front(wd);
	}
}

void CGameHelper::DoExplosionDamage(
	CFeature* feature,
	const float3& expPos,
	const float expRadius,
	const float expEdgeEffect,
	const DamageArray& damages,
	const int weaponDefID
) {
	float3 colVolPos;

	const CollisionVolume* colVol = CollisionVolume::GetVolume(feature, colVolPos);
	const float expDist = colVol->GetPointDistance(feature, expPos);
	const float expRim = expDist * expEdgeEffect;

	if (expDist >= expRadius)
		return;

	assert(expRadius >= expRim);

	const float expMod = (expRadius - expDist) / (expRadius + 0.01f - expRim);
	const float dmgMult = (damages.GetDefaultDamage() + damages.impulseBoost);

	const float rawImpulseScale = damages.impulseFactor * expMod * dmgMult;
	const float modImpulseScale = Clamp(rawImpulseScale, -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE);

	const float3 impulseDir = (colVolPos - expPos).SafeNormalize();
	const float3 expImpulse = impulseDir * modImpulseScale;

	feature->DoDamage(damages * expMod, expImpulse, NULL, weaponDefID);
}



void CGameHelper::Explosion(const ExplosionParams& params) {
	const float3& dir = params.dir;
	const float3 expPos = params.pos;
	const DamageArray& damages = params.damages;

	// if weaponDef is NULL, this is a piece-explosion
	// (implicit damage-type -DAMAGE_EXPLOSION_DEBRIS)
	const WeaponDef* weaponDef = params.weaponDef;
	const int weaponDefID = (weaponDef != NULL)? weaponDef->id: -CSolidObject::DAMAGE_EXPLOSION_DEBRIS;


	CUnit* owner = params.owner;
	CUnit* hitUnit = params.hitUnit;
	CFeature* hitFeature = params.hitFeature;

	const float craterAOE = std::max(1.0f, params.craterAreaOfEffect);
	const float damageAOE = std::max(1.0f, params.damageAreaOfEffect);
	const float expEdgeEffect = params.edgeEffectiveness;
	const float expSpeed = params.explosionSpeed;
	const float gfxMod = params.gfxMod;
	const float realHeight = ground->GetHeightReal(expPos.x, expPos.z);
	const float altitude = expPos.y - realHeight;

	const bool impactOnly = params.impactOnly;
	const bool ignoreOwner = params.ignoreOwner;
	const bool damageGround = params.damageGround;
	const bool noGfx = eventHandler.Explosion(weaponDefID, expPos, owner);

	if (luaUI) {
		if (weaponDef != NULL && weaponDef->cameraShake > 0.0f) {
			luaUI->ShockFront(weaponDef->cameraShake, expPos, damageAOE);
		}
	}

	if (impactOnly) {
		if (hitUnit) {
			DoExplosionDamage(hitUnit, owner, expPos, damageAOE, expSpeed, expEdgeEffect, ignoreOwner, damages, weaponDefID);
		} else if (hitFeature) {
			DoExplosionDamage(hitFeature, expPos, damageAOE, expEdgeEffect, damages, weaponDefID);
		}
	} else {
		{
			// damage all units within the explosion radius
			const vector<CUnit*>& units = qf->GetUnitsExact(expPos, damageAOE);
			bool hitUnitDamaged = false;

			for (vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
				CUnit* unit = *ui;

				if (unit == hitUnit) {
					hitUnitDamaged = true;
				}

				DoExplosionDamage(unit, owner, expPos, damageAOE, expSpeed, expEdgeEffect, ignoreOwner, damages, weaponDefID);
			}

			// HACK: for a unit with an offset coldet volume, the explosion
			// (from an impacting projectile) position might not correspond
			// to its quadfield position so we need to damage it separately
			if (hitUnit != NULL && !hitUnitDamaged) {
				DoExplosionDamage(hitUnit, owner, expPos, damageAOE, expSpeed, expEdgeEffect, ignoreOwner, damages, weaponDefID);
			}
		}

		{
			// damage all features within the explosion radius
			const vector<CFeature*>& features = qf->GetFeaturesExact(expPos, damageAOE);
			bool hitFeatureDamaged = false;

			for (vector<CFeature*>::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
				CFeature* feature = *fi;

				if (feature == hitFeature) {
					hitFeatureDamaged = true;
				}

				DoExplosionDamage(feature, expPos, damageAOE, expEdgeEffect, damages, weaponDefID);
			}

			if (hitFeature != NULL && !hitFeatureDamaged) {
				DoExplosionDamage(hitFeature, expPos, damageAOE, expEdgeEffect, damages, weaponDefID);
			}
		}

		// deform the map if the explosion was above-ground
		// (but had large enough radius to touch the ground)
		if (altitude >= -1.0f) {
			if (damageGround && !mapDamage->disabled && (craterAOE > altitude) && (damages.craterMult > 0.0f)) {
				// limit the depth somewhat
				const float craterDepth = damages.GetDefaultDamage() * (1.0f - (altitude / craterAOE));
				const float damageDepth = std::min(craterAOE * 10.0f, craterDepth);
				const float craterStrength = (damageDepth + damages.craterBoost) * damages.craterMult;
				const float craterRadius = craterAOE - altitude;

				mapDamage->Explosion(expPos, craterStrength, craterRadius);
			}
		}
	}

	if (!noGfx) {
		// use CStdExplosionGenerator by default
		IExplosionGenerator* explosionGenerator = stdExplosionGenerator;

		if (weaponDef != NULL && weaponDef->explosionGenerator != NULL) {
			explosionGenerator = weaponDef->explosionGenerator;
		}

		explosionGenerator->Explosion(0, expPos, damages.GetDefaultDamage(), damageAOE, owner, gfxMod, hitUnit, dir);
	}

	CExplosionEvent explosionEvent(expPos, damages.GetDefaultDamage(), damageAOE, weaponDef);
	FireExplosionEvent(explosionEvent);

	#if (PLAY_SOUNDS == 1)
	if (weaponDef != NULL) {
		const GuiSoundSet& soundSet = weaponDef->hitSound;

		const unsigned int soundFlags = CCustomExplosionGenerator::GetFlagsFromHeight(expPos.y, altitude);
		const int soundNum = ((soundFlags & (CCustomExplosionGenerator::SPW_WATER | CCustomExplosionGenerator::SPW_UNDERWATER)) != 0);
		const int soundID = soundSet.getID(soundNum);

		if (soundID > 0) {
			Channels::Battle.PlaySample(soundID, expPos, soundSet.getVolume(soundNum));
		}
	}
	#endif
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
	GML_RECMUTEX_LOCK(qnum); // QueryUnits

	const vector<int> &quads = qf->GetQuads(query.pos, query.radius);

	const int tempNum = gs->tempNum++;
	
	for (vector<int>::const_iterator qi = quads.begin(); qi != quads.end(); ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);
		for (int t = 0; t < teamHandler->ActiveAllyTeams(); ++t) {
			if (!filter.Team(t)) {
				continue;
			}
			std::list<CUnit*>::const_iterator ui;
			const std::list<CUnit*>& allyTeamUnits = quad.teamUnits[t];
			for (ui = allyTeamUnits.begin(); ui != allyTeamUnits.end(); ++ui) {
				if ((*ui)->tempNum != tempNum) {
					(*ui)->tempNum = tempNum;
					if (filter.Unit(*ui)) {
						query.AddUnit(*ui);
					}
				}
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
			bool Unit(const CUnit* unit) { return (unit != excludeUnit); }
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
				return (u != excludeUnit && u->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR));
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

	}; // end of namespace Filter


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
		 * Return the closest unit, using CGameHelper::GetUnitErrorPos
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
					unitPos = helper->GetUnitErrorPos(u, gu->myAllyTeam);
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
				Base(pos, searchRadius + uh->MaxUnitRadius()),
				closeDist(searchRadius), closeUnit(NULL), canBeBlind(canBeBlind) {}

			void AddUnit(CUnit* u) {
				// FIXME: use volumeBoundingRadius?
				// (more for consistency than need)
				const float dist = pos.distance(u->midPos) - u->radius;

				if (dist <= closeDist &&
					(canBeBlind || u->losRadius * loshandler->losDiv > dist)) {
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
					(canBeBlind || Square(u->losRadius * loshandler->losDiv) > sqDist)) {
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

	}; // end of namespace Query
}; // end of namespace

// Use this instead of unit->tempNum here, because it requires a mutex lock that will deadlock if luaRules is invoked simultaneously.
// Not the cleanest solution, but faster than e.g. a std::set, and this function is called quite frequently.
static int tempTargetUnits[MAX_UNITS] = {0};
static int targetTempNum = 2;

void CGameHelper::GenerateWeaponTargets(const CWeapon* weapon, const CUnit* lastTargetUnit, std::multimap<float, CUnit*>& targets)
{
	const CUnit* attacker = weapon->owner;
	const float radius    = weapon->range;
	const float3& pos     = attacker->pos;
	const float heightMod = weapon->heightMod;
	const float aHeight   = weapon->weaponPos.y;

	// how much damage the weapon deals over 1 second
	const float secDamage = weapon->weaponDef->damages.GetDefaultDamage() * weapon->salvoSize / weapon->reloadTime * GAME_SPEED;
	const bool paralyzer  = !!weapon->weaponDef->damages.paralyzeDamageTime;

	const std::vector<int>& quads = qf->GetQuads(pos, radius + (aHeight - std::max(0.f, readmap->initMinHeight)) * heightMod);

	const int tempNum = targetTempNum++;

	typedef std::vector<int>::const_iterator VectorIt;
	typedef std::list<CUnit*>::const_iterator ListIt;

	for (VectorIt qi = quads.begin(); qi != quads.end(); ++qi) {
		for (int t = 0; t < teamHandler->ActiveAllyTeams(); ++t) {
			if (teamHandler->Ally(attacker->allyteam, t)) {
				continue;
			}

			const std::list<CUnit*>& allyTeamUnits = qf->GetQuad(*qi).teamUnits[t];

			for (ListIt ui = allyTeamUnits.begin(); ui != allyTeamUnits.end(); ++ui) {
				CUnit* targetUnit = *ui;
				float targetPriority = 1.0f;

				if (!(targetUnit->category & weapon->onlyTargetCategory)) {
					continue;
				}
				if (targetUnit->GetTransporter() != NULL) {
					if (!modInfo.targetableTransportedUnits)
						continue;
					// the transportee might be "hidden" below terrain, in which case we can't target it
					if (targetUnit->pos.y < ground->GetHeightReal(targetUnit->pos.x, targetUnit->pos.z))
						continue;
				}
				if (tempTargetUnits[targetUnit->id] == tempNum) {
					continue;
				}

				tempTargetUnits[targetUnit->id] = tempNum;

				if (targetUnit->isUnderWater && !weapon->weaponDef->waterweapon) {
					continue;
				}
				if (targetUnit->isDead) {
					continue;
				}

				float3 targPos;
				const unsigned short targetLOSState = targetUnit->losStatus[attacker->allyteam];

				if (targetLOSState & LOS_INLOS) {
					targPos = targetUnit->aimPos;
				} else if (targetLOSState & LOS_INRADAR) {
					targPos = targetUnit->aimPos + (targetUnit->posErrorVector * radarhandler->radarErrorSize[attacker->allyteam]);
					targetPriority *= 10.0f;
				} else {
					continue;
				}

				const float modRange = radius + (aHeight - targPos.y) * heightMod;

				if ((pos - targPos).SqLength2D() > modRange * modRange) {
					continue;
				}

				const float dist2D = (pos - targPos).Length2D();
				const float rangeMul = (dist2D * weapon->weaponDef->proximityPriority + modRange * 0.4f + 100.0f);
				const float damageMul = weapon->weaponDef->damages[targetUnit->armorType] * targetUnit->curArmorMultiple;

				targetPriority *= rangeMul;

				if (targetLOSState & LOS_INLOS) {
					targetPriority *= (secDamage + targetUnit->health);

					if (targetUnit == lastTargetUnit) {
						targetPriority *= weapon->avoidTarget ? 10.0f : 0.4f;
					}

					if (paralyzer && targetUnit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? targetUnit->maxHealth: targetUnit->health)) {
						targetPriority *= 4.0f;
					}

					if (weapon->hasTargetWeight) {
						targetPriority *= weapon->TargetWeight(targetUnit);
					}
				} else {
					targetPriority *= (secDamage + 10000.0f);
				}

				if (targetLOSState & LOS_PREVLOS) {
					targetPriority /= (damageMul * targetUnit->power * (0.7f + gs->randFloat() * 0.6f));

					if (targetUnit->category & weapon->badTargetCategory) {
						targetPriority *= 100.0f;
					}
					if (targetUnit->IsCrashing()) {
						targetPriority *= 1000.0f;
					}
				}

				if (luaRules != NULL) {
					const bool targetAllowed = luaRules->AllowWeaponTarget(attacker->id, targetUnit->id, weapon->weaponNum, weapon->weaponDef->id, &targetPriority);
					if (!targetAllowed) {
						continue;
					}
				}

				targets.insert(std::pair<float, CUnit*>(targetPriority, targetUnit));
			}
		}
	}

#ifdef TRACE_SYNC
	{
		tracefile << "[GenerateWeaponTargets] attackerID, attackRadius: " << attacker->id << ", " << radius << " ";

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

float3 CGameHelper::GetUnitErrorPos(const CUnit* unit, int allyteam, bool aiming)
{
	float3 pos = aiming? unit->aimPos: unit->midPos;

	if (teamHandler->Ally(allyteam, unit->allyteam) || (unit->losStatus[allyteam] & LOS_INLOS)) {
		// ^ it's one of our own, or it's in LOS, so don't add an error ^
		return pos;
	}
	if (gameSetup->ghostedBuildings && (unit->losStatus[allyteam] & LOS_PREVLOS) && unit->unitDef->IsBuildingUnit()) {
		// ^ this is a ghosted building, so don't add an error ^
		return pos;
	}

	if ((unit->losStatus[allyteam] & LOS_INRADAR) != 0) {
		pos += (unit->posErrorVector * radarhandler->radarErrorSize[allyteam]);
	} else {
		pos += (unit->posErrorVector * radarhandler->baseRadarErrorSize * 2);
	}

	return pos;
}


void CGameHelper::BuggerOff(float3 pos, float radius, bool spherical, bool forced, int teamId, CUnit* excludeUnit)
{
	const std::vector<CUnit*> &units = qf->GetUnitsExact(pos, radius + SQUARE_SIZE, spherical);
	const int allyTeamId = teamHandler->AllyTeam(teamId);

	for (std::vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* u = *ui;

		// don't send BuggerOff commands to enemy units
		const int uAllyTeamId = u->allyteam;
		const bool allied = (
				teamHandler->Ally(uAllyTeamId,  allyTeamId) ||
				teamHandler->Ally(allyTeamId, uAllyTeamId));

		if ((u != excludeUnit) && allied && ((!u->unitDef->pushResistant && !u->usingScriptMoveType) || forced)) {
			u->commandAI->BuggerOff(pos, radius + SQUARE_SIZE);
		}
	}
}


float3 CGameHelper::Pos2BuildPos(const BuildInfo& buildInfo, bool synced)
{
	float3 pos;

	if (buildInfo.GetXSize() & 2)
		pos.x = math::floor((buildInfo.pos.x              ) / (SQUARE_SIZE * 2)) * SQUARE_SIZE * 2 + SQUARE_SIZE;
	else
		pos.x = math::floor((buildInfo.pos.x + SQUARE_SIZE) / (SQUARE_SIZE * 2)) * SQUARE_SIZE * 2;

	if (buildInfo.GetZSize() & 2)
		pos.z = math::floor((buildInfo.pos.z              ) / (SQUARE_SIZE * 2)) * SQUARE_SIZE * 2 + SQUARE_SIZE;
	else
		pos.z = math::floor((buildInfo.pos.z + SQUARE_SIZE) / (SQUARE_SIZE * 2)) * SQUARE_SIZE * 2;

	const UnitDef* ud = buildInfo.def;
	const float bh = uh->GetBuildHeight(pos, ud, synced);

	pos.y = bh;

	if (ud->floatOnWater && bh < 0.0f)
		pos.y = -ud->waterline;

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

		std::sort(searchOffsets.begin(), searchOffsets.end(), SearchOffsetComparator);
	}

	return searchOffsets;
}

//! only used by the AI callback of the same name
float3 CGameHelper::ClosestBuildSite(int team, const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing)
{
	if (!unitDef) {
		return float3(-1.0f, 0.0f, 0.0f);
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

		if (uh->TestUnitBuildSquare(bi, feature, allyTeam, false) && (!feature || feature->allyteam != allyTeam)) {
			const int xs = (int) (x / SQUARE_SIZE);
			const int zs = (int) (z / SQUARE_SIZE);
			const int xsize = bi.GetXSize();
			const int zsize = bi.GetZSize();

			bool good = true;

			int z2Min = std::max(       0, zs - (zsize    ) / 2 - minDist);
			int z2Max = std::min(gs->mapy, zs + (zsize + 1) / 2 + minDist);
			int x2Min = std::max(       0, xs - (xsize    ) / 2 - minDist);
			int x2Max = std::min(gs->mapx, xs + (xsize + 1) / 2 + minDist);

			// check for nearby blocking features
			for (int z2 = z2Min; z2 < z2Max; ++z2) {
				for (int x2 = x2Min; x2 < x2Max; ++x2) {
					CSolidObject* solObj = groundBlockingObjectMap->GroundBlockedUnsafe(z2 * gs->mapx + x2);

					if (solObj && solObj->immobile && !dynamic_cast<CFeature*>(solObj)) {
						good = false;
						break;
					}
				}
			}

			if (good) {
				z2Min = std::max(       0, zs - (zsize    ) / 2 - minDist - 2);
				z2Max = std::min(gs->mapy, zs + (zsize + 1) / 2 + minDist + 2);
				x2Min = std::max(       0, xs - (xsize    ) / 2 - minDist - 2);
				x2Max = std::min(gs->mapx, xs + (xsize + 1) / 2 + minDist + 2);

				// check for nearby factories with open yards
				for (int z2 = z2Min; z2 < z2Max; ++z2) {
					for (int x2 = x2Min; x2 < x2Max; ++x2) {
						CSolidObject* solObj = groundBlockingObjectMap->GroundBlockedUnsafe(z2 * gs->mapx + x2);

						if (solObj && solObj->immobile && dynamic_cast<CFactory*>(solObj) && static_cast<CFactory*>(solObj)->opening) {
							good = false;
							break;
						}
					}
				}
			}

			if (good) {
				return bi.pos;
			}
		}
	}

	return float3(-1.0f, 0.0f, 0.0f);
}

void CGameHelper::Update()
{
	std::list<WaitingDamage*>* wd = &waitingDamages[gs->frameNum & 127];

	while (!wd->empty()) {
		WaitingDamage* w = wd->back();
		wd->pop_back();

		CUnit* attackee = uh->units[w->target];
		CUnit* attacker = (w->attacker == -1)? NULL: uh->units[w->attacker];

		if (attackee != NULL)
			attackee->DoDamage(w->damage, w->impulse, attacker, w->weaponId);

		delete w;
	}
}
