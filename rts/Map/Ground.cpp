/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Ground.h"
#include "ReadMap.h"
#include "Game/Camera.h"
#include "Sim/Projectiles/Projectile.h"
#include "LogOutput.h"
#include "Sim/Misc/GeometricObjects.h"
#include "System/myMath.h"
#include <assert.h>

CGround* ground;

CGround::CGround()
{
}

CGround::~CGround()
{
	delete readmap;
}

void CGround::CheckColSquare(CProjectile* p, int x, int y)
{
	if (!(x >= 0 && y >= 0 && x < gs->mapx && y < gs->mapy))
		return;

	float xp = p->pos.x;
	float yp = p->pos.y;
	float zp = p->pos.z;

	const float* hm = readmap->GetHeightmap();
	const float3* fn = readmap->facenormals;
	const int hmIdx = (y * gs->mapx + x);
	const float xt = x * SQUARE_SIZE;
	const float& yt0 = hm[ y      * (gs->mapx + 1) + x    ];
	const float& yt1 = hm[(y + 1) * (gs->mapx + 1) + x + 1];
	const float zt = y * SQUARE_SIZE;

	const float3& fn0 = fn[hmIdx * 2    ];
	const float3& fn1 = fn[hmIdx * 2 + 1];
	const float dx0 = (xp -  xt     );
	const float dy0 = (yp -  yt0    );
	const float dz0 = (zp -  zt     );
	const float dx1 = (xp - (xt + 2));
	const float dy1 = (yp -  yt1    );
	const float dz1 = (zp - (zt + 2));
	const float d0 = dx0 * fn0.x + dy0 * fn0.y + dz0 * fn0.z;
	const float d1 = dx1 * fn1.x + dy1 * fn1.y + dz1 * fn1.z;
	const float s0 = xp + zp - xt - zt - p->radius;
	const float s1 = xp + zp - xt - zt - SQUARE_SIZE * 2 + p->radius;

	if ((d0 <= p->radius) && (s0 < SQUARE_SIZE)) {
		p->Collision();
	}
	if ((d1 <= p->radius) && (s1 > -SQUARE_SIZE)) {
		p->Collision();
	}

	return;
}

inline float LineGroundSquareCol(const float3& from, const float3& to, int xs, int ys)
{
	if ((xs < 0) || (ys < 0) || (xs >= gs->mapx - 1) || (ys >= gs->mapy - 1))
		return -1;

	float3 tri;
	const float* heightmap = readmap->GetHeightmap();

	//! Info:
	//! The terrain grid is constructed by a triangle strip
	//! so we have to check 2 triangles foreach quad

	//! triangle topright
	tri.x = xs * SQUARE_SIZE;
	tri.z = ys * SQUARE_SIZE;
	tri.y = heightmap[ys * (gs->mapx + 1) + xs];

	const float3& norm = readmap->facenormals[(ys * gs->mapx + xs) * 2];
	float side1 = (to - tri).dot(norm);

	if (side1 <= 0.0f) {
		float side2 = (from - tri).dot(norm);

		if (side2 != side1) {
			float frontpart = side2 / (side2 - side1);
			const float3 col = from * (1 - frontpart) + to * frontpart;

			if ((col.x >= tri.x) && (col.z >= tri.z) && (col.x + col.z <= tri.x + tri.z + SQUARE_SIZE)) {
				return col.distance(from);
			}
		}
	}

	//! triangle bottomleft
	tri.x += SQUARE_SIZE;
	tri.z += SQUARE_SIZE;
	tri.y = heightmap[(ys + 1) * (gs->mapx + 1) + xs + 1];

	const float3& norm2 = readmap->facenormals[(ys * gs->mapx + xs) * 2 + 1];
	side1 = (to - tri).dot(norm2);

	if (side1 <= 0.0f) {
		float side2 = (from - tri).dot(norm2);

		if (side2 != side1) {
			float frontpart = side2 / (side2 - side1);
			const float3 col = from * (1 - frontpart) + to * frontpart;

			if ((col.x <= tri.x) && (col.z <= tri.z) && (col.x + col.z >= tri.x + tri.z - SQUARE_SIZE)) {
				return col.distance(from);
			}
		}
	}
	return -2;
}

