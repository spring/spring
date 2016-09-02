/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LosMap.h"
#include "LosHandler.h"
#include "Map/ReadMap.h"
#include "System/myMath.h"
#include "System/float3.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/ThreadPool.h"
#ifdef USE_UNSYNCED_HEIGHTMAP
	#include "Game/GlobalUnsynced.h" // for myAllyTeam
#endif
#include <algorithm>
#include <array>



constexpr float LOS_BONUS_HEIGHT = 5.f;


static std::array<std::vector<float>, ThreadPool::MAX_THREADS> isqrt_table;


static float isqrt_lookup(unsigned r, int threadNum)
{
	assert(r < isqrt_table[threadNum].size());
	return isqrt_table[threadNum][r];
}

static void isqrt_verify(unsigned r, int threadNum)
{
	auto& isqrt = isqrt_table[threadNum];
	if (r >= isqrt.size()) {
		for (unsigned i=isqrt.size(); i<=r; ++i)
			isqrt.push_back(math::isqrt(std::max(i, 1u)));
	}
}



// Midpoint circle algorithm
// func() only get called for the lower top right octant.
// The others need to get by mirroring.
template<typename F>
inline void MidpointCircleAlgo(int radius, F func)
{
	int x = radius;
	int y = 0;
	int decisionOver2 = 1 - x;
	while (x >= y) {
		func(x,y);

		y++;
		if (decisionOver2 <= 0) {
			decisionOver2 += 2 * y + 1;
		} else {
			x--;
			decisionOver2 += 2 * (y - x) + 1;
		}
	}
}


// Calls func(half_line_width, y) for each line of the filled circle.
template<typename F>
inline void MidpointCircleAlgoPerLine(int radius, F func)
{
	int x = radius;
	int y = 0;
	int decisionOver2 = 1 - x;
	while (x >= y) {
		func(x, y);
		if (y != 0) func(x, -y);

		if (decisionOver2 <= 0) {
			y++;
			decisionOver2 += 2 * y + 1;
		} else {
			if (x != y) {
				func(y, x);
				if (x != 0) func(y, -x);
			}

			y++;
			x--;
			decisionOver2 += 2 * (y - x) + 1;
		}
	}
}



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/// CLosTables precalc helper

class CLosTables
{
public:
	typedef std::vector<char> LosLine;
	typedef std::vector<LosLine> LosTable;

	static void GenerateForLosSize(size_t losSize, int threadNum);

	// note: copy, references are not threadsafe
	static LosTable& GetLosTable(size_t losSize, int threadNum) {
		return instance.lostables[threadNum][losSize];
	}


private:
	static CLosTables instance;
	std::array<std::vector<LosTable>, ThreadPool::MAX_THREADS> lostables;

private:
	CLosTables();
	static LosLine GetRay(int x, int y);
	static LosTable GetLosRays(int radius);
	static void Debug(const LosTable& losRays, const std::vector<int2>& points, int radius);
	static std::vector<int2> GetCircleSurface(const int radius);
	static void AddMissing(LosTable& losRays, const std::vector<int2>& circlePoints, const int radius);
};
CLosTables CLosTables::instance;

void CLosTables::GenerateForLosSize(size_t losSize, int threadNum)
{
	if (instance.lostables[threadNum].size() <= losSize)
		instance.lostables[threadNum].resize(losSize+1);

	LosTable& tl = instance.lostables[threadNum][losSize];
	if (tl.empty())
		tl = std::move(GetLosRays(losSize));
}


CLosTables::CLosTables()
{
	for (auto& lt: lostables){
		lt.reserve(256);
		lt.emplace_back(); // zero radius
	}
}




// Calls func for every square in a ray
#define for_ray_square(line, func)                                      \
{                                                                       \
	const char inc_x = line[0];                                         \
	const char inc_y = !line[0];                                        \
                                                                        \
	int2 square(0, 0);                                                  \
	const char *ptr = &line[1];                                         \
	const char *end = ptr + line.size() - 1;                            \
	for (; ptr < end; ++ptr) {                                          \
		square += int2(inc_x || *ptr, inc_y || *ptr);                   \
		func;                                                           \
	}                                                                   \
}

