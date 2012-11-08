/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


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

	const int isx = x;
	const int isy = y;
	const float dx = x - isx;
	const float dy = y - isy;
	const int hs = isx + isy * gs->mapxp1;

	float h = 0.0f;

	if (dx + dy < 1.0f) {
		// top-left triangle
		const float h00 = heightmap[hs                 ];
		const float h10 = heightmap[hs + 1             ];
		const float h01 = heightmap[hs     + gs->mapxp1];
		const float xdif = (dx) * (h10 - h00);
		const float ydif = (dy) * (h01 - h00);

		h = h00 + xdif + ydif;
	} else {
		// bottom-right triangle
		const float h10 = heightmap[hs + 1             ];
		const float h11 = heightmap[hs + 1 + gs->mapxp1];
		const float h01 = heightmap[hs     + gs->mapxp1];
		const float xdif = (1.0f - dx) * (h01 - h11);
		const float ydif = (1.0f - dy) * (h10 - h11);

		h = h11 + xdif + ydif;
	}

	return h;
}


static inline float LineGroundSquareCol(
	const float* heightmap,
	const float3* normalmap,
	const float3& from,
	const float3& to,
	const int xs,
	const int ys)
{
	const bool inMap = (xs >= 0) && (ys >= 0) && (xs <= gs->mapxm1) && (ys <= gs->mapym1);
//	assert(inMap);
	if (!inMap)
		return -1.0f;

	const float3& faceNormalTL = normalmap[(ys * gs->mapx + xs) * 2    ];
	const float3& faceNormalBR = normalmap[(ys * gs->mapx + xs) * 2 + 1];
	float3 cornerVertex;

	// The terrain grid is "composed" of two right-isosceles triangles
	// per square, so we have to check both faces (triangles) whether an
	// intersection exists
	// for each triangle, we pick one representative vertex

	// top-left corner vertex
	cornerVertex.x = xs * SQUARE_SIZE;
	cornerVertex.z = ys * SQUARE_SIZE;
	cornerVertex.y = heightmap[ys * gs->mapxp1 + xs];

	// project \<to - cornerVertex\> vector onto the TL-normal
	// if \<to\> lies below the terrain, this will be negative
	float toFacePlaneDist = (to - cornerVertex).dot(faceNormalTL);
	float fromFacePlaneDist = 0.0f;

	if (toFacePlaneDist <= 0.0f) {
		// project \<from - cornerVertex\> onto the TL-normal
		fromFacePlaneDist = (from - cornerVertex).dot(faceNormalTL);

		if (fromFacePlaneDist != toFacePlaneDist) {
			const float alpha = fromFacePlaneDist / (fromFacePlaneDist - toFacePlaneDist);
			const float3 col = from * (1.0f - alpha) + to * alpha;

			if ((col.x >= cornerVertex.x) && (col.z >= cornerVertex.z) && (col.x + col.z <= cornerVertex.x + cornerVertex.z + SQUARE_SIZE)) {
				// point of intersection is inside the TL triangle
				return col.distance(from);
			}
		}
	}

	// bottom-right corner vertex
	cornerVertex.x += SQUARE_SIZE;
	cornerVertex.z += SQUARE_SIZE;
	cornerVertex.y = heightmap[(ys + 1) * gs->mapxp1 + (xs + 1)];

	// project \<to - cornerVertex\> vector onto the TL-normal
	// if \<to\> lies below the terrain, this will be negative
	toFacePlaneDist = (to - cornerVertex).dot(faceNormalBR);

	if (toFacePlaneDist <= 0.0f) {
		// project \<from - cornerVertex\> onto the BR-normal
		fromFacePlaneDist = (from - cornerVertex).dot(faceNormalBR);

		if (fromFacePlaneDist != toFacePlaneDist) {
			const float alpha = fromFacePlaneDist / (fromFacePlaneDist - toFacePlaneDist);
			const float3 col = from * (1.0f - alpha) + to * alpha;

			if ((col.x <= cornerVertex.x) && (col.z <= cornerVertex.z) && (col.x + col.z >= cornerVertex.x + cornerVertex.z - SQUARE_SIZE)) {
				// point of intersection is inside the BR triangle
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

inline static bool ClampInMapHeight(float3& from, float3& to)
{
	const float heightAboveMapMax = from.y - readmap->currMaxHeight;
	if (heightAboveMapMax <= 0)
		return false;

	const float3 dir = (to - from);
	if (dir.y >= 0.0f) {
		// both `from` & `to` are above map's height
		from = float3(-1.0f, -1.0f, -1.0f);
		to   = float3(-1.0f, -1.0f, -1.0f);
		return true;
	}

	from += dir * (-heightAboveMapMax / dir.y);
	return true;
}


float CGround::LineGroundCol(float3 from, float3 to, bool synced) const
{
	const float* hm  = readmap->GetCornerHeightMap(synced);
	const float3* nm = readmap->GetFaceNormals(synced);

	const float3 pfrom = from;

	// only for performance -> skip part that can impossibly collide
	// with the terrain, cause it is above map's current max height
	ClampInMapHeight(from, to);

	// handle special cases where the ray origin is out of bounds:
	// need to move <from> to the closest map-edge along the ray
	// (if both <from> and <to> are out of bounds, the ray might
	// still hit)
	// clamping <from> naively would change the direction of the
	// ray, hence we save the distance along it that got skipped
	ClampLineInMap(from, to);

	if (from == to) {
		// ClampLineInMap & ClampInMapHeight set `from == to == vec(-1,-1,-1)`
		// in case the line is outside of the map
		return -1.0f;
	}

	const float skippedDist = pfrom.distance(from);

	if (synced) { //TODO do this in unsynced too once the map border rendering is finished?
		// check if our start position is underground (assume ground is unpassable for cannons etc.)
		const int sx = from.x / SQUARE_SIZE;
		const int sz = from.z / SQUARE_SIZE;
		const float& h = hm[sz * gs->mapxp1 + sx];
		if (from.y <= h) {
			return 0.0f + skippedDist;
		}
	}

	const float dx = to.x - from.x;
	const float dz = to.z - from.z;
	const int dirx = (dx > 0.0f) ? 1 : -1;
	const int dirz = (dz > 0.0f) ? 1 : -1;

	// Claming is done cause LineGroundSquareCol() operates on the 2 triangles faces each heightmap
	// square is formed of.
	const float ffsx = Clamp(from.x / SQUARE_SIZE, 0.0f, (float)gs->mapx);
	const float ffsz = Clamp(from.z / SQUARE_SIZE, 0.0f, (float)gs->mapy);
	const float ttsx = Clamp(to.x / SQUARE_SIZE, 0.0f, (float)gs->mapx);
	const float ttsz = Clamp(to.z / SQUARE_SIZE, 0.0f, (float)gs->mapy);
	const int fsx = ffsx; // a>=0: int(a):=floor(a)
	const int fsz = ffsz;
	const int tsx = ttsx;
	const int tsz = ttsz;

	bool keepgoing = true;

	if ((fsx == tsx) && (fsz == tsz)) {
		// <from> and <to> are the same
		const float ret = LineGroundSquareCol(hm, nm,  from, to,  fsx, fsz);
		if (ret >= 0.0f) {
			return ret;
		}
	} else if (fsx == tsx) {
		// ray is parallel to z-axis
		int zp = fsz;

		while (keepgoing) {
			const float ret = LineGroundSquareCol(hm, nm,  from, to,  fsx, zp);
			if (ret >= 0.0f) {
				return ret + skippedDist;
			}

			keepgoing = (zp != tsz);

			zp += dirz;
		}
	} else if (fsz == tsz) {
		// ray is parallel to x-axis
		int xp = fsx;

		while (keepgoing) {
			const float ret = LineGroundSquareCol(hm, nm,  from, to,  xp, fsz);
			if (ret >= 0.0f) {
				return ret + skippedDist;
			}

			keepgoing = (xp != tsx);

			xp += dirx;
		}
	} else {
		// general case
		const float rdsx = SQUARE_SIZE / dx; // := 1 / (dx / SQUARE_SIZE)
		const float rdsz = SQUARE_SIZE / dz;

		// we need to shift the `test`-point in case of negative directions
		// case: dir<0
		//  ___________
		// |   |   |   |
		// |___|___|___|
		//     ^cur
		// ^cur + dir
		// >   < range of int(cur + dir)
		//     ^wanted test point := cur - epsilon
		// you can set epsilon=0 and then handle the `beyond end`-case (xn >= 1.0f && zn >= 1.0f) separate
		// (we already need to do so cause of floating point precision limits, so skipping epsilon doesn't add
		// any additional performance cost nor precision issue)
		//
		// case : dir>0
		// in case of `dir>0` the wanted test point is idential with `cur + dir`
		const float testposx = (dx > 0.0f) ? 0.0f : 1.0f;
		const float testposz = (dz > 0.0f) ? 0.0f : 1.0f;

		int curx = fsx;
		int curz = fsz;

		while (keepgoing) {
			// do the collision test with the squares triangles
			const float ret = LineGroundSquareCol(hm, nm,  from, to,  curx, curz);
			if (ret >= 0.0f) {
				return ret + skippedDist;
			}

			// check if we reached the end already and need to stop the loop
			const bool endReached = (curx == tsx && curz == tsz);
			const bool beyondEnd = ((curx - tsx) * dirx > 0) || ((curz - tsz) * dirz > 0);
			assert(!beyondEnd);
			keepgoing = !endReached && !beyondEnd;
			if (!keepgoing)
				 break;

			// calculate the `normalized position` of the next edge in x & z direction
			//  `normalized position`:=n :   x = from.x + n * (to.x - from.x)   (with 0<= n <=1)
			int nextx = curx + dirx;
			int nextz = curz + dirz;
			float xn = (nextx + testposx - ffsx) * rdsx;
			float zn = (nextz + testposz - ffsz) * rdsz;

			// handles the following 2 case:
			// case1: (floor(to.x) == to.x) && (to.x < from.x)
			//   In this case we calculate xn at to.x but set curx = to.x - 1,
			//   and so we would be beyond the end of the ray.
			// case2: floating point precision issues
			if ((nextx - tsx) * dirx > 0) { xn=1337.0f; nextx=tsx; }
			if ((nextz - tsz) * dirz > 0) { zn=1337.0f; nextz=tsz; }

			// advance to the next nearest edge in either x or z dir, or in the case we reached the end make sure
			// we set it to the exact square positions (floating point precision sometimes hinders us to hit it)
			if (xn >= 1.0f && zn >= 1.0f) {
				assert(curx != nextx || curz != nextz);
				curx = nextx;
				curz = nextz;
			} else if (xn < zn) {
				assert(curx != nextx);
				curx = nextx;
			} else {
				assert(curz != nextz);
				curz = nextz;
			}

			const bool beyondEnd_ = ((curx - tsx) * dirx > 0) || ((curz - tsz) * dirz > 0);
			assert(!beyondEnd_);
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
	int sx = (int) math::floor(x / SQUARE_SIZE);
	int sy = (int) math::floor(y / SQUARE_SIZE);

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

	// limit the checking to the `in map part` of the line
	std::pair<float,float> near_far = GetMapBoundaryIntersectionPoints(from, dir*length);

	// outside of map
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