float CGround::LineGroundCol(float3 from, float3 to) const
{
	float savedLength = 0.0f;

	if (from.z > float3::maxzpos && to.z < float3::maxzpos) {
		// a special case since the camera in overhead mode can often do this
		float3 dir = to - from;
		dir.SafeNormalize();
		savedLength = -(from.z - float3::maxzpos) / dir.z;
		from += dir * savedLength;
	}

	from.CheckInBounds();

	float3 dir = to - from;
	float maxLength = dir.Length();
	dir /= maxLength;

	if (from.x + dir.x * maxLength < 1.0f)
		maxLength = (1.0f - from.x) / dir.x;
	else if (from.x + dir.x * maxLength > float3::maxxpos)
		maxLength = (float3::maxxpos - from.x) / dir.x;

	if (from.z + dir.z * maxLength < 1.0f)
		maxLength = (1.0f - from.z) / dir.z;
	else if (from.z + dir.z * maxLength > float3::maxzpos)
		maxLength = (float3::maxzpos - from.z) / dir.z;

	to = from + dir * maxLength;

	const float dx=to.x-from.x;
	const float dz=to.z-from.z;
	float ret;

	bool keepgoing=true;

	if((floor(from.x/SQUARE_SIZE)==floor(to.x/SQUARE_SIZE)) && (floor(from.z/SQUARE_SIZE)==floor(to.z/SQUARE_SIZE))){
		ret = LineGroundSquareCol(from,to,(int)floor(from.x/SQUARE_SIZE),(int)floor(from.z/SQUARE_SIZE));
		if(ret>=0){
			return ret;
		}
	} else if(floor(from.x/SQUARE_SIZE)==floor(to.x/SQUARE_SIZE)){
		float zp = from.z/SQUARE_SIZE;
		int xp = (int)floor(from.x/SQUARE_SIZE);
		while(keepgoing){
			ret = LineGroundSquareCol(from, to, xp, (int)floor(zp));
			if(ret>=0){
				return ret+savedLength;
			}
			keepgoing = fabs(zp*SQUARE_SIZE-from.z)<fabs(dz);
			if(dz>0)
				zp+=1.0f;
			else
				zp-=1.0f;
		}
		// if you hit this the collision detection hit an infinite loop
		assert(!keepgoing);
	} else if(floor(from.z/SQUARE_SIZE)==floor(to.z/SQUARE_SIZE)){
		float xp=from.x/SQUARE_SIZE;
		int zp = (int)floor(from.z/SQUARE_SIZE);
		while(keepgoing){
			ret = LineGroundSquareCol(from,to,(int)floor(xp), zp);
			if(ret>=0){
				return ret+savedLength;
			}
			keepgoing=fabs(xp*SQUARE_SIZE-from.x)<fabs(dx);
			if(dx>0)
				xp+=1.0f;
			else
				xp-=1.0f;
		}
		// if you hit this the collision detection hit an infinite loop
		assert(!keepgoing);
	} else {
		float xp=from.x;
		float zp=from.z;
		while(keepgoing){
			float xn,zn;
			float xs, zs;

			// Push value just over the edge of the square
			// This is the best accuracy we can get with floats:
			// add one digit and (xp*constant) reduces to xp itself
			// This accuracy means that at (16384,16384) (lower right of 32x32 map)
			// 1 in every 1/(16384*1e-7f/8)=4883 clicks on the map will be ignored.
			if (dx>0) xs = floor(xp*1.0000001f/SQUARE_SIZE);
			else      xs = floor(xp*0.9999999f/SQUARE_SIZE);
			if (dz>0) zs = floor(zp*1.0000001f/SQUARE_SIZE);
			else      zs = floor(zp*0.9999999f/SQUARE_SIZE);

			ret = LineGroundSquareCol(from, to, (int)xs, (int)zs);
			if(ret>=0){
				return ret+savedLength;
			}
			keepgoing=fabs(xp-from.x)<fabs(dx) && fabs(zp-from.z)<fabs(dz);

			if(dx>0){
				// distance xp to right edge of square (xs,zs) divided by dx, xp += xn*dx puts xp on the right edge
				xn=(xs*SQUARE_SIZE+SQUARE_SIZE-xp)/dx;
			} else {
				// distance xp to left edge of square (xs,zs) divided by dx, xp += xn*dx puts xp on the left edge
				xn=(xs*SQUARE_SIZE-xp)/dx;
			}
			if(dz>0){
				// distance zp to bottom edge of square (xs,zs) divided by dz, zp += zn*dz puts zp on the bottom edge
				zn=(zs*SQUARE_SIZE+SQUARE_SIZE-zp)/dz;
			} else {
				// distance zp to top edge of square (xs,zs) divided by dz, zp += zn*dz puts zp on the top edge
				zn=(zs*SQUARE_SIZE-zp)/dz;
			}
			// xn and zn are always positive, minus signs are divided out above

			// this puts (xp,zp) exactly on the first edge you see if you look from (xp,zp) in the (dx,dz) direction
			if(xn<zn){
				xp+=xn*dx;
				zp+=xn*dz;
			} else {
				xp+=zn*dx;
				zp+=zn*dz;
			}
		}
	}
	return -1;
}


float CGround::GetApproximateHeight(float x, float y) const
{
	int xsquare = int(x) / SQUARE_SIZE;
	int ysquare = int(y) / SQUARE_SIZE;

	if (xsquare < 0)
		xsquare = 0;
	else if (xsquare > gs->mapx - 1)
		xsquare = gs->mapx - 1;
	if (ysquare < 0)
		ysquare = 0;
	else if (ysquare > gs->mapy - 1)
		ysquare = gs->mapy - 1;

	return readmap->centerheightmap[xsquare + ysquare * gs->mapx];
}

//rename to GetHeightAboveWater?
float CGround::GetHeight(float x, float y) const
{
	const float r = GetHeight2(x, y);
	return std::max(0.0f, r);
}


