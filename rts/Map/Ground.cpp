/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "Ground.h"
#include "ReadMap.h"
#include "Game/Camera.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Projectiles/Projectile.h"
#include "System/myMath.h"

#include <cassert>
#include <limits>

#undef far // avoid collision with windef.h
#undef near

static inline float InterpolateHeight(float x, float y, const float* heightmap)
{
	// NOTE:
	// This isn't a bilinear interpolation. Instead it interpolates
	// on the 2 triangles that form the ground quad:
	//
	// TL __________ TR
	//    |        /|
	//    | dx+dy / |
	//    | \<1  /  |
	//    |     /   |
	//    |    /    |
	//    |   /     |
	//    |  / dx+dy|
	//    | /  \>=1 |
	//    |/        |
	// BL ---------- BR

	x = Clamp(x, 0.f, float3::maxxpos) / SQUARE_SIZE;
	y = Clamp(y, 0.f, float3::maxzpos) / SQUARE_SIZE;

	const int isx = int(x);
	const int isy = int(y);
	const float dx = x - isx;
	const float dy = y - isy;
	const int hs = isx + isy * gs->mapxp1;

	if (dx + dy < 1.0f) {
		// top left triangle
		const float h00 = heightmap[hs                 ];
		const float h10 = heightmap[hs + 1             ];
		const float h01 = heightmap[hs     + gs->mapxp1];
		const float xdif = (dx) * (h10 - h00);
		const float ydif = (dy) * (h01 - h00);

		return h00 + xdif + ydif;
	} else {
		// bottom right triangle
		const float h10 = heightmap[hs + 1             ];
		const float h11 = heightmap[hs + 1 + gs->mapxp1];
		const float h01 = heightmap[hs     + gs->mapxp1];
		const float xdif = (1.0f - dx) * (h01 - h11);
		const float ydif = (1.0f - dy) * (h10 - h11);

		return h11 + xdif + ydif;
	}

	return 0.0f; // can not be reached
}


static inline float LineGroundSquareCol(
	const float*& heightmap,
	const float3*& normalmap,
	const float3& from,
	const float3& to,
	const int& xs,
	const int& ys)
{
	if ((xs < 0) || (ys < 0) || (xs >= gs->mapxm1) || (ys >= gs->mapym1))
		return -1.0f;

	const float3& faceNormalTL = normalmap[(ys * gs->mapx + xs) * 2    ];
	const float3& faceNormalBR = normalmap[(ys * gs->mapx + xs) * 2 + 1];
	float3 cornerVertex;

	//! The terrain grid is "composed" of two right-isosceles triangles
	//! per square, so we have to check both faces (triangles) whether an
	//! intersection exists
	//! for each triangle, we pick one representative vertex

	//! top-left corner vertex
	cornerVertex.x = xs * SQUARE_SIZE;
	cornerVertex.z = ys * SQUARE_SIZE;
	cornerVertex.y = heightmap[ys * gs->mapxp1 + xs];

	//! project \<to - cornerVertex\> vector onto the TL-normal
	//! if \<to\> lies below the terrain, this will be negative
	float toFacePlaneDist = (to - cornerVertex).dot(faceNormalTL);
	float fromFacePlaneDist = 0.0f;

	if (toFacePlaneDist <= 0.0f) {
		//! project \<from - cornerVertex\> onto the TL-normal
		fromFacePlaneDist = (from - cornerVertex).dot(faceNormalTL);

		if (fromFacePlaneDist != toFacePlaneDist) {
			const float alpha = fromFacePlaneDist / (fromFacePlaneDist - toFacePlaneDist);
			const float3 col = from * (1.0f - alpha) + (to * alpha);

			if ((col.x >= cornerVertex.x) && (col.z >= cornerVertex.z) && (col.x + col.z <= cornerVertex.x + cornerVertex.z + SQUARE_SIZE)) {
				//! point of intersection is inside the TL triangle
				return col.distance(from);
			}
		}
	}

	//! bottom-right corner vertex
	cornerVertex.x += SQUARE_SIZE;
	cornerVertex.z += SQUARE_SIZE;
	cornerVertex.y = heightmap[(ys + 1) * gs->mapxp1 + (xs + 1)];

	//! project \<to - cornerVertex\> vector onto the TL-normal
	//! if \<to\> lies below the terrain, this will be negative
	toFacePlaneDist = (to - cornerVertex).dot(faceNormalBR);

	if (toFacePlaneDist <= 0.0f) {
		//! project \<from - cornerVertex\> onto the BR-normal
		fromFacePlaneDist = (from - cornerVertex).dot(faceNormalBR);

		if (fromFacePlaneDist != toFacePlaneDist) {
			const float alpha = fromFacePlaneDist / (fromFacePlaneDist - toFacePlaneDist);
			const float3 col = from * (1.0f - alpha) + (to * alpha);

			if ((col.x <= cornerVertex.x) && (col.z <= cornerVertex.z) && (col.x + col.z >= cornerVertex.x + cornerVertex.z - SQUARE_SIZE)) {
				//! point of intersection is inside the BR triangle
				return col.distance(from);
			}
		}
	}

	return -2.0f;
}