// Calls func for every square in a ray, with a rule for breaking
#define for_ray_square_rule(line, func, rule)                           \
{                                                                       \
	const char inc_x = line[0];                                         \
	const char inc_y = !line[0];                                        \
                                                                        \
	int2 square(0, 0);                                                  \
	const char *ptr = &line[1];                                         \
	const char *end = ptr + line.size() - 1;                            \
	for (; ptr < end; ++ptr) {                                          \
		square += int2(inc_x || *ptr, inc_y || *ptr);                   \
		if (rule)                                                       \
			break;                                                      \
                                                                        \
		func;                                                           \
	}                                                                   \
}


/**
 * @brief Precalcs the rays for LineOfSight raytracing.
 * In LoS we raytrace all squares in a radius if they are in view
 * or obstructed by the heightmap. To do so we cast rays with the
 * given radius to the LoS circle's surface. But cause those rays
 * have no width, it happens that squares are missed inside of the
 * circle. So these squares get their own rays with length < radius.
 *
 * Note: We only return the rays for the upper right sector, the
 * others can be constructed by mirroring.
 */
CLosTables::LosTable CLosTables::GetLosRays(const int radius)
{
	LosTable losRays;

	std::vector<int2> circlePoints = GetCircleSurface(radius);
	losRays.reserve(2 * circlePoints.size()); // twice cause of AddMissing()
	for (int2& p: circlePoints) {
		losRays.push_back(GetRay(p.x, p.y));
	}
	AddMissing(losRays, circlePoints, radius);

	//if (radius == 30)
	//	Debug(losRays, circlePoints, radius);
	losRays.shrink_to_fit();
	return losRays;
}


/**
 * @brief returns the surface coords of a 2d circle.
 * Note, we only return the upper right part, the other 3 are generated via mirroring.
 */
std::vector<int2> CLosTables::GetCircleSurface(const int radius)
{
	// Midpoint circle algorithm
	// returns the surface points of a circle (without duplicates)
	std::vector<int2> circlePoints;
	circlePoints.reserve(2 * radius);
	MidpointCircleAlgo(radius, [&](int x, int y){
		// the upper 1/8th
		circlePoints.emplace_back(x, y);

		// the lower 1/8th, not added when:
		// first check prevents 45deg duplicates
		// second makes sure that only (0,radius) or (radius, 0) is generated (the other one is generated by mirroring later)
		if (y != x && y != 0)
			circlePoints.emplace_back(y, x);
	});
	assert(circlePoints.size() <= 2 * radius);
	return circlePoints;
}


/**
 * @brief Makes sure all squares in the radius are checked & adds rays to missing ones.
 */
void CLosTables::AddMissing(LosTable& losRays, const std::vector<int2>& circlePoints, const int radius)
{
	std::vector<char> image((radius+1) * (radius+1), 0);
	auto setpixel = [&](int2 p) { image[p.y * (radius+1) + p.x] = true; };
	auto getpixel = [&](int2 p) { return image[p.y * (radius+1) + p.x]; };
	for (auto& line: losRays) {
		for_ray_square(line, setpixel(square));
	}

	// start the check from 45deg bisector and go from there to 0deg & 90deg
	// advantage is we only need to iterate once this time
	for (auto it = circlePoints.rbegin(); it != circlePoints.rend(); ++it) { // note, we reverse iterate the list!
		const int2& p = *it;
		for (int a=p.x; a>=1 && a>=p.y; --a) {
			int2 t1(a, p.y);
			int2 t2(p.y, a);
			if (!getpixel(t1)) {
				losRays.push_back(GetRay(t1.x, t1.y));
				for_ray_square(losRays.back(), setpixel(square));
			}
			if (!getpixel(t2) && t2 != int2(0,radius)) { // (0,radius) is a mirror of (radius,0), so don't add it
				losRays.push_back(GetRay(t2.x, t2.y));
				for_ray_square(losRays.back(), setpixel(square));
			}
		}
	}
}


/**
 * @brief returns line coords of a ray with zero width to the coords (xf,yf)
 */
