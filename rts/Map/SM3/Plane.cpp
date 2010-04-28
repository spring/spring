/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <cassert>

#include "Plane.h"

void Plane::MakePlane(const Vector3& v1,const Vector3& v2,const Vector3& v3)
{
	float rx1=v2.x-v1.x,ry1=v2.y-v1.y,rz1=v2.z-v1.z;
	float rx2=v3.x-v1.x,ry2=v3.y-v1.y,rz2=v3.z-v1.z;
	a=ry1*rz2-ry2*rz1;
	b=rz1*rx2-rz2*rx1;
	c=rx1*ry2-rx2*ry1;
	float len = (float)sqrt(a*a+b*b+c*c);
	a /= len;
	b /= len;
	c /= len;
	d=a*v2.x+b*v2.y+c*v2.z;
}

bool Plane::operator==(const Plane &pln)
{
	if((pln.a < a + EPSILON) && (pln.a > a - EPSILON) &&
		(pln.b < b + EPSILON) && (pln.b > b - EPSILON) &&
		(pln.c < c + EPSILON) && (pln.c > c - EPSILON) &&
		(pln.d < d + EPSILON) && (pln.d > d - EPSILON))  return true;
	return false;
}

bool Plane::EpsilonCompare (const Plane& pln, float epsilon)
{
	Plane t;
	t.a = fabs(a-pln.a);
	t.b = fabs(b-pln.b);
	t.c = fabs(c-pln.c);
	t.d = fabs(d-pln.d);
	if(t.a > epsilon || t.b > epsilon || t.c > epsilon || t.d > epsilon)
		return false;
	return true;
}

