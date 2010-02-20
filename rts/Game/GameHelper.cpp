// GameHelper.cpp: implementation of the CGameHelperHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include "mmgr.h"

#include "GlobalUnsynced.h"
#include "Camera.h"
#include "GameSetup.h"
#include "Game.h"
#include "GameHelper.h"
#include "UI/LuaUI.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/GroundDecalHandler.h"
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
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sync/SyncTracer.h"
#include "EventHandler.h"
#include "myMath.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGameHelper* helper;


CGameHelper::CGameHelper()
{
	stdExplosionGenerator = new CStdExplosionGenerator;
}

CGameHelper::~CGameHelper()
{
	delete stdExplosionGenerator;

	for(int a=0;a<128;++a){
		std::list<WaitingDamage*>* wd=&waitingDamages[a];
		while(!wd->empty()){
			delete wd->back();
			wd->pop_back();
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Explosions/Damage
//////////////////////////////////////////////////////////////////////

void CGameHelper::DoExplosionDamage(CUnit* unit,
	const float3& expPos, float expRad, float expSpeed,
	bool ignoreOwner, CUnit* owner, float edgeEffectiveness,
	const DamageArray& damages, int weaponId)
{
	if (ignoreOwner && (unit == owner)) {
		return;
	}

	// dist is equal to the maximum of "distance from center
	// of unit to center of explosion" and "unit radius + 0.1",
	// where "center of unit" is determined by the relative
	// position of its collision volume and "unit radius" by
	// the volume's minimally-bounding sphere
	//
	float3 dif = (unit->midPos + unit->collisionVolume->GetOffsets()) - expPos;
	const float volRad = unit->collisionVolume->GetBoundingRadius();
	const float expDist = std::max(dif.Length(), volRad + 0.1f);

	// expDist2 is the distance from the boundary of the
	// _volume's_ minimally-bounding sphere (!) to the
	// explosion center, unless unit->isUnderWater and
	// the explosion is above water: then center2center
	// distance is used
	//
	// NOTE #1: this will be only an approximation when
	// the unit's collision volume is not a sphere, but
	// a better one than when using unit->radius
	//
	// NOTE #2: if an explosion occurs right underneath
	// a unit's map footprint, it can cause damage even
	// if the unit's collision volume is greatly offset
	// (because CQuadField is again based exclusively on
	// unit->radius, so the iteration will include units
	// that should not be touched)
	// Clamp expDist to radius to prevent division by zero
	// (expDist2 can never be > radius). We still need the
	// original expDist later to normalize dif.
	// expDist2 _can_ exceed radius when explosion is eg.
	// on shield surface: in that case don't do any damage
	float expDist2 = expDist - volRad;
	float expDist1 = std::min(expDist, expRad);

	if (expDist2 > expRad) {
		return;
	}

	if (unit->isUnderWater && (expPos.y > -1.0f)) {
		// should make it harder to damage subs with above-water weapons
		expDist2 += volRad;
		expDist2 = std::min(expDist2, expRad);
	}

	float mod  = (expRad - expDist1) / (expRad - expDist1 * edgeEffectiveness);
	float mod2 = (expRad - expDist2) / (expRad - expDist2 * edgeEffectiveness);
	dif /= expDist;
	dif.y += 0.12f;

	if (mod < 0.01f)
		mod = 0.01f;

	DamageArray damageDone = damages * mod2;
	float3 addedImpulse = dif * (damages.impulseFactor * mod * (damages[0] + damages.impulseBoost) * 3.2f);

	if (expDist2 < (expSpeed * 4.0f)) { //damage directly
		unit->DoDamage(damageDone, owner, addedImpulse, weaponId);
	} else { //damage later
		WaitingDamage* wd = new WaitingDamage((owner? owner->id: -1), unit->id, damageDone, addedImpulse, weaponId);
		waitingDamages[(gs->frameNum + int(expDist2 / expSpeed) - 3) & 127].push_front(wd);
	}
}

void CGameHelper::DoExplosionDamage(CFeature* feature,
	const float3& expPos, float expRad, CUnit* owner, const DamageArray& damages)
{
	CollisionVolume* cv = feature->collisionVolume;

	if (cv) {
		float3 dif = (feature->midPos + cv->GetOffsets()) - expPos;
		float expDist = std::max(dif.Length(), 0.1f);
		float expMod = (expRad - expDist) / expRad;

		// always do some damage with explosive stuff
		// (DDM wreckage etc. is too big to normally
		// be damaged otherwise, even by BB shells)
		// NOTE: this will also be only approximate
		// for non-spherical volumes
		if ((expRad > 8.0f) && (expDist < (cv->GetBoundingRadius() * 1.1f)) && (expMod < 0.1f)) {
			expMod = 0.1f;
		}
		if (expMod > 0.0f) {
			feature->DoDamage(damages * expMod, owner,
				dif * (damages.impulseFactor * expMod / expDist *
				(damages[0] + damages.impulseBoost)));
		}
	}
}


void CGameHelper::Explosion(
	float3 expPos, const DamageArray& damages,
	float expRad, float edgeEffectiveness,
	float expSpeed, CUnit* owner,
	bool damageGround, float gfxMod,
	bool ignoreOwner, bool impactOnly,
	CExplosionGenerator* explosionGraphics, CUnit* hit,
	const float3& impactDir, int weaponId,
	CFeature* hitfeature
) {
	if (luaUI) {
		if ((weaponId >= 0) && (weaponId <= weaponDefHandler->numWeaponDefs)) {
			WeaponDef& wd = weaponDefHandler->weaponDefs[weaponId];
			const float cameraShake = wd.cameraShake;
			if (cameraShake > 0.0f) {
				luaUI->ShockFront(cameraShake, expPos, expRad);
			}
		}
	}

	bool noGfx = eventHandler.Explosion(weaponId, expPos, owner);

#ifdef TRACE_SYNC
	tracefile << "Explosion: ";
	tracefile << expPos.x << " " << damages[0] <<  " " << expRad << "\n";
#endif

	float h2 = ground->GetHeight2(expPos.x, expPos.z);
	expPos.y = std::max(expPos.y, h2);
	expRad = std::max(expRad, 1.0f);

	if (impactOnly) {
		if (hit) {
			DoExplosionDamage(hit, expPos, expRad, expSpeed, ignoreOwner, owner, edgeEffectiveness, damages, weaponId);
		} else if (hitfeature) {
			DoExplosionDamage(hitfeature, expPos, expRad, owner, damages);
		}
	} else {
		float height = std::max(expPos.y - h2, 0.0f);

		// damage all units within the explosion radius
		vector<CUnit*> units = qf->GetUnitsExact(expPos, expRad);
		vector<CUnit*>::iterator ui;
		bool hitUnitDamaged = false;

		for (ui = units.begin(); ui != units.end(); ++ui) {
			CUnit* unit = *ui;

			if (unit == hit) {
				hitUnitDamaged = true;
			}

			DoExplosionDamage(unit, expPos, expRad, expSpeed, ignoreOwner, owner, edgeEffectiveness, damages, weaponId);
		}

		// HACK: for a unit with an offset coldet volume, the explosion
		// (from an impacting projectile) position might not correspond
		// to its quadfield position so we need to damage it separately
		if (hit && !hitUnitDamaged) {
			DoExplosionDamage(hit, expPos, expRad, expSpeed, ignoreOwner, owner, edgeEffectiveness, damages, weaponId);
		}


		// damage all features within the explosion radius
		vector<CFeature*> features = qf->GetFeaturesExact(expPos, expRad);
		vector<CFeature*>::iterator fi;
		bool hitFeatureDamaged = false;

		for (fi = features.begin(); fi != features.end(); ++fi) {
			CFeature* feature = *fi;

			if (hitfeature == feature) {
				hitFeatureDamaged = true;
			}
			DoExplosionDamage(feature, expPos, expRad, owner, damages);
		}

		if (hitfeature && !hitFeatureDamaged) {
			DoExplosionDamage(hitfeature, expPos, expRad, owner, damages);
		}

		// deform the map
		if (damageGround && !mapDamage->disabled &&
		    (expRad > height) && (damages.craterMult > 0.0f)) {
			float damage = damages[0] * (1.0f - (height / expRad));
			if (damage > (expRad * 10.0f)) {
				damage = expRad * 10.0f; // limit the depth somewhat
			}
			mapDamage->Explosion(expPos, (damage + damages.craterBoost) * damages.craterMult, expRad - height);
		}
	}

	// use CStdExplosionGenerator by default
	if (!noGfx) {
		if (!explosionGraphics) {
			explosionGraphics = stdExplosionGenerator;
		}
		explosionGraphics->Explosion(expPos, damages[0], expRad, owner, gfxMod, hit, impactDir);
	}

	groundDecals->AddExplosion(expPos, damages[0], expRad);

	water->AddExplosion(expPos, damages[0], expRad);
}

//////////////////////////////////////////////////////////////////////
// Raytracing
//////////////////////////////////////////////////////////////////////

// called by {CRifle, CBeamLaser, CLightningCannon}::Fire()
float CGameHelper::TraceRay(const float3& start, const float3& dir, float length, float /*power*/,
			    const CUnit* owner, const CUnit*& hit, int collisionFlags,
			    const CFeature** hitfeature)
{
	float groundLength = ground->LineGroundCol(start, start + dir * length);
	const bool ignoreAllies = !!(collisionFlags & COLLISION_NOFRIENDLY);
	const bool ignoreFeatures = !!(collisionFlags & COLLISION_NOFEATURE);
	const bool ignoreNeutrals = !!(collisionFlags & COLLISION_NONEUTRAL);

	if (length > groundLength && groundLength > 0) {
		length = groundLength;
	}

	CollisionQuery cq;

	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(start, dir, length, endQuad);

	if (!ignoreFeatures) {
		if (hitfeature)
			*hitfeature = 0;

		for (int* qi = quads; qi != endQuad; ++qi) {
			const CQuadField::Quad& quad = qf->GetQuad(*qi);

			for (std::list<CFeature*>::const_iterator ui = quad.features.begin(); ui != quad.features.end(); ++ui) {
				CFeature* f = *ui;

				if (!f->blocking || !f->collisionVolume) {
					// NOTE: why check the blocking property?
					continue;
				}

				if (CCollisionHandler::Intersect(f, start, start + dir * length, &cq)) {
					const float3& intPos = (cq.b0)? cq.p0: cq.p1;
					const float tmpLen = (intPos - start).Length();

					// we want the closest feature (intersection point) on the ray
					if (tmpLen < length) {
						length = tmpLen;
						if(hitfeature)
							*hitfeature = f;
					}
				}
			}
		}
	}

	hit = NULL;

	for (int* qi = quads; qi != endQuad; ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);

		for (std::list<CUnit*>::const_iterator ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			const CUnit* u = *ui;

			if (u == owner)
				continue;
			if (ignoreAllies && u->allyteam == owner->allyteam)
				continue;
			if (ignoreNeutrals && u->IsNeutral()) {
				continue;
			}

			if (CCollisionHandler::Intersect(u, start, start + dir * length, &cq)) {
				const float3& intPos = (cq.b0)? cq.p0: cq.p1;
				const float tmpLen = (intPos - start).Length();

				// we want the closest unit (intersection point) on the ray
				if (tmpLen < length) {
					length = tmpLen;
					hit = u;
				}
			}
		}
	}

	return length;
}

float CGameHelper::GuiTraceRay(const float3 &start, const float3 &dir, float length, const CUnit*& hit, bool useRadar, const CUnit* exclude)
{
	if (dir == ZeroVector) {
		return -1.0f;
	}

	// distance from start to ground intersection point + fudge
	float groundLen   = ground->LineGroundCol(start, start + dir * length);
	float returnLenSq = Square( (groundLen > 0.0f)? groundLen + 200.0f: length );

	hit = NULL;
	CollisionQuery cq;

	GML_RECMUTEX_LOCK(quad); // GuiTraceRay

	vector<int> quads = qf->GetQuadsOnRay(start, dir, length);
	vector<int>::iterator qi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);
		std::list<CUnit*>::const_iterator ui;

		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			CUnit* unit = *ui;
			if (unit == exclude) {
				continue;
			}

			if ((unit->allyteam == gu->myAllyTeam) || gu->spectatingFullView ||
				(unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR)) ||
				(useRadar && radarhandler->InRadar(unit, gu->myAllyTeam))) {

				CollisionVolume cv(unit->collisionVolume);

				if (unit->isIcon) {
					// for iconified units, just pretend the collision
					// volume is a sphere of radius <unit->IconRadius>
					cv.SetDefaultScale(unit->iconRadius);
				}

				if (CCollisionHandler::MouseHit(unit, start, start + dir * length, &cv, &cq)) {
					// get the distance to the ray-volume egress point
					// so we can still select stuff inside factories
					const float len = (cq.p1 - start).SqLength();

					if (len < returnLenSq) {
						returnLenSq = len;
						hit = unit;
					}
				}
			}
		}
	}

	return ((hit)? math::sqrt(returnLenSq): (math::sqrt(returnLenSq) - 200.0f));
}