CLosTables::LosLine CLosTables::GetRay(int xf, int yf)
{
	assert(xf >= 0);
	assert(yf >= 0);

	LosLine losline;
	int larger, smaller;
	if (xf > yf) {
		larger = xf;
		smaller = yf;
		losline.push_back(true);
	} else {
		larger = yf;
		smaller = xf;
		losline.push_back(false);
	}
	losline.reserve(larger + 1);
	const float m = (float) smaller / (float) larger;
	for (int i = 1; i <= larger; i++) {
		losline.push_back(Round(m*i) != Round(m*(i-1)));
	}

	assert(!losline.empty());
	return losline;
}


void CLosTables::Debug(const LosTable& losRays, const std::vector<int2>& points, int radius)
{
	// check for duplicated/included rays
	auto losRaysCopy = losRays;
	for (const auto& ray1: losRaysCopy) {
		if (ray1.empty())
			continue;

		for (auto& ray2: losRaysCopy) {
			if (ray2.empty())
				continue;

			if (&ray1 == &ray2)
				continue;

			// check if ray2 is part of ray1
			if (std::includes(ray1.begin(), ray1.end(), ray2.begin(), ray2.end())) {
				// prepare for deletion
				ray2.clear();
			}
		}
	}
	auto jt = std::remove_if(losRaysCopy.begin(), losRaysCopy.end(), [](LosLine& ray) { return ray.empty(); });
	assert(jt == losRaysCopy.end());

	// print the rays stats
	LOG("------------------------------------");

	// draw the sphere image
	LOG("- sketch -");
	std::vector<char> image((2*radius+1) * (2*radius+1), 0);
	auto setpixel = [&](int2 p, char value = 1) {
		image[p.y * (2*radius+1) + p.x] = value;
	};
	int2 midp = int2(radius, radius);
	for (auto& line: losRays) {
		for_ray_square(line,
			setpixel(midp + square, 127);
			setpixel(midp - square, 127);
			setpixel(midp + int2(square.y, -square.x), 127);
			setpixel(midp + int2(-square.y, square.x), 127);
		);
	}
	for (int2 p: points) {
		setpixel(midp + p, 1);
		setpixel(midp - p, 2);
		setpixel(midp + int2(p.y, -p.x), 4);
		setpixel(midp + int2(-p.y, p.x), 8);
	}
	for (int y = 0; y <= 2*radius; y++) {
		std::string l;
		for (int x = 0; x <= 2*radius; x++) {
			if (image[y*(2*radius+1) + x] == 127) {
				l += ".";
			} else {
				l += IntToString(image[y*(2*radius+1) + x]);
			}
		}
		LOG("%s", l.c_str());
	}

	// points on the sphere surface
	LOG("- surface points -");
	std::string s;
	for (int2 p: points) {
		s += "(" + IntToString(p.x) + "," + IntToString(p.y) + ") ";
	}
	LOG("%s", s.c_str());

	// rays to those points
	LOG("- los rays -");
	for (auto& line: losRays) {
		std::string s;
		for_ray_square(line, (s += "(" + IntToString(square.x) + "," + IntToString(square.y) + ") "));
		LOG("%s", s.c_str());
	}
	LOG_L(L_DEBUG, "------------------------------------");
}











//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/// CLosMap implementation

void CLosMap::AddCircle(SLosInstance* instance, int amount)
{
#ifdef USE_UNSYNCED_HEIGHTMAP
	//only AddRaycast supports UnsyncedHeightMap updates
#endif

	MidpointCircleAlgoPerLine(instance->radius, [&](int width, int y) {
		const unsigned y_ = instance->basePos.y + y;
		if (y_ < size.y) {
			const unsigned sx = Clamp(instance->basePos.x - width,     0, size.x);
			const unsigned ex = Clamp(instance->basePos.x + width + 1, 0, size.x);

			for (unsigned x_ = sx; x_ < ex; ++x_) {
				losmap[(y_ * size.x) + x_] += amount;
			}
		}
	});
}


