/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TraceRay.h"
#include "Camera.h"
#include "GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GeometricObjects.h"
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
inline static bool TestConeHelper(
	const float3& pos3D,
	const float3& dir3D,
	const float length,
	const float spread,
	const CSolidObject* obj)
{
	const CollisionVolume* cv = obj->collisionVolume;

	const float3 objVec3D = cv->GetWorldSpacePos(obj, ZeroVector) - pos3D;
	const float  objDst1D = Clamp(objVec3D.dot(dir3D), 0.0f, length);
	const float  coneSize = math::fabs(objDst1D) * spread + 1.0f;

	// theoretical impact position assuming no spread
	const float3 expVec3D = dir3D * objDst1D;
	const float3 expPos3D = pos3D + expVec3D;

	bool ret = false;

	if (obj->GetBlockingMapID() < unitHandler->MaxUnits()) {
		if (!ret) { ret = ((cv->GetPointSurfaceDistance(static_cast<const CUnit*>(obj), NULL,    pos3D) - coneSize) <= 0.0f); }
		if (!ret) { ret = ((cv->GetPointSurfaceDistance(static_cast<const CUnit*>(obj), NULL, expPos3D) - coneSize) <= 0.0f); }
	} else {
		if (!ret) { ret = ((cv->GetPointSurfaceDistance(static_cast<const CFeature*>(obj), NULL,    pos3D) - coneSize) <= 0.0f); }
		if (!ret) { ret = ((cv->GetPointSurfaceDistance(static_cast<const CFeature*>(obj), NULL, expPos3D) - coneSize) <= 0.0f); }
	}

	if (globalRendering->drawdebugtraceray && Threading::IsSimThread()) {
		#define go geometricObjects

		if (ret) {
			go->SetColor(go->AddLine(expPos3D - (UpVector * expPos3D.dot(UpVector)), expPos3D, 3, 1, GAME_SPEED), 1.0f, 0.0f, 0.0f, 1.0f);
		} else {
			go->SetColor(go->AddLine(expPos3D - (UpVector * expPos3D.dot(UpVector)), expPos3D, 3, 1, GAME_SPEED), 0.0f, 1.0f, 0.0f, 1.0f);
		}

		#undef go
	}

	return ret;
}

/**
 * helper for TestTrajectoryCone
 * @return true if object <o> is in the firing trajectory, false otherwise
 */
