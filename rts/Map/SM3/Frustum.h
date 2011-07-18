/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include "Vector3.h"
#include "Plane.h"

/**
 * Terrain Renderer using texture splatting and geomipmapping.
 * The frustum clips the polygons against a certain convex space.
 */
class Frustum
{
public:
	void CalcCameraPlanes(Vector3* base, Vector3* right, Vector3* up, Vector3* front, float tanHalfFov, float aspect);
	void InversePlanes ();

	enum VisType { Inside, Outside, Partial };
	VisType IsBoxVisible(const Vector3& min, const Vector3& max); ///< 3D test
	VisType IsPointVisible(const Vector3& pt);

	void Draw();

	std::vector<Plane> planes;
	Vector3 base;
	Vector3 pos[4];
	Vector3 front;
	Vector3 right;
	Vector3 up;
};

#endif // _FRUSTUM_H_