float CGameHelper::TraceRayTeam(const float3& start, const float3& dir, float length, CUnit*& hit, bool useRadar, CUnit* exclude, int allyteam)
{
	float groundLength = ground->LineGroundCol(start, start + dir * length);

	if (length > groundLength && groundLength > 0) {
		length = groundLength;
	}

	GML_RECMUTEX_LOCK(quad); // TraceRayTeam

	vector<int> quads = qf->GetQuadsOnRay(start, dir, length);
	hit = 0;

	vector<int>::iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);
		std::list<CUnit*>::const_iterator ui;

		// NOTE: switch this to custom volumes fully? (only
		// used in FPS unit control mode, maybe unnecessary)
		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			CUnit* u = *ui;

			if (u == exclude)
				continue;

			const CollisionVolume* cv = u->collisionVolume;

			if (teamHandler->Ally(u->allyteam, allyteam) || (u->losStatus[allyteam] & LOS_INLOS)) {
				float3 dif = (u->midPos + cv->GetOffsets()) - start;
				float closeLength = dif.dot(dir);

				if (closeLength < 0)
					continue;
				if (closeLength > length)
					continue;

				float3 closeVect = dif - dir * closeLength;

				if (closeVect.SqLength() < cv->GetBoundingRadiusSq()) {
					length = closeLength;
					hit = u;
				}
			} else if (useRadar && radarhandler->InRadar(u, allyteam)) {
				float3 dif =
					(u->midPos + cv->GetOffsets()) +
					u->posErrorVector * radarhandler->radarErrorSize[allyteam] -
					start;
				float closeLength = dif.dot(dir);

				if (closeLength < 0)
					continue;
				if (closeLength > length)
					continue;

				float3 closeVect = dif - dir * closeLength;

				if (closeVect.SqLength() < cv->GetBoundingRadiusSq()) {
					length = closeLength;
					hit = u;
				}
			}
		}
	}

	return length;
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
	GML_RECMUTEX_LOCK(qnum);

	vector<int> quads = qf->GetQuads(query.pos, query.radius);

	const int tempNum = gs->tempNum++;
	vector<int>::iterator qi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
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
	Friendly(int at) : Base(at) {}
	bool Team(int t) { return teamHandler->Ally(searchAllyteam, t); }
	bool Unit(const CUnit*) { return true; }
};

