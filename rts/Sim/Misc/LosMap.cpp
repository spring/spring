/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LosMap.h"
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
#include <cstring>
#include <array>


CR_BIND(CLosMap, )

CR_REG_METADATA(CLosMap, (
	CR_MEMBER(size),
	CR_MEMBER(map),
	CR_MEMBER(sendReadmapEvents)
))



CR_BIND(CLosAlgorithm, (int2(), 0.0f, 0.0f, NULL))

CR_REG_METADATA(CLosAlgorithm, (
	CR_MEMBER(size),
	CR_MEMBER(minMaxAng),
	CR_MEMBER(extraHeight)//,
	//CR_MEMBER(heightmap)
))





//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/// CLosMap implementation

void CLosMap::SetSize(int2 newSize, bool newSendReadmapEvents)
{
	size = newSize;
	sendReadmapEvents = newSendReadmapEvents;
	map.clear();
	map.resize(size.x * size.y, 0);
}



void CLosMap::AddMapArea(int2 pos, int allyteam, int radius, int amount)
{
	#ifdef USE_UNSYNCED_HEIGHTMAP
	const int LOS2HEIGHT_X = mapDims.mapx / size.x;
	const int LOS2HEIGHT_Z = mapDims.mapy / size.y;

	const bool updateUnsyncedHeightMap = (sendReadmapEvents && allyteam >= 0 && (allyteam == gu->myAllyTeam || gu->spectatingFullView));
	#endif

	const int sx = std::max(         0, pos.x - radius);
	const int ex = std::min(size.x - 1, pos.x + radius);
	const int sy = std::max(         0, pos.y - radius);
	const int ey = std::min(size.y - 1, pos.y + radius);

	const int rr = (radius * radius);

	for (int lmz = sy; lmz <= ey; ++lmz) {
		const int rrx = rr - Square(pos.y - lmz);
		for (int lmx = sx; lmx <= ex; ++lmx) {
			const int losMapSquareIdx = (lmz * size.x) + lmx;
			#ifdef USE_UNSYNCED_HEIGHTMAP
			const bool squareEnteredLOS = (map[losMapSquareIdx] == 0 && amount > 0);
			#endif

			if (Square(pos.x - lmx) > rrx) {
				continue;
			}

			map[losMapSquareIdx] += amount;

			#ifdef USE_UNSYNCED_HEIGHTMAP
			// update unsynced heightmap for all squares that
			// cover LOSmap square <x, y> (LOSmap resolution
			// is never greater than that of the heightmap)
			//
			// NOTE:
			//     CLosMap is also used by RadarHandler, so only
			//     update the unsynced heightmap from LosHandler
			//     (by checking if allyteam >= 0)
			//
			if (!updateUnsyncedHeightMap) { continue; }
			if (!squareEnteredLOS) { continue; }

			const int
				x1 = lmx * LOS2HEIGHT_X,
				z1 = lmz * LOS2HEIGHT_Z;
			const int
				x2 = std::min((lmx + 1) * LOS2HEIGHT_X, mapDims.mapxm1),
				z2 = std::min((lmz + 1) * LOS2HEIGHT_Z, mapDims.mapym1);

			readMap->UpdateLOS(SRectangle(x1, z1, x2, z2));
			#endif
		}
	}
}

void CLosMap::AddMapSquares(const std::vector<int>& squares, int allyteam, int amount)
{
	#ifdef USE_UNSYNCED_HEIGHTMAP
	const int LOS2HEIGHT_X = mapDims.mapx / size.x;
	const int LOS2HEIGHT_Z = mapDims.mapy / size.y;

	const bool updateUnsyncedHeightMap = (sendReadmapEvents && allyteam >= 0 && (allyteam == gu->myAllyTeam || gu->spectatingFullView));
	#endif

	for (const int losMapSquareIdx: squares) {
		#ifdef USE_UNSYNCED_HEIGHTMAP
		const bool squareEnteredLOS = (map[losMapSquareIdx] == 0 && amount > 0);
		#endif

		map[losMapSquareIdx] += amount;

		#ifdef USE_UNSYNCED_HEIGHTMAP
		if (!updateUnsyncedHeightMap) { continue; }
		if (!squareEnteredLOS) { continue; }

		const int
			lmx = losMapSquareIdx % size.x,
			lmz = losMapSquareIdx / size.x;
		const int
			x1 = lmx * LOS2HEIGHT_X,
			z1 = lmz * LOS2HEIGHT_Z,
			x2 = std::min((lmx + 1) * LOS2HEIGHT_X, mapDims.mapxm1),
			z2 = std::min((lmz + 1) * LOS2HEIGHT_Z, mapDims.mapym1);

		readMap->UpdateLOS(SRectangle(x1, z1, x2, z2));
		#endif
	}
}





