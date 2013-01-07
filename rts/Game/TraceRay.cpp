/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Camera.h"
#include "GlobalUnsynced.h"
#include "TraceRay.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "System/myMath.h"


//////////////////////////////////////////////////////////////////////
// Local/Helper functions
//////////////////////////////////////////////////////////////////////

/**
 * helper for TestCone
 * @return true if object <o> is in the firing cone, false otherwise
 */
inline bool TestConeHelper(const float3& from, const float3& weaponDir, float length, float spread, const CSolidObject* obj)
{
	// account for any offset, since we want to know if our shots might hit
	const float3 objDir = (obj->midPos + obj->collisionVolume->GetOffsets()) - from;

	// weaponDir defines the center of the cone
	float closeLength = objDir.dot(weaponDir);

	if (closeLength <= 0)
		return false;
	if (closeLength > length)
		closeLength = length;

	const float3 closeVect = objDir - weaponDir * closeLength;

	// NOTE: same caveat wrt. use of volumeBoundingRadius
	// as for ::Explosion(), this will result in somewhat
	// over-conservative tests for non-spherical volumes
	const float r = obj->collisionVolume->GetBoundingRadius() + spread * closeLength + 1;

	return (closeVect.SqLength() < r * r);
}

/**
 * helper for TestTrajectoryCone
 * @return true if object <o> is in the firing trajectory, false otherwise
 */
inline bool TestTrajectoryConeHelper(
	const float3& from,
	const float3& flatdir,
	float length,
	float linear,
	float quadratic,
	float spread,
	float baseSize,
	const CSolidObject* o)
{
	const CollisionVolume* cv = o->collisionVolume;
	float3 dif = (o->midPos + cv->GetOffsets()) - from;
	const float3 flatdif(dif.x, 0, dif.z);
	float closeFlatLength = flatdif.dot(flatdir);

	if (closeFlatLength <= 0)
		return false;
	if (closeFlatLength > length)
		closeFlatLength = length;

	if (math::fabs(linear - quadratic * closeFlatLength) < 0.15f) {
		// relatively flat region -> use approximation
		dif.y -= (linear + quadratic * closeFlatLength) * closeFlatLength;

		// NOTE: overly conservative for non-spherical volumes
		const float3 closeVect = dif - flatdir * closeFlatLength;
		const float r = cv->GetBoundingRadius() + spread * closeFlatLength + baseSize;
		if (closeVect.SqLength() < r * r) {
			return true;
		}
	} else {
		float3 newfrom = from + flatdir * closeFlatLength;
		newfrom.y += (linear + quadratic * closeFlatLength) * closeFlatLength;
		float3 dir = flatdir;
		dir.y = linear + quadratic * closeFlatLength;
		dir.Normalize();

		dif = (o->midPos + cv->GetOffsets()) - newfrom;
		const float closeLength = dif.dot(dir);

		// NOTE: overly conservative for non-spherical volumes
		const float3 closeVect = dif - dir * closeLength;
		const float r = cv->GetBoundingRadius() + spread * closeFlatLength + baseSize;
		if (closeVect.SqLength() < r * r) {
			return true;
		}
	}
	return false;
}



//////////////////////////////////////////////////////////////////////
// Raytracing
//////////////////////////////////////////////////////////////////////