/**
 * Look for enemy units only.
 * All units are included by default.
 */
struct Enemy : public Base
{
	Enemy(int at) : Base(at) {}
	bool Team(int t) { return !teamHandler->Ally(searchAllyteam, t); }
	bool Unit(const CUnit*) { return true; }
};

/**
 * Look for enemy units which are in LOS/Radar only.
 */
struct Enemy_InLos : public Enemy
{
	Enemy_InLos(int at) : Enemy(at) {}
	bool Unit(const CUnit* u) {
		return (u->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR));
	}
};

/**
 * Look for enemy aircraft which are in LOS/Radar only.
 */
struct EnemyAircraft : public Enemy_InLos
{
	EnemyAircraft(int at) : Enemy_InLos(at) {}
	bool Unit(const CUnit* u) {
		return u->unitDef->canfly && !u->crashing && Enemy_InLos::Unit(u);
	}
};

/**
 * Look for units of any team. Enemy units must be in LOS/Radar.
 *
 * NOT SYNCED
 */
struct Friendly_All_Plus_Enemy_InLos_NOT_SYNCED
{
	bool Team(int) { return true; }
	bool Unit(const CUnit* u) {
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
		Enemy_InLos(at), cai(cai) {}
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
		Base(pos, searchRadius + uh->maxUnitRadius),
		closeDist(searchRadius), closeUnit(NULL), canBeBlind(canBeBlind) {}

	void AddUnit(CUnit* u) {
		// FIXME: use volumeBoundingRadius?
		// (more for consistency than need)
		const float dist =
			(pos - u->midPos).Length() -
			u->radius;

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



void CGameHelper::GenerateTargets(const CWeapon *weapon, CUnit* lastTarget,
                                  std::map<float,CUnit*> &targets)
{
	GML_RECMUTEX_LOCK(qnum); // GenerateTargets

	CUnit* attacker = weapon->owner;
	float radius = weapon->range;
	float3 pos = attacker->pos;
	float heightMod = weapon->heightMod;
	float aHeight = weapon->weaponPos.y;
	// how much damage the weapon deals over 1 second
	float secDamage = weapon->weaponDef->damages[0] * weapon->salvoSize / weapon->reloadTime * 30;
	bool paralyzer = !!weapon->weaponDef->damages.paralyzeDamageTime;

	std::vector<int> quads = qf->GetQuads(pos, radius + (aHeight - std::max(0.f, readmap->minheight)) * heightMod);

	int tempNum = gs->tempNum++;
	std::vector<int>::iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		for (int t = 0; t < teamHandler->ActiveAllyTeams(); ++t) {
			if (teamHandler->Ally(attacker->allyteam, t)) {
				continue;
			}
			std::list<CUnit*>::const_iterator ui;
			const std::list<CUnit*>& allyTeamUnits = qf->GetQuad(*qi).teamUnits[t];
			for (ui = allyTeamUnits.begin(); ui != allyTeamUnits.end(); ++ui) {
				CUnit* unit = *ui;
				if (unit->tempNum != tempNum && (unit->category & weapon->onlyTargetCategory)) {
					unit->tempNum = tempNum;
					if (unit->isUnderWater && !weapon->weaponDef->waterweapon) {
						continue;
					}
					if (unit->isDead) {
						continue;
					}
					float3 targPos;
					float value = 1.0f;
					unsigned short unitLos = unit->losStatus[attacker->allyteam];
					if (unitLos & LOS_INLOS) {
						targPos = unit->midPos;
					} else if (unitLos & LOS_INRADAR) {
						const float radErr = radarhandler->radarErrorSize[attacker->allyteam];
						targPos = unit->midPos + (unit->posErrorVector * radErr);
						value *= 10.0f;
					} else {
						continue;
					}
					const float modRange = radius + (aHeight - targPos.y) * heightMod;
					if ((pos - targPos).SqLength2D() <= modRange * modRange){
						float dist2d = (pos - targPos).Length2D();
						value *= (dist2d * weapon->weaponDef->proximityPriority + modRange * 0.4f + 100.0f);
						if (unitLos & LOS_INLOS) {
							value *= (secDamage + unit->health);
							if (unit == lastTarget) {
								value *= weapon->avoidTarget ? 10.0f : 0.4f;
							}

							if (paralyzer && unit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health)) {
								value *= 4.0f;
							}

							if (weapon->hasTargetWeight) {
								value *= weapon->TargetWeight(unit);
							}
						} else {
							value *= (secDamage + 10000.0f);
						}
						if (unitLos & LOS_PREVLOS) {
							value /= weapon->weaponDef->damages[unit->armorType]
											 * unit->curArmorMultiple
									 * unit->power * (0.7f + gs->randFloat() * 0.6f);
							if (unit->category & weapon->badTargetCategory) {
								value *= 100.0f;
							}
							if (unit->crashing) {
								value *= 1000.0f;
							}
						}
						targets.insert(std::pair<float, CUnit*>(value, unit));
					}
				}
			}
		}
	}
/*
#ifdef TRACE_SYNC
	tracefile << "TargetList: " << attacker->id << " " << radius << " ";
	std::map<float,CUnit*>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();++ti)
		tracefile << (ti->first) <<  " " << (ti->second)->id <<  " ";
	tracefile << "\n";
#endif
*/
}

CUnit* CGameHelper::GetClosestUnit(const float3 &pos, float searchRadius)
{
	Query::ClosestUnit_ErrorPos_NOT_SYNCED q(pos, searchRadius);
	QueryUnits(Filter::Friendly_All_Plus_Enemy_InLos_NOT_SYNCED(), q);
	return q.GetClosestUnit();
}

CUnit* CGameHelper::GetClosestEnemyUnit(const float3& pos, float searchRadius, int searchAllyteam)
{
	Query::ClosestUnit q(pos, searchRadius);
	QueryUnits(Filter::Enemy_InLos(searchAllyteam), q);
	return q.GetClosestUnit();
}

CUnit* CGameHelper::GetClosestValidTarget(const float3& pos, float searchRadius, int searchAllyteam, const CMobileCAI* cai)
{
	Query::ClosestUnit q(pos, searchRadius);
	QueryUnits(Filter::Enemy_InLos_ValidTarget(searchAllyteam, cai), q);
	return q.GetClosestUnit();
}

CUnit* CGameHelper::GetClosestEnemyUnitNoLosTest(const float3 &pos, float searchRadius,
                                                 int searchAllyteam, bool sphere, bool canBeBlind)
{
	if (sphere) { // includes target radius

		Query::ClosestUnit_InLos q(pos, searchRadius, canBeBlind);
		QueryUnits(Filter::Enemy(searchAllyteam), q);
		return q.GetClosestUnit();

	} else { // cylinder  (doesn't include target radius)

		Query::ClosestUnit_InLos_Cylinder q(pos, searchRadius, canBeBlind);
		QueryUnits(Filter::Enemy(searchAllyteam), q);
		return q.GetClosestUnit();

	}
}

CUnit* CGameHelper::GetClosestFriendlyUnit(const float3 &pos, float searchRadius, int searchAllyteam)
{
	Query::ClosestUnit q(pos, searchRadius);
	QueryUnits(Filter::Friendly(searchAllyteam), q);
	return q.GetClosestUnit();
}

CUnit* CGameHelper::GetClosestEnemyAircraft(const float3 &pos, float searchRadius, int searchAllyteam)
{
	Query::ClosestUnit q(pos, searchRadius);
	QueryUnits(Filter::EnemyAircraft(searchAllyteam), q);
	return q.GetClosestUnit();
}

void CGameHelper::GetEnemyUnits(const float3 &pos, float searchRadius, int searchAllyteam, vector<int> &found)
{
	Query::AllUnitsById q(pos, searchRadius, found);
	QueryUnits(Filter::Enemy_InLos(searchAllyteam), q);
}

void CGameHelper::GetEnemyUnitsNoLosTest(const float3 &pos, float searchRadius, int searchAllyteam, vector<int> &found)
{
	Query::AllUnitsById q(pos, searchRadius, found);
	QueryUnits(Filter::Enemy(searchAllyteam), q);
}

//////////////////////////////////////////////////////////////////////
// Miscellaneous (i.e. not yet categorized)
//////////////////////////////////////////////////////////////////////

// called by {CFlameThrower, CLaserCannon, CEmgCannon, CBeamLaser, CLightningCannon}::TryTarget()
bool CGameHelper::LineFeatureCol(const float3& start, const float3& dir, float length)
{
	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(start, dir, length, endQuad);

	CollisionQuery cq;

	for (int* qi = quads; qi != endQuad; ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);

		for (std::list<CFeature*>::const_iterator ui = quad.features.begin(); ui != quad.features.end(); ++ui) {
			CFeature* f = *ui;
			CollisionVolume* cv = f->collisionVolume;

			if (!f->blocking || !cv) {
				// NOTE: why check the blocking property?
				continue;
			}

			if (CCollisionHandler::Intersect(f, start, start + dir * length, &cq)) {
				return true;
			}
		}
	}
	return false;
}


