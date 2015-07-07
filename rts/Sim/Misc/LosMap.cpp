/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on original los code in LosHandler.{cpp,h} and RadarHandler.{cpp,h} */

#include "LosMap.h"
#include "Map/ReadMap.h"
#include "System/myMath.h"
#include "System/float3.h"

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
namespace {
//////////////////////////////////////////////////////////////////////


#define MAX_LOS_TABLE 110

typedef std::vector<int2> TPoints;
typedef std::vector<int2> LosLine;
typedef std::vector<LosLine> LosTable;


class CLosTables
{
public:
	static const LosTable& GetForLosSize(size_t losSize) {
		static CLosTables instance;
		const size_t tablenum = std::min<size_t>(MAX_LOS_TABLE, losSize);
		return instance.lostables[tablenum - 1];
	}

private:
	std::array<LosTable, MAX_LOS_TABLE> lostables;

	CLosTables();
	void DrawLine(char* PaintTable, int x,int y,int Size);
	LosLine OutputLine(int x,int y);
	void OutputTable(int table);
};


CLosTables::CLosTables()
{
	for (int a = 1; a <= MAX_LOS_TABLE; ++a) {
		OutputTable(a);
	}
}


struct int2_comparer
{
	bool operator () (const int2& a, const int2& b) const
	{
		if (a.x != b.x)
			return a.x < b.x;
		else
			return a.y < b.y;
	}
};


void CLosTables::OutputTable(int Table)
{
	TPoints Points;
	LosTable lostable;

	int Radius = Table;
	char* PaintTable = new char[(Radius+1)*Radius];
	memset(PaintTable, 0 , (Radius+1)*Radius);
	Points.emplace_back(0, Radius);

	for (float i=Radius; i>=1; i-=0.5f) {
		const int r2 = (int)(i * i);

		int x = 1;
		int y = (int) (math::sqrt((float)r2 - 1) + 0.5f);
		while (x < y) {
			if (!PaintTable[x+y*Radius]) {
				DrawLine(PaintTable, x, y, Radius);
				Points.emplace_back(x,y);
			}
			if (!PaintTable[y+x*Radius]) {
				DrawLine(PaintTable, y, x, Radius);
				Points.emplace_back(y,x);
			}

			x += 1;
			y = (int) (math::sqrt((float)r2 - x*x) + 0.5f);
		}
		if (x == y) {
			if (!PaintTable[x+y*Radius]) {
				DrawLine(PaintTable, x, y, Radius);
				Points.emplace_back(x,y);
			}
		}
	}

	std::sort(Points.begin(), Points.end(), int2_comparer());

	int size = Points.size();
	lostable.reserve(size);
	for (int j = 0; j < size; j++) {
		lostable.push_back(OutputLine(Points.back().x, Points.back().y));
		Points.pop_back();
	}

	lostables[Table - 1] = lostable;

	delete[] PaintTable;
}


LosLine CLosTables::OutputLine(int x, int y)
{
	LosLine losline;

	int x0 = 0;
	int y0 = 0;
	int dx = x;
	int dy = y;

	if (abs(dx) > abs(dy)) {                    // slope <1
		float m = (float) dy / (float) dx;      // compute slope
		float b = y0 - m*x0;
		dx = (dx < 0) ? -1 : 1;
		while (x0 != x) {
			x0 += dx;
			losline.push_back(int2(x0, Round(m*x0 + b)));
		}
	} else if (dy != 0) {                       // slope = 1
		float m = (float) dx / (float) dy;      // compute slope
		float b = x0 - m*y0;
		dy = (dy < 0) ? -1 : 1;
		while (y0 != y) {
			y0 += dy;
			losline.push_back(int2(Round(m*y0 + b), y0));
		}
	}
	return losline;
}


void CLosTables::DrawLine(char* PaintTable, int x, int y, int Size)
{
	int x0 = 0;
	int y0 = 0;
	int dx = x;
	int dy = y;

	if (abs(dx) > abs(dy)) {                    // slope <1
		float m = (float) dy / (float) dx;      // compute slope
		float b = y0 - m*x0;
		dx = (dx < 0) ? -1 : 1;
		while (x0 != x) {
			x0 += dx;
			PaintTable[x0+Round(m*x0 + b)*Size] = 1;
		}
	} else if (dy != 0) {                       // slope = 1
		float m = (float) dx / (float) dy;      // compute slope
		float b = x0 - m*y0;
		dy = (dy < 0) ? -1 : 1;
		while (y0 != y) {
			y0 += dy;
			PaintTable[Round(m*y0 + b)+y0*Size] = 1;
		}
	}
}


//////////////////////////////////////////////////////////////////////
} // end of anon namespace
//////////////////////////////////////////////////////////////////////


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
	losHeight += heightmap[mapSquare]; //FIXME comment

	squares.push_back(mapSquare);
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