static inline float Interpolate(float x, float y, const float* heightmap)
{
	if (x < 1)
		x = 1;
	else if (x > float3::maxxpos)
		x = float3::maxxpos;

	if (y < 1)
		y = 1;
	else if (y > float3::maxzpos)
		y = float3::maxzpos;

	const int sx = (int) (x / SQUARE_SIZE);
	const int sy = (int) (y / SQUARE_SIZE);
	const float dx = (x - sx * SQUARE_SIZE) * (1.0f / SQUARE_SIZE);
	const float dy = (y - sy * SQUARE_SIZE) * (1.0f / SQUARE_SIZE);
	const int hs = sx + sy * (gs->mapx + 1);

	if (dx + dy < 1) {
		const float xdif = (dx) * (heightmap[hs +            1] - heightmap[hs]);
		const float ydif = (dy) * (heightmap[hs + gs->mapx + 1] - heightmap[hs]);

		return heightmap[hs] + xdif + ydif;
	}
	else {
		const float xdif = (1.0f - dx) * (heightmap[hs + gs->mapx + 1] - heightmap[hs + 1 + 1 + gs->mapx]);
		const float ydif = (1.0f - dy) * (heightmap[hs            + 1] - heightmap[hs + 1 + 1 + gs->mapx]);

		return heightmap[hs + 1 + 1 + gs->mapx] + xdif + ydif;
	}
	return 0; // can not be reached
}


float CGround::GetHeight2(float x, float y) const
{
	return Interpolate(x, y, readmap->GetHeightmap());
}


float CGround::GetOrigHeight(float x, float y) const
{
	return Interpolate(x, y, readmap->orgheightmap);
}


float3& CGround::GetNormal(float x, float y) const
{
	if (x < 1.0f)
		x = 1.0f;
	else if (x > float3::maxxpos)
		x = float3::maxxpos;

	if (y < 1.0f)
		y = 1.0f;
	else if (y > float3::maxzpos)
		y = float3::maxzpos;

	return readmap->centernormals[int(x) / SQUARE_SIZE + int(y) / SQUARE_SIZE * gs->mapx];
}


float CGround::GetSlope(float x, float y) const
{
	if (x < 1.0f)
		x = 1.0f;
	else if (x > float3::maxxpos)
		x = float3::maxxpos;

	if (y < 1.0f)
		y = 1.0f;
	else if (y > float3::maxzpos)
		y = float3::maxzpos;

	//return (1.0f - readmap->centernormals[int(x) / SQUARE_SIZE + int(y) / SQUARE_SIZE * gs->mapx].y);
	return readmap->slopemap[(int(x) / SQUARE_SIZE) / 2 + (int(y) / SQUARE_SIZE) / 2 * gs->hmapx];
}


float3 CGround::GetSmoothNormal(float x, float y) const
{
	int sx = (int) floor(x / SQUARE_SIZE);
	int sy = (int) floor(y / SQUARE_SIZE);

	if (sy < 1)
		sy = 1;
	if (sx < 1)
		sx = 1;
	if (sy >= gs->mapy - 1)
		sy = gs->mapy - 2;
	if (sx >= gs->mapx - 1)
		sx = gs->mapx - 2;

	float dx = (x / SQUARE_SIZE) - sx;
	float dy = (y / SQUARE_SIZE) - sy;

	int sy2;
	float fy;

	if (dy > 0.5f) {
		sy2 = sy + 1;
		fy = dy - 0.5f;
	} else {
		sy2 = sy - 1;
		fy = 0.5f - dy;
	}

	int sx2;
	float fx;

	if (dx > 0.5f) {
		sx2 = sx + 1;
		fx = dx - 0.5f;
	} else {
		sx2 = sx - 1;
		fx = 0.5f - dx;
	}

	float ify = 1.0f - fy;
	float ifx = 1.0f - fx;

	const float3* normals = readmap->centernormals;
	const float3& n1 = normals[sy  * gs->mapx + sx ] * ifx * ify;
	const float3& n2 = normals[sy  * gs->mapx + sx2] *  fx * ify;
	const float3& n3 = normals[sy2 * gs->mapx + sx ] * ifx * fy;
	const float3& n4 = normals[sy2 * gs->mapx + sx2] *  fx * fy;

	float3 norm1 = n1 + n2 + n3 + n4;
	norm1.Normalize();

	return norm1;
}

float CGround::TrajectoryGroundCol(float3 from, float3 flatdir, float length, float linear, float quadratic) const
{
	float3 dir(flatdir.x, linear, flatdir.z);

	//! limit the checking to the `in map part` of the line
	std::pair<float,float> near_far = GetMapBoundaryIntersectionPoints(from, dir*length);

	//! outside of map
	if (near_far.second < 0.f)
		return -1;

	const float near = length * std::max(0.f, near_far.first);
	const float far  = length * std::min(1.f, near_far.second);

	for (float l = near; l < far; l += SQUARE_SIZE) {
		float3 pos(from + dir*l);
		pos.y += quadratic * l * l;

		if (GetApproximateHeight(pos.x, pos.z) > pos.y) {
			return l;
		}
	}
	return -1.f;
}