namespace TraceRay {

// called by {CRifle, CBeamLaser, CLightningCannon}::Fire(), CWeapon::HaveFreeLineOfFire(), and Skirmish AIs
float TraceRay(
	const float3& start,
	const float3& dir,
	float length,
	int collisionFlags,
	const CUnit* owner,
	CUnit*& hitUnit,
	CFeature*& hitFeature
) {
	const bool ignoreEnemies  = ((collisionFlags & Collision::NOENEMIES   ) != 0);
	const bool ignoreAllies   = ((collisionFlags & Collision::NOFRIENDLIES) != 0);
	const bool ignoreFeatures = ((collisionFlags & Collision::NOFEATURES  ) != 0);
	const bool ignoreNeutrals = ((collisionFlags & Collision::NONEUTRALS  ) != 0);
	const bool ignoreGround   = ((collisionFlags & Collision::NOGROUND    ) != 0);

	const bool ignoreUnits = ignoreEnemies && ignoreAllies && ignoreNeutrals;

	hitFeature = NULL;
	hitUnit = NULL;

	if (dir == ZeroVector) {
		return -1.0f;
	}

	if (!ignoreFeatures || !ignoreUnits) {
		GML_RECMUTEX_LOCK(quad); // TraceRay
		CollisionQuery cq;

		int* begQuad = NULL;
		int* endQuad = NULL;

		qf->GetQuadsOnRay(start, dir, length, begQuad, endQuad);

		// feature intersection
		if (!ignoreFeatures) {
			for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
				const CQuadField::Quad& quad = qf->GetQuad(*quadPtr);

				for (std::list<CFeature*>::const_iterator ui = quad.features.begin(); ui != quad.features.end(); ++ui) {
					CFeature* f = *ui;

					// NOTE:
					//     if f is non-blocking, ProjectileHandler will not test
					//     for collisions with projectiles so we can skip it here
					if (!f->blocking)
						continue;

					if (CCollisionHandler::DetectHit(f, start, start + dir * length, &cq, true)) {
						const float3& intPos = (cq.IngressHit())? cq.GetIngressPos(): cq.GetEgressPos();
						const float len = (intPos - start).dot(dir); // same as (intPos - start).Length()

						// we want the closest feature (intersection point) on the ray
						if (len < length) {
							length = len;
							hitFeature = f;
						}
					}
				}
			}
		}

		// unit intersection
		if (!ignoreUnits) {
			for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
				const CQuadField::Quad& quad = qf->GetQuad(*quadPtr);

				for (std::list<CUnit*>::const_iterator ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
					CUnit* u = *ui;

					if (u == owner)
						continue;
					if (ignoreAllies && u->allyteam == owner->allyteam)
						continue;
					if (ignoreNeutrals && u->IsNeutral())
						continue;
					if (ignoreEnemies && u->allyteam != owner->allyteam)
						continue;

					if (CCollisionHandler::DetectHit(u, start, start + dir * length, &cq, true)) {
						const float3& intPos = (cq.IngressHit())? cq.GetIngressPos(): cq.GetEgressPos();
						const float len = (intPos - start).dot(dir); // same as (intPos - start).Length()

						// we want the closest unit (intersection point) on the ray
						if (len < length) {
							length = len;
							hitUnit = u;
						}
					}
				}
			}
			if (hitUnit)
				hitFeature = NULL;
		}
	}

	if (!ignoreGround) {
		// ground intersection
		const float groundLength = ground->LineGroundCol(start, start + dir * length);
		if (length > groundLength && groundLength > 0) {
			length = groundLength;
			hitUnit = NULL;
			hitFeature = NULL;
		}
	}

	return length;
}