float CGameHelper::GuiTraceRayFeature(const float3& start, const float3& dir, float length, const CFeature*& feature)
{
	float nearHit = length;

	GML_RECMUTEX_LOCK(quad); // GuiTraceRayFeature

	std::vector<int> quads = qf->GetQuadsOnRay(start, dir, length);
	std::vector<int>::iterator qi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);
		std::list<CFeature*>::const_iterator ui;

		// NOTE: switch this to custom volumes fully?
		// (not used for any LOF checks, maybe wasteful)
		for (ui = quad.features.begin(); ui != quad.features.end(); ++ui) {
			CFeature* f = *ui;

			if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam)) {
				continue;
			}
			if (f->noSelect) {
				continue;
			}

			CollisionVolume* cv = f->collisionVolume;
			const float3& midPosOffset = cv? cv->GetOffsets(): ZeroVector;
			const float3 dif = (f->midPos + midPosOffset) - start;
			float closeLength = dif.dot(dir);

			if (closeLength < 0)
				continue;
			if (closeLength > nearHit)
				continue;

			float3 closeVect = dif - dir * closeLength;
			if (closeVect.SqLength() < f->sqRadius) {
				nearHit = closeLength;
				feature = f;
			}
		}
	}

	return nearHit;
}

float3 CGameHelper::GetUnitErrorPos(const CUnit* unit, int allyteam)
{
	float3 pos = unit->midPos;
	if (teamHandler->Ally(allyteam,unit->allyteam) || (unit->losStatus[allyteam] & LOS_INLOS)) {
		// ^ it's one of our own, or it's in LOS, so don't add an error ^
	} else if ((!gameSetup || gameSetup->ghostedBuildings) && (unit->losStatus[allyteam] & LOS_PREVLOS) && !unit->mobility) {
		// ^ this is a ghosted building, so don't add an error ^
	} else if ((unit->losStatus[allyteam] & LOS_INRADAR)) {
		pos += unit->posErrorVector * radarhandler->radarErrorSize[allyteam];
	} else {
		pos += unit->posErrorVector * radarhandler->baseRadarErrorSize * 2;
	}
	return pos;
}