void CLosMap::AddRaycast(SLosInstance* instance, int amount)
{
	if (instance->squares.empty() || instance->squares.front().length == SLosInstance::EMPTY_RLE.length) {
		return;
	}

#ifdef USE_UNSYNCED_HEIGHTMAP
	// Inform ReadMap when squares enter LoS
	const bool updateUnsyncedHeightMap = (sendReadmapEvents && instance->allyteam >= 0 && (instance->allyteam == gu->myAllyTeam || gu->spectatingFullView));
	if ((amount > 0) && updateUnsyncedHeightMap) {
		for (const SLosInstance::RLE rle: instance->squares) {
			int idx = rle.start;
			for (int l = rle.length; l>0; --l, ++idx) {
				const bool squareEnteredLOS = (losmap[idx] == 0);
				losmap[idx] += amount;

				if (!squareEnteredLOS) { continue; }

				const int2 lm = IdxToCoord(idx, size.x);
				const int2 p1 = lm * LOS2HEIGHT;
				const int2 p2 = std::min(p1 + int2(1,1), int2(mapDims.mapxm1, mapDims.mapym1));

				readMap->UpdateLOS(SRectangle(p1.x, p1.y, p2.x, p2.y));
			}
		}

		return;
	}
#endif

	for (const SLosInstance::RLE rle: instance->squares) {
		int idx = rle.start;
		for (int l = rle.length; l>0; --l, ++idx) {
			losmap[idx] += amount;
		}
	}
}


void CLosMap::PrepareRaycast(SLosInstance* instance) const
{
	if (!instance->squares.empty())
		return;

	LosAdd(instance);
	if (instance->squares.empty()) {
		instance->squares.push_back(SLosInstance::EMPTY_RLE);
	}
}


#define MAP_SQUARE(pos) ((pos).y * size.x + (pos).x)


void CLosMap::LosAdd(SLosInstance* li) const
{
	auto MAP_SQUARE_FULLRES = [&](int2 pos) {
		float2 fpos = pos;
		fpos += 0.5f;
		fpos /= float2(size);
		int2 ipos = fpos * float2(mapDims.mapx, mapDims.mapy);
		//assert(ipos.y * mapDims.mapx + ipos.x < (mapDims.mapx * mapDims.mapy));
		return ipos.y * mapDims.mapx + ipos.x;
	};

	// skip when unit is underground
	const float* heightmapFull = readMap->GetCenterHeightMapSynced();
	if (SRectangle(0,0,size.x,size.y).Inside(li->basePos) && li->baseHeight <= heightmapFull[MAP_SQUARE_FULLRES(li->basePos)])
		return;

	// add all squares that are in the los radius
	SRectangle safeRect(li->radius, li->radius, size.x - li->radius, size.y - li->radius);
	if (safeRect.Inside(li->basePos)) {
		// we aren't touching the map borders -> we don't need to check for the map boundaries
		UnsafeLosAdd(li);
	} else {
		// we need to check each square if it's outside of the map boundaries
		SafeLosAdd(li);
	}
}


inline static constexpr size_t ToAngleMapIdx(const int2 p, const int radius)
{
	// [-radius, +radius]^2 -> [0, +2*radius]^2 -> idx
	return (p.y + radius) * (2*radius + 1) + (p.x + radius);
}


inline void CastLos(float* prevAng, float* maxAng, const int2& off, std::vector<char>& squaresMap, std::vector<float>& anglesMap, int radius, int threadNum)
{
	// check if we got a new maxAngle
	const size_t oidx = ToAngleMapIdx(off, radius);
	if (anglesMap[oidx] < *maxAng) {
		squaresMap[oidx] = false;
		return;
	}

	if (anglesMap[oidx] < *prevAng) {
		const float invR = isqrt_lookup(off.x*off.x + off.y*off.y, threadNum);
		*maxAng = *prevAng - LOS_BONUS_HEIGHT * invR;
		if (anglesMap[oidx] < *maxAng) {
			squaresMap[oidx] = false;
			return;
		}
	}
	*prevAng = anglesMap[oidx];
}


void CLosMap::AddSquaresToInstance(SLosInstance* li, const std::vector<char>& squaresMap) const
{
	const int2 pos   = li->basePos;
	const int radius = li->radius;

	const char *ptr = &squaresMap[0];
	for (int y = -radius; y<=radius; ++y) {
		SLosInstance::RLE rle = {MAP_SQUARE(pos + int2(-radius,y)), 0};
		for (int x = -radius; x<=radius; ++x) {
			if (*(ptr++)) {
				++rle.length;
			} else {
				if (rle.length > 0) li->squares.push_back(rle);
				rle.start  += rle.length + 1;
				rle.length = 0;
			}
		}
		if (rle.length > 0) li->squares.push_back(rle);
	}
}


