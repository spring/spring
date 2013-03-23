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


CR_BIND(CLosMap, );

CR_REG_METADATA(CLosMap, (
	CR_MEMBER(size),
	CR_MEMBER(map),
	CR_MEMBER(sendReadmapEvents)
));



CR_BIND(CLosAlgorithm, (int2(), 0.0f, 0.0f, NULL));

CR_REG_METADATA(CLosAlgorithm, (
	CR_MEMBER(size),
	CR_MEMBER(minMaxAng),
	CR_MEMBER(extraHeight)//,
	//CR_MEMBER(heightmap)
));



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
	static const int LOS2HEIGHT_X = gs->mapx / size.x;
	static const int LOS2HEIGHT_Z = gs->mapy / size.y;

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

			const SRectangle rect(lmx * LOS2HEIGHT_X, lmz * LOS2HEIGHT_Z, std::min(gs->mapxm1, (lmx + 1) * LOS2HEIGHT_X), std::min(gs->mapym1, (lmz + 1) * LOS2HEIGHT_Z));
			readmap->UpdateLOS(rect);
			#endif
		}
	}
}

void CLosMap::AddMapSquares(const std::vector<int>& squares, int allyteam, int amount)
{
	#ifdef USE_UNSYNCED_HEIGHTMAP
	static const int LOS2HEIGHT_X = gs->mapx / size.x;
	static const int LOS2HEIGHT_Z = gs->mapy / size.y;

	const bool updateUnsyncedHeightMap = (sendReadmapEvents && allyteam >= 0 && (allyteam == gu->myAllyTeam || gu->spectatingFullView));
	#endif

	std::vector<int>::const_iterator lsi;
	for (lsi = squares.begin(); lsi != squares.end(); ++lsi) {
		const int losMapSquareIdx = *lsi;
		#ifdef USE_UNSYNCED_HEIGHTMAP
		const bool squareEnteredLOS = (map[losMapSquareIdx] == 0 && amount > 0);
		#endif

		map[losMapSquareIdx] += amount;

		#ifdef USE_UNSYNCED_HEIGHTMAP
		if (!updateUnsyncedHeightMap) { continue; }
		if (!squareEnteredLOS) { continue; }

		const int lmx = losMapSquareIdx % size.x;
		const int lmz = losMapSquareIdx / size.x;
		const SRectangle rect(lmx * LOS2HEIGHT_X, lmz * LOS2HEIGHT_Z, std::min(gs->mapxm1, (lmx + 1) * LOS2HEIGHT_X), std::min(gs->mapym1, (lmz + 1) * LOS2HEIGHT_Z));
		readmap->UpdateLOS(rect);
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
	static const LosTable& GetForLosSize(int losSize) {
		static CLosTables instance;
		const int tablenum = std::min(MAX_LOS_TABLE, losSize);
		return instance.lostables[tablenum - 1];
	}

private:
	std::vector<LosTable> lostables;

	CLosTables();
	void DrawLine(char* PaintTable, int x,int y,int Size);
	LosLine OutputLine(int x,int y,int line);
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
	int2 P;

	int x, y, r2;

	P.x = 0;
	P.y = Radius;
	Points.push_back(P);
//  DrawLine(0, Radius, Radius);
	for(float i=Radius; i>=1; i-=0.5f) {
		r2 = (int)(i * i);

		x = 1;
		y = (int) (math::sqrt((float)r2 - 1) + 0.5f);
		while (x < y) {
			if(!PaintTable[x+y*Radius]) {
				DrawLine(PaintTable, x, y, Radius);
				P.x = x;
				P.y = y;
				Points.push_back(P);
			}
			if(!PaintTable[y+x*Radius]) {
				DrawLine(PaintTable, y, x, Radius);
				P.x = y;
				P.y = x;
				Points.push_back(P);
			}

			x += 1;
			y = (int) (math::sqrt((float)r2 - x*x) + 0.5f);
		}
		if (x == y) {
			if(!PaintTable[x+y*Radius]) {
				DrawLine(PaintTable, x, y, Radius);
				P.x = x;
				P.y = y;
				Points.push_back(P);
			}
		}
	}

	std::sort(Points.begin(), Points.end(), int2_comparer());

	int Line = 1;
	int Size = Points.size();
	for(int j=0; j<Size; j++) {
		lostable.push_back(OutputLine(Points.back().x, Points.back().y, Line));
		Points.pop_back();
		Line++;
	}

	lostables.push_back(lostable);

	delete[] PaintTable;
}


LosLine CLosTables::OutputLine(int x, int y, int Line)
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
}; // end of anon namespace
//////////////////////////////////////////////////////////////////////