float GuiTraceRay(
	const float3& start,
	const float3& dir,
	float length,
	bool useRadar,
	const CUnit* exclude,
	CUnit*& hitUnit,
	CFeature*& hitFeature,
	bool groundOnly
) {
	hitUnit = NULL;
	hitFeature = NULL;

	if (dir == ZeroVector)
		return -1.0f;

	// ground intersection
	const float origlength = length;
	const float groundLength = ground->LineGroundCol(start, start + dir * origlength, false);
	float length2 = length;

	if (groundOnly)
		return groundLength;

	GML_RECMUTEX_LOCK(quad); // GuiTraceRay

	int* begQuad = NULL;
	int* endQuad = NULL;
	bool hitFactory = false;
	CollisionQuery cq;

	qf->GetQuadsOnRay(start, dir, length, begQuad, endQuad);

	std::list<CUnit*>::const_iterator ui;
	std::list<CFeature*>::const_iterator fi;

	for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
		const CQuadField::Quad& quad = qf->GetQuad(*quadPtr);

		// Unit Intersection
		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			CUnit* unit = *ui;

			const bool unitIsEnemy = !teamHandler->Ally(unit->allyteam, gu->myAllyTeam);
			const bool unitOnRadar = (useRadar && radarhandler->InRadar(unit, gu->myAllyTeam));
			const bool unitInSight = (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR));
			const bool unitVisible = !unitIsEnemy || unitOnRadar || unitInSight || gu->spectatingFullView;

			if (unit == exclude)
				continue;
			if (!unitVisible)
				continue;

			CollisionVolume cv(unit->collisionVolume);

			if (unit->isIcon || (!unitInSight && unitOnRadar && unitIsEnemy)) {
				// for iconified units, just pretend the collision
				// volume is a sphere of radius <unit->IconRadius>
				// (count radar blips as such too)
				cv.InitSphere(unit->iconRadius);
			}

			if (CCollisionHandler::MouseHit(unit, start, start + dir * origlength, &cv, &cq)) {
				// get the distance to the ray-volume ingress point
				const float3& ingressPos = (cq.IngressHit())? cq.GetIngressPos() : cq.GetEgressPos();
				const float3&  egressPos = (cq.EgressHit())?  cq.GetEgressPos()  : cq.GetIngressPos();
				const float ingressDist  = (ingressPos - start).dot(dir); // same as (intPos  - start).Length()
				const float  egressDist  = ( egressPos - start).dot(dir); // same as (intPos2 - start).Length()
				const bool isFactory = unit->unitDef->IsFactoryUnit();

				// give units in a factory higher priority than the factory itself
				if (!hitUnit ||
					(isFactory && ((hitFactory && ingressDist < length) || (!hitFactory && egressDist < length))) ||
					(!isFactory && ((hitFactory && ingressDist < length2) || (!hitFactory && ingressDist < length)))) {
						hitFactory = isFactory;
						length = ingressDist;
						length2 = egressDist;
						hitUnit = unit;
						hitFeature = NULL;
				}
			}
		}

		// Feature Intersection
		// NOTE: switch this to custom volumes fully?
		// (not used for any LOF checks, maybe wasteful)
		for (fi = quad.features.begin(); fi != quad.features.end(); ++fi) {
			CFeature* f = *fi;

			// FIXME add useradar?
			if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam))
				continue;
			if (f->noSelect)
				continue;

			if (CCollisionHandler::DetectHit(f, start, start + dir * origlength, &cq, true)) {
				const float3& ingressPos = (cq.IngressHit())? cq.GetIngressPos() : cq.GetEgressPos();
				const float ingressDist = (ingressPos - start).dot(dir); // same as (intPos - start).Length()

				// we want the closest feature (intersection point) on the ray
				// give features in a factory (?) higher priority than the factory itself
				if (!hitUnit ||
					((hitFactory && ingressDist < length2) || (!hitFactory && ingressDist < length))) {
					hitFactory = false;
					length = ingressDist;
					hitFeature = f;
					hitUnit = NULL;
				}
			}
		}
	}

	if ((groundLength > 0.0f) && ((groundLength + 200.0f) < length)) {
		length     = groundLength;
		hitUnit    = NULL;
		hitFeature = NULL;
	}

	return length;
}




// called by CWeapon::TryTarget()
bool LineFeatureCol(const float3& start, const float3& dir, float length)
{
	GML_RECMUTEX_LOCK(quad); // LineFeatureCol

	int* begQuad = NULL;
	int* endQuad = NULL;

	if (qf->GetQuadsOnRay(start, dir, length, begQuad, endQuad) == 0)
		return false;

	CollisionQuery cq;

	for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
		const CQuadField::Quad& quad = qf->GetQuad(*quadPtr);

		for (std::list<CFeature*>::const_iterator ui = quad.features.begin(); ui != quad.features.end(); ++ui) {
			const CFeature* f = *ui;

			if (!f->blocking)
				continue;

			if (CCollisionHandler::DetectHit(f, start, start + dir * length, &cq, true)) {
				return true;
			}
		}
	}

	return false;
}



