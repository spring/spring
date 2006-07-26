/*---------------------------------------------------------------------
 Terrain Renderer using texture splatting and geomipmapping

 Copyright (2006) Jelmer Cnossen
 This code is released under GPL license (See LICENSE.html for info)
---------------------------------------------------------------------*/
#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "Plane.h"

/*
The frustum clips the polygons against a certain convex space
*/
class Frustum
{
public:
	void CalcCameraPlanes (Vector3 *base, Vector3 *right, Vector3* up, Vector3* front, float fov); // should at least have 
	void InversePlanes ();

	enum VisType { Inside, Outside, Partial };
	VisType IsBoxVisible (const Vector3& min, const Vector3& max); // 3D test
	VisType IsPointVisible (const Vector3& pt);

	std::vector<Plane> planes;
	Vector3 base, pos[4];
	void Draw ();
};

#endif