CGround* ground = NULL;

CGround::~CGround()
{
	delete readmap; readmap = NULL;
}

/*
void CGround::CheckColSquare(CProjectile* p, int x, int y)
{
	if (!(x >= 0 && y >= 0 && x < gs->mapx && y < gs->mapy))
		return;

	float xp = p->pos.x;
	float yp = p->pos.y;
	float zp = p->pos.z;

	const float* hm = readmap->GetCornerHeightMapSynced();
	const float3* fn = readmap->GetFaceNormalsSynced();
	const int hmIdx = (y * gs->mapx + x);
	const float xt = x * SQUARE_SIZE;
	const float& yt0 = hm[ y      * gs->mapxp1 + x    ];
	const float& yt1 = hm[(y + 1) * gs->mapxp1 + x + 1];
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
*/



float CGround::LineGroundCol(float3 from, float3 to, bool synced) const
{
	const float3 pfrom = from;

	// handle special cases where the ray origin is out of bounds:
	// need to move <from> to the closest map-edge along the ray
	// (if both <from> and <to> are out of bounds, the ray might
	// still hit)
	// clamping <from> naively would change the direction of the
	// ray, hence we save the distance along it that got skipped
	ClampLineInMap(from, to);

	const float skippedDist = (pfrom - from).Length();
	const float dx = to.x - from.x;
	const float dz = to.z - from.z;
	float ret;

	bool keepgoing = true;

	const float* hm  = readmap->GetCornerHeightMap(synced);
	const float3* nm = readmap->GetFaceNormals(synced);

	const int fsx = int(from.x / SQUARE_SIZE); // a>=0: int(a):=floor(a)
	const int fsz = int(from.z / SQUARE_SIZE);
	const int tsx = int(to.x / SQUARE_SIZE);
	const int tsz = int(to.z / SQUARE_SIZE);

	if ((fsx == tsx) && (fsz == tsz)) {
		// <from> and <to> are the same
		ret = LineGroundSquareCol(hm, nm,  from, to,  fsx, fsz);

		if (ret >= 0.0f) {
			return ret;
		}
	} else if (fsx == tsx) {
		// ray is parallel to z-axis
		int zp = fsz;

		while (keepgoing) {
			ret = LineGroundSquareCol(hm, nm,  from, to,  fsx, zp);

			if (ret >= 0.0f) {
				return ret + skippedDist;
			}

			keepgoing = (zp != tsz);

			if (dz > 0)
				zp++;
			else
				zp--;
		}
	} else if (fsz == tsz) {
		// ray is parallel to x-axis
		int xp = fsx;

		while (keepgoing) {
			ret = LineGroundSquareCol(hm, nm,  from, to,  xp, fsz);

			if (ret >= 0.0f) {
				return ret + skippedDist;
			}

			keepgoing = (xp != tsx);

			if (dx > 0.0f)
				xp++;
			else
				xp--;
		}
	} else {
		// general case
		assert(fabs(dx)>0);
		assert(fabs(dz)>0);
		const float rdx = 1.0f / dx;
		const float rdz = 1.0f / dz;

		float pos_relative = 0.0f;
		int lastxs = -1, lastzs = -1;

		while (keepgoing) {
			// Get next square to check for collision
			int xs, zs;
			do {
				const float xp = from.x + pos_relative * dx;
				const float zp = from.z + pos_relative * dz;

				// current square
				xs = int(xp * SQUARE_SIZE_RECIPROCAL);
				zs = int(zp * SQUARE_SIZE_RECIPROCAL);

				keepgoing = (xs != tsx && zs != tsz);
				if (!keepgoing)
					 break;

				// calculate the `step` needed to reach the next x-/z-square
				float xn, zn;
				if (dx > 0.0f) {
					xn = (xs * SQUARE_SIZE + SQUARE_SIZE - from.x) * rdx;
				} else {
					xn = (xs * SQUARE_SIZE - SQUARE_SIZE - from.x) * rdx;
				}
				if (dz > 0.0f) {
					zn = (zs * SQUARE_SIZE + SQUARE_SIZE - from.z) * rdz;
				} else {
					zn = (zs * SQUARE_SIZE - SQUARE_SIZE - from.z) * rdz;
				}

				// xn and zn are always positive, minus signs are divided out above
				assert((xn>0) || (zn>0));

				// go either a step into x and/or z direction depending on which is more near
				if (xn < zn) {
					assert(pos_relative < xn);
					pos_relative = xn;
				} else {
					assert(pos_relative < zn);
					pos_relative = zn;
				}
			} while (lastxs == xs && lastzs == zs);

			assert(lastxs != xs || lastzs != zs); // we don't want to check squares twice
			assert((lastxs < 0) || (lastzs < 0) || ((xs - lastxs <= 1) && (zs - lastzs <= 1))); // check if we skipped a square
			lastxs = xs;
			lastzs = zs;

			ret = LineGroundSquareCol(hm, nm,  from, to,  xs, zs);

			if (ret >= 0.0f) {
				return ret + skippedDist;
			}
		}
	}

	return -1.0f;
}