//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/// CLosTables precalc helper

typedef std::vector<int2> LosLine;
typedef std::vector<LosLine> LosTable;

class CLosTables
{
public:
	static const LosTable& GetForLosSize(size_t losSize);

private:
	static CLosTables instance;
	static const size_t MAX_LOS_SIZE = 300;
	std::vector<LosTable> lostables;

private:
	CLosTables();
	static LosLine GetRay(int x, int y);
	static LosTable GetLosRays(int radius);
	static void Debug(const LosTable& losRays, const std::vector<int2>& points, int radius);
	static std::vector<int2> GetCircleSurface(const int radius);
	static void AddMissing(LosTable& losRays, const std::vector<int2>& circlePoints, const int radius);
};

CLosTables CLosTables::instance;


const LosTable& CLosTables::GetForLosSize(size_t losSize)
{
	assert(losSize < MAX_LOS_SIZE);
	losSize = std::min<size_t>(losSize, MAX_LOS_SIZE);
	size_t s = instance.lostables.size();

	if (losSize + 1 > s) {
		//SCOPED_TIMER("LosTables");
		instance.lostables.resize(losSize+1);

		for_mt(s, losSize+1, [&](const int i) {
			instance.lostables[i] = std::move(GetLosRays(i));
		});
	}

	return instance.lostables[losSize];
}


CLosTables::CLosTables()
{
	lostables.reserve(128);
	lostables.emplace_back(); // zero radius
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
LosTable CLosTables::GetLosRays(const int radius)
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
	int x = radius;
	int y = 0;
	int decisionOver2 = 1 - radius;
	while (x >= y) {
		// the upper 1/8th
		circlePoints.emplace_back(x, y);

		// the lower 1/8th, not added when:
		// first check prevents 45deg duplicates
		// second makes sure that only (0,radius) or (radius, 0) is generated (they other one is generated by mirroring later)
		if (y != x && y != 0)
			circlePoints.emplace_back(y, x);

		y++;
		if (decisionOver2 <= 0) {
			decisionOver2 += 2 * y + 1;
		} else {
			x--;
			decisionOver2 += 2 * (y - x) + 1;
		}
	}
	assert(circlePoints.size() <= 2 * radius);
	return circlePoints;
}


/**
 * @brief Makes sure all squares in the radius are checked & adds rays to missing ones.
 */
void CLosTables::AddMissing(LosTable& losRays, const std::vector<int2>& circlePoints, const int radius)
{
	std::vector<bool> image((radius+1) * (radius+1), 0);
	auto setpixel = [&](int2 p) { image[p.y * (radius+1) + p.x] = true; };
	auto getpixel = [&](int2 p) { return image[p.y * (radius+1) + p.x]; };
	for (auto& line: losRays) {
		for (int2& p: line) {
			setpixel(p);
		}
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
				for (int2& p_: losRays.back()) {
					setpixel(p_);
				}
			}
			if (!getpixel(t2) && t2 != int2(0,radius)) { // (0,radius) is a mirror of (radius,0), so don't add it
				losRays.push_back(GetRay(t2.x, t2.y));
				for (int2& p_: losRays.back()) {
					setpixel(p_);
				}
			}
		}
	}
}


/**
 * @brief returns line coords of a ray with zero width to the coords (xf,yf)
 */
LosLine CLosTables::GetRay(int xf, int yf)
{
	assert(xf >= 0);
	assert(yf >= 0);

	LosLine losline;
	if (xf > yf) {
		// horizontal line
		const float m = (float) yf / (float) xf;
		losline.reserve(xf);
		for (int x = 1; x <= xf; x++) {
			losline.emplace_back(x, Round(m*x));
		}
	} else {
		// vertical line
		const float m = (float) xf / (float) yf;
		losline.reserve(yf);
		for (int y = 1; y <= yf; y++) {
			losline.emplace_back(Round(m*y), y);
		}
	}

	assert(losline.back() == int2(xf,yf));
	assert(!losline.empty());
	return losline;
}


void CLosTables::Debug(const LosTable& losRays, const std::vector<int2>& points, int radius)
{
	// only one should be included (the other one is generated via mirroring)
	assert(losRays.front().back() == int2(radius, 0));
	assert(losRays.back().back() != int2(0,radius));

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
		for (int2 p: line) {
			setpixel(midp + p, 127);
			setpixel(midp - p, 127);
			setpixel(midp + int2(p.y, -p.x), 127);
			setpixel(midp + int2(-p.y, p.x), 127);
		}
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
		for (int2 p: line) {
			s += "(" + IntToString(p.x) + "," + IntToString(p.y) + ") ";
		}
		LOG("%s", s.c_str());
	}
	LOG_L(L_DEBUG, "------------------------------------");
}






