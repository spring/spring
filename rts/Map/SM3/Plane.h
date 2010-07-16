/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PLANE_H_
#define _PLANE_H_

#include "float3.h"
typedef float3 Vector3;

#define EPSILON (0.01f)

/** Terrain Renderer using texture splatting and geomipmapping */
class Plane
{
public:
	float a,b,c,d;
	Plane() { a=b=c=d=0; }
	Plane(float ta,float tb,float tc,float td) { a=ta;b=tb;c=tc;d=td; }
	Plane(Vector3 dir, float dis) { a=dir.x; b=dir.y;c=dir.z; d=dis; }
	float Dist(const Vector3 *v) const { return a*v->x+b*v->y+c*v->z-d; }
	float Dist(float x,float y,float z) const {  return a*x+b*y+z*c-d; }
	bool EpsilonCompare (const Plane& pln, float epsilon);
	bool operator==(const Plane &pln);
	bool operator!=(const Plane &pln) { return !operator==(pln); }
	void MakePlane(const Vector3& v1, const Vector3& v2,const Vector3& v3);
	void Inverse() {a=-a; b=-b; c=-c; d=-d;} ///< Plane is the same, but is pointing to the inverse direction
	void SetVec(Vector3 v) { a=v.x;b=v.y;c=v.z; }
	void CalcDist(const Vector3& p) { d = a*p.x + b*p.y + c*p.z; }
	void copy (Plane *pl) { pl->a=a; pl->b=b; pl->c=c; pl->d=d; }
	Vector3& GetVector() { return *(Vector3*)this; }
	Vector3 GetCenter() { return Vector3 (a*d,b*d,c*d); }
};

#endif // _PLANE_H_