void CLosAlgorithm::LosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares)
{
	if (radius <= 0) { return; }

	pos.x = Clamp(pos.x, 0, size.x - 1);
	pos.y = Clamp(pos.y, 0, size.y - 1);

	if ((pos.x - radius < radius) || (pos.x + radius >= size.x - radius) || // FIXME: This additional margin is due to a suspect bug in losalgorithm
	    (pos.y - radius < radius) || (pos.y + radius >= size.y - radius)) { // causing rare crash with big units such as arm Colossus
		SafeLosAdd(pos, radius, baseHeight, squares);
	} else {
		UnsafeLosAdd(pos, radius, baseHeight, squares);
	}
}


#define MAP_SQUARE(pos) \
	((pos).y * size.x + (pos).x)

#define LOS_ADD(_square, _maxAng) \
	{ \
		const int square = _square; \
		const float dh = heightmap[square] - baseHeight; \
		float ang = (dh + extraHeight) * invR; \
		if(ang > _maxAng) { \
			squares.push_back(square); \
			ang = dh * invR; \
			if(ang > _maxAng) _maxAng = ang; \
		} \
	}


void CLosAlgorithm::UnsafeLosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares)
{
	const int mapSquare = MAP_SQUARE(pos);
	const LosTable& table = CLosTables::GetForLosSize(radius);

	baseHeight += heightmap[mapSquare];

	size_t neededSpace = squares.size() + 1;
	for(LosTable::const_iterator li = table.begin(); li != table.end(); ++li) {
		neededSpace += li->size() * 4;
	}
	squares.reserve(neededSpace);

	squares.push_back(mapSquare);

	for(LosTable::const_iterator li = table.begin(); li != table.end(); ++li) {
		const LosLine& line = *li;
		float maxAng1 = minMaxAng;
		float maxAng2 = minMaxAng;
		float maxAng3 = minMaxAng;
		float maxAng4 = minMaxAng;
		float r = 1;

		for(LosLine::const_iterator linei = line.begin(); linei != line.end(); ++linei) {
			const float invR = 1.0f / r;

			LOS_ADD(mapSquare + linei->x + linei->y * size.x, maxAng1);
			LOS_ADD(mapSquare - linei->x - linei->y * size.x, maxAng2);
			LOS_ADD(mapSquare - linei->x * size.x + linei->y, maxAng3);
			LOS_ADD(mapSquare + linei->x * size.x - linei->y, maxAng4);

			r++;
		}
	}
}


void CLosAlgorithm::SafeLosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares)
{
	const int mapSquare = MAP_SQUARE(pos);
	const LosTable& table = CLosTables::GetForLosSize(radius);

	baseHeight += heightmap[mapSquare];

	squares.push_back(mapSquare);

	for (LosTable::const_iterator li = table.begin(); li != table.end(); ++li) {
		const LosLine& line = *li;
		float maxAng1 = minMaxAng;
		float maxAng2 = minMaxAng;
		float maxAng3 = minMaxAng;
		float maxAng4 = minMaxAng;
		float r = 1;

		for(LosLine::const_iterator linei = line.begin(); linei != line.end(); ++linei) {
			const float invR = 1.0f / r;

			if ((pos.x + linei->x < size.x) && (pos.y + linei->y < size.y)) {
				LOS_ADD(mapSquare + linei->x + linei->y * size.x, maxAng1);
			}
			if ((pos.x - linei->x >= 0) && (pos.y - linei->y >= 0)) {
				LOS_ADD(mapSquare - linei->x - linei->y * size.x, maxAng2);
			}
			if ((pos.x + linei->y < size.x) && (pos.y - linei->x >= 0)) {
				LOS_ADD(mapSquare - linei->x * size.x + linei->y, maxAng3);
			}
			if ((pos.x - linei->y >= 0) && (pos.y + linei->x < size.y)) {
				LOS_ADD(mapSquare + linei->x * size.x - linei->y, maxAng4);
			}

			r++;
		}
	}
}