inline static bool TestTrajectoryConeHelper(
	const float3& pos3D,
	const float3& dir2D,
	float length,
	float linear,
	float quadratic,
	float spread,
	float baseSize,
	const CSolidObject* obj)
{
	// trajectory is a parabola f(x)=a*x*x + b*x with
	// parameters a = quadratic, b = linear, and c = 0
	// (x = objDst1D, negative values represent objects
	// "behind" the testee whose collision volumes might
	// still be intersected by its trajectory arc)
	//
	// firing-cone is centered along dir2D with radius
	// <x * spread + baseSize> (usually baseSize != 0
	// so weapons with spread = 0 will test against a
	// cylinder, not an infinitely thin line as safety
	// measure against friendly-fire damage in tightly
	// packed unit groups)
	//
	// return true iff the world-space point <x, f(x)>
	// lies on or inside the object's collision volume
	// (where 'x' is actually the projected xz-distance
	// to the object's colvol-center along dir2D)
	//
	// !NOTE!:
	//   THE TRAJECTORY CURVE MIGHT STILL INTERSECT
	//   EVEN WHEN <x, f(x)> DOES NOT LIE INSIDE CV
	//   SO THIS CAN GENERATE FALSE NEGATIVES
	const CollisionVolume* cv = obj->collisionVolume;

	const float3 objVec3D = cv->GetWorldSpacePos(obj, ZeroVector) - pos3D;
	const float  objDst1D = Clamp(objVec3D.dot(dir2D), 0.0f, length);
	const float  coneSize = math::fabs(objDst1D) * spread + baseSize;

	// theoretical impact position assuming no spread
	// note that unlike TestConeHelper these positions
	// lie along curve f(x) here, not a straight line
	// (if 1D object-distance is 0, pos3D == expPos3D)
	const float3 expVec2D = dir2D * objDst1D;
	const float3 expPos2D = pos3D + expVec2D;
	const float3 expPos3D = expPos2D + (UpVector * (quadratic * objDst1D * objDst1D + linear * objDst1D));

	bool ret = false;

	if (obj->GetBlockingMapID() < unitHandler->MaxUnits()) {
		// first test the muzzle-position, then the impact-position
		// (if neither is inside obstacle's CV, the weapon can fire)
		if (!ret) { ret = ((cv->GetPointSurfaceDistance(static_cast<const CUnit*>(obj), NULL,    pos3D) - coneSize) <= 0.0f); }
		if (!ret) { ret = ((cv->GetPointSurfaceDistance(static_cast<const CUnit*>(obj), NULL, expPos3D) - coneSize) <= 0.0f); }
	} else {
		if (!ret) { ret = ((cv->GetPointSurfaceDistance(static_cast<const CFeature*>(obj), NULL,    pos3D) - coneSize) <= 0.0f); }
		if (!ret) { ret = ((cv->GetPointSurfaceDistance(static_cast<const CFeature*>(obj), NULL, expPos3D) - coneSize) <= 0.0f); }
	}

	if (globalRendering->drawdebugtraceray && Threading::IsSimThread()) {
		// FIXME? seems to under-estimate gravity near edge of range
		// (place objects along trajectory of a cannon to visualize)
		#define go geometricObjects

		if (ret) {
			go->SetColor(go->AddLine(expPos2D, expPos3D, 3, 1, GAME_SPEED), 1.0f, 0.0f, 0.0f, 1.0f);
		} else {
			go->SetColor(go->AddLine(expPos2D, expPos3D, 3, 1, GAME_SPEED), 0.0f, 1.0f, 0.0f, 1.0f);
		}

		#undef go
	}

	return ret;
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
	int avoidFlags,
	const CUnit* owner,
	CUnit*& hitUnit,
	CFeature*& hitFeature,
	CollisionQuery* hitColQuery
) {
	const bool ignoreEnemies  = ((avoidFlags & Collision::NOENEMIES   ) != 0);
	const bool ignoreAllies   = ((avoidFlags & Collision::NOFRIENDLIES) != 0);
	const bool ignoreFeatures = ((avoidFlags & Collision::NOFEATURES  ) != 0);
	const bool ignoreNeutrals = ((avoidFlags & Collision::NONEUTRALS  ) != 0);
	const bool ignoreGround   = ((avoidFlags & Collision::NOGROUND    ) != 0);

	const bool ignoreUnits = ignoreEnemies && ignoreAllies && ignoreNeutrals;

	hitFeature = NULL;
	hitUnit = NULL;

	if (dir == ZeroVector)
		return -1.0f;

	if (!ignoreFeatures || !ignoreUnits) {
		CollisionQuery cq;

		int* begQuad = NULL;
		int* endQuad = NULL;

		quadField->GetQuadsOnRay(start, dir, length, begQuad, endQuad);

		// locally point somewhere non-NULL; we cannot pass hitColQuery
		// to DetectHit directly because each call resets it internally
		if (hitColQuery == NULL)
			hitColQuery = &cq;

		// feature intersection
		if (!ignoreFeatures) {
			for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
				const CQuadField::Quad& quad = quadField->GetQuad(*quadPtr);

				for (std::list<CFeature*>::const_iterator ui = quad.features.begin(); ui != quad.features.end(); ++ui) {
					CFeature* f = *ui;

					// NOTE:
					//     if f is non-blocking, ProjectileHandler will not test
					//     for collisions with projectiles so we can skip it here
					if (!f->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
						continue;

					if (CCollisionHandler::DetectHit(f, start, start + dir * length, &cq, true)) {
						const float len = cq.GetHitPosDist(start, dir);

						// we want the closest feature (intersection point) on the ray
						if (len < length) {
							length = len;
							hitFeature = f;
							*hitColQuery = cq;
						}
					}
				}
			}
		}

		// unit intersection
		if (!ignoreUnits) {
			for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
				const CQuadField::Quad& quad = quadField->GetQuad(*quadPtr);

				for (std::list<CUnit*>::const_iterator ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
					CUnit* u = *ui;

					if (u == owner)
						continue;
					if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
						continue;
					if (ignoreAllies && u->allyteam == owner->allyteam)
						continue;
					if (ignoreNeutrals && u->IsNeutral())
						continue;
					if (ignoreEnemies && u->allyteam != owner->allyteam)
						continue;

					if (CCollisionHandler::DetectHit(u, start, start + dir * length, &cq, true)) {
						const float len = cq.GetHitPosDist(start, dir);

						// we want the closest unit (intersection point) on the ray
						if (len < length) {
							length = len;
							hitUnit = u;
							*hitColQuery = cq;
						}
					}
				}
			}

			if (hitUnit != NULL) {
				hitFeature = NULL;
			}
		}
	}

	if (!ignoreGround) {
		// ground intersection
		const float groundLength = CGround::LineGroundCol(start, start + dir * length);

		if (length > groundLength && groundLength > 0.0f) {
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
	const float length,
	const CUnit* exclude,
	CUnit*& hitUnit,
	CFeature*& hitFeature,
	bool useRadar,
	bool groundOnly,
	bool ignoreWater
) {
	hitUnit = NULL;
	hitFeature = NULL;

	if (dir == ZeroVector)
		return -1.0f;

	// ground and water-plane intersection
	const float guiRayLength = length;
	const float groundRayLength = CGround::LineGroundCol(start, start + dir * guiRayLength, false);
	const float waterRayLength = math::floorf(math::fabs(start.y / std::min(dir.y, -0.00001f)));

	float minRayLength = groundRayLength;
	float minIngressDist = length;
	float minEgressDist = length;

	bool hitFactory = false;

	// if ray cares about water, take minimum
	// of distance to ground and water surface
	if (!ignoreWater)
		minRayLength = std::min(groundRayLength, waterRayLength);
	if (groundOnly)
		return minRayLength;

	int* begQuad = NULL;
	int* endQuad = NULL;

	quadField->GetQuadsOnRay(start, dir, length, begQuad, endQuad);

	std::list<CUnit*>::const_iterator ui;
	std::list<CFeature*>::const_iterator fi;

	CollisionQuery cq;

	for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
		const CQuadField::Quad& quad = quadField->GetQuad(*quadPtr);

		// Unit Intersection
		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			CUnit* unit = *ui;

			const bool unitIsEnemy = !teamHandler->Ally(unit->allyteam, gu->myAllyTeam);
			const bool unitOnRadar = (useRadar && radarHandler->InRadar(unit, gu->myAllyTeam));
			const bool unitInSight = (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR));
			const bool unitVisible = !unitIsEnemy || unitOnRadar || unitInSight || gu->spectatingFullView;

			if (unit == exclude)
				continue;
			// test this bit only in synced traces, rely on noSelect here
			if (false && !unit->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
				continue;
			if (unit->noSelect)
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

			if (CCollisionHandler::MouseHit(unit, start, start + dir * guiRayLength, &cv, &cq)) {
				// get the distance to the ray-volume ingress point
				// (not likely to generate inside-hit special cases)
				const float ingressDist = cq.GetIngressPosDist(start, dir);
				const float  egressDist = cq.GetEgressPosDist(start, dir);

				const bool factoryUnderCursor = unit->unitDef->IsFactoryUnit();
				const bool factoryHitBeforeUnit = ((hitFactory && ingressDist < minIngressDist) || (!hitFactory &&  egressDist < minIngressDist));
				const bool unitHitInsideFactory = ((hitFactory && ingressDist <  minEgressDist) || (!hitFactory && ingressDist < minIngressDist));

				// give units in a factory higher priority than the factory itself
				if (hitUnit == NULL || (factoryUnderCursor && factoryHitBeforeUnit) || (!factoryUnderCursor && unitHitInsideFactory)) {
					hitFactory = factoryUnderCursor;
					minIngressDist = ingressDist;
					minEgressDist = egressDist;

					hitUnit = unit;
					hitFeature = NULL;
				}
			}
		}

		// Feature Intersection
		for (fi = quad.features.begin(); fi != quad.features.end(); ++fi) {
			CFeature* f = *fi;

			if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam))
				continue;
			// test this bit only in synced traces, rely on noSelect here
			if (false && !f->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
				continue;
			if (f->noSelect)
				continue;

			if (CCollisionHandler::DetectHit(f, start, start + dir * guiRayLength, &cq, true)) {
				const float hitDist = cq.GetHitPosDist(start, dir);

				const bool factoryHitBeforeUnit = ( hitFactory && hitDist <  minEgressDist);
				const bool unitHitInsideFactory = (!hitFactory && hitDist < minIngressDist);

				// we want the closest feature (intersection point) on the ray
				// give features in a factory (?) higher priority than the factory itself
				if (hitUnit == NULL || factoryHitBeforeUnit || unitHitInsideFactory) {
					hitFactory = false;
					minIngressDist = hitDist;

					hitFeature = f;
					hitUnit = NULL;
				}
			}
		}
	}

	if ((minRayLength > 0.0f) && ((minRayLength + 200.0f) < minIngressDist)) {
		minIngressDist = minRayLength;

		hitUnit    = NULL;
		hitFeature = NULL;
	}

	return minIngressDist;
}