void CLosMap::UnsafeLosAdd(SLosInstance* li) const
{
	// How does it work?
	// We spawn rays (those created by CLosTables::GenerateForLosSize), and cast them on the
	// heightmap. Meaning we compute the angle to the given squares and compare them with
	// the highest cached one on that ray. When the new angle is higher the square is
	// visible and gets added to the squares array.
	//
	// How does prevAng optimisation work?
	// We don't really need to save every angle as the maximum, if we're going up a mountain
	// we can just mark them true and continue until we reach the top.
	// So now, only hilltops are cached in maxAng, and they're only cached when checking
	// the square after the hilltop, since otherwise we can't know that the ascent ended.

	const int2 pos   = li->basePos;
	const int radius = li->radius;
	const float losHeight = li->baseHeight;
	const size_t area = Square((2*radius) + 1);
	int threadNum = ThreadPool::GetThreadNum();
	CLosTables::GenerateForLosSize(radius, threadNum); //Only generates if not in cache
	std::vector<char> squaresMap(area, false); // saves the list of visible squares
	std::vector<float> anglesMap(area, -1e8);
	isqrt_verify((radius + 1) * (radius + 1), threadNum);

	// Optimization: precalc all angles, cause:
	// 1. Many squares are accessed by multiple rays. Imagine you got a 128 radius circle
	//    then the center squares are accessed much more often than the circle border ones.
	// 2. The heightmap is much bigger than the circle, and won't fit into the L2/L3. So
	//    when we buffer the precalc in a vector just large enough for the processed data,
	//    we reduce latter the amount of cache misses.
	MidpointCircleAlgoPerLine(radius, [&](int width, int y) {
		const unsigned y_ = pos.y + y;
		const unsigned sx = pos.x - width;
		const unsigned ex = pos.x + width + 1;

		const size_t oidx = ToAngleMapIdx(int2(sx - pos.x, y), radius);
		float* anglesPtr = &anglesMap[oidx];
		char* squaresPtr = &squaresMap[oidx];
		int idx = MAP_SQUARE(int2(sx, y_));
		for (unsigned x_ = sx; x_ < ex; ++x_) {
			const int2 off(x_ - pos.x, y);

			if (off == int2(0,0)) {
				++idx;
				++anglesPtr;
				++squaresPtr;
				continue;
			}

			const float invR = isqrt_lookup(off.x*off.x + off.y*off.y, threadNum);
			const float dh = std::max(0.f, heightmap[idx++]) - losHeight;

			*(anglesPtr++) = (dh + LOS_BONUS_HEIGHT) * invR;
			*(squaresPtr++) = true;
		}
	});

	// Cast the Rays
	squaresMap[ToAngleMapIdx(int2(0,0), radius)] = true;
	for (const CLosTables::LosLine& line: CLosTables::GetLosTable(radius, threadNum)) {
		float maxAng[4] = {-1e7, -1e7, -1e7, -1e7};
		float prevAng[4] = {-1e7, -1e7, -1e7, -1e7};

		for_ray_square(line,
			CastLos(&prevAng[0], &maxAng[0], square,                    squaresMap, anglesMap, radius, threadNum);
			CastLos(&prevAng[1], &maxAng[1], -square,                   squaresMap, anglesMap, radius, threadNum);
			CastLos(&prevAng[2], &maxAng[2], int2(square.y, -square.x), squaresMap, anglesMap, radius, threadNum);
			CastLos(&prevAng[3], &maxAng[3], int2(-square.y, square.x), squaresMap, anglesMap, radius, threadNum);
		);
	}

	// translate visible square indices to map square idx + RLE
	AddSquaresToInstance(li, squaresMap);
}


