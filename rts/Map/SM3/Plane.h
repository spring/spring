/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PLANE_H_
#define _PLANE_H_

#include "Vector3.h"

#define EPSILON (0.01f)

/** Terrain Renderer using texture splatting and geo-mip-mapping */
class Plane
{
public:
	float a;
	float b;
	float c;
	float d;

	Plane() : a(0.0f), b(0.0f), c(0.0f), d(0.0f) {}
	Plane(float a, float b, float c, float d) : a(a), b(b), c(c), d(d) {}
	Plane(const Vector3& dir, float dis) : a(dir.x), b(dir.y), c(dir.z), d(dis) {}

	bool operator==(const Plane& pln) const;
	bool operator!=(const Plane& pln) const { return !operator==(pln); }

	float Dist(const Vector3* v) const { return a*v->x + b*v->y + c*v->z - d; }
	float Dist(float x, float y, float z) const {  return a*x + b*y + z*c - d; }
	bool EpsilonCompare(const Plane& pln, float epsilon);
	void MakePlane(const Vector3& v1, const Vector3& v2, const Vector3& v3);
	/// Plane is the same, but is pointing to the inverse direction
	void Inverse() { a = -a; b = -b; c = -c; d = -d; }
	void SetVec(const Vector3& v) { a = v.x; b = v.y; c = v.z; }
	void CalcDist(const Vector3& p) { d = a*p.x + b*p.y + c*p.z; }
	void copy(Plane* pl) const { pl->a = a; pl->b = b; pl->c = c; pl->d = d; }
	Vector3& GetVector() { return *(reinterpret_cast<Vector3*>(this)); }
	Vector3 GetCenter() const { return Vector3(a*d, b*d, c*d); }
};

#endif // _PLANE_H_
