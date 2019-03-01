/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Ground.h"
#include "ReadMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/SpringMath.h"

#include <cassert>
#include <limits>

#undef far // avoid collision with windef.h
#undef near

static inline float InterpolateHeight(float x, float z, const float* heightmap)
{
	// NOTE:
	// This isn't a bilinear interpolation. Instead it interpolates
	// on the 2 triangles that form the ground quad:
	//
	// TL __________ TR
	//    |        /|
	//    | dx+dz / |
	//    | \<1  /  |
	//    |     /   |
	//    |    /    |
	//    |   /     |
	//    |  / dx+dz|
	//    | /  \>=1 |
	//    |/        |
	// BL ---------- BR

	x = Clamp(x, 0.0f, float3::maxxpos) / SQUARE_SIZE;
	z = Clamp(z, 0.0f, float3::maxzpos) / SQUARE_SIZE;

	const int isx = x;
	const int isz = z;
	const float dx = x - isx;
	const float dz = z - isz;
	const int hs = isx + isz * mapDims.mapxp1;

	float h = 0.0f;

	if (dx + dz < 1.0f) {
		// top-left triangle
		const float h00 = heightmap[hs                 ];
		const float h10 = heightmap[hs + 1             ];
		const float h01 = heightmap[hs     + mapDims.mapxp1];
		const float xdif = dx * (h10 - h00);
		const float zdif = dz * (h01 - h00);

		h = h00 + xdif + zdif;
	} else {
		// bottom-right triangle
		const float h10 = heightmap[hs + 1             ];
		const float h11 = heightmap[hs + 1 + mapDims.mapxp1];
		const float h01 = heightmap[hs     + mapDims.mapxp1];
		const float xdif = (1.0f - dx) * (h01 - h11);
		const float zdif = (1.0f - dz) * (h10 - h11);

		h = h11 + xdif + zdif;
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
	const bool inMap = (xs >= 0) && (ys >= 0) && (xs <= mapDims.mapxm1) && (ys <= mapDims.mapym1);
//	assert(inMap);
	if (!inMap)
		return -1.0f;

	// The terrain grid is "composed" of two right-isosceles triangles
	// per square, so we have to check both faces (triangles) whether an
	// intersection exists
	// for each triangle, we pick one representative vertex

	// top-left corner vertex
	{
		float3 cornerVertex;
		cornerVertex.x = xs * SQUARE_SIZE;
		cornerVertex.z = ys * SQUARE_SIZE;
		cornerVertex.y = heightmap[ys * mapDims.mapxp1 + xs];

		// project \<to - cornerVertex\> vector onto the TL-normal
		// if \<to\> lies below the terrain, this will be negative
		const float3 faceNormalTL = normalmap[(ys * mapDims.mapx + xs) * 2    ];
		float toFacePlaneDist = (to - cornerVertex).dot(faceNormalTL);

		if (toFacePlaneDist <= 0.0f) {
			// project \<from - cornerVertex\> onto the TL-normal
			float fromFacePlaneDist = (from - cornerVertex).dot(faceNormalTL);

			if (fromFacePlaneDist != toFacePlaneDist) {
				const float alpha = fromFacePlaneDist / (fromFacePlaneDist - toFacePlaneDist);
				const float3 col = mix(from, to, alpha);

				if ((col.x >= cornerVertex.x) && (col.z >= cornerVertex.z) && (col.x + col.z <= cornerVertex.x + cornerVertex.z + SQUARE_SIZE)) {
					// point of intersection is inside the TL triangle
					return col.distance(from);
				}
			}
		}
	}

	// bottom-right corner vertex
	{
		float3 cornerVertex;
		cornerVertex.x = (xs + 1) * SQUARE_SIZE;
		cornerVertex.z = (ys + 1) * SQUARE_SIZE;
		cornerVertex.y = heightmap[(ys + 1) * mapDims.mapxp1 + (xs + 1)];

		// project \<to - cornerVertex\> vector onto the TL-normal
		// if \<to\> lies below the terrain, this will be negative
		const float3 faceNormalBR = normalmap[(ys * mapDims.mapx + xs) * 2 + 1];
		float toFacePlaneDist = (to - cornerVertex).dot(faceNormalBR);

		if (toFacePlaneDist <= 0.0f) {
			// project \<from - cornerVertex\> onto the BR-normal
			float fromFacePlaneDist = (from - cornerVertex).dot(faceNormalBR);

			if (fromFacePlaneDist != toFacePlaneDist) {
				const float alpha = fromFacePlaneDist / (fromFacePlaneDist - toFacePlaneDist);
				const float3 col = mix(from, to, alpha);

				if ((col.x <= cornerVertex.x) && (col.z <= cornerVertex.z) && (col.x + col.z >= cornerVertex.x + cornerVertex.z - SQUARE_SIZE)) {
					// point of intersection is inside the BR triangle
					return col.distance(from);
				}
			}
		}
	}

	return -2.0f;
}



/*
void CGround::CheckColSquare(CProjectile* p, int x, int y)
{
	if (!(x >= 0 && y >= 0 && x < mapDims.mapx && y < mapDims.mapy))
		return;

	float xp = p->pos.x;
	float yp = p->pos.y;
	float zp = p->pos.z;

	const float* hm = readMap->GetCornerHeightMapSynced();
	const float3* fn = readMap->GetFaceNormalsSynced();
	const int hmIdx = (y * mapDims.mapx + x);
	const float xt = x * SQUARE_SIZE;
	const float& yt0 = hm[ y      * mapDims.mapxp1 + x    ];
	const float& yt1 = hm[(y + 1) * mapDims.mapxp1 + x + 1];
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
	const float heightAboveMapMax = from.y - readMap->GetCurrMaxHeight();

	if (heightAboveMapMax <= 0.0f)
		return false;

	const float3 dir = (to - from);

	if (dir.y >= 0.0f) {
		// both `from` & `to` are above map's height
		from = -OnesVector;
		to   = -OnesVector;
		return true;
	}

	from += (dir * (-heightAboveMapMax / dir.y));
	return true;
}


float CGround::LineGroundCol(float3 from, float3 to, bool synced)
{
	const float* hm  = readMap->GetSharedCornerHeightMap(synced);
	const float3* nm = readMap->GetSharedFaceNormals(synced);

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

		if (from.y <= hm[sz * mapDims.mapxp1 + sx]) {
			return 0.0f + skippedDist;
		}
	}

	const float dx = to.x - from.x;
	const float dz = to.z - from.z;
	const int dirx = (dx > 0.0f) ? 1 : -1;
	const int dirz = (dz > 0.0f) ? 1 : -1;

	// Clamping is done cause LineGroundSquareCol() operates on the 2 triangles faces each heightmap
	// square is formed of.
	const float ffsx = Clamp(from.x / SQUARE_SIZE, 0.0f, (float)mapDims.mapx);
	const float ffsz = Clamp(from.z / SQUARE_SIZE, 0.0f, (float)mapDims.mapy);
	const float ttsx = Clamp(to.x / SQUARE_SIZE, 0.0f, (float)mapDims.mapx);
	const float ttsz = Clamp(to.z / SQUARE_SIZE, 0.0f, (float)mapDims.mapy);
	const int fsx = ffsx;
	const int fsz = ffsz;
	const int tsx = ttsx;
	const int tsz = ttsz;

	bool keepgoing = true;

	if ((fsx == tsx) && (fsz == tsz)) {
		// <from> and <to> are the same
		const float ret = LineGroundSquareCol(hm, nm,  from, to,  fsx, fsz);

		if (ret >= 0.0f) {
			return (ret + skippedDist);
		}
	} else if (fsx == tsx) {
		// ray is parallel to z-axis
		int zp = fsz;

		while (keepgoing) {
			const float ret = LineGroundSquareCol(hm, nm,  from, to,  fsx, zp);

			if (ret >= 0.0f) {
				return (ret + skippedDist);
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
				return (ret + skippedDist);
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
				return (ret + skippedDist);
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
		}
	}

	return -1.0f;
}

float CGround::LineGroundCol(const float3 pos, const float3 dir, float len, bool synced)
{
	return (LineGroundCol(pos, pos + dir * std::max(len, 0.0f), synced));
}


float CGround::LinePlaneCol(const float3 pos, const float3 dir, float len, float hgt)
{
	const float3 end = pos + dir * std::max(len, 0.0f);

	// no intersection if starting below or ending above (xz-)plane
	if (pos.y < hgt)
		return -1.0f;
	if (end.y > hgt)
		return -1.0f;

	// no intersection if going parallel to or away from (xz-)plane
	if (dir.y >= 0.0f)
		return (std::numeric_limits<float>::max());

	return ((pos.y - hgt) / -dir.y);
}


float CGround::LineGroundWaterCol(const float3 pos, const float3 dir, float len, bool testWater, bool synced)
{
	const float terraDist = LineGroundCol(pos, dir, len, synced);
	if (!testWater)
		return terraDist;

	const float waterDist = LinePlaneCol(pos, dir, len, 0.0f);
	if (waterDist < 0.0f)
		return terraDist;

	const float3 end = pos + dir * waterDist;

	if (end.x < 0.0f || end.x > float3::maxxpos)
		return terraDist;
	if (end.z < 0.0f || end.z > float3::maxzpos)
		return terraDist;

	if (terraDist < 0.0f)
		return waterDist;

	return std::min(terraDist, waterDist);
}


float CGround::GetApproximateHeight(float x, float z, bool synced)
{
	const float* heightMap = readMap->GetSharedCenterHeightMap(synced);

	const int xsquare = Clamp(int(x) / SQUARE_SIZE, 0, mapDims.mapxm1);
	const int zsquare = Clamp(int(z) / SQUARE_SIZE, 0, mapDims.mapym1);
	return heightMap[zsquare * mapDims.mapx + xsquare];
}


float CGround::GetApproximateHeightUnsafe(int x, int z, bool synced)
{
	const float* heightMap = readMap->GetSharedCenterHeightMap(synced);
	return heightMap[z * mapDims.mapx + x];
}


float CGround::GetHeightAboveWater(float x, float z, bool synced)
{
	return std::max(0.0f, GetHeightReal(x, z, synced));
}

float CGround::GetHeightReal(float x, float z, bool synced)
{
	return InterpolateHeight(x, z, readMap->GetSharedCornerHeightMap(synced));
}

float CGround::GetOrigHeight(float x, float z)
{
	return InterpolateHeight(x, z, readMap->GetOriginalHeightMapSynced());
}


const float3& CGround::GetNormal(float x, float z, bool synced)
{
	const int xsquare = Clamp(int(x) / SQUARE_SIZE, 0, mapDims.mapxm1);
	const int zsquare = Clamp(int(z) / SQUARE_SIZE, 0, mapDims.mapym1);

	const float3* normalMap = readMap->GetSharedCenterNormals(synced);
	return normalMap[xsquare + zsquare * mapDims.mapx];
}

const float3& CGround::GetNormalAboveWater(float x, float z, bool synced)
{
	if (GetHeightReal(x, z, synced) <= 0.0f)
		return UpVector;

	return (GetNormal(x, z, synced));
}


float CGround::GetSlope(float x, float z, bool synced)
{
	const int xhsquare = Clamp(int(x) / (2 * SQUARE_SIZE), 0, mapDims.hmapx - 1);
	const int zhsquare = Clamp(int(z) / (2 * SQUARE_SIZE), 0, mapDims.hmapy - 1);
	const float* slopeMap = readMap->GetSharedSlopeMap(synced);

	return slopeMap[xhsquare + zhsquare * mapDims.hmapx];
}


float3 CGround::GetSmoothNormal(float x, float z, bool synced)
{
	const int sx = Clamp(int(math::floor(x / SQUARE_SIZE)), 1, mapDims.mapx - 2);
	const int sz = Clamp(int(math::floor(z / SQUARE_SIZE)), 1, mapDims.mapy - 2);

	const float dx = (x / SQUARE_SIZE) - sx;
	const float dz = (z / SQUARE_SIZE) - sz;

	int sx2;
	int sz2;
	float fx;
	float fz;

	if (dz > 0.5f) {
		sz2 = sz + 1;
		fz = dz - 0.5f;
	} else {
		sz2 = sz - 1;
		fz = 0.5f - dz;
	}

	if (dx > 0.5f) {
		sx2 = sx + 1;
		fx = dx - 0.5f;
	} else {
		sx2 = sx - 1;
		fx = 0.5f - dx;
	}

	const float ifz = 1.0f - fz;
	const float ifx = 1.0f - fx;

	const float3* normalMap = readMap->GetSharedCenterNormals(synced);

	const float3& n1 = normalMap[sz  * mapDims.mapx + sx ] * ifx * ifz;
	const float3& n2 = normalMap[sz  * mapDims.mapx + sx2] *  fx * ifz;
	const float3& n3 = normalMap[sz2 * mapDims.mapx + sx ] * ifx *  fz;
	const float3& n4 = normalMap[sz2 * mapDims.mapx + sx2] *  fx *  fz;

	return ((n1 + n2 + n3 + n4).Normalize());
}



float CGround::SimTrajectoryGroundColDist(const float3& trajStartPos, const float3& trajStartDir, const float3& acc, const float2& args)
{
	// args.x := speed, args.y := length
	const float2 ips = GetMapBoundaryIntersectionPoints(trajStartPos, trajStartDir * XZVector * args.y);

	// outside map
	if (ips.y < 0.0f)
		return -1.0;

	const float minDist = args.y * std::max(0.0f, ips.x);
	const float maxDist = args.y * std::min(1.0f, ips.y);

	float3 pos = trajStartPos;
	float3 vel = trajStartDir * args.x;

	// sample heights along the trajectory of a virtual projectile launched
	// from <pos> with velocity <trajStartDir * speed>; <pos> is assumed to
	// start inside map
	while (pos.SqDistance2D(trajStartPos) < Square(minDist)) {
		vel += acc;
		pos += vel;
	}
	while (pos.y >= GetHeightReal(pos)) {
		vel += acc;
		pos += vel;
	}

	if (pos.SqDistance2D(trajStartPos) >= Square(maxDist))
		return -1.0f;

	return (math::sqrt(pos.SqDistance2D(trajStartPos)));
}

float CGround::TrajectoryGroundCol(const float3& trajStartPos, const float3& trajTargetDir, float length, float linCoeff, float qdrCoeff)
{
	// trajTargetDir should be the normalized xz-vector from <trajStartPos> to the target
	const float3 dir = {trajTargetDir.x, linCoeff, trajTargetDir.z};
	const float3 alt = UpVector * qdrCoeff;

	// limit checking to the in-map part of the line
	const float2 ips = GetMapBoundaryIntersectionPoints(trajStartPos, dir * length);

	// outside map
	if (ips.y < 0.0f)
		return -1.0;

	const float minDist = length * std::max(0.0f, ips.x);
	const float maxDist = length * std::min(1.0f, ips.y);

	for (float dist = minDist; dist < maxDist; dist += SQUARE_SIZE) {
		const float3 pos = (trajStartPos + dir * dist) + (alt * dist * dist);

		#if 1
		if (GetApproximateHeight(pos) > pos.y)
			return dist;
		#else
		if (GetHeightReal(pos) > pos.y)
			return dist;
		#endif
	}

	return -1.0f;
}



int CGround::GetSquare(const float3& pos) {
	const int x = Clamp((int(pos.x) / SQUARE_SIZE), 0, mapDims.mapxm1);
	const int z = Clamp((int(pos.z) / SQUARE_SIZE), 0, mapDims.mapym1);

	return (x + z * mapDims.mapx);
};