bool TestCone(
	const float3& from,
	const float3& dir,
	float length,
	float spread,
	int allyteam,
	bool testFriendly,
	bool testNeutral,
	bool testFeatures,
	CUnit* owner)
{
	GML_RECMUTEX_LOCK(quad); // TestCone

	int* begQuad = NULL;
	int* endQuad = NULL;

	if (qf->GetQuadsOnRay(from, dir, length, begQuad, endQuad) == 0)
		return true;

	for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
		const CQuadField::Quad& quad = qf->GetQuad(*quadPtr);

		if (testFriendly) {
			const std::list<CUnit*>& units = quad.teamUnits[allyteam];
			      std::list<CUnit*>::const_iterator unitsIt;

			for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
				const CUnit* u = *unitsIt;

				if (u == owner)
					continue;

				if (TestConeHelper(from, dir, length, spread, u))
					return true;
			}
		}

		if (testNeutral) {
			const std::list<CUnit*>& units = quad.units;
			      std::list<CUnit*>::const_iterator unitsIt;

			for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
				const CUnit* u = *unitsIt;

				if (u == owner)
					continue;
				if (!u->IsNeutral())
					continue;

				if (TestConeHelper(from, dir, length, spread, u))
					return true;
			}
		}

		if (testFeatures) {
			const std::list<CFeature*>& features = quad.features;
			      std::list<CFeature*>::const_iterator featuresIt;

			for (featuresIt = features.begin(); featuresIt != features.end(); ++featuresIt) {
				const CFeature* f = *featuresIt;

				if (!f->blocking)
					continue;

				if (TestConeHelper(from, dir, length, spread, f))
					return true;
			}
		}
	}

	return false;
}



bool TestTrajectoryCone(
	const float3& from,
	const float3& dir,
	float length,
	float linear,
	float quadratic,
	float spread,
	float baseSize,
	int allyteam,
	bool testFriendly,
	bool testNeutral,
	bool testFeatures,
	CUnit* owner)
{
	GML_RECMUTEX_LOCK(quad); // TestTrajectoryCone

	int* begQuad = NULL;
	int* endQuad = NULL;

	if (qf->GetQuadsOnRay(from, dir, length, begQuad, endQuad) == 0)
		return true;

	for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
		const CQuadField::Quad& quad = qf->GetQuad(*quadPtr);

		// friendly units in this quad
		if (testFriendly) {
			const std::list<CUnit*>& units = quad.teamUnits[allyteam];
			      std::list<CUnit*>::const_iterator unitsIt;

			for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
				const CUnit* u = *unitsIt;

				if (u == owner)
					continue;

				if (TestTrajectoryConeHelper(from, dir, length, linear, quadratic, spread, baseSize, u))
					return true;
			}
		}

		// neutral units in this quad
		if (testNeutral) {
			const std::list<CUnit*>& units = quad.units;
			      std::list<CUnit*>::const_iterator unitsIt;

			for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
				const CUnit* u = *unitsIt;

				if (u == owner)
					continue;
				if (!u->IsNeutral())
					continue;

				if (TestTrajectoryConeHelper(from, dir, length, linear, quadratic, spread, baseSize, u))
					return true;
			}
		}

		// features in this quad
		if (testFeatures) {
			const std::list<CFeature*>& features = quad.features;
			      std::list<CFeature*>::const_iterator featuresIt;

			for (featuresIt = features.begin(); featuresIt != features.end(); ++featuresIt) {
				const CFeature* f = *featuresIt;

				if (!f->blocking)
					continue;

				if (TestTrajectoryConeHelper(from, dir, length, linear, quadratic, spread, baseSize, f))
					return true;
			}
		}
	}

	return false;
}



} //namespace TraceRay