void CLosMap::SafeLosAdd(SLosInstance* li) const
{
	// How does it work?
	// see above

	const int2 pos   = li->basePos;
	const int radius = li->radius;
	const float losHeight = li->baseHeight;
	const size_t area = Square((2*radius) + 1);
	int threadNum = ThreadPool::GetThreadNum();
	CLosTables::GenerateForLosSize(radius, threadNum); //Only generates if not in cache
	std::vector<char> squaresMap(area, false); // saves the list of visible squares
	std::vector<float> anglesMap(area, -1e8);
	const SRectangle safeRect(0, 0, size.x, size.y);
	isqrt_verify((radius + 1) * (radius + 1), threadNum);

	// Optimization: precalc all angles
	MidpointCircleAlgoPerLine(radius, [&](int width, int y) {
		const unsigned y_ = pos.y + y;
		if (y_ < size.y) {
			const unsigned sx = Clamp(pos.x - width,     0, size.x);
			const unsigned ex = Clamp(pos.x + width + 1, 0, size.x);

			const size_t oidx = ToAngleMapIdx(int2(sx - pos.x, y), radius);
			float* anglesPtr = &anglesMap[oidx];
			char* squaresPtr = &squaresMap[oidx];
			int idx = MAP_SQUARE(int2(sx, y_));
			for (unsigned x_ = sx; x_ < ex; ++x_) {
				const int2 off(x_ - pos.x, y);

				if (off == int2(0,0)) {
					++idx;
					++anglesPtr;
					++squaresPtr;
					continue;
				}

				const float invR = isqrt_lookup(off.x*off.x + off.y*off.y, threadNum);
				const float dh = std::max(0.f, heightmap[idx++]) - losHeight;

				*(anglesPtr++) = (dh + LOS_BONUS_HEIGHT) * invR;
				*(squaresPtr++) = true;
			}
		}
	});


	// Cast the Rays
	if (safeRect.Inside(pos)) {
		squaresMap[ToAngleMapIdx(int2(0,0), radius)] = true;

		for (const CLosTables::LosLine& line: CLosTables::GetLosTable(radius, threadNum)) {
			float maxAng[4] = {-1e7, -1e7, -1e7, -1e7};
			float prevAng[4] = {-1e7, -1e7, -1e7, -1e7};

			for_ray_square_rule(line,
				CastLos(&prevAng[0], &maxAng[0], square,                    squaresMap, anglesMap, radius, threadNum)
			,
				!safeRect.Inside(pos + square)
			);
			for_ray_square_rule(line,
				CastLos(&prevAng[1], &maxAng[1], -square,                   squaresMap, anglesMap, radius, threadNum)
			,
				!safeRect.Inside(pos - square)
			);
			for_ray_square_rule(line,
				CastLos(&prevAng[2], &maxAng[2], int2(square.y, -square.x), squaresMap, anglesMap, radius, threadNum);
			,
				!safeRect.Inside(pos + int2(square.y, -square.x))
			);
			for_ray_square_rule(line,
				CastLos(&prevAng[3], &maxAng[3], int2(-square.y, square.x), squaresMap, anglesMap, radius, threadNum);
			,
				!safeRect.Inside(pos + int2(-square.y, square.x))
			);
		}
	} else {
		// emit position outside the map
		for (const CLosTables::LosLine& line: CLosTables::GetLosTable(radius, threadNum)) {
			float maxAng[4] = {-1e7, -1e7, -1e7, -1e7};
			float prevAng[4] = {-1e7, -1e7, -1e7, -1e7};

			for_ray_square(line,
				if (safeRect.Inside(pos + square)) {
					CastLos(&prevAng[0], &maxAng[0], square,                    squaresMap, anglesMap, radius, threadNum);
				}
				if (safeRect.Inside(pos - square)) {
					CastLos(&prevAng[1], &maxAng[1], -square,                   squaresMap, anglesMap, radius, threadNum);
				}
				if (safeRect.Inside(pos + int2(square.y, -square.x))) {
					CastLos(&prevAng[2], &maxAng[2], int2(square.y, -square.x), squaresMap, anglesMap, radius, threadNum);
				}
				if (safeRect.Inside(pos + int2(-square.y, square.x))) {
					CastLos(&prevAng[3], &maxAng[3], int2(-square.y, square.x), squaresMap, anglesMap, radius, threadNum);
				}
			);
		}
	}

	// translate visible square indices to map square idx + RLE
	AddSquaresToInstance(li, squaresMap);
}
