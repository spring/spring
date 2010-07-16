/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include "Plane.h"

/**
 * Terrain Renderer using texture splatting and geomipmapping.
 * The frustum clips the polygons against a certain convex space.
 */
class Frustum
{
public:
	void CalcCameraPlanes (Vector3 *base, Vector3 *right, Vector3* up, Vector3* front, float tanHalfFov, float aspect); // should at least have 
	void InversePlanes ();

	enum VisType { Inside, Outside, Partial };
	VisType IsBoxVisible (const Vector3& min, const Vector3& max); ///<s 3D test
	VisType IsPointVisible (const Vector3& pt);

	std::vector<Plane> planes;
	Vector3 base, pos[4], front, right, up;
	void Draw ();
};

#endif // _FRUSTUM_H_