bool TestCone(
	const float3& from,
	const float3& dir,
	float length,
	float spread,
	int allyteam,
	int avoidFlags,
	CUnit* owner)
{
	int* begQuad = NULL;
	int* endQuad = NULL;

	if (quadField->GetQuadsOnRay(from, dir, length, begQuad, endQuad) == 0)
		return true;

	const bool ignoreAllies   = ((avoidFlags & Collision::NOFRIENDLIES) != 0);
	const bool ignoreNeutrals = ((avoidFlags & Collision::NONEUTRALS  ) != 0);
	const bool ignoreFeatures = ((avoidFlags & Collision::NOFEATURES  ) != 0);

	for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
		const CQuadField::Quad& quad = quadField->GetQuad(*quadPtr);

		if (!ignoreAllies) {
			const std::list<CUnit*>& units = quad.teamUnits[allyteam];
			      std::list<CUnit*>::const_iterator unitsIt;

			for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
				const CUnit* u = *unitsIt;

				if (u == owner)
					continue;
				if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				if (TestConeHelper(from, dir, length, spread, u))
					return true;
			}
		}

		if (!ignoreNeutrals) {
			const std::list<CUnit*>& units = quad.units;
			      std::list<CUnit*>::const_iterator unitsIt;

			for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
				const CUnit* u = *unitsIt;

				if (u == owner)
					continue;
				if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;
				if (!u->IsNeutral())
					continue;

				if (TestConeHelper(from, dir, length, spread, u))
					return true;
			}
		}

		if (!ignoreFeatures) {
			const std::list<CFeature*>& features = quad.features;
			      std::list<CFeature*>::const_iterator featuresIt;

			for (featuresIt = features.begin(); featuresIt != features.end(); ++featuresIt) {
				const CFeature* f = *featuresIt;

				if (!f->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
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
	int allyteam,
	int avoidFlags,
	CUnit* owner)
{
	int* begQuad = NULL;
	int* endQuad = NULL;

	if (quadField->GetQuadsOnRay(from, dir, length, begQuad, endQuad) == 0)
		return true;

	const bool ignoreAllies   = ((avoidFlags & Collision::NOFRIENDLIES) != 0);
	const bool ignoreNeutrals = ((avoidFlags & Collision::NONEUTRALS  ) != 0);
	const bool ignoreFeatures = ((avoidFlags & Collision::NOFEATURES  ) != 0);

	for (int* quadPtr = begQuad; quadPtr != endQuad; ++quadPtr) {
		const CQuadField::Quad& quad = quadField->GetQuad(*quadPtr);

		// friendly units in this quad
		if (!ignoreAllies) {
			const std::list<CUnit*>& units = quad.teamUnits[allyteam];
			      std::list<CUnit*>::const_iterator unitsIt;

			for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
				const CUnit* u = *unitsIt;

				if (u == owner)
					continue;
				if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				if (TestTrajectoryConeHelper(from, dir, length, linear, quadratic, spread, 0.0f, u)) {
					return true;
				}
			}
		}

		// neutral units in this quad
		if (!ignoreNeutrals) {
			const std::list<CUnit*>& units = quad.units;
			      std::list<CUnit*>::const_iterator unitsIt;

			for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
				const CUnit* u = *unitsIt;

				if (u == owner)
					continue;
				if (!u->IsNeutral())
					continue;
				if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				if (TestTrajectoryConeHelper(from, dir, length, linear, quadratic, spread, 0.0f, u))
					return true;
			}
		}

		// features in this quad
		if (!ignoreFeatures) {
			const std::list<CFeature*>& features = quad.features;
			      std::list<CFeature*>::const_iterator featuresIt;

			for (featuresIt = features.begin(); featuresIt != features.end(); ++featuresIt) {
				const CFeature* f = *featuresIt;

				if (!f->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				if (TestTrajectoryConeHelper(from, dir, length, linear, quadratic, spread, 0.0f, f))
					return true;
			}
		}
	}

	return false;
}



} //namespace TraceRay