void CGameHelper::BuggerOff(float3 pos, float radius, bool spherical, CUnit* exclude)
{
	std::vector<CUnit*> units = qf->GetUnitsExact(pos, radius + 8, spherical);

	for (std::vector<CUnit*>::iterator ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* u = *ui;
		bool allied = true;

		if (exclude) {
			const int eAllyTeam = exclude->allyteam;
			const int uAllyTeam = u->allyteam;
			allied = (teamHandler->Ally(uAllyTeam, eAllyTeam) || teamHandler->Ally(eAllyTeam, uAllyTeam));
		}

		if (u != exclude && allied && !u->unitDef->pushResistant && !u->usingScriptMoveType) {
			u->commandAI->BuggerOff(pos, radius + 8);
		}
	}
}


float3 CGameHelper::Pos2BuildPos(const float3& pos, const UnitDef* ud)
{
	return Pos2BuildPos(BuildInfo(ud,pos,0));
}

float3 CGameHelper::Pos2BuildPos(const BuildInfo& buildInfo)
{
	float3 pos;
	if (buildInfo.GetXSize() & 2)
		pos.x = floor((buildInfo.pos.x    ) / (SQUARE_SIZE * 2)) * SQUARE_SIZE * 2 + 8;
	else
		pos.x = floor((buildInfo.pos.x + 8) / (SQUARE_SIZE * 2)) * SQUARE_SIZE * 2;

	if (buildInfo.GetZSize() & 2)
		pos.z = floor((buildInfo.pos.z    ) / (SQUARE_SIZE * 2)) * SQUARE_SIZE * 2 + 8;
	else
		pos.z = floor((buildInfo.pos.z + 8) / (SQUARE_SIZE * 2)) * SQUARE_SIZE * 2;

	pos.y = uh->GetBuildHeight(pos,buildInfo.def);
	if (buildInfo.def->floater && pos.y < 0)
		pos.y = -buildInfo.def->waterline;

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

		for (int y=0;y<radius*2;y++)
			for (int x=0;x<radius*2;x++)
			{
				SearchOffset& i = searchOffsets[y*radius*2+x];

				i.dx = x-radius;
				i.dy = y-radius;
				i.qdist = i.dx*i.dx+i.dy*i.dy;
			}

		std::sort (searchOffsets.begin(), searchOffsets.end(), SearchOffsetComparator);
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
		bi.pos = Pos2BuildPos(bi);

		if (uh->TestUnitBuildSquare(bi, feature, allyTeam) && (!feature || feature->allyteam != allyTeam)) {
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

						if (solObj && solObj->immobile && dynamic_cast<CFactory*>(solObj) && ((CFactory*)solObj)->opening) {
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

void CGameHelper::Update(void)
{
	std::list<WaitingDamage*>* wd = &waitingDamages[gs->frameNum&127];
	while (!wd->empty()) {
		WaitingDamage* w = wd->back();
		wd->pop_back();
		if (uh->units[w->target])
			uh->units[w->target]->DoDamage(w->damage, w->attacker == -1? NULL: uh->units[w->attacker], w->impulse, w->weaponId);
		delete w;
	}
}




/** @return true if there is an allied unit within
    the firing cone of <owner> (that might be hit) */
bool CGameHelper::TestAllyCone(const float3& from, const float3& weaponDir, float length, float spread, int allyteam, CUnit* owner)
{
	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(from, weaponDir, length, endQuad);

	for (int* qi = quads; qi != endQuad; ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);
		for (std::list<CUnit*>::const_iterator ui = quad.teamUnits[allyteam].begin(); ui != quad.teamUnits[allyteam].end(); ++ui) {
			CUnit* u = *ui;

			if (u == owner)
				continue;

			if (TestConeHelper(from, weaponDir, length, spread, u))
				return true;
		}
	}
	return false;
}

/** same as TestAllyCone, but looks for neutral units */
bool CGameHelper::TestNeutralCone(const float3& from, const float3& weaponDir, float length, float spread, CUnit* owner)
{
	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(from, weaponDir, length, endQuad);

	for (int* qi = quads; qi != endQuad; ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);

		for (std::list<CUnit*>::const_iterator ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			CUnit* u = *ui;

			if (u == owner)
				continue;

			if (u->IsNeutral()) {
				if (TestConeHelper(from, weaponDir, length, spread, u))
					return true;
			}
		}
	}
	return false;
}


/** helper for TestAllyCone and TestNeutralCone
    @return true if the unit u is in the firing cone, false otherwise */
bool CGameHelper::TestConeHelper(const float3& from, const float3& weaponDir, float length, float spread, const CUnit* u)
{
	// account for any offset, since we want to know if our shots might hit
	float3 unitDir = (u->midPos + u->collisionVolume->GetOffsets()) - from;
	// weaponDir defines the center of the cone
	float closeLength = unitDir.dot(weaponDir);

	if (closeLength <= 0)
		return false;
	if (closeLength > length)
		closeLength = length;

	float3 closeVect = unitDir - weaponDir * closeLength;
	// NOTE: same caveat wrt. use of volumeBoundingRadius
	// as for ::Explosion(), this will result in somewhat
	// over-conservative tests for non-spherical volumes
	float r = u->collisionVolume->GetBoundingRadius() + spread * closeLength + 1;

	return (closeVect.SqLength() < r * r);
}




/** @return true if there is an allied unit within
    the firing trajectory of <owner> (that might be hit) */
bool CGameHelper::TestTrajectoryAllyCone(const float3& from, const float3& flatdir, float length, float linear, float quadratic, float spread, float baseSize, int allyteam, CUnit* owner)
{
	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(from, flatdir, length, endQuad);

	for (int* qi = quads; qi != endQuad; ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);
		for (std::list<CUnit*>::const_iterator ui = quad.teamUnits[allyteam].begin(); ui != quad.teamUnits[allyteam].end(); ++ui) {
			CUnit* u = *ui;

			if (u == owner)
				continue;

			if (TestTrajectoryConeHelper(from, flatdir, length, linear, quadratic, spread, baseSize, u))
				return true;
		}
	}
	return false;
}