float CGround::GetApproximateHeight(float x, float y, bool synced) const
{
	int xsquare = int(x) / SQUARE_SIZE;
	int ysquare = int(y) / SQUARE_SIZE;
	xsquare = Clamp(xsquare, 0, gs->mapxm1);
	ysquare = Clamp(ysquare, 0, gs->mapym1);

	const float* heightMap = readmap->GetCenterHeightMap(synced);
	return heightMap[xsquare + ysquare * gs->mapx];
}

float CGround::GetHeightAboveWater(float x, float y, bool synced) const
{
	return std::max(0.0f, GetHeightReal(x, y, synced));
}

float CGround::GetHeightReal(float x, float y, bool synced) const
{
	return InterpolateHeight(x, y, readmap->GetCornerHeightMap(synced));
}

float CGround::GetOrigHeight(float x, float y) const
{
	return InterpolateHeight(x, y, readmap->GetOriginalHeightMapSynced());
}


const float3& CGround::GetNormal(float x, float y, bool synced) const
{
	int xsquare = int(x) / SQUARE_SIZE;
	int ysquare = int(y) / SQUARE_SIZE;
	xsquare = Clamp(xsquare, 0, gs->mapxm1);
	ysquare = Clamp(ysquare, 0, gs->mapym1);

	const float3* normalMap = readmap->GetCenterNormals(synced);
	return normalMap[xsquare + ysquare * gs->mapx];
}


float CGround::GetSlope(float x, float y, bool synced) const
{
	int xhsquare = int(x) / (2 * SQUARE_SIZE);
	int yhsquare = int(y) / (2 * SQUARE_SIZE);
	xhsquare = Clamp(xhsquare, 0, gs->hmapx - 1);
	yhsquare = Clamp(yhsquare, 0, gs->hmapy - 1);

	const float* slopeMap = readmap->GetSlopeMap(synced);
	return slopeMap[xhsquare + yhsquare * gs->hmapx];
}


float3 CGround::GetSmoothNormal(float x, float y, bool synced) const
{
	int sx = (int) floor(x / SQUARE_SIZE);
	int sy = (int) floor(y / SQUARE_SIZE);

	if (sy < 1)
		sy = 1;
	if (sx < 1)
		sx = 1;
	if (sy >= gs->mapym1)
		sy = gs->mapy - 2;
	if (sx >= gs->mapxm1)
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

	const float3* normalMap = readmap->GetCenterNormals(synced);

	const float3& n1 = normalMap[sy  * gs->mapx + sx ] * ifx * ify;
	const float3& n2 = normalMap[sy  * gs->mapx + sx2] *  fx * ify;
	const float3& n3 = normalMap[sy2 * gs->mapx + sx ] * ifx * fy;
	const float3& n4 = normalMap[sy2 * gs->mapx + sx2] *  fx * fy;

	float3 norm1 = n1 + n2 + n3 + n4;
	norm1.Normalize();

	return norm1;
}

float CGround::TrajectoryGroundCol(float3 from, const float3& flatdir, float length, float linear, float quadratic) const
{
	float3 dir(flatdir.x, linear, flatdir.z);

	//! limit the checking to the `in map part` of the line
	std::pair<float,float> near_far = GetMapBoundaryIntersectionPoints(from, dir*length);

	//! outside of map
	if (near_far.second < 0.0f)
		return -1.0;

	const float near = length * std::max(0.0f, near_far.first);
	const float far  = length * std::min(1.0f, near_far.second);

	for (float l = near; l < far; l += SQUARE_SIZE) {
		float3 pos(from + dir*l);
		pos.y += quadratic * l * l;

		if (GetApproximateHeight(pos.x, pos.z) > pos.y) {
			return l;
		}
	}

	return -1.0f;
}