//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/// CLosAlgorithm implementation



void CLosAlgorithm::LosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares)
{
	if (radius <= 0) { return; }

	SRectangle safeRect(radius, radius, size.x - radius, size.y - radius);

	// add all squares that are in the los radius
	if (safeRect.Inside(pos)) {
		// we aren't touching the map borders -> we don't need to check for the map boundaries
		UnsafeLosAdd(pos, radius, baseHeight, squares);
	} else {
		// we need to check each square if it's outsid of the map boundaries
		SafeLosAdd(pos, radius, baseHeight, squares);
	}

	// delete duplicates
	std::sort(squares.begin(), squares.end());
	auto it = std::unique(squares.begin(), squares.end());
	squares.erase(it, squares.end());
}


inline static void CastLos(std::vector<int>* squares, float* maxAng, const int square, const float invR, const float* heightmap, float losHeight)
{
	const float dh = heightmap[square] - losHeight;
	float ang = dh * invR;
	if (ang >= *maxAng) {
		squares->push_back(square);
		*maxAng = ang;
	}
}


#define MAP_SQUARE(pos) ((pos).y * size.x + (pos).x)


void CLosAlgorithm::UnsafeLosAdd(int2 pos, int radius, float losHeight, std::vector<int>& squares)
{
	const int mapSquare = MAP_SQUARE(pos);
	const LosTable& table = CLosTables::GetForLosSize(radius);


	size_t neededSpace = squares.size() + 1;
	for (const LosLine& line: table) {
		neededSpace += line.size() * 4;
	}
	squares.reserve(neededSpace);

	losHeight += heightmap[mapSquare]; //FIXME comment

	squares.push_back(mapSquare);
	for (const LosLine& line: table) {
		float maxAng1 = minMaxAng;
		float maxAng2 = minMaxAng;
		float maxAng3 = minMaxAng;
		float maxAng4 = minMaxAng;
		float r = 1;

		for (const int2& square: line) {
			const float invR = math::isqrt2(square.x*square.x + square.y*square.y);


			CastLos(&squares, &maxAng1, MAP_SQUARE(pos + square),                    invR, heightmap, losHeight);
			CastLos(&squares, &maxAng2, MAP_SQUARE(pos - square),                    invR, heightmap, losHeight);
			CastLos(&squares, &maxAng3, MAP_SQUARE(pos + int2(square.y, -square.x)), invR, heightmap, losHeight);
			CastLos(&squares, &maxAng4, MAP_SQUARE(pos + int2(-square.y, square.x)), invR, heightmap, losHeight);
			r++;
		}
	}
}


void CLosAlgorithm::SafeLosAdd(int2 pos, int radius, float losHeight, std::vector<int>& squares)
{
	const int mapSquare = MAP_SQUARE(pos);
	const LosTable& table = CLosTables::GetForLosSize(radius);
	size_t neededSpace = squares.size() + 1;
	for (const LosLine& line: table) {
		neededSpace += line.size() * 4;
	}
	squares.reserve(neededSpace);

	SRectangle safeRect(0, 0, size.x, size.y);

	if (safeRect.Inside(pos)) {
		losHeight += heightmap[mapSquare]; //FIXME comment
		squares.push_back(mapSquare);
	}
	for (const LosLine& line: table) {
		float maxAng1 = minMaxAng;
		float maxAng2 = minMaxAng;
		float maxAng3 = minMaxAng;
		float maxAng4 = minMaxAng;
		float r = 1;

		for (const int2 square: line) {
			const float invR = math::isqrt2(square.x*square.x + square.y*square.y);

			if (safeRect.Inside(pos + square)) {
				CastLos(&squares, &maxAng1, MAP_SQUARE(pos + square),                    invR, heightmap, losHeight);
			}
			if (safeRect.Inside(pos - square)) {
				CastLos(&squares, &maxAng2, MAP_SQUARE(pos - square),                    invR, heightmap, losHeight);
			}
			if (safeRect.Inside(pos + int2(square.y, -square.x))) {
				CastLos(&squares, &maxAng3, MAP_SQUARE(pos + int2(square.y, -square.x)), invR, heightmap, losHeight);
			}
			if (safeRect.Inside(pos + int2(-square.y, square.x))) {
				CastLos(&squares, &maxAng4, MAP_SQUARE(pos + int2(-square.y, square.x)), invR, heightmap, losHeight);
			}

			r++;
		}
	}
}