/** same as TestTrajectoryAllyCone, but looks for neutral units */
bool CGameHelper::TestTrajectoryNeutralCone(const float3& from, const float3& flatdir, float length, float linear, float quadratic, float spread, float baseSize, CUnit* owner)
{
	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(from, flatdir, length, endQuad);

	for (int* qi = quads; qi != endQuad; ++qi) {
		const CQuadField::Quad& quad = qf->GetQuad(*qi);
		for (std::list<CUnit*>::const_iterator ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			CUnit* u = *ui;

			if (u == owner)
				continue;

			if (u->IsNeutral()) {
				if (TestTrajectoryConeHelper(from, flatdir, length, linear, quadratic, spread, baseSize, u))
					return true;
			}
		}
	}
	return false;
}


/** helper for TestTrajectoryAllyCone and TestTrajectoryNeutralCone
    @return true if the unit u is in the firing trajectory, false otherwise */
bool CGameHelper::TestTrajectoryConeHelper(const float3& from, const float3& flatdir, float length, float linear, float quadratic, float spread, float baseSize, const CUnit* u)
{
	const CollisionVolume* cv = u->collisionVolume;
	float3 dif = (u->midPos + cv->GetOffsets()) - from;
	float3 flatdif(dif.x, 0, dif.z);
	float closeFlatLength = flatdif.dot(flatdir);

	if (closeFlatLength <= 0)
		return false;
	if (closeFlatLength > length)
		closeFlatLength = length;

	if (fabs(linear - quadratic * closeFlatLength) < 0.15f) {
		// relatively flat region -> use approximation
		dif.y -= (linear + quadratic * closeFlatLength) * closeFlatLength;

		// NOTE: overly conservative for non-spherical volumes
		float3 closeVect = dif - flatdir * closeFlatLength;
		float r = cv->GetBoundingRadius() + spread * closeFlatLength + baseSize;
		if (closeVect.SqLength() < r * r) {
			return true;
		}
	} else {
		float3 newfrom = from + flatdir * closeFlatLength;
		newfrom.y += (linear + quadratic * closeFlatLength) * closeFlatLength;
		float3 dir = flatdir;
		dir.y = linear + quadratic * closeFlatLength;
		dir.Normalize();

		dif = (u->midPos + cv->GetOffsets()) - newfrom;
		float closeLength = dif.dot(dir);

		// NOTE: overly conservative for non-spherical volumes
		float3 closeVect = dif - dir * closeLength;
		float r = cv->GetBoundingRadius() + spread * closeFlatLength + baseSize;
		if (closeVect.SqLength() < r * r) {
			return true;
		}
	}
	return false;
}
